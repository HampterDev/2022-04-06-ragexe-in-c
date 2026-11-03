#ifndef _WIN32
#include "platform/system_memory.h"
#include <stdlib.h>
#include <unistd.h>

void platform_heap_compact(void)
{
}

uint64_t platform_physical_memory(void)
{
    long pages = sysconf(_SC_PHYS_PAGES), page_size = sysconf(_SC_PAGESIZE);

    return pages > 0 && page_size > 0 ? (uint64_t)pages * (uint64_t)page_size : 0;
}
#endif
