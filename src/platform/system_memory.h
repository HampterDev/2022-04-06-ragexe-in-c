#ifndef PLATFORM_SYSTEM_MEMORY_H
#define PLATFORM_SYSTEM_MEMORY_H

#include <stddef.h>
#include <stdint.h>

void *platform_pages_alloc(size_t size);
void platform_pages_free(void *pages);
void platform_heap_compact(void);
uint64_t platform_physical_memory(void);

#endif
