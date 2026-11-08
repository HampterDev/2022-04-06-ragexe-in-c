#include "render/cdc.h"
#include "render/device.h"
#include "core/text_format.h"
#include "core/hex.h"
#include "platform/code_page.h"
#include "platform/language.h"
#include "platform/language_api.h"
#include "render/text_unicode.h"
#include "render/textdc.h"
#include "render/fonts.h"
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
    int lang = g_languageIndex, face = a, size = b;

    if (a == 0 && (unsigned)(lang - 1) <= 10) {
        switch (lang) {
        case 1:
            if (g_serviceType == 6) {
                face = 8;
                size = b + 2;
                break;
            }
            size = b + 3;
            if (g_serviceType == 0xe) {
                face = 9;
                break;
            }
            face = 1;
            if (g_language && g_language->folder_lang == 0x1f)
                face = 6;
            break;
        case 2:
            face = 2;
            break;
        case 3:
            face = 3;
            break;
        case 4:
            face = 4;
            break;
        case 5:
            face = 5;
            size = b + 3;
            break;
        case 6:
            face = 7;
            size = b + 3;
            if (g_serviceType == 0xc) {
                size -= 2;
            } else if (g_serviceType == 0x12) {
                if (g_language->folder_lang == 0x1f)
                    face = 6;
                else
                    size -= 2;
            }
            break;
        case 7:
            face = 0xa;
            size = b + 3;
            break;
        case 11:
            face = 0xb;
            size = b + 3;
            break;
        default:
            break;
        }
    }
    self->face = face;
    self->size = size;
    if (lang == 0 && face >= FONT_FACE_EOT_FIRST)
        self->charset = g_3dDevice.faceCharsets[0];
    else if (face >= 0 && (size_t)face < sizeof g_3dDevice.faceCharsets / sizeof g_3dDevice.faceCharsets[0])
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

struct TextRun {
    const char *text;
    int length, charset;
};
struct TextRuns {
    const char *next;
    size_t remaining;
    int charset, tags;
};

static int next_text_run(struct TextRuns *runs, struct TextRun *run)
{
    while (runs->remaining) {
        size_t length = text_find_charset_tag(runs->next, runs->remaining, runs->tags);
        if (length) {
            run->text = runs->next;
            run->length = (int)length;
            run->charset = runs->charset;
            runs->next += length;
            runs->remaining -= length;
            return 1;
        }
        runs->charset = hex_digit_value((unsigned char)runs->next[1]) * 16
                      + hex_digit_value((unsigned char)runs->next[2]);
        runs->next += 3;
        runs->remaining -= 3;
    }
    return 0;
}

void text_dc_text_extent(struct TextDC *self, const char *s, int len, SIZE *out)
{
    struct TextRuns runs = {s, len > 0 && s ? (size_t)len : 0, self->charset, g_useCharsetTag};
    struct TextRun run;
    out->cx = out->cy = 0;
    while (next_text_run(&runs, &run)) {
        SIZE part;
        text_dc_measure_in_charset(self, &part, run.text, run.length, run.charset);
        out->cx += part.cx;
        if (part.cy > out->cy) out->cy = part.cy;
    }
}

static void textdc_text_out(struct TextDC *self, int x, int y, const char *s, int len, int charset)
{
    struct TextUnicode text;
    HFONT font;
    HGDIOBJ old;
    if (!s || len <= 0 || !text_unicode_init(&text, s, len, get_code_page_from_charset(charset)))
        return;
    font = font_cache_get_font(&g_fontCache, self->face, self->size, charset, self->bold, self->italic);
    old = font ? SelectObject(self->hdc, font) : NULL;
    if (old && old != HGDI_ERROR)
        TextOutW(self->hdc, x, y, text.data, text.length);
    text_unicode_release(&text);
}

void text_dc_text_out(struct TextDC *self, int x, int y, const char *s, int len)
{
    struct TextRuns runs = {s, len > 0 && s ? (size_t)len : 0, self->charset, g_useCharsetTag};
    struct TextRun run;
    while (next_text_run(&runs, &run)) {
        SIZE extent;
        textdc_text_out(self, x, y, run.text, run.length, run.charset);
        if (runs.remaining) {
            text_dc_measure_in_charset(self, &extent, run.text, run.length, run.charset);
            x += extent.cx;
        }
    }
}
