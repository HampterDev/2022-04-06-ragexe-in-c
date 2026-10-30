#ifndef CONTENTS_CLIENT_LUA_CALLS_H
#define CONTENTS_CLIENT_LUA_CALLS_H

struct ClientLua;

int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...);
int client_lua_call_table(struct ClientLua *lua, const char *table, const char *function,
                          const char *format, ...);

int client_lua_table_int(struct ClientLua *lua, const char *table, int key);

#endif
