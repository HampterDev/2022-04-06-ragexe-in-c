#include "platform/code_page.h"
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include <string.h>
#include <stdlib.h>

#include "render/textdc.h"
#include "core/cstr.h"

#define NEXT_CHAR(p) CharNextExA((WORD)g_codePage, (p), 0)

static void destroy_string(void *value) { string_free(value); }

static void push_line(struct List *lines, const struct String *line)
{
    struct String copy;

    string_copy(&copy, line);
    list_push_back(lines, &copy, sizeof copy);
}

static int char_width(struct UIWindow *self, const char *p, const char *next, int face, int height, int arg)
{
    struct String ch;
    const char *s;
    int w = 0, n;

    string_init(&ch);
    if (p != next)
        string_assign(&ch, p, next - p);
    s = string_cstr(&ch);
    if (s && (n = (int)strlen(s)) != 0) {
        struct TextDC dc;
        SIZE size;

        text_dc_construct(&dc, self->canvas);
        text_dc_select_font(&dc, face, height, arg, 0);
        text_dc_text_extent(&dc, s, n, &size);
        w = size.cx;
        text_dc_destruct(&dc);
    }
    string_free(&ch);
    return w;
}

struct List *ui_window_wrap_text_plain(struct UIWindow *self, struct List *out,
                                                  struct String text, struct List *lines, int width,
                                                  int font_face, int font_height, int font_arg)
{
    struct String line;
    const char *p, *end;
    int cur = 0;

    list_clear(lines, destroy_string);
    list_init(out);
    string_init(&line);
    p = string_cstr(&text);
    end = p + strlen(p);
    while (p != end) {
        const char *next;
        int w;

        if (*p == '\n') {
            push_line(lines, &line);
            string_clear(&line);
            list_push_back(out, &cur, sizeof cur);
            p++;
            cur = 0;
            continue;
        }
        if (p[1] == '\\' && p[2] == 'n') {
            push_line(lines, &line);
            string_clear(&line);
            list_push_back(out, &cur, sizeof cur);
            cur = 0;
            p += 2;
            continue;
        }
        next = NEXT_CHAR(p);
        w = char_width(self, p, next, font_face, font_height, font_arg);
        if (cur + w > width) {
            push_line(lines, &line);
            string_clear(&line);
            list_push_back(out, &cur, sizeof cur);
            cur = 0;
        }
        cur += w;
        string_append(&line, p, next - p);
        p = next;
        if (p == end)
            push_line(lines, &line);
    }
    string_free(&line);
    string_free(&text);
    return out;
}

static struct String *str_char_plus(struct String *out, char c, const struct String *s)
{
    string_init(out);
    string_reserve(out, s->size + 1);
    string_append_char(out, c);
    string_append(out, string_cstr(s), s->size);
    return out;
}

static void restart_line(struct String *line, const struct String *colour)
{
    struct String tmp;

    if (colour->size == 0) {
        string_clear(line);
        return;
    }
    str_char_plus(&tmp, '^', colour);
    string_free(line);
    *line = tmp;
}

struct List *ui_window_wrap_text(struct UIWindow *self, struct List *out,
                                             struct String text, struct List *lines, int width,
                                             int font_face, int font_height, BYTE bold)
{
    struct String line, colour;
    const char *p, *end;
    BYTE cur_bold;
    int cur = 0;

    list_clear(lines, destroy_string);
    if (width <= 0) {
        struct String copy;
        const char *s = string_cstr(&text);

        string_init(&copy);
        string_assign_cstr(&copy, s);
        width = ui_window_markup_text_width(self, copy, font_face, font_height, bold);
    }
    list_init(out);
    string_init(&line);
    string_init(&colour);
    cur_bold = bold;
    p = string_cstr(&text);
    end = p + strlen(p);
    while (p != end) {
        const char *next;
        int w;

        if (*p == '\n') {
            push_line(lines, &line);
            restart_line(&line, &colour);
            list_push_back(out, &cur, sizeof cur);
            cur = 0;
            p++;
            continue;
        }
        if (p[1] == '\\' && p[2] == 'n') {
            push_line(lines, &line);
            string_clear(&line);
            list_push_back(out, &cur, sizeof cur);
            cur = 0;
            p += 2;
            continue;
        }
        if (*p == '^') {
            char digits[7];
            char *stop = NULL;

            if (p[1] == 'b') {
                cur_bold = 1;
                string_append(&line, p, 2);
                p += 2;
                if (p == end)
                    push_line(lines, &line);
                continue;
            }
            if (p[1] == '/' && p[2] == 'b') {
                cur_bold = bold;
                string_append(&line, p, (size_t)(p + 3 - p));
                p += 3;
                if (p == end)
                    push_line(lines, &line);
                continue;
            }
            memset(digits, 0, sizeof digits);
            cstr_copy_n(digits, 7, p + 1, 6);
            strtol(digits, &stop, 16);
            if (stop - digits == 6) {
                string_assign_cstr(&colour, digits);
                string_append(&line, p, 7);
                p += 7;
                if (p == end)
                    push_line(lines, &line);
                continue;
            }
        }
        next = NEXT_CHAR(p);
        w = char_width(self, p, next, font_face, font_height, cur_bold);
        if (cur + w > width) {
            push_line(lines, &line);
            restart_line(&line, &colour);
            list_push_back(out, &cur, sizeof cur);
            cur = 0;
        }
        cur += w;
        string_append(&line, p, next - p);
        p = next;
        if (p == end)
            push_line(lines, &line);
    }
    string_free(&colour);
    string_free(&line);
    string_free(&text);
    return out;
}
