#include "web/work_manager.h"
#include "web/work_manager_internal.h"
#include "web/work_pool.h"
#include <windows.h>
#include <process.h>
#include <stdint.h>
#include <stdlib.h>

struct QueuedWork {
    struct QueuedWork *next;
    struct AsyncWork *work;
    unsigned started;
};

struct WorkQueue {
    struct QueuedWork *first, *last;
};

struct AsyncWorkMgr {
    CRITICAL_SECTION lock;
    int lock_initialized;
    HANDLE ready, stop;
    struct WorkPoolMgr *pools;
    struct WorkQueue requests, responses;
    struct QueuedWork *running;
    size_t thread_count, started_threads;
    HANDLE threads[];
};

struct AsyncWorkMgr *g_asyncWorkMgr;

static void append_work(struct WorkQueue *queue, struct QueuedWork *entry)
{
    entry->next = NULL;
    if (queue->last)
        queue->last->next = entry;
    else
        queue->first = entry;
    queue->last = entry;
}

static void signal_event(HANDLE event)
{
    if (!SetEvent(event))
        abort();
}

static void reset_event(HANDLE event)
{
    if (!ResetEvent(event))
        abort();
}

static struct QueuedWork *take_request(struct AsyncWorkMgr *manager)
{
    struct QueuedWork *entry, *previous = NULL;
    unsigned scanned = 0;

    EnterCriticalSection(&manager->lock);
    for (entry = manager->requests.first; entry && scanned < 40;
         previous = entry, entry = entry->next, ++scanned) {
        struct QueuedWork *running;
        for (running = manager->running; running; running = running->next)
            if (running->started == entry->work->started)
                break;
        if (running)
            continue;
        if (previous)
            previous->next = entry->next;
        else
            manager->requests.first = entry->next;
        if (manager->requests.last == entry)
            manager->requests.last = previous;
        entry->started = entry->work->started;
        entry->next = manager->running;
        manager->running = entry;
        if (!manager->requests.first)
            reset_event(manager->ready);
        LeaveCriticalSection(&manager->lock);
        return entry;
    }
    reset_event(manager->ready);
    LeaveCriticalSection(&manager->lock);
    return NULL;
}

static void finish_request(struct AsyncWorkMgr *manager, struct QueuedWork *entry, int retry)
{
    struct QueuedWork **link;
    EnterCriticalSection(&manager->lock);
    for (link = &manager->running; *link && *link != entry; link = &(*link)->next) {}
    if (!*link)
        abort();
    *link = entry->next;
    if (retry) {
        entry->next = manager->requests.first;
        manager->requests.first = entry;
        if (!manager->requests.last)
            manager->requests.last = entry;
    } else {
        append_work(&manager->responses, entry);
    }
    if (manager->requests.first)
        signal_event(manager->ready);
    LeaveCriticalSection(&manager->lock);
}

static unsigned __stdcall worker_main(void *context)
{
    struct AsyncWorkMgr *manager = context;
    HANDLE events[2] = {manager->stop, manager->ready};
    for (;;) {
        struct QueuedWork *entry;
        DWORD ready = WaitForMultipleObjects(2, events, FALSE, INFINITE);
        if (ready == WAIT_OBJECT_0)
            return 0;
        if (ready != WAIT_OBJECT_0 + 1)
            abort();
        entry = take_request(manager);
        if (entry)
            finish_request(manager, entry, entry->work->ops->run(entry->work));
    }
}

static void stop_threads(struct AsyncWorkMgr *manager)
{
    size_t i;
    if (!manager->started_threads)
        return;
    signal_event(manager->stop);
    for (i = 0; i < manager->started_threads; ++i) {
        if (WaitForSingleObject(manager->threads[i], INFINITE) != WAIT_OBJECT_0)
            abort();
        CloseHandle(manager->threads[i]);
        manager->threads[i] = NULL;
    }
    manager->started_threads = 0;
}

static void release_queue(struct AsyncWorkMgr *manager, struct WorkQueue *queue)
{
    struct QueuedWork *entry = queue->first;
    while (entry) {
        struct QueuedWork *next = entry->next;
        work_pool_mgr_release(manager->pools, entry->work);
        free(entry);
        entry = next;
    }
    queue->first = queue->last = NULL;
}

static void destroy_manager(struct AsyncWorkMgr *manager)
{
    stop_threads(manager);
    if (manager->running)
        abort();
    release_queue(manager, &manager->requests);
    release_queue(manager, &manager->responses);
    work_pool_mgr_destroy(manager->pools);
    if (manager->ready)
        CloseHandle(manager->ready);
    if (manager->stop)
        CloseHandle(manager->stop);
    if (manager->lock_initialized)
        DeleteCriticalSection(&manager->lock);
    free(manager);
}

int async_work_mgr_create(int thread_count)
{
    struct AsyncWorkMgr *manager;
    if (g_asyncWorkMgr || thread_count <= 0 ||
        (size_t)thread_count > (SIZE_MAX - sizeof *manager) / sizeof manager->threads[0])
        return 0;
    manager = calloc(1, sizeof *manager + (size_t)thread_count * sizeof manager->threads[0]);
    if (!manager)
        return 0;
    manager->thread_count = (size_t)thread_count;
    if (!InitializeCriticalSectionAndSpinCount(&manager->lock, 0)) {
        free(manager);
        return 0;
    }
    manager->lock_initialized = 1;
    manager->ready = CreateEventA(NULL, TRUE, FALSE, NULL);
    manager->stop = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (manager->ready && manager->stop)
        manager->pools = work_pool_mgr_create();
    if (!manager->pools) {
        destroy_manager(manager);
        return 0;
    }
    g_asyncWorkMgr = manager;
    return 1;
}

void async_work_mgr_destroy(void)
{
    struct AsyncWorkMgr *manager = g_asyncWorkMgr;
    g_asyncWorkMgr = NULL;
    if (manager)
        destroy_manager(manager);
}

int async_work_mgr_start(void)
{
    struct AsyncWorkMgr *manager = g_asyncWorkMgr;
    if (!manager || manager->started_threads)
        return 0;
    reset_event(manager->stop);
    while (manager->started_threads < manager->thread_count) {
        HANDLE thread = (HANDLE)_beginthreadex(NULL, 0, worker_main, manager, 0, NULL);
        if (!thread) {
            stop_threads(manager);
            return 0;
        }
        manager->threads[manager->started_threads++] = thread;
    }
    return 1;
}

void async_work_mgr_stop(void)
{
    if (g_asyncWorkMgr)
        stop_threads(g_asyncWorkMgr);
}

void async_work_mgr_process_responses(unsigned count)
{
    struct AsyncWorkMgr *manager = g_asyncWorkMgr;
    unsigned i;
    if (!manager)
        return;
    for (i = 0; i < count; ++i) {
        struct QueuedWork *entry;
        EnterCriticalSection(&manager->lock);
        entry = manager->responses.first;
        if (entry) {
            manager->responses.first = entry->next;
            if (!manager->responses.first)
                manager->responses.last = NULL;
        }
        LeaveCriticalSection(&manager->lock);
        if (!entry)
            return;
        entry->work->ops->done(entry->work);
        work_pool_mgr_release(manager->pools, entry->work);
        free(entry);
    }
}

struct AsyncWork *async_work_mgr_acquire(struct AsyncWorkMgr *manager, const char *name)
{
    return work_pool_mgr_acquire(manager ? manager->pools : NULL, name);
}

int async_work_mgr_submit(struct AsyncWorkMgr *manager, struct AsyncWork *work)
{
    struct QueuedWork *entry;
    if (!manager || !work)
        return 0;
    entry = calloc(1, sizeof *entry);
    if (!entry) {
        work_pool_mgr_release(manager->pools, work);
        return 0;
    }
    entry->work = work;
    EnterCriticalSection(&manager->lock);
    append_work(&manager->requests, entry);
    signal_event(manager->ready);
    LeaveCriticalSection(&manager->lock);
    return 1;
}

void async_work_mgr_cancel(struct AsyncWorkMgr *manager, struct AsyncWork *work)
{
    if (manager && work)
        work_pool_mgr_release(manager->pools, work);
}
