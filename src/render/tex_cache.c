#include "render/texture_cache.h"
#include "render/texture_globals.h"
#include "resources/bitmap_types.h"
#include "fileio/resource_api.h"
#include "game/context.h"
#include "skills/world_video.h"
#include "render/cache_lock.h"
#include "render/render_api.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <mmsystem.h>
#include "fileio/res_mgr.h"

struct TextureCacheStorage {
    struct Texture **textures;
    size_t count, capacity;
};

int g_textureDivisor = 1;
struct TexCache g_textureCache;
struct Texture g_whiteTexture;

static void *cache_enter(int operation)
{
    return tex_lock_enter(operation);
}

static void cache_leave(void *section)
{
    tex_lock_leave(section);
}

static void cache_write_begin(void)
{
    cache_lock_write_begin();
}

static void cache_write_end(void)
{
    cache_lock_write_end();
}

static size_t cache_lower_bound(const struct TextureCacheStorage *storage, const char *name)
{
    size_t first = 0, last = storage ? storage->count : 0;
    while (first < last) {
        size_t middle = first + (last - first) / 2;
        if (strcmp(storage->textures[middle]->name, name) < 0)
            first = middle + 1;
        else
            last = middle;
    }
    return first;
}

static struct Texture *cache_at_name(const struct TextureCacheStorage *storage,
    size_t index, const char *name)
{
    if (storage && index < storage->count && !strcmp(storage->textures[index]->name, name))
        return storage->textures[index];
    return NULL;
}

static int cache_reserve(struct TexCache *cache)
{
    struct TextureCacheStorage *storage = cache->storage;
    struct Texture **textures;
    size_t capacity, limit = SIZE_MAX / sizeof *textures;
    if (storage && storage->count < storage->capacity) return 1;
    if (storage && storage->count == limit) return 0;
    if (!storage) {
        storage = calloc(1, sizeof *storage);
        if (!storage) return 0;
    }
    capacity = storage->capacity ? storage->capacity * 2 : 16;
    if (capacity < storage->capacity || capacity > limit) capacity = limit;
    textures = realloc(storage->textures, capacity * sizeof *textures);
    if (!textures) {
        if (!cache->storage) free(storage);
        return 0;
    }
    storage->textures = textures;
    storage->capacity = capacity;
    cache->storage = storage;
    return 1;
}

static void cache_remove(struct TextureCacheStorage *storage, size_t index)
{
    struct Texture *texture = storage->textures[index];
    cache_write_begin();
    --storage->count;
    memmove(storage->textures + index, storage->textures + index + 1,
        (storage->count - index) * sizeof *storage->textures);
    texture_destroy(texture);
    cache_write_end();
}

void tex_cache_destruct(struct TexCache *cache)
{
    struct TextureCacheStorage *storage;
    size_t i;
    if (!cache) return;
    storage = cache->storage;
    cache->storage = NULL;
    if (!storage) return;
    for (i = 0; i < storage->count; ++i) texture_destroy(storage->textures[i]);
    free(storage->textures);
    free(storage);
}

void texcache_clear(struct TexCache *cache)
{
    void *section = cache_enter(TEXTURE_CACHE_MAINTENANCE);
    cache_write_begin();
    tex_cache_destruct(cache);
    cache_write_end();
    cache_leave(section);
}

struct Texture *texture_from_pixels(int w, int h, const DWORD *bits,
                                     int format, int keyed, int full_size)
{
    struct Texture *tex;

    if (full_size == 1) {
        tex = texture_create(w, h, format);
    } else {
        int th = h / g_textureDivisor, tw = w / g_textureDivisor;

        tex = texture_create(tw, th, format);
    }
    if (tex) {
        if (!texture_upload(tex, 0, 0, w, h, bits, keyed, 0)) {
            texture_destroy(tex);
            return NULL;
        }
        tex->fullSize = (BYTE)full_size;
    }
    return tex;
}

static struct Texture *black_texture(int key)
{
    DWORD zero = 0;
    int size = 1 / g_textureDivisor;
    struct Texture *tex = texture_create(size > 0 ? size : 1, size > 0 ? size : 1, 0);
    if (tex) {
        if (!texture_upload(tex, 0, 0, 1, 1, &zero, key, 0)) {
            texture_destroy(tex);
            return NULL;
        }
        tex->fullSize = 0;
    }
    return tex;
}

static struct Texture *texture_load(const char *name, int a, int b)
{
    if (strstr(name, ".bik")) {
        struct WorldVideoList *video_list = game_current_video_list();
        if (video_list) {
            struct WorldVideo *frame = world_video_list_open(video_list, name, 1);
            int w, h;
            if (world_video_dimensions(frame, &w, &h)) {
                struct Texture *tex;

                tex = texture_create(w, h, 4);
                if (tex)
                    return tex;
            }
        }
        return black_texture(a);
    } else {
        struct BitmapRes *res = (struct BitmapRes *)res_mgr_get(g_resMgr(), name);
        struct Texture *tex;

        if (!res)
            return black_texture(a);
        tex = texture_from_pixels(res->width, res->height, res->pixels,
                         res->hasAlpha != 0, a, (char)b);
        res_mgr_release(g_resMgr(), &res->res);
        return tex;
    }
}

struct Texture *texture_cache_get(struct TexCache *cache, const char *name,
    int keyed, int full_size)
{
    struct Texture *texture, *existing;
    size_t index, length = 0;
    void *section;
    if (!cache || !name) return &g_whiteTexture;
    while (length < sizeof g_whiteTexture.name && name[length]) ++length;
    if (length == sizeof g_whiteTexture.name) return &g_whiteTexture;
    section = cache_enter(TEXTURE_CACHE_LOAD);
    index = cache_lower_bound(cache->storage, name);
    texture = cache_at_name(cache->storage, index, name);
    if (texture) goto found;
    texture = texture_load(name, keyed, full_size);
    if (!texture) {
        texture = &g_whiteTexture;
        goto done;
    }
    index = cache_lower_bound(cache->storage, name);
    existing = cache_at_name(cache->storage, index, name);
    if (existing) {
        texture_destroy(texture);
        texture = existing;
        goto found;
    }
    cache_write_begin();
    if (!cache_reserve(cache)) {
        cache_write_end();
        texture_destroy(texture);
        texture = &g_whiteTexture;
        goto done;
    }
    memcpy(texture->name, name, length + 1);
    memmove(cache->storage->textures + index + 1, cache->storage->textures + index,
        (cache->storage->count - index) * sizeof *cache->storage->textures);
    cache->storage->textures[index] = texture;
    ++cache->storage->count;
    cache_write_end();
found:
    texture->lastUse = timeGetTime();
done:
    cache_leave(section);
    return texture;
}

void ui_tex_cache_release(struct TexCache *cache, struct Texture *texture)
{
    void *section;
    size_t index;
    if (!cache || !texture || texture == &g_whiteTexture) return;
    section = cache_enter(TEXTURE_CACHE_RELEASE);
    if (texture->lockCount == 0) {
        index = cache_lower_bound(cache->storage, texture->name);
        if (cache_at_name(cache->storage, index, texture->name) == texture)
            cache_remove(cache->storage, index);
    }
    cache_leave(section);
}

static void texture_reload(struct Texture *tex, int w, int h, const DWORD *bits)
{
    struct Texture replacement = {0};
    int width = w, height = h;
    if (w <= 0 || h <= 0 || !bits)
        return;
    if (tex->fullSize != 1) {
        width = w / g_textureDivisor;
        height = h / g_textureDivisor;
        if (width < 1) width = 1;
        if (height < 1) height = 1;
    }
    texture_init(&replacement, width, height, tex->format);
    if (texture_upload(&replacement, 0, 0, w, h, bits, 0, 0)) {
        surface_reset(&tex->base);
        tex->base = replacement.base;
        tex->reqWidth = replacement.reqWidth;
        tex->reqHeight = replacement.reqHeight;
        tex->colorKeyed = replacement.colorKeyed;
        replacement.base.surface = NULL;
    }
    surface_reset(&replacement.base);
}

void texcache_reload_all(struct TexCache *cache)
{
    void *section = cache_enter(TEXTURE_CACHE_MAINTENANCE);
    struct TextureCacheStorage *storage = cache ? cache->storage : NULL;
    size_t i;
    if (storage) {
        for (i = 0; i < storage->count; ++i) {
            struct Texture *texture = storage->textures[i];
            struct BitmapRes *res = (struct BitmapRes *)res_mgr_get(g_resMgr(), texture->name);
            if (res) {
                texture_reload(texture, res->width, res->height, res->pixels);
                res_mgr_release(g_resMgr(), &res->res);
            }
        }
    }
    cache_leave(section);
}

void texcache_evict(struct TexCache *cache)
{
    void *section = cache_enter(TEXTURE_CACHE_PURGE);
    struct TextureCacheStorage *storage = cache ? cache->storage : NULL;
    DWORD now = timeGetTime();
    size_t i = 0;
    while (storage && i < storage->count) {
        struct Texture *texture = storage->textures[i];
        if (texture->lockCount == 0 && (DWORD)(now - texture->lastUse) > g_resPurgeAgeMs)
            cache_remove(storage, i);
        else
            ++i;
    }
    HeapCompact(GetProcessHeap(), 0);
    gfx_evict_managed_textures(&g_3dDevice);
    cache_leave(section);
}

void texture_cache_destroy(void)
{
    tex_cache_destruct(&g_textureCache);
}

void texture_cache_init(void)
{
    atexit(texture_cache_destroy);
}
