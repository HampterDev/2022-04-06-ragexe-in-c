#ifndef PLATFORM_THREAD_H
#define PLATFORM_THREAD_H

#include <stdint.h>

struct PlatformMutex;
struct PlatformEvent;
struct PlatformThread;

struct PlatformMutex *platform_mutex_create(void);
void platform_mutex_destroy(struct PlatformMutex *mutex);
void platform_mutex_lock(struct PlatformMutex *mutex);
int platform_mutex_try_lock(struct PlatformMutex *mutex);
void platform_mutex_unlock(struct PlatformMutex *mutex);

struct PlatformEvent *platform_event_create(void);
void platform_event_destroy(struct PlatformEvent *event);
void platform_event_set(struct PlatformEvent *event);
void platform_event_reset(struct PlatformEvent *event);
int platform_event_wait(struct PlatformEvent *event, unsigned milliseconds);

struct PlatformThread *platform_thread_create(void (*entry)(void *context), void *context);
void platform_thread_join(struct PlatformThread *thread);
void platform_thread_raise_priority(void);
void platform_thread_yield(void);
void platform_thread_sleep(unsigned milliseconds);
uint32_t platform_thread_id(void);

int32_t platform_atomic_increment(volatile int32_t *value);
int32_t platform_atomic_decrement(volatile int32_t *value);
int32_t platform_atomic_load(volatile int32_t *value);

#endif
