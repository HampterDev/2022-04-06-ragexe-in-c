#include "render/frame_ring.h"
#include <string.h>

void frame_ring_init(struct FrameRing *r)
{
    memset(r, 0, sizeof *r);
}

static int slot_of(uint64_t frame)
{
    return (int)(frame % FRAME_RING_SLOTS);
}

/* Frame b refills the slot of frame b - FRAME_RING_SLOTS, whose GPU work the submitting thread
   waited for before it recorded frame b - FRAME_RING_SLOTS + FRAME_RING_IN_FLIGHT: once that one
   is retired, so are all before it, and the slot is free. */
int frame_ring_begin(struct FrameRing *r)
{
    if (r->begun != r->posted || r->begun - r->retired >= FRAME_RING_SLOTS - FRAME_RING_IN_FLIGHT)
        return -1;
    return slot_of(r->begun++);
}

int frame_ring_post(struct FrameRing *r)
{
    if (r->begun == r->posted)
        return -1;
    return slot_of(r->posted++);
}

int frame_ring_take(struct FrameRing *r, int *wait)
{
    *wait = -1;
    if (r->taken != r->finished || r->taken == r->posted)
        return -1;
    if (r->taken >= FRAME_RING_IN_FLIGHT)
        *wait = slot_of(r->taken - FRAME_RING_IN_FLIGHT);
    return slot_of(r->taken++);
}

int frame_ring_finish(struct FrameRing *r)
{
    if (r->finished == r->taken)
        return 0;
    r->finished++;
    return 1;
}

int frame_ring_retire(struct FrameRing *r)
{
    if (r->retired == r->finished)
        return -1;
    return slot_of(r->retired++);
}

int frame_ring_drained(const struct FrameRing *r)
{
    return r->retired == r->posted;
}
