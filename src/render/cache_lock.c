#include "render/cache_lock.h"
#include "render/texture_cache.h"
#include "fileio/res_mgr.h"
#define WIN32_LEAN_AND_MEAN
#include "platform/win_types.h"
#include "render/render_api.h"

enum {
    S_RELEASES, S_SKIPPED, S_CONTENDED, S_PURGES, S_LOADS, S_TEX_CONTENDED,
    S_LOOKUP_CALLS, S_LOOKUP_CONTENDED, S_WRITES, S_DANGEROUS, S_TEX_PURGES,
    S_TEX_RELEASES, S_TRYENTER, S_COUNT
};
unsigned cache_stat[16] = { 0 };

static BOOL (WINAPI *try_enter)(CRITICAL_SECTION *);
static int try_enter_state;
static volatile LONG write_depth;

static void cs_enter(CRITICAL_SECTION *cs, int calls, int contended)
{
    if (calls >= 0)
        cache_stat[calls]++;
    if (try_enter_state == 0) {
        HMODULE k = GetModuleHandleA("kernel32.dll");
        try_enter = k ? (BOOL (WINAPI *)(CRITICAL_SECTION *))GetProcAddress(k, "TryEnterCriticalSection") : NULL;
        try_enter_state = try_enter ? 2 : 1;
        cache_stat[S_TRYENTER] = try_enter_state;
    }
    if (try_enter_state == 2) {
        if (try_enter(cs))
            return;
        if (contended >= 0) {
            cache_stat[contended]++;
            if (write_depth)
                cache_stat[S_DANGEROUS]++;
        }
    }
    EnterCriticalSection(cs);
}

static void cs_leave(CRITICAL_SECTION *cs)
{
    LeaveCriticalSection(cs);
}

void cache_section_enter(CRITICAL_SECTION *cs, int calls, int contended)
{
    cs_enter(cs, calls, contended);
}

void cache_section_leave(CRITICAL_SECTION *cs)
{
    cs_leave(cs);
}

void cache_purge(struct ResMgr *self)
{
    CRITICAL_SECTION *cs = &self->lock;

    cs_enter(cs, S_PURGES, -1);
    res_mgr_purge_stale(self);
    cs_leave(cs);
}

void *tex_lock_enter(int operation)
{
    CRITICAL_SECTION *cs = &g_resMgr()->lock;

    if (operation == TEXTURE_CACHE_RELEASE)
        cs_enter(cs, S_TEX_RELEASES, -1);
    else if (operation == TEXTURE_CACHE_PURGE)
        cs_enter(cs, S_TEX_PURGES, -1);
    else if (operation == TEXTURE_CACHE_MAINTENANCE)
        cs_enter(cs, -1, -1);
    else
        cs_enter(cs, S_LOADS, S_TEX_CONTENDED);
    return cs;
}

void tex_lock_leave(void *cs)
{
    cs_leave((CRITICAL_SECTION *)cs);
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
