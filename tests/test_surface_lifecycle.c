#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "owned_storage_test_allocator.h"

static void *checked_calloc(size_t count, size_t size)
{
    void *result;
    assert(!size || count <= SIZE_MAX / size);
    result = checked_malloc(count * size);
    if (result) memset(result, 0, count * size);
    return result;
}
#define calloc checked_calloc
#define free checked_free
#include "../src/render/csurface.c"
#undef free
#undef calloc
#include "../src/render/tex_cache.c"

struct gfx_surface {
    int width, height, format, live;
    unsigned version;
    DWORD pixels[64 * 64];
};
struct C3dDevice g_3dDevice;
unsigned g_resPurgeAgeMs = 15000;
struct gfx_caps g_gfxCaps;
struct perf_counters g_perf;
static unsigned handles, releases, copies, stretches;
static int fail_create, fail_lock, fail_unlock, fail_mips, fail_copy, fail_size;
static unsigned locks, mip_refreshes, tick;
static struct Surface *resetting;
static struct Texture *drawn_texture;

gfx_surface *gfx_create_texture(int w, int h, int format)
{
    gfx_surface *surface;
    if (fail_create) return NULL;
    assert(w > 0 && h > 0 && w <= 64 && h <= 64);
    surface = checked_calloc(1, sizeof *surface);
    if (!surface) return NULL;
    surface->width = w; surface->height = h; surface->format = format; surface->live = 1;
    surface->version = ++tick;
    ++handles;
    return surface;
}
gfx_surface *gfx_create_surface(int w, int h) { return gfx_create_texture(w, h, GFX_FMT_8888); }
void gfx_release(gfx_surface *surface)
{
    assert(surface && surface->live && handles);
    if (resetting) assert(!resetting->surface);
    surface->live = 0; --handles; ++releases; checked_free(surface);
}
int gfx_texture_format_ok(int format) { (void)format; return 1; }
int gfx_get_size(gfx_surface *surface, int *w, int *h)
{ assert(surface && surface->live); if (fail_size) return -1; *w = surface->width; *h = surface->height; return 0; }
int gfx_lock(gfx_surface *surface, const RECT *rect, int flags, struct gfx_lock *out)
{
    unsigned bytes;
    (void)flags;
    ++locks;
    if (fail_lock) return -1;
    assert(surface && surface->live);
    bytes = surface->format == GFX_FMT_8888 ? 4 : 2;
    out->bits = (BYTE *)surface->pixels + (rect ? rect->top * surface->width + rect->left : 0) * bytes;
    out->pitch = surface->width * bytes; out->bpp = bytes * 8;
    out->width = rect ? rect->right - rect->left : surface->width;
    out->height = rect ? rect->bottom - rect->top : surface->height;
    return 0;
}
int gfx_unlock(gfx_surface *surface)
{ assert(surface && surface->live); surface->version = ++tick; return fail_unlock ? -1 : 0; }
int gfx_blit(gfx_surface *dest, const RECT *dr, gfx_surface *source, const RECT *sr, int flags)
{
    assert(dest && source && dest->live && source->live); (void)dr; (void)sr; (void)flags;
    if (fail_copy) return -1;
    dest->pixels[0] = source->pixels[0]; ++copies; return 0;
}
int gfx_refresh_mips(gfx_surface *surface, const RECT *rect)
{ assert(surface && rect); ++mip_refreshes; return fail_mips ? -1 : 0; }
unsigned gfx_content_version(gfx_surface *surface) { return surface->version; }
void gfx_evict_managed_textures(struct C3dDevice *device) { (void)device; abort(); }
void c3d_device_clamp_texture_size(struct C3dDevice *device, unsigned *w, unsigned *h)
{ assert(device == &g_3dDevice); if (*w > 16) *w = 16; if (*h > 16) *h = 16; }
void error_msg(const char *message, int flags) { (void)message; (void)flags; assert(0); }
static int texture_lock_depth, cache_write_depth;
void *tex_lock_enter(int operation) { (void)operation; ++texture_lock_depth; return &texture_lock_depth; }
void tex_lock_leave(void *section)
{ assert(section == &texture_lock_depth && texture_lock_depth > 0); --texture_lock_depth; }
void cache_lock_write_begin(void) { ++cache_write_depth; }
void cache_lock_write_end(void) { assert(cache_write_depth > 0); --cache_write_depth; }
void texture_draw_surface_stretch(struct Texture *texture, int x, int y, int w, int h)
{ assert(texture && x == 2 && y == 3 && w == 4 && h == 5); drawn_texture = texture; ++stretches; }

static void test_texture(void)
{
    struct Texture *texture = texture_create(4, 4, GFX_FMT_8888);
    DWORD pixels[16]; unsigned i;
    assert(texture && texture->base.ops == &texture_surface_ops && handles == 1 && outstanding == 2);
    for (i = 0; i < 16; ++i) pixels[i] = 0xff010203u + i;
    assert(texture_upload(texture, 0, 0, 4, 4, pixels, 0, 0));
    assert(!memcmp(texture->base.surface->pixels, pixels, sizeof pixels));
    surface_stretch(&texture->base, 2, 3, 4, 5); assert(stretches == 1 && drawn_texture == texture);
    resetting = &texture->base; surface_reset(&texture->base); surface_reset(&texture->base); resetting = NULL;
    assert(!handles && outstanding == 1 && texture->base.ops == &texture_surface_ops);
    texture_destroy(texture); assert(!outstanding);
}
static void test_surface(void)
{
    struct Surface plain, *wallpaper;
    DWORD pixels[16] = {0xff112233};
    assert(surface_construct_sys_mem(&plain, 4, 4) == &plain && plain.ops == &surface_ops);
    assert(surface_upload(&plain, 0, 0, 4, 4, pixels, 0, 0));
    assert(plain.surface->pixels[0] == pixels[0]);
    surface_reset(&plain); surface_reset(&plain); assert(!outstanding && !handles);
    wallpaper = c3d_device_new_wallpaper_surface(&g_3dDevice, 32, 32);
    assert(wallpaper && wallpaper->ops == &wallpaper_surface_ops);
    assert(surface_upload(wallpaper, 0, 0, 4, 4, pixels, 0, 0));
    assert(wallpaper->surface->pixels[0] == pixels[0] && copies == 1 && handles == 1);
    fail_create = 1; assert(!surface_upload(wallpaper, 0, 0, 4, 4, pixels, 0, 0)); fail_create = 0;
    assert(wallpaper->surface->pixels[0] == pixels[0] && copies == 1 && handles == 1);
    surface_destroy(wallpaper); assert(!handles && !outstanding);
    wallpaper = c3d_device_new_wallpaper_surface(&g_3dDevice, 4, 4);
    assert(wallpaper && wallpaper->ops == &texture_surface_ops);
    surface_destroy(wallpaper); assert(!handles && !outstanding);
}
static void test_failures(void)
{
    unsigned point;
    assert(!texture_create(0, 1, 0) && !texture_create(1, -1, 0));
    for (point = 1; point <= 2; ++point) {
        fail_at = allocation_calls + point; assert(!texture_create(4, 4, GFX_FMT_8888));
        assert(!outstanding && !handles);
        fail_at = allocation_calls + point; assert(!c3d_device_new_wallpaper_surface(&g_3dDevice, 32, 32));
        assert(!outstanding && !handles);
    }
    fail_at = 0; fail_create = 1;
    assert(!texture_create(4, 4, GFX_FMT_8888));
    assert(!c3d_device_new_wallpaper_surface(&g_3dDevice, 32, 32));
    fail_create = 0; fail_lock = 1;
    assert(!texture_create(4, 4, GFX_FMT_BUMP));
    fail_lock = 0; assert(!outstanding && !handles);
    surface_destroy(NULL); texture_destroy(NULL);
}
static void test_replacement(void)
{
    struct Surface surface, saved_surface;
    struct Texture texture, saved_texture;
    surface_construct_sys_mem(&surface, 4, 4);
    surface.surface->pixels[0] = 0xff123456;
    saved_surface = surface;
    fail_create = 1;
    assert(!surface_recreate(&surface, 9, 6));
    fail_create = 0;
    assert(!surface_recreate(&surface, INT_MAX, 4));
    assert(!surface_recreate(&surface, -1, 4));
    assert(!memcmp(&surface, &saved_surface, sizeof surface));
    assert(surface.surface->pixels[0] == 0xff123456);
    resetting = &surface;
    assert(surface_recreate(&surface, 9, 6));
    resetting = NULL;
    assert(surface.width == 12 && surface.height == 8 && handles == 1);
    surface_reset(&surface);

    memset(&texture, 0xa5, sizeof texture);
    texture_init(&texture, 4, 4, GFX_FMT_8888);
    assert(!texture.colorKeyed && !texture.fullSize && !texture.name[0]);
    assert(texture.reqWidth == 4 && texture.reqHeight == 4);
    texture.lockCount = 2; texture.fullSize = 1;
    texture.base.surface->pixels[0] = 0xffabcdef;
    saved_texture = texture;
    fail_create = 1;
    assert(!texture_recreate(&texture, 8, 8, GFX_FMT_1555));
    fail_create = 0;
    assert(!texture_recreate(&texture, 0, 8, GFX_FMT_1555));
    fail_lock = 1;
    assert(!texture_recreate(&texture, 8, 8, GFX_FMT_BUMP));
    fail_lock = 0;
    fail_unlock = 1;
    assert(!texture_recreate(&texture, 8, 8, GFX_FMT_BUMP));
    fail_unlock = 0;
    assert(!memcmp(&texture, &saved_texture, sizeof texture));
    assert(texture.base.surface->pixels[0] == 0xffabcdef && handles == 1);
    resetting = &texture.base;
    assert(texture_recreate(&texture, 30, 8, GFX_FMT_1555));
    resetting = NULL;
    assert(texture.base.width == 16 && texture.reqWidth == 15 && texture.reqHeight == 8);
    assert(texture.format == GFX_FMT_1555 && texture.lockCount == 2 && texture.fullSize == 1);
    assert(texture_recreate(&texture, 4, 4, GFX_FMT_BUMP));
    assert(texture.format == GFX_FMT_BUMP && !strcmp(texture.name, "WATERBUMPMAP"));
    assert(texture.reqWidth == 4 && texture.reqHeight == 4 && handles == 1);
    surface_reset(&texture.base);
    fail_create = 1;
    memset(&texture, 0xa5, sizeof texture);
    texture_init(&texture, 4, 4, GFX_FMT_8888);
    fail_create = 0;
    assert(!texture.base.surface && !texture.base.width && !texture.reqWidth && !texture.name[0]);
    surface_reset(&texture.base);
    assert(!handles && !outstanding);
}
static void test_pixels(void)
{
    const DWORD source[] = {0xff123456, 0xffff00ff, 0xff000000, 0x80102030};
    const int formats[] = {GFX_FMT_1555, GFX_FMT_4444, GFX_FMT_565};
    const WORD expected[][4] = {
        {0x88ca, 0, 0, 0x8886}, {0xf135, 0, 0, 0x8123}, {0x11aa, 0, 0, 0x1106},
    };
    unsigned i, x, y;
    DWORD wide[32 * 32];
    struct Texture *texture;
    for (i = 0; i < 3; ++i) {
        texture = texture_create(4, 1, formats[i]);
        assert(texture_upload(texture, 0, 0, 4, 1, source, 1, 0));
        assert(!memcmp(texture->base.surface->pixels, expected[i], sizeof expected[i]));
        texture_destroy(texture);
    }
    for (i = 0; i < 32 * 32; ++i) wide[i] = 0xff000000 | i;
    texture = texture_create(32, 32, GFX_FMT_8888);
    assert(texture_upload(texture, 0, 0, 32, 32, wide, 0, 0));
    assert(texture->reqWidth == 16 && texture->reqHeight == 16);
    for (y = 0; y < 16; ++y)
        for (x = 0; x < 16; ++x)
            assert(texture->base.surface->pixels[y * 16 + x] == wide[y * 2 * 32 + x * 2]);
    texture_destroy(texture);
    assert(!handles && !outstanding);
}
static void test_upload_failure(void)
{
    DWORD pixels[16] = {0xff123456};
    struct Texture *texture = texture_create(4, 4, GFX_FMT_8888);
    struct Surface plain, *wallpaper;
    unsigned before;
    int *failures[] = {&fail_size, &fail_lock, &fail_unlock, &fail_mips};
    unsigned i;
    assert(!surface_upload(NULL, 0, 0, 4, 4, pixels, 0, 0));
    assert(!texture_upload(NULL, 0, 0, 4, 4, pixels, 0, 0));
    before = locks;
    assert(!texture_upload(texture, -1, 0, 4, 4, pixels, 0, 0));
    assert(!texture_upload(texture, 1, 0, 4, 4, pixels, 0, 0));
    assert(!texture_upload(texture, 0, INT_MAX, 4, 4, pixels, 0, 0));
    assert(!texture_upload(texture, 0, 0, 4, 4, pixels, 0, 3));
    assert(!texture_upload(texture, 0, 0, INT_MAX, 2, pixels, 0, 0));
    assert(!texture_upload(texture, 0, 0, 4, -1, pixels, 0, 0));
    assert(!texture_upload(texture, 0, 0, 0, 4, pixels, 0, 0));
    assert(!texture_upload(texture, 0, 0, 4, 4, NULL, 0, 0));
    assert(locks == before);
    for (i = 0; i < sizeof failures / sizeof *failures; ++i) {
        struct Texture saved = *texture;
        ++pixels[0]; *failures[i] = 1;
        assert(!texture_upload(texture, 0, 0, 2, 2, pixels, 1, 4));
        assert(!memcmp(&saved, texture, sizeof saved));
        *failures[i] = 0; before = locks;
        assert(texture_upload(texture, 0, 0, 2, 2, pixels, 1, 4));
        assert(locks == before + 1 && texture->reqWidth == 2 && texture->colorKeyed);
        assert(texture->base.surface->pixels[0] == pixels[0]);
        before = locks;
        assert(texture_upload(texture, 0, 0, 2, 2, pixels, 1, 4));
        assert(locks == before);
    }
    texture_destroy(texture);
    surface_construct_sys_mem(&plain, 4, 4);
    fail_lock = 1; assert(!surface_upload(&plain, 0, 0, 4, 4, pixels, 0, 0)); fail_lock = 0;
    fail_unlock = 1; assert(!surface_upload(&plain, 0, 0, 4, 4, pixels, 0, 0)); fail_unlock = 0;
    assert(surface_upload(&plain, 0, 0, 4, 4, pixels, 0, 0)); surface_reset(&plain);
    wallpaper = c3d_device_new_wallpaper_surface(&g_3dDevice, 32, 32);
    before = copies;
    fail_lock = 1; assert(!surface_upload(wallpaper, 0, 0, 4, 4, pixels, 0, 0)); fail_lock = 0;
    fail_unlock = 1; assert(!surface_upload(wallpaper, 0, 0, 4, 4, pixels, 0, 0)); fail_unlock = 0;
    fail_copy = 1; assert(!surface_upload(wallpaper, 0, 0, 4, 4, pixels, 0, 0)); fail_copy = 0;
    assert(copies == before && handles == 1);
    assert(surface_upload(wallpaper, 0, 0, 4, 4, pixels, 0, 0));
    surface_destroy(wallpaper); assert(!handles && !outstanding);
}

static void test_sprite_upload(void)
{
    BYTE encoded[] = {1, 0, 2, 2, 0, 0, 1};
    struct SprImg frame = {.width = 3, .height = 2, .pixels = encoded};
    DWORD palette[256] = {0, 0x1234, 0x5678};
    const WORD expected[] = {0x1234, 0, 0, 0, 0x5678, 0, 0x1234, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct Texture *texture = texture_create(4, 4, GFX_FMT_1555);
    unsigned before = locks;
    assert(!texture_draw_sprite_frame(texture, 0, 0, 2, 4, &frame, palette));
    assert(!texture_draw_sprite_frame(texture, 1, 0, 4, 4, &frame, palette));
    assert(locks == before);
    fail_lock = 1; assert(!texture_draw_sprite_frame(texture, 0, 0, 4, 4, &frame, palette)); fail_lock = 0;
    fail_unlock = 1; assert(!texture_draw_sprite_frame(texture, 0, 0, 4, 4, &frame, palette)); fail_unlock = 0;
    fail_mips = 1; assert(!texture_draw_sprite_frame(texture, 0, 0, 4, 4, &frame, palette)); fail_mips = 0;
    assert(texture_draw_sprite_frame(texture, 0, 0, 4, 4, &frame, palette));
    assert(!memcmp(texture->base.surface->pixels, expected, sizeof expected));
    texture_destroy(texture); assert(!handles && !outstanding);
}

static void test_cache_upload(void)
{
    DWORD pixels[16] = {0xff987654};
    struct Texture *texture, saved;
    int *failures[] = {&fail_create, &fail_lock, &fail_unlock, &fail_mips};
    unsigned i;
    texture = texture_from_pixels(4, 4, pixels, GFX_FMT_8888, 0, 1);
    assert(texture); strcpy(texture->name, "owned.bmp"); texture->lockCount = 3; texture->lastUse = 123;
    saved = *texture;
    for (i = 0; i < sizeof failures / sizeof *failures; ++i) {
        *failures[i] = 1;
        assert(!texture_from_pixels(4, 4, pixels, GFX_FMT_8888, 0, 1));
        assert(!black_texture(0));
        texture_reload(texture, 2, 2, pixels);
        assert(!memcmp(texture, &saved, sizeof saved) && handles == 1);
        assert(texture->base.surface->pixels[0] == pixels[0]);
        *failures[i] = 0;
    }
    texture_reload(texture, 2, 2, pixels);
    assert(texture->base.surface != saved.base.surface && texture->base.width == 2 && texture->reqWidth == 2);
    assert(!strcmp(texture->name, saved.name) && texture->lockCount == 3 && texture->lastUse == 123);
    assert(texture->fullSize == 1 && texture->format == GFX_FMT_8888 && handles == 1);
    texture_destroy(texture); assert(!handles && !outstanding);
}

int main(void)
{
    g_3dDevice.width = g_3dDevice.height = 8;
    g_3dDevice.maxTexW = g_3dDevice.maxTexH = 16;
    g_3dDevice.rBits = g_3dDevice.gBits = g_3dDevice.bBits = 3;
    g_3dDevice.rShift = 10; g_3dDevice.gShift = 5;
    g_gfxCaps.bumpEnv = g_gfxCaps.bumpEnvLuminance = 1;
    test_texture(); test_surface(); test_failures(); test_replacement(); test_pixels();
    test_upload_failure(); test_sprite_upload(); test_cache_upload();
    assert(releases && !handles && !outstanding);
    puts("Surface ownership: typed callbacks, four pixel formats, downsampling, wallpaper, transactional recreation, allocation/driver failures and repeated cleanup passed");
    return 0;
}

void *mem_alloc(size_t size) { (void)size; abort(); }
void mem_free(void *pointer)
{ (void)pointer;  abort(); }

struct ResMgr *g_resMgr(void) { abort(); }
void *res_mgr_get(struct ResMgr *self, const char *name) { abort(); }
void res_mgr_release(struct ResMgr *self, struct Res *res) { abort(); }
struct WorldVideoList *game_current_video_list(void) { abort(); }
struct WorldVideo *world_video_list_open(struct WorldVideoList *player, const char *name, int one) { abort(); }

int world_video_dimensions(struct WorldVideo *video, int *width, int *height)
{ (void)video; (void)width; (void)height; abort(); }
