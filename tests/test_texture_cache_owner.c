#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "owned_storage_test_allocator.h"

static void *checked_calloc(size_t n, size_t size)
{
    void *p;
    assert(!size || n <= SIZE_MAX / size);
    p = checked_malloc(n * size);
    if (p) memset(p, 0, n * size);
    return p;
}
static void *checked_realloc(void *old, size_t size)
{
    void *p = checked_malloc(size);
    if (p && old) {
        size_t n = ((struct Allocation *)old - 1)->size;
        memcpy(p, old, n < size ? n : size);
        checked_free(old);
    }
    return p;
}
static DWORD fixture_time;
static uint32_t fixture_clock(void) { return fixture_time; }
#define calloc checked_calloc
#define realloc checked_realloc
#define free checked_free
#define timeGetTime fixture_clock
#define platform_ticks_ms fixture_clock
#include "../src/render/tex_cache.c"
#undef timeGetTime
#undef platform_ticks_ms
#undef free
#undef realloc
#undef calloc
#include "../src/render/cache_lock.c"

struct gfx_surface { unsigned pixels; };
static struct ResMgr manager;
static int manager_live = 1, fail_upload, missing_bitmap, recurse;
static int resource_delay;
static unsigned live_textures, live_handles, loads, releases, purges;
static char loaded_names[8192][256];
static struct BitmapRes bitmap;
static DWORD pixels[256];
static struct TexCache *recursive_cache;
struct C3dDevice g_3dDevice;
unsigned g_resPurgeAgeMs = 15;

struct ResMgr *g_resMgr(void) { assert(manager_live); return &manager; }
void res_mgr_purge_stale(struct ResMgr *unused) { (void)unused; abort(); }
void *res_mgr_get(struct ResMgr *self, const char *name)
{
    assert(self == &manager && loads < 8192);
    cache_section_enter(&manager.lock, -1, -1);
    strcpy(loaded_names[loads++], name);
    if (resource_delay) Sleep(1);
    if (recurse) {
        recurse = 0;
        assert(texture_cache_get(recursive_cache, name, 0, 1) != &g_whiteTexture);
    }
    cache_section_leave(&manager.lock);
    return missing_bitmap ? NULL : &bitmap;
}
void res_mgr_release(struct ResMgr *self, struct Res *res)
{ assert(self == &manager && res == &bitmap.res); ++releases; }
void surface_reset(struct Surface *surface)
{
    if (surface->surface) { assert(live_handles); --live_handles; checked_free(surface->surface); }
    surface->surface = NULL;
}
void surface_destroy(struct Surface *surface)
{
    assert(live_textures); --live_textures;
    surface_reset(surface); checked_free(surface);
}
int texture_upload_pixels(struct Texture *texture, int x, int y, int w, int h,
    const DWORD *bits, int keyed, int pitch)
{
    assert(!x && !y && !pitch && w > 0 && h > 0 && bits);
    if (!texture->base.surface || fail_upload) return 0;
    texture->base.surface->pixels = bits[0];
    texture->reqWidth = w; texture->reqHeight = h; texture->colorKeyed = (BYTE)keyed;
    return 1;
}
struct Texture *texture_init(struct Texture *texture, int w, int h, int format)
{
    texture->base.width = w; texture->base.height = h;
    texture->reqWidth = w; texture->reqHeight = h; texture->format = format;
    texture->base.surface = w > 0 && h > 0 ? checked_calloc(1, sizeof(struct gfx_surface)) : NULL;
    if (texture->base.surface) ++live_handles;
    return texture;
}
struct Texture *texture_create(int w, int h, int format)
{
    struct Texture *texture = checked_calloc(1, sizeof *texture);
    if (!texture) return NULL;
    texture_init(texture, w, h, format);
    if (!texture->base.surface) { checked_free(texture); return NULL; }
    ++live_textures;
    return texture;
}
void gfx_evict_managed_textures(struct C3dDevice *device) { assert(device == &g_3dDevice); ++purges; }

static void empty(struct TexCache *cache)
{
    texcache_clear(cache); tex_cache_destruct(cache); tex_cache_destruct(cache);
    assert(!cache->storage && !live_textures && !live_handles && !outstanding);
}
static void ownership_and_failures(void)
{
    struct TexCache cache = {0};
    struct Texture *first, *second;
    unsigned i, before;
    char name[300];
    for (i = 1; i <= 4; ++i) {
        fail_at = allocation_calls + i;
        assert(texture_cache_get(&cache, "first.bmp", 0, 1) == &g_whiteTexture);
        fail_at = 0;
        empty(&cache);
    }
    fail_upload = 1;
    assert(texture_cache_get(&cache, "first.bmp", 0, 1) == &g_whiteTexture);
    fail_upload = 0; empty(&cache);
    first = texture_cache_get(&cache, "first.bmp", 1, 1);
    assert(first != &g_whiteTexture && first->colorKeyed && first->fullSize == 1);
    before = loads; fixture_time = 20;
    assert(texture_cache_get(&cache, "first.bmp", 0, 0) == first && loads == before && first->lastUse == 20);
    for (i = 0; i < 15; ++i) {
        sprintf(name, "%02u.bmp", i);
        assert(texture_cache_get(&cache, name, 0, 1) != &g_whiteTexture);
    }
    fail_at = allocation_calls + 3;
    assert(texture_cache_get(&cache, "growth.bmp", 0, 1) == &g_whiteTexture);
    fail_at = 0;
    assert(live_textures == 16 && texture_cache_get(&cache, "first.bmp", 0, 1) == first);
    assert(texture_cache_get(&cache, "growth.bmp", 0, 1) != &g_whiteTexture);
    first->lockCount = 1; before = live_textures;
    ui_tex_cache_release(&cache, first); assert(live_textures == before);
    first->lockCount = 0;
    second = texture_create(1, 1, 0); strcpy(second->name, first->name);
    ui_tex_cache_release(&cache, second); assert(live_textures == before + 1);
    texture_destroy(second);
    ui_tex_cache_release(&cache, first); assert(live_textures == before - 1);
    ui_tex_cache_release(&cache, NULL); ui_tex_cache_release(&cache, &g_whiteTexture);
    before = loads; memset(name, 'a', sizeof name); name[299] = 0;
    assert(texture_cache_get(&cache, name, 0, 1) == &g_whiteTexture && loads == before);
    name[255] = 0;
    assert(strlen(texture_cache_get(&cache, name, 0, 1)->name) == 255);
    assert(texture_cache_get(NULL, "x", 0, 1) == &g_whiteTexture);
    assert(texture_cache_get(&cache, NULL, 0, 1) == &g_whiteTexture);
    empty(&cache);
}

static void lookup_reload_and_recursion(void)
{
    const char *names[] = {"z.bmp", "a.bmp", "한글.bmp", "A.bmp", ""};
    struct TexCache cache = {0};
    struct Texture *textures[5];
    gfx_surface *old;
    unsigned i, start;
    for (i = 0; i < 5; ++i) textures[i] = texture_cache_get(&cache, names[i], 0, 1);
    assert(textures[1] != textures[3]);
    start = loads; pixels[0] = 99;
    texcache_reload_all(&cache);
    assert(loads == start + 5);
    for (i = start + 1; i < loads; ++i) assert(strcmp(loaded_names[i-1], loaded_names[i]) < 0);
    for (i = 0; i < 5; ++i) assert(textures[i]->base.surface->pixels == 99);
    old = textures[0]->base.surface;
    fail_upload = 1; pixels[0] = 101; texcache_reload_all(&cache); fail_upload = 0;
    assert(textures[0]->base.surface == old && old->pixels == 99);
    missing_bitmap = 1; texcache_reload_all(&cache); missing_bitmap = 0;
    assert(textures[0]->base.surface == old);
    recursive_cache = &cache; recurse = 1; start = live_textures;
    assert(texture_cache_get(&cache, "recursive.bmp", 0, 1) != &g_whiteTexture);
    assert(live_textures == start + 1);
    empty(&cache);
}

static void time_and_missing_textures(void)
{
    struct TexCache cache = {0};
    struct Texture *recent, *locked, *texture;
    fixture_time = 1;
    recent = texture_cache_get(&cache, "recent.bmp", 0, 1);
    texcache_evict(&cache); assert(live_textures == 1);
    fixture_time = 16; texcache_evict(&cache); assert(live_textures == 1);
    fixture_time = 17; texcache_evict(&cache); assert(!live_textures);
    fixture_time = 0xfffffff0;
    locked = texture_cache_get(&cache, "locked.bmp", 0, 1); locked->lockCount = 1;
    recent = texture_cache_get(&cache, "recent.bmp", 0, 1);
    fixture_time = 10; texcache_evict(&cache); assert(live_textures == 1);
    assert(texture_cache_get(&cache, "locked.bmp", 0, 1) == locked);
    locked->lockCount = 0; empty(&cache);
    (void)recent;
    missing_bitmap = 1;
    texture = texture_cache_get(&cache, "missing.bmp", 0, 1);
    assert(texture != &g_whiteTexture && texture->base.width == 1 && texture->base.surface->pixels == 0);
    missing_bitmap = 0; empty(&cache);
}

static DWORD WINAPI lookup_thread(void *context)
{
    struct TexCache *cache = context;
    unsigned i;
    char name[32];
    for (i = 0; i < 1024; ++i) {
        sprintf(name, "parallel-%02u.bmp", i % 64);
        assert(texture_cache_get(cache, name, 0, 1) != &g_whiteTexture);
    }
    return 0;
}
static void parallel_lookup(void)
{
    struct TexCache cache = {0};
    HANDLE threads[4];
    unsigned i, start = loads;
    resource_delay = 1;
    for (i = 0; i < 4; ++i) {
        threads[i] = CreateThread(NULL, 0, lookup_thread, &cache, 0, NULL);
        assert(threads[i]);
    }
    assert(WaitForMultipleObjects(4, threads, TRUE, 10000) == WAIT_OBJECT_0);
    for (i = 0; i < 4; ++i) CloseHandle(threads[i]);
    resource_delay = 0;
    assert(loads == start + 64 && live_textures == 64);
    empty(&cache);
    puts("Texture cache synchronization: four threads, 4096 lookups and exactly 64 owned textures passed.");
}

int main(void)
{
    struct TexCache final_cache = {0};
    InitializeCriticalSection(&manager.lock);
    bitmap.width = bitmap.height = 16; bitmap.pixels = pixels; pixels[0] = 7;
    ownership_and_failures(); lookup_reload_and_recursion(); time_and_missing_textures();
    parallel_lookup(); assert(!write_depth && cache_stat[S_TEX_PURGES] == purges);
    assert(texture_cache_get(&final_cache, "shutdown.bmp", 0, 1) != &g_whiteTexture);
    DeleteCriticalSection(&manager.lock); manager_live = 0;
    tex_cache_destruct(&final_cache); tex_cache_destruct(&final_cache);
    assert(!outstanding && !live_textures && !live_handles);
    puts("Texture cache: C ownership, guarded allocation failures, stable textures, ordered reload, names, recursive loads, wraparound eviction and shutdown passed.");
    return 0;
}
