#include "contents/client_lua.h"
#include "contents/item_db_to_id.h"
#include "fileio/file.h"
#include "session/state.h"
#include "session/service_address.h"
#include "platform/client_config.h"
#include "std/std.h"
#include <lauxlib.h>
#include <lualib.h>
#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world_state_memory.h"

struct GameFileMgr g_fileMgr;
const char *g_serverAddress = "server", *g_serverPort = "6900", *g_serverDomain;
static const char *source_text;
static int source_missing, exists_lua = 1, exists_lub, queries, releases, executions;
static void *source_allocation;
static unsigned char *source_bytes;
static char checked_paths[2][256], loaded_path[256], last_error[500];
static unsigned char last_disk_only;
static int dialogs, registration_error, registrations;
static long owned_attempts, owned_fail = -1, owned_live;

struct LuaMemory { long attempts, fail_at, live; };
static struct LuaMemory lua_memory = {0, -1, 0};
static void *lua_allocator(void *context, void *pointer, size_t old_size, size_t size)
{
    struct LuaMemory *memory = context;
    void *result;
    if (!pointer) old_size = 0;
    if (!size) {
        if (pointer) { free(pointer); --memory->live; }
        return NULL;
    }
    if (size > old_size && memory->attempts++ == memory->fail_at) return NULL;
    result = realloc(pointer, size);
    assert(result);
    if (!pointer) ++memory->live;
    return result;
}

static lua_State *new_state(void) { return lua_newstate(lua_allocator, &lua_memory); }
static void *owned_malloc(size_t size)
{
    void *memory;
    if (owned_attempts++ == owned_fail) return NULL;
    memory = malloc(size);
    assert(memory);
    ++owned_live;
    return memory;
}
static void owned_free(void *memory)
{
    if (memory) { assert(owned_live > 0); --owned_live; free(memory); }
}

static int message_box(HWND window, const char *text, const char *caption, UINT flags)
{
    assert(!window && !flags && text && caption && strlen(text) < sizeof last_error);
    strcpy(last_error, text);
    ++dialogs;
    return 0;
}

void client_lua_loadfile_hook(const char *name) { (void)name; abort(); }

#define malloc owned_malloc
#define free owned_free
#define luaL_newstate new_state
#include "../src/contents/client_lua.c"
#undef luaL_newstate
#include "../src/contents/client_lua_globals.c"
#include "../src/core/owned_text.c"
#include "../src/session/service_address.c"
#undef free
#undef malloc
#define MessageBoxA message_box
#include "../src/contents/lua_calls.c"
#undef MessageBoxA

void *mem_try_alloc(size_t bytes) { (void)bytes; abort(); }
struct LuaScript *lua_script_construct(struct LuaScript *self) { (void)self; abort(); }
void lua_script_destruct(struct LuaScript *self) { (void)self; abort(); }
char lua_script_run_file(struct LuaScript *self, const char *path) { (void)self; (void)path; abort(); }

static int observe(lua_State *state)
{
    assert(!source_allocation);
    ++executions;
    lua_pushinteger(state, 1);
    return 1;
}
void client_lua_register_functions(struct ClientLua *self)
{
    ++registrations;
    if (registration_error) luaL_error(self->L, "registration failed");
    lua_pushcfunction(self->L, observe);
    lua_setglobal(self->L, "observe");
}

BYTE game_file_mgr_is_file_exist(struct GameFileMgr *mgr, const char *path, BYTE disk_only)
{
    size_t length = strlen(path);
    assert(mgr == &g_fileMgr && queries < 2 && length < sizeof checked_paths[0]);
    strcpy(checked_paths[queries++], path);
    last_disk_only = disk_only;
    return (BYTE)(!strcmp(path + length - 4, ".lua") ? exists_lua : exists_lub);
}
BYTE *game_file_mgr_get_data(struct GameFileMgr *mgr, const char *path, int *size, BYTE disk_only)
{
    SYSTEM_INFO info;
    DWORD old;
    assert(mgr == &g_fileMgr && !source_allocation && strlen(path) < sizeof loaded_path);
    strcpy(loaded_path, path);
    last_disk_only = disk_only;
    if (source_missing) return NULL;
    GetSystemInfo(&info);
    *size = (int)strlen(source_text);
    assert((unsigned)*size < info.dwPageSize);
    source_allocation = VirtualAlloc(NULL, info.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(source_allocation);
    assert(VirtualProtect((char *)source_allocation + info.dwPageSize, info.dwPageSize, PAGE_NOACCESS, &old));
    source_bytes = (unsigned char *)source_allocation + info.dwPageSize - *size;
    if (*size) memcpy(source_bytes, source_text, *size);
    return source_bytes;
}
void game_file_mgr_release_data(struct GameFileMgr *mgr, void *data)
{
    assert(mgr == &g_fileMgr && data == source_bytes && source_allocation);
    assert(VirtualFree(source_allocation, 0, MEM_RELEASE));
    source_allocation = NULL;
    source_bytes = NULL;
    ++releases;
}
void error_msg(const char *text, int flags) { (void)text; (void)flags; abort(); }

static void sentinels(lua_State *state)
{
    lua_pushinteger(state, 123);
    lua_pushliteral(state, "retained");
}
static void check_stack(lua_State *state)
{
    assert(lua_gettop(state) == 2 && lua_tointeger(state, 1) == 123);
    assert(!strcmp(lua_tostring(state, 2), "retained"));
}

static void loaders(struct ClientLua *lua)
{
    int i, before = executions;
    source_text = "assert(observe()==1)";
    for (i = 0; i < 100; ++i) {
        queries = 0;
        assert(client_lua_load_file(lua, "테스트", 1, 0));
        assert(queries == 2 && !strcmp(checked_paths[0], "data\\LuaFiles514\\테스트.lub"));
        assert(!strcmp(checked_paths[1], "data\\LuaFiles514\\테스트.lua"));
        assert(!strcmp(loaded_path, checked_paths[1]) && !last_disk_only);
        check_stack(lua->L);
    }
    assert(executions == before + 100 && !owned_live);
    exists_lua = 0; exists_lub = 1; queries = 0;
    assert(client_lua_load_file(lua, "saved/options", 0, 1));
    assert(queries == 1 && !strcmp(checked_paths[0], "saved/options.lub"));
    assert(!strcmp(loaded_path, checked_paths[0]) && last_disk_only == 1);
    exists_lua = exists_lub = 0; queries = 0;
    assert(!client_lua_load_file(lua, "missing", 1, 0) && queries == 2);
    exists_lua = 1; queries = 0; source_missing = 1;
    assert(!client_lua_load_file(lua, "missing", 0, 0));
    source_missing = 0;
    source_text = ""; queries = 0;
    assert(client_lua_load_file(lua, "empty", 0, 0));
    source_text = "this is invalid"; queries = 0;
    assert(!client_lua_load_file(lua, "syntax", 0, 0) && strstr(last_error, "expected"));
    source_text = "error({})"; queries = 0;
    assert(!client_lua_load_file(lua, "object", 0, 0) && strstr(last_error, "Lua script failed"));
    source_text = "error(string.rep('x',10000))"; queries = 0;
    assert(!client_lua_load_file(lua, "long", 0, 0) && strlen(last_error) == 499);
    check_stack(lua->L);
    owned_attempts = 0; owned_fail = 0; queries = 0;
    assert(!client_lua_load_file(lua, "no-memory", 0, 0) && !queries && !owned_live);
    owned_fail = -1;
    assert(!client_lua_load_file(NULL, "none", 0, 0));
    assert(!client_lua_load_file(lua, NULL, 0, 0));
}

static void globals(struct ClientLua *lua)
{
    int i;
    assert(!luaL_dostring(lua->L, "number=-27.75; text='hello'; numeric='123.5'; yes={}; no=false; huge=1e100; nan=0/0"));
    for (i = 0; i < 1000; ++i) {
        assert(client_lua_get_global_int(lua, "number") == -27);
        assert(client_lua_get_global_int(lua, "numeric") == 123);
        assert(client_lua_get_global_int(lua, "missing") == -1);
        assert(client_lua_get_global_int(lua, "huge") == -1);
        assert(client_lua_get_global_int(lua, "nan") == -1);
        assert(client_lua_has_global_table(lua, "yes"));
        assert(!client_lua_has_global_table(lua, "no"));
    }
    assert(!luaL_dostring(lua->L, "setmetatable(_G,{__index=function() error('lookup') end})"));
    assert(client_lua_get_global_int(lua, "absent") == -1);
    assert(!client_lua_has_global_table(lua, "absent"));
    assert(!luaL_dostring(lua->L, "setmetatable(_G,nil)"));
    assert(client_lua_get_global_int(NULL, "x") == 0);
    assert(!client_lua_has_global_table(NULL, "x"));
    assert(client_lua_get_global_int(lua, NULL) == -1);
    check_stack(lua->L);
}

static void item_names(struct ClientLua *lua)
{
    struct ItemDbToIdMgr db = {0};
    int i;
    db.script.L = lua->L;
    assert(!luaL_dostring(lua->L, "function ItemDB_To_ItemID(n) if n=='Red_Potion' then return 501 elseif n=='number' then return '-12.5' elseif n=='error' then error('bad') elseif n=='huge' then return 1e100 end end"));
    for (i = 0; i < 1000; ++i) {
        assert(item_db_to_id_lookup(&db, "Red_Potion") == 501);
        assert(item_db_to_id_lookup(&db, "number") == -12);
        assert(!item_db_to_id_lookup(&db, "missing"));
        assert(!item_db_to_id_lookup(&db, "error"));
        assert(!item_db_to_id_lookup(&db, "huge"));
    }
    assert(item_db_to_id_lookup(&db, "Red_Potion\0ignored") == 501);
    assert(!item_db_to_id_lookup(&db, NULL) && !item_db_to_id_lookup(NULL, "x"));
    assert(!luaL_dostring(lua->L, "ItemDB_To_ItemID=nil"));
    assert(!item_db_to_id_lookup(&db, "x"));
    check_stack(lua->L);
}

static void address(struct ClientLua *lua)
{
    static struct Session session;
    struct OwnedText result = {0}, first = {0}, second = {0};
    int path, failure;
    long attempts;
    session.luaState = lua;
    assert(!luaL_dostring(lua->L, "AssistAddr='direct'; function GetTableStringValueForC(t,k) assert(t=='AssistAddrTbl'); return k end"));
    assert(session_assist_addr(&session, &result));
    assert(!strcmp(owned_text_cstr(&result), "direct"));
    assert(!luaL_dostring(lua->L, "AssistAddr=''"));
    assert(session_assist_addr(&session, &result));
    assert(!strcmp(owned_text_cstr(&result), "server:6900"));
    g_serverDomain = "complete.example:1234";
    assert(session_assist_addr(&session, &result));
    assert(!strcmp(owned_text_cstr(&result), g_serverDomain));
    g_serverDomain = NULL;
    for (path = 0; path < 2; ++path) {
        assert(!luaL_dostring(lua->L, path ? "AssistAddr=''" : "AssistAddr='direct'"));
        owned_attempts = 0;
        assert(session_service_urls(&session, &first, "/save", &second, "/load"));
        attempts = owned_attempts;
        assert(!strcmp(owned_text_cstr(&first), path ? "server:6900/save" : "direct/save"));
        assert(!strcmp(owned_text_cstr(&second), path ? "server:6900/load" : "direct/load"));
        for (failure = 0; failure < attempts; ++failure) {
            assert(owned_text_assign(&first, text_view_cstr("old-save")));
            assert(owned_text_assign(&second, text_view_cstr("old-load")));
            owned_attempts = 0; owned_fail = failure;
            assert(!session_service_urls(&session, &first, "/save", &second, "/load"));
            owned_fail = -1;
            assert(!strcmp(owned_text_cstr(&first), "old-save"));
            assert(!strcmp(owned_text_cstr(&second), "old-load"));
            check_stack(lua->L);
        }
    }
    assert(!luaL_dostring(lua->L, "AssistAddr='direct'"));
    assert(session_service_url(&session, &result, "/party/search"));
    assert(!strcmp(owned_text_cstr(&result), "direct/party/search"));
    session.luaState = NULL;
    assert(!session_assist_addr(&session, &result));
    assert(!strcmp(owned_text_cstr(&result), "direct/party/search"));
    owned_text_clear(&result); owned_text_clear(&first); owned_text_clear(&second);
    assert(!live_blocks && !owned_live);
    check_stack(lua->L);
}

static long open_case(long failure)
{
    struct ClientLua lua;
    long attempts;
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE error = GetStdHandle(STD_ERROR_HANDLE);
    DWORD output_type = GetFileType(output), error_type = GetFileType(error);
    client_lua_construct(&lua);
    lua_memory.attempts = 0; lua_memory.fail_at = failure;
    client_lua_open(&lua);
    attempts = lua_memory.attempts;
    if (failure >= 0 && lua.L)
        fprintf(stderr, "Initialization survived failure %ld of %ld observed allocations\n", failure, attempts);
    if (failure >= 0) assert(!lua.L);
    else assert(lua.L && lua_gettop(lua.L) == 0);
    lua_memory.fail_at = -1;
    client_lua_destruct(&lua);
    assert(!lua.L && !lua_memory.live);
    assert(GetFileType(output) == output_type && GetFileType(error) == error_type);
    return attempts;
}

static long operation_case(int operation, long failure)
{
    struct ClientLua lua;
    struct ItemDbToIdMgr db = {0};
    char name[1025];
    int ok = 0, released = releases;
    long attempts;
    client_lua_construct(&lua);
    client_lua_open(&lua);
    assert(lua.L);
    assert(!luaL_dostring(lua.L, "Integer=42; Table={}; function ItemDB_To_ItemID(s) return #s end"));
    sentinels(lua.L);
    memset(name, 'x', sizeof name - 1); name[sizeof name - 1] = 0;
    db.script.L = lua.L;
    lua_memory.attempts = 0; lua_memory.fail_at = failure;
    switch (operation) {
    case 0: ok = client_lua_get_global_int(&lua, "Integer") == 42; break;
    case 1: ok = client_lua_has_global_table(&lua, "Table"); break;
    case 2: ok = item_db_to_id_lookup(&db, name) == 1024; break;
    case 3:
        source_text = "local values={1,2,3}; local function count(v) return #v end; assert(count(values)==3)";
        queries = 0;
        ok = client_lua_load_file(&lua, "allocation", 0, 0);
        assert(releases == released + 1 && !source_allocation);
        break;
    }
    attempts = lua_memory.attempts;
    lua_memory.fail_at = -1;
    assert(ok == (failure < 0));
    check_stack(lua.L);
    client_lua_close(&lua);
    assert(!lua_memory.live && !owned_live);
    return attempts;
}

int main(void)
{
    struct ClientLua lua;
    long allocations, query_allocations = 0, i;
    int operation;
    client_lua_construct(&lua);
    client_lua_open(&lua);
    assert(lua.L);
    sentinels(lua.L);
    loaders(&lua);
    globals(&lua);
    item_names(&lua);
    address(&lua);
    registration_error = 1;
    client_lua_open(&lua);
    assert(!lua.L && !lua_memory.live);
    registration_error = 0;
    client_lua_close(&lua);
    client_lua_close(NULL);
    client_lua_destruct(NULL);
    client_lua_open(NULL);
    assert(!client_lua_construct(NULL));
    allocations = open_case(-1);
    for (i = 0; i < allocations; ++i) open_case(i);
    for (operation = 0; operation < 4; ++operation) {
        long count = operation_case(operation, -1);
        assert(count > 0);
        for (i = 0; i < count; ++i) operation_case(operation, i);
        query_allocations += count;
    }
    assert(!owned_live && !source_allocation && !lua_memory.live && !live_blocks);
    printf("Lua boundaries: typed ownership, guarded loading, globals, item names, service addresses, %ld initialization and %ld query/load allocation failures passed\n", allocations, query_allocations);
    return 0;
}
