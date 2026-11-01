#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include <string.h>
#include "diag/perf.h"
#include "ui/controls/controls_std.h"
#include "ui/ui_std.h"
#include "std/tree.h"

struct UIListBoxEx;
struct UIWindow;
const struct UIListBoxExOps ui_list_box_ex_ops = {
    .base = {
        .destroy = ui_list_box_ex_destroy,
        .resize = ui_list_box_ex_resize,
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
        .on_create = ui_list_box_ex_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_list_box_ex_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_list_box_ex_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_ex_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_ex_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_list_box_ex_send_msg,
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
    .clear_items = ui_list_box_ex_clear_items,
    .update_scroll_bar = ui_list_box_ex_update_scroll_bar,
};

#define ROW_COUNT(lb) ((lb)->rows.size)

static void destroy_row(void *value)
{
    string_free(&((struct UIListBoxExPair *)value)->row.text);
}

struct UIListBoxExRow *list_box_ex_entry_at(struct Tree *rows, const int *key)
{
    struct UIListBoxExPair *pair = tree_get(rows, key, sizeof *key, sizeof *pair, tree_less_int);

    return &pair->row;
}

struct UIListBoxEx *ui_list_box_ex_construct(struct UIListBoxEx *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_list_box_ex_ops.base;
    tree_init(&self->rows);
    self->selected = 0;
    self->top = 0;
    self->scroll_x = 0;
    self->widest = 0;
    self->vbar = NULL;
    self->hbar = NULL;
    self->has_vscroll = 0;
    self->extra_lines = 0;
    self->transparent = 0;
    self->line_height = 16;
    self->r = 0xff;
    self->g = 0xff;
    self->b = 0xff;
    self->selection_colour = 0xfbdfca;
    return self;
}

void ui_list_box_ex_destruct(struct UIListBoxEx *self)
{
    self->base.ops = &ui_list_box_ex_ops.base;
    tree_free(&self->rows, destroy_row);
    ui_window_destruct(&self->base);
}

void ui_list_box_ex_destroy(struct UIWindow *window)
{
    struct UIListBoxEx *self = (struct UIListBoxEx *)window;
    ui_list_box_ex_destruct(self);
    mem_free(self);
}

void ui_list_box_ex_resize(struct UIWindow *window, int width, int height)
{
    struct UIListBoxEx *self = (struct UIListBoxEx *)window;
    ui_window_resize_base(&self->base, width, height);
    ui_scroll_bar_resize(&self->vbar->base, 12, height);
    ui_window_move_base(&self->vbar->base, width - 13, 0);
    ui_scroll_bar_resize(&self->hbar->base, width - 13, 12);
    ui_window_move_base(&self->hbar->base, 0, height - 13);
    ui_list_box_ex_refresh_scroll(self);
}

void ui_list_box_ex_on_create(struct UIWindow *window, int width, int height)
{
    struct UIListBoxEx *self = (struct UIListBoxEx *)window;
    void *mem = mem_alloc(sizeof(struct UIScrollBar));
    struct UIScrollBar *bar = mem ? ui_scroll_bar_construct(mem) : NULL;
    int thickness;

    self->vbar = bar;
    thickness = bar->thickness;
    bar->orientation = UI_SCROLL_VERT;
    ui_window_resize_base(&bar->base, thickness, height);
    ui_window_move_base(&bar->base, width - 13, 0);
    ui_window_on_create_base(&bar->base, thickness, height);
    ui_window_add_child(&self->base, &self->vbar->base);

    mem = mem_alloc(sizeof(struct UIScrollBar));
    bar = mem ? ui_scroll_bar_construct(mem) : NULL;
    self->hbar = bar;
    thickness = bar->thickness;
    bar->orientation = UI_SCROLL_HORZ;
    ui_window_resize_base(&bar->base, width - 13, thickness);
    ui_window_move_base(&bar->base, 0, height - 13);
    ui_window_on_create_base(&bar->base, width - 13, thickness);
    ui_window_add_child(&self->base, &self->hbar->base);
}

void ui_list_box_ex_on_draw(struct UIWindow *window)
{
    struct UIListBoxEx *self = (struct UIListBoxEx *)window;
    struct UIWindow *wnd = &self->base;
    int first, end, line;

    if (self->transparent) {
        ui_window_clear_canvas(wnd, UI_TRANSPARENT);
        if (wnd->parent)
            ui_window_invalidate(wnd->parent);
    } else {
        ui_window_clear_canvas(wnd, RGB(self->r, self->g, self->b));
    }
    first = self->top;
    end = wnd->height / self->line_height - self->extra_lines + first;
    if ((unsigned)end >= ROW_COUNT(self))
        end = (int)ROW_COUNT(self);
    for (line = first; line < end; line++) {
        COLORREF colour;
        int len;
        const char *text;

        if (line == self->selected)
            ui_window_fill_rect(wnd, 0, (line - first) * self->line_height, wnd->width, self->line_height - 1,
                              self->selection_colour);
        colour = list_box_ex_entry_at(&self->rows, &line)->colour;
        len = (int)list_box_ex_entry_at(&self->rows, &line)->text.size;
        text = string_cstr(&list_box_ex_entry_at(&self->rows, &line)->text);
        ui_window_text_out_runs(wnd, 3 - self->scroll_x, (line - first) * self->line_height, text, len, &colour, 0, 12, 0);
    }
}

static int hit_row(const struct UIListBoxEx *self, int x, int y)
{
    int row;

    if (x < 0 || y < 0 || x >= self->base.width || y >= self->base.height)
        return -1;
    row = y / self->line_height + self->top;
    if (row < 0 || (unsigned)row >= ROW_COUNT(self))
        return -1;
    return row;
}

void ui_list_box_ex_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIListBoxEx *self = (struct UIListBoxEx *)window;
    self->selected = hit_row(self, x, y);
    ui_window_invalidate_base(&self->base);
}

void ui_list_box_ex_on_l_btn_dbl_clk(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIListBoxEx *self = (struct UIListBoxEx *)window;
    if (self->selected != -1)
        ui_window_send_msg(self->base.parent, &self->base, UI_MSG_COMMAND, self->base.id, self->selected, 0, 0);
}

void ui_list_box_ex_on_wheel(struct UIWindow *window, int delta)
{
    struct UIListBoxEx *self = (struct UIListBoxEx *)window;
    if (!self->has_vscroll)
        return;
    if (delta < 0)
        ui_window_send_msg(&self->base, &self->vbar->base, UI_MSG_SCROLL_V, 3, 0, 0, 0);
    else
        ui_window_send_msg(&self->base, &self->vbar->base, UI_MSG_SCROLL_V, -3, 0, 0, 0);
}

static void scroll_one(struct UIListBoxEx *self, int lines)
{
    ui_window_send_msg(&self->base, NULL, UI_MSG_SCROLL_V, lines, 0, 0, 0);
}

static void redraw_scrolled(struct UIListBoxEx *self)
{
    ui_window_invalidate_base(&self->base);
    if (self->transparent && self->base.parent)
        ui_window_invalidate(self->base.parent);
}

int ui_list_box_ex_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                   int val3, int val4)
{
    struct UIListBoxEx *self = (struct UIListBoxEx *)window;
    int old, max;

    switch (msg) {
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        old = self->top;
        if (msg == UI_MSG_PAGE_UP)
            self->top = old - self->base.height / self->line_height + 1;
        else if (msg == UI_MSG_PAGE_DOWN)
            self->top = old - 1 + self->base.height / self->line_height;
        else
            self->top = old + val1;
        if (self->top < 0)
            self->top = 0;
        max = (int)ROW_COUNT(self) + self->extra_lines - self->base.height / self->line_height;
        if ((unsigned)self->top > (unsigned)max)
            self->top = max;
        self->vbar->pos = self->top;
        ui_window_invalidate_base(&self->vbar->base);
        redraw_scrolled(self);
        if (ui_window_get_ancestor(&self->base))
            ui_window_invalidate(ui_window_get_ancestor(&self->base));
        return self->top - old;
    case UI_MSG_SCROLL_H:
    case UI_MSG_PAGE_LEFT:
    case UI_MSG_PAGE_RIGHT:
        old = self->scroll_x;
        if (msg == UI_MSG_PAGE_LEFT)
            val1 = -15;
        else if (msg == UI_MSG_PAGE_RIGHT)
            val1 = 15;
        self->scroll_x = old + val1;
        if (self->scroll_x < 0)
            self->scroll_x = 0;
        max = self->widest + self->has_vscroll * 16 - self->base.width;
        if (self->scroll_x > max)
            self->scroll_x = max;
        self->hbar->pos = self->scroll_x;
        ui_window_invalidate_base(&self->hbar->base);
        redraw_scrolled(self);
        return self->scroll_x - old;
    case UI_MSG_BAR_PAGE_UP:
        if (--self->selected < 0)
            self->selected = 0;
        if (self->selected < self->top)
            scroll_one(self, -1);
        break;
    case UI_MSG_BAR_PAGE_DOWN:
        if ((unsigned)++self->selected > ROW_COUNT(self) - 1)
            self->selected = (int)ROW_COUNT(self) - 1;
        if (self->selected > self->base.height / self->line_height + self->top - 1)
            scroll_one(self, 1);
        break;
    case COMBO_MSG_SELECT:
        self->selected = val1;
        if (self->selected < 0)
            self->selected = 0;
        if ((unsigned)self->selected > ROW_COUNT(self) - 1)
            self->selected = (int)ROW_COUNT(self) - 1;
        if (self->selected < self->top)
            scroll_one(self, -1);
        if (self->selected > self->base.height / self->line_height - 1 + self->top)
            scroll_one(self, 1);
        break;
    default:
        return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
    }
    ui_window_invalidate_base(&self->base);
    ui_list_box_ex_refresh_scroll(self);
    return 0;
}

void ui_list_box_ex_create(struct UIListBoxEx *self, int x, int y, int width, int height, BYTE transparent)
{
    self->transparent = transparent;
    ui_window_resize_base(&self->base, width, height);
    ui_window_move_base(&self->base, x, y);
    ui_window_on_create(&self->base, width, height);
}

void ui_list_box_ex_add_item(struct UIListBoxEx *self, const char *text, COLORREF colour)
{
    struct UIListBoxExPair pair;
    int added, widest;

    pair.key = (int)ROW_COUNT(self);
    string_init(&pair.row.text);
    string_assign_cstr(&pair.row.text, text);
    pair.row.colour = colour;
    tree_add(&self->rows, &pair, sizeof pair, tree_less_int, &added);
    if (!added)
        string_free(&pair.row.text);
    widest = self->widest;
    if (widest <= ui_window_text_width(&self->base, text, 0, 0, 12, 0, 0))
        widest = ui_window_text_width(&self->base, text, 0, 0, 12, 0, 0);
    self->widest = widest;
    ui_list_box_ex_refresh_scroll(self);
}

void ui_list_box_ex_clear_items(struct UIListBoxEx *self)
{
    self->selected = -1;
    self->widest = 0;
    self->scroll_x = 0;
    self->top = 0;
    self->has_vscroll = 0;
    self->extra_lines = 0;
    tree_clear(&self->rows, destroy_row);
}

void ui_list_box_ex_update_scroll_bar(struct UIListBoxEx *self)
{
    struct UIScrollBar *bar = self->vbar;
    int in_view = self->base.height / self->line_height;
    int beyond = (int)ROW_COUNT(self) - in_view;

    if (beyond < 0)
        beyond = 0;
    self->has_vscroll = beyond != 0;
    bar->max_pos = beyond ? self->extra_lines + beyond : 0;
    bar->page = in_view;
    bar->total = (int)ROW_COUNT(self);
    ui_window_invalidate_base(&bar->base);
    if (bar->pos > bar->max_pos)
        bar->pos = bar->max_pos;
    ui_window_invalidate_base(&self->vbar->base);
}
