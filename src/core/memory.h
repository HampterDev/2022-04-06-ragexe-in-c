#ifndef CORE_MEMORY_H
#define CORE_MEMORY_H

#include <stddef.h>

void *mem_alloc(size_t bytes);
void *mem_try_alloc(size_t bytes);
void mem_free(void *memory);

#endif
