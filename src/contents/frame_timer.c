#include "contents/frame_timer.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

static DWORD frameFreq = 0;

static DWORD frameTime = 0;

int g_frameskip = 1;

void frame_timer_start(unsigned rate)
{
    frameTime = timeGetTime();
    frameFreq = 1000 / rate;
}

void frame_timer_reset(void)
{
    frameTime = timeGetTime();
}

int frame_timer_step_count(void)
{
    DWORD steps = (timeGetTime() - frameTime) / frameFreq;

    frameTime += frameFreq * steps;
    return g_frameskip ? (int)steps : 1;
}
