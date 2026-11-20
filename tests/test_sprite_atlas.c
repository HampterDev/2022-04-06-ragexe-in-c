#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "owned_storage_test_allocator.h"
#include "render/sprite_atlas.h"
#include "render/renderer.h"
#include "render/texture.h"
#include <mmsystem.h>

static DWORD ticks;
static uint32_t test_ticks(void) { return ticks; }
static void *checked_calloc(size_t count, size_t size)
{
    void *p;
    assert(!size || count <= SIZE_MAX / size);
    p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
#define calloc checked_calloc
#define free checked_free
#define timeGetTime test_ticks
#define platform_ticks_ms test_ticks
#include "../src/render/rp_atlas.c"
#undef timeGetTime
#undef platform_ticks_ms
#undef free
#undef calloc

unsigned g_resPurgeAgeMs = 1000;
static int failed_texture, failed_upload, clamp_width, clamp_height;
static unsigned textures, uploads;
struct PaintedCell { int x, y; DWORD color; };
struct TestTexture { struct Texture texture; struct PaintedCell painted[256]; size_t count; };
void surface_destroy(struct Surface *surface)
{ assert(textures); --textures; checked_free(surface); }
static int power_two(int x) { int n = 1; while (n < x) n *= 2; return n; }
struct Texture *texture_create_layer(int width, int height, int format)
{
    struct TestTexture *texture;
    assert(format == GFX_FMT_1555 && width > 0 && height > 0);
    if (failed_texture) return NULL;
    texture = checked_calloc(1, sizeof *texture);
    if (!texture) return NULL;
    width = power_two(width); height = power_two(height);
    if (clamp_width && width > clamp_width) width = clamp_width;
    if (clamp_height && height > clamp_height) height = clamp_height;
    texture->texture.base.width = texture->texture.reqWidth = width;
    texture->texture.base.height = texture->texture.reqHeight = height;
    ++textures; return &texture->texture;
}
int texture_draw_sprite_frame(struct Texture *texture, int x, int y, int w, int h,
                             const struct SprImg *frame, const DWORD *palette)
{
    struct TestTexture *capture = (struct TestTexture *)texture;
    size_t i;
    assert(x >= 0 && y >= 0 && w >= frame->width && h >= frame->height);
    assert(x + w <= texture->base.width && y + h <= texture->base.height);
    ++uploads;
    for (i = 0; i < capture->count; ++i)
        if (capture->painted[i].x == x && capture->painted[i].y == y) break;
    assert(i < 256);
    if (i == capture->count) ++capture->count;
    capture->painted[i].x = x; capture->painted[i].y = y;
    capture->painted[i].color = failed_upload ? 0xbad : palette[frame->pixels[0]];
    return !failed_upload;
}
static DWORD painted(struct Texture *texture, const struct SpriteRegion *region)
{
    struct TestTexture *capture = (struct TestTexture *)texture;
    size_t i;
    for (i = 0; i < capture->count; ++i)
        if (capture->painted[i].x == region->x && capture->painted[i].y == region->y)
            return capture->painted[i].color;
    abort();
}
static void empty_region(const struct SpriteRegion *region)
{ struct SpriteRegion zero = {0}; assert(!memcmp(region, &zero, sizeof zero)); }
static BYTE pixels[] = {1};
static DWORD palette[256] = {0, 0x1234};
static void clean(struct Renderer *renderer)
{
    sprite_atlas_destroy(renderer); sprite_atlas_destroy(renderer);
    assert(!textures && !outstanding && !renderer->sprite_atlas);
}
static void failures(void)
{
    unsigned point;
    struct SprImg image = {.width = 16, .height = 24, .pixels = pixels};
    struct SpriteRegion region;
    struct Renderer renderer = {0};
    for (point = 1; point <= 5; ++point) {
        memset(&region, 0xa7, sizeof region); fail_at = allocation_calls + point;
        assert(!sprite_atlas_resolve(&renderer, &image, palette, &region));
        empty_region(&region); fail_at = 0;
        assert(sprite_atlas_resolve(&renderer, &image, palette, &region)); clean(&renderer);
    }
    failed_texture = 1;
    assert(!sprite_atlas_resolve(&renderer, &image, palette, &region)); empty_region(&region);
    failed_texture = 0; failed_upload = 1;
    assert(!sprite_atlas_resolve(&renderer, &image, palette, &region)); empty_region(&region);
    {
        unsigned before = uploads;
        failed_upload = 0;
        assert(sprite_atlas_resolve(&renderer, &image, palette, &region));
        assert(uploads == before + 1);
        assert(sprite_atlas_resolve(&renderer, &image, palette, &region));
        assert(uploads == before + 1);
    }
    clean(&renderer);
    image.width = 0;
    assert(!sprite_atlas_resolve(&renderer, &image, palette, &region) && !renderer.sprite_atlas);
    empty_region(&region);
    assert(!sprite_atlas_resolve(NULL, NULL, NULL, &region)); empty_region(&region);
    sprite_atlas_invalidate_images(NULL, &image, 1); sprite_atlas_invalidate_palette(NULL, palette);
    sprite_atlas_finish_frame(NULL); sprite_atlas_clear(NULL); sprite_atlas_purge(NULL);
}
static void lifetimes(void)
{
    struct Renderer renderer = {0};
    struct SprImg images[3] = {{.width=16,.height=24,.pixels=pixels},
                              {.width=16,.height=24,.pixels=pixels},
                              {.width=16,.height=24,.pixels=pixels}};
    struct SpriteRegion first, second, third, changed;
    struct Texture *texture;
    unsigned before;
    ticks = 0;
    texture = sprite_atlas_resolve(&renderer, images, palette, &first);
    assert(texture && first.x == 0 && first.y == 0 && first.u1 == 16.0f/256.0f);
    assert(sprite_atlas_resolve(&renderer, images+1, palette, &second) == texture);
    assert(second.x == 32 && second.u == 32.0f/256.0f);
    assert(sprite_atlas_resolve(&renderer, images+2, palette, &third) == texture);
    sprite_atlas_invalidate_images(&renderer, images, 2);
    before = uploads;
    assert(sprite_atlas_resolve(&renderer, images+2, palette, &changed) == texture && uploads == before);
    ticks += 10000;
    assert(sprite_atlas_resolve(&renderer, images, palette, &changed) == texture && uploads == before+1);
    assert(changed.x != first.x && painted(texture, &first) == 0x1234);
    sprite_atlas_invalidate_palette(&renderer, palette); palette[1] = 0x5678;
    assert(sprite_atlas_resolve(&renderer, images, palette, &changed) == texture);
    assert(painted(texture, &changed) == 0x5678 && painted(texture, &first) == 0x1234);
    sprite_atlas_purge(&renderer); assert(textures == 1);
    sprite_atlas_clear(&renderer); assert(textures == 1 && painted(texture, &first) == 0x1234);
    assert(sprite_atlas_resolve(&renderer, images, palette, &changed) != texture && textures == 2);
    sprite_atlas_finish_frame(&renderer); assert(textures == 1);
    clean(&renderer); palette[1] = 0x1234;
}
static void reuse_and_wrap(void)
{
    struct Renderer renderer = {0};
    struct SprImg images[65]; struct SpriteRegion region;
    unsigned i;
    struct Texture *first;
    for (i=0;i<65;++i) { memset(&images[i],0,sizeof images[i]); images[i].width=images[i].height=32; images[i].pixels=pixels; }
    ticks = 0xfffffff0u;
    first = sprite_atlas_resolve(&renderer, images, palette, &region);
    for (i=1;i<64;++i) assert(sprite_atlas_resolve(&renderer,images+i,palette,&region)==first);
    ticks += 2000;
    assert(sprite_atlas_resolve(&renderer,images+64,palette,&region)!=first && textures==2);
    sprite_atlas_purge(&renderer); assert(textures==2);
    sprite_atlas_finish_frame(&renderer);
    sprite_atlas_invalidate_images(&renderer,images+64,1);
    assert(sprite_atlas_resolve(&renderer,images+64,palette,&region)==first && textures==2);
    sprite_atlas_finish_frame(&renderer); ticks += 2000;
    sprite_atlas_purge(&renderer); assert(!textures); clean(&renderer);
    ticks=0; g_resPurgeAgeMs=0;
    assert(sprite_atlas_resolve(&renderer,images,palette,&region)); clean(&renderer);
    g_resPurgeAgeMs=1000;
}
static void dimensions_and_spares(void)
{
    struct Renderer renderer = {0}; struct SpriteRegion a,b;
    struct SprImg images[2]={{.width=16,.height=16,.pixels=pixels},{.width=16,.height=16,.pixels=pixels}};
    struct Texture *texture;
    clamp_width=clamp_height=128;
    texture=sprite_atlas_resolve(&renderer,images,palette,&a);
    assert(texture && sprite_atlas_resolve(&renderer,images+1,palette,&b)==texture);
    assert(b.x==32 && b.u==0.25f); clean(&renderer);
    images[0].width=129;
    assert(!sprite_atlas_resolve(&renderer,images,palette,&a)); empty_region(&a); clean(&renderer);
    clamp_width=clamp_height=0;
    images[0].width=images[1].width=512;
    texture=sprite_atlas_resolve(&renderer,images,palette,&a);
    assert(texture && texture->base.width==1024 && a.u1==0.5f);
    assert(sprite_atlas_resolve(&renderer,images+1,palette,&b)==texture && b.x==512 && b.u==0.5f);
    clean(&renderer);
    images[0].width=65;images[0].height=33;
    assert(sprite_atlas_resolve(&renderer,images,palette,&a));
    assert(a.u1==65.0f/256.0f && a.v1==33.0f/256.0f);clean(&renderer);
    sprite_atlas_initialize(&renderer); assert(textures==50);
    assert(sprite_atlas_resolve(&renderer,images,palette,&a));
    assert(textures==50);
    sprite_atlas_clear(&renderer);
    sprite_atlas_finish_frame(&renderer); clean(&renderer);
    fail_at=allocation_calls+1; sprite_atlas_initialize(&renderer); assert(!renderer.sprite_atlas); fail_at=0;
    assert(sprite_atlas_resolve(&renderer,images,palette,&a)); clean(&renderer);
}
/* The cell the atlas's first form picked for an image by scanning its bucket's pages in order: the
   cell holding the image and palette, else the first cell it fits that is free or unused for a
   quarter of the purge age, else none (a new page). */
static struct SpriteAtlasCell *scan_pick(struct SpriteAtlasCache *cache, const struct SprImg *image,
                                         const DWORD *colors, int *hit)
{
    struct SpriteAtlasCell *available = NULL;
    struct SpriteAtlasPage *page;
    int bucket = atlas_step(image->width) * 5 + atlas_step(image->height);
    size_t i;
    *hit = 0;
    for (page = cache ? cache->pages[bucket] : NULL; page; page = page->next) {
        for (i = 0; i < page->cell_count; ++i) {
            struct SpriteAtlasCell *cell = &page->cells[i];
            if (cell->image == image && cell->palette == colors) { *hit = 1; return cell; }
            if (!available && image->width <= page->cell_width && image->height <= page->cell_height
                && (!cell->occupied || (!cell->pinned && (DWORD)(ticks - cell->last_use) > (g_resPurgeAgeMs >> 2))))
                available = cell;
        }
    }
    return available;
}
/* The index holds exactly the cells with an image in the live pages, each under its own key. */
static void index_matches_pages(struct SpriteAtlasCache *cache)
{
    unsigned bucket;
    size_t cells = 0, i;
    if (!cache) return;
    for (bucket = 0; bucket < ATLAS_BUCKETS; ++bucket) {
        struct SpriteAtlasPage *page;
        for (page = cache->pages[bucket]; page; page = page->next) {
            assert(page->bucket == (int)bucket);
            for (i = 0; i < page->cell_count; ++i) {
                struct SpriteAtlasCell *cell = &page->cells[i];
                assert(cell->page == page);
                if (!cell->image) continue;
                ++cells;
                assert(atlas_index_find(&cache->index, cell->image, cell->palette, page->bucket) == cell);
            }
        }
    }
    assert(cache->index.count == cells);
}
static size_t bucket_pages(struct SpriteAtlasCache *cache, int bucket)
{
    struct SpriteAtlasPage *page;
    size_t count = 0;
    for (page = cache ? cache->pages[bucket] : NULL; page; page = page->next) ++count;
    return count;
}
/* Fuzzed takes, uses, reuses, invalidations, purges and clears: every resolve picks the cell the
   scan picks on the same state, and the index matches the pages after every step. */
static void against_the_scan(unsigned seed)
{
    static const int sizes[][2] = {{16,16},{32,32},{33,20},{20,40},{64,64},{100,50},{129,129},
                                   {200,240},{300,40},{40,300},{600,30},{520,520}};
    static DWORD colors[3][256];
    struct Renderer renderer = {0};
    struct SprImg images[48];
    unsigned state = seed, step, resolves = 0, hits = 0, pages = 0, reuses = 0;
    for (step = 0; step < 48; ++step) {
        memset(&images[step], 0, sizeof images[step]);
        images[step].width = sizes[step % 12][0];
        images[step].height = sizes[step % 12][1];
        images[step].pixels = pixels;
    }
    for (step = 0; step < 3; ++step) colors[step][1] = 0x100u * (step + 1);
    ticks = 1000;
    for (step = 0; step < 60000; ++step) {
        unsigned op;
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        op = state % 1000;
        if (op < 800) {
            const struct SprImg *image = &images[(state >> 10) % 48];
            const DWORD *pal = colors[(state >> 20) % 3];
            struct SpriteAtlasCache *cache = renderer.sprite_atlas;
            int hit, bucket = atlas_step(image->width) * 5 + atlas_step(image->height);
            struct SpriteAtlasCell *expected = scan_pick(cache, image, pal, &hit);
            int reused = expected && !hit && expected->occupied;
            size_t before = bucket_pages(cache, bucket);
            unsigned uploads_before = uploads;
            struct SpriteRegion region;
            struct Texture *texture;
            failed_upload = op < 8;
            texture = sprite_atlas_resolve(&renderer, image, pal, &region);
            cache = renderer.sprite_atlas;
            ++resolves;
            if (failed_upload && !hit) {
                assert(!texture && uploads == uploads_before + 1);
                assert(!expected || !expected->image);
            } else if (expected) {
                assert(texture == expected->page->texture && region.x == expected->region.x
                       && region.y == expected->region.y && bucket_pages(cache, bucket) == before);
                assert(uploads == uploads_before + (hit ? 0u : 1u));
                assert(expected->image == image && expected->palette == pal && expected->pinned);
                hits += (unsigned)hit;
                reuses += (unsigned)reused;
            } else {
                struct SpriteAtlasPage *last = cache->pages[bucket];
                while (last->next) last = last->next;
                assert(bucket_pages(cache, bucket) == before + 1 && texture == last->texture);
                assert(region.x == 0 && region.y == 0 && last->cells[0].image == image);
                ++pages;
            }
            failed_upload = 0;
        } else if (op < 900) {
            ticks += state % 400;
        } else if (op < 950) {
            sprite_atlas_finish_frame(&renderer);
        } else if (op < 965) {
            unsigned first = (state >> 8) % 48;
            sprite_atlas_invalidate_images(&renderer, images + first, (state >> 16) % (48 - first + 1));
        } else if (op < 975) {
            sprite_atlas_invalidate_palette(&renderer, colors[(state >> 8) % 3]);
        } else if (op < 990) {
            ticks += 1100;
            sprite_atlas_purge(&renderer);
        } else if (op < 995) {
            sprite_atlas_clear(&renderer);
        } else {
            sprite_atlas_finish_frame(&renderer);
            sprite_atlas_clear(&renderer);
            sprite_atlas_finish_frame(&renderer);
            assert(!renderer.sprite_atlas || !renderer.sprite_atlas->index.count);
        }
        index_matches_pages(renderer.sprite_atlas);
    }
    assert(resolves > 40000 && hits > 5000 && reuses > 1000 && pages > 50);
    clean(&renderer);
}
int main(void)
{
    failures(); lifetimes(); reuse_and_wrap(); dimensions_and_spares();
    against_the_scan(0x2545f491u); against_the_scan(0x9e3779b9u); against_the_scan(0x7f4a7c15u);
    puts("Sprite atlas: C ownership, allocation/upload retry, typed regions, invalidation, queued draws, clock wrap, actual page sizes, spare reuse, and 180000 fuzzed steps picking the cells a scan of the pages picks passed.");
    return 0;
}
