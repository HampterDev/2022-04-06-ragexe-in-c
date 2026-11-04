#include "platform/code_page.h"
#include "core/utf8.h"
#include "ui/draw.h"
#include "render/textdc.h"
#include <string.h>
#include <stdlib.h>
#include "core/cstr.h"

static struct TextDC *dc_open(struct TextDC *dc, struct UIWindow *self, int face, int height, int arg3)
{
    text_dc_construct(dc, self->canvas);
    text_dc_select_font(dc, face, height, arg3, 0);
    return dc;
}

static int char_width(struct UIWindow *self, const char *p, const char *next, int face, int height, int arg3)
{
    struct String ch;
    struct TextDC dc;
    const char *s;
    SIZE size;
    int width;

    string_init(&ch);
    if (p != next)
        string_assign(&ch, p, next - p);
    s = string_cstr(&ch);
    if (!s || strlen(s) == 0) {
        width = 0;
    } else {
        dc_open(&dc, self, face, height, arg3);
        text_dc_text_extent(&dc, s, (int)strlen(s), &size);
        width = size.cx;
        text_dc_destruct(&dc);
    }
    string_free(&ch);
    return width;
}

static void char_draw(struct UIWindow *self, int x, int y, const char *p, const char *next, int face, int height,
                      int arg3, COLORREF colour)
{
    struct String ch;
    struct TextDC dc;
    const char *s;

    string_init(&ch);
    if (p != next)
        string_assign(&ch, p, next - p);
    s = string_cstr(&ch);
    dc_open(&dc, self, face, height, arg3);
    text_dc_set_text_color(&dc, colour);
    text_dc_text_out(&dc, x, y, s, (int)strlen(s));
    text_dc_destruct(&dc);
    string_free(&ch);
}

static void outlined_draw(struct TextDC *dc, int x, int y, const char *text, int len, COLORREF colour,
                          COLORREF outline)
{
    text_dc_set_text_color(dc, outline);
    text_dc_text_out(dc, x - 1, y, text, len);
    text_dc_text_out(dc, x + 1, y, text, len);
    text_dc_text_out(dc, x, y - 1, text, len);
    text_dc_text_out(dc, x, y + 1, text, len);
    text_dc_set_text_color(dc, colour);
    text_dc_text_out(dc, x, y, text, len);
}

void ui_window_text_out_outlined_right(struct UIWindow *self, int x, int y,
                                              const char *text, int len, COLORREF colour, COLORREF outline,
                                              int face, int height, int arg3)
{
    struct TextDC dc;
    SIZE size;

    if (!len)
        len = (int)strlen(text);
    dc_open(&dc, self, face, height, arg3);
    text_dc_text_extent(&dc, text, (int)strlen(text), &size);
    outlined_draw(&dc, x - size.cx, y, text, len, colour, outline);
    text_dc_destruct(&dc);
}

void ui_window_draw_text_outlined_vertical(struct UIWindow *self, int x, int y, const char *text, int len,
                           COLORREF colour, COLORREF outline, int font_face, int font_height, int font_arg3)
{
    struct TextDC dc;
    SIZE size;

    if (!len)
        len = (int)strlen(text);
    dc_open(&dc, self, font_face, font_height, font_arg3);
    text_dc_text_extent(&dc, text, len, &size);
    outlined_draw(&dc, x + size.cy, y, text, len, colour, outline);
    text_dc_destruct(&dc);
}

void ui_window_text_out_right(struct UIWindow *self, int x, int y, const char *text,
                                      int len, int font_face, int font_height, COLORREF colour, int font_arg)
{
    struct TextDC dc;
    SIZE size;

    if (!len)
        len = (int)strlen(text);
    dc_open(&dc, self, font_face, font_height, font_arg);
    text_dc_text_extent(&dc, text, (int)strlen(text), &size);
    text_dc_set_text_color(&dc, colour);
    text_dc_text_out(&dc, x - size.cx, y, text, len);
    text_dc_destruct(&dc);
}

void ui_window_draw_shadow_text(struct UIWindow *self, int x, int y, const char *text,
                                        int len, COLORREF colour, COLORREF shadow, int font_face, int font_height,
                                        int font_arg)
{
    struct TextDC dc;

    if (!len)
        len = (int)strlen(text);
    dc_open(&dc, self, font_face, font_height, font_arg);
    text_dc_set_text_color(&dc, shadow);
    text_dc_text_out(&dc, x + 1, y + 1, text, len);
    text_dc_set_text_color(&dc, colour);
    text_dc_text_out(&dc, x, y, text, len);
    text_dc_destruct(&dc);
}

void ui_window_text_out_line(struct UIWindow *self, int x, int y,
                                     const struct String *text, int width, int align, int face, int height,
                                     COLORREF colour, BYTE bold)
{
    struct String copy;
    const char *p, *end, *next;
    int text_width, advance = 0, w;
    BYTE bold_now;
    char hex[7];
    char *hex_end;
    long value;

    string_copy(&copy, text);
    text_width = ui_window_markup_text_width(self, copy, face, height, bold);
    if (!width)
        width = text_width;
    switch (align) {
    case 2:
        x -= text_width;
        break;
    case 6:
        x = (int)((float)(width - text_width) * 0.5f + (float)x);
        break;
    }
    bold_now = bold;
    p = string_cstr(text);
    end = p + strlen(p);
    while (end != p) {
        if (*p == '^') {
            if (p[1] == 'b') {
                bold_now = 1;
                p += 2;
                continue;
            }
            if (p[1] == '/' && p[2] == 'b') {
                bold_now = bold;
                p += 3;
                continue;
            }
            memset(hex, 0, sizeof hex);
            cstr_copy_n(hex, 7, p + 1, 6);
            hex_end = NULL;
            value = strtol(hex, &hex_end, 16);
            if (hex_end - hex == 6) {
                colour = ((((DWORD)value & 0xff) << 8 | ((DWORD)value & 0xffff) >> 8) << 8)
                         | (((DWORD)value >> 16) & 0xff);
                p += 7;
                continue;
            }
        }
        next = utf8_step(p, end);
        w = char_width(self, p, next, face, height, bold_now);
        char_draw(self, x + advance, y, p, next, face, height, bold_now, colour);
        advance += w;
        if (advance > width)
            break;
        p = next;
    }
}

void ui_window_text_out_doubled(struct UIWindow *self, int x, int y, const char *text,
                                        COLORREF colour, COLORREF colour2, int len, int font_face, int font_height)
{
    struct TextDC dc;

    if (!len)
        len = (int)strlen(text);
    dc_open(&dc, self, font_face, font_height, 0);
    text_dc_set_text_color(&dc, colour);
    text_dc_text_out(&dc, x, y, text, len);
    text_dc_set_text_color(&dc, colour2);
    text_dc_text_out(&dc, x - 1, y, text, len);
    text_dc_destruct(&dc);
}
