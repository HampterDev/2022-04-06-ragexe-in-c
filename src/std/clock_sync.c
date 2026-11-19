#include "std/clock_sync.h"

/* The clock in use closes at most a tenth of the time that passed. More than a second behind,
   it jumps forward, and so does a clock that is behind at all while the first samples come in:
   the answer it started from may have waited out a slow frame, and its round trip is then that
   frame. Ahead by any amount, it only slows, so it never runs backward. */
#define SLEW_DIVISOR 10
#define JUMP_MS 1000

void clock_sync_reset(struct ClockSync *sync)
{
    sync->count = 0;
    sync->next = 0;
    sync->round_trip = 0;
    sync->target = 0;
    sync->offset = 0;
    sync->slewed_at = 0;
}

int clock_sync_sample(struct ClockSync *sync, uint32_t client_send, uint32_t server_receive,
                      uint32_t server_send, uint32_t client_receive)
{
    uint32_t up = server_receive - client_send;   /* offset plus the way up */
    uint32_t down = server_send - client_receive; /* offset minus the way down */
    int32_t round_trip = (int32_t)(up - down);
    int i, best;

    if (round_trip < 0)
        return 0;
    sync->samples[sync->next].round_trip = (uint32_t)round_trip;
    sync->samples[sync->next].offset = up - (uint32_t)round_trip / 2;
    sync->next = (sync->next + 1) % CLOCK_SYNC_SAMPLES;
    if (sync->count < CLOCK_SYNC_SAMPLES)
        sync->count++;

    best = 0;
    for (i = 1; i < sync->count; i++)
        if (sync->samples[i].round_trip < sync->samples[best].round_trip)
            best = i;
    sync->round_trip = sync->samples[best].round_trip;
    sync->target = sync->samples[best].offset;
    if (sync->count == 1) {
        sync->offset = sync->target;
        sync->slewed_at = client_receive;
    }
    return 1;
}

uint32_t clock_sync_now(struct ClockSync *sync, uint32_t now)
{
    int32_t off, step;

    if (!sync->count)
        return now;
    off = (int32_t)(sync->target - sync->offset);
    if (off > JUMP_MS || (off > 0 && sync->count < CLOCK_SYNC_SAMPLES)) {
        sync->offset = sync->target;
        sync->slewed_at = now;
    } else {
        step = (int32_t)(now - sync->slewed_at) / SLEW_DIVISOR;
        if (step > 0) {
            sync->slewed_at += (uint32_t)step * SLEW_DIVISOR;
            if (off > 0)
                sync->offset += (uint32_t)(step < off ? step : off);
            else if (off < 0)
                sync->offset -= (uint32_t)(step < -off ? step : -off);
        }
    }
    return now + sync->offset;
}
