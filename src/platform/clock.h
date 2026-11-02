#ifndef PLATFORM_CLOCK_H
#define PLATFORM_CLOCK_H

#include <stdint.h>
#include <time.h>

struct PlatformDateTime {
    uint16_t year;
    uint16_t month;
    uint16_t day_of_week;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
    uint16_t milliseconds;
};

uint32_t platform_ticks_ms(void);
uint32_t platform_coarse_ticks_ms(void);
int64_t platform_perf_counter(void);
int64_t platform_perf_frequency(void);
unsigned platform_timer_period_begin(void);
void platform_timer_period_end(unsigned period);

int32_t platform_time_now(void);
struct tm *platform_time_local(int32_t seconds);
int32_t platform_time_make(struct tm *local);
double platform_time_diff(int32_t end, int32_t start);
const char *platform_time_text(int32_t seconds);
void platform_date_time_local(struct PlatformDateTime *out);
void platform_date_time_utc(struct PlatformDateTime *out);

#endif
