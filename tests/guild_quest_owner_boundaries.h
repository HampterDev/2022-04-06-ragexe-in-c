#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
DWORD guild_quest_test_time(void);
void *test_heap_alloc(size_t size);
void *test_heap_realloc(void *memory, size_t size);
void test_heap_free(void *memory);
#define timeGetTime guild_quest_test_time
#define malloc test_heap_alloc
#define realloc test_heap_realloc
#define free test_heap_free
