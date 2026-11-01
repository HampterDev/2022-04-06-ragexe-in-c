#include "ui/controls/ui_string.h"
#include "ui/controls/controls_std.h"
#include "ui/controls/text_viewer.h"
#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "diag/perf.h"

struct UITextViewEX {
    struct UITextViewer viewer;
    struct Vec words;
    struct Vec rows;
};

const struct UIListBoxOps ui_text_view_ex_ops = {
    .base = {
        .destroy = ui_text_view_ex_destroy,
        .resize = ui_list_box_resize,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_window_store_info_base,
        .save_pos = ui_window_save_pos_base,
        .move_delta = ui_window_move_delta_base,
        .set_show = ui_window_set_show_base,
        .on_create = ui_text_view_ex_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_text_view_ex_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_text_viewer_ignore_click,
        .on_l_btn_dbl_clk = ui_text_viewer_ignore_click,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_text_viewer_ignore_click,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_text_view_ex_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .create = ui_text_view_ex_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_text_view_ex_clear,
    .update_scroll_bar = ui_text_view_ex_update_scroll_bar,
};

#define ROW_HEIGHT 0x15
#define WORD_GAP 5
#define TOP_MARGIN 7
#define RIGHT_MARGIN 0xf
#define BLANKS " \t"
#define WORD_BREAKS " \t\n"
#define END_TAG "<\\end>"
#define WORD_SIZE 100

#define WORD_COUNT(v) VEC_COUNT(v, struct UIString *)
#define ROW_COUNT(tv) VEC_COUNT(&(tv)->rows, struct Vec)

static void rows_clear(struct Vec *rows)
{
    struct Vec *row;

    for (row = rows->first; row != rows->last; row++)
        vec_free(row);
    vec_clear(rows);
}

static void text_view_ex_destruct(struct UITextViewEX *self)
{
    self->viewer.list.base.ops = &ui_text_view_ex_ops.base;
    rows_clear(&self->rows);
    vec_free(&self->rows);
    vec_free(&self->words);
    self->viewer.list.base.ops = &ui_text_viewer_ops.base;
    vec_free(&self->viewer.embeds);
    ui_list_box_destruct(&self->viewer.list);
}

void ui_text_view_ex_destroy(struct UIWindow *window)
{
    struct UITextViewEX *self = (struct UITextViewEX *)window;
    text_view_ex_destruct(self);
    mem_free(self);
}

int ui_text_view_ex_add_words(struct UITextViewEX *self, struct TextView input, int joined, int r, int g, int b)
{
    struct String text, word;
    size_t pos = 0, brk;

    string_init(&text);
    string_assign(&text, input.data, input.size);
    string_trim_left(&text, BLANKS);
    string_trim_right(&text, BLANKS);
    for (;;) {
        struct UIString *w;

        brk = string_find_first_of(&text, WORD_BREAKS, pos);
        string_init(&word);
        string_substring(&word, &text, pos, brk - pos);
        if (pos == brk) {
            if (string_cstr(&text)[pos] != '\n')
                goto next;
            string_assign_cstr(&word, "\n");
        } else if (string_equal_cstr(&word, "\r")) {
            goto next;
        }
        if (pos >= text.size)
            break;
        w = ui_string_create(WORD_SIZE, WORD_SIZE, string_view(&word));
        ui_window_set_show_base(&w->base, 0);
        w->joined = joined;
        w->r = r;
        w->g = g;
        w->b = b;
        joined = 0;
        ui_window_add_child(&self->viewer.list.base, &w->base);
        vec_push(&self->words, &w, sizeof w);
        if (brk == (size_t)-1)
            break;
next:
        pos = brk + 1;
        string_free(&word);
    }
    string_free(&word);
    string_free(&text);
    return joined;
}

static int add_coloured_run(struct UITextViewEX *self, struct String *run, int joined)
{
    struct String hex, words, rs, gs, bs;
    char *stop;
    int r, g, b;

    string_init(&hex);
    string_substring(&hex, run, 1, 6);
    string_init(&words);
    string_substring(&words, run, 7, run->size - 14);
    string_init(&rs);
    string_substring(&rs, &hex, 0, 2);
    string_init(&gs);
    string_substring(&gs, &hex, 2, 2);
    string_init(&bs);
    string_substring(&bs, &hex, 4, 2);
    r = strtol(string_cstr(&rs), &stop, 16);
    g = strtol(string_cstr(&gs), &stop, 16);
    b = strtol(string_cstr(&bs), &stop, 16);
    joined = ui_text_view_ex_add_words(self, string_view(&words), joined, r, g, b);
    string_free(&bs);
    string_free(&gs);
    string_free(&rs);
    string_free(&words);
    string_free(&hex);
    return joined;
}

void ui_text_view_ex_set_text(struct UITextViewEX *self, struct TextView input)
{
    struct String text, line, run, rest;
    size_t pos = 0, lt, p, c, next, end;
    int joined;

    string_init(&text);
    string_assign(&text, input.data, input.size);
    for (;;) {
        struct UIString *w;

        lt = string_find_first_of(&text, "<", pos);
        string_init(&line);
        string_substring(&line, &text, pos, lt - pos);
        joined = string_find_first_not_of(&line, BLANKS, 0) == 0;
        string_trim_left(&line, BLANKS);
        string_trim_right(&line, BLANKS);

        p = 0;
        for (;;) {
            c = string_find_first_of(&line, "^", p);
            if (c == p) {
                next = string_find_first_of(&line, "^", c + 1);
                string_init(&run);
                string_substring(&run, &line, c, next - c + 7);
                if (string_cstr(&run)[0] == '^')
                    joined = add_coloured_run(self, &run, joined);
                p = next + 7;
                string_free(&run);
                continue;
            }
            if (c == (size_t)-1) {
                string_init(&rest);
                string_substring(&rest, &line, p, ~p);
                ui_text_view_ex_add_words(self, string_view(&rest), joined, 0, 0, 0);
                string_free(&rest);
                break;
            }
            string_init(&rest);
            string_substring(&rest, &line, p, c - p);
            joined = ui_text_view_ex_add_words(self, string_view(&rest), joined, 0, 0, 0);
            string_free(&rest);
            next = string_find_first_of(&line, "^", c + 1);
            string_init(&run);
            string_substring(&run, &line, c, next - c + 7);
            if (string_cstr(&run)[0] == '^')
                joined = add_coloured_run(self, &run, joined);
            p = next + 7;
            string_free(&run);
            if (p >= line.size)
                break;
        }

        if (lt == (size_t)-1)
            break;
        end = text_view_find(string_view(&text), text_view(END_TAG, 6), lt);
        string_init(&run);
        string_substring(&run, &text, lt, end - lt);
        string_free(&line);
        string_move(&line, &run);
        w = ui_string_create(WORD_SIZE, WORD_SIZE, string_view(&line));
        ui_window_set_show_base(&w->base, 0);
        ui_window_add_child(&self->viewer.list.base, &w->base);
        vec_push(&self->words, &w, sizeof w);
        pos = end + 6;
        string_free(&line);
    }
    string_free(&line);
    ui_text_view_ex_break_rows(self);
    string_free(&text);
}

void ui_text_view_ex_add_row(struct UITextViewEX *self, struct UIString **first,
                                    struct UIString **last)
{
    struct Vec row = { NULL, NULL, NULL }, copy;

    for (; first != last; first++)
        vec_push(&row, first, sizeof *first);
    vec_copy(&copy, &row, sizeof(struct UIString *));
    vec_push(&self->rows, &copy, sizeof copy);
    vec_free(&row);
}

void ui_text_view_ex_break_rows(struct UITextViewEX *self)
{
    struct UIString **it = self->words.first, **last = self->words.last, **row = it;
    int x = 0;

    for (; it != last; it++) {
        struct UIString *w = *it;

        x += WORD_GAP + w->base.width;
        if (x + RIGHT_MARGIN > self->viewer.list.base.width
            || string_equal_cstr(&w->text, "\n")) {
            ui_text_view_ex_add_row(self, row, it);
            row = it;
            x = string_equal_cstr(&w->text, "\n") ? 0 : w->base.width + WORD_GAP;
        }
    }
    ui_text_view_ex_add_row(self, row, last);
    ui_list_box_refresh_scroll(&self->viewer.list);
}

static void drop_words(struct UITextViewEX *self)
{
    struct UIString **w = self->words.first;
    size_t n = self->words.first > self->words.last ? 0 : WORD_COUNT(&self->words), i;

    for (i = 0; i < n; i++, w++)
        ui_window_remove_child(&self->viewer.list.base, &(*w)->base, 1);
    self->words.last = self->words.first;
    rows_clear(&self->rows);
}

void ui_text_view_ex_create(struct UIListBox *base_list, int x, int y, int width, int height, BYTE transparent)
{
    struct UITextViewEX *self = (struct UITextViewEX *)base_list;
    struct UIWindow *wnd = &self->viewer.list.base;

    self->viewer.list.transparent = transparent;
    ui_window_resize_base(wnd, width, height);
    ui_window_move_base(wnd, x, y);
    ui_window_on_create(wnd, width, height);
    self->viewer.list.line_height = ROW_HEIGHT;
}

void ui_text_view_ex_clear(struct UIListBox *base_list)
{
    struct UITextViewEX *self = (struct UITextViewEX *)base_list;
    struct UIWindow **e;

    drop_words(self);
    ui_list_box_clear_items(&self->viewer.list);
    for (e = self->viewer.embeds.first; e != self->viewer.embeds.last; e++)
        ui_window_remove_child(&self->viewer.list.base, *e, 1);
    self->viewer.embeds.last = self->viewer.embeds.first;
    ui_window_invalidate_base(&self->viewer.list.base);
    ui_window_invalidate_base(&self->viewer.list.base);
}

void ui_text_view_ex_update_scroll_bar(struct UIListBox *base_list)
{
    struct UITextViewEX *self = (struct UITextViewEX *)base_list;
    struct UIListBox *list = &self->viewer.list;
    struct UIScrollBar *bar = list->vbar;
    int page = list->base.height / list->line_height;
    int rows = (int)ROW_COUNT(self);
    int extra = rows - page < 0 ? 0 : rows - page;

    if (extra) {
        list->has_vscroll = 1;
        bar->max_pos = list->extra_lines + extra;
    } else {
        list->has_vscroll = 0;
        bar->max_pos = 0;
    }
    bar->total = rows;
    bar->page = page;
    ui_window_invalidate_base(&bar->base);
    if (bar->pos > bar->max_pos)
        bar->pos = bar->max_pos;
    ui_window_invalidate_base(&list->vbar->base);
}

void ui_text_view_ex_on_create(struct UIWindow *window, int width, int height)
{
    struct UITextViewEX *self = (struct UITextViewEX *)window;
    ui_list_box_on_create((struct UIWindow *)(&self->viewer.list), width, height);
}

void ui_text_view_ex_on_draw(struct UIWindow *window)
{
    struct UITextViewEX *self = (struct UITextViewEX *)window;
    struct UIWindow *wnd = &self->viewer.list.base;
    struct Vec *row;
    int top = self->viewer.list.top, index = 0, y = TOP_MARGIN;

    if (self->viewer.list.transparent) {
        ui_window_clear_canvas(wnd, UI_TRANSPARENT);
        if (wnd->parent)
            ui_window_invalidate(wnd->parent);
    } else {
        ui_window_clear_canvas(wnd, self->viewer.background);
    }

    for (row = self->rows.first; row != self->rows.last; row++, index++) {
        struct UIString **it;

        if (index < top) {
            for (it = row->first; it != row->last; it++)
                ui_window_set_show_base(&(*it)->base, 0);
            continue;
        }
        if (row->first != row->last) {
            int x = 0;

            for (it = row->first; it != row->last; it++) {
                struct UIString *w = *it;

                if (string_equal_cstr(&w->text, "\n"))
                    continue;
                ui_window_set_show_base(&w->base, 1);
                if (w->joined)
                    x -= WORD_GAP;
                if (x < 0)
                    x = 0;
                if (w->image.size)
                    ui_window_move_base(&w->base, x, y - TOP_MARGIN);
                else
                    ui_window_move_base(&w->base, x, y);
                x += w->base.width + WORD_GAP;
            }
        }
        y += ROW_HEIGHT;
    }
    ui_list_box_refresh_scroll(&self->viewer.list);
}

int ui_text_view_ex_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                    int val3, int val4)
{
    struct UITextViewEX *self = (struct UITextViewEX *)window;
    struct UIListBox *list = &self->viewer.list;
    int old, top, step, page_rows;
    unsigned last;

    switch (msg) {
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        old = list->top;
        step = list->line_height;
        if (msg == UI_MSG_PAGE_UP)
            top = old - list->base.height / step + 1;
        else if (msg == UI_MSG_PAGE_DOWN)
            top = old - 1 + list->base.height / step;
        else
            top = old + val1;
        list->top = top;
        if (top < 0) {
            list->top = 0;
            top = 0;
        }
        last = (unsigned)(list->extra_lines + ((int)ROW_COUNT(self) - list->base.height / step));
        if ((unsigned)top > last) {
            list->top = (int)last;
            top = (int)last;
        }
        list->vbar->pos = top;
        ui_window_invalidate_base(&list->vbar->base);
        ui_window_invalidate_base(&self->viewer.list.base);
        if (list->transparent && list->base.parent)
            ui_window_invalidate(list->base.parent);
        return list->top - old;
    case UI_MSG_BAR_PAGE_UP:
        if (list->has_vscroll)
            ui_window_send_msg(&self->viewer.list.base, &list->vbar->base, UI_MSG_SCROLL_V, -1, 0, 0, 0);
        return 0;
    case UI_MSG_BAR_PAGE_DOWN:
        if (list->has_vscroll)
            ui_window_send_msg(&self->viewer.list.base, &list->vbar->base, UI_MSG_SCROLL_V, 1, 0, 0, 0);
        return 0;
    case UI_MSG_ADD_TEXT: {
        struct String text;

        string_init(&text);
        string_assign_cstr(&text, (const char *)val1);
        drop_words(self);
        ui_text_view_ex_set_text(self, string_view(&text));
        ui_window_invalidate_base(&self->viewer.list.base);
        string_free(&text);
        return 0;
    }
    default:
        (void)page_rows;
        return ui_list_box_send_msg((struct UIWindow *)(list), sender, msg, val1, val2, val3, val4);
    }
}
