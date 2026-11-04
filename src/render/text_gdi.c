#include "render/text_backend.h"
#ifdef TEXT_BACKEND_GDI
#include "render/cdc.h"
#include "render/textdc.h"
#include "render/gfx.h"
#include "core/layout.h"

/* The GDI text backend (render/text_backend.h): bitmaps are DIB sections in a memory DC, text DCs
   are a bitmap's DC or a surface's, and GDI draws and measures. */

STATIC_ASSERT(text_dc_background_modes, TEXT_DC_TRANSPARENT == TRANSPARENT && TEXT_DC_OPAQUE == OPAQUE);

int text_bitmap_open(struct DcBitmap *self)
{
    self->text.hdc = CreateCompatibleDC(NULL);
    return self->text.hdc != NULL;
}

DWORD *text_bitmap_resize(struct DcBitmap *self, int w, int h)
{
    BITMAPINFO info = {0};
    HBITMAP dib;
    HGDIOBJ previous;
    DWORD *bits;
    if (!self->text.hdc)
        return NULL;
    info.bmiHeader.biSize = sizeof info.bmiHeader;
    info.bmiHeader.biWidth = w;
    info.bmiHeader.biHeight = -h;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biXPelsPerMeter = info.bmiHeader.biYPelsPerMeter = 100;
    dib = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
    if (!dib)
        return NULL;
    previous = SelectObject(self->text.hdc, dib);
    if (!previous || previous == HGDI_ERROR) {
        DeleteObject(dib);
        return NULL;
    }
    if (self->text.dib)
        DeleteObject(self->text.dib);
    else
        self->text.oldBitmap = previous;
    self->text.dib = dib;
    return bits;
}

void text_bitmap_close(struct DcBitmap *self)
{
    if (self->text.hdc) {
        if (self->text.oldBitmap)
            SelectObject(self->text.hdc, self->text.oldBitmap);
        if (self->text.dib)
            DeleteObject(self->text.dib);
        DeleteDC(self->text.hdc);
    }
    self->text.hdc = NULL;
    self->text.dib = NULL;
    self->text.oldBitmap = NULL;
}

static HDC bitmap_dc(struct DcBitmap *self)
{
    if (!self || !self->text.hdc)
        return NULL;
    self->changed = 1;
    return self->text.hdc;
}

void dc_bitmap_fill_triangle(struct DcBitmap *self, const POINT points[3], COLORREF colour)
{
    HDC dc = bitmap_dc(self);
    HRGN region;
    HBRUSH brush;

    if (!dc)
        return;
    region = CreatePolygonRgn(points, 3, WINDING);
    brush = CreateSolidBrush(colour);
    FillRgn(dc, region, brush);
    DeleteObject(region);
    DeleteObject(brush);
}

static void text_dc_prepare(struct TextDC *self)
{
    if (self->state.hdc) {
        self->state.oldFont = GetCurrentObject(self->state.hdc, OBJ_FONT);
        SetBkMode(self->state.hdc, TRANSPARENT);
    }
}

struct TextDC *text_dc_construct(struct TextDC *self, struct DcBitmap *bitmap)
{
    *self = (struct TextDC){0};
    self->state.hdc = bitmap_dc(bitmap);
    text_dc_prepare(self);
    return self;
}

struct TextDC *text_dc_from_surface(struct TextDC *self, gfx_surface *surface)
{
    *self = (struct TextDC){0};
    if (surface) {
        gfx_addref(surface);
        self->surface = surface;
        if (gfx_get_dc(surface, &self->state.hdc) < 0)
            self->state.hdc = NULL;
        text_dc_prepare(self);
    }
    return self;
}

void text_dc_destruct(struct TextDC *self)
{
    if (self->state.hdc) {
        if (self->state.oldFont)
            SelectObject(self->state.hdc, self->state.oldFont);
        if (self->surface)
            gfx_release_dc(self->surface, self->state.hdc);
    }
    if (self->surface)
        gfx_release(self->surface);
    self->state.hdc = NULL;
    self->surface = NULL;
    self->state.oldFont = NULL;
}

COLORREF text_dc_set_bk_color(struct TextDC *self, COLORREF colour)
{
    return SetBkColor(self->state.hdc, colour);
}

int text_dc_set_bk_mode(struct TextDC *self, int mode)
{
    return SetBkMode(self->state.hdc, mode);
}

COLORREF text_dc_set_text_color(struct TextDC *self, COLORREF colour)
{
    return SetTextColor(self->state.hdc, colour);
}

int text_dc_use_font(struct TextDC *self, FontHandle font)
{
    HGDIOBJ old = SelectObject(self->state.hdc, font);
    return old && old != HGDI_ERROR;
}

/* Italic text is measured without the overhang of each character's right side. A
   surrogate pair is one character, measured as a whole. */
static int italic_width(HDC dc, const WCHAR *text, int length, LONG *width)
{
    int i, sum = 0;
    for (i = 0; i < length; i++) {
        if (text[i] >= 0xd800 && text[i] <= 0xdbff && i + 1 < length
            && text[i + 1] >= 0xdc00 && text[i + 1] <= 0xdfff) {
            SIZE pair;
            if (!GetTextExtentPoint32W(dc, text + i, 2, &pair))
                return 0;
            sum += pair.cx;
            i++;
        } else {
            ABC abc;
            if (!GetCharABCWidthsW(dc, text[i], text[i], &abc))
                return 0;
            sum += abc.abcA + abc.abcB;
        }
    }
    *width = sum;
    return 1;
}

int text_dc_measure_run(struct TextDC *self, const uint16_t *text, int length, SIZE *out)
{
    int ok = GetTextExtentPoint32W(self->state.hdc, text, length, out);
    if (ok && self->italic == 1)
        ok = italic_width(self->state.hdc, text, length, &out->cx);
    return ok;
}

void text_dc_draw_run(struct TextDC *self, int x, int y, const uint16_t *text, int length)
{
    TextOutW(self->state.hdc, x, y, text, length);
}

int text_dc_font_covers(struct TextDC *self, FontHandle font, int script)
{
    return font_cache_covers(&g_fontCache, self->state.hdc, font, script);
}

int text_dc_font_ascent(struct TextDC *self, FontHandle font)
{
    return font_cache_ascent(&g_fontCache, self->state.hdc, font);
}

void text_dc_text_out_plain(struct TextDC *self, int x, int y, const char *s, int len)
{
    TextOutA(self->state.hdc, x, y, s, len);
}
#endif
