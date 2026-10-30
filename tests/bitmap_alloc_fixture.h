#include <stdlib.h>
void *bitmap_test_malloc(size_t size);
void *bitmap_test_calloc(size_t count, size_t size);
void *bitmap_test_realloc(void *memory, size_t size);
void bitmap_test_free(void *memory);
#define malloc bitmap_test_malloc
#define calloc bitmap_test_calloc
#define realloc bitmap_test_realloc
#define free bitmap_test_free
