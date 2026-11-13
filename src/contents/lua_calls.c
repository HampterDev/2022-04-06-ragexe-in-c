#include "contents/client_lua_calls.h"
#include "contents/client_lua.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform/system.h"
#include "platform/window.h"

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
    platform_message_box(NULL, text, caption, 0);
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

/* A function's answers take ANSWERS_FIRST slots at first and twice as many each time a new answer
   would take another's place, up to ANSWERS_MOST (16384 slots of 68 bytes); from then on a new
   answer in a full set takes the place of the one used longest ago in it. */
#define ANSWERS_FIRST 64
#define ANSWERS_MOST 16384

static size_t value_size(char kind)
{
    switch (kind) {
    case 'b': return 1;
    case 'd': return sizeof(int);
    case 'l': return sizeof(double);
    }
    return 0;
}

/* The call as its function's key: the format with its terminator, then the arguments as
   call_values pushes them. 0 when the call cannot be remembered: a string among its values, a bad
   format, a format without results (nothing to remember), or a key or answer too long. */
static size_t call_key(unsigned char *key, const char *format, va_list values)
{
    size_t format_size = strlen(format) + 1, size = format_size, answer = 0;
    const char *p;
    int results = 0;
    va_list args;

    if (size > LUA_MEMO_KEY_MAX) return 0;
    for (p = format; *p; ++p) {
        if (*p == '>') {
            if (results) return 0;
            results = 1;
            continue;
        }
        if (!value_size(*p)) return 0;
        if (results) answer += value_size(*p);
        else size += value_size(*p);
    }
    if (!answer || size > LUA_MEMO_KEY_MAX || answer > LUA_MEMO_VALUE_MAX) return 0;
    memcpy(key, format, format_size);
    size = format_size;
    va_copy(args, values);
    for (p = format; *p && *p != '>'; ++p) {
        if (*p == 'b') {
            key[size++] = (unsigned char)va_arg(args, int);
        } else if (*p == 'd') {
            int value = va_arg(args, int);
            memcpy(key + size, &value, sizeof value); size += sizeof value;
        } else {
            double value = va_arg(args, double);
            memcpy(key + size, &value, sizeof value); size += sizeof value;
        }
    }
    va_end(args);
    return size;
}

/* Walks the results of a call whose format call_key took, through its value list past the
   arguments: from the answer into the caller's variables (give), or from them into the answer
   (take). Both loops stop at the end of the format. */
static size_t walk_answer(const char *format, va_list values, unsigned char *answer, int give)
{
    const char *p = format;
    size_t size = 0;
    va_list args;

    va_copy(args, values);
    for (; *p && *p != '>'; ++p) {
        if (*p == 'l') (void)va_arg(args, double);
        else (void)va_arg(args, int);
    }
    if (*p) {
        for (++p; *p && value_size(*p); ++p) {
            void *variable = *p == 'b' ? (void *)va_arg(args, char *)
                             : *p == 'd' ? (void *)va_arg(args, int *) : (void *)va_arg(args, double *);
            if (give) memcpy(variable, answer + size, value_size(*p));
            else memcpy(answer + size, variable, value_size(*p));
            size += value_size(*p);
        }
    }
    va_end(args);
    return size;
}

static void remember(struct LuaMemo *memo, const unsigned char *key, size_t key_size,
                     const unsigned char *answer, size_t answer_size)
{
    size_t slots;

    while ((slots = lua_memo_slots(memo)) < ANSWERS_MOST
           && (!slots || lua_memo_would_replace(memo, key, key_size))) {
        size_t grown_slots = slots ? slots * 2 : ANSWERS_FIRST;
        struct LuaMemoEntry *entries = malloc(grown_slots * sizeof *entries);
        struct LuaMemo grown;

        if (!entries)
            break;
        lua_memo_init(&grown, entries, grown_slots);
        lua_memo_move(&grown, memo);
        free(memo->entries);
        *memo = grown;
    }
    lua_memo_store(memo, key, key_size, answer, answer_size);
}

/* A function that answered a call it had answered before differently, or failed it: its answers
   are dropped and it goes to Lua every time while the state lives. Said once, in the debug
   output. */
static void turn_off(struct LuaMemoFunction *place, const char *table, const char *function)
{
    char text[200];

    free(place->answers.entries);
    lua_memo_init(&place->answers, NULL, 0);
    place->off = 1;
    snprintf(text, sizeof text, "Lua: %s%s%s answered a call differently than before; its answers are not "
             "remembered any more.\n", table ? table : "", table ? "." : "", function);
    platform_debug_output(text);
}

static int pure_call(struct ClientLua *lua, const char *table, const char *function, const char *format,
                     va_list values)
{
    unsigned char key[LUA_MEMO_KEY_MAX], known[LUA_MEMO_VALUE_MAX], answer[LUA_MEMO_VALUE_MAX];
    struct LuaMemoFunction *place;
    const struct LuaMemoEntry *entry;
    size_t key_size, known_size = 0, answer_size;
    int check = 0;

    if (!lua || !lua->L || !function || !format || !(key_size = call_key(key, format, values))
        || !(place = lua_memo_book_function(&lua->answers, table, function)) || place->off)
        return call_values(lua, table, function, format, values);
    entry = lua_memo_find(&place->answers, key, key_size);
    if (entry) {
        known_size = entry->value_size;
        memcpy(known, entry->value, known_size);
        check = lua_memo_check_due(&place->answers);
        if (!check) {
            walk_answer(format, values, known, 1);
            return 1;
        }
    }
    if (!call_values(lua, table, function, format, values)) {
        if (check && (place = lua_memo_book_function(&lua->answers, table, function)) && !place->off)
            turn_off(place, table, function);
        return 0;
    }
    answer_size = walk_answer(format, values, answer, 0);
    /* Found again: the call ran Lua, which may have loaded a script or opened the state again. */
    place = lua_memo_book_function(&lua->answers, table, function);
    if (!place || place->off)
        return 1;
    if (!check)
        remember(&place->answers, key, key_size, answer, answer_size);
    else if (answer_size != known_size || memcmp(answer, known, answer_size))
        turn_off(place, table, function);
    return 1;
}

int client_lua_call_pure(struct ClientLua *lua, const char *function, const char *format, ...)
{
    va_list values;
    int ok;
    va_start(values, format);
    ok = pure_call(lua, NULL, function, format, values);
    va_end(values);
    return ok;
}

int client_lua_call_table_pure(struct ClientLua *lua, const char *table, const char *function,
                               const char *format, ...)
{
    va_list values;
    int ok;
    if (!table) return 0;
    va_start(values, format);
    ok = pure_call(lua, table, function, format, values);
    va_end(values);
    return ok;
}

void client_lua_forget_answers(struct ClientLua *lua)
{
    int i;
    if (!lua) return;
    for (i = 0; i < LUA_MEMO_FUNCTIONS; ++i) {
        struct LuaMemo *answers = &lua->answers.functions[i].answers;
        free(answers->entries);
        lua_memo_init(answers, NULL, 0);
    }
}

void client_lua_release_answers(struct ClientLua *lua)
{
    if (!lua) return;
    client_lua_forget_answers(lua);
    lua_memo_book_init(&lua->answers);
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
