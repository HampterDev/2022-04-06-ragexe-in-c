#include <stdlib.h>
#include "std/std.h"

void *container_test_vector_allocate(size_t bytes)
{
    struct Vec v;

    vec_init(&v);
    vec_reserve(&v, bytes, 1);
    return v.first;
}

#define CONTAINER_MEMORY_PROBE 3
#include "container_memory_unreached.h"
