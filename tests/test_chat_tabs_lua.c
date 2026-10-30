#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "session/state.h"
#include "session/chat_tabs_lua.h"
#include <lauxlib.h>
#include "owned_storage_test_allocator.h"

struct Session g_session1;
void client_lua_loadfile_hook(const char *name) { (void)name; abort(); }

#define malloc checked_malloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/session/chat_tabs.c"
#undef free
#undef malloc
#include "../src/session/chat_tabs_lua.c"

static int add(lua_State *L, const char *kind, size_t kind_size, const char *name, size_t name_size)
{
    lua_pushcfunction(L, lua_set_sub_chat_wnd_list);
    lua_pushlstring(L, kind, kind_size);
    lua_pushlstring(L, name, name_size);
    lua_pushnumber(L, -14.75);
    lua_pushnumber(L, 25.5);
    lua_pushnumber(L, 320);
    lua_pushnumber(L, 123);
    return lua_pcall(L, 6, 1, 0);
}

static void option(lua_State *L, const char *kind, size_t kind_size, int key, int flag, int on)
{
    lua_pushcfunction(L, lua_set_sub_chat_wnd_option);
    lua_pushlstring(L, kind, kind_size);
    lua_pushnumber(L, key);
    lua_pushnumber(L, flag);
    lua_pushnumber(L, on);
    assert(lua_pcall(L, 4, 1, 0) == 0);
    assert(lua_tonumber(L, -1) == on);
    lua_pop(L, 1);
}

static void clear(void)
{
    chat_tabs_clear(&g_session1.chatWndTabs);
    chat_tabs_clear(&g_session1.subChatWnds);
    assert(outstanding == 0);
}

static void check_args_and_failures(lua_State *L)
{
    struct ChatTab before;
    struct ChatTab *storage;
    unsigned live, n;
    int bad;

    for (bad = 1; bad <= 6; ++bad) {
        int i;
        unsigned calls = allocation_calls;
        lua_pushcfunction(L, lua_set_sub_chat_wnd_list);
        for (i = 1; i <= 6; ++i) {
            if (i == bad) lua_newtable(L);
            else if (i == 1) lua_pushstring(L, "On");
            else if (i == 2) lua_pushstring(L, "valid name");
            else lua_pushnumber(L, 14);
        }
        assert(lua_pcall(L, 6, 1, 0) != 0);
        lua_pop(L, 1);
        assert(allocation_calls == calls && outstanding == 0);
        assert(!g_session1.chatWndTabs.size && !g_session1.subChatWnds.size);
    }
    assert(add(L, "On", 2, "existing", 8) == 0);
    lua_pop(L, 1);
    before = g_session1.chatWndTabs.items[0];
    storage = g_session1.chatWndTabs.items;
    live = outstanding;
    for (n = 1; n <= 3; ++n) {
        fail_at = allocation_calls + n;
        assert(add(L, "On", 2, "new", 3) != 0);
        assert(lua_isstring(L, -1));
        lua_pop(L, 1);
        fail_at = 0;
        assert(outstanding == live);
        assert(g_session1.chatWndTabs.items == storage && g_session1.chatWndTabs.size == 1);
        assert(!memcmp(&before, storage, sizeof before));
        assert(!memcmp(before.value.name.data, "existing", 9));
    }
    clear();
}

static void check_settings(lua_State *L)
{
    const char name[] = "한글\0tab";
    struct ChatTabSettings *value;
    struct ChatTab before;
    unsigned i;

    assert(add(L, "On", 2, name, sizeof name - 1) == 0);
    assert(lua_tonumber(L, -1) == 0);
    lua_pop(L, 1);
    assert(g_session1.chatWndTabs.size == 1 && !g_session1.subChatWnds.size);
    value = &g_session1.chatWndTabs.items[0].value;
    assert(value->name.size == sizeof name - 1);
    assert(!memcmp(value->name.data, name, sizeof name));
    assert(value->x == -14 && value->y == 25 && value->width == 320 && value->height == 123);
    for (i = 0; i < CHAT_TAB_FLAG_COUNT; ++i) assert(value->shown[i] == 0);
    for (i = 1; i <= CHAT_TAB_FLAG_COUNT; ++i) option(L, "On", 2, 0, i, i % 2 ? -4 : 0);
    for (i = 0; i < CHAT_TAB_FLAG_COUNT; ++i) assert(value->shown[i] == (i % 2 == 0));
    before = g_session1.chatWndTabs.items[0];
    option(L, "On", 2, 0, 0, 1);
    option(L, "On", 2, 0, 23, 1);
    option(L, "On", 2, -1, 1, 1);
    option(L, "On", 2, 14, 1, 1);
    assert(!memcmp(&before, g_session1.chatWndTabs.items, sizeof before));

    assert(add(L, "On\0", 3, "separate", 8) == 0);
    assert(lua_tonumber(L, -1) == 0);
    lua_pop(L, 1);
    assert(add(L, "No", 2, "other", 5) == 0);
    assert(lua_tonumber(L, -1) == 1);
    lua_pop(L, 1);
    assert(g_session1.subChatWnds.size == 2);
    option(L, "On\0", 3, 0, 22, 1);
    assert(g_session1.subChatWnds.items[0].value.shown[21] == 1);
    assert(g_session1.subChatWnds.items[1].value.shown[21] == 0);
    for (i = 0; i < 7; ++i) {
        assert(add(L, "On", 2, "extra", 5) == 0);
        assert(lua_tonumber(L, -1) == i + 1);
        lua_pop(L, 1);
    }
    assert(g_session1.chatWndTabs.size == 8 && g_session1.subChatWnds.size == 2);
    i = allocation_calls;
    assert(add(L, "On", 2, "over limit", 10) == 0);
    assert(lua_tonumber(L, -1) == 123);
    lua_pop(L, 1);
    assert(allocation_calls == i && g_session1.chatWndTabs.size == 8);
    clear();
}

int main(void)
{
    lua_State *L = luaL_newstate();
    assert(L);
    check_args_and_failures(L);
    check_settings(L);
    assert(lua_gettop(L) == 0);
    lua_close(L);
    puts("Chat-tab Lua: actual Lua 5.1, counted Korean names, kind routing, geometry, all 22 options, tab limit, bad arguments and allocation rollback passed");
    return 0;
}
