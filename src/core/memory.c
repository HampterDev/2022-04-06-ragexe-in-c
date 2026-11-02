#include "core/memory.h"
#include <stdlib.h>
#include <string.h>

void *mem_try_alloc(size_t bytes)
{
    return malloc(bytes ? bytes : 1);
}

void *mem_alloc(size_t bytes)
{
    void *memory = mem_try_alloc(bytes);
    if (!memory)
        abort();
    return memory;
}

void mem_free(void *memory)
{
    free(memory);
}

void mem_copy_checked(void *dst, size_t size, const void *src, size_t count)
{
    if (count > size)
        abort();
    memcpy(dst, src, count);
}
