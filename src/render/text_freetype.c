#include "render/text_backend.h"
#ifndef TEXT_BACKEND_GDI
#include "render/cdc.h"
#include "render/textdc.h"
#include "render/gfx.h"
#include <stdlib.h>

/* The FreeType text backend (render/text_backend.h): bitmaps are plain pixels, text DCs draw into
   a bitmap's pixels or into a locked 32-bit surface, and render/fonts_ft.c draws and measures. */

int text_bitmap_open(struct DcBitmap *self)
{
    (void)self;
    return 1;
}

DWORD *text_bitmap_resize(struct DcBitmap *self, int w, int h)
{
    DWORD *bits = calloc((size_t)w * (size_t)h, sizeof *bits);
    if (bits)
        free(self->bits);
    return bits;
}

void text_bitmap_close(struct DcBitmap *self)
{
    free(self->bits);
}

static long long ceil_div(long long numerator, long long denominator)
{
    long long quotient = numerator / denominator;
    return quotient + (numerator % denominator > 0);
}

void dc_bitmap_fill_triangle(struct DcBitmap *self, const POINT points[3], COLORREF colour)
{
    DWORD pixel = (DWORD)GetRValue(colour) << 16 | (DWORD)GetGValue(colour) << 8 | GetBValue(colour);
    int y, i;

    if (!self || !self->bits)
        return;
    self->changed = 1;
    for (y = 0; y < self->height; y++) {
        long long cross[3];
        int count = 0, x, left, right;
        for (i = 0; i < 3; i++) {
            const POINT *a = &points[i], *b = &points[(i + 1) % 3];
            if (a->y == b->y)
                continue;
            if (a->y > b->y) {
                const POINT *t = a;
                a = b;
                b = t;
            }
            if (y < a->y || y >= b->y)
                continue;
            cross[count++] = ceil_div((long long)a->x * (b->y - a->y) + (long long)(b->x - a->x) * (y - a->y),
                                      b->y - a->y);
        }
        if (count != 2)
            continue;
        left = (int)(cross[0] < cross[1] ? cross[0] : cross[1]);
        right = (int)(cross[0] < cross[1] ? cross[1] : cross[0]);
        if (left < 0)
            left = 0;
        if (right > self->width)
            right = self->width;
        for (x = left; x < right; x++)
            self->bits[self->width * y + x] = pixel;
    }
}

static void text_dc_prepare(struct TextDC *self)
{
    self->state.bkColour = RGB(0xff, 0xff, 0xff);
    self->state.bkMode = TEXT_DC_TRANSPARENT;
}

struct TextDC *text_dc_construct(struct TextDC *self, struct DcBitmap *bitmap)
{
    *self = (struct TextDC){0};
    if (bitmap && bitmap->bits) {
        bitmap->changed = 1;
        self->state.bitmap = bitmap;
    }
    text_dc_prepare(self);
    return self;
}

struct TextDC *text_dc_from_surface(struct TextDC *self, gfx_surface *surface)
{
    *self = (struct TextDC){0};
    if (surface) {
        gfx_addref(surface);
        self->surface = surface;
    }
    text_dc_prepare(self);
    return self;
}

void text_dc_destruct(struct TextDC *self)
{
    if (self->state.locked > 0)
        gfx_unlock(self->surface);
    if (self->surface)
        gfx_release(self->surface);
    self->surface = NULL;
    self->state.bitmap = NULL;
    self->state.font = NULL;
    self->state.locked = 0;
}

COLORREF text_dc_set_bk_color(struct TextDC *self, COLORREF colour)
{
    COLORREF previous = self->state.bkColour;
    self->state.bkColour = colour;
    return previous;
}

int text_dc_set_bk_mode(struct TextDC *self, int mode)
{
    int previous = self->state.bkMode;
    self->state.bkMode = mode;
    return previous;
}

COLORREF text_dc_set_text_color(struct TextDC *self, COLORREF colour)
{
    COLORREF previous = self->state.textColour;
    self->state.textColour = colour;
    return previous;
}

/* The pixels text goes to: the bitmap's, or the surface's, locked on the first run and until the
   DC goes. */
static int text_dc_target(struct TextDC *self, struct FontTarget *target)
{
    struct gfx_lock lock;
    if (self->state.bitmap) {
        target->bits = (BYTE *)self->state.bitmap->bits;
        target->width = self->state.bitmap->width;
        target->height = self->state.bitmap->height;
        target->pitch = self->state.bitmap->width * 4;
        return target->bits != NULL;
    }
    if (!self->surface || self->state.locked < 0)
        return 0;
    if (!self->state.locked) {
        self->state.locked = -1;
        if (gfx_lock(self->surface, NULL, GFX_LOCK_WAIT, &lock) < 0)
            return 0;
        if (lock.bpp != 32) {
            gfx_unlock(self->surface);
            return 0;
        }
        self->state.target.bits = lock.bits;
        self->state.target.width = (int)lock.width;
        self->state.target.height = (int)lock.height;
        self->state.target.pitch = (int)lock.pitch;
        self->state.locked = 1;
    }
    *target = self->state.target;
    return 1;
}

int text_dc_use_font(struct TextDC *self, FontHandle font)
{
    self->state.font = font;
    return 1;
}

int text_dc_measure_run(struct TextDC *self, const uint16_t *text, int length, SIZE *out)
{
    font_text_extent(self->state.font, text, length, self->italic == 1, out);
    return 1;
}

void text_dc_draw_run(struct TextDC *self, int x, int y, const uint16_t *text, int length)
{
    struct FontTarget target;
    if (text_dc_target(self, &target))
        font_text_out(self->state.font, &target, x, y, text, length, self->state.textColour,
                      self->state.bkMode == TEXT_DC_OPAQUE ? &self->state.bkColour : NULL);
}

int text_dc_font_covers(struct TextDC *self, FontHandle font, int script)
{
    (void)self;
    return font_covers(font, script);
}

int text_dc_font_ascent(struct TextDC *self, FontHandle font)
{
    (void)self;
    return font_ascent(font);
}

void text_dc_text_out_plain(struct TextDC *self, int x, int y, const char *s, int len)
{
    text_dc_draw_plain(self, x, y, s, len);
}
#endif
