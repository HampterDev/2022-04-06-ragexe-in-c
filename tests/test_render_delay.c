#include "std/render_delay.h"
#include <assert.h>
#include <stdio.h>

static void fill(struct RenderDelay *delay, int late, int count)
{
    while (count--)
        render_delay_sample(delay, late);
}

static void starts_at_the_first_guess(void)
{
    struct RenderDelay delay;

    render_delay_reset(&delay);
    assert(render_delay_now(&delay, 5000) == RENDER_DELAY_START);
    /* too few records timed to say anything */
    fill(&delay, 20, RENDER_DELAY_WARM - 1);
    assert(render_delay_now(&delay, 9000) == RENDER_DELAY_START);
}

static void follows_the_lateness_a_millisecond_at_a_time(void)
{
    struct RenderDelay delay;

    render_delay_reset(&delay);
    assert(render_delay_now(&delay, 1000) == 100);
    fill(&delay, 45, RENDER_DELAY_WARM);
    /* wants 45 + 10; one millisecond every 20 */
    assert(render_delay_now(&delay, 1019) == 100);
    assert(render_delay_now(&delay, 1020) == 99);
    assert(render_delay_now(&delay, 1200) == 90);
    assert(render_delay_now(&delay, 1000 + 45 * 20) == 55);
    assert(render_delay_now(&delay, 60000) == 55);
    /* the way down gets longer: up again, as slowly */
    fill(&delay, 80, 2);
    assert(render_delay_now(&delay, 60100) == 60);
    assert(render_delay_now(&delay, 70000) == 90);
}

static void stays_within_its_bounds(void)
{
    struct RenderDelay delay;

    render_delay_reset(&delay);
    render_delay_now(&delay, 0);
    fill(&delay, 3, RENDER_DELAY_SAMPLES);
    assert(render_delay_now(&delay, 100000) == RENDER_DELAY_MIN);
    fill(&delay, 900, RENDER_DELAY_SAMPLES);
    assert(render_delay_now(&delay, 200000) == RENDER_DELAY_MAX);
}

static void one_record_held_up_does_not_set_it(void)
{
    struct RenderDelay delay;

    render_delay_reset(&delay);
    render_delay_now(&delay, 0);
    fill(&delay, 30, RENDER_DELAY_SAMPLES - 1);
    render_delay_sample(&delay, 700);
    assert(render_delay_now(&delay, 100000) == 40);
    /* a second one does */
    render_delay_sample(&delay, 150);
    assert(render_delay_now(&delay, 200000) == 160);
}

static void a_record_sent_ahead_of_its_tick_is_not_a_sample(void)
{
    struct RenderDelay delay;

    render_delay_reset(&delay);
    fill(&delay, -120, 100);
    assert(delay.count == 0);
    assert(render_delay_now(&delay, 100000) == RENDER_DELAY_START);
}

static void old_samples_leave(void)
{
    struct RenderDelay delay;

    render_delay_reset(&delay);
    render_delay_now(&delay, 0);
    fill(&delay, 120, RENDER_DELAY_SAMPLES);
    assert(render_delay_now(&delay, 100000) == 130);
    fill(&delay, 20, RENDER_DELAY_SAMPLES);
    assert(render_delay_now(&delay, 200000) == RENDER_DELAY_MIN);
}

static void the_client_clock_may_wrap(void)
{
    struct RenderDelay delay;

    render_delay_reset(&delay);
    assert(render_delay_now(&delay, 0xffffff00u) == 100);
    fill(&delay, 45, RENDER_DELAY_WARM);
    assert(render_delay_now(&delay, 0x00000100u) == 100 - 0x200 / 20);
}

int main(void)
{
    starts_at_the_first_guess();
    follows_the_lateness_a_millisecond_at_a_time();
    stays_within_its_bounds();
    one_record_held_up_does_not_set_it();
    a_record_sent_ahead_of_its_tick_is_not_a_sample();
    old_samples_leave();
    the_client_clock_may_wrap();
    puts("render_delay: ok");
    return 0;
}
