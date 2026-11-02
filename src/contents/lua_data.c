#include "core/thread_text.h"
#include <string.h>
#include "platform/win_types.h"
#include "contents/lua_data.h"
#include "std/std.h"
#include <lauxlib.h>
#include <lualib.h>
#include "core/text_util.h"
#include "platform/window.h"
#include "contents/lua_file.h"

static char main_result_ok(lua_State *L, const char *owner, enum LuaDataCheck check)
{
    const char *message;
    struct String text;

    if (!lua_isstring(L, -1)) {
        platform_message_box(0, "second return value is not string...", owner, 0);
        return 0;
    }
    if (lua_toboolean(L, -2))
        return 1;
    message = lua_tolstring(L, -1, 0);
    if (check == LUA_DATA_ERROR_FORMAT) {
        platform_message_box(0, thread_text_format("error : %s", message), owner, 0);
    } else if (check == LUA_DATA_ERROR_STRING) {
        string_init(&text);
        string_format(&text, "error : %s", message);
        platform_message_box(0, string_cstr(&text), owner, 0);
        string_free(&text);
    } else {
        string_init(&text);
        string_assign_cstr(&text, "error found :");
        string_append_cstr(&text, message);
        platform_message_box(0, string_cstr(&text), owner, 0);
        string_free(&text);
    }
    return 0;
}

char lua_data_run(const char *path, const char *owner,
                  const struct LuaDataFunction *functions, int function_count,
                  enum LuaDataCheck check)
{
    lua_State *L = luaL_newstate();
    char ok;
    int i;

    if (!L)
        return 0;
    lua_open_client_libs(L);
    for (i = 0; i < function_count; i++) {
        lua_pushcclosure(L, functions[i].function, 0);
        lua_setfield(L, LUA_GLOBALSINDEX, functions[i].name);
    }
    if (lua_load_disk_file(L, path) != 0 || lua_pcall(L, 0, 0, 0) != 0) {
        ok = 0;
    } else {
        lua_getfield(L, LUA_GLOBALSINDEX, "main");
        ok = lua_pcall(L, 0, 2, 0) == 0;
    }
    if (!ok)
        platform_message_box(0, lua_tolstring(L, -1, 0), owner, 0);
    else if (check != LUA_DATA_IGNORE_RESULT)
        ok = main_result_ok(L, owner, check);
    lua_close(L);
    return ok;
}

struct LuaDataResult *lua_data_result(struct LuaDataResult *result, char ok, const char *message)
{
    result->ok = ok;
    string_init(&result->message);
    string_assign_cstr(&result->message, message);
    return result;
}

int lua_data_reply(lua_State *L, int ok, const char *message)
{
    lua_pushboolean(L, ok);
    lua_pushstring(L, message);
    return 2;
}

int lua_data_reply_result(lua_State *L, struct LuaDataResult *result, const char *fail_format)
{
    if (result->ok) {
        lua_data_reply(L, 1, "good");
    } else {
        lua_pushboolean(L, 0);
        lua_pushstring(L, thread_text_format(fail_format, string_cstr(&result->message)));
    }
    string_free(&result->message);
    return 2;
}
