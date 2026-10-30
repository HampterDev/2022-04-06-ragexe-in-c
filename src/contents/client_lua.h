#ifndef CONTENTS_CLIENT_LUA_H
#define CONTENTS_CLIENT_LUA_H

#include "base.h"
#include "contents/client_lua_calls.h"
#include <lua.h>

struct ClientLua {
    lua_State *L;
};
SIZE_ASSERT(ClientLua, 4);

struct ClientLua *client_lua_construct(struct ClientLua *self);
void client_lua_destruct(struct ClientLua *self);
void client_lua_close(struct ClientLua *self);
void client_lua_open(struct ClientLua *self);
void client_lua_register_functions(struct ClientLua *self);

char client_lua_load_file(struct ClientLua *self, const char *name, char via_cfile, char disk_only);
void client_lua_error_box(struct ClientLua *self, const char *caption, const char *format, ...);

int client_lua_get_global_int(struct ClientLua *self, const char *name);
int client_lua_copy_global_string_checked(struct ClientLua *self, const char *name, char **out);
int client_lua_has_global_table(struct ClientLua *self, const char *name);

#endif
