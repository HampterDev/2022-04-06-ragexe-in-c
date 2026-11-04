#include "lua.h"
#include "lauxlib.h"
#include <stdio.h>
#include <string.h>

static int write_chunk(lua_State *state, const void *data, size_t size, void *file)
{
    (void)state;
    return fwrite(data, 1, size, file) != size;
}

int main(int argc, char **argv)
{
    lua_State *state;
    int status;
    if (argc != 2 && !(argc == 4 && !strcmp(argv[1], "--dump"))) {
        fputs("usage: lua_chunk FILE | lua_chunk --dump SOURCE OUTPUT\n", stderr);
        return 2;
    }
    state = luaL_newstate();
    if (!state)
        return 1;
    status = luaL_loadfile(state, argv[argc == 2 ? 1 : 2]);
    if (status) {
        fprintf(stderr, "%s\n", lua_tostring(state, -1));
    } else if (argc == 4) {
        FILE *file = fopen(argv[3], "wb");
        if (!file) {
            status = 1;
        } else {
            status = lua_dump(state, write_chunk, file);
            if (fclose(file))
                status = 1;
        }
    }
    lua_close(state);
    return status ? 1 : 0;
}
