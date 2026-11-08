#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "owned_storage_test_allocator.h"

static int fail_dib;
static HBITMAP test_dib(HDC dc, const BITMAPINFO *info, UINT use, void **bits, HANDLE section, DWORD offset)
{
    if (fail_dib) return NULL;
    return CreateDIBSection(dc, info, use, bits, section, offset);
}
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
#define CreateDIBSection test_dib
#include "../src/render/cdc.c"
#undef CreateDIBSection
#undef free
#undef calloc

static unsigned creates, live, uploads, draws, fail_texture, fail_upload;
static unsigned sprite_draws;
static int atlas_failure, source_x[4], source_y[4];
static struct Texture atlas_texture;
static struct Texture *drawn[16];
static int draw_width[16], draw_height[16];
static const DWORD *uploaded[16];
void surface_destroy(struct Surface *surface)
{
    assert(surface && live); --live; checked_free(surface);
}
int texture_upload_pixels(struct Texture *tex, int x, int y, int w, int h,
                           const DWORD *pixels, int keyed, int pitch)
{
    assert(!x && !y && !keyed && pitch >= w);
    assert(w == tex->reqWidth && h == tex->reqHeight && pixels);
    assert(uploads < 16); uploaded[uploads++] = pixels;
    return uploads != fail_upload;
}
void texture_draw_surface(struct Texture *tex, int x, int y, int w, int h, DWORD color)
{
    (void)x; (void)y;
    assert(tex && color == 0x12345678 && w > 0 && h > 0 && draws < 16);
    drawn[draws] = tex;
    draw_width[draws] = w; draw_height[draws++] = h;
}
struct Texture *texture_create(int w, int h, int format)
{
    struct Texture *tex;
    if (++creates == fail_texture) return NULL;
    tex = checked_calloc(1, sizeof *tex);
    if (!tex) return NULL;
    tex->reqWidth = w; tex->reqHeight = h; tex->format = format;
    ++live; return tex;
}
struct Renderer *g_renderer;
struct TexCache g_textureCache;
struct Texture *texture_from_pixels(int w,int h,const DWORD *bits,int a,int b,int d)
{ (void)w;(void)h;(void)bits;(void)a;(void)b;(void)d;abort();return NULL; }
struct SprClip *motion_get_clip(const struct Motion *m, unsigned i)
{ assert(i < m->clip_count); return &m->clips[i]; }
struct Texture *sprite_atlas_resolve(struct Renderer *renderer, const struct SprImg *image,
    const DWORD *palette, struct SpriteRegion *region)
{
    (void)renderer; assert(image && palette && region); memset(region, 0, sizeof *region);
    if (atlas_failure) return NULL;
    region->u = 0.25f; region->v = 0.5f; region->x = 32; region->y = 64;
    return &atlas_texture;
}
void dc_bitmap_draw_texels(struct DcBitmap *b, int x, int y, struct Texture *t, int sx, int sy, int sw, int sh, int m, int cw, int ch)
{ (void)b;(void)x;(void)y;(void)t;(void)sx;(void)sy;(void)sw;(void)sh;(void)m;(void)cw;(void)ch;abort(); }
void dc_bitmap_draw_texels_scaled(struct DcBitmap *b,int x,int y,int dw,int dh,struct Texture *t,int sw,int sh,float zx,float zy,DWORD c,char m,int ox,int oy,int sx,int sy)
{
    (void)x;(void)y;(void)dw;(void)dh;(void)c;(void)m;(void)ox;(void)oy;
    assert(b && t && sw==8 && sh==8 && zx==1 && zy==1 && sprite_draws<4);
    source_x[sprite_draws]=sx; source_y[sprite_draws++]=sy;
}

static void clear_counts(void) { creates = uploads = draws = 0; fail_texture = fail_upload = 0; }
static void allocation_failures(int single)
{
    unsigned failure, failures = single ? 2 : 7;
    for (failure = 1; failure <= failures; ++failure) {
        struct DcBitmap bitmap;
        clear_counts();
        assert(dc_bitmap_construct(&bitmap, 513, 257)); bitmap.singleTexture = (BYTE)single;
        memset(bitmap.bits, 0x29, 513 * 257 * 4);
        fail_at = allocation_calls + failure;
        dc_bitmap_upload(&bitmap);
        assert(bitmap.changed && !bitmap.textures && !bitmap.texture_count && !uploads && !live && !outstanding);
        fail_at = 0; clear_counts();
        dc_bitmap_upload(&bitmap);
        assert(!bitmap.changed && bitmap.texture_count == (single ? 1u : 6u));
        assert(uploads == bitmap.texture_count && live == bitmap.texture_count);
        if (single) assert(bitmap.bits[0] == 0x29292929 && bitmap.textures[0]->format == 4);
        else {
            assert(bitmap.bits[0] == 0xff292929 && bitmap.textures[0]->format == 0);
            assert(uploaded[3] == bitmap.bits + 513 * 256);
            assert(bitmap.textures[5]->reqWidth == 1 && bitmap.textures[5]->reqHeight == 1);
        }
        dc_bitmap_draw(&bitmap, 4, 8, 100, 257, 0x12345678);
        assert(draws == (single ? 1u : 2u));
        assert(drawn[0] == bitmap.textures[0] && draw_width[0] == 100);
        if (!single) assert(drawn[1] == bitmap.textures[3] && draw_height[1] == 1);
        dc_bitmap_destruct(&bitmap); dc_bitmap_destruct(&bitmap);
        assert(!live && !outstanding);
    }
}
static void sprite_regions(void)
{
    struct DcBitmap bitmap = {0}; struct SprRes sprite = {0};
    struct Texture rgba_texture = {0};
    struct SprImg indexed = {.width=8,.height=8}, rgba = {.width=8,.height=8,.texture=&rgba_texture};
    struct SprClip clips[2] = {{.sprIndex=0,.zoomX=1,.zoomY=1},
                               {.sprIndex=0,.clipType=1,.zoomX=1,.zoomY=1}};
    struct Motion motion = {.clips=clips,.clip_count=2};
    sprite.images[0].items=&indexed; sprite.images[0].count=1;
    sprite.images[1].items=&rgba; sprite.images[1].count=1;
    dc_bitmap_draw_sprite(&bitmap,0,0,32,32,&sprite,&motion,0,0,1,NULL);
    assert(sprite_draws==2 && source_x[0]==32 && source_y[0]==64 && source_x[1]==0 && source_y[1]==0);
    atlas_failure=1; sprite_draws=0;
    dc_bitmap_draw_sprite(&bitmap,0,0,32,32,&sprite,&motion,0,0,1,NULL);
    assert(sprite_draws==1 && source_x[0]==0 && source_y[0]==0);
    rgba.texture=NULL; sprite_draws=0;
    dc_bitmap_draw_sprite(&bitmap,0,0,32,32,&sprite,&motion,0,0,1,NULL);
    assert(!sprite_draws); atlas_failure=0;
}
static void heap_canvas_lifetime(void)
{
    struct DcBitmap *bitmap;
    HDC dc=(HDC)(uintptr_t)1;
    size_t i;
    assert(!dc_bitmap_get_dc(NULL,&dc) && !dc);
    dc_bitmap_destroy(NULL);
    fail_at=allocation_calls+1; assert(!dc_bitmap_create(3,2) && !outstanding); fail_at=0;
    fail_dib=1; assert(!dc_bitmap_create(3,2) && !outstanding); fail_dib=0;
    assert(!dc_bitmap_create(-1,2) && !outstanding);
    bitmap=dc_bitmap_create(3,2); assert(bitmap && outstanding==1);
    assert(bitmap->width==3 && bitmap->height==2 && bitmap->hdc && bitmap->bits);
    for(i=0;i<6;++i) assert(bitmap->bits[i]==CDC_COLOR_KEY);
    bitmap->changed=0; assert(dc_bitmap_get_dc(bitmap,&dc) && dc==bitmap->hdc && bitmap->changed);
    dc_bitmap_destroy(bitmap); assert(!outstanding);
}
int main(void)
{
    struct DcBitmap bitmap;
    unsigned i;
    DWORD before, *bits;
    HBITMAP dib;
    heap_canvas_lifetime();
    sprite_regions();
    allocation_failures(0); allocation_failures(1);
    for (i = 1; i <= 6; ++i) {
        clear_counts(); assert(dc_bitmap_construct(&bitmap, 513, 257));
        fail_upload = i;
        dc_bitmap_draw(&bitmap, 0, 0, 513, 257, 0x12345678);
        assert(bitmap.changed && bitmap.texture_count == 6 && uploads == i && !draws && live == 6);
        clear_counts(); dc_bitmap_draw(&bitmap, 0, 0, 513, 257, 0x12345678);
        assert(!bitmap.changed && uploads == 6 && draws == 6 && !creates);
        dc_bitmap_destruct(&bitmap); assert(!live && !outstanding);
    }
    for (i = 1; i <= 6; ++i) {
        clear_counts(); assert(dc_bitmap_construct(&bitmap, 513, 257));
        fail_texture = i; dc_bitmap_draw(&bitmap, 0, 0, 513, 257, 0x12345678);
        assert(bitmap.changed && !bitmap.texture_count && !draws && !live && !outstanding);
        clear_counts(); dc_bitmap_upload(&bitmap); assert(uploads == 6);
        dc_bitmap_destruct(&bitmap); assert(!live && !outstanding);
    }
    clear_counts(); assert(dc_bitmap_construct(&bitmap, 2, 2));
    bitmap.bits[0] = 0x12345678; dc_bitmap_upload(&bitmap);
    bits = bitmap.bits; dib = bitmap.dib; before = bitmap.bits[0];
    fail_dib = 1; dc_bitmap_alloc(&bitmap, 4, 4);
    assert(bitmap.bits == bits && bitmap.dib == dib && bitmap.bits[0] == before && bitmap.width == 2 && bitmap.texture_count == 1);
    fail_dib = 0; dc_bitmap_alloc(&bitmap, 4, 4);
    assert(bitmap.width == 4 && bitmap.height == 4 && bitmap.changed && !bitmap.texture_count && !live);
    {
        struct DcBitmap empty;
        memset(&empty, 0, sizeof empty);
        empty.width = 3; empty.height = 3;
        memset(bitmap.bits, 0x5a, 4 * 4 * 4); bitmap.changed = 0;
        dc_bitmap_copy_bitmap(&bitmap, 1, 1, 0, 0, NULL);
        dc_bitmap_copy_bitmap(&bitmap, 1, 1, 2, 2, &empty);
        dc_bitmap_copy_rect(&bitmap, 0, 0, 1, 1, 2, 2, NULL);
        dc_bitmap_copy_rect(&bitmap, 0, 0, 1, 1, 2, 2, &empty);
        clear_counts(); dc_bitmap_draw(NULL, 0, 0, 2, 2, 0x12345678);
        assert(!uploads && !draws);
        assert(!bitmap.changed && bitmap.bits[0] == 0x5a5a5a5a && bitmap.bits[5] == 0x5a5a5a5a);
    }
    {
        DWORD source[4] = {1, 2, 3, 4};
        memset(bitmap.bits, 0, 4 * 4 * 4); bitmap.changed = 0;
        fail_at = allocation_calls + 1;
        dc_bitmap_copy_scaled(&bitmap, 0, 0, 2, 2, 2, 2, source);
        assert(!bitmap.changed && !bitmap.bits[0] && !outstanding);
        fail_at = 0;
        dc_bitmap_copy_scaled(&bitmap, 0, 0, 2, 2, 2, 2, source);
        assert(bitmap.changed && bitmap.bits[0] == 1 && bitmap.bits[4] == 3 && bitmap.bits[1] == 0);
        assert(!outstanding);
    }
    bitmap.bits[0] = 0x00ffffff; bitmap.bits[1] = 0x00000000; bitmap.bits[2] = 0x34ffffff;
    dc_bitmap_alpha_from_brightness(&bitmap, 0xff);
    assert(bitmap.bits[0] == 0xffffffff && bitmap.bits[1] == 0 && bitmap.bits[2] == 0xffffffff);
    bitmap.bits[0] = 0x00404040; dc_bitmap_alpha_from_brightness(&bitmap, 0x80);
    assert(bitmap.bits[0] == 0x7f404040);
    dc_bitmap_destruct(&bitmap);
    assert(!dc_bitmap_construct(&bitmap, -1, 2) && !bitmap.hdc);
    assert(!dc_bitmap_construct(&bitmap, INT_MAX, INT_MAX) && !bitmap.hdc);
    fail_dib = 1; assert(!dc_bitmap_construct(&bitmap, 2, 2) && !bitmap.hdc); fail_dib = 0;
    assert(!live && !outstanding);
    puts("Canvas owner: C tile arrays, every allocation failure, retry, cropped rows, GDI replacement, copies from a missing or pixel-less canvas and repeated cleanup passed.");
    return 0;
}
