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
static DWORD test_ticks(void) { return ticks; }
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
#include "../src/render/rp_atlas.c"
#undef timeGetTime
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
struct Texture *texture_create(int width, int height, int format)
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
    for (point = 1; point <= 4; ++point) {
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
int main(void)
{
    failures(); lifetimes(); reuse_and_wrap(); dimensions_and_spares();
    puts("Sprite atlas: C ownership, allocation/upload retry, typed regions, invalidation, queued draws, clock wrap, actual page sizes and spare reuse passed.");
    return 0;
}
