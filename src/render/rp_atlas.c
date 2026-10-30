#include "render/sprite_atlas.h"
#include "render/renderer.h"
#include "render/texture.h"
#include "core/client.h"
#include "fileio/res_mgr.h"
#include <mmsystem.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "platform/language.h"

#define ATLAS_BUCKETS 25
#define ATLAS_SPARES 50
#define ATLAS_PAGE_SIZE 256
#define ATLAS_UV_TRIM 0.005f

struct SpriteAtlasCell {
    const struct SprImg *image;
    const DWORD *palette;
    struct SpriteRegion region;
    DWORD last_use;
    int occupied, pinned;
};

struct SpriteAtlasPage {
    struct SpriteAtlasPage *next;
    struct Texture *texture;
    struct SpriteAtlasCell *cells;
    size_t cell_count;
    int cell_width, cell_height, reusable;
    DWORD last_use;
};

struct SpriteAtlasCache {
    struct SpriteAtlasPage *pages[ATLAS_BUCKETS];
    struct SpriteAtlasPage *retired;
    struct Texture *spares[ATLAS_SPARES];
    size_t spare_count;
};

static int atlas_step(int side)
{
    if (side <= 32) return 0;
    if (side <= 64) return 1;
    if (side <= 128) return 2;
    return side <= 256 ? 3 : 4;
}

static struct SpriteAtlasCache *atlas_cache(struct Renderer *renderer)
{
    if (!renderer) return NULL;
    if (!renderer->sprite_atlas)
        renderer->sprite_atlas = calloc(1, sizeof *renderer->sprite_atlas);
    return renderer->sprite_atlas;
}

void sprite_atlas_initialize(struct Renderer *renderer)
{
    struct SpriteAtlasCache *cache = atlas_cache(renderer);
    if (!cache) return;
    while (cache->spare_count < ATLAS_SPARES) {
        struct Texture *texture = texture_create(ATLAS_PAGE_SIZE, ATLAS_PAGE_SIZE, GFX_FMT_1555);
        if (!texture) break;
        cache->spares[cache->spare_count++] = texture;
    }
}

static int atlas_page_pinned(const struct SpriteAtlasPage *page)
{
    size_t i;
    for (i = 0; i < page->cell_count; ++i)
        if (page->cells[i].pinned) return 1;
    return 0;
}

static void atlas_page_free(struct SpriteAtlasCache *cache, struct SpriteAtlasPage *page)
{
    if (page->reusable && cache->spare_count < ATLAS_SPARES)
        cache->spares[cache->spare_count++] = page->texture;
    else
        texture_destroy(page->texture);
    free(page->cells);
    free(page);
}

static struct SpriteAtlasPage *atlas_page_create(struct SpriteAtlasCache *cache, int bucket,
                                                const struct SprImg *image)
{
    struct SpriteAtlasPage *page;
    struct Texture *texture;
    int width = ATLAS_PAGE_SIZE, height = ATLAS_PAGE_SIZE, reusable = 0;
    int cell_width = 32 << (bucket / 5), cell_height = 32 << (bucket % 5);
    size_t columns, rows, i;
    if (image->width > width) width = (image->width / ATLAS_PAGE_SIZE + 1) * ATLAS_PAGE_SIZE;
    if (image->height > height) height = (image->height / ATLAS_PAGE_SIZE + 1) * ATLAS_PAGE_SIZE;
    if (image->width > cell_width) cell_width = image->width;
    if (image->height > cell_height) cell_height = image->height;
    page = calloc(1, sizeof *page);
    if (!page) return NULL;
    if (width == ATLAS_PAGE_SIZE && height == ATLAS_PAGE_SIZE && cache->spare_count) {
        texture = cache->spares[--cache->spare_count];
        reusable = 1;
    } else {
        texture = texture_create(width, height, GFX_FMT_1555);
    }
    page->texture = texture;
    page->reusable = reusable;
    if (!texture || texture->base.width < cell_width || texture->base.height < cell_height) {
        atlas_page_free(cache, page);
        return NULL;
    }
    columns = (size_t)texture->base.width / cell_width;
    rows = (size_t)texture->base.height / cell_height;
    if (rows > SIZE_MAX / columns || rows * columns > SIZE_MAX / sizeof *page->cells) {
        atlas_page_free(cache, page);
        return NULL;
    }
    page->cell_count = rows * columns;
    page->cells = calloc(page->cell_count, sizeof *page->cells);
    if (!page->cells) {
        atlas_page_free(cache, page);
        return NULL;
    }
    page->cell_width = cell_width;
    page->cell_height = cell_height;
    for (i = 0; i < page->cell_count; ++i) {
        struct SpriteRegion *region = &page->cells[i].region;
        region->x = (int)(i % columns) * cell_width;
        region->y = (int)(i / columns) * cell_height;
        region->u = (float)region->x / (float)texture->base.width;
        region->v = (float)region->y / (float)texture->base.height;
    }
    {
        struct SpriteAtlasPage **tail = &cache->pages[bucket];
        while (*tail) tail = &(*tail)->next;
        *tail = page;
    }
    return page;
}

static struct Texture *atlas_use(struct SpriteAtlasPage *page, struct SpriteAtlasCell *cell,
                                  DWORD now, struct SpriteRegion *region)
{
    cell->pinned = 1;
    cell->last_use = page->last_use = now;
    *region = cell->region;
    return page->texture;
}

struct Texture *sprite_atlas_resolve(struct Renderer *renderer, const struct SprImg *image,
                                      const DWORD *palette, struct SpriteRegion *region)
{
    struct SpriteAtlasCache *cache;
    struct SpriteAtlasPage *page, *available_page = NULL;
    struct SpriteAtlasCell *available = NULL;
    size_t i;
    int bucket;
    DWORD now = timeGetTime();
    if (!region) return NULL;
    memset(region, 0, sizeof *region);
    if (!image || !palette || !image->pixels || image->width <= 0 || image->height <= 0)
        return NULL;
    cache = atlas_cache(renderer);
    if (!cache) return NULL;
    bucket = atlas_step(image->width) * 5 + atlas_step(image->height);
    for (page = cache->pages[bucket]; page; page = page->next) {
        for (i = 0; i < page->cell_count; ++i) {
            struct SpriteAtlasCell *cell = &page->cells[i];
            if (cell->image == image && cell->palette == palette)
                return atlas_use(page, cell, now, region);
            if (!available && image->width <= page->cell_width && image->height <= page->cell_height
                && (!cell->occupied || (!cell->pinned && (DWORD)(now - cell->last_use) > (g_resPurgeAgeMs >> 2)))) {
                available = cell;
                available_page = page;
            }
        }
    }
    if (!available) {
        available_page = atlas_page_create(cache, bucket, image);
        if (!available_page) return NULL;
        available = &available_page->cells[0];
    }
    page = available_page;
    available->image = NULL;
    available->palette = NULL;
    if (!texture_draw_sprite_frame(page->texture, available->region.x, available->region.y,
                                  page->cell_width, page->cell_height, image, palette))
        return NULL;
    available->image = image;
    available->palette = palette;
    available->occupied = 1;
    available->region.u1 = (float)(available->region.x + image->width) / (float)page->texture->base.width;
    available->region.v1 = (float)(available->region.y + image->height) / (float)page->texture->base.height;
    if (g_serviceType == 0xa) {
        if (image->width > 64) available->region.u1 -= ATLAS_UV_TRIM;
        if (image->height > 64) available->region.v1 -= ATLAS_UV_TRIM;
    }
    return atlas_use(page, available, now, region);
}

static void atlas_invalidate(struct Renderer *renderer, const struct SprImg *images, size_t count,
                             const DWORD *palette)
{
    struct SpriteAtlasCache *cache = renderer ? renderer->sprite_atlas : NULL;
    unsigned bucket;
    if (!cache) return;
    for (bucket = 0; bucket < ATLAS_BUCKETS; ++bucket) {
        struct SpriteAtlasPage *page;
        for (page = cache->pages[bucket]; page; page = page->next) {
            size_t i;
            for (i = 0; i < page->cell_count; ++i) {
                struct SpriteAtlasCell *cell = &page->cells[i];
                uintptr_t address = (uintptr_t)cell->image, begin = (uintptr_t)images;
                if ((palette && cell->palette == palette)
                    || (images && address >= begin && (address - begin) / sizeof *images < count
                        && (address - begin) % sizeof *images == 0)) {
                    cell->image = NULL;
                    cell->palette = NULL;
                }
            }
        }
    }
}

void sprite_atlas_invalidate_images(struct Renderer *renderer, const struct SprImg *images, size_t count)
{
    atlas_invalidate(renderer, images, count, NULL);
}

void sprite_atlas_invalidate_palette(struct Renderer *renderer, const DWORD *palette)
{
    atlas_invalidate(renderer, NULL, 0, palette);
}

void sprite_atlas_clear(struct Renderer *renderer)
{
    struct SpriteAtlasCache *cache = renderer ? renderer->sprite_atlas : NULL;
    unsigned bucket;
    if (!cache) return;
    for (bucket = 0; bucket < ATLAS_BUCKETS; ++bucket) {
        struct SpriteAtlasPage *page = cache->pages[bucket];
        cache->pages[bucket] = NULL;
        while (page) {
            struct SpriteAtlasPage *next = page->next;
            if (atlas_page_pinned(page)) {
                page->next = cache->retired;
                cache->retired = page;
            } else {
                atlas_page_free(cache, page);
            }
            page = next;
        }
    }
}

void sprite_atlas_finish_frame(struct Renderer *renderer)
{
    struct SpriteAtlasCache *cache = renderer ? renderer->sprite_atlas : NULL;
    unsigned bucket;
    if (!cache) return;
    for (bucket = 0; bucket < ATLAS_BUCKETS; ++bucket) {
        struct SpriteAtlasPage *page;
        for (page = cache->pages[bucket]; page; page = page->next) {
            size_t i;
            for (i = 0; i < page->cell_count; ++i) page->cells[i].pinned = 0;
        }
    }
    while (cache->retired) {
        struct SpriteAtlasPage *page = cache->retired;
        cache->retired = page->next;
        atlas_page_free(cache, page);
    }
}

void sprite_atlas_purge(struct Renderer *renderer)
{
    struct SpriteAtlasCache *cache = renderer ? renderer->sprite_atlas : NULL;
    DWORD now = timeGetTime();
    unsigned bucket;
    if (!cache) return;
    for (bucket = 0; bucket < ATLAS_BUCKETS; ++bucket) {
        struct SpriteAtlasPage **link = &cache->pages[bucket];
        while (*link) {
            struct SpriteAtlasPage *page = *link;
            if (!atlas_page_pinned(page) && (DWORD)(now - page->last_use) > g_resPurgeAgeMs) {
                *link = page->next;
                atlas_page_free(cache, page);
            } else {
                link = &page->next;
            }
        }
    }
}

void sprite_atlas_destroy(struct Renderer *renderer)
{
    struct SpriteAtlasCache *cache = renderer ? renderer->sprite_atlas : NULL;
    if (!cache) return;
    sprite_atlas_finish_frame(renderer);
    sprite_atlas_clear(renderer);
    while (cache->spare_count) texture_destroy(cache->spares[--cache->spare_count]);
    free(cache);
    renderer->sprite_atlas = NULL;
}
