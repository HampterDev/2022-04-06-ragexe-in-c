#include "contents/lua_script.h"
#include "fileio/file.h"
#include <lauxlib.h>
#include <lualib.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct GameFileMgr g_fileMgr;
static const char *file_text;
static void *file_allocation;
static unsigned char *file_bytes;
static int file_queries, file_releases, file_missing, dialogs, executions;
static int fail_state, freed_owners;
static char last_message[512], last_caption[256];
void client_lua_loadfile_hook(const char *name) { (void)name; abort(); }

BYTE *game_file_mgr_get_data(struct GameFileMgr *mgr, const char *path, int *size, BYTE disk_only)
{
    DWORD old;
    SYSTEM_INFO info;
    assert(mgr == &g_fileMgr && !disk_only && !file_allocation);
    assert(!strncmp(path, "data\\", 5));
    ++file_queries;
    if (file_missing) return NULL;
    GetSystemInfo(&info);
    *size = (int)strlen(file_text);
    assert((unsigned)*size < info.dwPageSize);
    file_allocation = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(file_allocation);
    assert(VirtualProtect((char *)file_allocation + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old));
    file_bytes = (unsigned char *)file_allocation + info.dwPageSize - *size;
    if (*size) memcpy(file_bytes, file_text, *size);
    return file_bytes;
}

void game_file_mgr_release_data(struct GameFileMgr *mgr, void *data)
{
    assert(mgr == &g_fileMgr && data == file_bytes && file_allocation);
    assert(VirtualFree(file_allocation, 0, MEM_RELEASE));
    file_allocation = NULL;
    file_bytes = NULL;
    ++file_releases;
}

BYTE game_file_mgr_is_file_exist(struct GameFileMgr *mgr, const char *path, BYTE disk_only)
{ (void)mgr; (void)path; (void)disk_only; abort(); }
void error_msg(const char *text, int flags) { (void)text; (void)flags; abort(); }

static int message_box(HWND window, const char *text, const char *caption, UINT flags)
{
    assert(!window && !flags && text && caption);
    assert(strlen(text) < sizeof last_message && strlen(caption) < sizeof last_caption);
    strcpy(last_message, text);
    strcpy(last_caption, caption);
    ++dialogs;
    return 0;
}

static lua_State *new_state(void) { return fail_state ? NULL : luaL_newstate(); }
static void free_owner(void *owner) { ++freed_owners; free(owner); }
#define MessageBoxA message_box
#define luaL_newstate new_state
#include "../src/core/lua_instance.c"
#undef luaL_newstate
#undef MessageBoxA

static int observe(lua_State *state)
{
    assert(!file_allocation);
    ++executions;
    lua_pushinteger(state, 42);
    return 1;
}

static void retained_stack(lua_State *state)
{
    assert(lua_gettop(state) == 2);
    assert(lua_tointeger(state, 1) == 123);
    assert(!strcmp(lua_tostring(state, 2), "sentinel"));
}

static void push_sentinels(lua_State *state)
{
    lua_pushinteger(state, 123);
    lua_pushliteral(state, "sentinel");
}

static void script_behavior(void)
{
    struct LuaScript script;
    char number_name[] = "number";
    int before, i;
    assert(lua_script_construct(&script) == &script && script.L);
    push_sentinels(script.L);
    lua_script_set_number(&script, number_name, -12.5);
    memset(number_name, 'x', sizeof number_name - 1);
    lua_script_set_boolean(&script, "enabled", -1);
    lua_script_set_boolean(&script, "disabled", 0);
    lua_script_register(&script, "observe", observe);
    retained_stack(script.L);
    file_text = "assert(number==-12.5 and enabled and not disabled); assert(observe()==42)";
    for (i = 0; i < 100; ++i) {
        assert(lua_script_run_file(&script, "Lua Files\\테스트.lua"));
        retained_stack(script.L);
        assert(!file_allocation);
    }
    assert(executions == 100 && !dialogs);
    file_text = "";
    assert(lua_script_run_file(&script, "empty.lua"));
    retained_stack(script.L);
    file_text = "this is not valid Lua";
    assert(!lua_script_run_file(&script, "syntax.lua"));
    assert(strstr(last_message, "expected") && !strcmp(last_caption, "syntax.lua"));
    retained_stack(script.L);
    file_text = "error('intentional failure')";
    assert(!lua_script_run_file(&script, "runtime.lua"));
    assert(strstr(last_message, "intentional failure"));
    retained_stack(script.L);
    file_text = "error({})";
    assert(!lua_script_run_file(&script, "object.lua"));
    assert(!strcmp(last_message, "Lua script failed"));
    retained_stack(script.L);
    file_text = "MessageBox('테스트'); MessageBox(nil)";
    assert(lua_script_run_file(&script, "message.lua"));
    assert(!strcmp(last_message, "") && !strcmp(last_caption, "CLuaInstance"));
    retained_stack(script.L);
    before = file_releases;
    file_missing = 1;
    assert(!lua_script_run_file(&script, "missing.lua"));
    assert(file_releases == before && !file_allocation);
    file_missing = 0;
    retained_stack(script.L);
    before = file_queries;
    assert(!lua_script_run_file(NULL, "none.lua"));
    assert(!lua_script_run_file(&script, NULL));
    assert(file_queries == before);
    lua_script_destruct(&script);
    assert(!script.L && !freed_owners);
    lua_script_destruct(&script);
    assert(!lua_script_run_file(&script, "closed.lua") && file_queries == before);

    fail_state = 1;
    lua_script_construct(&script);
    assert(!script.L);
    lua_script_destruct(&script);
    fail_state = 0;
    {
        struct LuaScript *heap = malloc(sizeof *heap);
        assert(heap);
        lua_script_construct(heap);
        lua_script_destruct(heap);
        free_owner(heap);
        assert(freed_owners == 1);
    }
}

struct Allocator { size_t live; unsigned attempts, fail_at; };
static void *lua_memory(void *context, void *pointer, size_t old_size, size_t size)
{
    struct Allocator *allocator = context;
    void *result;
    if (!pointer) old_size = 0;
    if (!size) {
        free(pointer);
        assert(allocator->live >= old_size);
        allocator->live -= old_size;
        return NULL;
    }
    if (++allocator->attempts == allocator->fail_at) return NULL;
    result = realloc(pointer, size);
    if (result) allocator->live = allocator->live - old_size + size;
    return result;
}

static unsigned allocation_case(unsigned fail_at)
{
    struct Allocator allocator = {0};
    struct LuaScript script = {lua_newstate(lua_memory, &allocator)};
    unsigned attempts;
    int released = file_releases, ok, before = executions;
    assert(script.L);
    lua_script_register(&script, "observe", observe);
    push_sentinels(script.L);
    allocator.attempts = 0;
    allocator.fail_at = fail_at;
    file_text = "local names={'alpha','beta','gamma'}; local function f(x) return x..'delta' end; observe()";
    ok = lua_script_run_file(&script, "allocation.lua");
    attempts = allocator.attempts;
    allocator.fail_at = 0;
    retained_stack(script.L);
    assert(!file_allocation && file_releases == released + 1);
    assert(executions == before + (ok != 0));
    lua_close(script.L);
    assert(!allocator.live);
    return attempts;
}

int main(void)
{
    unsigned allocations, i;
    script_behavior();
    allocations = allocation_case(0);
    assert(allocations > 10);
    for (i = 1; i <= allocations; ++i) allocation_case(i);
    printf("Lua scripts: borrowed names, guarded file spans, stack ownership, errors, lifecycle and %u allocation failure points passed\n", allocations);
    return 0;
}
