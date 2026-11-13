#include "contents/client_lua.h"
#include "contents/client_lua_calls.h"
#include <lauxlib.h>
#include <lualib.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int errors;
static char last_error[500];

static void message_box(HWND window, const char *text, const char *caption, unsigned flags)
{
    assert(!window && !flags && caption && strlen(text) < sizeof last_error);
    strcpy(last_error, text);
    ++errors;
}
static int logs;
static char last_log[256];

static void debug_output(const char *text)
{
    assert(strlen(text) < sizeof last_log);
    strcpy(last_log, text);
    ++logs;
}
#include "platform/window.h"
#include "platform/system.h"
#define MessageBoxA message_box
#define platform_message_box message_box
#define platform_debug_output debug_output
#ifdef LUA_CALL_ORACLE
#include "std/string.h"
#include "world_state_memory.h"
#include "lua_call_oracle.h"
static struct String old_name(const char *text)
{
    struct String value;
    string_init(&value);
    string_assign_cstr(&value, text);
    return value;
}
#define CALL(lua, name, ...) LuaCall(lua, old_name(name), __VA_ARGS__)
#define TABLE(lua, table, name, ...) lua_call_table_function(lua, old_name(table), old_name(name), __VA_ARGS__)
#else
#include "../src/contents/lua_calls.c"
#include "fixtures/lua_calls_reference.h"
#define CALL(lua, name, ...) client_lua_call(lua, name, __VA_ARGS__)
#define TABLE(lua, table, name, ...) client_lua_call_table(lua, table, name, __VA_ARGS__)
#endif
#undef MessageBoxA
#undef platform_message_box
#undef platform_debug_output

static const char script[] =
    "function mixed(b,d,l,s) return b,d+3,l*2,s end\n"
    "function coerce() return 0,'-27.75','3.5',12345 end\n"
    "function nils() return nil,nil,nil,nil end\n"
    "function empty() end\n"
    "function allocate(s) return 123,string.rep(s,4),987654321,123456789 end\n"
    "function fail() error('intentional failure') end\n"
    "function long_error() error(string.rep('x',10000)) end\n"
    "function object_error() error({}) end\n"
    "T={mixed=mixed,empty=empty}\n"
    "Broken=setmetatable({}, {__index=function() error('lookup failed') end})\n"
    "Numbers={[1]=42,[2]='-17.9',[3]='bad',[4]=false}\n";

static void prepare(lua_State *state)
{
    assert(state);
    luaL_openlibs(state);
    assert(!luaL_dostring(state, script));
    lua_gc(state, LUA_GCSTOP, 0);
}

static void nominal(struct ClientLua *lua, char output[2048])
{
    char boolean;
    int integer, mode, count = 0;
    double number;
    const char *string;
    for (mode = 0; mode < 4; ++mode) {
        int ok;
        boolean = 9; integer = 999; number = -999; string = NULL;
        if (mode & 1)
            ok = TABLE(lua, "T", "mixed", "bdls>bdls", mode ? 257 : 256, -19, 3.125, "테스트", &boolean, &integer, &number, &string);
        else
            ok = CALL(lua, "mixed", "bdls>bdls", mode ? 257 : 256, -19, 3.125, "테스트", &boolean, &integer, &number, &string);
        assert(ok && string);
        count += sprintf(output + count, "%d|%d|%.17g|%s\n", boolean, integer, number, string);
    }
    assert(CALL(lua, "coerce", ">bdls", &boolean, &integer, &number, &string));
    count += sprintf(output + count, "%d|%d|%.17g|%s\n", boolean, integer, number, string);
    assert(CALL(lua, "nils", ">bdls", &boolean, &integer, &number, &string));
    count += sprintf(output + count, "%d|%d|%.17g|%s\n", boolean, integer, number, string ? string : "<null>");
    assert(CALL(lua, "empty", "") && TABLE(lua, "T", "empty", ""));
    assert(count < 2048 && !errors);
}

#ifndef LUA_CALL_ORACLE
static void stack_and_errors(struct ClientLua *lua)
{
    lua_State *state = lua->L;
    struct ClientLua empty = {0};
    int value = 765, i;
    char format[9001];
    lua_settop(state, 0);
    lua_newtable(state);
    lua_pushinteger(state, 908);
    for (i = 0; i < 1000; ++i) {
        assert(client_lua_call(lua, "empty", ""));
        assert(client_lua_call_table(lua, "T", "empty", ""));
        assert(lua_gettop(state) == 2 && lua_istable(state, 1) && lua_tointeger(state, 2) == 908);
    }
    lua_gc(state, LUA_GCRESTART, 0);
    lua_gc(state, LUA_GCSETPAUSE, 1);
    lua_gc(state, LUA_GCSETSTEPMUL, 100000);
    for (i = 0; i < 100; ++i) {
        const char *a, *b, *c;
        assert(client_lua_call(lua, "allocate", "s>dsss", "temporary string", &value, &a, &b, &c));
        assert(value == 123 && strlen(a) == 64 && !strcmp(b, "987654321") && !strcmp(c, "123456789"));
    }
    lua_gc(state, LUA_GCSTOP, 0);
    value = 765;
    assert(!client_lua_call(NULL, "empty", ""));
    assert(!client_lua_call(&empty, "empty", ""));
    assert(!client_lua_call(lua, NULL, ""));
    assert(!client_lua_call(lua, "empty", NULL));
    assert(!client_lua_call_table(lua, NULL, "empty", ""));
    assert(!client_lua_call(lua, "empty", "z>d", &value));
    assert(!client_lua_call(lua, "empty", ">>d", &value));
    assert(!client_lua_call(lua, "absent", ">d", &value));
    assert(!client_lua_call_table(lua, "absent", "empty", ">d", &value));
    assert(!client_lua_call_table(lua, "Broken", "absent", ">d", &value));
    assert(!client_lua_call(lua, "fail", ">d", &value));
    assert(value == 765 && lua_gettop(state) == 2);
    assert(!client_lua_call(lua, "long_error", "") && strlen(last_error) == sizeof last_error - 1);
    assert(!client_lua_call(lua, "object_error", "") && !strcmp(last_error, "Lua call failed\n"));
    memset(format, 'd', sizeof format - 1); format[sizeof format - 1] = 0;
    assert(!client_lua_call(lua, "empty", format) && lua_gettop(state) == 2);
    assert(client_lua_table_int(lua, "Numbers", 1) == 42);
    assert(client_lua_table_int(lua, "Numbers", 2) == -17);
    assert(client_lua_table_int(lua, "Numbers", 3) == -1);
    assert(client_lua_table_int(lua, "Numbers", 4) == -1);
    assert(client_lua_table_int(lua, "Numbers", 5) == -1);
    assert(client_lua_table_int(lua, "absent", 1) == -1);
    assert(client_lua_table_int(lua, "Broken", 1) == -1);
    assert(client_lua_table_int(lua, NULL, 1) == -1);
    assert(client_lua_table_int(&empty, "Numbers", 1) == 0);
    assert(client_lua_table_int(NULL, "Numbers", 1) == 0);
    assert(lua_gettop(state) == 2 && lua_tointeger(state, 2) == 908);
}

static long lua_calls_made(lua_State *state)
{
    long calls;
    lua_getglobal(state, "calls");
    calls = (long)lua_tointeger(state, -1);
    lua_pop(state, 1);
    return calls;
}

static int no_storage(const struct ClientLua *lua)
{
    int i;
    for (i = 0; i < LUA_MEMO_FUNCTIONS; ++i)
        if (lua->answers.functions[i].answers.entries || lua_memo_slots(&lua->answers.functions[i].answers))
            return 0;
    return 1;
}

static int no_functions(const struct ClientLua *lua)
{
    int i;
    for (i = 0; i < LUA_MEMO_FUNCTIONS; ++i)
        if (lua->answers.functions[i].name_size || lua->answers.functions[i].off) return 0;
    return no_storage(lua);
}

static void pure_answers(struct ClientLua *lua)
{
    lua_State *state = lua->L;
    char greater = 9, flag = 0, long_name[80];
    int sum = 0, top;
    double half = 0;
    assert(!luaL_dostring(state, "calls = 0\n"
                                 "function counted(a, b) calls = calls + 1 return a + b, a > b end\n"
                                 "function halve(x, on) calls = calls + 1 return x / 2, not on end\n"
                                 "function length(s) calls = calls + 1 return #s end\n"
                                 "function noted(x) calls = calls + 1 end\n"
                                 "T.counted = counted\n"));
    top = lua_gettop(state);
    assert(client_lua_call_pure(lua, "counted", "dd>db", 2, 3, &sum, &greater) && sum == 5 && !greater);
    assert(lua_calls_made(state) == 1);
    sum = 0; greater = 9;
    assert(client_lua_call_pure(lua, "counted", "dd>db", 2, 3, &sum, &greater) && sum == 5 && !greater);
    assert(lua_calls_made(state) == 1 && lua_gettop(state) == top);
    assert(client_lua_call_pure(lua, "counted", "dd>db", 3, 2, &sum, &greater) && sum == 5 && greater == 1);
    assert(lua_calls_made(state) == 2);
    assert(client_lua_call_table_pure(lua, "T", "counted", "dd>db", 2, 3, &sum, &greater) && sum == 5);
    assert(client_lua_call_table_pure(lua, "T", "counted", "dd>db", 2, 3, &sum, &greater) && sum == 5);
    assert(lua_calls_made(state) == 3);
    assert(client_lua_call_pure(lua, "halve", "lb>lb", 5.0, 1, &half, &flag) && half == 2.5 && !flag);
    assert(client_lua_call_pure(lua, "halve", "lb>lb", 5.0, 0, &half, &flag) && half == 2.5 && flag == 1);
    half = 0; flag = 9;
    assert(client_lua_call_pure(lua, "halve", "lb>lb", 5.0, 256, &half, &flag) && half == 2.5 && flag == 1);
    assert(lua_calls_made(state) == 5);
    /* Calls that are not remembered: a string among the values, a format without results (with
       or without '>': its walk would have had nothing to stop it), a name too long to keep. */
    assert(client_lua_call_pure(lua, "length", "s>d", "four", &sum) && sum == 4);
    assert(client_lua_call_pure(lua, "length", "s>d", "four", &sum) && sum == 4);
    assert(lua_calls_made(state) == 7);
    assert(client_lua_call_pure(lua, "noted", "d", 5) && client_lua_call_pure(lua, "noted", "d", 5));
    assert(client_lua_call_pure(lua, "noted", "d>", 5) && client_lua_call_pure(lua, "noted", "d>", 5));
    assert(client_lua_call_table_pure(lua, "T", "counted", "dd", 1, 2) && lua_calls_made(state) == 12);
    assert(client_lua_call_pure(lua, "noted", "", 5) && client_lua_call_pure(lua, "noted", ">", 5));
    assert(lua_calls_made(state) == 14 && lua_gettop(state) == top);
    memset(long_name, 'x', sizeof long_name - 1); long_name[sizeof long_name - 1] = 0;
    assert(!luaL_dostring(state, "_G[string.rep('x', 79)] = counted"));
    assert(client_lua_call_pure(lua, long_name, "dd>db", 1, 1, &sum, &greater) && sum == 2);
    assert(client_lua_call_pure(lua, long_name, "dd>db", 1, 1, &sum, &greater) && sum == 2);
    assert(lua_calls_made(state) == 16);
    errors = 0;
    assert(!client_lua_call_pure(lua, "fail", ">d", &sum) && !client_lua_call_pure(lua, "fail", ">d", &sum));
    assert(errors == 2 && !client_lua_call_pure(lua, "absent", "d>d", 1, &sum) && errors == 3);
    assert(!client_lua_call_pure(lua, "counted", "z>d", &sum) && !client_lua_call_table_pure(lua, NULL, "counted", ""));
    assert(!client_lua_call_pure(NULL, "counted", "dd>d", 1, 2, &sum));
    errors = 0;
    client_lua_forget_answers(lua);
    assert(no_storage(lua) && !no_functions(lua));
    assert(client_lua_call_pure(lua, "counted", "dd>db", 2, 3, &sum, &greater) && sum == 5);
    assert(lua_calls_made(state) == 17);
    client_lua_release_answers(lua);
    assert(no_functions(lua));
}

/* One answer in LUA_MEMO_CHECK_EVERY is asked of Lua again: a function whose answer changed, or
   that fails now, is found out, said once and called every time from then on, past a script
   load, until the state is opened again; a function that answers the same stays remembered. */
static void pure_checks(struct ClientLua *lua)
{
    lua_State *state = lua->L;
    struct LuaMemoFunction *place;
    long calls;
    int answer = 0, i, before = logs;
    assert(!luaL_dostring(state, "calls = 0 bump = 0 broken = false\n"
                                 "function drifting(x) calls = calls + 1 return x + bump end\n"
                                 "function steady(x) calls = calls + 1 return x * 2 end\n"
                                 "function flaky(x) calls = calls + 1 if broken then error('broken') end return x end\n"));
    assert(client_lua_call_pure(lua, "drifting", "d>d", 1, &answer) && answer == 1);
    assert(!luaL_dostring(state, "bump = 100"));
    for (i = 1; i < LUA_MEMO_CHECK_EVERY; ++i)
        assert(client_lua_call_pure(lua, "drifting", "d>d", 1, &answer) && answer == 1);
    assert(lua_calls_made(state) == 1 && logs == before);
    assert(client_lua_call_pure(lua, "drifting", "d>d", 1, &answer) && answer == 101);
    assert(lua_calls_made(state) == 2 && logs == before + 1 && strstr(last_log, "drifting"));
    place = lua_memo_book_function(&lua->answers, NULL, "drifting");
    assert(place && place->off && !place->answers.entries);
    for (i = 0; i < 10; ++i)
        assert(client_lua_call_pure(lua, "drifting", "d>d", 1, &answer) && answer == 101);
    assert(lua_calls_made(state) == 12 && logs == before + 1);

    assert(client_lua_call_pure(lua, "steady", "d>d", 3, &answer) && answer == 6);
    for (i = 0; i < 4 * LUA_MEMO_CHECK_EVERY; ++i)
        assert(client_lua_call_pure(lua, "steady", "d>d", 3, &answer) && answer == 6);
    assert(lua_calls_made(state) == 17 && logs == before + 1);

    assert(client_lua_call_pure(lua, "flaky", "d>d", 4, &answer) && answer == 4);
    assert(!luaL_dostring(state, "broken = true"));
    for (i = 1; i < LUA_MEMO_CHECK_EVERY; ++i)
        assert(client_lua_call_pure(lua, "flaky", "d>d", 4, &answer) && answer == 4);
    errors = 0;
    answer = 0;
    assert(!client_lua_call_pure(lua, "flaky", "d>d", 4, &answer) && !answer && errors == 1);
    assert(logs == before + 2 && strstr(last_log, "flaky"));
    assert(!client_lua_call_pure(lua, "flaky", "d>d", 4, &answer) && errors == 2 && logs == before + 2);
    assert(!luaL_dostring(state, "broken = false"));
    calls = lua_calls_made(state);
    assert(client_lua_call_pure(lua, "flaky", "d>d", 4, &answer) && answer == 4);
    assert(client_lua_call_pure(lua, "flaky", "d>d", 4, &answer) && lua_calls_made(state) == calls + 2);

    client_lua_forget_answers(lua);
    calls = lua_calls_made(state);
    assert(client_lua_call_pure(lua, "drifting", "d>d", 1, &answer) && answer == 101);
    assert(client_lua_call_pure(lua, "drifting", "d>d", 1, &answer) && lua_calls_made(state) == calls + 2);
    assert(client_lua_call_pure(lua, "steady", "d>d", 3, &answer) && answer == 6);
    assert(client_lua_call_pure(lua, "steady", "d>d", 3, &answer) && lua_calls_made(state) == calls + 3);

    client_lua_release_answers(lua);
    assert(no_functions(lua));
    assert(client_lua_call_pure(lua, "drifting", "d>d", 1, &answer) && answer == 101);
    assert(client_lua_call_pure(lua, "drifting", "d>d", 1, &answer) && lua_calls_made(state) == calls + 4);
    assert(logs == before + 2);
    client_lua_release_answers(lua);
}

/* Each function's answers are kept apart and bounded: one function with many keys takes at most
   ANSWERS_MOST slots and pushes out none of another's answers; past the first LUA_MEMO_FUNCTIONS
   functions, calls go to Lua every time. */
static void pure_bounds(struct ClientLua *lua)
{
    lua_State *state = lua->L;
    struct LuaMemoFunction *place;
    char sum_greater = 0, name[8];
    int answer = 0, i;
    long calls;
    assert(!luaL_dostring(state, "calls = 0\n"
                                 "function kept(x) calls = calls + 1 return x + 1 end\n"
                                 "for i = 0, 9 do _G['g' .. i] = function(x) calls = calls + 1 return x end end\n"));
    assert(client_lua_call_pure(lua, "kept", "d>d", 7, &answer) && answer == 8);
    for (i = 0; i < 20000; ++i)
        assert(client_lua_call_pure(lua, "counted", "dd>db", i, -i, &answer, &sum_greater) && answer == 0
               && sum_greater == (i > 0));
    assert(lua_calls_made(state) == 20001);
    place = lua_memo_book_function(&lua->answers, NULL, "counted");
    assert(place && lua_memo_slots(&place->answers) == ANSWERS_MOST && place->answers.count <= ANSWERS_MOST);
    assert(client_lua_call_pure(lua, "kept", "d>d", 7, &answer) && answer == 8 && lua_calls_made(state) == 20001);
    for (i = 20000 - 100; i < 20000; ++i)
        assert(client_lua_call_pure(lua, "counted", "dd>db", i, -i, &answer, &sum_greater) && answer == 0);
    assert(lua_calls_made(state) == 20001);
    client_lua_release_answers(lua);

    for (i = 0; i < 10; ++i) {
        sprintf(name, "g%d", i);
        calls = lua_calls_made(state);
        assert(client_lua_call_pure(lua, name, "d>d", i, &answer) && answer == i);
        assert(client_lua_call_pure(lua, name, "d>d", i, &answer) && answer == i);
        assert(lua_calls_made(state) == calls + (i < LUA_MEMO_FUNCTIONS ? 1 : 2));
    }
    client_lua_release_answers(lua);
    assert(no_functions(lua));
    client_lua_forget_answers(NULL);
    client_lua_release_answers(NULL);
}

struct Memory { long attempts, fail_at; size_t blocks; };
static void *memory(void *context, void *pointer, size_t old_size, size_t size)
{
    struct Memory *owner = context;
    void *result;
    if (!size) {
        if (pointer) { free(pointer); --owner->blocks; }
        return NULL;
    }
    if (size > old_size && owner->attempts++ == owner->fail_at) return NULL;
    result = realloc(pointer, size);
    assert(result);
    if (!pointer) ++owner->blocks;
    return result;
}

static long allocation_case(long fail_at)
{
    struct Memory owner = {0, -1, 0};
    struct ClientLua lua = {lua_newstate(memory, &owner)};
    char input[1025];
    const char *a = "untouched", *b = "untouched", *c = "untouched";
    int number = -987, ok;
    long attempts;
    prepare(lua.L);
    lua_pushinteger(lua.L, 908);
    memset(input, 'a', sizeof input - 1); input[sizeof input - 1] = 0;
    owner.attempts = 0; owner.fail_at = fail_at;
    ok = client_lua_call(&lua, "allocate", "s>dsss", input, &number, &a, &b, &c);
    attempts = owner.attempts;
    if (ok) {
        assert(number == 123 && strlen(a) == 4096 && !strcmp(b, "987654321") && !strcmp(c, "123456789"));
    } else {
        assert(number == -987 && !strcmp(a, "untouched") && !strcmp(b, "untouched") && !strcmp(c, "untouched"));
    }
    assert(lua_gettop(lua.L) == 1 && lua_tointeger(lua.L, 1) == 908);
    owner.attempts = 0; owner.fail_at = 0;
    assert(client_lua_table_int(&lua, "Numbers", 1) == -1);
    owner.fail_at = -1;
    assert(client_lua_table_int(&lua, "Numbers", 1) == 42);
    assert(lua_gettop(lua.L) == 1 && lua_tointeger(lua.L, 1) == 908);
    lua_close(lua.L);
    assert(!owner.blocks);
    if (fail_at >= 0) assert(!ok && attempts > fail_at);
    return attempts;
}
#endif

int main(void)
{
    struct ClientLua lua = {luaL_newstate()};
    char output[2048] = {0};
    prepare(lua.L);
    nominal(&lua, output);
#ifdef LUA_CALL_ORACLE
    {
        FILE *file = fopen("lua_calls_reference.bin", "wb");
        assert(file && fwrite(output, 1, strlen(output), file) == strlen(output) && !fclose(file));
    }
    assert(!live_blocks);
#else
    {
        long i, allocations;
        assert(strlen(output) == sizeof lua_calls_reference - 1 && !memcmp(output, lua_calls_reference, sizeof lua_calls_reference));
        stack_and_errors(&lua);
        pure_answers(&lua);
        pure_checks(&lua);
        pure_bounds(&lua);
        allocations = allocation_case(-1);
        assert(allocations > 2);
        for (i = 0; i < allocations; ++i) allocation_case(i);
        printf("Lua calls: archived values, borrowed names, stack ownership, protected errors, bounded messages, numeric lookups, remembered answers (checked against Lua, bounded and apart for each function) and %ld allocation failures passed\n", allocations);
    }
#endif
    lua_close(lua.L);
    return 0;
}
