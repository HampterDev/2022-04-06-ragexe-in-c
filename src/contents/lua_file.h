#ifndef CONTENTS_LUA_FILE_H
#define CONTENTS_LUA_FILE_H

#include <lauxlib.h>
#include <lualib.h>

#ifdef _WIN32
static __inline int lua_load_disk_file(lua_State *L, const char *path)
{
    return luaL_loadfile(L, path);
}

static __inline void lua_open_client_libs(lua_State *L)
{
    luaL_openlibs(L);
}
#else
int lua_load_disk_file(lua_State *L, const char *path);
void lua_open_client_libs(lua_State *L);
#endif

#endif
