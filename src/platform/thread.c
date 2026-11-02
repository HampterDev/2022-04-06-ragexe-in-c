#include "platform/thread.h"
#include <stdlib.h>

#ifdef _WIN32
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
#else
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <time.h>

struct PlatformMutex {
    pthread_mutex_t mutex;
};

struct PlatformEvent {
    pthread_mutex_t lock;
    pthread_cond_t changed;
    int signaled;
};

struct PlatformThread {
    pthread_t handle;
    void (*entry)(void *context);
    void *context;
};

struct PlatformMutex *platform_mutex_create(void)
{
    struct PlatformMutex *mutex = malloc(sizeof *mutex);
    pthread_mutexattr_t attributes;
    if (!mutex || pthread_mutexattr_init(&attributes))
        abort();
    if (pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE) ||
        pthread_mutex_init(&mutex->mutex, &attributes))
        abort();
    pthread_mutexattr_destroy(&attributes);
    return mutex;
}

void platform_mutex_destroy(struct PlatformMutex *mutex)
{
    pthread_mutex_destroy(&mutex->mutex);
    free(mutex);
}

void platform_mutex_lock(struct PlatformMutex *mutex)
{
    if (pthread_mutex_lock(&mutex->mutex))
        abort();
}

int platform_mutex_try_lock(struct PlatformMutex *mutex)
{
    return pthread_mutex_trylock(&mutex->mutex) == 0;
}

void platform_mutex_unlock(struct PlatformMutex *mutex)
{
    if (pthread_mutex_unlock(&mutex->mutex))
        abort();
}

struct PlatformEvent *platform_event_create(void)
{
    struct PlatformEvent *event = malloc(sizeof *event);
    if (!event)
        return NULL;
    if (pthread_mutex_init(&event->lock, NULL)) {
        free(event);
        return NULL;
    }
    if (pthread_cond_init(&event->changed, NULL)) {
        pthread_mutex_destroy(&event->lock);
        free(event);
        return NULL;
    }
    event->signaled = 0;
    return event;
}

void platform_event_destroy(struct PlatformEvent *event)
{
    pthread_cond_destroy(&event->changed);
    pthread_mutex_destroy(&event->lock);
    free(event);
}

void platform_event_set(struct PlatformEvent *event)
{
    pthread_mutex_lock(&event->lock);
    event->signaled = 1;
    pthread_cond_broadcast(&event->changed);
    pthread_mutex_unlock(&event->lock);
}

void platform_event_reset(struct PlatformEvent *event)
{
    pthread_mutex_lock(&event->lock);
    event->signaled = 0;
    pthread_mutex_unlock(&event->lock);
}

int platform_event_wait(struct PlatformEvent *event, unsigned milliseconds)
{
    struct timespec deadline;
    int signaled, result = 0;
    if (clock_gettime(CLOCK_REALTIME, &deadline))
        abort();
    deadline.tv_sec += milliseconds / 1000;
    deadline.tv_nsec += (long)(milliseconds % 1000) * 1000000;
    if (deadline.tv_nsec >= 1000000000) {
        ++deadline.tv_sec;
        deadline.tv_nsec -= 1000000000;
    }
    pthread_mutex_lock(&event->lock);
    while (!event->signaled && milliseconds && result != ETIMEDOUT)
        result = pthread_cond_timedwait(&event->changed, &event->lock, &deadline);
    signaled = event->signaled;
    pthread_mutex_unlock(&event->lock);
    return signaled;
}

static void *thread_main(void *argument)
{
    struct PlatformThread *thread = argument;
    thread->entry(thread->context);
    return NULL;
}

struct PlatformThread *platform_thread_create(void (*entry)(void *context), void *context)
{
    struct PlatformThread *thread = malloc(sizeof *thread);
    if (!thread)
        return NULL;
    thread->entry = entry;
    thread->context = context;
    if (pthread_create(&thread->handle, NULL, thread_main, thread)) {
        free(thread);
        return NULL;
    }
    return thread;
}

void platform_thread_join(struct PlatformThread *thread)
{
    pthread_join(thread->handle, NULL);
    free(thread);
}

void platform_thread_raise_priority(void)
{
    struct sched_param parameters;
    int policy;
    if (pthread_getschedparam(pthread_self(), &policy, &parameters))
        return;
    parameters.sched_priority = sched_get_priority_max(policy);
    pthread_setschedparam(pthread_self(), policy, &parameters);
}

void platform_thread_yield(void)
{
    sched_yield();
}

uint32_t platform_thread_id(void)
{
    static uint32_t next_id;
    static _Thread_local uint32_t id;
    if (!id)
        id = __atomic_add_fetch(&next_id, 1, __ATOMIC_RELAXED);
    return id;
}

int32_t platform_atomic_increment(volatile int32_t *value)
{
    return __atomic_add_fetch(value, 1, __ATOMIC_SEQ_CST);
}

int32_t platform_atomic_decrement(volatile int32_t *value)
{
    return __atomic_sub_fetch(value, 1, __ATOMIC_SEQ_CST);
}

int32_t platform_atomic_load(volatile int32_t *value)
{
    return __atomic_load_n(value, __ATOMIC_SEQ_CST);
}
#endif
