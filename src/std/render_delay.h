#ifndef STD_RENDER_DELAY_H
#define STD_RENDER_DELAY_H

#include <stdint.h>

/* How far behind the server's clock everyone else is drawn (plans/core-loop.md). A record of
   another actor is played when the render clock reaches its tick, so the delay has to cover
   the way down and its jitter, and should cover no more: it is what a player sees of others
   later than they did it. The delay follows how late the last records arrived, between
   RENDER_DELAY_MIN and RENDER_DELAY_MAX, and moves a little at a time, so the render clock
   never jumps and never runs backward. */

#define RENDER_DELAY_SAMPLES 16
/* Used until RENDER_DELAY_WARM records have been timed. */
#define RENDER_DELAY_START 100
#define RENDER_DELAY_WARM 8
#define RENDER_DELAY_MIN 40
#define RENDER_DELAY_MAX 200
/* Added to the lateness the delay is taken from. */
#define RENDER_DELAY_MARGIN 10
/* The delay moves one millisecond in this many, so the render clock runs 5% slow or fast
   while it does. */
#define RENDER_DELAY_SLEW 20

struct RenderDelay {
    int late[RENDER_DELAY_SAMPLES]; /* ms after its tick that a record arrived */
    int count;                      /* samples held, at most RENDER_DELAY_SAMPLES */
    int next;                       /* where the next sample goes */
    unsigned delay;                 /* in use */
    uint32_t slewed_at;             /* client time up to which `delay` has followed its target */
    int started;                    /* `slewed_at` is set */
};

void render_delay_reset(struct RenderDelay *delay);
/* Takes how long after its tick a record arrived, by the server's clock. A record that came
   before its tick was sent ahead of time and says nothing about the way down; it is not kept. */
void render_delay_sample(struct RenderDelay *delay, int late_ms);
/* The delay at the client time `now`. */
unsigned render_delay_now(struct RenderDelay *delay, uint32_t now);

#endif
