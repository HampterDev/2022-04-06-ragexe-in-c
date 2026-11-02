#include "platform/system_memory.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

void *platform_pages_alloc(size_t size)
{
    return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
}

void platform_pages_free(void *pages)
{
    VirtualFree(pages, 0, MEM_RELEASE);
}

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

#define PAGE_BYTES 4096

void *platform_pages_alloc(size_t size)
{
    if (!size || size > SIZE_MAX - (PAGE_BYTES - 1))
        return NULL;
    return calloc(1, (size + PAGE_BYTES - 1) / PAGE_BYTES * PAGE_BYTES);
}

void platform_pages_free(void *pages)
{
    free(pages);
}

void platform_heap_compact(void)
{
}

uint64_t platform_physical_memory(void)
{
    long pages = sysconf(_SC_PHYS_PAGES), page_size = sysconf(_SC_PAGESIZE);

    return pages > 0 && page_size > 0 ? (uint64_t)pages * (uint64_t)page_size : 0;
}
#endif
