#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <time.h>
#include "base.h"
#include "ui/game.h"

SIZE_ASSERT(Time, 4);
SIZE_ASSERT(TimeRecord, 16);

struct Time *time_construct(struct Time *self, __time32_t t)
{
    self->t = t;
    return self;
}

void time_destruct(struct Time *self)
{
    (void)self;
}

struct TimeRecord *time_get_as_record(struct Time *self, struct TimeRecord *out)
{
    struct tm *tm;

    memset(out, 0, sizeof *out);
    if (self->t < 0)
        self->t = 0;
    tm = _localtime32(&self->t);
    out->fraction = 0;
    out->year = (WORD)(tm->tm_year + 1900);
    out->month = (WORD)(tm->tm_mon + 1);
    out->day = (WORD)tm->tm_mday;
    out->hour = (WORD)tm->tm_hour;
    out->minute = (WORD)tm->tm_min;
    out->second = (WORD)tm->tm_sec;
    return out;
}

BYTE time_is_past(struct Time *self)
{
    __time32_t now;

    _time32(&now);
    return 0.0 > _difftime32(self->t, now);
}
