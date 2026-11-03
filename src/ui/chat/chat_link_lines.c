#include <stdlib.h>
#include "core/memory.h"
#include "core/markup_tags.h"
#include "ui/chat/chat_history.h"
#include "ui/chat/markup_links.h"
#include "ui/ui_std.h"
#include <string.h>
#include <time.h>

#define NAVI_FONT_HEIGHT 0xc
#define NAVI_COLOUR 0x82f2e1
#define MODE_COLOURED 3
#define REWRAP_MAX_LINES 0x17

static int text_width(struct UIChatHisBox *self, const char *text)
{
    return ui_window_text_width(&self->list.base, text, 0, 0, NAVI_FONT_HEIGHT, 0, 0);
}

static void break_text(struct UIChatHisBox *self, const int *mode, struct TextArray *lines,
                       struct TextView text)
{
    if (*mode == MODE_COLOURED)
        ui_window_break_coloured_lines(&self->list.base, text.data, lines, NULL,
                                     self->list.base.width - 0x18);
    else
        ui_window_break_lines(&self->list.base, text.data, lines, self->list.base.width - 0x10);
}

static void assign(struct OwnedText *text, struct TextView value)
{
    if (!owned_text_assign(text, value)) abort();
}

static void append(struct OwnedText *text, struct TextView value)
{
    if (!owned_text_concat(text, owned_text_view(text), value)) abort();
}

static void cut(struct OwnedText *text, size_t start, size_t count)
{
    if (start > text->size) start = text->size;
    if (count > text->size - start) count = text->size - start;
    assign(text, text_view(owned_text_cstr(text) + start, count));
}

static int find(const struct OwnedText *text, char character)
{
    const char *begin = owned_text_cstr(text);
    const char *found = memchr(begin, character, text->size);
    return found ? (int)(found - begin) : -1;
}

static void swap(struct OwnedText *a, struct OwnedText *b)
{
    struct OwnedText value = *a;
    *a = *b;
    *b = value;
}

static void push(struct TextArray *lines, const struct OwnedText *line)
{
    if (!text_array_append(lines, owned_text_view(line))) abort();
}

static void rewrap(struct UIChatHisBox *self, struct TextArray *lines, const int *mode)
{
    struct OwnedText cur = {0};
    unsigned li;
    for (li = 0; li < lines->size; ++li) {
        struct TextArray sub = {0};
        unsigned n, j;
        int f, offset;
        if (lines->size > REWRAP_MAX_LINES) break;
        break_text(self, mode, &sub, text_array_at(lines, li));
        n = sub.size;
        if (n > 1) {
            for (j = 0; j < n; ++j) {
                struct OwnedText *piece = &sub.items[j];
                if (j == 0) assign(&lines->items[li], owned_text_view(piece));
                if (li + j < lines->size && j == n - 1) {
                    struct OwnedText *next = &lines->items[li + j];
                    append(piece, owned_text_view(next));
                    swap(next, piece);
                } else if (j != 0) {
                    struct TextArray rebuilt = {0};
                    unsigned k;
                    for (k = 0; k < lines->size; ++k)
                        if (lines->items[k].size) push(&rebuilt, &lines->items[k]);
                    if (piece->size) push(&rebuilt, piece);
                    for (k = li + 1; k < lines->size; ++k)
                        if (lines->items[k].size) push(&rebuilt, &lines->items[k]);
                    {
                        struct TextArray old = *lines;
                        *lines = rebuilt;
                        rebuilt = old;
                    }
                    text_array_clear(&rebuilt);
                }
            }
        }
        if (li < lines->size) {
            assign(&cur, text_array_at(lines, li));
            offset = 0;
            while ((f = find(&cur, '>')) != -1) {
                offset += f + 1;
                cut(&cur, f + 1, (size_t)-1);
            }
            f = find(&cur, '<');
            if (f != -1) {
                struct OwnedText rest = {0};
                offset += f;
                cut(&cur, f, (size_t)-1);
                assign(&rest, text_view(owned_text_cstr(&cur) + 1, cur.size - 1));
                while ((f = find(&rest, '<')) != -1) {
                    offset += f;
                    assign(&cur, owned_text_view(&rest));
                    assign(&rest, text_view(owned_text_cstr(&cur) + 1, cur.size - 1));
                }
                if (li + 1 < lines->size && find(&lines->items[li + 1], '>') != -1) {
                    append(&cur, text_array_at(lines, li + 1));
                    swap(&lines->items[li + 1], &cur);
                    cut(&lines->items[li], 0, (unsigned)offset);
                }
                owned_text_clear(&rest);
            }
        }
        text_array_clear(&sub);
    }
    owned_text_clear(&cur);
}

static struct String *build_navi_line(struct UIChatHisBox *self, struct String *out, const char *line,
                                         struct String *remain)
{
    struct Vec buttons = { 0 };
    struct String s, result, cur;
    int p;

    if (!strstr(line, "<") &&
        !strstr(line, ">")) {
        p = string_find(remain, line, 0);
        if (p != -1)
            string_substring(remain, remain, p + strlen(line), (size_t)-1);
        string_init(out);
        string_assign_cstr(out, line);
        return out;
    }
    string_copy(&s, remain);
    string_init(&result);
    string_init(&cur);
    for (;;) {
        struct String label, rest, whole;
        struct String readable = {0};
        struct String fit = {0};
        struct UINaviLinkButton *btn;
        void *value;
        int q, r;

        p = text_view_find(string_view(&s), string_view(&text_tag_NAVIL), 0);
        if (p == -1)
            break;
        string_clear(&result);
        string_append(&result, string_cstr(&s), (unsigned)p < s.size ? (unsigned)p : s.size);
        string_substring(remain, remain, p, (size_t)-1);
        string_init(&rest);
        string_substring(&rest, &s, p, (size_t)-1);
        string_free(&cur);
        string_move(&cur, &rest);
        q = text_view_find(string_view(&cur), string_view(&text_tag_NAVIL_END), 0);
        if (q == -1)
            break;
        string_truncate(&cur, q + text_tag_NAVIL_END.size);
        navi_links_readable_link(&readable, string_view(&cur), 0);
        string_init(&label);
        string_assign(&label, "<", 1);
        ui_window_fit_plain_text(&self->list.base, &fit, string_view(&readable), self->list.base.width / 2);
        string_append(&label, string_cstr(&fit), fit.size);
        string_free(&fit);
        string_append_cstr(&label, ">");
        string_init(&rest);
        string_init(&whole);
        string_assign_cstr(&whole, line);
        r = text_view_find(string_view(&whole), string_view(&label), 0);
        if (r != -1) {
            string_substring(&rest, &whole, r + label.size, (size_t)-1);
        }
        btn = ui_navi_link_button_create();
        if (btn) {
            string_assign(&btn->link, string_cstr(&cur), cur.size);
            ui_text_button_set_text(&btn->text.button.base, string_cstr(&label), NAVI_COLOUR, 0,
                                 NAVI_FONT_HEIGHT, 0, 0);
            if (readable.size > label.size - 2)
                ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)btn, string_cstr(&readable));
            btn->text.hover_colour = NAVI_COLOUR;
            btn->text.button.base.id = UI_ID_NAVI_LINK;
            ui_text_button_fit_width(&btn->text, text_width(self, string_cstr(&label)) + 1,
                                  btn->text.button.base.height);
            ui_window_add_child(&self->list.base, &btn->text.button.base);
            value = btn;
            vec_push(&buttons, &value, sizeof(void *));
            btn->line = (int)VEC_COUNT(&self->list.items, struct String);
            btn->line_x = text_width(self, string_cstr(&result)) - self->list.scroll_x;
        }
        string_append(&result, string_cstr(&label), label.size);
        string_append(&result, string_cstr(&rest), rest.size);
        string_substring(remain, remain, rest.size + label.size, (size_t)-1);
        string_assign(&s, string_cstr(remain), remain->size);
        string_free(&whole);
        string_free(&rest);
        string_free(&label);
        string_free(&readable);
    }
    if (buttons.first != buttons.last) {
        struct Vec copy;

        vec_copy(&copy, &buttons, sizeof(DWORD));
        list_push_back(&self->navi_links, &copy, sizeof copy);
    }
    *out = result;
    string_free(&cur);
    string_free(&s);
    vec_free(&buttons);
    return out;
}

void ui_chat_his_box_add_item_link_line(struct UIChatHisBox *self, const char *text, const COLORREF *colour,
                                             const int *mode)
{
    struct TextArray lines = {0};
    struct String readable = {0}, rest = {0};
    unsigned i;

    if (!strstr(text, string_cstr(&text_tag_ITEML)) ||
        !strstr(text, string_cstr(&text_tag_ITEML_END))) {
        ui_list_box_add_item(&self->list, text);
        return;
    }
    ui_chat_his_box_readable_item_links(&self->list.base, &readable, text_view_cstr(text));
    break_text(self, mode, &lines, string_view(&readable));
    rewrap(self, &lines, mode);
    string_assign_cstr(&rest, text);
    for (i = 0; i < lines.size; i++) {
        struct String built = {0};

        ui_chat_his_box_build_item_line(self, &built, text_array_at(&lines, i).data, &rest);
        assign(&lines.items[i], string_view(&built));
        string_free(&built);
        if (i) {
            struct String empty;

            vec_push(&self->colours, colour, sizeof(COLORREF));
            string_init(&empty);
            string_vec_push_move(&self->line_extra, &empty);
            string_free(&empty);
        }
        ui_list_box_add_item(&self->list, text_array_at(&lines, i).data);
    }
    string_free(&rest);
    string_free(&readable);
    text_array_clear(&lines);
}

void ui_chat_his_box_add_navi_link_line(struct UIChatHisBox *self, const char *text, const COLORREF *colour,
                                             const int *mode)
{
    struct TextArray lines = {0};
    struct String rest;
    struct String readable = {0};
    unsigned i;

    if (!navi_links_has_link(text_view_cstr(text))) {
        ui_list_box_add_item(&self->list, text);
        return;
    }
    navi_links_readable_links(&readable, text_view_cstr(text));
    break_text(self, mode, &lines, string_view(&readable));
    rewrap(self, &lines, mode);
    string_init(&rest);
    string_assign_cstr(&rest, text);
    for (i = 0; i < lines.size; i++) {
        struct String built;

        build_navi_line(self, &built, text_array_at(&lines, i).data, &rest);
        assign(&lines.items[i], string_view(&built));
        string_free(&built);
        if (i) {
            vec_push(&self->colours, colour, sizeof(COLORREF));
        }
        ui_list_box_add_item(&self->list, text_array_at(&lines, i).data);
    }
    string_free(&rest);
    string_free(&readable);
    text_array_clear(&lines);
}
