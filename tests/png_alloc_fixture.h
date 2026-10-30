#include <stdlib.h>
void *png_test_malloc(size_t size);
void *png_test_calloc(size_t count, size_t size);
void png_test_free(void *memory);
#define malloc png_test_malloc
#define calloc png_test_calloc
#define free png_test_free
