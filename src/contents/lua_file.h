#ifndef CONTENTS_LUA_FILE_H
#define CONTENTS_LUA_FILE_H

#include <lauxlib.h>

#ifdef _WIN32
static __inline int lua_load_disk_file(lua_State *L, const char *path)
{
    return luaL_loadfile(L, path);
}
#else
int lua_load_disk_file(lua_State *L, const char *path);
#endif

#endif
