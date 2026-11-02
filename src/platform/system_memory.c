#include "platform/system_memory.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

void platform_heap_compact(void)
{
    HeapCompact(GetProcessHeap(), 0);
}

uint64_t platform_physical_memory(void)
{
    MEMORYSTATUS memory;

    memset(&memory, 0, sizeof(memory));
    memory.dwLength = sizeof(memory);
    GlobalMemoryStatus(&memory);
    return memory.dwTotalPhys;
}
#else
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
