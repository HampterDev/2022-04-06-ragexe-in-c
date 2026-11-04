#include "render/cache_lock.h"
#include "render/texture_cache.h"
#include "fileio/res_mgr.h"
#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "render/render_api.h"

enum {
    S_RELEASES, S_SKIPPED, S_CONTENDED, S_PURGES, S_LOADS, S_TEX_CONTENDED,
    S_LOOKUP_CALLS, S_LOOKUP_CONTENDED, S_WRITES, S_DANGEROUS, S_TEX_PURGES,
    S_TEX_RELEASES, S_COUNT
};
unsigned cache_stat[16] = { 0 };

static volatile LONG write_depth;

static void cs_enter(struct PlatformMutex *mutex, int calls, int contended)
{
    if (calls >= 0)
        cache_stat[calls]++;
    if (platform_mutex_try_lock(mutex))
        return;
    if (contended >= 0) {
        cache_stat[contended]++;
        if (write_depth)
            cache_stat[S_DANGEROUS]++;
    }
    platform_mutex_lock(mutex);
}

static void cs_leave(struct PlatformMutex *mutex)
{
    platform_mutex_unlock(mutex);
}

void cache_section_enter(struct PlatformMutex *mutex, int calls, int contended)
{
    cs_enter(mutex, calls, contended);
}

void cache_section_leave(struct PlatformMutex *mutex)
{
    cs_leave(mutex);
}

void cache_purge(struct ResMgr *self)
{
    struct PlatformMutex *mutex = self->lock;

    cs_enter(mutex, S_PURGES, -1);
    res_mgr_purge_stale(self);
    cs_leave(mutex);
}

void *tex_lock_enter(int operation)
{
    struct PlatformMutex *mutex = g_resMgr()->lock;

    if (operation == TEXTURE_CACHE_RELEASE)
        cs_enter(mutex, S_TEX_RELEASES, -1);
    else if (operation == TEXTURE_CACHE_PURGE)
        cs_enter(mutex, S_TEX_PURGES, -1);
    else if (operation == TEXTURE_CACHE_MAINTENANCE)
        cs_enter(mutex, -1, -1);
    else
        cs_enter(mutex, S_LOADS, S_TEX_CONTENDED);
    return mutex;
}

void tex_lock_leave(void *mutex)
{
    cs_leave(mutex);
}

void cache_lock_write_begin(void)
{
    write_depth++;
    cache_stat[S_WRITES]++;
}

void cache_lock_write_end(void)
{
    write_depth--;
}
