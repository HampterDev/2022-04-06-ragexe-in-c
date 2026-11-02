#ifdef _WIN32
#include "platform/thread.h"
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

struct PlatformMutex {
    CRITICAL_SECTION section;
};

struct PlatformThread {
    HANDLE handle;
    void (*entry)(void *context);
    void *context;
};

struct PlatformMutex *platform_mutex_create(void)
{
    struct PlatformMutex *mutex = malloc(sizeof *mutex);
    if (!mutex)
        abort();
    InitializeCriticalSection(&mutex->section);
    return mutex;
}

void platform_mutex_destroy(struct PlatformMutex *mutex)
{
    DeleteCriticalSection(&mutex->section);
    free(mutex);
}

void platform_mutex_lock(struct PlatformMutex *mutex)
{
    EnterCriticalSection(&mutex->section);
}

int platform_mutex_try_lock(struct PlatformMutex *mutex)
{
    return TryEnterCriticalSection(&mutex->section) != FALSE;
}

void platform_mutex_unlock(struct PlatformMutex *mutex)
{
    LeaveCriticalSection(&mutex->section);
}

struct PlatformEvent *platform_event_create(void)
{
    return (struct PlatformEvent *)CreateEventW(NULL, TRUE, FALSE, NULL);
}

void platform_event_destroy(struct PlatformEvent *event)
{
    CloseHandle((HANDLE)event);
}

void platform_event_set(struct PlatformEvent *event)
{
    SetEvent((HANDLE)event);
}

void platform_event_reset(struct PlatformEvent *event)
{
    ResetEvent((HANDLE)event);
}

int platform_event_wait(struct PlatformEvent *event, unsigned milliseconds)
{
    return WaitForSingleObject((HANDLE)event, milliseconds) == WAIT_OBJECT_0;
}

static unsigned __stdcall thread_main(void *argument)
{
    struct PlatformThread *thread = argument;
    thread->entry(thread->context);
    return 0;
}

struct PlatformThread *platform_thread_create(void (*entry)(void *context), void *context)
{
    struct PlatformThread *thread = malloc(sizeof *thread);
    if (!thread)
        return NULL;
    thread->entry = entry;
    thread->context = context;
    thread->handle = (HANDLE)_beginthreadex(NULL, 0, thread_main, thread, 0, NULL);
    if (!thread->handle) {
        free(thread);
        return NULL;
    }
    return thread;
}

void platform_thread_join(struct PlatformThread *thread)
{
    WaitForSingleObject(thread->handle, INFINITE);
    CloseHandle(thread->handle);
    free(thread);
}

void platform_thread_raise_priority(void)
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
}

void platform_thread_yield(void)
{
    Sleep(0);
}

void platform_thread_sleep(unsigned milliseconds)
{
    Sleep(milliseconds);
}

uint32_t platform_thread_id(void)
{
    return GetCurrentThreadId();
}

int32_t platform_atomic_increment(volatile int32_t *value)
{
    return InterlockedIncrement((volatile LONG *)value);
}

int32_t platform_atomic_decrement(volatile int32_t *value)
{
    return InterlockedDecrement((volatile LONG *)value);
}

int32_t platform_atomic_load(volatile int32_t *value)
{
    return InterlockedCompareExchange((volatile LONG *)value, 0, 0);
}
#endif
