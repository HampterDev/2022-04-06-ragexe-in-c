#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>
#include "web/work_pool_internal.h"

static void *checked_calloc(size_t count, size_t size);
static void checked_free(void *pointer);
static BOOL WINAPI checked_lock(LPCRITICAL_SECTION lock, DWORD spins);
static void WINAPI checked_unlock_destroy(LPCRITICAL_SECTION lock);
static HANDLE WINAPI checked_event(LPSECURITY_ATTRIBUTES attributes, BOOL manual, BOOL signaled, LPCSTR name);
static BOOL WINAPI checked_close(HANDLE handle);
static uintptr_t checked_thread(void *security, unsigned stack, unsigned (__stdcall *entry)(void *), void *argument, unsigned flags, unsigned *id);

#define calloc checked_calloc
#define free checked_free
#define InitializeCriticalSectionAndSpinCount checked_lock
#define DeleteCriticalSection checked_unlock_destroy
#define CreateEventA checked_event
#define CloseHandle checked_close
#define _beginthreadex checked_thread
#include "../src/web/work_pool.c"
#include "../src/web/work_mgr.c"
#undef _beginthreadex
#undef CloseHandle
#undef CreateEventA
#undef DeleteCriticalSection
#undef InitializeCriticalSectionAndSpinCount
#undef free
#undef calloc

static CRITICAL_SECTION bookkeeping;
static struct Allocation { unsigned char *raw; size_t size; } allocations[1024];
static struct Handle { HANDLE value; int thread; } handles[64];
static LONG allocated, allocation_attempts, fail_allocation;
static LONG locks, lock_attempts, fail_lock, open_handles;
static LONG event_attempts, fail_event, thread_attempts, fail_thread;
static LONG constructed, disposed, completed, active_groups[100];
static LONG slow_finished, slow_disposed;
static HANDLE slow_entered, slow_release;
static DWORD main_thread;
static unsigned delivered[1024], delivered_count;

static void *checked_calloc(size_t count, size_t size)
{
    unsigned i;
    size_t bytes;
    assert(!size || count <= SIZE_MAX / size);
    bytes = count * size;
    assert(bytes <= SIZE_MAX - 64);
    EnterCriticalSection(&bookkeeping);
    if (++allocation_attempts == fail_allocation) {
        LeaveCriticalSection(&bookkeeping);
        return NULL;
    }
    for (i = 0; i < 1024 && allocations[i].raw; ++i) {}
    assert(i < 1024);
    allocations[i].raw = malloc(bytes + 64);
    assert(allocations[i].raw);
    allocations[i].size = bytes;
    memset(allocations[i].raw, 0xa7, bytes + 64);
    memset(allocations[i].raw + 32, 0, bytes);
    ++allocated;
    LeaveCriticalSection(&bookkeeping);
    return allocations[i].raw + 32;
}

static void checked_free(void *pointer)
{
    unsigned i;
    size_t j;
    if (!pointer) return;
    assert(pointer != g_asyncWorkMgr);
    EnterCriticalSection(&bookkeeping);
    for (i = 0; i < 1024 && (!allocations[i].raw || allocations[i].raw + 32 != pointer); ++i) {}
    assert(i < 1024);
    for (j = 0; j < 32; ++j) {
        assert(allocations[i].raw[j] == 0xa7);
        assert(allocations[i].raw[32 + allocations[i].size + j] == 0xa7);
    }
    free(allocations[i].raw);
    allocations[i].raw = NULL;
    --allocated;
    LeaveCriticalSection(&bookkeeping);
}

static BOOL WINAPI checked_lock(LPCRITICAL_SECTION lock, DWORD spins)
{
    if (InterlockedIncrement(&lock_attempts) == fail_lock) return FALSE;
    assert(InitializeCriticalSectionAndSpinCount(lock, spins));
    InterlockedIncrement(&locks);
    return TRUE;
}

static void WINAPI checked_unlock_destroy(LPCRITICAL_SECTION lock)
{
    assert(InterlockedDecrement(&locks) >= 0);
    DeleteCriticalSection(lock);
}

static HANDLE remember_handle(HANDLE handle, int thread)
{
    unsigned i;
    assert(handle);
    EnterCriticalSection(&bookkeeping);
    for (i = 0; i < 64 && handles[i].value; ++i) {}
    assert(i < 64);
    handles[i].value = handle;
    handles[i].thread = thread;
    ++open_handles;
    LeaveCriticalSection(&bookkeeping);
    return handle;
}

static HANDLE WINAPI checked_event(LPSECURITY_ATTRIBUTES attributes, BOOL manual, BOOL signaled, LPCSTR name)
{
    if (InterlockedIncrement(&event_attempts) == fail_event) return NULL;
    assert(manual && !name);
    return remember_handle(CreateEventA(attributes, manual, signaled, name), 0);
}

static uintptr_t checked_thread(void *security, unsigned stack, unsigned (__stdcall *entry)(void *), void *argument, unsigned flags, unsigned *id)
{
    if (InterlockedIncrement(&thread_attempts) == fail_thread) return 0;
    return (uintptr_t)remember_handle((HANDLE)_beginthreadex(security, stack, entry, argument, flags, id), 1);
}

static BOOL WINAPI checked_close(HANDLE handle)
{
    unsigned i;
    DWORD code;
    EnterCriticalSection(&bookkeeping);
    for (i = 0; i < 64 && handles[i].value != handle; ++i) {}
    assert(i < 64);
    if (handles[i].thread)
        assert(GetExitCodeThread(handle, &code) && code == 0);
    handles[i].value = NULL;
    --open_handles;
    LeaveCriticalSection(&bookkeeping);
    return CloseHandle(handle);
}

struct Task {
    struct AsyncWork base;
    unsigned id, runs, done;
    int mode;
    LONG active;
    unsigned char payload[257];
    unsigned tail;
};

static void task_init(void *pointer)
{
    struct Task *task = pointer;
    task->tail = 0x1579aebc;
    InterlockedIncrement(&constructed);
}

static void task_dispose(void *pointer)
{
    struct Task *task = pointer;
    assert(task->tail == 0x1579aebc && !task->active);
    if (task->mode == 2) InterlockedIncrement(&slow_disposed);
    InterlockedIncrement(&disposed);
}

static char task_run(void *pointer)
{
    struct Task *task = pointer;
    unsigned i;
    assert(task->mode && GetCurrentThreadId() != main_thread);
    assert(task->base.started < 100);
    assert(InterlockedIncrement(&task->active) == 1);
    assert(InterlockedIncrement(&active_groups[task->base.started]) == 1);
    memset(task->payload, (unsigned char)task->id, sizeof task->payload);
    if (task->mode == 2) {
        assert(SetEvent(slow_entered));
        assert(WaitForSingleObject(slow_release, 10000) == WAIT_OBJECT_0);
        InterlockedIncrement(&slow_finished);
    }
    for (i = 0; i < 5; ++i) SwitchToThread();
    assert(task->payload[0] == (unsigned char)task->id && task->payload[256] == (unsigned char)task->id);
    assert(InterlockedDecrement(&active_groups[task->base.started]) == 0);
    assert(InterlockedDecrement(&task->active) == 0);
    ++task->runs;
    return task->mode == 1 && task->runs == 1;
}

static void task_done(void *pointer)
{
    struct Task *task = pointer;
    assert(GetCurrentThreadId() == main_thread && !task->done && !task->active);
    if (task->mode == 1) assert(task->runs == 2);
    if (task->mode == 2) assert(task->runs == 1);
    task->done = 1;
    if (delivered_count < 1024) delivered[delivered_count++] = task->id;
    InterlockedIncrement(&completed);
}

struct WorkPoolMgr *work_pool_mgr_create(void)
{
    const struct WorkPoolType type = {"task", sizeof(struct Task), task_init, task_dispose, {task_run, task_done}};
    return work_pool_create_with_types(&type, 1);
}

static struct Task *submit(unsigned id, unsigned timestamp, int mode)
{
    struct Task *task = (struct Task *)async_work_mgr_acquire(g_asyncWorkMgr, "task");
    assert(task);
    task->id = id;
    task->runs = task->done = 0;
    task->mode = mode;
    task->base.started = timestamp;
    assert(async_work_mgr_submit(g_asyncWorkMgr, &task->base));
    return task;
}

static void clean(void)
{
    unsigned i;
    assert(!g_asyncWorkMgr && !allocated && !locks && !open_handles);
    assert(constructed == disposed);
    for (i = 0; i < 100; ++i) assert(!active_groups[i]);
}

static void failures(void)
{
    unsigned i;
    assert(!async_work_mgr_create(0) && !async_work_mgr_create(-1));
    assert(!async_work_mgr_start());
    assert(!async_work_mgr_acquire(NULL, "task"));
    assert(!async_work_mgr_submit(NULL, NULL));
    async_work_mgr_stop(); async_work_mgr_process_responses(~0u); async_work_mgr_destroy();
    for (i = 1; i <= 2; ++i) {
        fail_allocation = allocation_attempts + i;
        assert(!async_work_mgr_create(3)); clean();
    }
    fail_allocation = 0;
    for (i = 1; i <= 2; ++i) {
        fail_lock = lock_attempts + i;
        assert(!async_work_mgr_create(3)); clean();
    }
    fail_lock = 0;
    for (i = 1; i <= 2; ++i) {
        fail_event = event_attempts + i;
        assert(!async_work_mgr_create(3)); clean();
    }
    fail_event = 0;
    for (i = 1; i <= 3; ++i) {
        assert(async_work_mgr_create(3));
        fail_thread = thread_attempts + i;
        assert(!async_work_mgr_start());
        assert(!g_asyncWorkMgr->started_threads && open_handles == 2);
        fail_thread = 0;
        assert(async_work_mgr_start());
        assert(!async_work_mgr_start() && g_asyncWorkMgr->started_threads == 3);
        async_work_mgr_destroy(); clean();
    }
    for (i = 1; i <= 2; ++i) {
        assert(async_work_mgr_create(2));
        fail_allocation = allocation_attempts + i;
        assert(!async_work_mgr_acquire(g_asyncWorkMgr, "task"));
        fail_allocation = 0;
        async_work_mgr_destroy(); clean();
    }
    assert(async_work_mgr_create(2));
    {
        struct AsyncWork *work = async_work_mgr_acquire(g_asyncWorkMgr, "task");
        assert(work);
        fail_allocation = allocation_attempts + 1;
        assert(!async_work_mgr_submit(g_asyncWorkMgr, work));
        assert(!g_asyncWorkMgr->requests.first && !g_asyncWorkMgr->pools->pools[0].first->leased);
        fail_allocation = 0;
    }
    async_work_mgr_destroy(); clean();
}

static void bounded_selection(void)
{
    struct QueuedWork *first, *second, *third, *fourth;
    LONG done = completed;
    unsigned i;
    assert(async_work_mgr_create(2));
    delivered_count = 0;
    for (i = 0; i < 41; ++i) submit(i, 10, 0);
    submit(41, 20, 0); submit(42, 30, 0);
    first = take_request(g_asyncWorkMgr);
    assert(((struct Task *)first->work)->id == 0);
    assert(!take_request(g_asyncWorkMgr));
    assert(WaitForSingleObject(g_asyncWorkMgr->ready, 0) == WAIT_TIMEOUT);
    finish_request(g_asyncWorkMgr, first, 0);
    assert(WaitForSingleObject(g_asyncWorkMgr->ready, 0) == WAIT_OBJECT_0);
    second = take_request(g_asyncWorkMgr);
    third = take_request(g_asyncWorkMgr);
    fourth = take_request(g_asyncWorkMgr);
    assert(((struct Task *)second->work)->id == 1);
    assert(((struct Task *)third->work)->id == 41);
    assert(((struct Task *)fourth->work)->id == 42);
    finish_request(g_asyncWorkMgr, third, 1);
    assert(take_request(g_asyncWorkMgr) == third);
    finish_request(g_asyncWorkMgr, fourth, 0);
    finish_request(g_asyncWorkMgr, third, 0);
    finish_request(g_asyncWorkMgr, second, 0);
    async_work_mgr_process_responses(0); assert(completed == done);
    async_work_mgr_process_responses(1); assert(completed == done + 1 && delivered[0] == 0);
    async_work_mgr_process_responses(~0u);
    assert(completed == done + 4 && delivered[1] == 42 && delivered[2] == 41 && delivered[3] == 1);
    async_work_mgr_destroy(); clean();
}

static void wait_completed(LONG expected)
{
    DWORD deadline = GetTickCount() + 10000;
    while (completed != expected && (LONG)(GetTickCount() - deadline) < 0) {
        async_work_mgr_process_responses(7);
        Sleep(1);
    }
    assert(completed == expected);
}

static void threaded_dispatch(void)
{
    unsigned cycle, i;
    for (cycle = 0; cycle < 25; ++cycle) {
        LONG expected = completed + 200;
        assert(async_work_mgr_create(2));
        for (i = 0; i < 200; ++i) submit(i, i / 4, 1);
        fail_allocation = allocation_attempts + 1;
        assert(async_work_mgr_start());
        wait_completed(expected);
        assert(allocation_attempts + 1 == fail_allocation);
        fail_allocation = 0;
        async_work_mgr_stop();
        async_work_mgr_destroy(); clean();
    }
    for (cycle = 0; cycle < 100; ++cycle) {
        LONG expected = completed + 4;
        assert(async_work_mgr_create(2));
        assert(!async_work_mgr_create(2));
        for (i = 0; i < 4; ++i) submit(i, i, 1);
        assert(async_work_mgr_start()); wait_completed(expected);
        async_work_mgr_stop();
        submit(9, 9, 1);
        assert(async_work_mgr_start()); wait_completed(expected + 1);
        async_work_mgr_destroy(); async_work_mgr_destroy(); clean();
    }
}

static unsigned __stdcall release_slow_job(void *unused)
{
    (void)unused;
    Sleep(3300);
    assert(!slow_finished && !slow_disposed);
    assert(SetEvent(slow_release));
    return 0;
}

static void slow_shutdown(void)
{
    unsigned destroy, i;
    for (destroy = 0; destroy < 2; ++destroy) {
        HANDLE helper;
        LONG done = completed;
        slow_finished = slow_disposed = 0;
        slow_entered = CreateEventA(NULL, TRUE, FALSE, NULL);
        slow_release = CreateEventA(NULL, TRUE, FALSE, NULL);
        assert(slow_entered && slow_release && async_work_mgr_create(1));
        submit(0, 0, 2);
        for (i = 1; i < 20; ++i) submit(i, 0, 1);
        assert(async_work_mgr_start());
        assert(WaitForSingleObject(slow_entered, 5000) == WAIT_OBJECT_0);
        helper = (HANDLE)_beginthreadex(NULL, 0, release_slow_job, NULL, 0, NULL);
        assert(helper);
        if (destroy) {
            async_work_mgr_destroy();
            assert(completed == done && slow_disposed == 1);
        } else {
            async_work_mgr_stop();
            assert(!slow_disposed && slow_finished == 1);
            async_work_mgr_process_responses(~0u);
            assert(completed == done + 1);
            async_work_mgr_destroy();
        }
        assert(slow_finished == 1);
        assert(WaitForSingleObject(helper, 5000) == WAIT_OBJECT_0);
        CloseHandle(helper); CloseHandle(slow_release); CloseHandle(slow_entered);
        clean();
    }
}

int main(void)
{
    main_thread = GetCurrentThreadId();
    InitializeCriticalSection(&bookkeeping);
    failures(); bounded_selection(); threaded_dispatch(); slow_shutdown();
    clean();
    DeleteCriticalSection(&bookkeeping);
    puts("Worker queue: private C ownership, creation/submission/start failures, bounded selection, timestamp serialization, allocation-free retries, main-thread delivery, 5000 stress jobs, 100 restart cycles and slow-job joined shutdown passed");
    return 0;
}
