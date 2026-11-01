#include "lua.h"
#include "lauxlib.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Bytes {
    char *data;
    size_t size;
};

static int save_chunk(lua_State *state, const void *data, size_t size, void *context)
{
    struct Bytes *bytes = context;
    char *next = realloc(bytes->data, bytes->size + size);
    (void)state;
    assert(next);
    memcpy(next + bytes->size, data, size);
    bytes->data = next;
    bytes->size += size;
    return 0;
}

static void check_results(lua_State *state)
{
    size_t size;
    const char *result;
    assert(lua_pcall(state, 0, 3, 0) == 0);
    result = lua_tolstring(state, -3, &size);
    assert(size == 11);
    assert(!memcmp(result, "\276\310\263\347\0suffix", size));
    assert(lua_tonumber(state, -2) == 42);
    assert(lua_toboolean(state, -1));
    lua_settop(state, 0);
}

int main(int argc, char **argv)
{
    lua_State *state = luaL_newstate();
    FILE *file;
    struct Bytes native = {0};
    unsigned char data[4096], changed[4096];
    size_t size, i;
    assert(state && argc == 2);
    file = fopen(argv[1], "rb");
    assert(file);
    size = fread(data, 1, sizeof data, file);
    assert(size > 12 && size < sizeof data && !ferror(file));
    assert(!fclose(file));
    assert(data[8] == 4);
    assert(luaL_loadbuffer(state, (const char *)data, size, "fixture") == 0);
    assert(lua_dump(state, save_chunk, &native) == 0);
    check_results(state);
    assert((unsigned char)native.data[8] == sizeof(size_t));
    assert(luaL_loadbuffer(state, native.data, native.size, "native") == 0);
    check_results(state);
    free(native.data);

    for (i = 1; i < size; ++i) {
        assert(luaL_loadbuffer(state, (const char *)data, i, "truncated") == LUA_ERRSYNTAX);
        lua_settop(state, 0);
    }
    for (i = 1; i < 12; ++i) {
        memcpy(changed, data, size);
        changed[i] ^= 0x40;
        assert(luaL_loadbuffer(state, (const char *)changed, size, "bad-header") == LUA_ERRSYNTAX);
        lua_settop(state, 0);
    }
    memcpy(changed, data, size);
    assert(data[12] && !data[13] && !data[14] && !data[15]);
    changed[16 + data[12] - 1] = 1;
    assert(luaL_loadbuffer(state, (const char *)changed, size, "bad-string") == LUA_ERRSYNTAX);
    lua_close(state);
    puts("Lua: Win32 chunks, native round-trip, nested closures and malformed input passed");
    return 0;
}
