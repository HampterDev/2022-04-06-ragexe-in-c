#include <stdlib.h>
#include <windows.h>
#include <lauxlib.h>
void *test_heap_alloc(size_t bytes);
void test_heap_free(void *memory);
lua_State *test_lua_newstate(void);
int test_lua_loadfile(lua_State *L, const char *filename);
int WINAPI test_message_box(HWND window, LPCSTR message, LPCSTR caption, UINT flags);
#define malloc test_heap_alloc
#define free test_heap_free
#define luaL_newstate test_lua_newstate
#define luaL_loadfile test_lua_loadfile
#include "platform/win_utf8.h"
#define win_utf8_message_box test_message_box
