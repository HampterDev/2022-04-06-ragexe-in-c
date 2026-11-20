/* render/frame_ring.c: the calls that have nothing to act on answer so and change nothing; and
   every ordering of the three parties over a run of frames, explored exhaustively: the recording
   thread (begin, post, retire, whenever the ring lets it), the submitting thread (take, wait for
   the GPU to finish the frame the ring names, record, finish) and the GPU (finishing the frames
   submitted, in order). In every reachable state a slot is refilled only once its last frame is
   retired and the GPU is done with it, the submitting thread takes the frames in order, after
   they were posted, and finds the frame it waits for still in the slot the ring names (its fence
   lives there), and something can always go on until every frame has come back. */
#include "render/frame_ring.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAMES 12

/* the model's state beside the ring: which frame each slot holds (-1 for none yet), how many
   frames the GPU has finished, and where the submitting thread is with the frame it took */
enum { IDLE, TAKEN, WAITED };
struct model {
    struct FrameRing ring;
    int owner[FRAME_RING_SLOTS];
    uint64_t gpuDone;
    int phase, slot, wait;
};

static unsigned char seen[FRAMES + 1][FRAMES + 1][FRAMES + 1][FRAMES + 1][FRAMES + 1][FRAMES + 1][3];
static unsigned long states, ends;

/* whether the state is new; the slots' owners and the frame taken follow from the counts */
static int visit(const struct model *m)
{
    unsigned char *s = &seen[m->ring.begun][m->ring.posted][m->ring.retired][m->ring.taken][m->ring.finished]
                            [m->gpuDone][m->phase];
    if (*s)
        return 0;
    *s = 1;
    states++;
    return 1;
}

static void explore(const struct model *m);

static int main_begin(struct model m)
{
    uint64_t b = m.ring.begun;
    int slot;
    if (b == FRAMES)
        return 0;
    slot = frame_ring_begin(&m.ring);
    if (slot < 0)
        return 0;
    assert(slot == (int)(b % FRAME_RING_SLOTS));
    if (m.owner[slot] >= 0) {
        uint64_t last = (uint64_t)m.owner[slot];
        assert(last + FRAME_RING_SLOTS == b);
        /* retired, so the submitting thread is done with it, and the GPU too */
        assert(last < m.ring.retired);
        assert(last < m.gpuDone);
    }
    m.owner[slot] = (int)b;
    explore(&m);
    return 1;
}

static int main_post(struct model m)
{
    uint64_t p = m.ring.posted;
    int slot = frame_ring_post(&m.ring);
    if (slot < 0)
        return 0;
    assert(m.owner[slot] == (int)p);
    explore(&m);
    return 1;
}

static int main_retire(struct model m)
{
    uint64_t r = m.ring.retired;
    int slot = frame_ring_retire(&m.ring);
    if (slot < 0)
        return 0;
    assert(r < m.ring.finished && m.owner[slot] == (int)r);
    explore(&m);
    return 1;
}

static int render_take(struct model m)
{
    uint64_t n = m.ring.taken;
    int wait, slot;
    if (m.phase != IDLE)
        return 0;
    slot = frame_ring_take(&m.ring, &wait);
    if (slot < 0)
        return 0;
    assert(n < m.ring.posted && m.owner[slot] == (int)n);
    if (n >= FRAME_RING_IN_FLIGHT) {
        /* the frame it waits for is submitted, and still in its slot */
        assert(wait == (int)((n - FRAME_RING_IN_FLIGHT) % FRAME_RING_SLOTS));
        assert(m.owner[wait] == (int)(n - FRAME_RING_IN_FLIGHT) && n - FRAME_RING_IN_FLIGHT < m.ring.finished);
    } else {
        assert(wait == -1);
    }
    m.phase = TAKEN;
    m.slot = slot;
    m.wait = wait;
    explore(&m);
    return 1;
}

/* the wait for the GPU, then the recording, which reads the slot */
static int render_record(struct model m)
{
    uint64_t n = m.ring.taken - 1;
    if (m.phase != TAKEN || (n >= FRAME_RING_IN_FLIGHT && m.gpuDone <= n - FRAME_RING_IN_FLIGHT))
        return 0;
    assert(m.owner[m.slot] == (int)n);
    m.phase = WAITED;
    explore(&m);
    return 1;
}

static int render_finish(struct model m)
{
    if (m.phase != WAITED)
        return 0;
    assert(m.owner[m.slot] == (int)(m.ring.taken - 1));
    assert(frame_ring_finish(&m.ring));
    m.phase = IDLE;
    explore(&m);
    return 1;
}

static int gpu_finish(struct model m)
{
    if (m.gpuDone == m.ring.finished)
        return 0;
    m.gpuDone++;
    explore(&m);
    return 1;
}

static void explore(const struct model *m)
{
    int moved;
    if (!visit(m))
        return;
    moved = main_begin(*m) | main_post(*m) | main_retire(*m) | render_take(*m) | render_record(*m)
            | render_finish(*m) | gpu_finish(*m);
    if (m->ring.retired == FRAMES) {
        assert(frame_ring_drained(&m->ring));
        ends++;
        return;
    }
    if (!moved) {
        fprintf(stderr, "stuck: begun %llu posted %llu retired %llu taken %llu finished %llu gpu %llu phase %d\n",
                (unsigned long long)m->ring.begun, (unsigned long long)m->ring.posted,
                (unsigned long long)m->ring.retired, (unsigned long long)m->ring.taken,
                (unsigned long long)m->ring.finished, (unsigned long long)m->gpuDone, m->phase);
        abort();
    }
}

static void check_empty_calls(void)
{
    struct FrameRing r, before;
    int wait = 5;
    frame_ring_init(&r);
    before = r;
    assert(frame_ring_post(&r) == -1);
    assert(frame_ring_take(&r, &wait) == -1 && wait == -1);
    assert(!frame_ring_finish(&r));
    assert(frame_ring_retire(&r) == -1);
    assert(frame_ring_drained(&r));
    assert(!memcmp(&r, &before, sizeof r));
    /* a frame begun is the one recorded until it is posted */
    assert(frame_ring_begin(&r) == 0);
    assert(frame_ring_begin(&r) == -1);
    assert(frame_ring_take(&r, &wait) == -1);
    assert(frame_ring_post(&r) == 0);
    assert(!frame_ring_drained(&r));
    /* the next may be begun while the first is out, not a third */
    assert(frame_ring_begin(&r) == 1);
    assert(frame_ring_post(&r) == 1);
    assert(frame_ring_begin(&r) == -1);
    /* taken in order, one at a time */
    assert(frame_ring_take(&r, &wait) == 0 && wait == -1);
    assert(frame_ring_take(&r, &wait) == -1);
    assert(frame_ring_retire(&r) == -1);
    assert(frame_ring_finish(&r) && !frame_ring_finish(&r));
    assert(frame_ring_retire(&r) == 0);
    assert(frame_ring_begin(&r) == 2);
}

int main(void)
{
    struct model m;
    check_empty_calls();
    memset(&m, 0, sizeof m);
    frame_ring_init(&m.ring);
    memset(m.owner, -1, sizeof m.owner);
    m.phase = IDLE;
    explore(&m);
    assert(ends > 0);
    printf("frame_ring: %lu states over %d frames, every ordering ends with all frames back\n", states, FRAMES);
    return 0;
}
