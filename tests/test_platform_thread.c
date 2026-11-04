#include "platform/clock.h"
#include "platform/thread.h"
#include <assert.h>
#include <stdio.h>

#define WORKERS 4
#define ROUNDS 20000

struct Shared {
    struct PlatformMutex *mutex;
    struct PlatformEvent *start;
    struct PlatformEvent *done;
    volatile int32_t counter;
    int guarded;
    int try_result;
    uint32_t id;
};

static void count_up(void *context)
{
    struct Shared *shared = context;
    int i;
    assert(platform_event_wait(shared->start, 5000));
    for (i = 0; i < ROUNDS; ++i) {
        platform_atomic_increment(&shared->counter);
        platform_mutex_lock(shared->mutex);
        platform_mutex_lock(shared->mutex);
        ++shared->guarded;
        platform_mutex_unlock(shared->mutex);
        platform_mutex_unlock(shared->mutex);
        if (i % 1000 == 0)
            platform_thread_yield();
    }
}

static void count_down(void *context)
{
    struct Shared *shared = context;
    int i;
    for (i = 0; i < ROUNDS; ++i)
        platform_atomic_decrement(&shared->counter);
}

static void try_from_other_thread(void *context)
{
    struct Shared *shared = context;
    shared->try_result = platform_mutex_try_lock(shared->mutex);
    if (shared->try_result)
        platform_mutex_unlock(shared->mutex);
    shared->id = platform_thread_id();
}

static void signal_later(void *context)
{
    struct Shared *shared = context;
    platform_thread_raise_priority();
    assert(!platform_event_wait(shared->start, 30));
    platform_event_set(shared->done);
}

static void test_mutex(struct Shared *shared)
{
    struct PlatformThread *thread;
    assert(platform_mutex_try_lock(shared->mutex));
    platform_mutex_lock(shared->mutex);
    thread = platform_thread_create(try_from_other_thread, shared);
    assert(thread);
    platform_thread_join(thread);
    assert(!shared->try_result);
    assert(shared->id && shared->id != platform_thread_id());
    assert(platform_thread_id() == platform_thread_id());
    platform_mutex_unlock(shared->mutex);
    platform_mutex_unlock(shared->mutex);
    thread = platform_thread_create(try_from_other_thread, shared);
    assert(thread);
    platform_thread_join(thread);
    assert(shared->try_result);
}

static void test_event(struct Shared *shared)
{
    struct PlatformThread *thread;
    uint32_t started;
    assert(!platform_event_wait(shared->start, 0));
    started = platform_ticks_ms();
    assert(!platform_event_wait(shared->start, 50));
    assert((uint32_t)(platform_ticks_ms() - started) >= 40);
    platform_event_set(shared->start);
    assert(platform_event_wait(shared->start, 0));
    assert(platform_event_wait(shared->start, 1000));
    platform_event_reset(shared->start);
    assert(!platform_event_wait(shared->start, 0));
    thread = platform_thread_create(signal_later, shared);
    assert(thread);
    assert(platform_event_wait(shared->done, 5000));
    platform_thread_join(thread);
    platform_event_reset(shared->done);
}

static void test_workers(struct Shared *shared)
{
    struct PlatformThread *threads[WORKERS];
    int i;
    for (i = 0; i < WORKERS; ++i) {
        threads[i] = platform_thread_create(count_up, shared);
        assert(threads[i]);
    }
    platform_event_set(shared->start);
    for (i = 0; i < WORKERS; ++i)
        platform_thread_join(threads[i]);
    assert(platform_atomic_load(&shared->counter) == WORKERS * ROUNDS);
    assert(shared->guarded == WORKERS * ROUNDS);
    for (i = 0; i < WORKERS; ++i) {
        threads[i] = platform_thread_create(count_down, shared);
        assert(threads[i]);
    }
    for (i = 0; i < WORKERS; ++i)
        platform_thread_join(threads[i]);
    assert(platform_atomic_load(&shared->counter) == 0);
    assert(platform_atomic_increment(&shared->counter) == 1);
    assert(platform_atomic_decrement(&shared->counter) == 0);
}

int main(void)
{
    struct Shared shared = {0};
    shared.mutex = platform_mutex_create();
    shared.start = platform_event_create();
    shared.done = platform_event_create();
    assert(shared.mutex && shared.start && shared.done);
    test_mutex(&shared);
    test_event(&shared);
    test_workers(&shared);
    platform_event_destroy(shared.done);
    platform_event_destroy(shared.start);
    platform_mutex_destroy(shared.mutex);
    puts("Platform threads: recursive mutex, contended try-lock, manual-reset events with timeouts, joined workers and atomics passed");
    return 0;
}
