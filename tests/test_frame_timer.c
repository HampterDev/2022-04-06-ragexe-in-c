#include "contents/frame_timer.h"
#include "platform/clock.h"
#include <assert.h>
#include <stdio.h>

static uint32_t now;

uint32_t platform_ticks_ms(void)
{
    return now;
}

int main(void)
{
    now = UINT32_MAX - 9;
    frame_timer_start(50);
    now = 9;
    assert(frame_timer_step_count() == 0);
    now = 10;
    assert(frame_timer_step_count() == 1);
    now = 70;
    assert(frame_timer_step_count() == 3);
    assert(frame_timer_step_count() == 0);
    now = 100;
    frame_timer_reset();
    assert(frame_timer_step_count() == 0);
    g_frameskip = 0;
    assert(frame_timer_step_count() == 1);
    g_frameskip = 1;
    frame_timer_start(0);
    now += 1000;
    assert(frame_timer_step_count() == 1);
    frame_timer_start(1001);
    ++now;
    assert(frame_timer_step_count() == 1);
    puts("Frame timer: wraparound, catch-up, reset and rate bounds passed");
    return 0;
}
