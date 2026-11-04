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

static int call_with_disk_paths(lua_State *L)
{
    int paths = (int)lua_tointeger(L, lua_upvalueindex(2));
    int args = lua_gettop(L), i;
    for (i = 1; i <= paths && i <= args; i++) {
        char disk[4096], *name;
        const char *path;
        if (lua_type(L, i) != LUA_TSTRING)
            continue;
        path = lua_tostring(L, i);
        if (platform_path_is_native(path))
            continue;
        if (!platform_path_full(path, disk, sizeof disk, &name))
            return luaL_error(L, "bad path %s", path);
        lua_pushstring(L, disk);
        lua_replace(L, i);
    }
    lua_pushvalue(L, lua_upvalueindex(1));
    lua_insert(L, 1);
    lua_call(L, args, LUA_MULTRET);
    return lua_gettop(L);
}

static void wrap_path_function(lua_State *L, const char *table, const char *name, int paths)
{
    if (table)
        lua_getglobal(L, table);
    else
        lua_pushvalue(L, LUA_GLOBALSINDEX);
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, name);
        if (lua_isfunction(L, -1)) {
            lua_pushinteger(L, paths);
            lua_pushcclosure(L, call_with_disk_paths, 2);
            lua_setfield(L, -2, name);
        } else {
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
}

void lua_open_client_libs(lua_State *L)
{
    luaL_openlibs(L);
    wrap_path_function(L, "io", "open", 1);
    wrap_path_function(L, "io", "lines", 1);
    wrap_path_function(L, "os", "remove", 1);
    wrap_path_function(L, "os", "rename", 2);
    wrap_path_function(L, NULL, "dofile", 1);
    wrap_path_function(L, NULL, "loadfile", 1);
}
#endif
