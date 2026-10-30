#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include <string.h>
#include "diag/perf.h"
#include "ui/controls/controls_std.h"
#include "ui/ui_std.h"
#include "std/tree.h"

struct UIListBoxID;
struct UIWindow;
const struct UIListBoxExOps ui_list_box_id_ops = {
    .base = {
        .destroy = ui_list_box_id_destroy,
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
        .on_draw = ui_list_box_id_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_list_box_ex_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_id_on_l_btn_dbl_clk,
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
        .get_records = ui_window_get_records_base,
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .clear_items = ui_list_box_id_clear_items,
    .update_scroll_bar = ui_list_box_ex_update_scroll_bar,
};

struct UIListBoxTitle;
const struct UIListBoxExOps ui_list_box_title_ops = {
    .base = {
        .destroy = ui_list_box_title_destroy,
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
        .on_draw = ui_list_box_title_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_list_box_ex_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_id_on_l_btn_dbl_clk,
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
        .get_records = ui_window_get_records_base,
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .clear_items = ui_list_box_id_clear_items,
    .update_scroll_bar = ui_list_box_ex_update_scroll_bar,
};

#define ROW_COUNT(lb) ((lb)->rows.size)

struct UIListBoxID *ui_list_box_id_construct(struct UIListBoxID *self)
{
    struct UIListBoxEx *lb = &self->list;

    ui_window_construct(&lb->base, 0);
    lb->base.ops = &ui_list_box_id_ops.base;
    tree_init(&lb->rows);
    lb->selected = 0;
    lb->top = 0;
    lb->scroll_x = 0;
    lb->widest = 0;
    lb->vbar = NULL;
    lb->hbar = NULL;
    lb->has_vscroll = 0;
    lb->extra_lines = 0;
    lb->transparent = 0;
    lb->line_height = 16;
    lb->r = 0xff;
    lb->g = 0xff;
    lb->b = 0xff;
    lb->selection_colour = 0xfbdfca;
    return self;
}

static void destroy_row(void *value)
{
    string_free(&((struct UIListBoxIDPair *)value)->row.text);
}

struct UIListBoxIDRow *ui_list_box_id_row_at(struct Tree *rows, const int *key)
{
    struct UIListBoxIDPair *pair = tree_get(rows, key, sizeof *key, sizeof *pair, tree_less_int);

    return &pair->row;
}

void ui_list_box_id_destruct(struct UIListBoxID *self)
{
    self->list.base.ops = &ui_list_box_id_ops.base;
    tree_free(&self->list.rows, destroy_row);
    ui_window_destruct(&self->list.base);
}

void ui_list_box_id_destroy(struct UIWindow *window)
{
    struct UIListBoxID *self = (struct UIListBoxID *)window;
    ui_list_box_id_destruct(self);
    mem_free(self);
}

static void draw_background(struct UIListBoxEx *lb, int transparent)
{
    if (transparent) {
        ui_window_clear_canvas(&lb->base, UI_TRANSPARENT);
        if (lb->base.parent)
            ui_window_invalidate(lb->base.parent);
    } else {
        ui_window_clear_canvas(&lb->base, RGB(lb->r, lb->g, lb->b));
    }
}

static int end_row(const struct UIListBoxEx *lb)
{
    int end = lb->base.height / lb->line_height - lb->extra_lines + lb->top;

    if ((unsigned)end >= ROW_COUNT(lb))
        end = (int)ROW_COUNT(lb);
    return end;
}

static void fill_row(struct UIListBoxEx *lb, int line, COLORREF colour)
{
    ui_window_fill_rect(&lb->base, 0, (line - lb->top) * lb->line_height, lb->base.width, lb->line_height - 1,
                      colour);
}

static void draw_row_text(struct UIListBoxEx *lb, int line, int first, COLORREF *colour)
{
    int len = (int)ui_list_box_id_row_at(&lb->rows, &line)->text.size;
    const char *text = string_cstr(&ui_list_box_id_row_at(&lb->rows, &line)->text);

    ui_window_text_out_runs(&lb->base, 3 - lb->scroll_x, (line - first) * lb->line_height, text, len, colour, 0, 12, 0);
}

void ui_list_box_id_on_draw(struct UIWindow *window)
{
    struct UIListBoxID *self = (struct UIListBoxID *)window;
    struct UIListBoxEx *lb = &self->list;
    COLORREF colour = 0;
    int first, end, line;

    draw_background(lb, lb->transparent != 0);
    first = lb->top;
    end = end_row(lb);
    for (line = first; line < end; line++) {
        if (line == lb->selected)
            fill_row(lb, line, lb->selection_colour);
        draw_row_text(lb, line, first, &colour);
    }
}

void ui_list_box_id_on_l_btn_dbl_clk(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIListBoxID *self = (struct UIListBoxID *)window;
    struct UIListBoxEx *lb = &self->list;

    if (lb->selected != -1)
        ui_window_send_msg(lb->base.parent, &lb->base, UI_MSG_COMMAND, lb->base.id,
                 ui_list_box_id_row_at(&lb->rows, &lb->selected)->id, 0, 0);
}

void ui_list_box_id_add_item(struct UIListBoxID *self, const char *text, int id)
{
    struct UIListBoxEx *lb = &self->list;
    struct UIListBoxIDPair pair;
    int added, widest;

    pair.key = (int)ROW_COUNT(lb);
    pair.row.id = id;
    string_init(&pair.row.text);
    string_assign_cstr(&pair.row.text, text);
    tree_add(&lb->rows, &pair, sizeof pair, tree_less_int, &added);
    if (!added)
        string_free(&pair.row.text);
    widest = lb->widest;
    if (widest <= ui_window_text_width(&lb->base, text, 0, 0, 12, 0, 0))
        widest = ui_window_text_width(&lb->base, text, 0, 0, 12, 0, 0);
    lb->widest = widest;
    ui_list_box_ex_refresh_scroll(lb);
}

void ui_list_box_id_clear_items(struct UIListBoxEx *base_list)
{
    struct UIListBoxEx *lb = base_list;

    lb->selected = -1;
    lb->widest = 0;
    lb->scroll_x = 0;
    lb->top = 0;
    lb->has_vscroll = 0;
    lb->extra_lines = 0;
    tree_clear(&lb->rows, destroy_row);
}

struct UIListBoxTitle *ui_list_box_title_construct(struct UIListBoxTitle *self)
{
    ui_list_box_id_construct(&self->id);
    self->id.list.base.ops = &ui_list_box_title_ops.base;
    self->id.list.line_height = 15;
    self->listed_colour = 0x92c0fa;
    return self;
}

void ui_list_box_title_destroy(struct UIWindow *window)
{
    struct UIListBoxTitle *self = (struct UIListBoxTitle *)window;
    self->id.list.base.ops = &ui_list_box_title_ops.base;
    ui_list_box_id_destruct(&self->id);
    mem_free(self);
}

static int is_listed(int id)
{
    const int *it = g_session1.newTitles.first;
    const int *last = g_session1.newTitles.last;

    for (; it != last; it++)
        if (*it == id)
            return 1;
    return 0;
}

void ui_list_box_title_on_draw(struct UIWindow *window)
{
    struct UIListBoxTitle *self = (struct UIListBoxTitle *)window;
    struct UIListBoxEx *lb = &self->id.list;
    COLORREF colour = 0;
    int first, end, line;

    draw_background(lb, lb->transparent == 1);
    first = lb->top;
    end = end_row(lb);
    for (line = first; line < end; line++) {
        if (line == lb->selected)
            fill_row(lb, line, lb->selection_colour);
        else if (is_listed(ui_list_box_id_row_at(&lb->rows, &line)->id))
            fill_row(lb, line, self->listed_colour);
        draw_row_text(lb, line, first, &colour);
    }
}
