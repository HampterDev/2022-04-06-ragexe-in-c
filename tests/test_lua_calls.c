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

static int message_box(HWND window, const char *text, const char *caption, UINT flags)
{
    assert(!window && !flags && caption && strlen(text) < sizeof last_error);
    strcpy(last_error, text);
    ++errors;
    return 0;
}
#define MessageBoxA message_box
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
        allocations = allocation_case(-1);
        assert(allocations > 2);
        for (i = 0; i < allocations; ++i) allocation_case(i);
        printf("Lua calls: archived values, borrowed names, stack ownership, protected errors, bounded messages, numeric lookups and %ld allocation failures passed\n", allocations);
    }
#endif
    lua_close(lua.L);
    return 0;
}
