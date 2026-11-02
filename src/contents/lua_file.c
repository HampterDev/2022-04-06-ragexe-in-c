#ifndef _WIN32
#include "contents/lua_file.h"
#include "platform/filesystem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int lua_load_disk_file(lua_State *L, const char *path)
{
    FILE *file = platform_file_open(path, "rb");
    char *data = NULL, *grown, chunk[0x2000], *name;
    size_t size = 0, count, skip = 0, length = strlen(path);
    int status;
    if (!file) {
        lua_pushfstring(L, "cannot open %s", path);
        return LUA_ERRFILE;
    }
    while ((count = fread(chunk, 1, sizeof chunk, file)) > 0) {
        grown = realloc(data, size + count);
        if (!grown) {
            free(data);
            fclose(file);
            lua_pushfstring(L, "cannot read %s", path);
            return LUA_ERRMEM;
        }
        data = grown;
        memcpy(data + size, chunk, count);
        size += count;
    }
    fclose(file);
    if (size && data[0] == '#') {
        while (skip < size && data[skip] != '\n')
            skip++;
    }
    name = malloc(length + 2);
    if (!name) {
        free(data);
        lua_pushfstring(L, "cannot read %s", path);
        return LUA_ERRMEM;
    }
    name[0] = '@';
    memcpy(name + 1, path, length + 1);
    status = luaL_loadbuffer(L, data ? data + skip : "", size - skip, name);
    free(name);
    free(data);
    return status;
}
#endif
