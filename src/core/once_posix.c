#ifndef _WIN32
#include "core/once.h"
#include <pthread.h>
#include <stdint.h>

enum { ONCE_IDLE, ONCE_RUNNING, ONCE_DONE };

static pthread_mutex_t g_onceLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_onceDone = PTHREAD_COND_INITIALIZER;

static uintptr_t once_state(struct Once *once)
{
    return (uintptr_t)__atomic_load_n(&once->state, __ATOMIC_ACQUIRE);
}

int once_begin(struct Once *once)
{
    int pending;
    if (once_state(once) == ONCE_DONE)
        return 0;
    pthread_mutex_lock(&g_onceLock);
    while (once_state(once) == ONCE_RUNNING)
        pthread_cond_wait(&g_onceDone, &g_onceLock);
    pending = once_state(once) == ONCE_IDLE;
    if (pending)
        __atomic_store_n(&once->state, (void *)(uintptr_t)ONCE_RUNNING, __ATOMIC_RELAXED);
    pthread_mutex_unlock(&g_onceLock);
    return pending;
}

void once_complete(struct Once *once)
{
    pthread_mutex_lock(&g_onceLock);
    __atomic_store_n(&once->state, (void *)(uintptr_t)ONCE_DONE, __ATOMIC_RELEASE);
    pthread_cond_broadcast(&g_onceDone);
    pthread_mutex_unlock(&g_onceLock);
}
#endif
