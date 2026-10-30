#include "contents/client_lua_calls.h"
#include "contents/client_lua.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

struct LuaCallArguments {
    const char *table;
    const char *function;
    const char *format;
    int inputs, outputs, anchored;
    va_list values;
};

void client_lua_error_box(struct ClientLua *lua, const char *caption, const char *format, ...)
{
    char text[500];
    va_list values;
    (void)lua;
    va_start(values, format);
    vsnprintf(text, sizeof text, format, values);
    va_end(values);
    MessageBoxA(NULL, text, caption, 0);
}

static int parse_format(const char *format, int *inputs, int *outputs)
{
    int result = 0, total = 0;
    *inputs = *outputs = 0;
    for (; *format; ++format) {
        if (*format == '>') {
            if (result) return 0;
            result = 1;
        } else {
            if (*format != 'b' && *format != 'd' && *format != 'l' && *format != 's') return 0;
            if (total == INT_MAX - 4) return 0;
            ++total;
            if (result) ++*outputs;
            else ++*inputs;
        }
    }
    return 1;
}

static int dispatch_call(lua_State *state)
{
    struct LuaCallArguments *call = lua_touserdata(state, 1);
    const char *format = call->format;
    int index, first;
    if (!lua_checkstack(state, call->inputs + call->outputs + 4)) {
        lua_pushliteral(state, "too many Lua call values");
        return lua_error(state);
    }
    if (call->table) {
        lua_getfield(state, LUA_GLOBALSINDEX, call->table);
        lua_getfield(state, -1, call->function);
    } else {
        lua_getfield(state, LUA_GLOBALSINDEX, call->function);
    }
    while (*format && *format != '>') {
        switch (*format++) {
        case 'b': lua_pushboolean(state, (unsigned char)va_arg(call->values, int)); break;
        case 'd': lua_pushnumber(state, (double)va_arg(call->values, int)); break;
        case 'l': lua_pushnumber(state, va_arg(call->values, double)); break;
        case 's': lua_pushstring(state, va_arg(call->values, const char *)); break;
        }
    }
    lua_call(state, call->inputs, call->outputs);
    format = strchr(call->format, '>');
    if (format) {
        for (++format, index = -call->outputs; *format; ++format, ++index)
            if (*format == 's') lua_tolstring(state, index, NULL);
    }
    if (call->outputs) {
        first = lua_gettop(state) - call->outputs + 1;
        lua_createtable(state, call->outputs, 0);
        for (index = 0; index < call->outputs; ++index) {
            lua_pushvalue(state, first + index);
            lua_rawseti(state, -2, index + 1);
        }
        lua_pushlightuserdata(state, call);
        lua_pushvalue(state, -2);
        lua_rawset(state, LUA_REGISTRYINDEX);
        call->anchored = 1;
    }
    return 0;
}

static int call_values(struct ClientLua *lua, const char *table, const char *function,
                       const char *format, va_list values)
{
    struct LuaCallArguments call;
    int base, ok, index;
    const char *results;
    lua_State *state;
    if (!lua || !lua->L || !function || !format) return 0;
    if (!parse_format(format, &call.inputs, &call.outputs)) {
        client_lua_error_box(lua, function, "%s", "invalid Lua call format");
        return 0;
    }
    state = lua->L;
    base = lua_gettop(state);
    call.table = table;
    call.function = function;
    call.format = format;
    call.anchored = 0;
    va_copy(call.values, values);
    ok = lua_cpcall(state, dispatch_call, &call) == 0;
    if (ok && call.anchored) {
        lua_pushlightuserdata(state, &call);
        lua_rawget(state, LUA_REGISTRYINDEX);
        results = strchr(format, '>') + 1;
        for (index = 1; *results; ++results, ++index) {
            lua_rawgeti(state, -1, index);
            switch (*results) {
            case 'b': *va_arg(call.values, char *) = lua_toboolean(state, -1) != 0; break;
            case 'd': *va_arg(call.values, int *) = (int)lua_tonumber(state, -1); break;
            case 'l': *va_arg(call.values, double *) = lua_tonumber(state, -1); break;
            case 's': *va_arg(call.values, const char **) = lua_tolstring(state, -1, NULL); break;
            }
            lua_pop(state, 1);
        }
    } else if (!ok) {
        const char *error = lua_type(state, -1) == LUA_TSTRING ? lua_tostring(state, -1) : NULL;
        client_lua_error_box(lua, function, "%s\n", error ? error : "Lua call failed");
    }
    va_end(call.values);
    if (call.anchored) {
        lua_pushlightuserdata(state, &call);
        lua_pushnil(state);
        lua_rawset(state, LUA_REGISTRYINDEX);
    }
    lua_settop(state, base);
    return ok;
}

int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...)
{
    va_list values;
    int ok;
    va_start(values, format);
    ok = call_values(lua, NULL, function, format, values);
    va_end(values);
    return ok;
}

int client_lua_call_table(struct ClientLua *lua, const char *table, const char *function,
                          const char *format, ...)
{
    va_list values;
    int ok;
    if (!table) return 0;
    va_start(values, format);
    ok = call_values(lua, table, function, format, values);
    va_end(values);
    return ok;
}

struct LuaIntegerLookup {
    const char *table;
    int key;
    int value;
};

static int lookup_integer(lua_State *state)
{
    struct LuaIntegerLookup *lookup = lua_touserdata(state, 1);
    lua_getfield(state, LUA_GLOBALSINDEX, lookup->table);
    if (!lua_istable(state, -1)) return 0;
    lua_pushinteger(state, lookup->key);
    lua_gettable(state, -2);
    if (lua_isnumber(state, -1)) lookup->value = (int)lua_tonumber(state, -1);
    return 0;
}

int client_lua_table_int(struct ClientLua *lua, const char *table, int key)
{
    struct LuaIntegerLookup lookup = {table, key, -1};
    int base;
    lua_State *state;
    if (!lua || !lua->L) return 0;
    if (!table) return -1;
    state = lua->L;
    base = lua_gettop(state);
    if (lua_cpcall(state, lookup_integer, &lookup) != 0) lookup.value = -1;
    lua_settop(state, base);
    return lookup.value;
}
