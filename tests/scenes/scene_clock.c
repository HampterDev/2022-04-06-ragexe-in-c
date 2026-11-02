/* Virtual clock for scene runs: time only moves when the driver advances it, one step per frame,
   and the wall clock is a fixed date in UTC, so every run sees the same times. */
#include "platform/clock.h"
#include "scene.h"
#include <stdio.h>
#include <string.h>

#define SCENE_EPOCH 1767268800 /* 2026-01-01 12:00:00 UTC */

static volatile uint32_t g_ticks = 100000;

void scene_clock_advance(uint32_t ms)
{
    g_ticks += ms;
}

uint32_t platform_ticks_ms(void)
{
    return g_ticks;
}

uint32_t platform_coarse_ticks_ms(void)
{
    return g_ticks;
}

int64_t platform_perf_counter(void)
{
    return (int64_t)g_ticks * 1000;
}

int64_t platform_perf_frequency(void)
{
    return 1000000;
}

unsigned platform_timer_period_begin(void)
{
    return 0;
}

void platform_timer_period_end(unsigned period)
{
    (void)period;
}

int32_t platform_time_now(void)
{
    return SCENE_EPOCH + (int32_t)((g_ticks - 100000) / 1000);
}

struct tm *platform_time_local(int32_t seconds)
{
    static _Thread_local struct tm local;
    time_t stamp = seconds;
    return seconds >= 0 && gmtime_r(&stamp, &local) ? &local : NULL;
}

int32_t platform_time_make(struct tm *local)
{
    time_t stamp = timegm(local);
    return stamp >= 0 && stamp <= INT32_MAX ? (int32_t)stamp : -1;
}

double platform_time_diff(int32_t end, int32_t start)
{
    return (double)end - (double)start;
}

const char *platform_time_text(int32_t seconds)
{
    static const char days[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char months[12][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    static _Thread_local char text[64];
    struct tm *local = platform_time_local(seconds);
    if (!local)
        return NULL;
    snprintf(text, sizeof text, "%s %s%3d %02d:%02d:%02d %d\n", days[local->tm_wday],
             months[local->tm_mon], local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec,
             local->tm_year + 1900);
    return text;
}

static void now_parts(struct PlatformDateTime *out)
{
    struct tm *parts = platform_time_local(platform_time_now());
    memset(out, 0, sizeof *out);
    out->year = (uint16_t)(parts->tm_year + 1900);
    out->month = (uint16_t)(parts->tm_mon + 1);
    out->day_of_week = (uint16_t)parts->tm_wday;
    out->day = (uint16_t)parts->tm_mday;
    out->hour = (uint16_t)parts->tm_hour;
    out->minute = (uint16_t)parts->tm_min;
    out->second = (uint16_t)parts->tm_sec;
    out->milliseconds = (uint16_t)((g_ticks - 100000) % 1000);
}

void platform_date_time_local(struct PlatformDateTime *out)
{
    now_parts(out);
}

void platform_date_time_utc(struct PlatformDateTime *out)
{
    now_parts(out);
}
