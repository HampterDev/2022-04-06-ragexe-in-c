#include "render/device.h"
#include "render/device_palette.h"
#include "render/render_api.h"
#include "diag/diag.h"
#include <string.h>
#include <stdlib.h>
#include "platform/code_page.h"
#include "core/drive_state.h"
#include "render/render_options.h"
#include "platform/window.h"
#include "platform/clock.h"
#include "diag/perf.h"

const char g_msgBgr565NotSupported[] = "BGR 565 MODE. NOT SUPPORTED";
const char g_msgModeNotSupported[] = "NOT SUPPORTED MODE";

float g_texelOffsetU = 0.5f;
float g_texelOffsetV = 0.5f;

/* Localized family names: 굴림, ＭＳ Ｐゴシック, 宋体 and 細明體. */
#define FACE_GULIM_KR   L"\xad74\xb9bc"
#define FACE_MS_PGOTHIC L"\xff2d\xff33 \xff30\x30b4\x30b7\x30c3\x30af"
#define FACE_SIMSUN_CN  L"\x5b8b\x4f53"
#define FACE_MINGLIU_TW L"\x7d30\x660e\x9ad4"
#define HEX_FLAGS_0_9   ['0'] = 1, ['1'] = 1, ['2'] = 1, ['3'] = 1, ['4'] = 1, \
                        ['5'] = 1, ['6'] = 1, ['7'] = 1, ['8'] = 1, ['9'] = 1
#define HEX_VALUES_0_9  ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4, ['5'] = 5, \
                        ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9
struct C3dDevice g_3dDevice = {
    .rBits = 8, .gBits = 8, .bBits = 8, .aBits = 8,
    .colorKeyPixel = 0x421,
    .faceNamesB = { L"Gulim", L"Arial", L"MS Gothic", L"SimSun", L"MingLiU", L"Tahoma",
                    L"Arial", L"Verdana", L"Verdana", L"Arial", L"Arial", L"Tahoma" },
    .faceNamesA = { FACE_GULIM_KR, L"Arial", FACE_MS_PGOTHIC, FACE_SIMSUN_CN, FACE_MINGLIU_TW, L"Tahoma",
                    L"Arial", L"Verdana", L"Verdana", L"Arial", L"Arial", L"Tahoma" },
    .faceCharsets = { CHARSET_HANGUL, CHARSET_ANSI, CHARSET_SHIFTJIS, CHARSET_GB2312, CHARSET_CHINESEBIG5,
                      CHARSET_THAI, CHARSET_VIETNAMESE, CHARSET_ANSI, CHARSET_ANSI, CHARSET_RUSSIAN,
                      CHARSET_VIETNAMESE + 1, CHARSET_ARABIC },
    .hexDigitFlags = { HEX_FLAGS_0_9, ['A'] = 1, ['B'] = 1, ['C'] = 1, ['D'] = 1, ['E'] = 1, ['F'] = 1,
                       ['a'] = 1, ['b'] = 1, ['c'] = 1, ['d'] = 1, ['e'] = 1, ['f'] = 1 },
    .hexDigitValues = { HEX_VALUES_0_9, ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15 },
};

void c3d_device_clamp_texture_size(struct C3dDevice *self, unsigned *pw, unsigned *ph)
{
    unsigned pw2 = 1, ph2 = 1, w, h, aspect;
    while (pw2 < *pw)
        pw2 += pw2;
    w = pw2 > self->minTexW ? pw2 : self->minTexW;
    if (w > self->maxTexW)
        w = self->maxTexW;
    while (ph2 < *ph)
        ph2 += ph2;
    h = ph2 > self->minTexH ? ph2 : self->minTexH;
    if (h > self->maxTexH)
        h = self->maxTexH;
    aspect = self->texAspect;
    if (aspect != 0) {
        if (w / h > aspect)
            h = w / aspect;
        if (h / w > aspect) {
            *pw = h / aspect;
            *ph = h;
            return;
        }
    }
    *pw = w;
    *ph = h;
}

void c3d_device_save_frame(struct C3dDevice *self)
{
    if (self->save && self->target) {
        if (gfx_blit(self->save, NULL, self->target, NULL, GFX_BLT_ASYNC) < 0)
            gfx_blit(self->save, NULL, self->target, NULL, GFX_BLT_WAIT);
    }
}

int c3d_device_clear(struct C3dDevice *self, DWORD colour)
{
    RECT r;
    r.left = 0;
    r.top = 0;
    r.right = self->width;
    r.bottom = self->height;
    return gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, colour, 1.0f, &r);
}

int c3d_device_clear_z(struct C3dDevice *self)
{
    RECT r;
    r.left = 0;
    r.top = 0;
    r.right = self->width;
    r.bottom = self->height;
    return gfx_clear(GFX_CLEAR_ZBUFFER, 0, 1.0f, &r);
}

static void mask_shift(DWORD mask, DWORD *shift, DWORD *bits)
{
    if (!mask)
        return;
    for (;;) {
        DWORD before = mask;
        if (mask & 1)
            break;
        (*shift)++;
        mask >>= 1;
        if (before < 2)
            return;
    }
    if (mask) {
        DWORD n = 8;
        do {
            n--;
            mask >>= 1;
        } while (mask);
        *bits = n;
    }
}

/* custom: the screen surfaces at the window's client size in pixels, windowed or fullscreen
   (render/display_options.h); the display's mode never changes. */
static int create_screen(struct C3dDevice *self, int flags)
{
    int hr;
    platform_client_rect(self->hwnd, &self->rcWindow);
    self->width = self->rcWindow.right;
    self->height = self->rcWindow.bottom;
    hr = gfx_create_screen(self, flags);
    if (self->primary) {
        self->rShift = 0; self->rBits = 8;
        self->gShift = 0; self->gBits = 8;
        self->bShift = 0; self->bBits = 8;
        self->aShift = 0; self->aBits = 8;
        mask_shift(self->rMask, &self->rShift, &self->rBits);
        mask_shift(self->gMask, &self->gShift, &self->gBits);
        mask_shift(self->bMask, &self->bShift, &self->bBits);
        mask_shift(self->aMask, &self->aShift, &self->aBits);
    }
    return hr;
}

static int create_environment(struct C3dDevice *self, int flags)
{
    int hr;
    hr = gfx_open(self, flags);
    if (hr < 0)
        return hr;
    hr = create_screen(self, flags);
    if (hr < 0)
        return hr;
    hr = gfx_create_3d(self, flags);
    if (hr < 0)
        return hr;
    gfx_get_caps(self, &g_gfxCaps);
    if (g_gfxCaps.zbufferless)
        return 0;
    return gfx_create_zbuffer(self);
}

/* The window is in its display mode already (core/app_sdl.c, app_create_window). */
int c3d_device_init(struct C3dDevice *self, PlatformWindow hwnd, int flags)
{
    int hr;
    self->hwnd = hwnd;
    if (!hwnd)
        return GFX_E_INVALIDARG;
    hr = create_environment(self, flags);
    if (hr < 0)
        gfx_close(self);
    return hr;
}

void c3d_device_enable_mipmap(struct C3dDevice *self)
{
    (void)self;
    gfx_set_mipmaps(g_trilinearFilter && g_gfxCaps.trilinear);
}

struct Renderer *c3d_device_create_renderer(struct C3dDevice *self)
{
    struct Renderer *r;

    if (self->rMask != 0xf800 && self->rMask != 0x7c00) {
        if (self->rMask == 0x1f)
            error_msg(g_msgBgr565NotSupported, 0);
        else if (self->bpp != 32)
            error_msg(g_msgModeNotSupported, 0);
    }

    if (gfx_get_caps(self, &g_gfxCaps) == 0) {
        self->minTexW = g_gfxCaps.minTexW;
        self->minTexH = g_gfxCaps.minTexH;
        self->maxTexW = g_gfxCaps.maxTexW;
        self->maxTexH = g_gfxCaps.maxTexH;
        self->texAspect = g_gfxCaps.texAspect;
        gfx_set_mipmaps(g_trilinearFilter && g_gfxCaps.trilinear);
    }
    self->bltStretch = g_gfxCaps.bltStretch;
    self->tex1024 = (self->maxTexW >= 0x400 && self->maxTexH >= 0x400) ? 1 : 0;

    r = calloc(1, sizeof *r);
    if (!r) return NULL;
    if (!renderer_construct(r)) {
        free(r);
        return NULL;
    }
    renderer_set_projection(r, self->width, self->height);
    if (r->clipLeft == 0.0f) {
        r->clipLeft = -40.0f;
        r->clipTop = -30.0f;
        r->clipRight = (float)(r->width + 640);
        r->clipBottom = (float)(r->height + 480);
    } else {
        r->clipLeft = g_gfxCaps.guardLeft;
        r->clipRight = g_gfxCaps.guardRight;
        r->clipTop = g_gfxCaps.guardTop;
        r->clipBottom = g_gfxCaps.guardBottom;
    }
    c3d_device_probe_texel_offset(self);
    return r;
}

void c3d_device_delete_renderer(struct C3dDevice *self, struct Renderer *r)
{
    if (r) {
        renderer_destruct(r);
        free(r);
    }
}

int c3d_device_release_all(struct C3dDevice *self)
{
    return gfx_close(self);
}

void c3d_device_restore_frame(struct C3dDevice *self)
{
    if (self->target && self->save) {
        if (gfx_blit(self->target, NULL, self->save, NULL, GFX_BLT_ASYNC) < 0)
            gfx_blit(self->target, NULL, self->save, NULL, GFX_BLT_WAIT);
    }
}

/* custom: the frame's presentation is timed for the counters (diag/perf.h) before the frame counts
   as shown */
int c3d_device_show_frame(struct C3dDevice *self, char vsync)
{
    int64_t start = platform_stopwatch_ns();
    int hr;
    if (!self->primary)
        hr = GFX_E_NO_PRIMARY;
    else
        hr = gfx_present(self, vsync != 0);
    g_perf.presentNs += platform_stopwatch_ns() - start;
    drive_state_frame_shown();
    return hr;
}

static const DWORD probe_pattern[16] = {
    0, 0, 0xffffffff, 0xffffffff, 0, 0, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0, 0, 0xffffffff, 0xffffffff, 0, 0,
};

static int probe_codes(struct C3dDevice *self, struct Surface *surf, float texW, float texH, unsigned *best)
{
    const DWORD *pattern = probe_pattern;
    struct Vertex { float x, y; DWORD z, rhw, diffuse; float u, v; } v[4];
    struct gfx_lock lk;
    unsigned cols, rows, n, base, i, k;
    int bestScore = -0x400;

    cols = (unsigned)self->width >> 2;
    rows = (unsigned)self->height >> 2;
    base = 0;
    do {
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0, 1.0f, NULL);
        if (gfx_begin_scene() < 0)
            return 0;
        n = rows * cols;
        for (i = 0; i < n; i++) {
            unsigned code = base + i;
            float x, y, offx, offy, u0, u1, v0, v1;
            if ((int)code > 0xffff)
                break;
            y = (float)(int)((i / cols) * 4);
            x = (float)(int)((i % cols) * 4);
            offx = (float)(int)(code % 256) * (1.0f / 256.0f);
            offy = (float)(int)(code / 256) * (1.0f / 256.0f);
            u0 = (offx + 0.0f) / texW;
            u1 = (offx + 4.0f) / texW;
            v0 = (offy + 0.0f) / texH;
            v1 = (offy + 4.0f) / texH;
            for (k = 0; k < 4; k++) {
                v[k].z = 0x3727c5ac;
                v[k].rhw = 0x3f7fff58;
                v[k].diffuse = 0xffffffff;
            }
            v[0].x = x;        v[0].y = y;        v[0].u = u0; v[0].v = v0;
            v[1].x = x;        v[1].y = y + 4.0f; v[1].u = u0; v[1].v = v1;
            v[2].x = x + 4.0f; v[2].y = y;        v[2].u = u1; v[2].v = v0;
            v[3].x = x + 4.0f; v[3].y = y + 4.0f; v[3].u = u1; v[3].v = v1;
            gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
        }
        gfx_end_scene();
        gfx_blit(surf->surface, NULL, self->target, NULL, GFX_BLT_WAIT);
        if (gfx_lock(surf->surface, NULL, GFX_LOCK_WAIT, &lk) < 0)
            return 0;
        {
            unsigned bpp = lk.bpp >> 3;
            for (i = 0; i < n; i++) {
                unsigned code = base + i;
                unsigned py, px, sumA = 0, sumB = 0;
                if ((int)code > 0xffff)
                    break;
                py = (i / cols) * 4;
                px = (i % cols) * 4;
                for (k = 0; k < 16; k++) {
                    unsigned x = (k & 3) + px, y = (k >> 2) + py, p, r, g, b;
                    const BYTE *at = (const BYTE *)lk.bits + y * lk.pitch + x * bpp;
                    p = (bpp == 2) ? *(const WORD *)at : *(const DWORD *)at;
                    p &= 0xffff;
                    r = ((p & self->rMask) >> self->rShift) << self->rBits;
                    g = ((p & self->gMask) >> self->gShift) << self->gBits;
                    b = ((p & self->bMask) >> self->bShift) << self->bBits;
                    if (pattern[k])
                        sumA += r + g + b;
                    else
                        sumB += r + g + b;
                }
                if ((int)sumA >= 0x1680 && (int)sumB <= 0x180) {
                    int score = (int)sumA - (int)sumB;
                    if (score > bestScore) {
                        bestScore = score;
                        *best = code;
                    }
                }
            }
        }
        gfx_unlock(surf->surface);
        base += n;
    } while (base < 0x10000);
    return 1;
}

int c3d_device_probe_texel_offset(struct C3dDevice *self)
{
    struct Surface surf;
    struct Texture tex;
    unsigned best = 0x8080;
    int ok;

    surface_construct_sys_mem(&surf, self->width, self->height);
    texture_init(&tex, 4, 4, 0);
    texture_upload_pixels(&tex, 0, 0, 4, 4, probe_pattern, 0, 0);
    gfx_set_mode(DRAW_PROBE);
    gfx_set_texture(0, tex.base.surface);

    ok = probe_codes(self, &surf, (float)(unsigned)tex.base.width, (float)(unsigned)tex.base.height, &best);
    if (ok) {
        g_texelOffsetU = (float)(int)(best % 256) * (1.0f / 256.0f);
        g_texelOffsetV = (float)(int)(best / 256) * (1.0f / 256.0f);
    }
    surface_reset(&tex.base);
    surface_reset(&surf);
    return ok;
}
