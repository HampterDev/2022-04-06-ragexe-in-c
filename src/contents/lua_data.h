#ifndef CONTENTS_LUA_DATA_H
#define CONTENTS_LUA_DATA_H

#include "base.h"
#include <lua.h>

struct LuaDataFunction {
    const char *name;
    lua_CFunction function;
};

enum LuaDataCheck {
    LUA_DATA_ERROR_FORMAT,
    LUA_DATA_ERROR_STRING,
    LUA_DATA_ERROR_PREFIX
};

char lua_data_run(const char *path, const char *owner,
                  const struct LuaDataFunction *functions, int function_count,
                  enum LuaDataCheck check);

struct LuaDataResult {
    char ok;
    struct String message;
};
OFFSET_ASSERT(LuaDataResult, message, 0x04);

struct LuaDataResult *lua_data_result(struct LuaDataResult *result, char ok, const char *message);

int lua_data_reply(lua_State *L, int ok, const char *message);
int lua_data_reply_result(lua_State *L, struct LuaDataResult *result, const char *fail_format);

#endif
