#include "render/device.h"
#include "render/device_palette.h"
#include "render/render_api.h"
#include "diag/diag.h"
#include <string.h>
#include <stdlib.h>
#include "core/app.h"
#include "ui/render/graphic_setting.h"
#include "platform/window.h"

const char g_msgBgr565NotSupported[] = "BGR 565 MODE. NOT SUPPORTED";
const char g_msgModeNotSupported[] = "NOT SUPPORTED MODE";

float g_texelOffsetU = 0.5f;
float g_texelOffsetV = 0.5f;

BYTE g_waterBumpSupported;

#define FACE_GULIM      "Gulim"
#define FACE_GULIM_KR   "굴림"
#define FACE_MS_PGOTHIC "굃굍 굊긕긘긞긏"
#define FACE_SIMSUN_CN  "芥竟"
#define FACE_MINGLIU_TW "꾄ⁿ톱"
#define HEX_FLAGS_0_9   ['0'] = 1, ['1'] = 1, ['2'] = 1, ['3'] = 1, ['4'] = 1, \
                        ['5'] = 1, ['6'] = 1, ['7'] = 1, ['8'] = 1, ['9'] = 1
#define HEX_VALUES_0_9  ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4, ['5'] = 5, \
                        ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9
struct C3dDevice g_3dDevice = {
    .rBits = 8, .gBits = 8, .bBits = 8, .aBits = 8,
    .colorKeyPixel = 0x421,
    .faceNamesB = { FACE_GULIM, "Arial", "MS Gothic", "SimSun", "MingLiU", "Tahoma",
                    "Arial", "Verdana", "Verdana", "Arial", "Arial", "Tahoma" },
    .faceNamesA = { FACE_GULIM_KR, "Arial", FACE_MS_PGOTHIC, FACE_SIMSUN_CN, FACE_MINGLIU_TW, "Tahoma",
                    "Arial", "Verdana", "Verdana", "Arial", "Arial", "Tahoma" },
    .faceCharsets = { CHARSET_HANGUL, CHARSET_ANSI, CHARSET_SHIFTJIS, CHARSET_GB2312, CHARSET_CHINESEBIG5,
                      CHARSET_THAI, CHARSET_VIETNAMESE, CHARSET_ANSI, CHARSET_ANSI, CHARSET_RUSSIAN,
                      CHARSET_VIETNAMESE + 1, CHARSET_ARABIC },
    .hexDigitFlags = { HEX_FLAGS_0_9, ['A'] = 1, ['B'] = 1, ['C'] = 1, ['D'] = 1, ['E'] = 1, ['F'] = 1,
                       ['a'] = 1, ['b'] = 1, ['c'] = 1, ['d'] = 1, ['e'] = 1, ['f'] = 1 },
    .hexDigitValues = { HEX_VALUES_0_9, ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15 },
};

GUID init_adapter;

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

int c3d_device_check_lost(struct C3dDevice *self)
{
    int hrPrimary, hrZ = 0;
    if (!self->surfacesLost)
        return 0;
    hrPrimary = gfx_is_lost(self->primary);
    if (self->zbuffer)
        hrZ = gfx_is_lost(self->zbuffer);
    if (hrPrimary == GFX_E_SURFACELOST && hrZ == GFX_E_SURFACELOST) {
        if (self->zbuffer)
            gfx_restore(self->zbuffer);
        gfx_restore_all(self);
        self->surfacesLost = 0;
    }
    return 1;
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

RECT *window_client_rect_on_monitor(RECT *out, HWND hwnd)
{
    platform_client_screen_rect(hwnd, out);
    out->left -= g_monitorRect.left;
    out->top -= g_monitorRect.top;
    out->right -= g_monitorRect.left;
    out->bottom -= g_monitorRect.top;
    return out;
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

int c3d_device_set_display_mode(struct C3dDevice *self, const struct SurfaceDesc *mode, int flags)
{
    int hr;
    if (flags & 1) {
        self->rcWindow = (RECT){0, 0, mode->width, mode->height};
        self->width = self->rcWindow.right;
        self->height = self->rcWindow.bottom;
        self->rcClient = self->rcWindow;
    } else {
        RECT rc;
        platform_client_rect(self->hwnd, &self->rcWindow);
        self->width = self->rcWindow.right;
        self->height = self->rcWindow.bottom;
        self->rcClient = *window_client_rect_on_monitor(&rc, self->hwnd);
        if (!(flags & 2))
            platform_client_screen_rect(self->hwnd, &self->rcWindow);
    }
    hr = gfx_create_screen(self, mode, flags);
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

int c3d_device_create_environment(struct C3dDevice *self, const GUID *adapter, const GUID *device,
                                            const struct SurfaceDesc *mode, int flags)
{
    int hr;
    hr = gfx_open(self, adapter, flags);
    if (hr < 0)
        return hr;
    hr = c3d_device_set_display_mode(self, mode, flags);
    if (hr < 0)
        return hr;
    if (!device)
        return 0;
    hr = gfx_create_3d(self, device, flags);
    if (hr < 0)
        return hr;
    gfx_get_caps(self, &g_gfxCaps);
    if (g_gfxCaps.zbufferless)
        return 0;
    return gfx_create_zbuffer(self);
}

int c3d_device_init(struct C3dDevice *self, HWND hwnd, const GUID *adapter, const GUID *device,
                               const struct SurfaceDesc *mode, int flags)
{
    int hr;
    RECT rcMon;
    self->hwnd = hwnd;
    if (!hwnd || !device)
        return E_INVALIDARG;
    if (!mode && (flags & 1))
        return E_INVALIDARG;
    self->fullscreen = flags & 1;
    if (adapter)
        init_adapter = *adapter;
    else
        memset(&init_adapter, 0, sizeof(init_adapter));
    if (self->fullscreen && mode && borderless_rect(adapter, &rcMon)) {
        self->fullscreen = 0;
        flags &= ~1;
        platform_window_frameless(g_hMainWnd, &rcMon);
    } else if (self->fullscreen) {
        platform_window_fullscreen(g_hMainWnd);
    }
    hr = c3d_device_create_environment(self, adapter, device, mode, flags);
    if (hr < 0) {
        gfx_close(self);
        if (hr == E_FAIL)
            return GFX_E_INIT;
    }
    if (self->dev)
        gfx_default_states(self);
    return hr;
}

void c3d_device_enable_mipmap(struct C3dDevice *self)
{
    (void)self;
    gfx_set_mipmaps(g_trilinearFilter && g_gfxCaps.mipNearest);
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
        g_waterBumpSupported = (BYTE)g_gfxCaps.waterBump;
        gfx_set_mipmaps(g_trilinearFilter && g_gfxCaps.mipNearest);
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

void device_flip_to_gdi(void)
{
    if (g_3dDevice.dd)
        gfx_flip_to_gdi(&g_3dDevice, 1);
}

void c3d_device_restore_frame(struct C3dDevice *self)
{
    if (self->target && self->save) {
        if (gfx_blit(self->target, NULL, self->save, NULL, GFX_BLT_ASYNC) < 0)
            gfx_blit(self->target, NULL, self->save, NULL, GFX_BLT_WAIT);
    }
}

int c3d_device_restore_surfaces(struct C3dDevice *self)
{
    return gfx_restore_all(self);
}

void c3d_device_on_surfaces_lost(struct C3dDevice *self)
{
    self->surfacesLost = 1;
}

int c3d_device_show_frame(struct C3dDevice *self, char vsync)
{
    RECT rc;
    if (!self->primary)
        return GFX_E_NO_PRIMARY;
    if (!self->back)
        return gfx_is_lost(self->primary);
    if (vsync) {
        if (self->fullscreen && !self->bltToClient) {
            platform_client_screen_rect(self->hwnd, &self->rcClient);
            return gfx_present(self, 1, 1);
        }
        self->rcClient = *window_client_rect_on_monitor(&rc, self->hwnd);
        return gfx_present(self, 1, 0);
    }
    platform_client_screen_rect(self->hwnd, &self->rcClient);
    return gfx_present(self, 0, 0);
}

static const DWORD probe_pattern[16] = {
    0, 0, 0xffffffff, 0xffffffff, 0, 0, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0, 0, 0xffffffff, 0xffffffff, 0, 0,
};

static int probe_codes(struct C3dDevice *self, struct Surface *surf, float texW, float texH, unsigned *best)
{
    const DWORD *pattern = probe_pattern;
    struct Vertex { float x, y; DWORD z, rhw, diffuse, specular; float u, v; } v[4];
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
                v[k].specular = 0xffffffff;
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
    gfx_set_render_state(GFX_RS_ALPHATESTENABLE, FALSE);
    gfx_set_render_state(GFX_RS_ALPHABLENDENABLE, FALSE);
    gfx_set_render_state(GFX_RS_ZENABLE, FALSE);
    gfx_set_stage_state(0, GFX_TSS_MINFILTER, GFX_FILTER_LINEAR);
    gfx_set_stage_state(0, GFX_TSS_MAGFILTER, GFX_FILTER_LINEAR);
    gfx_set_texture(0, tex.base.surface);

    ok = probe_codes(self, &surf, (float)(unsigned)tex.base.width, (float)(unsigned)tex.base.height, &best);
    if (ok) {
        g_texelOffsetU = (float)(int)(best % 256) * (1.0f / 256.0f);
        g_texelOffsetV = (float)(int)(best / 256) * (1.0f / 256.0f);
        gfx_set_stage_state(0, GFX_TSS_MINFILTER, GFX_FILTER_LINEAR);
        gfx_set_stage_state(0, GFX_TSS_MAGFILTER, GFX_FILTER_LINEAR);
    }
    surface_reset(&tex.base);
    surface_reset(&surf);
    return ok;
}
