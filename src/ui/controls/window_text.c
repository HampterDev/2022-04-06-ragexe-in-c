#include "core/text_format.h"
#include "core/utf8.h"
#include "platform/code_page.h"
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include <string.h>
#include <stdlib.h>

#include "render/textdc.h"
#include "ui/ui_std.h"

#define TEXT_BK_ON(w) ((w)->text_bk_on)
#define TEXT_BK_COLOUR(w) ((COLORREF)(w)->text_bk_colour)

static void dc_open(struct TextDC *dc, struct UIWindow *self)
{
    text_dc_construct(dc, self->canvas);
}

void ui_window_text_out(struct UIWindow *self, int x, int y, const char *text, int len,
                                 int font_face, int font_height, COLORREF colour, int font_arg3, int font_arg4)
{
    struct TextDC dc;

    if (!len)
        len = (int)strlen(text);
    dc_open(&dc, self);
    text_dc_select_font(&dc, font_face, font_height, font_arg3, font_arg4);
    text_dc_set_text_color(&dc, colour);
    text_dc_text_out(&dc, x, y, text, len);
    text_dc_destruct(&dc);
}

void ui_window_text_out_down(struct UIWindow *self, int x, int y, const char *text,
                                     int len, int font_face, int font_height, COLORREF colour)
{
    struct TextDC dc;
    const char *end, *p, *next;
    int widest = 0;
    SIZE size;

    if (!len)
        len = (int)strlen(text);
    end = text + len;
    dc_open(&dc, self);
    text_dc_select_font(&dc, font_face, font_height, 0, 0);
    text_dc_set_text_color(&dc, colour);
    for (p = text; p < end; p = next) {
        next = utf8_step(p, end);
        text_dc_text_extent(&dc, p, (int)(next - p), &size);
        if (size.cx > widest)
            widest = size.cx;
    }
    for (p = text; p < end; p = next) {
        next = utf8_step(p, end);
        text_dc_text_extent(&dc, p, (int)(next - p), &size);
        text_dc_text_out(&dc, x + (widest - size.cx) / 2, y, p, (int)(next - p));
        y += size.cy;
    }
    text_dc_destruct(&dc);
}

void ui_window_text_out_centred(struct UIWindow *self, int x, int y, const char *text,
                                        int width, int font_face, int font_height, COLORREF colour, int font_arg)
{
    struct TextDC dc;
    SIZE size;

    dc_open(&dc, self);
    text_dc_select_font(&dc, font_face, font_height, font_arg, 0);
    text_dc_set_text_color(&dc, colour);
    if (!width) {
        text_dc_text_out(&dc, x, y, text, (int)strlen(text));
    } else {
        text_dc_text_extent(&dc, text, (int)strlen(text), &size);
        text_dc_text_out(&dc, (int)((double)(width - size.cx) * 0.5 + (double)x), y, text, (int)strlen(text));
    }
    text_dc_destruct(&dc);
}

void ui_window_text_out_outlined(struct UIWindow *self, int x, int y, const char *text,
                                         int len, COLORREF colour, COLORREF outline, int font_face, int font_height,
                                         int font_arg3, int font_arg4)
{
    struct TextDC dc;

    if (!len)
        len = (int)strlen(text);
    dc_open(&dc, self);
    text_dc_select_font(&dc, font_face, font_height, font_arg3, font_arg4);
    if (TEXT_BK_ON(self)) {
        text_dc_set_bk_color(&dc, TEXT_BK_COLOUR(self));
        text_dc_set_bk_mode(&dc, OPAQUE);
    }
    text_dc_set_text_color(&dc, outline);
    text_dc_text_out(&dc, x - 1, y, text, len);
    text_dc_text_out(&dc, x + 1, y, text, len);
    text_dc_text_out(&dc, x, y - 1, text, len);
    text_dc_text_out(&dc, x, y + 1, text, len);
    text_dc_set_text_color(&dc, colour);
    text_dc_text_out(&dc, x, y, text, len);
    if (TEXT_BK_ON(self))
        text_dc_set_bk_mode(&dc, TRANSPARENT);
    text_dc_destruct(&dc);
}

enum { RUNS_OUTLINE, RUNS_SHADOW };

static void draw_runs(struct UIWindow *self, struct TextDC *dc, int x, int y, const char *text, const char *end,
                      COLORREF *colour, int under, COLORREF outline, BYTE shadow)
{
    const char *p = text;

    while (p < end) {
        const char *from = text_read_colour_code(p, end, colour);
        const char *to = text_find_colour_code(from, end);
        struct String run;
        SIZE size;

        string_init(&run);
        if (from != to)
            string_assign(&run, from, to - from);
        if (to != from) {
            if (under == RUNS_OUTLINE) {
                text_dc_set_text_color(dc, outline);
                text_dc_text_out(dc, x - 1, y, string_cstr(&run), run.size);
                text_dc_text_out(dc, x + 1, y, string_cstr(&run), run.size);
                text_dc_text_out(dc, x, y - 1, string_cstr(&run), run.size);
                text_dc_text_out(dc, x, y + 1, string_cstr(&run), run.size);
            } else if (shadow) {
                text_dc_set_text_color(dc, 0);
                text_dc_text_out(dc, x + 1, y + 1, string_cstr(&run), run.size);
            }
            text_dc_set_text_color(dc, *colour);
            text_dc_text_out(dc, x, y, string_cstr(&run), run.size);
            text_dc_text_extent(dc, from, (int)(to - from), &size);
            x += size.cx;
        }
        string_free(&run);
        p = to;
    }
}

void ui_window_text_out_outlined_ptr(struct UIWindow *self, int x, int y, const char *text,
                                            int len, COLORREF *colour, COLORREF outline, int font_face,
                                            int font_height, int font_arg)
{
    struct TextDC dc;
    const char *end;

    if (!len)
        len = (int)strlen(text);
    end = text + len;
    dc_open(&dc, self);
    text_dc_select_font(&dc, font_face, font_height, font_arg, 0);
    draw_runs(self, &dc, x, y, text, end, colour, RUNS_OUTLINE, outline, 0);
    text_dc_destruct(&dc);
}

static void draw_runs_bk(struct UIWindow *self, struct TextDC *dc, int x, int y, const char *text,
                         const char *end, COLORREF *colour, BYTE shadow)
{
    if (TEXT_BK_ON(self)) {
        text_dc_set_bk_color(dc, TEXT_BK_COLOUR(self));
        text_dc_set_bk_mode(dc, OPAQUE);
    }
    draw_runs(self, dc, x, y, text, end, colour, RUNS_SHADOW, 0, shadow);
    if (TEXT_BK_ON(self))
        text_dc_set_bk_mode(dc, TRANSPARENT);
}

void ui_window_text_out_runs(struct UIWindow *self, int x, int y, const char *text, int len,
                           COLORREF *colour, int font_face, int font_height, BYTE shadow)
{
    struct TextDC dc;
    const char *end;

    if (!len)
        len = (int)strlen(text);
    end = text + len;
    dc_open(&dc, self);
    text_dc_select_font(&dc, font_face, font_height, 0, 0);
    draw_runs_bk(self, &dc, x, y, text, end, colour, shadow);
    text_dc_destruct(&dc);
}

static BOOL extent(struct UIWindow *self, const char *text, int len, int font_face, int font_height, int font_arg3,
                   int font_arg4, SIZE *size)
{
    struct TextDC dc;

    if (!text)
        return FALSE;
    if (!len) {
        len = (int)strlen(text);
        if (!len)
            return FALSE;
    }
    dc_open(&dc, self);
    text_dc_select_font(&dc, font_face, font_height, font_arg3, font_arg4);
    text_dc_text_extent(&dc, text, len, size);
    text_dc_destruct(&dc);
    return TRUE;
}

int ui_window_text_width(struct UIWindow *self, const char *text, int len, int font_face,
                                  int font_height, int font_arg3, int font_arg4)
{
    SIZE size;

    if (!extent(self, text, len, font_face, font_height, font_arg3, font_arg4, &size))
        return 0;
    return size.cx;
}

int ui_window_text_height(struct UIWindow *self, const char *text, int len,
                                   int font_face, int font_height, int font_arg3, int font_arg4)
{
    SIZE size;

    if (!extent(self, text, len, font_face, font_height, font_arg3, font_arg4, &size))
        return 0;
    return size.cy;
}

SIZE *ui_window_text_extent(struct UIWindow *self, SIZE *out, const char *text, int len,
                                     int font_face, int font_height, int font_arg3, int font_arg4)
{
    SIZE size;

    size.cx = 0;
    size.cy = 0;
    extent(self, text, len, font_face, font_height, font_arg3, font_arg4, &size);
    *out = size;
    return out;
}

int ui_window_vertical_text_length(struct UIWindow *self, const char *text, int len,
                                           int font_face, int font_height, int font_arg3)
{
    struct TextDC dc;
    const char *end, *p, *next;
    int total = 0;
    SIZE size;

    if (!text)
        return 0;
    if (!len) {
        len = (int)strlen(text);
        if (!len)
            return 0;
    }
    end = text + len;
    p = text;
    dc_open(&dc, self);
    text_dc_select_font(&dc, font_face, font_height, font_arg3, 0);
    for (; p < end; p = next) {
        next = utf8_step(p, end);
        text_dc_text_extent(&dc, p, (int)(next - p), &size);
        total += size.cy;
    }
    text_dc_destruct(&dc);
    return total;
}

int ui_window_markup_text_width(struct UIWindow *self, struct String text, int font_face,
                                        int font_height, BYTE bold)
{
    const char *p = string_cstr(&text);
    const char *end = p + strlen(p);
    BYTE cur = bold;
    int width = 0;

    while (p != end) {
        const char *next;
        struct String ch;
        const char *s;
        int n, w;

        if (*p == '^') {
            char digits[7];
            char *stop = NULL;

            if (p[1] == 'b') {
                cur = 1;
                p += 2;
                continue;
            }
            if (p[1] == '/' && p[2] == 'b') {
                cur = bold;
                p += 3;
                continue;
            }
            memset(digits, 0, sizeof digits);
            strncpy_s(digits, 7, p + 1, 6);
            strtol(digits, &stop, 16);
            if (stop - digits == 6) {
                p += 7;
                continue;
            }
        } else if (p[1] == '\\' && p[2] == 'n') {
            p += 2;
            continue;
        }
        next = utf8_step(p, end);
        string_init(&ch);
        if (p != next)
            string_assign(&ch, p, next - p);
        s = string_cstr(&ch);
        w = 0;
        if (s && (n = (int)strlen(s)) != 0) {
            struct TextDC dc;
            SIZE size;

            dc_open(&dc, self);
            text_dc_select_font(&dc, font_face, font_height, cur, 0);
            text_dc_text_extent(&dc, s, n, &size);
            w = size.cx;
            text_dc_destruct(&dc);
        }
        string_free(&ch);
        width += w;
        p = next;
    }
    string_free(&text);
    return width;
}

static void destroy_string(void *value) { string_free(value); }

int ui_window_text_out_wrapped(struct UIWindow *self, int x, int y,
                                       const struct String *text, int wrap_width, int line_gap, int align,
                                       int face, int height, int arg9, BYTE arg10)
{
    struct List lines;
    struct List widths;
    struct String copy;
    struct ListNode *node;
    int off = 0;

    list_init(&lines);
    string_copy(&copy, text);
    ui_window_wrap_text(self, &widths, copy, &lines, wrap_width, face, height, arg10);
    list_free(&widths, NULL);
    if (!wrap_width) {
        string_copy(&copy, text);
        wrap_width = ui_window_markup_text_width(self, copy, 0, 12, 0);
    }
    node = lines.head->next;
    if (node != lines.head) {
        for (;;) {
            ui_window_text_out_line(self, x, y + off, LIST_VALUE(node, struct String), wrap_width, align, face,
                                    height, arg9, arg10);
            off += height;
            node = node->next;
            if (node == lines.head)
                break;
            off += line_gap;
        }
    }
    list_free(&lines, destroy_string);
    return off;
}

void ui_window_text_out_price(struct UIWindow *self, int value, struct String text,
                                      int x, int y)
{
    struct TextDC dc;
    const char *s = string_cstr(&text);
    int len = (int)strlen(s);
    COLORREF first = 0, second = 0;
    COLORREF single = 0;
    BOOL one = FALSE;

    if (value >= 1000000000) {
        first = 0x7f00ff;
        second = 0xff;
    } else if (value >= 100000000) {
        first = 0x64c8c8;
    } else if (value >= 10000000) {
        one = TRUE;
        single = 0xff;
    } else if (value >= 1000000) {
        first = 0xff00;
    } else if (value >= 100000) {
        one = TRUE;
        single = 0xff0000;
    } else if (value >= 10000) {
        one = TRUE;
        single = 0xff1eff;
    } else if (value >= 1000) {
        first = 0xffff;
        second = 0xff;
    } else if (value >= 100) {
        first = 0xffff00;
        second = 0xff0000;
    } else if (value >= 10) {
        first = 0xc800c8;
        second = 0xff0000;
    } else {
        first = 0xffff00;
    }
    dc_open(&dc, self);
    text_dc_select_font(&dc, 0, 12, 0, 0);
    if (one) {
        text_dc_set_text_color(&dc, single);
        text_dc_text_out(&dc, x, y, s, len);
    } else {
        text_dc_set_text_color(&dc, first);
        text_dc_text_out(&dc, x, y, s, len);
        text_dc_set_text_color(&dc, second);
        text_dc_text_out(&dc, x - 1, y, s, len);
    }
    text_dc_destruct(&dc);
    string_free(&text);
}

struct AmountColours *ui_window_amount_colours(struct UIWindow *self,
                                                        struct AmountColours *out, long long amount)
{
    (void)self;
    if (amount >= 1000000000) {
        out->colour = 0x7f00ff;
        out->double_colour = 0xff;
    } else if (amount >= 100000000) {
        out->colour = 0x64c8c8;
        out->double_colour = 0;
    } else if (amount >= 10000000) {
        out->colour = 0xff;
        out->double_colour = -1;
    } else if (amount >= 1000000) {
        out->colour = 0xff00;
        out->double_colour = 0;
    } else if (amount >= 100000) {
        out->colour = 0xff0000;
        out->double_colour = -1;
    } else if (amount >= 10000) {
        out->colour = 0xff1eff;
        out->double_colour = -1;
    } else if (amount >= 1000) {
        out->colour = 0xffff;
        out->double_colour = 0xff;
    } else if (amount >= 100) {
        out->colour = 0xffff00;
        out->double_colour = 0xff0000;
    } else if (amount >= 10) {
        out->colour = 0xc800c8;
        out->double_colour = 0xff0000;
    } else {
        out->colour = 0xffff00;
        out->double_colour = 0;
    }
    return out;
}

int ui_window_get_color_text_width(struct UIWindow *self, const char *text, int len,
                                          int font_face, int font_height, int font_arg3)
{
    COLORREF colour;
    int width;

    if (!text)
        return 0;
    if (!len) {
        len = (int)strlen(text);
        if (!len)
            return 0;
    }
    width = ui_window_text_width(self, text, len, font_face, font_height, font_arg3, 0);
    colour = 0;
    if (text_read_colour_code(text_find_colour_code(text, text + len), text + len, &colour) &&
        colour) {
        struct TextDC dc;
        SIZE size;

        dc_open(&dc, self);
        text_dc_select_font(&dc, 0, 12, 0, 0);
        text_dc_text_extent(&dc, "^000000", 7, &size);
        text_dc_destruct(&dc);
        width += (3 - size.cx) * 2;
    }
    return width;
}
