#include "session/talk_options_lua.h"
#include "session/talk_options.h"
#include <lauxlib.h>

int lua_set_cmd_on_off_list(lua_State *L)
{
    const char *name = luaL_checklstring(L, 1, NULL);
    int value = (int)luaL_checknumber(L, 2);

    talk_option_set(name, value);
    return 0;
}

int lua_get_cmd_on_off_value(lua_State *L)
{
    const char *name = luaL_checklstring(L, 1, NULL);
    struct TalkOption option = talk_option_find(name);
    int value;

    if (talk_option_read(&option, &value))
        lua_pushnumber(L, (double)(unsigned char)value);
    else
        lua_pushnumber(L, -1.0);
    return 1;
}
