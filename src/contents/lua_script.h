#ifndef CONTENTS_LUA_SCRIPT_H
#define CONTENTS_LUA_SCRIPT_H

#include "contents/client_lua_calls.h"
#include "base.h"

struct lua_State;
struct LuaScript {
    struct lua_State *L;
};

struct LuaScript *lua_script_construct(struct LuaScript *self);
void lua_script_destruct(struct LuaScript *self);
void lua_script_set_number(struct LuaScript *self, const char *name, double value);
void lua_script_set_boolean(struct LuaScript *self, const char *name, char value);
void lua_script_register(struct LuaScript *self, const char *name,
                         int (*fn)(struct lua_State *));
char lua_script_run_file(struct LuaScript *self, const char *path);

#endif
