#include "render/render_thread.h"
#include "render/frame_ring.h"
#include <SDL3/SDL.h>
#include <stdlib.h>

struct RenderThread {
    SDL_Thread *thread;
    SDL_Mutex *lock;
    SDL_Condition *posted;  /* a frame handed over, or the end asked for */
    SDL_Condition *ran;     /* a frame run */
    struct FrameRing ring;
    int stopping;
    RenderThreadRun run;
    void *context;
};

static int SDLCALL thread_main(void *data)
{
    struct RenderThread *t = data;
    for (;;) {
        int slot, wait;
        SDL_LockMutex(t->lock);
        while ((slot = frame_ring_take(&t->ring, &wait)) < 0 && !t->stopping)
            SDL_WaitCondition(t->posted, t->lock);
        SDL_UnlockMutex(t->lock);
        if (slot < 0)
            return 0;
        t->run(t->context, slot, wait);
        SDL_LockMutex(t->lock);
        frame_ring_finish(&t->ring);
        SDL_BroadcastCondition(t->ran);
        SDL_UnlockMutex(t->lock);
    }
}

static void free_thread(struct RenderThread *t)
{
    if (t->ran)
        SDL_DestroyCondition(t->ran);
    if (t->posted)
        SDL_DestroyCondition(t->posted);
    if (t->lock)
        SDL_DestroyMutex(t->lock);
    free(t);
}

struct RenderThread *render_thread_start(RenderThreadRun run, void *context)
{
    struct RenderThread *t = calloc(1, sizeof *t);
    if (!t)
        return NULL;
    frame_ring_init(&t->ring);
    t->run = run;
    t->context = context;
    t->lock = SDL_CreateMutex();
    t->posted = SDL_CreateCondition();
    t->ran = SDL_CreateCondition();
    if (t->lock && t->posted && t->ran)
        t->thread = SDL_CreateThread(thread_main, "render", t);
    if (!t->thread) {
        free_thread(t);
        return NULL;
    }
    return t;
}

/* The thread takes every frame posted before it sees the end: frame_ring_take answers one while
   any is left. */
void render_thread_stop(struct RenderThread *t)
{
    if (!t)
        return;
    SDL_LockMutex(t->lock);
    t->stopping = 1;
    SDL_BroadcastCondition(t->posted);
    SDL_UnlockMutex(t->lock);
    SDL_WaitThread(t->thread, NULL);
    free_thread(t);
}

int render_thread_begin(struct RenderThread *t)
{
    int slot;
    SDL_LockMutex(t->lock);
    slot = frame_ring_begin(&t->ring);
    SDL_UnlockMutex(t->lock);
    return slot;
}

void render_thread_post(struct RenderThread *t)
{
    SDL_LockMutex(t->lock);
    if (frame_ring_post(&t->ring) >= 0)
        SDL_BroadcastCondition(t->posted);
    SDL_UnlockMutex(t->lock);
}

int render_thread_retire(struct RenderThread *t)
{
    int slot;
    SDL_LockMutex(t->lock);
    while ((slot = frame_ring_retire(&t->ring)) < 0 && !frame_ring_drained(&t->ring))
        SDL_WaitCondition(t->ran, t->lock);
    SDL_UnlockMutex(t->lock);
    return slot;
}
