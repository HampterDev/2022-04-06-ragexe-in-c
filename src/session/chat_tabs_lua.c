#include "session/chat_tabs_lua.h"
#include "session/state.h"
#include <lauxlib.h>

static struct ChatTabs *tabs_for_kind(const char *kind, size_t size)
{
    return size == 2 && memcmp(kind, "On", 2) == 0
        ? &g_session1.chatWndTabs : &g_session1.subChatWnds;
}

int lua_set_sub_chat_wnd_list(lua_State *L)
{
    struct ChatTabSettings tab = {0};
    struct ChatTabs *tabs;
    const char *kind, *name;
    size_t kind_size, name_size;
    int key, copied;

    if (g_session1.chatWndTabs.size + g_session1.subChatWnds.size >= CHAT_TAB_LIMIT)
        return 1;
    kind = luaL_checklstring(L, 1, &kind_size);
    name = luaL_checklstring(L, 2, &name_size);
    tab.x = (int)luaL_checknumber(L, 3);
    tab.y = (int)luaL_checknumber(L, 4);
    tab.width = (int)luaL_checknumber(L, 5);
    tab.height = (int)luaL_checknumber(L, 6);

    tabs = tabs_for_kind(kind, kind_size);
    key = (int)tabs->size;
    if (!owned_text_assign(&tab.name, text_view(name, name_size)))
        return luaL_error(L, "Unable to allocate chat tab name");
    copied = chat_tabs_set(tabs, key, &tab);
    chat_tab_settings_clear(&tab);
    if (!copied)
        return luaL_error(L, "Unable to allocate chat tab settings");
    lua_pushnumber(L, (double)key);
    return 1;
}

int lua_set_sub_chat_wnd_option(lua_State *L)
{
    size_t kind_size;
    const char *kind = luaL_checklstring(L, 1, &kind_size);
    int key = (int)luaL_checknumber(L, 2);
    int option = (int)luaL_checknumber(L, 3);
    int on = (int)luaL_checknumber(L, 4);

    chat_tabs_set_option(tabs_for_kind(kind, kind_size), key, option, on);
    return 1;
}
