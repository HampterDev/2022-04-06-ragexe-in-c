#ifdef _WIN32
#include "platform/clock.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

uint32_t platform_ticks_ms(void)
{
    return (uint32_t)timeGetTime();
}

uint32_t platform_coarse_ticks_ms(void)
{
    return (uint32_t)GetTickCount();
}

int64_t platform_perf_counter(void)
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
}

int64_t platform_perf_frequency(void)
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
}

unsigned platform_timer_period_begin(void)
{
    TIMECAPS caps;
    UINT period = 0;
    if (timeGetDevCaps(&caps, sizeof caps) == 0) {
        UINT p = caps.wPeriodMin > 1 ? caps.wPeriodMin : 1;

        period = caps.wPeriodMax;
        if (p < period)
            period = p;
        timeBeginPeriod(period);
    }
    return period;
}

void platform_timer_period_end(unsigned period)
{
    timeEndPeriod(period);
}

int32_t platform_time_now(void)
{
    return _time32(NULL);
}

struct tm *platform_time_local(int32_t seconds)
{
    __time32_t stamp = seconds;
    return _localtime32(&stamp);
}

int32_t platform_time_make(struct tm *local)
{
    return _mktime32(local);
}

double platform_time_diff(int32_t end, int32_t start)
{
    return _difftime32(end, start);
}

const char *platform_time_text(int32_t seconds)
{
    __time32_t stamp = seconds;
    return _ctime32(&stamp);
}

static void copy_date_time(struct PlatformDateTime *out, const SYSTEMTIME *now)
{
    out->year = now->wYear;
    out->month = now->wMonth;
    out->day_of_week = now->wDayOfWeek;
    out->day = now->wDay;
    out->hour = now->wHour;
    out->minute = now->wMinute;
    out->second = now->wSecond;
    out->milliseconds = now->wMilliseconds;
}

void platform_date_time_local(struct PlatformDateTime *out)
{
    SYSTEMTIME now;
    GetLocalTime(&now);
    copy_date_time(out, &now);
}

void platform_date_time_utc(struct PlatformDateTime *out)
{
    SYSTEMTIME now;
    GetSystemTime(&now);
    copy_date_time(out, &now);
}
#endif
