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

void text_dc_select_font(struct TextDC *self, int a, int b, int c, int d)
{
    int face = a, size = b;

    /* Face 0 is the default UI face: Arial (face 1), or Arial with face 6's
       charset when Windows uses Turkish; both three points larger. */
    if (a == 0) {
        size = b + 3;
        face = g_language && g_language->folder_lang == LANGUAGE_ID_TURKISH ? 6 : 1;
    }
    self->face = face;
    self->size = size;
    /* Faces past the 12-entry table used to read the bytes after it. Keep the values that
       mattered: SCDream got 0x10101, which fonts truncate to DEFAULT_CHARSET, and faces
       12-19 got ANSI_CHARSET; the EOT fonts ignore the charset. */
    if (face >= 0 && (size_t)face < sizeof g_3dDevice.faceCharsets / sizeof g_3dDevice.faceCharsets[0])
        self->charset = g_3dDevice.faceCharsets[face];
    else
        self->charset = face == FONT_FACE_SCDREAM ? CHARSET_DEFAULT : CHARSET_ANSI;
    self->bold = (BYTE)c;
    self->italic = (BYTE)d;
}

static FontHandle face_font(struct TextDC *self)
{
    return font_cache_get_font(&g_fontCache, self->face, self->size, self->charset, self->bold, self->italic);
}

/* The selected face draws a run unless it lacks the run's script; then the script's
   fallback font does, if one is installed. */
static FontHandle run_font(struct TextDC *self, FontHandle face, enum TextScript script)
{
    FontHandle fallback;

    if (script == TEXT_SCRIPT_DEFAULT || font_covers(face, script))
        return face;
    fallback = font_cache_get_script_font(&g_fontCache, script, self->size, self->bold, self->italic);
    return fallback ? fallback : face;
}

/* How far down to draw a fallback run so that it shares the selected face's baseline. */
static int baseline_offset(FontHandle face, FontHandle font)
{
    int want, have;

    if (font == face)
        return 0;
    want = font_ascent(face);
    have = font_ascent(font);
    return want > 0 && have > 0 ? want - have : 0;
}

/* The pixels text goes to: the bitmap's. */
static int text_target(struct TextDC *self, struct FontTarget *target)
{
    if (!self->bitmap)
        return 0;
    target->bits = (BYTE *)self->bitmap->bits;
    target->width = self->bitmap->width;
    target->height = self->bitmap->height;
    target->pitch = self->bitmap->width * 4;
    return target->bits != NULL;
}

static void draw_run(struct TextDC *self, FontHandle font, int x, int y, const uint16_t *text, int length)
{
    struct FontTarget target;
    if (text_target(self, &target))
        font_text_out(font, &target, x, y, text, length, self->textColour,
                      self->bkMode == TEXT_DC_OPAQUE ? &self->bkColour : NULL);
}

struct TextDC *text_dc_construct(struct TextDC *self, struct DcBitmap *bitmap)
{
    *self = (struct TextDC){0};
    if (bitmap && bitmap->bits) {
        bitmap->changed = 1;
        self->bitmap = bitmap;
    }
    self->bkColour = RGB(0xff, 0xff, 0xff);
    self->bkMode = TEXT_DC_TRANSPARENT;
    return self;
}

void text_dc_destruct(struct TextDC *self)
{
    self->bitmap = NULL;
}

COLORREF text_dc_set_bk_color(struct TextDC *self, COLORREF colour)
{
    COLORREF previous = self->bkColour;
    self->bkColour = colour;
    return previous;
}

int text_dc_set_bk_mode(struct TextDC *self, int mode)
{
    int previous = self->bkMode;
    self->bkMode = mode;
    return previous;
}

COLORREF text_dc_set_text_color(struct TextDC *self, COLORREF colour)
{
    COLORREF previous = self->textColour;
    self->textColour = colour;
    return previous;
}

void text_dc_text_extent(struct TextDC *self, const char *s, int len, SIZE *out)
{
    size_t at = 0, length = s && len > 0 ? (size_t)len : 0, run;
    enum TextScript script;
    FontHandle face;

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
    FontHandle face = NULL;

    while (at < length) {
        struct TextUnicode text;
        FontHandle font;
        SIZE extent;

        run = text_script_run(s + at, length - at, &script);
        if (!text_unicode_init(&text, s + at, (int)run, CODE_PAGE_UTF8))
            return;
        if (!face)
            face = face_font(self);
        font = face ? run_font(self, face, script) : NULL;
        if (font)
            draw_run(self, font, x, y + baseline_offset(face, font), text.data, text.length);
        text_unicode_release(&text);
        if (!font)
            return;
        at += run;
        if (at < length) {
            text_dc_measure_font(self, &extent, s + at - run, (int)run, font);
            x += extent.cx;
        }
    }
}

void text_dc_text_extent_plain(struct TextDC *self, const char *s, int len, SIZE *out)
{
    FontHandle font = face_font(self);

    out->cx = out->cy = 0;
    if (font)
        text_dc_measure_font(self, out, s, len, font);
}

void text_dc_text_out_plain(struct TextDC *self, int x, int y, const char *s, int len)
{
    struct TextUnicode text;
    FontHandle font;

    if (!s || len <= 0 || !text_unicode_init(&text, s, len, CODE_PAGE_UTF8))
        return;
    font = face_font(self);
    if (font)
        draw_run(self, font, x, y, text.data, text.length);
    text_unicode_release(&text);
}
