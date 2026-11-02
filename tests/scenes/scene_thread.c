/* No client threads in scene runs. The model loader falls back to loading on the main thread when
   its thread cannot start, so every run loads in the same order and reaches the same frame; the
   music fade, the only other thread, simply does not run. Everything else is platform/thread.c. */
#define platform_thread_create platform_thread_create_unused
#include "platform/thread.c"
#undef platform_thread_create

struct PlatformThread *platform_thread_create(void (*entry)(void *context), void *context)
{
    (void)entry;
    (void)context;
    return NULL;
}
