#include "core/memory.h"
#include <stdlib.h>

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
