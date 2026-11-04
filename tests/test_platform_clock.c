#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "platform/clock.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void use_utc(void)
{
#ifdef _WIN32
    assert(_putenv_s("TZ", "UTC0") == 0);
    _tzset();
#else
    assert(setenv("TZ", "UTC0", 1) == 0);
    tzset();
#endif
}

static void test_ticks(void)
{
    uint32_t start = platform_ticks_ms(), end, coarse = platform_coarse_ticks_ms(), later;
    int64_t counter = platform_perf_counter();
    clock_t deadline = clock() + CLOCKS_PER_SEC;
    do {
        end = platform_ticks_ms();
        assert((uint32_t)(end - start) < 60000);
    } while (end == start && clock() < deadline);
    assert(end != start);
    deadline = clock() + 2 * CLOCKS_PER_SEC;
    do
        later = platform_coarse_ticks_ms();
    while (later == coarse && clock() < deadline);
    assert(later != coarse && (uint32_t)(later - coarse) < 60000);
    assert(platform_perf_frequency() > 0);
    assert(platform_perf_counter() > counter);
}

static void test_calendar(void)
{
    struct PlatformDateTime utc, local;
    struct tm *parts, copy;
    int32_t now = platform_time_now();
    time_t wall = time(NULL);
    unsigned period;

    assert(now > 1600000000 && now - wall < 5 && wall - now < 5);
    parts = platform_time_local(0);
    assert(parts && parts->tm_year == 70 && parts->tm_mon == 0 && parts->tm_mday == 1);
    assert(parts->tm_hour == 0 && parts->tm_min == 0 && parts->tm_sec == 0 && parts->tm_wday == 4);
    parts = platform_time_local(951782400);
    assert(parts && parts->tm_year == 100 && parts->tm_mon == 1 && parts->tm_mday == 29 && parts->tm_yday == 59);
    assert(!platform_time_local(-1));
    assert(strcmp(platform_time_text(0), "Thu Jan  1 00:00:00 1970\n") == 0);
    assert(strcmp(platform_time_text(1577934245), "Thu Jan  2 03:04:05 2020\n") == 0);
    assert(!platform_time_text(-5));
    copy = *platform_time_local(1577934245);
    copy.tm_mday += 31;
    assert(platform_time_make(&copy) == 1577934245 + 31 * 86400);
    assert(copy.tm_mon == 1 && copy.tm_mday == 2);
    copy.tm_year = 60;
    assert(platform_time_make(&copy) == -1);
    assert(platform_time_diff(10, 4) == 6.0 && platform_time_diff(4, 10) == -6.0);

    platform_date_time_utc(&utc);
    platform_date_time_local(&local);
    assert(utc.year >= 2024 && utc.month >= 1 && utc.month <= 12 && utc.day >= 1 && utc.day <= 31);
    assert(utc.hour < 24 && utc.minute < 60 && utc.second < 61 && utc.milliseconds < 1000 && utc.day_of_week < 7);
    assert(local.year >= 2024 && local.month >= 1 && local.month <= 12 && local.day >= 1 && local.day <= 31);
    assert(local.hour < 24 && local.minute < 60 && local.milliseconds < 1000 && local.day_of_week < 7);

    period = platform_timer_period_begin();
    if (period)
        platform_timer_period_end(period);
}

int main(void)
{
    use_utc();
    test_ticks();
    test_calendar();
    puts("Platform clock: monotonic milliseconds, coarse ticks, performance counter, 32-bit calendar time and date parts passed");
    return 0;
}
