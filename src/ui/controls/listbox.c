#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include <string.h>
#include "diag/perf.h"

struct UIListBox;
struct UIWindow;
const struct UIListBoxOps ui_list_box_ops = {
    .base = {
        .destroy = ui_list_box_destroy,
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
        .on_create = ui_list_box_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_list_box_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_list_box_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_list_box_send_msg,
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
    .create = ui_list_box_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_list_box_clear_items,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};

struct UIListBoxNavi;
const struct UIListBoxOps ui_list_box_navi_ops = {
    .base = {
        .destroy = ui_list_box_navi_destroy,
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
        .on_create = ui_list_box_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_list_box_navi_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_list_box_navi_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_list_box_send_msg,
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
    .create = ui_list_box_create,
    .add_item = ui_list_box_navi_add_item,
    .clear_items = ui_list_box_navi_clear_items,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};

#define LINE_COUNT(lb) VEC_COUNT(&(lb)->items, struct String)
#define LINE(lb, i) (&((struct String *)(lb)->items.first)[i])

struct UIListBox *ui_list_box_construct(struct UIListBox *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_list_box_ops.base;
    self->items.first = NULL;
    self->items.last = NULL;
    self->items.end = NULL;
    self->text_colour = 0;
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
    self->selectable = 1;
    self->centre_lines = 0;
    return self;
}

void ui_list_box_destruct(struct UIListBox *self)
{
    self->base.ops = &ui_list_box_ops.base;
    string_vec_free(&self->items);
    ui_window_destruct(&self->base);
}

void ui_list_box_destroy(struct UIWindow *window)
{
    struct UIListBox *self = (struct UIListBox *)window;
    ui_list_box_destruct(self);
    mem_free(self);
}

void ui_list_box_resize(struct UIWindow *window, int width, int height)
{
    struct UIListBox *self = (struct UIListBox *)window;
    ui_window_resize_base(&self->base, width, height);
    ui_scroll_bar_resize(&self->vbar->base, 12, height);
    ui_window_move_base(&self->vbar->base, width - 13, 0);
    ui_scroll_bar_resize(&self->hbar->base, width - 13, 12);
    ui_window_move_base(&self->hbar->base, 0, height - 13);
    ui_list_box_refresh_scroll(self);
    self->top = 0;
}

static struct UIScrollBar *new_bar(struct UIListBox *self, int orientation, int x, int y, int length)
{
    void *mem = mem_alloc(sizeof(struct UIScrollBar));
    struct UIScrollBar *bar = mem ? ui_scroll_bar_construct(mem) : NULL;
    int thickness = bar->thickness;

    bar->orientation = orientation;
    if (orientation == UI_SCROLL_VERT) {
        ui_window_resize_base(&bar->base, thickness, length);
        ui_window_move_base(&bar->base, x, y);
        ui_window_on_create_base(&bar->base, thickness, length);
    } else {
        ui_window_resize_base(&bar->base, length, thickness);
        ui_window_move_base(&bar->base, x, y);
        ui_window_on_create_base(&bar->base, length, thickness);
    }
    return bar;
}

void ui_list_box_on_create(struct UIWindow *window, int width, int height)
{
    struct UIListBox *self = (struct UIListBox *)window;
    self->vbar = new_bar(self, UI_SCROLL_VERT, width - 13, 0, height);
    ui_window_add_child(&self->base, &self->vbar->base);
    self->hbar = new_bar(self, UI_SCROLL_HORZ, 0, height - 13, width - 13);
    ui_window_add_child(&self->base, &self->hbar->base);
}

int ui_list_box_hit_line(struct UIListBox *self, int x, int y)
{
    int line;

    if (x < 0 || y < 0 || x >= self->base.width || y >= self->base.height || !self->selectable)
        return -1;
    line = y / self->line_height + self->top;
    if (line < 0 || (unsigned)line >= LINE_COUNT(self))
        return -1;
    return line;
}

void ui_list_box_on_draw(struct UIWindow *window)
{
    struct UIListBox *self = (struct UIListBox *)window;
    struct UIWindow *wnd = &self->base;
    DWORD colour;
    int first, end, row, x, y;
    struct String *line;

    if (self->transparent) {
        ui_window_clear_canvas(wnd, UI_TRANSPARENT);
        if (wnd->parent)
            ui_window_invalidate(wnd->parent);
    } else {
        ui_window_clear_canvas(wnd, RGB(self->r, self->g, self->b));
    }
    first = self->top;
    end = wnd->height / self->line_height - self->extra_lines + first;
    if ((unsigned)end > LINE_COUNT(self))
        end = (int)LINE_COUNT(self);
    colour = self->text_colour;
    for (row = 0; first + row < end; row++) {
        if (first + row == self->selected)
            ui_window_fill_rect(wnd, 0, row * self->line_height, wnd->width, self->line_height - 1,
                              self->selection_colour);
        line = LINE(self, first + row);
        x = 3 - self->scroll_x;
        y = self->line_height * row;
        if (self->centre_lines == 1) {
            int text_h = ui_window_text_height(wnd, string_cstr(line), 0, 0, 12, 0, 0);

            y = (int)((float)(self->line_height - text_h) * 0.5f + (float)(row * self->line_height));
        }
        ui_window_text_out_runs(wnd, x, y, string_cstr(line), (int)line->size, &colour, 0, 12, 0);
    }
}

void ui_list_box_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIListBox *self = (struct UIListBox *)window;
    self->selected = ui_list_box_hit_line(self, x, y);
    ui_window_invalidate_base(&self->base);
}

void ui_list_box_on_l_btn_dbl_clk(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIListBox *self = (struct UIListBox *)window;
    if (self->selected != -1)
        ui_window_send_msg(self->base.parent, &self->base, UI_MSG_COMMAND, self->base.id, self->selected, 0, 0);
}

void ui_list_box_on_wheel(struct UIWindow *window, int delta)
{
    struct UIListBox *self = (struct UIListBox *)window;
    if (!self->has_vscroll)
        return;
    if (delta < 0)
        ui_window_send_msg(&self->base, &self->vbar->base, UI_MSG_SCROLL_V, 3, 0, 0, 0);
    else
        ui_window_send_msg(&self->base, &self->vbar->base, UI_MSG_SCROLL_V, -3, 0, 0, 0);
}

void ui_list_box_create(struct UIListBox *self, int x, int y, int width, int height, BYTE transparent)
{
    self->transparent = transparent;
    ui_window_resize_base(&self->base, width, height);
    ui_window_move_base(&self->base, x, y);
    ui_window_on_create(&self->base, width, height);
}

void ui_list_box_add_item(struct UIListBox *self, const char *text)
{
    struct String line;
    int widest;

    string_init(&line);
    string_assign_cstr(&line, text);
    string_vec_push_move(&self->items, &line);
    string_free(&line);
    widest = self->widest;
    if (widest <= ui_window_text_width(&self->base, text, 0, 0, 12, 0, 0))
        widest = ui_window_text_width(&self->base, text, 0, 0, 12, 0, 0);
    self->widest = widest;
    ui_list_box_refresh_scroll(self);
}

void ui_list_box_clear_items(struct UIListBox *self)
{
    struct String *line;

    self->selected = -1;
    self->widest = 0;
    self->scroll_x = 0;
    self->top = 0;
    self->has_vscroll = 0;
    self->extra_lines = 0;
    for (line = self->items.first; line != (struct String *)self->items.last; line++)
        string_free(line);
    self->items.last = self->items.first;
    ui_list_box_refresh_scroll(self);
    ui_window_invalidate_base(&self->base);
}

void ui_list_box_update_scroll_bar(struct UIListBox *self)
{
    struct UIScrollBar *bar = self->vbar;
    int in_view = self->base.height / self->line_height;
    int beyond = (int)LINE_COUNT(self) - in_view;

    if (beyond < 0)
        beyond = 0;
    if (beyond) {
        self->has_vscroll = 1;
        bar->max_pos = self->extra_lines + beyond;
    } else {
        self->has_vscroll = 0;
        bar->max_pos = 0;
    }
    bar->total = (int)LINE_COUNT(self);
    bar->page = in_view;
    ui_window_invalidate_base(&bar->base);
    if (bar->pos > bar->max_pos)
        bar->pos = bar->max_pos;
    ui_window_invalidate_base(&self->vbar->base);
}

#define ROW_FLAG(lb, i) (((int *)(lb)->flags.first)[i])

struct UIListBoxNavi *ui_list_box_navi_construct(struct UIListBoxNavi *self)
{
    ui_list_box_construct(&self->list);
    self->list.base.ops = &ui_list_box_navi_ops.base;
    self->flags.first = NULL;
    self->flags.last = NULL;
    self->flags.end = NULL;
    self->row_base = -1;
    self->flag_colour = 0xd2d2d2;
    return self;
}

void ui_list_box_navi_destruct(struct UIListBoxNavi *self)
{
    self->list.base.ops = &ui_list_box_navi_ops.base;
    self->flags.last = self->flags.first;
    vec_free(&self->flags);
    ui_list_box_destruct(&self->list);
}

void ui_list_box_navi_destroy(struct UIWindow *window)
{
    struct UIListBoxNavi *self = (struct UIListBoxNavi *)window;
    ui_list_box_navi_destruct(self);
    mem_free(self);
}

void ui_list_box_navi_on_draw(struct UIWindow *window)
{
    struct UIListBoxNavi *self = (struct UIListBoxNavi *)window;
    struct UIListBox *lb = &self->list;
    struct UIWindow *wnd = &lb->base;
    COLORREF colour = 0;
    int first, end, line;

    if (lb->transparent) {
        ui_window_clear_canvas(wnd, UI_TRANSPARENT);
        if (wnd->parent)
            ui_window_invalidate(wnd->parent);
    } else {
        ui_window_clear_canvas(wnd, RGB(lb->r, lb->g, lb->b));
    }
    first = lb->top;
    end = wnd->height / lb->line_height - lb->extra_lines + first;
    if ((unsigned)end > LINE_COUNT(lb))
        end = (int)LINE_COUNT(lb);
    for (line = first; line < end; line++) {
        if (ROW_FLAG(self, line) == 1)
            colour = self->flag_colour;
        if (line == lb->selected)
            ui_window_fill_rect(wnd, 0, (line - first) * lb->line_height, wnd->width, lb->line_height - 1,
                              lb->selection_colour);
        ui_window_text_out_runs(wnd, 3 - lb->scroll_x, (line - first) * lb->line_height, string_cstr(LINE(lb, line)),
                   (int)LINE(lb, line)->size, &colour, 0, 12, 0);
        if (ROW_FLAG(self, line) == 1)
            colour = 0;
    }
}

void ui_list_box_navi_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIListBoxNavi *self = (struct UIListBoxNavi *)window;
    struct UIListBox *lb = &self->list;

    lb->selected = ui_list_box_hit_line(lb, x, y);
    if (lb->selected != -1)
        ui_window_send_msg(lb->base.parent, &lb->base, UI_MSG_COMMAND, lb->base.id, lb->selected, 0, 0);
    ui_window_invalidate_base(&lb->base);
}

void ui_list_box_navi_add_item(struct UIListBox *base_list, const char *text)
{
    struct UIListBoxNavi *self = (struct UIListBoxNavi *)base_list;
    struct UIListBox *lb = &self->list;
    struct String line;
    int flag = 0;
    int widest;

    string_init(&line);
    string_assign_cstr(&line, text);
    string_vec_push_move(&lb->items, &line);
    string_free(&line);
    vec_push(&self->flags, &flag, sizeof flag);
    widest = lb->widest;
    if (widest <= ui_window_text_width(&lb->base, text, 0, 0, 12, 0, 0))
        widest = ui_window_text_width(&lb->base, text, 0, 0, 12, 0, 0);
    lb->widest = widest;
    ui_list_box_update_scroll_bar(lb);
}

void ui_list_box_navi_clear_items(struct UIListBox *base_list)
{
    struct UIListBoxNavi *self = (struct UIListBoxNavi *)base_list;
    struct UIListBox *lb = &self->list;
    struct String *line;

    lb->selected = -1;
    lb->widest = 0;
    lb->scroll_x = 0;
    lb->top = 0;
    lb->has_vscroll = 0;
    lb->extra_lines = 0;
    for (line = lb->items.first; line != (struct String *)lb->items.last; line++)
        string_free(line);
    lb->items.last = lb->items.first;
    self->flags.last = self->flags.first;
    ui_list_box_update_scroll_bar(lb);
    self->row_base = -1;
}

void ui_list_box_navi_set_flag(struct UIListBoxNavi *self, int index, int flag)
{
    int row = self->row_base + index;

    if (row < 0 || (unsigned)row > LINE_COUNT(&self->list))
        return;
    ROW_FLAG(self, row) = flag;
    ui_window_invalidate_base(&self->list.base);
}

void ui_list_box_navi_clear_flags(struct UIListBoxNavi *self)
{
    int *flag;

    for (flag = self->flags.first; flag != (int *)self->flags.last; flag++)
        *flag = 0;
    ui_window_invalidate_base(&self->list.base);
}

int ui_list_box_navi_row_base(struct UIListBoxNavi *self)
{
    return self->row_base;
}

void ui_list_box_navi_set_row_base(struct UIListBoxNavi *self, int base)
{
    self->row_base = base;
}

void ui_list_box_navi_set_flag_colour(struct UIListBoxNavi *self, COLORREF colour)
{
    self->flag_colour = colour;
    ui_window_invalidate_base(&self->list.base);
}

int ui_list_box_count(struct UIListBox *self)
{
    return (int)LINE_COUNT(self);
}

struct UIListBox2 {
    struct UIListBox list;
    int font_face;
    int font_height;
    BYTE font_arg3, font_arg4;
};
void ui_list_box2_destroy(struct UIWindow *window);
void ui_list_box2_on_draw(struct UIWindow *self);
const struct UIListBoxOps ui_list_box2_ops = {
    .base = {
        .destroy = ui_list_box2_destroy,
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
        .on_create = ui_list_box_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_list_box2_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_list_box_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_list_box_send_msg,
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
    .create = ui_list_box_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_list_box_clear_items,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};

#define LB2_CENTRE_FONT 0xc

void ui_list_box2_destroy(struct UIWindow *window)
{
    struct UIListBox2 *self = (struct UIListBox2 *)window;
    self->list.base.ops = &ui_list_box2_ops.base;
    ui_list_box_destruct(&self->list);
    mem_free(self);
}

void ui_list_box2_on_draw(struct UIWindow *window)
{
    struct UIListBox2 *self = (struct UIListBox2 *)window;
    struct UIListBox *lb = &self->list;
    int rows, count, top, end, i;
    COLORREF colour;

    if (lb->transparent) {
        ui_window_clear_canvas(&lb->base, UI_TRANSPARENT);
        if (lb->base.parent)
            ui_window_invalidate(lb->base.parent);
    } else {
        ui_window_clear_canvas(&lb->base, (BYTE)lb->r | (BYTE)lb->g << 8 | (DWORD)(BYTE)lb->b << 16);
    }
    rows = lb->base.height / lb->line_height;
    count = (int)VEC_COUNT(&lb->items, struct String);
    top = lb->top;
    if ((unsigned)(rows - lb->extra_lines + top) >= (unsigned)count)
        end = count;
    else
        end = rows - lb->extra_lines + top;
    colour = lb->text_colour;
    for (i = 0; i + top < end; i++) {
        struct String *line = &VEC_AT(&lb->items, struct String, i + top);
        int x, y;

        if (i + top == lb->selected)
            ui_window_fill_rect(&lb->base, 0, i * lb->line_height, lb->base.width, lb->line_height - 1,
                              lb->selection_colour);
        y = lb->line_height * i;
        x = 3 - lb->scroll_x;
        if (lb->centre_lines == 1) {
            int h = ui_window_text_height(&lb->base, string_cstr(line), 0, 0, LB2_CENTRE_FONT, 0, 0);

            y = (int)((float)(lb->line_height - h) * 0.5f + (float)(i * lb->line_height));
        }
        ui_window_text_out_runs_ex(&lb->base, x, y, string_cstr(line), (int)line->size, &colour,
                               self->font_face, self->font_height, self->font_arg3, self->font_arg4, 0);
    }
}
