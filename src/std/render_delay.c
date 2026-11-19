#include "std/render_delay.h"

void render_delay_reset(struct RenderDelay *delay)
{
    delay->count = 0;
    delay->next = 0;
    delay->delay = RENDER_DELAY_START;
    delay->slewed_at = 0;
    delay->started = 0;
}

void render_delay_sample(struct RenderDelay *delay, int late_ms)
{
    if (late_ms < 0)
        return;
    delay->late[delay->next] = late_ms;
    delay->next = (delay->next + 1) % RENDER_DELAY_SAMPLES;
    if (delay->count < RENDER_DELAY_SAMPLES)
        delay->count++;
}

/* The delay the samples ask for: the second latest of them and the margin, so that one record
   held up on the way does not set it. */
static unsigned target(const struct RenderDelay *delay)
{
    int first = 0, second = 0, i, want;

    if (delay->count < RENDER_DELAY_WARM)
        return RENDER_DELAY_START;
    for (i = 0; i < delay->count; i++) {
        if (delay->late[i] > first) {
            second = first;
            first = delay->late[i];
        } else if (delay->late[i] > second) {
            second = delay->late[i];
        }
    }
    want = second + RENDER_DELAY_MARGIN;
    if (want < RENDER_DELAY_MIN)
        want = RENDER_DELAY_MIN;
    if (want > RENDER_DELAY_MAX)
        want = RENDER_DELAY_MAX;
    return (unsigned)want;
}

unsigned render_delay_now(struct RenderDelay *delay, uint32_t now)
{
    unsigned want = target(delay);
    uint32_t steps;

    if (!delay->started) {
        delay->started = 1;
        delay->slewed_at = now;
    }
    steps = (uint32_t)(now - delay->slewed_at) / RENDER_DELAY_SLEW;
    delay->slewed_at += steps * RENDER_DELAY_SLEW;
    if (delay->delay < want)
        delay->delay = want - delay->delay > steps ? delay->delay + steps : want;
    else
        delay->delay = delay->delay - want > steps ? delay->delay - steps : want;
    return delay->delay;
}
