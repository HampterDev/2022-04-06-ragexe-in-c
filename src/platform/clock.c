#include "platform/clock.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

uint32_t platform_ticks_ms(void)
{
    return (uint32_t)timeGetTime();
}
#else
#include <stdlib.h>
#include <time.h>

uint32_t platform_ticks_ms(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
        abort();
    return (uint32_t)((uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000);
}
#endif
