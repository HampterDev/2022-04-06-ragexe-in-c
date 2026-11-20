#ifndef RENDER_RENDER_THREAD_H
#define RENDER_RENDER_THREAD_H

/* A thread of its own that runs the frames another thread records, one at a time and in the order
   they are handed over: the recording thread begins a frame in a slot of the ring
   (render/frame_ring.h), fills that slot's packet, posts it and goes on to the next, and later
   retires the frames the thread has run, taking their packets back; the thread runs each with the
   callback it was started with. A slot's packet belongs to one side at a time, so neither needs a
   lock to read or write it; the ring's counts are kept under a lock taken a few times a frame, and
   each side sleeps on a condition while it waits for the other. Uses SDL's threads. */

struct RenderThread;

/* Runs the frame in `slot` on the thread: its GPU work must wait for the frame in slot `wait` (-1
   for none) to be finished on the GPU, which the callback sees to. */
typedef void (*RenderThreadRun)(void *context, int slot, int wait);

/* NULL where the thread cannot be made. */
struct RenderThread *render_thread_start(RenderThreadRun run, void *context);

/* Runs every frame handed over, ends the thread and frees it; frames not retired stay so. */
void render_thread_stop(struct RenderThread *t);

/* The slot of the next frame to record (frame_ring_begin), or -1 until more frames are retired. */
int render_thread_begin(struct RenderThread *t);

/* Hands the frame begun over to the thread. */
void render_thread_post(struct RenderThread *t);

/* The slot of the oldest frame handed over and not retired yet, now retired, once the thread has
   run it (waiting for it to), or -1 where every frame handed over is retired. */
int render_thread_retire(struct RenderThread *t);

#endif
