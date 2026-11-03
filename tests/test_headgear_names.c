#include "contents/client_lua.h"
#include "session/session_names.h"
#include "std/string.h"
#include <lauxlib.h>
#include <lualib.h>
#include <limits.h>
#include "world_state_memory.h"

/* Use the actual lookup, Lua bridge/runtime and String implementation. */
static int errors, checks;

static int message_box(HWND window, const char *text, const char *caption, UINT flags)
{
    assert(!window && !flags && text && caption);
    ++errors;
    return 0;
}
#include "platform/win_utf8.h"
#define win_utf8_message_box message_box
#include "../src/contents/lua_calls.c"
#undef win_utf8_message_box
#include "headgear_names.inc"

static void run(lua_State *state, const char *text)
{
    int base = lua_gettop(state);
    assert(luaL_dostring(state, text) == 0);
    assert(lua_gettop(state) == base);
}

static void expect_name(struct Session *session, int id, const char *expected)
{
    struct String name;
    lua_State *state = session->luaState ? session->luaState->L : NULL;
    int base = state ? lua_gettop(state) : 0;
    assert(session_get_accessory_name(session, &name, id) == &name);
    assert(!strcmp(string_cstr(&name), expected));
    assert(name.size == strlen(expected));
    if (state) assert(lua_gettop(state) == base);
    string_free(&name);
    assert(live_blocks == 0);
    ++checks;
}

int main(void)
{
    struct ClientLua lua = {luaL_newstate()}, closed = {NULL};
    static struct Session session;
    struct String retained;
    session.luaState = &lua;
    assert(lua.L);
    luaL_openlibs(lua.L);
    run(lua.L,
        "calls=0; AccNameTable={[1]='_goggles',[2999]='_edge',"
        "[3000]='_custom_3000',[10000]='_custom_10000',"
        "[32767]='_wire_edge',[2147483647]='_int_edge',"
        "[4000]='',[5000]='_\\189\\180\\199\\213'}; "
        "function ReqAccName(id) calls=calls+1; "
        "if AccNameTable == nil then return '' end "
        "return AccNameTable[id] or '' end");
    lua_pushliteral(lua.L, "stack sentinel");
    expect_name(&session, 0, "");
    expect_name(&session, -1, "");
    expect_name(&session, INT_MIN, "");
    lua_getglobal(lua.L, "calls");
    assert(lua_tointeger(lua.L, -1) == 0);
    lua_pop(lua.L, 1);
    expect_name(&session, 1, "_goggles");
    expect_name(&session, 2999, "_edge");
    expect_name(&session, 3000, "_custom_3000");
    expect_name(&session, 10000, "_custom_10000");
    expect_name(&session, 32767, "_wire_edge");
    /* INT_MAX tests the lookup only, not packet representation. */
    expect_name(&session, INT_MAX, "_int_edge");
    expect_name(&session, 9999, "");
    expect_name(&session, 4000, "");
    expect_name(&session, 5000, "_\xbd\xb4\xc7\xd5");
    run(lua.L, "AccNameTable[3000]='_changed'");
    expect_name(&session, 3000, "_changed");
    run(lua.L, "AccNameTable=nil");
    expect_name(&session, 3000, "");
    run(lua.L, "ReqAccName=function() return nil end");
    expect_name(&session, 3000, "");
    run(lua.L, "ReqAccName=function() error('bad catalog') end");
    expect_name(&session, 3000, "");
    run(lua.L, "ReqAccName=nil");
    expect_name(&session, 3000, "");
    assert(errors == 2);
    session.luaState = &closed;
    expect_name(&session, 3000, "");
    session.luaState = NULL;
    expect_name(&session, 3000, "");
    session.luaState = &lua;
    /* A computed filename must survive Lua collection and shutdown. */
    run(lua.L, "ReqAccName=function(id) return '_dynamic_' .. id .. string.rep('x', 80) end");
    session_get_accessory_name(&session, &retained, 50000);
    run(lua.L, "ReqAccName=nil; collectgarbage('collect')");
    assert(lua_gettop(lua.L) == 1);
    assert(!strcmp(lua_tostring(lua.L, -1), "stack sentinel"));
    lua_close(lua.L);
    assert(retained.size == strlen("_dynamic_50000") + 80);
    assert(!strncmp(string_cstr(&retained), "_dynamic_50000", strlen("_dynamic_50000")));
    for (size_t i = strlen("_dynamic_50000"); i < retained.size; ++i)
        assert(retained.data[i] == 'x');
    string_free(&retained);
    assert(live_blocks == 0);
    printf("Headgear names: %d lookup cases plus owned filename after Lua GC/shutdown passed.\n", checks);
    return 0;
}
