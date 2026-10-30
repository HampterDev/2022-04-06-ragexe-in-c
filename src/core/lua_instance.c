#include <windows.h>
#include <stdlib.h>
#include "contents/lua_script.h"
#include "fileio/file_api.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static int lua_message_box(lua_State *L)
{
    const char *text = lua_tolstring(L, -1, 0);

    MessageBoxA(NULL, text ? text : "", "CLuaInstance", 0);
    return 0;
}

static void register_message_box(struct LuaScript *self)
{
    lua_pushcclosure(self->L, lua_message_box, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, "MessageBox");
}

struct LuaScript *lua_script_construct(struct LuaScript *self)
{
    self->L = luaL_newstate();
    if (self->L != 0) {
        luaL_openlibs(self->L);
        register_message_box(self);
    }
    return self;
}

void lua_script_destruct(struct LuaScript *self)
{
    if (self->L != 0) {
        lua_close(self->L);
        self->L = 0;
    }
}

char lua_script_run_file(struct LuaScript *self, const char *path)
{
    struct File file;
    const unsigned char *text;
    lua_State *state;
    int base, status;

    if (!self || !self->L || !path)
        return 0;
    state = self->L;
    base = lua_gettop(state);
    file_construct(&file);
    if (!file_open(&file, path, 0, 1, 0)) {
        file_destruct(&file);
        return 0;
    }
    text = file_consume(&file, (size_t)file.size, 1);
    if (!text) {
        file_destruct(&file);
        return 0;
    }
    status = luaL_loadbuffer(state, (const char *)text, (size_t)file.size, "buf");
    file_destruct(&file);
    if (!status)
        status = lua_pcall(state, 0, 0, 0);
    if (status) {
        const char *error = lua_type(state, -1) == LUA_TSTRING ? lua_tostring(state, -1) : NULL;
        MessageBoxA(NULL, error ? error : "Lua script failed", path, 0);
    }
    lua_settop(state, base);
    return status == 0;
}

void lua_script_set_boolean(struct LuaScript *self, const char *name, char value)
{
    lua_pushboolean(self->L, (unsigned char)value);
    lua_setfield(self->L, LUA_GLOBALSINDEX, name);
}

void lua_script_register(struct LuaScript *self, const char *name, int (*fn)(struct lua_State *))
{
    lua_pushcclosure(self->L, fn, 0);
    lua_setfield(self->L, LUA_GLOBALSINDEX, name);
}

void lua_script_set_number(struct LuaScript *self, const char *name, double value)
{
    lua_pushnumber(self->L, value);
    lua_setfield(self->L, LUA_GLOBALSINDEX, name);
}
