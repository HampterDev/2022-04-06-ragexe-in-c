#include <stdlib.h>
void *model_test_calloc(size_t count, size_t size);
void model_test_free(void *memory);
#define calloc model_test_calloc
#define free model_test_free
#include "../src/resources/granny_mesh.c"
