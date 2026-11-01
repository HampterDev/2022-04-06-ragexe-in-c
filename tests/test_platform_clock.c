#include "platform/clock.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

int main(void)
{
    uint32_t start = platform_ticks_ms(), end;
    clock_t deadline = clock() + CLOCKS_PER_SEC;
    do {
        end = platform_ticks_ms();
        assert((uint32_t)(end - start) < 60000);
    } while (end == start && clock() < deadline);
    assert(end != start);
    puts("Platform clock: monotonic milliseconds advance");
    return 0;
}
