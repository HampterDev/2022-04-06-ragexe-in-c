#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "contents/achievement.h"
#include "contents/game_session.h"
#include "std/integer_map.h"
#include <lauxlib.h>
#undef malloc
#undef free
#undef luaL_newstate
#undef luaL_loadfile
#undef MessageBoxA

static struct Block { unsigned char *raw; size_t size; } blocks[2048];
static unsigned live_blocks, allocation_count, message_count, load_count;
static size_t lua_bytes;
static int fail_manager, fail_state, script_mode;
static unsigned trace_hash = 2166136261u;
static FILE *trace;
unsigned g_codePage = 949;
struct Session g_session1;

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

#ifdef ACHIEVEMENT_OWNER_REFERENCE
struct AchievementMgr *g_achievementDb;
#endif

struct SessionAchievementState *session_get_achievement(void *session,
    struct SessionAchievementState *out, int id)
{
    assert(session == &g_session1);
    memset(out, 0, sizeof *out);
    out->completed = id == 101 ? 2 : id >= 102 && id <= 112 && id % 2 == 0;
    return out;
}

enum { SCRIPT_VALID, SCRIPT_MISSING, SCRIPT_BAD_CHUNK, SCRIPT_NO_MAIN, SCRIPT_FALSE_RESULT };
static const char achievement_script[] =
    "function main() "
    "assert(not InsertAchieveInfo('bad','a','b','c',1)); "
    "assert(not InsertAchieveResource(999,'missing',1,0)); "
    "assert(not InsertAchieveRewardItem(999,1)); "
    "assert(not InsertAchieveRewardTitle(999,1)); "
    "assert(not InsertAchieveRewardBuff(999,1)); "
    "assert(not InsertAchieveUIType(999,1,'missing')); "
    "assert(not SetAchieveIDByTab('bad',1,0)); "
    "for id=101,112 do "
    "assert(InsertAchieveInfo(id,'한국어 업적 제목 long title '..id,'description '..id,'summary '..id,id*10)); "
    "assert(InsertAchieveUIType(id,id%2,'유저인터페이스\\\\achievement\\\\icon'..id..'.bmp')); "
    "assert(InsertAchieveRewardItem(id,id==108 and 0 or id+500)); "
    "assert(InsertAchieveRewardTitle(id,id==108 and 0 or id+900)); "
    "assert(InsertAchieveRewardBuff(id,id==108 and 0 or id+42)); "
    "for j=1,8 do assert(InsertAchieveResource(id,'조건 long resource text '..j,j*2,100+j)) end "
    "end "
    "assert(not InsertAchieveInfo(101,'duplicate','','',0)); "
    "assert(not InsertAchieveResource(101,'조건 long resource text 1',99,99)); "
    "assert(SetAchieveIDByTab(101,3,0)); assert(SetAchieveIDByTab(102,3,0)); "
    "assert(not SetAchieveIDByTab(101,3,0)); assert(SetAchieveIDByTab(103,3,1)); "
    "assert(SetAchieveIDByTab(101,4,0)); assert(SetAchieveIDByTab(101,4,1)); "
    "assert(SetAchieveIDByTab(999,5,0)); return true,'good' end";

int test_lua_loadfile(lua_State *L, const char *filename)
{
    const char *source = achievement_script;
    char failed_script[4096];
    int length;
    assert(!strcmp(filename, "system\\Achievement_list.lub"));
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

int WINAPI test_message_box(HWND window, LPCSTR message, LPCSTR caption, UINT flags)
{
    assert(!window && !flags && message && !strcmp(caption, "CAchievementMgr"));
    ++message_count;
    return IDOK;
}

static void emit(const void *data, size_t size)
{
    const unsigned char *bytes = data;
    size_t i;
    for (i = 0; i < size; ++i) { trace_hash ^= bytes[i]; trace_hash *= 16777619u; }
    if (trace) assert(fwrite(data, 1, size, trace) == size);
}

static void emit_text(const struct String *s)
{
    emit(string_cstr(s), s->size + 1);
}

static void emit_info(const struct AchievementInfo *info)
{
    const struct AchievementResource *resource;
    unsigned i;
    for (i = 0; i < 3; ++i) emit_text(&info->text[i]);
    emit(&info->info_number, sizeof info->info_number);
    emit(&info->reward_item, sizeof info->reward_item);
    emit(&info->reward_title, sizeof info->reward_title);
    emit(&info->reward_buff, sizeof info->reward_buff);
    emit(&info->ui_type, sizeof info->ui_type);
    emit_text(&info->ui_text);
    for (resource = info->resources.first; resource != info->resources.last; ++resource) {
        emit_text(&resource->text);
        emit(&resource->target, sizeof resource->target);
        emit(&resource->goto_id, sizeof resource->goto_id);
    }
}

static void empty(void)
{
    assert(!g_achievementDb && !live_blocks && !lua_bytes);
}

static void destroy(void)
{
    achievement_destroy();
    empty();
    achievement_destroy();
    empty();
}

static int default_fields(void)
{
    struct AchievementInfo info;
    int ok;
    memset(&info, 0xa5, sizeof info);
    achievement_info_init(&info);
    ok = info.info_number == 0 && info.ui_type == 0;
    printf("Default achievement fields: points=%d, UI type=%d.\n", info.info_number, info.ui_type);
    assert(info.reward_item == -1 && info.reward_title == -1 && info.reward_buff == -1);
    assert(!info.resources.first && !info.resources.last && !info.resources.end);
    achievement_info_destruct(&info);
    achievement_info_destruct(&info);
    empty();
    return ok;
}

static void populated(void)
{
    struct AchievementInfo info, copy, moved;
    struct String s;
    struct Tree map;
    struct TreeNode *node;
    struct AchievementInfoEntry *found;
    struct AchievementResource *resource;
    unsigned before, i;
    int id, major, minor, key;
    script_mode = SCRIPT_VALID;
    message_count = 0;
    assert(achievement_create() && g_achievementDb && !lua_bytes && !message_count);
    before = allocation_count;
    assert(!achievement_create() && allocation_count == before);
    assert(achievement_tab_count(g_achievementDb, 0, -1) == 12);
    assert(achievement_tab_count_flagged(g_achievementDb, 0, -1) == 6);
    achievement_info_init(&info);
    for (id = 101; id <= 112; ++id) {
        char expected[128], expected_icon[128];
        assert(achievement_get_info(g_achievementDb, &info, id));
        snprintf(expected, sizeof expected, "한국어 업적 제목 long title %d", id);
        assert(!strcmp(string_cstr(&info.text[0]), expected));
        snprintf(expected_icon, sizeof expected_icon, "유저인터페이스\\achievement\\icon%d.bmp", id);
        assert(!strcmp(string_cstr(&info.ui_text), expected_icon));
        assert(info.info_number == id * 10 && info.ui_type == id % 2);
        assert(info.reward_item == (id == 108 ? 0 : id + 500));
        assert(info.reward_title == (id == 108 ? 0 : id + 900));
        assert(info.reward_buff == (id == 108 ? 0 : id + 42));
        assert(achievement_info_number(g_achievementDb, id) == id * 10);
        assert(achievement_reward_title(g_achievementDb, id) == info.reward_title);
        assert(achievement_has_reward(g_achievementDb, id) == (id != 108));
        assert(VEC_COUNT(&info.resources, struct AchievementResource) == 8);
        for (i = 0; i < 8; ++i) {
            resource = (struct AchievementResource *)info.resources.first + i;
            assert(resource->target == (int)(i + 1) * 2 && resource->goto_id == (int)i + 101);
        }
        emit_info(&info);
        achievement_text1(g_achievementDb, &s, id); assert(!strcmp(string_cstr(&s), string_cstr(&info.text[1]))); string_free(&s);
        achievement_ui_text(g_achievementDb, &s, id); assert(!strcmp(string_cstr(&s), string_cstr(&info.ui_text))); string_free(&s);
    }
    before = live_blocks;
    achievement_info_assign(&info, &info);
    assert(live_blocks == before);
    assert(!achievement_get_info(g_achievementDb, &info, 998) && info.info_number == 1120);
    assert(achievement_info_number(g_achievementDb, 998) == -1 && achievement_reward_title(g_achievementDb, 998) == -1);
    assert(!achievement_has_reward(g_achievementDb, 998));
    achievement_text1(g_achievementDb, &s, 998); assert(!s.size); string_free(&s);
    achievement_ui_text(g_achievementDb, &s, 998); assert(!s.size); string_free(&s);
    assert(achievement_tab_count(g_achievementDb, 0, -1) == 12);

    achievement_info_copy(&copy, &info);
    assert(copy.resources.first != info.resources.first);
    string_assign_cstr(&copy.text[0], "changed");
    ((struct AchievementResource *)copy.resources.first)->target = 999;
    assert(strcmp(string_cstr(&info.text[0]), "changed"));
    assert(((struct AchievementResource *)info.resources.first)->target == 2);
    moved = copy;
    achievement_info_init(&copy);
    assert(!copy.text[0].size && !copy.ui_text.size && !copy.resources.first);
    achievement_info_destruct(&copy);
    achievement_info_assign(&moved, &info);
    emit_info(&moved);
    achievement_info_destruct(&moved);

    tree_init(&map);
    assert(achievement_collect_tab(g_achievementDb, &map, 3, -1) && map.size == 3);
    for (node = tree_first(&map); node != map.head; node = tree_next(node)) {
        struct AchievementInfoEntry *entry = TREE_VALUE(node, struct AchievementInfoEntry);
        emit(&entry->id, sizeof entry->id); emit_info(&entry->info);
        string_assign_cstr(&entry->info.text[0], "changed");
    }
    assert(achievement_tab_count(g_achievementDb, 3, -1) == 3);
    assert(achievement_tab_count(g_achievementDb, 3, 0) == 2);
    assert(achievement_tab_count_flagged(g_achievementDb, 3, -1) == 1);
    assert(achievement_tab_count_flagged(g_achievementDb, 3, 0) == 1);
    assert(achievement_find_tab(g_achievementDb, 101, &major, &minor) && major == 3 && minor == 0);
    major = minor = -7;
    assert(!achievement_find_tab(g_achievementDb, 998, &major, &minor) && major == -7 && minor == -7);
    assert(!achievement_collect_tab(g_achievementDb, &map, 4, -1) && map.size == 1);
    assert(achievement_collect_tab(g_achievementDb, &map, 5, 0) && map.size == 1);
    assert(achievement_get_info(g_achievementDb, &copy, 998) == 0);
    assert(achievement_tab_count(g_achievementDb, 0, -1) == 13);
    key = 999;
    found = tree_find(&map, &key, tree_less_int);
    assert(found && !found->info.text[0].size);
    assert(found->info.reward_item == -1);
#ifndef ACHIEVEMENT_OWNER_REFERENCE
    assert(found->info.info_number == 0 && found->info.ui_type == 0);
#endif
    assert(achievement_collect_tab(g_achievementDb, &map, 6, -1) && !map.size);
    for (i = 0; i < 97; ++i) {
        key = (int)(i * 17 % 97);
        achievement_info_assign(achievement_at(&map, key), &info);
        found = tree_find(&map, &key, tree_less_int);
        assert(achievement_at(&map, key) == &found->info);
    }
    assert(map.size == 97);
    for (key = 0; key < 98; ++key) {
        assert(!tree_find(&map, &key, tree_less_int) == (key == 97));
        emit(&key, sizeof key);
    }
    tree_free(&map, achievement_info_entry_destroy);
    achievement_destroy();
    assert(!g_achievementDb && !lua_bytes);
    assert(info.info_number == 1120 && info.ui_type == 0);
    emit_info(&info);
    achievement_info_destruct(&info);
    achievement_info_destruct(&info);
    destroy();
}

static void failed_loads(void)
{
    unsigned before;
    for (script_mode = SCRIPT_MISSING; script_mode <= SCRIPT_FALSE_RESULT; ++script_mode) {
        message_count = 0;
        assert(!achievement_create() && message_count == 1);
        empty(); destroy();
    }
    fail_state = 1; message_count = 0; before = load_count;
    assert(!achievement_create() && !message_count && load_count == before);
    empty(); fail_state = 0;
    fail_manager = 1; before = allocation_count;
    assert(!achievement_create() && allocation_count == before);
    empty(); fail_manager = 0;
}

int main(int argc, char **argv)
{
    int i;
    if (argc == 2 && !strcmp(argv[1], "--defaults-only")) return default_fields() ? 0 : 2;
    if (argc == 2) { trace = fopen(argv[1], "wb"); assert(trace); }
    destroy();
    for (i = 0; i < 3; ++i) populated();
    failed_loads(); populated();
#ifndef ACHIEVEMENT_OWNER_REFERENCE
    assert(default_fields());
#endif
    if (trace) assert(fclose(trace) == 0);
    assert(trace_hash == 0x79b6d165u);
    printf("Achievement owner: real Lua, values, tabs, rewards, copies and failure cleanup passed; digest %08x.\n", trace_hash);
    return 0;
}

#include "achievement_owner_unreached.h"
