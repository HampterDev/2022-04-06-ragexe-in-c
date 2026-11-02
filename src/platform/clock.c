#include "platform/clock.h"

#ifdef _WIN32
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
#else
#include <stdio.h>
#include <stdlib.h>

static struct timespec clock_now(clockid_t clock)
{
    struct timespec now;
    if (clock_gettime(clock, &now))
        abort();
    return now;
}

uint32_t platform_ticks_ms(void)
{
    struct timespec now = clock_now(CLOCK_MONOTONIC);
    return (uint32_t)((uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000);
}

uint32_t platform_coarse_ticks_ms(void)
{
    return platform_ticks_ms();
}

int64_t platform_perf_counter(void)
{
    struct timespec now = clock_now(CLOCK_MONOTONIC);
    return (int64_t)now.tv_sec * 1000000000 + now.tv_nsec;
}

int64_t platform_perf_frequency(void)
{
    return 1000000000;
}

unsigned platform_timer_period_begin(void)
{
    return 0;
}

void platform_timer_period_end(unsigned period)
{
    (void)period;
}

static int local_time(int32_t seconds, struct tm *out)
{
    time_t stamp = seconds;
    if (seconds < 0)
        return 0;
    tzset();
    return localtime_r(&stamp, out) != NULL;
}

int32_t platform_time_now(void)
{
    time_t now = time(NULL);
    return now >= 0 && now <= INT32_MAX ? (int32_t)now : -1;
}

struct tm *platform_time_local(int32_t seconds)
{
    static _Thread_local struct tm local;
    return local_time(seconds, &local) ? &local : NULL;
}

int32_t platform_time_make(struct tm *local)
{
    time_t stamp = mktime(local);
    return stamp >= 0 && stamp <= INT32_MAX ? (int32_t)stamp : -1;
}

double platform_time_diff(int32_t end, int32_t start)
{
    return difftime(end, start);
}

const char *platform_time_text(int32_t seconds)
{
    static const char days[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char months[12][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    static _Thread_local char text[64];
    struct tm local;
    if (!local_time(seconds, &local))
        return NULL;
    snprintf(text, sizeof text, "%s %s%3d %02d:%02d:%02d %d\n", days[local.tm_wday],
             months[local.tm_mon], local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec,
             local.tm_year + 1900);
    return text;
}

static void copy_date_time(struct PlatformDateTime *out, const struct tm *parts, long nanoseconds)
{
    out->year = (uint16_t)(parts->tm_year + 1900);
    out->month = (uint16_t)(parts->tm_mon + 1);
    out->day_of_week = (uint16_t)parts->tm_wday;
    out->day = (uint16_t)parts->tm_mday;
    out->hour = (uint16_t)parts->tm_hour;
    out->minute = (uint16_t)parts->tm_min;
    out->second = (uint16_t)parts->tm_sec;
    out->milliseconds = (uint16_t)(nanoseconds / 1000000);
}

void platform_date_time_local(struct PlatformDateTime *out)
{
    struct timespec now = clock_now(CLOCK_REALTIME);
    struct tm parts;
    tzset();
    if (!localtime_r(&now.tv_sec, &parts))
        abort();
    copy_date_time(out, &parts, now.tv_nsec);
}

void platform_date_time_utc(struct PlatformDateTime *out)
{
    struct timespec now = clock_now(CLOCK_REALTIME);
    struct tm parts;
    if (!gmtime_r(&now.tv_sec, &parts))
        abort();
    copy_date_time(out, &parts, now.tv_nsec);
}
#endif
