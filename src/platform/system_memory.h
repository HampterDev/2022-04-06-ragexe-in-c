#ifndef PLATFORM_SYSTEM_MEMORY_H
#define PLATFORM_SYSTEM_MEMORY_H

#include <stddef.h>
#include <stdint.h>

void platform_heap_compact(void);
uint64_t platform_physical_memory(void);

#endif
