/* The frame delay an attack's animation is played with for its hit frame to show when the swing
   lands (std/swing_pace.h). */
#include "std/swing_pace.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>

static int about(float a, float b)
{
    return fabsf(a - b) <= 0.001f * (fabsf(b) > 1.0f ? fabsf(b) : 1.0f);
}

/* An attack whose hit is on frame 4 and whose frames take 4 units of 24 ms: on its own the hit
   shows 384 ms after it began. */
static void the_hit_frame_shows_when_the_swing_lands(void)
{
    assert(about(swing_pace_hit_ms(4.0f, 4.0f), 384.0f));
    /* lands when the hit would show anyway: as it is */
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, 384), 4.0f));
    /* a slow attacker's swing of 912 ms: every frame stays longer */
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, 912), 9.5f));
    assert(about(swing_pace_hit_ms(swing_pace_frame_delay(4.0f, 4.0f, 912), 4.0f), 912.0f));
    /* a swing that has less time left: faster */
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, 288), 3.0f));
    assert(about(swing_pace_hit_ms(swing_pace_frame_delay(4.0f, 4.0f, 288), 4.0f), 288.0f));
    /* the delay the attack has on its own does not matter within the bounds: the hit frame
       and the time say it all */
    assert(about(swing_pace_frame_delay(2.5f, 4.0f, 384), 4.0f));
    assert(about(swing_pace_frame_delay(7.0f, 4.0f, 384), 4.0f));
    /* a hit frame between two frames, as a bow's is once its arrow's flight is added */
    assert(about(swing_pace_frame_delay(4.0f, 5.5f, 660), 5.0f));
}

static void no_faster_than_twice_and_no_slower_than_eight_times(void)
{
    /* half the time it takes on its own is the least */
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, 192), 2.0f));
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, 191), 2.0f));
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, 1), 2.0f));
    /* a swing that has landed is played as fast as that */
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, 0), 2.0f));
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, -500), 2.0f));
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, INT_MIN), 2.0f));
    /* eight times is the most */
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, 384 * 8), 32.0f));
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, 384 * 8 + 1), 32.0f));
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, 60000), 32.0f));
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, INT_MAX), 32.0f));
    /* just inside either bound it is the time's */
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, 384 * 8 - 96), 31.0f));
    assert(about(swing_pace_frame_delay(4.0f, 4.0f, 216), 2.25f));
}

static void an_attack_without_a_frame_to_go_by_keeps_its_delay(void)
{
    float not_a_number = nanf("");

    /* no hit frame: the sprite has one frame, or the hit is on the first */
    assert(swing_pace_frame_delay(4.0f, 0.0f, 912) == 4.0f);
    assert(swing_pace_frame_delay(4.0f, -1.0f, 912) == 4.0f);
    /* no delay of its own */
    assert(swing_pace_frame_delay(0.0f, 4.0f, 912) == 0.0f);
    assert(swing_pace_frame_delay(-2.0f, 4.0f, 912) == -2.0f);
    /* what is not a number stays out of the animation's clock */
    assert(swing_pace_frame_delay(4.0f, not_a_number, 912) == 4.0f);
    assert(isnan(swing_pace_frame_delay(not_a_number, 4.0f, 912)));
}

/* Over many attacks: within the bounds the hit frame shows when the swing lands, to the
   rounding of a float, and outside them at the bound. */
static void every_attack_lands_on_time_or_at_a_bound(void)
{
    unsigned seed = 2463534242u;
    int round;

    for (round = 0; round < 200000; round++) {
        float delay, frame, paced, shows;
        int lands;

        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        delay = 1.0f + (float)(seed % 2000) / 100.0f;
        frame = 1.0f + (float)((seed >> 11) % 120) / 10.0f;
        lands = (int)((seed >> 3) % 6000) - 500;
        paced = swing_pace_frame_delay(delay, frame, lands);
        shows = swing_pace_hit_ms(paced, frame);
        assert(paced >= delay / 2.0f * 0.999f && paced <= delay * 8.0f * 1.001f);
        if (about(paced, delay / 2.0f))
            assert((float)lands <= shows * 1.001f);
        else if (about(paced, delay * 8.0f))
            assert((float)lands >= shows * 0.999f);
        else
            assert(about(shows, (float)lands));
    }
}

int main(void)
{
    the_hit_frame_shows_when_the_swing_lands();
    no_faster_than_twice_and_no_slower_than_eight_times();
    an_attack_without_a_frame_to_go_by_keeps_its_delay();
    every_attack_lands_on_time_or_at_a_bound();
    puts("swing_pace: ok");
    return 0;
}
