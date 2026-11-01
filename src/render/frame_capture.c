#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "render/frame_capture.h"
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

char renderer_screenshot(struct Renderer *self, struct String filename)
{
    struct gfx_lock lock;
    struct JpegImage image = {0};
    char ok = 0;
    (void)self;

    if (gfx_lock(g_3dDevice.target, NULL, GFX_LOCK_WAIT, &lock) >= 0) {
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
                    if (lock.bpp == 16) {
                        pixel16_to_bgr(*(const WORD *)src, dst);
                    } else {
                        memcpy(dst, src, 3);
                    }
                }
            }
            ok = 1;
        }
        gfx_unlock(g_3dDevice.target);
        if (ok) {
            FILE *file = fopen(string_cstr(&filename), "wb");
            ok = file && jpeg_image_write(&image, file);
            if (file && fclose(file))
                ok = 0;
        }
    }
    jpeg_image_destroy(&image);
    string_free(&filename);
    return ok;
}
