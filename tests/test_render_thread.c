/* render/render_thread.c with real threads and no GPU: the main thread records 20,000 frames into
   the ring's packets, each a pattern of its own, posting and retiring them at random points with
   random pauses on both sides, while the render thread runs them with a pretend GPU (a frame run is
   "submitted", and the frame it waits for "finished" then). The thread runs the frames in order,
   finds each packet as it was recorded, and the frame it waits for submitted and still in its slot;
   the main thread refills a slot only once its frame was run and finished on the pretend GPU, and
   retires the frames in order, waiting for each to run. Stopping runs every frame handed over
   first. */
#include "render/frame_ring.h"
#include "render/render_thread.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FRAMES 20000
#define WORDS 1024

struct packet {
    uint64_t frame;
    uint32_t words[WORDS];
    int recorded;    /* posted by the main thread, not yet retired */
    int ran;         /* run by the thread */
    int submitted;   /* on the pretend GPU until the thread waits for it */
};

static struct packet packets[FRAME_RING_SLOTS];
static uint64_t nextRun;
static uint32_t threadSeed = 7;

static uint32_t next_random(uint32_t *seed)
{
    *seed = *seed * 1664525u + 1013904223u;
    return *seed >> 8;
}

static uint32_t pattern(uint64_t frame, int i)
{
    return (uint32_t)(frame * 2654435761u) ^ (uint32_t)i * 40503u;
}

static void pause_a_little(uint32_t *seed)
{
    uint32_t r = next_random(seed) % 64;
    if (r < 2)
        SDL_DelayNS(20000);
    else if (r < 12)
        SDL_DelayNS(0);
}

static void run(void *context, int slot, int wait)
{
    struct packet *p = &packets[slot];
    int i;
    (void)context;
    assert(p->recorded && !p->ran && p->frame == nextRun);
    if (p->frame >= FRAME_RING_IN_FLIGHT) {
        struct packet *w = &packets[wait];
        assert(wait >= 0 && w->frame == p->frame - FRAME_RING_IN_FLIGHT && w->ran);
        w->submitted = 0;
    } else {
        assert(wait == -1);
    }
    pause_a_little(&threadSeed);
    for (i = 0; i < WORDS; i++)
        assert(p->words[i] == pattern(p->frame, i));
    p->ran = 1;
    p->submitted = 1;
    nextRun++;
}

static uint64_t retired;

static int retire_one(struct RenderThread *t)
{
    int slot = render_thread_retire(t);
    if (slot < 0)
        return 0;
    assert(packets[slot].recorded && packets[slot].ran && packets[slot].frame == retired);
    packets[slot].recorded = 0;
    retired++;
    return 1;
}

int main(void)
{
    struct RenderThread *t;
    uint32_t seed = 12345;
    uint64_t f;
    int i;
    for (i = 0; i < FRAME_RING_SLOTS; i++)
        packets[i].frame = UINT64_MAX;
    t = render_thread_start(run, NULL);
    if (!t) {
        fprintf(stderr, "no thread: %s\n", SDL_GetError());
        return 1;
    }
    for (f = 0; f < FRAMES; f++) {
        struct packet *p;
        int slot;
        while ((slot = render_thread_begin(t)) < 0)
            assert(retire_one(t));
        p = &packets[slot];
        /* its last frame was retired and is no longer on the pretend GPU */
        assert(!p->recorded && !p->submitted);
        p->frame = f;
        p->ran = 0;
        pause_a_little(&seed);
        for (i = 0; i < WORDS; i++)
            p->words[i] = pattern(f, i);
        p->recorded = 1;
        render_thread_post(t);
        /* the client sometimes retires every frame handed over before recording the next, waiting
           for those not run yet, and otherwise only once the ring has no slot left */
        if (next_random(&seed) % 2 == 0)
            while (retire_one(t))
                ;
    }
    /* stopping runs what was handed over */
    render_thread_stop(t);
    assert(nextRun == FRAMES);
    printf("render_thread: %d frames run in order, %llu retired before the end\n", FRAMES,
           (unsigned long long)retired);
    return 0;
}
