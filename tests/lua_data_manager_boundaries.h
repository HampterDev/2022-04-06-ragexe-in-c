#include <stdlib.h>
#include <windows.h>
#include <lauxlib.h>
void *test_heap_alloc(size_t bytes);
void test_heap_free(void *memory);
lua_State *test_lua_newstate(void);
int test_lua_loadfile(lua_State *L, const char *filename);
void test_message_box(HWND window, const char *message, const char *caption, unsigned flags);
#define malloc test_heap_alloc
#define free test_heap_free
#define luaL_newstate test_lua_newstate
#define luaL_loadfile test_lua_loadfile
#include "platform/window.h"
#define MessageBoxA test_message_box
#define platform_message_box test_message_box
