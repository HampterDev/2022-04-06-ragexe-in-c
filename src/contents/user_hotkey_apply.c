#include "contents/user_hotkeys.h"
#include "contents/client_lua.h"
#include "session/state.h"

static const char *const names[] = {"USERKEY_1", "USERKEY_2", "USERKEY_3", "USERKEY_4"};

static int apply(lua_State *state)
{
    const struct UserHotkeyList *keys = lua_touserdata(state, 1);
    int group;
    size_t i;
    for (group = 0; group < 4; ++group) {
        lua_newtable(state);
        for (i = 0; i < keys->size; ++i) {
            const struct UserHotkey *key = &keys->items[i];
            if (key->group != group) continue;
            lua_newtable(state);
            lua_pushlstring(state, owned_text_cstr(&key->key_name), key->key_name.size);
            lua_setfield(state, -2, "EXE");
            lua_pushinteger(state, key->key1);
            lua_setfield(state, -2, "KEY1");
            lua_pushinteger(state, key->key2);
            lua_setfield(state, -2, "KEY2");
            lua_rawseti(state, -2, key->id);
        }
    }
    for (group = 3; group >= 0; --group)
        lua_setfield(state, LUA_GLOBALSINDEX, names[group]);
    return 0;
}

int user_hotkey_mgr_apply(const struct UserHotkeyList *keys)
{
    lua_State *state = g_session1.luaState ? g_session1.luaState->L : NULL;
    int base, ok;
    if (!state) return 0;
    base = lua_gettop(state);
    ok = lua_cpcall(state, apply, (void *)keys) == 0;
    lua_settop(state, base);
    return ok;
}
