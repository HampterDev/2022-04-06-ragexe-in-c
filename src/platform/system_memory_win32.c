#ifdef _WIN32
#include "platform/system_memory.h"
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
#endif
