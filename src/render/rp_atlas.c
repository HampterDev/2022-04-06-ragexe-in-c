#include "render/sprite_atlas.h"
#include "render/atlas_index.h"
#include "render/renderer.h"
#include "render/texture.h"
#include "core/client.h"
#include "fileio/res_mgr.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "platform/language.h"
#include "platform/clock.h"

#define ATLAS_BUCKETS 25
#define ATLAS_SPARES 50
#define ATLAS_PAGE_SIZE 256
#define ATLAS_INDEX_FIRST 256

struct SpriteAtlasCell {
    const struct SprImg *image;
    const DWORD *palette;
    struct SpriteAtlasPage *page;
    struct SpriteRegion region;
    DWORD last_use;
    int occupied, pinned;
};

struct SpriteAtlasPage {
    struct SpriteAtlasPage *next;
    struct Texture *texture;
    struct SpriteAtlasCell *cells;
    size_t cell_count;
    int bucket, cell_width, cell_height, reusable;
    DWORD last_use;
};

/* index finds the cell holding an image and palette among the pages of pages[] (not retired), in
   the bucket of the image's size: every such cell with an image is in it, and nothing else. */
struct SpriteAtlasCache {
    struct SpriteAtlasPage *pages[ATLAS_BUCKETS];
    struct SpriteAtlasPage *retired;
    struct Texture *spares[ATLAS_SPARES];
    size_t spare_count;
    struct AtlasIndex index;
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
        struct Texture *texture = texture_create_layer(ATLAS_PAGE_SIZE, ATLAS_PAGE_SIZE, GFX_FMT_1555);
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
        texture = texture_create_layer(width, height, GFX_FMT_1555);
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
    page->bucket = bucket;
    page->cell_width = cell_width;
    page->cell_height = cell_height;
    for (i = 0; i < page->cell_count; ++i) {
        struct SpriteRegion *region = &page->cells[i].region;
        page->cells[i].page = page;
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

static void atlas_empty_cell(struct SpriteAtlasCache *cache, struct SpriteAtlasCell *cell)
{
    atlas_index_remove(&cache->index, cell->image, cell->palette, cell->page->bucket);
    cell->image = NULL;
    cell->palette = NULL;
}

/* The page leaves pages[]: its cells can no longer be found. */
static void atlas_forget_page(struct SpriteAtlasCache *cache, struct SpriteAtlasPage *page)
{
    size_t i;
    for (i = 0; i < page->cell_count; ++i)
        atlas_index_remove(&cache->index, page->cells[i].image, page->cells[i].palette, page->bucket);
}

static int atlas_index_room(struct SpriteAtlasCache *cache)
{
    struct AtlasIndex grown;
    struct AtlasIndexSlot *slots;
    size_t capacity;
    if (!atlas_index_full(&cache->index)) return 1;
    capacity = cache->index.capacity ? cache->index.capacity * 2 : ATLAS_INDEX_FIRST;
    if (capacity < cache->index.capacity) return 0;
    slots = calloc(capacity, sizeof *slots);
    if (!slots) return 0;
    atlas_index_init(&grown, slots, capacity);
    atlas_index_move(&grown, &cache->index);
    free(cache->index.slots);
    cache->index = grown;
    return 1;
}

struct Texture *sprite_atlas_resolve(struct Renderer *renderer, const struct SprImg *image,
                                      const DWORD *palette, struct SpriteRegion *region)
{
    struct SpriteAtlasCache *cache;
    struct SpriteAtlasPage *page;
    struct SpriteAtlasCell *available = NULL;
    size_t i;
    int bucket;
    DWORD now = platform_ticks_ms();
    if (!region) return NULL;
    memset(region, 0, sizeof *region);
    if (!image || !palette || !image->pixels || image->width <= 0 || image->height <= 0)
        return NULL;
    cache = atlas_cache(renderer);
    if (!cache) return NULL;
    bucket = atlas_step(image->width) * 5 + atlas_step(image->height);
    available = atlas_index_find(&cache->index, image, palette, bucket);
    if (available)
        return atlas_use(available->page, available, now, region);
    if (!atlas_index_room(cache)) return NULL;
    for (page = cache->pages[bucket]; page && !available; page = page->next) {
        if (image->width > page->cell_width || image->height > page->cell_height) continue;
        for (i = 0; i < page->cell_count; ++i) {
            struct SpriteAtlasCell *cell = &page->cells[i];
            if (!cell->occupied || (!cell->pinned && (DWORD)(now - cell->last_use) > (g_resPurgeAgeMs >> 2))) {
                available = cell;
                break;
            }
        }
    }
    if (!available) {
        page = atlas_page_create(cache, bucket, image);
        if (!page) return NULL;
        available = &page->cells[0];
    }
    page = available->page;
    atlas_empty_cell(cache, available);
    if (!texture_draw_sprite_frame(page->texture, available->region.x, available->region.y,
                                  page->cell_width, page->cell_height, image, palette))
        return NULL;
    available->image = image;
    available->palette = palette;
    available->occupied = 1;
    atlas_index_insert(&cache->index, image, palette, bucket, available);
    available->region.u1 = (float)(available->region.x + image->width) / (float)page->texture->base.width;
    available->region.v1 = (float)(available->region.y + image->height) / (float)page->texture->base.height;
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
                        && (address - begin) % sizeof *images == 0))
                    atlas_empty_cell(cache, cell);
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
            atlas_forget_page(cache, page);
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
    DWORD now = platform_ticks_ms();
    unsigned bucket;
    if (!cache) return;
    for (bucket = 0; bucket < ATLAS_BUCKETS; ++bucket) {
        struct SpriteAtlasPage **link = &cache->pages[bucket];
        while (*link) {
            struct SpriteAtlasPage *page = *link;
            if (!atlas_page_pinned(page) && (DWORD)(now - page->last_use) > g_resPurgeAgeMs) {
                *link = page->next;
                atlas_forget_page(cache, page);
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
    free(cache->index.slots);
    free(cache);
    renderer->sprite_atlas = NULL;
}
