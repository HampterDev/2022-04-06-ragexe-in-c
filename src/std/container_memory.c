#include "std/std.h"
#include <stdint.h>
#include <stdlib.h>

void *std_allocate_bytes(size_t bytes)
{
    return bytes ? mem_alloc(bytes) : NULL;
}

void *std_allocate(size_t count, size_t elem_size, size_t max_count)
{
    if (count > max_count || (elem_size && count > SIZE_MAX / elem_size))
        abort();
    return std_allocate_bytes(count * elem_size);
}
