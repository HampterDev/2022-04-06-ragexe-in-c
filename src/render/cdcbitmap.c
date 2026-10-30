#include "render/render_api.h"
#include <string.h>

#define ALPHA_STEP 0.06666667f
#define ALPHA_FULL 0.97f

static void blend4444(struct DcBitmap *b, WORD pix, int dx0, int dy0, int cw, int ch)
{
    float a = (float)(pix >> 12) * ALPHA_STEP, inv;
    int r = (pix >> 4) & 0xf0, g = pix & 0xf0, bl = (pix & 0xf) << 4, j, k;
    if (a > ALPHA_FULL)
        a = 1.0f;
    inv = 1.0f - a;
    for (k = 0; k < ch; k++) {
        int dy = dy0 + k;
        for (j = 0; j < cw; j++) {
            int dx = dx0 + j;
            DWORD *p, d;
            if (dx < 0 || dy < 0 || (unsigned)b->width < (unsigned)dx || (unsigned)b->height < (unsigned)dy)
                continue;
            p = &b->bits[dy * b->width + dx];
            d = *p;
            *p = ((DWORD)(BYTE)(int)((float)((d >> 16) & 0xff) * inv + (float)r * a) << 16)
               | ((DWORD)(BYTE)(int)((float)((d >> 8) & 0xff) * inv + (float)g * a) << 8)
               |  (DWORD)(BYTE)(int)((float)(d & 0xff) * inv + (float)bl * a);
        }
    }
}

static void write1555(struct DcBitmap *b, WORD pix, int dx0, int dy0, int cw, int ch)
{
    DWORD v = ((pix >> 7) & 0xf8) + 0xff00;
    int j, k;
    v = (v << 8) + ((pix >> 2) & 0xf8);
    v = (v << 8) + ((pix << 3) & 0xff);
    for (k = 0; k < ch; k++) {
        int dy = dy0 + k;
        for (j = 0; j < cw; j++) {
            int dx = dx0 + j;
            if (dx < 0 || dy < 0 || (unsigned)b->width < (unsigned)dx || (unsigned)b->height < (unsigned)dy)
                continue;
            b->bits[dy * b->width + dx] = v;
        }
    }
}

void dc_bitmap_draw_texels(struct DcBitmap *self, int x, int y, struct Texture *tex, int sx, int sy,
                           int sw, int sh, int mirror, int cw, int ch)
{
    struct gfx_lock lk;
    const WORD *src;
    int pitch2, r, i, col;
    if (!tex || !self->bits || !tex->base.surface)
        return;
    if (gfx_lock(tex->base.surface, NULL, GFX_LOCK_WAIT, &lk) < 0)
        return;
    src = (const WORD *)lk.bits;
    pitch2 = lk.pitch / 2;
    self->changed = 1;
    if (tex->format == GFX_FMT_4444) {
        if (mirror) {
            for (r = 0; r < sh; r++) {
                const WORD *row = src + pitch2 * (sy + r) + sx;
                for (i = sw - 1, col = 0; i >= 0; i--, col += cw)
                    if (row[i] & 0xf000)
                        blend4444(self, row[i], x + col, y + r * ch, cw, ch);
            }
        } else {
            for (r = 0; r < sh; r++) {
                const WORD *row = src + pitch2 * (sy + r) + sx;
                for (i = 0, col = 0; i < sw; i++, col += cw)
                    if (row[i] & 0xf000)
                        blend4444(self, row[i], x + col, y + r * ch, cw, ch);
            }
        }
    } else if (tex->format == GFX_FMT_1555) {
        if (mirror) {
            for (r = 0; r < sh; r++) {
                const WORD *row = src + pitch2 * (sy + r) + sx;
                for (i = sw - 1, col = 0; i >= 0; i--, col += cw)
                    if (row[i] & 0x8000)
                        write1555(self, row[i], x + col, y + r * ch, cw, ch);
            }
        } else {
            for (r = 0; r < sh; r++) {
                const WORD *row = src + pitch2 * (sy + r) + sx;
                for (i = 0, col = 0; i < sw; i++, col += cw)
                    if (row[i] & 0x8000)
                        write1555(self, row[i], x + col, y + r * ch, cw, ch);
            }
        }
    }
    gfx_unlock(tex->base.surface);
}

void dc_bitmap_draw_texels_scaled(struct DcBitmap *self, int dx0, int dy0, int dw, int dh, struct Texture *tex,
                           int sw, int sh, float scaleX, float scaleY, DWORD colour, char mirror,
                           int offX, int offY, int sx, int sy)
{
    struct gfx_lock lk;
    gfx_surface *surf;
    const WORD *srcRow;
    DWORD *dst;
    float a, cr, cg, cb, inv, invSX, invSY;
    int pitch2, srcCount, w, h, i, j, format;
    if (!tex || dx0 < 0 || (unsigned)self->width < (unsigned)dx0 || dy0 < 0 || (unsigned)self->height < (unsigned)dy0
        || dw < 0 || dh < 0 || (unsigned)self->width < (unsigned)(dx0 + dw) || (unsigned)self->height < (unsigned)(dy0 + dh)
        || 0.0f >= scaleX || 0.0f >= scaleY || 0.0f >= (float)colour)
        return;
    surf = tex->base.surface;
    format = tex->format;
    if (!surf)
        return;
    if (gfx_lock(surf, NULL, GFX_LOCK_WAIT | GFX_LOCK_UNSIZED, &lk) < 0)
        return;
    pitch2 = lk.pitch / 2;
    srcRow = (const WORD *)lk.bits + pitch2 * sy + sx;
    dst = self->bits + self->width * dy0 + dx0;
    a = (float)(colour >> 24) / 255.0f;
    cr = (float)((colour >> 16) & 0xff) * a / 255.0f;
    cg = (float)((colour >> 8) & 0xff) * a / 255.0f;
    cb = (float)(colour & 0xff) * a / 255.0f;
    inv = 1.0f - a;
    invSX = 1.0f / scaleX;
    invSY = 1.0f / scaleY;
    w = (int)((float)sw * scaleX);
    h = (int)((float)sh * scaleY);
    srcCount = pitch2 * sh;
    if (format == GFX_FMT_4444 || format == GFX_FMT_1555) {
        for (i = 0; i < h; i++) {
            int py = offY + i;
            for (j = 0; j < w; j++) {
                int px = offX + j, si, di, r, g, b;
                WORD pix;
                DWORD dv, v;
                if (px < 0 || dw < px || py < 0 || dh < py)
                    continue;
                si = (int)((float)i * invSY) * pitch2;
                if (mirror)
                    si = si - (int)((float)j * invSX) + sw - 1;
                else
                    si += (int)((float)j * invSX);
                if (si < 0 || srcCount <= si)
                    continue;
                pix = srcRow[si];
                if (format == GFX_FMT_4444) {
                    if (!(pix & 0xf000))
                        continue;
                    r = (pix >> 4) & 0xf0;
                    g = pix & 0xf0;
                    b = (pix & 0xf) << 4;
                } else {
                    if (!(pix & 0x8000))
                        continue;
                    r = (pix >> 7) & 0xf8;
                    g = (pix >> 2) & 0xf8;
                    b = (pix & 0x1f) << 3;
                }
                di = self->width * py + j + offX;
                dv = dst[di];
                v = (BYTE)(int)((float)((dv >> 16) & 0xff) * inv + (float)r * cr) + 0xff00;
                v = (v << 8) + (BYTE)(int)((float)((dv >> 8) & 0xff) * inv + (float)g * cg);
                v = (v << 8) + (BYTE)(int)((float)b * cb + (float)(dv & 0xff) * inv);
                dst[di] = v;
            }
        }
    }
    gfx_unlock(surf);
}
