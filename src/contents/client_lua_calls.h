#ifndef CONTENTS_CLIENT_LUA_CALLS_H
#define CONTENTS_CLIENT_LUA_CALLS_H

struct ClientLua;

int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...);
int client_lua_call_table(struct ClientLua *lua, const char *table, const char *function,
                          const char *format, ...);

int client_lua_table_int(struct ClientLua *lua, const char *table, int key);

/* The same calls for a function that answers from its arguments and from tables its scripts set
   up as they load: each answer is remembered by the call (its format and arguments, apart for
   each function, contents/lua_memo.h) and given again without Lua until a script is loaded or the
   state is opened or closed again. A remembered answer is asked of Lua again once in
   LUA_MEMO_CHECK_EVERY; a function that then answers differently, or fails, goes to Lua every time
   while the state lives, and the debug output says so once. Only calls with values 'b', 'd' and
   'l' and at least one result are remembered; any other call (a string among its values, no
   result, too long to remember), the calls of a function past the first LUA_MEMO_FUNCTIONS, and a
   call that fails go to Lua every time. */
int client_lua_call_pure(struct ClientLua *lua, const char *function, const char *format, ...);
int client_lua_call_table_pure(struct ClientLua *lua, const char *table, const char *function,
                               const char *format, ...);
/* Forget every remembered answer, keeping the functions turned off (after a script is loaded);
   release forgets the functions too (the state opened or closed). */
void client_lua_forget_answers(struct ClientLua *lua);
void client_lua_release_answers(struct ClientLua *lua);

#endif
