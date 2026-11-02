#include "render/device.h"
#include "render/texture_cache.h"
#include "render/sprite_atlas.h"
#include "resources/sprite.h"
#include "render/render_api.h"
#include "diag/perf.h"
#include "render/cdc.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include "platform/clock.h"
#include "platform/window.h"

static const char unsupported_caption[] = "notsuppot";

#pragma comment(lib, "winmm.lib")

static WORD pixel16(const struct C3dDevice *dev, DWORD r, DWORD g, DWORD b)
{
    return (WORD)((((int)b >> dev->bBits) << dev->bShift) + (((int)g >> dev->gBits) << dev->gShift)
                  + (((int)r >> dev->rBits) << dev->rShift));
}

static int round4(int v)
{
    return v + ((-v) & 3);
}

static void whole(RECT *r, const struct Surface *s)
{
    r->left = 0;
    r->top = 0;
    r->right = s->width;
    r->bottom = s->height;
}

struct Surface *surface_construct_sys_mem(struct Surface *self, int width, int height)
{
    self->width = self->height = 0;
    self->surface = NULL;
    self->ops = &surface_ops;
    surface_recreate(self, width, height);
    return self;
}

struct Surface *surface_wallpaper_construct(struct Surface *self, int width, int height)
{
    surface_construct_sys_mem(self, width, height);
    self->ops = &wallpaper_surface_ops;
    return self;
}

void surface_reset(struct Surface *self)
{
    gfx_surface *handle = self->surface;
    self->surface = NULL;
    if (handle)
        gfx_release(handle);
}

int surface_recreate(struct Surface *self, int width, int height)
{
    gfx_surface *handle;
    if (width <= 0 || height <= 0 || width > INT_MAX - 3 || height > INT_MAX - 3)
        return 0;
    width = round4(width);
    height = round4(height);
    handle = gfx_create_surface(width, height);
    if (!handle)
        return 0;
    surface_reset(self);
    self->width = width;
    self->height = height;
    self->surface = handle;
    return 1;
}

void surface_stretch_to_target(struct Surface *self, int unused1, int unused2, int w, int h)
{
    RECT r;
    r.left = 0;
    r.top = 0;
    r.right = w;
    r.bottom = h;
    gfx_blit(g_3dDevice.target, &r, self->surface, NULL, GFX_BLT_WAIT);
    (void)unused1;
    (void)unused2;
}

static int upload_source_valid(const DWORD *bits, int w, int h, int pitch)
{
    return bits && w > 0 && h > 0 && pitch >= w
        && (size_t)w <= SIZE_MAX / sizeof *bits
        && (size_t)(h - 1) <= (SIZE_MAX / sizeof *bits - (size_t)w) / (size_t)pitch;
}

int surface_fill_pixels(struct Surface *self, int x, int y, int w, int h, const DWORD *bits,
                           int unused, int pitch)
{
    struct gfx_lock lk;
    RECT r;
    int row;
    (void)unused;
    if (pitch == 0)
        pitch = w;
    if (!upload_source_valid(bits, w, h, pitch) || !self || !self->surface
        || x < 0 || y < 0 || x > self->width || y > self->height
        || w > self->width - x || h > self->height - y)
        return 0;
    r.left = x;
    r.top = y;
    r.right = x + w;
    r.bottom = y + h;
    if (gfx_lock(self->surface, &r, GFX_LOCK_WAIT | GFX_LOCK_WRITEONLY, &lk) < 0)
        return 0;
    if (lk.bpp != 16 && lk.bpp != 32) {
        gfx_unlock(self->surface);
        return 0;
    }
    {
        WORD key = pixel16(&g_3dDevice, 0xff, 0, 0xff);
        if (lk.bpp == 16) {
            BYTE *dst = (BYTE *)lk.bits;
            for (row = h; row > 0; row--) {
                int i;
                for (i = 0; i < w; i++) {
                    DWORD p = bits[i];
                    WORD v = pixel16(&g_3dDevice, (p >> 16) & 0xff, (p >> 8) & 0xff, p & 0xff);
                    if (v != key)
                        ((WORD *)dst)[i] = v;
                }
                dst += lk.pitch;
                if (row > 1) bits += pitch;
            }
        } else {
            BYTE *dst = (BYTE *)lk.bits;
            for (row = h; row > 0; row--) {
                int i;
                for (i = 0; i < w; i++) {
                    DWORD p = bits[i];
                    if ((p & 0xffffff) != 0xff00ff)
                        ((DWORD *)dst)[i] = p;
                }
                dst += lk.pitch;
                if (row > 1) bits += pitch;
            }
        }
    }
    return gfx_unlock(self->surface) >= 0;
}

int surface_wallpaper_fill_pixels(struct Surface *self, int unused1, int unused2, int w, int h,
                           const DWORD *bits, int unused6, int pitch)
{
    struct Surface tmp;
    int uploaded;
    (void)unused1; (void)unused2; (void)unused6;
    if (!self || !self->surface || !upload_source_valid(bits, w, h, pitch ? pitch : w))
        return 0;
    surface_construct_sys_mem(&tmp, w, h);
    if (!tmp.surface)
        return 0;
    uploaded = surface_fill_pixels(&tmp, 0, 0, w, h, bits, 0, pitch)
        && gfx_blit(self->surface, NULL, tmp.surface, NULL, GFX_BLT_WAIT) >= 0;
    surface_reset(&tmp);
    return uploaded;
}

static void ensure_format(int fmt)
{
    if (fmt != 0 && fmt != 1 && fmt != 4 && fmt != 2)
        error_msg("Create Texture::Unsupported pixelformat", 0);
    if (!gfx_texture_format_ok(fmt))
        error_msg("GFC 를 구동할 수 없는 비디오시스템입니다", 0);
}

static void fit(int req, int have, int *out)
{
    unsigned v = (unsigned)req;
    while (v > (unsigned)have)
        v >>= 1;
    *out = (int)v;
}

struct Texture *texture_create(int width, int height, int format)
{
    struct Texture *texture;
    if (width <= 0 || height <= 0)
        return NULL;
    texture = calloc(1, sizeof *texture);
    if (!texture)
        return NULL;
    texture_init(texture, width, height, format);
    if (!texture->base.surface) {
        free(texture);
        return NULL;
    }
    return texture;
}

struct Texture *texture_init(struct Texture *self, int width, int height, int format)
{
    memset(self, 0, sizeof *self);
    self->base.ops = &texture_surface_ops;
    if (format == GFX_FMT_BUMP)
        texture_create_bump(self, width, height);
    else
        texture_recreate(self, width, height, format);
    return self;
}

int texture_copy_rect_from(struct Texture *self, gfx_surface *src, RECT *r)
{
    if (!self->base.surface)
        return 0;
    return gfx_blit(self->base.surface, r, src, r, GFX_BLT_WAIT) >= 0;
}

int texture_blit_rect(struct Texture *self, struct Surface *src, int sx, int sy, int sw, int sh,
                          int dx, int dy, int dw, int dh)
{
    RECT srcRect, dstRect, all;
    if (!self->base.surface)
        return 0;
    srcRect.left = sx;
    srcRect.top = sy;
    srcRect.right = sx + sw;
    srcRect.bottom = sy + sh;
    dstRect.left = dx;
    dstRect.top = dy;
    dstRect.right = dx + dw;
    dstRect.bottom = dy + dh;
    if (gfx_blit(self->base.surface, &dstRect, src->surface, &srcRect, GFX_BLT_WAIT) != 0)
        return 0;
    whole(&all, &self->base);
    return texture_update_mip_chain(self, &all);
}

int texture_recreate(struct Texture *self, int width, int height, int format)
{
    unsigned actual_width, actual_height;
    gfx_surface *handle;
    if (width <= 0 || height <= 0)
        return 0;
    if (format == GFX_FMT_BUMP)
        return texture_create_bump(self, width, height);
    actual_width = width;
    actual_height = height;
    c3d_device_clamp_texture_size(&g_3dDevice, &actual_width, &actual_height);
    if (!actual_width || !actual_height || actual_width > INT_MAX || actual_height > INT_MAX)
        return 0;
    ensure_format(format);
    handle = gfx_create_texture(actual_width, actual_height, format);
    if (!handle)
        return 0;
    surface_reset(&self->base);
    self->base.surface = handle;
    self->base.width = actual_width;
    self->base.height = actual_height;
    self->format = format;
    fit(width, self->base.width, &self->reqWidth);
    fit(height, self->base.height, &self->reqHeight);
    return 1;
}

static void bump_caps_check(void)
{
    if (!g_gfxCaps.bumpEnv)
        platform_message_box(NULL, "D3DTEXOPCAPS_BUMPENVMAP", unsupported_caption, 0);
    if (!g_gfxCaps.bumpEnvLuminance)
        platform_message_box(NULL, "D3DTEXOPCAPS_BUMPENVMAPLUMINANCE", unsupported_caption, 0);
}

int texture_create_bump(struct Texture *self, int width, int height)
{
    struct gfx_lock lk;
    gfx_surface *s;
    unsigned x, y, actual_width, actual_height;
    if (width <= 0 || height <= 0)
        return 0;
    actual_width = width;
    actual_height = height;
    c3d_device_clamp_texture_size(&g_3dDevice, &actual_width, &actual_height);
    if (!actual_width || !actual_height || actual_width > INT_MAX || actual_height > INT_MAX)
        return 0;
    bump_caps_check();
    s = gfx_create_texture(actual_width, actual_height, GFX_FMT_BUMP);
    if (!s)
        return 0;
    if (gfx_lock(s, NULL, GFX_LOCK_RETRY, &lk) < 0) {
        gfx_release(s);
        return 0;
    }
    for (y = 0; y < lk.height; y++) {
        BYTE *row = (BYTE *)lk.bits + y * lk.pitch;
        for (x = 0; x < lk.width; x++) {
            float fy = (float)y / (float)lk.height - 0.5f;
            float r = (float)sqrt((double)(fy * fy + 0.0f));
            float e = (float)exp((double)(r * -5.0f));
            float a = r * 300.0f;
            float c1 = (float)cos((double)a) * 32.0f * e;
            float s1 = (float)sin((double)a) * 32.0f * e;
            float b = (fy + 0.0f) * 300.0f * 0.5f;
            float c = (0.0f - fy) * 280.0f * 0.5f;
            BYTE du = (BYTE)(int)c1, dv = (BYTE)(int)s1;
            du = (BYTE)(du + (BYTE)(int)((float)cos((double)b) * 16.0f));
            dv = (BYTE)(dv + (BYTE)(int)((float)sin((double)b) * 16.0f));
            du = (BYTE)(du + (BYTE)(int)((float)cos((double)c) * 7.0f));
            dv = (BYTE)(dv + (BYTE)(int)((float)sin((double)c) * 7.0f));
            row[x * 2] = du;
            row[x * 2 + 1] = dv;
        }
    }
    if (gfx_unlock(s) < 0) {
        gfx_release(s);
        return 0;
    }
    surface_reset(&self->base);
    self->base.surface = s;
    self->base.width = actual_width;
    self->base.height = actual_height;
    self->format = GFX_FMT_BUMP;
    fit(width, self->base.width, &self->reqWidth);
    fit(height, self->base.height, &self->reqHeight);
    memcpy(self->name, "WATERBUMPMAP", sizeof "WATERBUMPMAP");
    return 1;
}

void texture_lock(struct Texture *self)
{
    self->lastUse = platform_ticks_ms();
    self->lockCount++;
}

void texture_unlock(struct Texture *self)
{
    self->lockCount--;
}

int texture_update_mip_chain(struct Texture *self, const RECT *r)
{
    return gfx_refresh_mips(self->base.surface, r) >= 0;
}

struct build_record {
    struct Texture *tex;
    gfx_surface *surface;
    unsigned version;
    DWORD hash[4];
    int x, y, w, h, pitch;
    BYTE colorKeyed;
};
#define BUILD_SLOTS 8192
#define BUILD_PROBES 8
static struct build_record build_records[BUILD_SLOTS];

static void build_hash(const DWORD *src, int rows, int cols, size_t stride, DWORD out[4])
{
    DWORD a = 2166136261u, b = 0x9e3779b9u, c = 0x85ebca6bu, d = 0xc2b2ae35u;
    int r, i;
    for (r = 0; r < rows; r++) {
        const DWORD *p = src + (size_t)r * stride;
        for (i = 0; i + 4 <= cols; i += 4) {
            a = (a ^ p[i]) * 16777619u;
            b = (b ^ p[i + 1]) * 16777619u;
            c = (c ^ p[i + 2]) * 16777619u;
            d = (d ^ p[i + 3]) * 16777619u;
        }
        for (; i < cols; i++)
            a = (a ^ p[i]) * 16777619u;
    }
    out[0] = a; out[1] = b; out[2] = c; out[3] = d;
}

static struct build_record *build_slot(struct Texture *tex)
{
    unsigned h = ((unsigned)(ULONG_PTR)tex >> 4) * 2654435761u, i;
    struct build_record *first_free = NULL;
    for (i = 0; i < BUILD_PROBES; i++) {
        struct build_record *e = &build_records[(h + i) & (BUILD_SLOTS - 1)];
        if (e->tex == tex)
            return e;
        if (!e->tex && !first_free)
            first_free = e;
    }
    return first_free;
}

int texture_upload_pixels(struct Texture *self, int x, int y, int w, int h, const DWORD *bits, int colorKeyed, int pitch)
{
    struct gfx_lock lk;
    RECT r, all;
    int sx = 1, sy = 1, x0, y0, w0, h0, row, texW, texH, req_width, req_height;
    struct build_record *rec = NULL;
    unsigned version;
    DWORD hash[4];
    if (pitch == 0)
        pitch = w;
    if (!upload_source_valid(bits, w, h, pitch) || !self || !self->base.surface || x < 0 || y < 0)
        return 0;
    if (self->format != GFX_FMT_1555 && self->format != GFX_FMT_4444
        && self->format != GFX_FMT_565 && self->format != GFX_FMT_8888)
        return 0;
    if (gfx_get_size(self->base.surface, &texW, &texH) < 0)
        return 0;
    if (texW <= 0 || texH <= 0)
        return 0;
    sx = (int)((unsigned)w / ((unsigned)texW + 1)) + 1;
    sy = (int)((unsigned)h / ((unsigned)texH + 1)) + 1;
    x0 = x / sx;
    y0 = y / sy;
    w0 = w / sx;
    h0 = h / sy;
    if (!w0 || !h0 || x0 > texW || y0 > texH || w0 > texW - x0 || h0 > texH - y0)
        return 0;
    fit(w, self->base.width, &req_width);
    fit(h, self->base.height, &req_height);
    version = gfx_content_version(self->base.surface);
    if (version) {
        build_hash(bits, h0, w0 * sx, (size_t)pitch * sy, hash);
        rec = build_slot(self);
        if (rec && rec->tex == self && rec->surface == self->base.surface && rec->version == version
            && rec->x == x && rec->y == y && rec->w == w && rec->h == h && rec->pitch == pitch
            && rec->colorKeyed == (BYTE)colorKeyed && memcmp(rec->hash, hash, sizeof(hash)) == 0) {
            g_perf.buildsSkipped++;
            self->reqWidth = req_width; self->reqHeight = req_height;
            self->colorKeyed = (BYTE)colorKeyed;
            return 1;
        }
    }
    g_perf.builds++;
    r.left = x0;
    r.top = y0;
    r.right = x0 + w0;
    r.bottom = y0 + h0;
    if (gfx_lock(self->base.surface, &r, GFX_LOCK_WAIT | GFX_LOCK_WRITEONLY, &lk) < 0)
        return 0;
    {
        BYTE *dst = (BYTE *)lk.bits;
        const DWORD *src = bits;
        size_t srcStep = (size_t)pitch * sy;
        switch (self->format) {
        case GFX_FMT_1555:
            for (row = 0; row < h0; row++) {
                int i;
                for (i = 0; i < w0; i++) {
                    DWORD p = src[i * sx];
                    DWORD v = ((p >> 6) & 0x3e0) + ((p >> 3) & 0x1f) + ((p >> 16) & 0x8000) + ((p >> 9) & 0x7c00);
                    if (v == 0xfc1f)
                        v = 0;
                    else if (colorKeyed && (v & 0x7fff) == 0)
                        v = 0;
                    ((WORD *)dst)[i] = (WORD)v;
                }
                dst += lk.pitch;
                if (row + 1 < h0) src += srcStep;
            }
            break;
        case GFX_FMT_4444:
            for (row = 0; row < h0; row++) {
                int i;
                for (i = 0; i < w0; i++) {
                    DWORD p = src[i * sx];
                    DWORD v = ((p >> 16) & 0xf000) + ((p >> 12) & 0xf00) + ((p >> 8) & 0xf0) + ((p >> 4) & 0xf);
                    if ((v & 0xfff) == 0xf0f)
                        v = 0;
                    else if (colorKeyed && (v & 0xfff) == 0)
                        v = 0;
                    ((WORD *)dst)[i] = (WORD)v;
                }
                dst += lk.pitch;
                if (row + 1 < h0) src += srcStep;
            }
            break;
        case GFX_FMT_565:
            for (row = 0; row < h0; row++) {
                int i;
                for (i = 0; i < w0; i++) {
                    DWORD p = src[i * sx];
                    DWORD v = ((p >> 5) & 0x7e0) + ((p >> 3) & 0x1f) + ((p >> 8) & 0xf800);
                    if (!(colorKeyed && v == 0xf81f))
                        ((WORD *)dst)[i] = (WORD)v;
                }
                dst += lk.pitch;
                if (row + 1 < h0) src += srcStep;
            }
            break;
        case GFX_FMT_8888:
            for (row = 0; row < h0; row++) {
                int i;
                for (i = 0; i < w0; i++)
                    ((DWORD *)dst)[i] = src[i * sx];
                dst += lk.pitch;
                if (row + 1 < h0) src += srcStep;
            }
            break;
        default:
            break;
        }
    }
    if (gfx_unlock(self->base.surface) < 0)
        return 0;
    whole(&all, &self->base);
    if (!texture_update_mip_chain(self, &all))
        return 0;
    self->reqWidth = req_width; self->reqHeight = req_height;
    self->colorKeyed = (BYTE)colorKeyed;
    if (version && rec) {
        rec->tex = self;
        rec->surface = self->base.surface;
        rec->version = gfx_content_version(self->base.surface);
        memcpy(rec->hash, hash, sizeof(hash));
        rec->x = x; rec->y = y; rec->w = w; rec->h = h; rec->pitch = pitch;
        rec->colorKeyed = (BYTE)colorKeyed;
    }
    return 1;
}

int texture_draw_sprite_frame(struct Texture *self, int x, int y, int w, int h, const struct SprImg *spr,
                           const DWORD *palette)
{
    struct gfx_lock lk;
    RECT r;
    BYTE *dst;
    const BYTE *src;
    int row = 0, run = 0;
    if (!self || !self->base.surface || !spr || !spr->pixels || !palette
        || spr->width <= 0 || spr->height <= 0 || w < spr->width || h < spr->height
        || x < 0 || y < 0 || x > self->base.width || y > self->base.height
        || w > self->base.width - x || h > self->base.height - y)
        return 0;
    if (gfx_lock(self->base.surface, NULL, GFX_LOCK_WAIT, &lk) < 0)
        return 0;
    if (lk.bpp != 16) {
        gfx_unlock(self->base.surface);
        return 0;
    }
    dst = (BYTE *)lk.bits + y * lk.pitch + x * 2;
    src = spr->pixels;
    for (row = 0; row < spr->height; row++) {
        int col = 0;
        for (; col < spr->width; col++) {
            if (run) {
                ((WORD *)dst)[col] = 0;
                run--;
            } else {
                BYTE v = *src;
                if (v) {
                    ((WORD *)dst)[col] = (WORD)palette[v];
                    src++;
                } else {
                    src++;
                    ((WORD *)dst)[col] = 0;
                    run = *src ? *src - 1 : 0;
                    src++;
                }
            }
        }
        memset(dst + col * 2, 0, (w - col) * 2);
        dst += lk.pitch;
    }
    for (; row < h; row++) {
        memset(dst, 0, w * 2);
        dst += lk.pitch;
    }
    if (gfx_unlock(self->base.surface) < 0)
        return 0;
    r.left = x;
    r.top = y;
    r.right = x + w;
    r.bottom = y + h;
    return texture_update_mip_chain(self, &r);
}

struct Surface *c3d_device_new_wallpaper_surface(struct C3dDevice *self, unsigned w, unsigned h)
{
    struct Surface *s;
    if (w <= self->maxTexW && h <= self->maxTexH)
        return (struct Surface *)texture_create(w, h, 0);
    s = calloc(1, sizeof *s);
    if (!s)
        return NULL;
    surface_wallpaper_construct(s, self->width, self->height);
    if (!s->surface) {
        surface_destroy(s);
        return NULL;
    }
    return s;
}

void surface_destroy(struct Surface *surface)
{
    if (!surface)
        return;
    surface_reset(surface);
    free(surface);
}
static int texture_surface_upload(struct Surface *surface, int x, int y, int w, int h,
                                   const DWORD *bits, int keyed, int pitch)
{
    return texture_upload_pixels((struct Texture *)surface, x, y, w, h, bits, keyed, pitch);
}
static void texture_surface_stretch(struct Surface *surface, int x, int y, int w, int h)
{
    texture_draw_surface_stretch((struct Texture *)surface, x, y, w, h);
}
const struct SurfaceOps surface_ops = {
    .upload = surface_fill_pixels, .stretch = surface_stretch_to_target,
};
const struct SurfaceOps wallpaper_surface_ops = {
    .upload = surface_wallpaper_fill_pixels, .stretch = surface_stretch_to_target,
};
const struct SurfaceOps texture_surface_ops = {
    .upload = texture_surface_upload, .stretch = texture_surface_stretch,
};
