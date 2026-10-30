#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "render/frame_capture.h"
#include "resources/bitmap_types.h"
#include "render/render_api.h"
#include "resources/jpeg_codec.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

void pixel16_to_bgr(WORD pix, BYTE *out)
{
    const struct C3dDevice *d = &g_3dDevice;
    out[0] = (BYTE)((BYTE)((pix & d->bMask) >> d->bShift) << d->bBits);
    out[1] = (BYTE)((BYTE)((pix & d->gMask) >> d->gShift) << d->gBits);
    out[2] = (BYTE)((BYTE)((pix & d->rMask) >> d->rShift) << d->rBits);
}

int renderer_lock_target(struct Renderer *self)
{
    struct gfx_lock lk;
    if (!self || gfx_lock(g_3dDevice.target, NULL, GFX_LOCK_WAIT, &lk) < 0)
        return 0;
    if (!lk.bits || self->width <= 0 || self->height <= 0 ||
        (unsigned)self->width > lk.width || (unsigned)self->height > lk.height ||
        (lk.bpp != 16 && lk.bpp != 32) || lk.pitch <= 0 ||
        (unsigned)self->width > (unsigned)lk.pitch / (lk.bpp / 8)) {
        gfx_unlock(g_3dDevice.target);
        return 0;
    }
    self->targetBits = lk.bits;
    self->bpp = lk.bpp;
    self->targetPitch = lk.pitch;
    return 1;
}

static const DWORD *logo_pixel(const struct BitmapRes *logo, int w, int h, int x, int y)
{
    int lx, ly;
    if (!logo || !logo->pixels || logo->width <= 0 || logo->height <= 0 ||
        x < w - logo->width || y < h - logo->height)
        return NULL;
    ly = y - h + logo->height;
    lx = logo->width - w + x;
    if (lx < 0 || lx >= logo->width || ly < 0 || ly >= logo->height)
        return NULL;
    return &logo->pixels[logo->width * ly + lx];
}

int renderer_save_jpeg_region(struct Renderer *self, const char *filename, const RECT *rc)
{
    struct JpegImage image = {0};
    int w, h, x, y, ok = 0;

    if (!self || !rc || rc->left < 0 || rc->top < 0 ||
        rc->right > self->width || rc->bottom > self->height ||
        rc->right <= rc->left || rc->bottom <= rc->top)
        return 0;
    if (!renderer_lock_target(self))
        return 0;
    w = rc->right - rc->left;
    h = rc->bottom - rc->top;
    if (self->targetBits && (self->bpp == 16 || self->bpp == 32) &&
        self->targetPitch > 0 &&
        (size_t)self->width <= (size_t)self->targetPitch / (self->bpp / 8) &&
        jpeg_image_create(&image, w, h)) {
        for (y = 0; y < h; ++y) {
            const BYTE *src = (const BYTE *)self->targetBits +
                (size_t)(rc->top + y) * self->targetPitch + (size_t)rc->left * (self->bpp / 8);
            BYTE *dst = image.pixels + (size_t)y * image.stride;
            for (x = 0; x < w; ++x, src += self->bpp / 8, dst += 3) {
                if (self->bpp == 16)
                    pixel16_to_bgr(*(const WORD *)src, dst);
                else
                    memcpy(dst, src, 3);
            }
        }
        ok = 1;
    }
    gfx_unlock(g_3dDevice.target);
    if (ok)
        ok = jpeg_image_write(&image, filename);
    jpeg_image_destroy(&image);
    return ok;
}

char renderer_screenshot(struct Renderer *self, struct String filename)
{
    struct BitmapRes *logo;
    struct gfx_lock lock;
    struct JpegImage image = {0};
    char ok = 0;
    (void)self;

    logo = (struct BitmapRes *)res_mgr_get(g_resMgr(), "scr_logo.bmp");
    if (logo && gfx_lock(g_3dDevice.target, NULL, GFX_LOCK_WAIT, &lock) >= 0) {
        if (lock.bits && lock.width && lock.height &&
            lock.width <= INT_MAX && lock.height <= INT_MAX &&
            (lock.bpp == 16 || lock.bpp == 32) && lock.pitch > 0 &&
            lock.width <= (unsigned)lock.pitch / (lock.bpp / 8) &&
            jpeg_image_create(&image, (int)lock.width, (int)lock.height)) {
            unsigned x, y;
            for (y = 0; y < lock.height; ++y) {
                const BYTE *src = (const BYTE *)lock.bits + (size_t)y * lock.pitch;
                BYTE *dst = image.pixels + (size_t)y * image.stride;
                for (x = 0; x < lock.width; ++x, src += lock.bpp / 8, dst += 3) {
                    const DWORD *p = logo_pixel(logo, (int)lock.width, (int)lock.height, (int)x, (int)y);
                    if (p && (*p & 0xffffff) != 0xff00ff) {
                        dst[0] = (BYTE)*p;
                        dst[1] = (BYTE)(*p >> 8);
                        dst[2] = (BYTE)(*p >> 16);
                    } else if (lock.bpp == 16) {
                        pixel16_to_bgr(*(const WORD *)src, dst);
                    } else {
                        memcpy(dst, src, 3);
                    }
                }
            }
            ok = 1;
        }
        gfx_unlock(g_3dDevice.target);
        if (ok)
            ok = (char)jpeg_image_write(&image, string_cstr(&filename));
    }
    jpeg_image_destroy(&image);
    string_free(&filename);
    return ok;
}
