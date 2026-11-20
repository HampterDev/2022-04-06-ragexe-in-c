#ifndef RENDER_FRAME_RING_H
#define RENDER_FRAME_RING_H

/* The frames one thread records and another submits to the GPU, as pure logic: a ring of slots,
   each the packet of one frame (its lists and the memory its records go up from), which the
   recording thread fills and hands over and the submitting thread takes in order, records into
   the GPU's command buffer and submits; the recording thread then takes it back. Frames are
   numbered from 0, and frame n lives in slot n % FRAME_RING_SLOTS. Before the submitting thread
   records frame n it waits for the GPU to finish frame n - FRAME_RING_IN_FLIGHT, so the recording
   thread may refill a slot once the frame that waited for the slot's last frame has come back.
   The two threads call these with the ring locked; nothing here waits. Uses nothing else of the
   client. */
#include <stdint.h>

/* the frames the GPU may still be drawing when the submitting thread takes the next one */
#define FRAME_RING_IN_FLIGHT 2
/* one frame being recorded, one being submitted, FRAME_RING_IN_FLIGHT on the GPU */
#define FRAME_RING_SLOTS (FRAME_RING_IN_FLIGHT + 2)

/* Counts of frames, each written by one thread: begun, posted (handed over) and retired (taken
   back) by the recording thread, taken and finished (submitted, or given up) by the submitting
   one. finished <= taken <= posted <= begun, retired <= finished, and at most one frame is begun
   and not posted or taken and not finished. */
struct FrameRing {
    uint64_t begun, posted, retired;
    uint64_t taken, finished;
};

void frame_ring_init(struct FrameRing *r);

/* Recording thread: begins the next frame and answers its slot; -1 while a frame begun is not
   posted yet, or while the slot's last frame may still be in use: until the frame that waited
   for it on the GPU is retired. */
int frame_ring_begin(struct FrameRing *r);

/* Recording thread: hands the frame begun over and answers its slot; -1 without one. */
int frame_ring_post(struct FrameRing *r);

/* Submitting thread: takes the next frame handed over and answers its slot, with *wait the slot
   of the frame whose GPU work must be finished before it is recorded, or -1 for none; -1 while
   the frame taken before is not finished or nothing new was handed over. */
int frame_ring_take(struct FrameRing *r, int *wait);

/* Submitting thread: the frame taken is submitted, or given up; answers 0 without one taken. */
int frame_ring_finish(struct FrameRing *r);

/* Recording thread: takes back the oldest finished frame not taken back yet and answers its slot;
   -1 for none. */
int frame_ring_retire(struct FrameRing *r);

/* Whether every frame handed over has been taken back. */
int frame_ring_drained(const struct FrameRing *r);

#endif
