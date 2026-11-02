#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "contents/town_info.h"
#include "std/integer_map.h"
#include <lauxlib.h>
#undef malloc
#undef free
#undef luaL_newstate
#undef luaL_loadfile
#undef MessageBoxA
#undef platform_message_box

static struct Block { unsigned char *raw; size_t size; } blocks[2048];
static unsigned live_blocks, allocation_count, message_count, load_count;
static size_t lua_bytes;
static int fail_manager, fail_state, script_mode;
static unsigned trace_hash = 2166136261u;
static FILE *trace;
unsigned g_codePage = 949;

void *test_heap_alloc(size_t size)
{
    unsigned i;
    for (i = 0; i < 2048 && blocks[i].raw; ++i) {}
    assert(i < 2048);
    blocks[i].raw = malloc(size + 32);
    assert(blocks[i].raw);
    blocks[i].size = size;
    memset(blocks[i].raw, 0xa5, size + 32);
    ++live_blocks;
    ++allocation_count;
    return blocks[i].raw + 16;
}

void test_heap_free(void *memory)
{
    unsigned i, j;
    if (!memory) return;
    for (i = 0; i < 2048 && (!blocks[i].raw || blocks[i].raw + 16 != memory); ++i) {}
    assert(i < 2048 && live_blocks);
    for (j = 0; j < 16; ++j) {
        assert(blocks[i].raw[j] == 0xa5);
        assert(blocks[i].raw[16 + blocks[i].size + j] == 0xa5);
    }
    free(blocks[i].raw);
    blocks[i].raw = NULL;
    --live_blocks;
}

void *mem_alloc(size_t size) { return test_heap_alloc(size); }
void *mem_try_alloc(size_t size) { return fail_manager ? NULL : test_heap_alloc(size); }
void mem_free(void *memory) { test_heap_free(memory); }
void fatal_error(const char *message) { (void)message; abort(); }
void error_msg(const char *message, int flags) { (void)flags; fprintf(stderr, "%s\n", message); abort(); }

static void *lua_memory(void *context, void *memory, size_t old_size, size_t size)
{
    void *result;
    (void)context;
    if (!memory) old_size = 0;
    assert(lua_bytes >= old_size);
    if (!size) {
        free(memory);
        lua_bytes -= old_size;
        return NULL;
    }
    result = realloc(memory, size);
    assert(result);
    lua_bytes = lua_bytes - old_size + size;
    return result;
}

lua_State *test_lua_newstate(void) { return fail_state ? NULL : lua_newstate(lua_memory, NULL); }

const char *thread_text_format(const char *format, ...)
{
    static char buffer[2048];
    va_list args;
    int length;
    va_start(args, format);
    length = vsnprintf(buffer, sizeof buffer, format, args);
    va_end(args);
    assert(length >= 0 && (size_t)length < sizeof buffer);
    return buffer;
}

#ifdef TOWN_INFO_REFERENCE
struct TownInfoMgr *g_townInfo;
#endif

enum { SCRIPT_VALID, SCRIPT_MISSING, SCRIPT_BAD_CHUNK, SCRIPT_NO_MAIN, SCRIPT_FALSE_RESULT };
static const char town_script[] =
    "function main() "
    "assert(not AddTownInfo('a','b',1,2,'bad')); "
    "assert(not AddTownInfo('a','b',1,'bad',0)); "
    "assert(not AddTownInfo('a','b','bad',2,0)); "
    "assert(not AddTownInfo('a',false,1,2,0)); "
    "assert(not AddTownInfo(false,'b',1,2,0)); "
    "for kind=0,9 do assert(AddTownInfo('prontera','한국어 시설 이름 long facility name',kind+10,kind+20,kind)) end "
    "assert(AddTownInfo('z_town','short',1,2,0)); "
    "assert(AddTownInfo('a_town','',-1,-2,1)); "
    "assert(AddTownInfo('한국어_긴_지도_이름_123456','another facility',30,40,9)); "
    "return true,'good' end";

int test_lua_loadfile(lua_State *L, const char *filename)
{
    const char *source = town_script;
    char failed_script[2048];
    int length;
    assert(!strcmp(filename, "System\\Towninfo.lub"));
    ++load_count;
    if (script_mode == SCRIPT_MISSING) {
        lua_pushliteral(L, "fixture: missing file");
        return LUA_ERRFILE;
    }
    if (script_mode == SCRIPT_BAD_CHUNK) return luaL_loadstring(L, "this is not Lua!");
    if (script_mode == SCRIPT_NO_MAIN) return luaL_loadstring(L, "loaded=true");
    if (script_mode == SCRIPT_FALSE_RESULT) {
        length = snprintf(failed_script, sizeof failed_script,
            "%s local populate=main function main() populate() return false,'fixture failure' end", source);
        assert(length >= 0 && (size_t)length < sizeof failed_script);
        source = failed_script;
    }
    return luaL_loadstring(L, source);
}

void test_message_box(HWND window, const char *message, const char *caption, unsigned flags)
{
    assert(!window && !flags && message && !strcmp(caption, "CTownInfoMgr"));
    ++message_count;
}

static void emit_bytes(const void *data, size_t size)
{
    const unsigned char *bytes = data;
    size_t i;
    for (i = 0; i < size; ++i) {
        trace_hash ^= bytes[i];
        trace_hash *= 16777619u;
    }
    if (trace) assert(fwrite(data, 1, size, trace) == size);
}

static void emit_info(const struct TownInfo *info)
{
    emit_bytes(&info->kind, sizeof info->kind);
    emit_bytes(&info->x, sizeof info->x);
    emit_bytes(&info->y, sizeof info->y);
    emit_bytes(string_cstr(&info->name), info->name.size + 1);
    emit_bytes(string_cstr(&info->icon), info->icon.size + 1);
}

static struct String text(const char *value)
{
    struct String s;
    string_init(&s);
    string_assign_cstr(&s, value);
    return s;
}

static void release_info(struct TownInfo *info)
{
    string_free(&info->icon);
    string_free(&info->name);
}

static unsigned persistent_blocks(void)
{
#ifdef TOWN_INFO_REFERENCE
    return g_town_icon_paths[0].size ? 10 : 0;
#else
    return 0;
#endif
}

static void destroy(void)
{
    town_info_destroy();
    assert(!g_townInfo && live_blocks == persistent_blocks() && !lua_bytes);
    town_info_destroy();
    assert(live_blocks == persistent_blocks());
}

static void missing(const char *map, unsigned index)
{
    struct TownInfo info;
    assert(town_info_get(g_townInfo, &info, text(map), index) == &info);
    assert(info.kind == -1 && info.x == -1 && info.y == -1);
    assert(!info.name.size && !info.icon.size);
    emit_info(&info);
    release_info(&info);
}

static void populated(void)
{
    static const char *const icons[] = {
        "Information\\Store.bmp", "Information\\Weaponshop.bmp", "Information\\ArmorShops.bmp",
        "Information\\Smithy.bmp", "Information\\Guide.bmp", "Information\\Inn.bmp",
        "Information\\kafra.bmp", "Information\\style.bmp", "Information\\warp.bmp", "minimap\\Quest.bmp"
    };
    struct TownInfo info, copy;
    unsigned i, before, loads;
    char expected[128];
    script_mode = SCRIPT_VALID;
    message_count = 0;
    loads = load_count;
    assert(town_info_create() && g_townInfo);
    assert(load_count == loads);
    before = allocation_count;
    assert(!town_info_create() && allocation_count == before);
    missing("prontera.rsw", 0);
    assert(town_info_load() && !lua_bytes && !message_count);
    for (i = 0; i < 10; ++i) {
        assert(town_info_get(g_townInfo, &info, text("prontera.rsw.backup"), i) == &info);
        assert(info.kind == (int)i && info.x == (int)i + 10 && info.y == (int)i + 20);
        assert(!strcmp(string_cstr(&info.name), "한국어 시설 이름 long facility name"));
        snprintf(expected, sizeof expected, "유저인터페이스\\%s", icons[i]);
        assert(!strcmp(string_cstr(&info.icon), expected));
        emit_info(&info);
        string_assign_cstr(&info.name, "changed");
        string_assign_cstr(&info.icon, "changed");
        town_info_get(g_townInfo, &copy, text("prontera"), i);
        assert(strcmp(string_cstr(&copy.name), "changed"));
        assert(!strcmp(string_cstr(&copy.icon), expected));
        release_info(&copy);
        release_info(&info);
    }
    missing("prontera", 10);
    missing("prontera", ~0u);
    missing("missing.rsw", 0);
    missing("", 0);
    town_info_get(g_townInfo, &info, text("a_town.rsw"), 0);
    assert(info.kind == 1 && info.x == -1 && info.y == -2 && !info.name.size);
    emit_info(&info);
    release_info(&info);
    town_info_get(g_townInfo, &info, text("한국어_긴_지도_이름_123456.rsw"), 0);
    assert(info.kind == 9 && info.x == 30 && info.y == 40);
    emit_info(&info);
    release_info(&info);
    assert(town_info_load());
    town_info_get(g_townInfo, &info, text("prontera"), 19);
    assert(info.kind == 9 && info.x == 19 && info.y == 29);
    emit_info(&info);
    town_info_destroy();
    assert(!g_townInfo && live_blocks == persistent_blocks() + 2 && !lua_bytes);
    assert(!strcmp(string_cstr(&info.name), "한국어 시설 이름 long facility name"));
    release_info(&info);
    destroy();
}

static void failed_loads(void)
{
    struct TownInfo info;
    unsigned before;
    for (script_mode = SCRIPT_MISSING; script_mode <= SCRIPT_FALSE_RESULT; ++script_mode) {
        message_count = 0;
        assert(town_info_create());
        assert(!town_info_load() && message_count == 1 && !lua_bytes);
        if (script_mode == SCRIPT_FALSE_RESULT) {
            town_info_get(g_townInfo, &info, text("prontera"), 9);
            assert(info.kind == 9);
            emit_info(&info);
            release_info(&info);
        } else {
            missing("prontera", 0);
        }
        destroy();
    }
    fail_state = 1;
    message_count = 0;
    assert(town_info_create());
    before = load_count;
    assert(!town_info_load() && !message_count && load_count == before);
    destroy();
    fail_state = 0;
    fail_manager = 1;
    before = allocation_count;
    assert(!town_info_create() && allocation_count == before);
    destroy();
    fail_manager = 0;
}

int main(int argc, char **argv)
{
    int i;
    if (argc == 2) { trace = fopen(argv[1], "wb"); assert(trace); }
    destroy();
    for (i = 0; i < 3; ++i) populated();
    failed_loads();
    populated();
#ifdef TOWN_INFO_REFERENCE
    town_icon_paths_destroy();
#endif
    assert(!live_blocks && !lua_bytes && !g_townInfo);
    if (trace) assert(fclose(trace) == 0);
    assert(trace_hash == 0x762fe954u);
    printf("Town info: real Lua, all icons, owned queries, failures and cleanup passed; digest %08x.\n", trace_hash);
    return 0;
}
