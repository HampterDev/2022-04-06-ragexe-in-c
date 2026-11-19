#include "std/clock_sync.h"
#include <assert.h>
#include <stdio.h>

/* One timed exchange: the client sends at `now`, the packet takes `up` ms, the server holds it
   for `held` ms and the answer takes `down` ms. `offset` is the server's clock minus the
   client's. */
static int exchange(struct ClockSync *sync, uint32_t now, uint32_t offset, uint32_t up, uint32_t held,
                    uint32_t down)
{
    uint32_t server_receive = now + up + offset;

    return clock_sync_sample(sync, now, server_receive, server_receive + held, now + up + held + down);
}

static void before_any_sample(void)
{
    struct ClockSync sync;

    clock_sync_reset(&sync);
    assert(clock_sync_now(&sync, 1234) == 1234 && sync.count == 0);
}

static void first_sample_sets_the_clock(void)
{
    struct ClockSync sync;

    clock_sync_reset(&sync);
    assert(exchange(&sync, 1000, 500000, 40, 3, 40));
    assert(sync.count == 1 && sync.round_trip == 80);
    assert(clock_sync_now(&sync, 1083) == 1083 + 500000);
    assert(clock_sync_now(&sync, 5000) == 5000 + 500000);
}

/* Delays that differ each way lean the estimate by half their difference, and no more. */
static void uneven_delays(void)
{
    struct ClockSync sync;

    clock_sync_reset(&sync);
    assert(exchange(&sync, 0, 7000, 70, 0, 30));
    assert(sync.round_trip == 100 && clock_sync_now(&sync, 100) == 100 + 7000 + 20);
}

/* Among the last eight, the exchange with the shortest round trip is the one trusted. */
static void best_of_the_window(void)
{
    struct ClockSync sync;
    uint32_t now = 0, t;
    int i;

    clock_sync_reset(&sync);
    for (i = 0; i < 8; i++, now += 250)
        assert(exchange(&sync, now, 90000, 30 + (uint32_t)i * 7 % 40, 2, 30 + (uint32_t)i * 13 % 50));
    assert(sync.count == CLOCK_SYNC_SAMPLES && sync.round_trip == 60);
    for (t = now; t < now + 2000; t += 16)
        clock_sync_now(&sync, t);
    assert(clock_sync_now(&sync, now + 2000) == now + 2000 + 90000);

    /* eight worse exchanges push the good one out of the window */
    for (i = 0; i < 8; i++, now += 250)
        assert(exchange(&sync, now, 90000, 90, 0, 50));
    assert(sync.round_trip == 140 && sync.target == 90000 + 20);
}

/* While the first samples come in, a clock that is behind is set forward at once: the first
   answer after a map load can wait out a slow frame, and its round trip is then that frame. A
   clock that is ahead still only slows. */
static void steps_forward_while_the_window_fills(void)
{
    struct ClockSync sync;
    uint32_t value;

    clock_sync_reset(&sync);
    assert(exchange(&sync, 0, 5000, 10, 0, 610)); /* read 600 ms late: 300 behind */
    assert(sync.round_trip == 620 && clock_sync_now(&sync, 620) == 620 + 5000 - 300);
    assert(exchange(&sync, 700, 5000, 10, 0, 10));
    assert(sync.count == 2 && clock_sync_now(&sync, 721) == 721 + 5000);

    clock_sync_reset(&sync);
    assert(exchange(&sync, 0, 5000, 610, 0, 10)); /* sent 600 ms late: 300 ahead */
    assert(clock_sync_now(&sync, 620) == 620 + 5000 + 300);
    assert(exchange(&sync, 700, 5000, 10, 0, 10));
    value = clock_sync_now(&sync, 721);
    assert(value > 620 + 5000 + 300 && value > 721 + 5000 + 200 && value <= 721 + 5000 + 300);
}

/* With the window full, a new estimate is approached at a tenth of the time that passes; the
   clock never runs backward and never stands still. */
static void slews(void)
{
    struct ClockSync sync;
    uint32_t t, last, last_t, value;
    int i;

    clock_sync_reset(&sync);
    for (i = 0; i < CLOCK_SYNC_SAMPLES; i++)
        assert(exchange(&sync, (uint32_t)i * 10, 1000, 20, 0, 20));
    last_t = 110;
    last = clock_sync_now(&sync, last_t);
    assert(last == 1110 && sync.count == CLOCK_SYNC_SAMPLES);
    assert(exchange(&sync, 200, 1050, 10, 0, 10));
    assert(sync.target == 1050 && sync.offset == 1000);
    for (t = 220; t <= 1300; t += 16) {
        value = clock_sync_now(&sync, t);
        assert(value - last >= t - last_t && value - last <= (t - last_t) + (t - last_t) / 10 + 1);
        last = value;
        last_t = t;
    }
    assert(clock_sync_now(&sync, 1300) == 1300 + 1050);

    assert(exchange(&sync, 1400, 1010, 5, 0, 5));
    last = clock_sync_now(&sync, 1410);
    for (t = 1426; t <= 2500; t += 16) {
        value = clock_sync_now(&sync, t);
        assert(value > last);
        last = value;
    }
    assert(clock_sync_now(&sync, 2500) == 2500 + 1010);
}

/* Far behind, the clock jumps forward. Far ahead, it only slows: it never runs backward. */
static void jumps_forward_only(void)
{
    struct ClockSync sync;
    uint32_t t, last, value;

    clock_sync_reset(&sync);
    assert(exchange(&sync, 0, 1000, 20, 0, 20));
    assert(exchange(&sync, 100, 60000, 10, 0, 10));
    assert(clock_sync_now(&sync, 121) == 121 + 60000);

    clock_sync_reset(&sync);
    assert(exchange(&sync, 0, 60000, 20, 0, 20));
    last = clock_sync_now(&sync, 40);
    assert(exchange(&sync, 100, 1000, 10, 0, 10));
    for (t = 136; t <= 4000; t += 16) {
        value = clock_sync_now(&sync, t);
        assert(value > last);
        last = value;
    }
    assert(sync.offset < 60000 && sync.offset > 1000);
}

static void wraps(void)
{
    struct ClockSync sync;
    uint32_t now = 0xffffff00u, offset = 0x80000123u;

    clock_sync_reset(&sync);
    assert(exchange(&sync, now, offset, 100, 5, 100));
    assert(sync.round_trip == 200);
    assert(clock_sync_now(&sync, now + 400) == now + 400 + offset);
    assert(exchange(&sync, now + 500, offset, 60, 5, 60));
    assert(sync.round_trip == 120 && clock_sync_now(&sync, now + 700) == now + 700 + offset);
}

static void refuses_an_impossible_exchange(void)
{
    struct ClockSync sync;

    clock_sync_reset(&sync);
    assert(exchange(&sync, 0, 1000, 20, 0, 20));
    assert(!clock_sync_sample(&sync, 100, 1100, 1300, 150));
    assert(sync.count == 1 && sync.target == 1000);
}

int main(void)
{
    before_any_sample();
    first_sample_sets_the_clock();
    uneven_delays();
    best_of_the_window();
    steps_forward_while_the_window_fills();
    slews();
    jumps_forward_only();
    wraps();
    refuses_an_impossible_exchange();
    printf("Clock sync: first sample, uneven delays, best of eight, forward steps while the window fills, slewing, forward jumps, wrapping and refusals passed.\n");
    return 0;
}
