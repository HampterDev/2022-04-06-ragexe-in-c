#ifndef STD_CONTAINER_MEMORY_H
#define STD_CONTAINER_MEMORY_H
#include <stddef.h>

void *std_allocate_bytes(size_t bytes);
void *std_allocate(size_t count, size_t elem_size, size_t max_count);
#endif
