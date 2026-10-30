#include <stdlib.h>
#include <lauxlib.h>
void *test_heap_alloc(size_t bytes);
void test_heap_free(void *memory);
lua_State *test_lua_newstate(void);
#define malloc test_heap_alloc
#define free test_heap_free
#define luaL_newstate test_lua_newstate
