#include "std/way_up.h"
#include <assert.h>
#include <stdio.h>

/* The server's clock is the client's plus this. */
#define OFFSET 0x10000000u

static void sample(struct WayUp *way, uint32_t sent, uint32_t way_up)
{
    way_up_sample(way, sent, sent + OFFSET + way_up);
}

static void nothing_known_before_the_first_exchange(void)
{
    struct WayUp way;

    way_up_reset(&way);
    assert(way.count == 0);
    assert(way_up_arrival(&way, 1234) == 1234);
}

static void one_exchange_is_taken_as_it_is(void)
{
    struct WayUp way;

    way_up_reset(&way);
    sample(&way, 1000, 40);
    assert(way_up_arrival(&way, 1100) == 1100 + OFFSET + 40);
    /* two: the longer of the two, and half its height above the shorter */
    sample(&way, 1250, 30);
    assert(way_up_arrival(&way, 1300) == 1300 + OFFSET + 45);
}

static void one_held_up_on_the_way_does_not_set_it(void)
{
    struct WayUp way;
    uint32_t now = 1000;
    int i;

    way_up_reset(&way);
    /* 30, 32, ... 44 and one of 300 */
    for (i = 0; i < 8; i++, now += 250)
        sample(&way, now, 30 + 2 * i);
    sample(&way, now, 300);
    /* the second longest is 44, 14 above the shortest: 44 + 7 */
    assert(way_up_arrival(&way, now) == now + OFFSET + 51);
}

static void the_lead_grows_at_once(void)
{
    struct WayUp way;
    uint32_t now = 1000;
    int i;

    way_up_reset(&way);
    for (i = 0; i < 4; i++, now += 250)
        sample(&way, now, 40);
    assert(way_up_arrival(&way, now) == now + OFFSET + 40);
    /* two slow ones: the second longest is 80, 40 above the shortest */
    sample(&way, now, 80);
    sample(&way, now, 80);
    assert(way_up_arrival(&way, now) == now + OFFSET + 100);
}

static void the_lead_shrinks_slowly_and_its_tick_never_runs_backward(void)
{
    struct WayUp way;
    uint32_t now = 1000, last, at;
    int i;

    way_up_reset(&way);
    /* two slow exchanges, then the rest fast: once the slow ones have left the window the lead
       has 60 ms to give back, one in WAY_UP_SLEW */
    sample(&way, now, 100);
    sample(&way, now, 100);
    for (i = 0; i < 14; i++, now += 250)
        sample(&way, now, 40);
    last = way_up_arrival(&way, now);
    assert(last == now + OFFSET + 130);
    sample(&way, now, 40);
    sample(&way, now, 40);
    /* the window holds sixteen of 40 now: the target is 40 */
    for (i = 1; i <= 2000; i++) {
        at = way_up_arrival(&way, now + (uint32_t)i);
        assert((int32_t)(at - last) >= 0);
        last = at;
    }
    assert(way.lead == OFFSET + 40);
    /* 90 ms to give back take 900 ms */
    way_up_reset(&way);
    sample(&way, now, 130);
    assert(way_up_arrival(&way, now) == now + OFFSET + 130);
    for (i = 0; i < 2; i++)
        sample(&way, now, 40);
    sample(&way, now, 40);
    /* longest 130, second 40: target 40 */
    assert(way_up_arrival(&way, now + 450) == now + 450 + OFFSET + 130 - 45);
    assert(way_up_arrival(&way, now + 900) == now + 900 + OFFSET + 40);
}

static void ticks_may_wrap(void)
{
    struct WayUp way;
    uint32_t now = 0xffffff00u;

    way_up_reset(&way);
    /* samples on both sides of the wrap of the difference itself */
    way_up_sample(&way, now, now + 0x7fffffe0u);
    way_up_sample(&way, now + 10, now + 10 + 0x80000010u);
    way_up_sample(&way, now + 20, now + 20 + 0x7ffffff0u);
    /* the second longest is 0x7ffffff0, 0x10 above the shortest */
    assert(way_up_arrival(&way, now + 30) == now + 30 + 0x7ffffff0u + 8u);
    /* and the client's clock wraps */
    way_up_reset(&way);
    sample(&way, 0xfffffff0u, 50);
    assert(way_up_arrival(&way, 0x20u) == 0x20u + OFFSET + 50);
}

/* The samples of both sources, the timed exchanges every two seconds and the confirmed requests
   at the pace of the clicks, are one kind: the client's clock when it sent, the server's when it
   arrived. A line that becomes slower is followed at the next request, where the exchanges alone
   would follow it two seconds later at the earliest; a request that lost a datagram on its way
   is one long sample, which does not set the lead by itself. */
static void exchanges_and_requests(void)
{
    struct WayUp alone, both;
    uint32_t now = 1000, slow = 0;
    int i;

    way_up_reset(&alone);
    way_up_reset(&both);
    /* a line of 40 ms: an exchange every 2000 ms, a request every 100 ms between them */
    for (i = 0; i < 80; i++, now += 100) {
        if (i % 20 == 0) {
            sample(&alone, now, 40);
            sample(&both, now, 40);
        } else {
            sample(&both, now, i == 45 ? 400 : 40); /* one lost a datagram on its way */
        }
    }
    assert(way_up_arrival(&alone, now) == now + OFFSET + 40);
    assert(way_up_arrival(&both, now) == now + OFFSET + 40);
    /* the line becomes 120 ms: two requests later the lead is the new line's */
    for (i = 0; i < 40 && !slow; i++, now += 100) {
        if (i % 20 == 0)
            sample(&alone, now, 120);
        else
            sample(&both, now, 120);
        if (way_up_arrival(&both, now) - now - OFFSET >= 120)
            slow = (uint32_t)i;
    }
    assert(slow == 2);
    /* the exchanges alone: one slow exchange is the longest, not yet the second longest */
    assert(way_up_arrival(&alone, now) - now - OFFSET < 120);
}

int main(void)
{
    nothing_known_before_the_first_exchange();
    one_exchange_is_taken_as_it_is();
    one_held_up_on_the_way_does_not_set_it();
    the_lead_grows_at_once();
    the_lead_shrinks_slowly_and_its_tick_never_runs_backward();
    ticks_may_wrap();
    exchanges_and_requests();
    puts("way_up: ok");
    return 0;
}
