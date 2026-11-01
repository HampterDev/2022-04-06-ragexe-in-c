#include "contents/frame_timer.h"
#include "platform/clock.h"

static uint32_t frameFreq = 1;

static uint32_t frameTime = 0;

int g_frameskip = 1;

void frame_timer_start(unsigned rate)
{
    frameTime = platform_ticks_ms();
    if (!rate)
        rate = 1;
    frameFreq = rate > 1000 ? 1 : 1000 / rate;
}

void frame_timer_reset(void)
{
    frameTime = platform_ticks_ms();
}

int frame_timer_step_count(void)
{
    uint32_t steps = (platform_ticks_ms() - frameTime) / frameFreq;

    frameTime += frameFreq * steps;
    return g_frameskip ? (int)steps : 1;
}
