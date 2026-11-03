#include "render/cdc.h"
#include "render/device.h"
#include "platform/code_page.h"
#include "platform/language.h"
#include "platform/language_api.h"
#include "render/text_unicode.h"
#include "render/textdc.h"
#include "render/fonts.h"
#include "render/text_runs.h"
#include "platform/clientinfo.h"

static void text_dc_prepare(struct TextDC *self)
{
    if (self->hdc) {
        self->oldFont = GetCurrentObject(self->hdc, OBJ_FONT);
        SetBkMode(self->hdc, TRANSPARENT);
    }
}

struct TextDC *text_dc_construct(struct TextDC *self, struct DcBitmap *bitmap)
{
    *self = (struct TextDC){0};
    dc_bitmap_get_dc(bitmap, &self->hdc);
    text_dc_prepare(self);
    return self;
}

struct TextDC *text_dc_from_surface(struct TextDC *self, gfx_surface *surface)
{
    *self = (struct TextDC){0};
    if (surface) {
        gfx_addref(surface);
        self->surface = surface;
        if (gfx_get_dc(surface, &self->hdc) < 0)
            self->hdc = NULL;
        text_dc_prepare(self);
    }
    return self;
}

void text_dc_destruct(struct TextDC *self)
{
    if (self->hdc) {
        if (self->oldFont) SelectObject(self->hdc, self->oldFont);
        if (self->surface) gfx_release_dc(self->surface, self->hdc);
    }
    if (self->surface) gfx_release(self->surface);
    self->hdc = NULL;
    self->surface = NULL;
    self->oldFont = NULL;
}

void text_dc_select_font(struct TextDC *self, int a, int b, int c, int d)
{
    int face = a, size = b;

    /* Face 0 is the default UI face: Arial (face 1), or Arial with face 6's
       charset when Windows uses Turkish; both three points larger. */
    if (a == 0) {
        size = b + 3;
        face = g_language && g_language->folder_lang == LANG_TURKISH ? 6 : 1;
    }
    self->face = face;
    self->size = size;
    /* Faces past the 12-entry table used to read the bytes after it. Keep the values that
       mattered: SCDream got 0x10101, which fonts truncate to DEFAULT_CHARSET, and faces
       12-19 got ANSI_CHARSET; the EOT fonts ignore the charset. */
    if (face >= 0 && (size_t)face < sizeof g_3dDevice.faceCharsets / sizeof g_3dDevice.faceCharsets[0])
        self->charset = g_3dDevice.faceCharsets[face];
    else
        self->charset = face == FONT_FACE_SCDREAM ? DEFAULT_CHARSET : ANSI_CHARSET;
    self->bold = (BYTE)c;
    self->italic = (BYTE)d;
}

COLORREF text_dc_set_bk_color(struct TextDC *self, COLORREF colour)
{
    return SetBkColor(self->hdc, colour);
}

int text_dc_set_bk_mode(struct TextDC *self, int mode)
{
    return SetBkMode(self->hdc, mode);
}

COLORREF text_dc_set_text_color(struct TextDC *self, COLORREF colour)
{
    return SetTextColor(self->hdc, colour);
}

static HFONT face_font(struct TextDC *self)
{
    return font_cache_get_font(&g_fontCache, self->face, self->size, self->charset, self->bold, self->italic);
}

/* The selected face draws a run unless it lacks the run's script; then the script's
   Windows fallback font does, if one is installed. */
static HFONT run_font(struct TextDC *self, HFONT face, enum TextScript script)
{
    HFONT fallback;

    if (script == TEXT_SCRIPT_DEFAULT || font_cache_covers(&g_fontCache, self->hdc, face, script))
        return face;
    fallback = font_cache_get_script_font(&g_fontCache, script, self->size, self->bold, self->italic);
    return fallback ? fallback : face;
}

/* How far down to draw a fallback run so that it shares the selected face's baseline. */
static int baseline_offset(struct TextDC *self, HFONT face, HFONT font)
{
    int want, have;

    if (font == face)
        return 0;
    want = font_cache_ascent(&g_fontCache, self->hdc, face);
    have = font_cache_ascent(&g_fontCache, self->hdc, font);
    return want > 0 && have > 0 ? want - have : 0;
}

void text_dc_text_extent(struct TextDC *self, const char *s, int len, SIZE *out)
{
    size_t at = 0, length = s && len > 0 ? (size_t)len : 0, run;
    enum TextScript script;
    HFONT face;

    out->cx = out->cy = 0;
    if (!length || !(face = face_font(self)))
        return;
    while (at < length) {
        SIZE part;

        run = text_script_run(s + at, length - at, &script);
        text_dc_measure_font(self, &part, s + at, (int)run, run_font(self, face, script));
        out->cx += part.cx;
        if (part.cy > out->cy)
            out->cy = part.cy;
        at += run;
    }
}

/* Text is UTF-8, drawn run by run in the font for each run's script; each run starts
   where the measured width of the runs before it ends. */
void text_dc_text_out(struct TextDC *self, int x, int y, const char *s, int len)
{
    size_t at = 0, length = s && len > 0 ? (size_t)len : 0, run;
    enum TextScript script;
    HFONT face = NULL;

    while (at < length) {
        struct TextUnicode text;
        HFONT font;
        HGDIOBJ old = NULL;
        SIZE extent;
        int top = y;

        run = text_script_run(s + at, length - at, &script);
        if (!text_unicode_init(&text, s + at, (int)run, CP_UTF8))
            return;
        if (!face)
            face = face_font(self);
        font = face ? run_font(self, face, script) : NULL;
        if (font) {
            top += baseline_offset(self, face, font);
            old = SelectObject(self->hdc, font);
        }
        if (old && old != HGDI_ERROR)
            TextOutW(self->hdc, x, top, text.data, text.length);
        text_unicode_release(&text);
        if (!old || old == HGDI_ERROR)
            return;
        at += run;
        if (at < length) {
            text_dc_measure_font(self, &extent, s + at - run, (int)run, font);
            x += extent.cx;
        }
    }
}
