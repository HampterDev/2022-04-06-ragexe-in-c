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
#include "core/layout.h"

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
    self->charset = (lang == 0 && face >= 0x14) ? g_3dDevice.faceCharsets[0] : g_3dDevice.faceCharsets[face];
    self->bold = (BYTE)c;
    self->italic = (BYTE)d;
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

void text_dc_draw_in_charset(struct TextDC *self, int x, int y, const char *s, int len, int charset)
{
    struct TextUnicode text;
    FontHandle font;
    if (!s || len <= 0 || !text_unicode_init(&text, s, len, get_code_page_from_charset(charset)))
        return;
    font = font_cache_get_font(&g_fontCache, self->face, self->size, charset, self->bold, self->italic);
    if (font && text_dc_use_font(self, font))
        text_dc_draw_run(self, x, y, text.data, text.length);
    text_unicode_release(&text);
}

void text_dc_text_out(struct TextDC *self, int x, int y, const char *s, int len)
{
    struct TextRuns runs = {s, len > 0 && s ? (size_t)len : 0, self->charset, g_useCharsetTag};
    struct TextRun run;
    while (next_text_run(&runs, &run)) {
        SIZE extent;
        text_dc_draw_in_charset(self, x, y, run.text, run.length, run.charset);
        if (runs.remaining) {
            text_dc_measure_in_charset(self, &extent, run.text, run.length, run.charset);
            x += extent.cx;
        }
    }
}
