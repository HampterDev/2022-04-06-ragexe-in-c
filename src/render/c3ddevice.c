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

/* Localized family names: 굴림, ＭＳ Ｐゴシック, 宋体 and 細明體. */
#define FACE_GULIM_KR   L"\xad74\xb9bc"
#define FACE_MS_PGOTHIC L"\xff2d\xff33 \xff30\x30b4\x30b7\x30c3\x30af"
#define FACE_SIMSUN_CN  L"\x5b8b\x4f53"
#define FACE_MINGLIU_TW L"\x7d30\x660e\x9ad4"
struct C3dDevice g_3dDevice = {
    .rBits = 8, .gBits = 8, .bBits = 8,
    .colorKeyPixel = 0x421,
    .faceNamesB = { L"Gulim", L"Arial", L"MS Gothic", L"SimSun", L"MingLiU", L"Tahoma",
                    L"Arial", L"Verdana", L"Verdana", L"Arial", L"Arial", L"Tahoma" },
    .faceNamesA = { FACE_GULIM_KR, L"Arial", FACE_MS_PGOTHIC, FACE_SIMSUN_CN, FACE_MINGLIU_TW, L"Tahoma",
                    L"Arial", L"Verdana", L"Verdana", L"Arial", L"Arial", L"Tahoma" },
    .faceCharsets = { CHARSET_HANGUL, CHARSET_ANSI, CHARSET_SHIFTJIS, CHARSET_GB2312, CHARSET_CHINESEBIG5,
                      CHARSET_THAI, CHARSET_VIETNAMESE, CHARSET_ANSI, CHARSET_ANSI, CHARSET_RUSSIAN,
                      CHARSET_VIETNAMESE + 1, CHARSET_ARABIC },
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

/* custom: the frame is kept in a render target of its size, made when first needed and again when
   the size changes, where Direct3D 9 had one made with the screen; the copy stays on the GPU. */
void c3d_device_save_frame(struct C3dDevice *self)
{
    int w = 0, h = 0;
    if (!self->target)
        return;
    if (self->save && (gfx_get_size(self->save, &w, &h) < 0 || w != self->width || h != self->height)) {
        gfx_release(self->save);
        self->save = NULL;
    }
    if (!self->save)
        self->save = gfx_create_render_target(self->width, self->height);
    if (self->save)
        gfx_blit(self->save, NULL, self->target, NULL);
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
static int create_screen(struct C3dDevice *self)
{
    RECT client;
    int hr;
    platform_client_rect(self->hwnd, &client);
    self->width = client.right;
    self->height = client.bottom;
    hr = gfx_create_screen(self);
    if (self->target) {
        self->rShift = 0; self->rBits = 8;
        self->gShift = 0; self->gBits = 8;
        self->bShift = 0; self->bBits = 8;
        mask_shift(self->rMask, &self->rShift, &self->rBits);
        mask_shift(self->gMask, &self->gShift, &self->gBits);
        mask_shift(self->bMask, &self->bShift, &self->bBits);
    }
    return hr;
}

static int create_environment(struct C3dDevice *self)
{
    int hr;
    hr = gfx_open(self);
    if (hr < 0)
        return hr;
    hr = create_screen(self);
    if (hr < 0)
        return hr;
    gfx_get_caps(&g_gfxCaps);
    return gfx_create_zbuffer(self);
}

/* The window is in its display mode already (core/app_sdl.c, app_create_window). */
int c3d_device_init(struct C3dDevice *self, PlatformWindow hwnd)
{
    int hr;
    self->hwnd = hwnd;
    if (!hwnd)
        return GFX_E_INVALIDARG;
    hr = create_environment(self);
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

    if (gfx_get_caps(&g_gfxCaps) == 0) {
        self->minTexW = g_gfxCaps.minTexW;
        self->minTexH = g_gfxCaps.minTexH;
        self->maxTexW = g_gfxCaps.maxTexW;
        self->maxTexH = g_gfxCaps.maxTexH;
        self->texAspect = g_gfxCaps.texAspect;
        gfx_set_mipmaps(g_trilinearFilter && g_gfxCaps.trilinear);
    }

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
    if (self->target && self->save)
        gfx_blit(self->target, NULL, self->save, NULL);
}

/* custom: the frame's presentation is timed for the counters (diag/perf.h) before the frame counts
   as shown */
int c3d_device_show_frame(struct C3dDevice *self, char vsync)
{
    int64_t start = platform_stopwatch_ns();
    int hr;
    if (!self->target)
        hr = GFX_E_NO_PRIMARY;
    else
        hr = gfx_present(vsync != 0);
    g_perf.presentNs += platform_stopwatch_ns() - start;
    drive_state_frame_shown();
    return hr;
}
