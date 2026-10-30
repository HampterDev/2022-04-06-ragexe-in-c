#include "contents/client_lua.h"
#include "fileio/file.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <lauxlib.h>
#include <lualib.h>

struct ClientLua *client_lua_construct(struct ClientLua *self)
{
    if (self) self->L = NULL;
    return self;
}

void client_lua_close(struct ClientLua *self)
{
    if (self && self->L) {
        lua_close(self->L);
        self->L = NULL;
    }
}

void client_lua_destruct(struct ClientLua *self)
{
    client_lua_close(self);
}

static int open_libraries(lua_State *state)
{
    struct ClientLua *self = lua_touserdata(state, 1);
    luaL_openlibs(state);
    client_lua_register_functions(self);
    return 0;
}

void client_lua_open(struct ClientLua *self)
{
    if (!self) return;
    client_lua_close(self);
    self->L = luaL_newstate();
    if (self->L && lua_cpcall(self->L, open_libraries, self))
        client_lua_close(self);
}

static int script_exists(const char *path, char via_cfile, char disk_only)
{
    if (via_cfile) return file_make_file_name(path, disk_only);
    return game_file_mgr_is_file_exist(&g_fileMgr, path, disk_only);
}

char client_lua_load_file(struct ClientLua *self, const char *name, char via_cfile, char disk_only)
{
    static const char prefix[] = "LuaFiles514\\";
    struct File file;
    const unsigned char *text;
    char *path;
    size_t length, start;
    int base, status;
    lua_State *state;

    if (!self || !self->L || !name) return 0;
    start = disk_only ? 0 : sizeof prefix - 1;
    length = strlen(name);
    if (length > SIZE_MAX - start - 5) return 0;
    path = malloc(start + length + 5);
    if (!path) return 0;
    memcpy(path, prefix, start);
    memcpy(path + start, name, length);
    length += start;
    memcpy(path + length, ".lub", 5);
    if (!script_exists(path, via_cfile, disk_only)) {
        memcpy(path + length, ".lua", 5);
        if (!script_exists(path, via_cfile, disk_only)) {
            free(path);
            return 0;
        }
    }
    file_construct(&file);
    status = file_open(&file, path, 0, via_cfile, disk_only);
    free(path);
    if (!status) {
        file_destruct(&file);
        return 0;
    }
    text = file_consume(&file, (size_t)file.size, 1);
    if (!text) {
        file_destruct(&file);
        return 0;
    }
    state = self->L;
    base = lua_gettop(state);
    status = luaL_loadbuffer(state, (const char *)text, (size_t)file.size, "buf");
    file_destruct(&file);
    if (!status) status = lua_pcall(state, 0, 0, 0);
    if (status) {
        const char *error = lua_type(state, -1) == LUA_TSTRING ? lua_tostring(state, -1) : NULL;
        client_lua_error_box(self, name, "%s\n", error ? error : "Lua script failed");
    }
    lua_settop(state, base);
    return status == 0;
}
