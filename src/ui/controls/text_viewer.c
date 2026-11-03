#include "ui/controls/text_viewer.h"
#include "ui/controls/listbox.h"
#include "platform/code_page.h"
#include "core/text_format.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include "diag/perf.h"

struct UITextViewer;
struct UIWindow;

const struct UIListBoxOps ui_text_viewer_ops = {
    .base = {
        .destroy = ui_text_viewer_destroy,
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
        .on_create = ui_text_viewer_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_text_viewer_on_draw,
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
        .send_msg = ui_text_viewer_send_msg,
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
    .clear_items = ui_text_viewer_clear_items,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};

#define LINE_COUNT(tv) VEC_COUNT(&(tv)->list.items, struct String)
#define LINE(tv, i) (&((struct String *)(tv)->list.items.first)[i])

struct UIWindow *ui_text_viewer_construct(struct UIWindow *wnd)
{
    struct UITextViewer *self = (struct UITextViewer *)wnd;

    ui_list_box_construct(&self->list);
    self->list.base.ops = &ui_text_viewer_ops.base;
    self->embeds.first = NULL;
    self->embeds.last = NULL;
    self->embeds.end = NULL;
    self->coloured = 0;
    self->coloured_arg = 0;
    self->list.line_height = 0x12;
    self->background = ui_skin_argb(0x12, 2);
    ui_window_invalidate_base(&self->list.base);
    self->list.outlined = 0;
    return wnd;
}

void ui_text_viewer_destroy(struct UIWindow *window)
{
    struct UITextViewer *self = (struct UITextViewer *)window;
    self->list.base.ops = &ui_text_viewer_ops.base;
    vec_free(&self->embeds);
    ui_list_box_destruct(&self->list);
    mem_free(self);
}

void ui_text_viewer_on_draw(struct UIWindow *window)
{
    struct UITextViewer *self = (struct UITextViewer *)window;
    struct UIWindow *wnd = &self->list.base;
    COLORREF colour = 0;
    int step, first, end, row, i;

    if (self->list.transparent) {
        ui_window_clear_canvas(wnd, UI_TRANSPARENT);
        if (wnd->parent)
            ui_window_invalidate(wnd->parent);
    } else {
        ui_window_clear_canvas(wnd, self->background);
    }

    step = self->list.line_height - self->list.extra_lines;
    if (!step)
        step = 1;
    first = self->list.top;
    end = first + wnd->height / step;
    if ((unsigned)end > LINE_COUNT(self))
        end = (int)LINE_COUNT(self);

    for (i = 0; i < first; i++) {
        struct String *line = LINE(self, i);
        const char *p = string_cstr(line);
        const char *stop = p + line->size;

        while (p < stop)
            p = text_read_colour_code(text_find_colour_code(p, stop), stop, &colour);
    }

    if (first >= end)
        return;
    for (row = 0; row < end - first; row++) {
        struct String *line = LINE(self, first + row);
        int y = self->list.line_height * row + 4;

        if (self->list.outlined)
            ui_window_text_out_outlined(wnd, 3, y, string_cstr(line), (int)line->size, 0, 0xffffff, 0, 12,
                                     0, 0);
        else if (self->coloured)
            ui_window_text_out_outlined_ptr(wnd, 3, y, string_cstr(line), (int)line->size, &colour, self->background, 0, 12,
                       self->coloured_arg);
        else
            ui_window_text_out_runs(wnd, 3, y, string_cstr(line), (int)line->size, &colour, 0, 12, 0);
    }
}

void ui_text_viewer_ignore_click(struct UIWindow *self, int x, int y)
{
}

void ui_text_viewer_clear_items(struct UIListBox *base_list)
{
    struct UITextViewer *self = (struct UITextViewer *)base_list;
    struct UIWindow **embed;

    ui_list_box_clear_items(&self->list);
    for (embed = self->embeds.first; embed != (struct UIWindow **)self->embeds.last; embed++)
        ui_window_remove_child(&self->list.base, *embed, 1);
    self->embeds.last = self->embeds.first;
    ui_window_invalidate_base(&self->list.base);
}



void ui_text_viewer_on_create(struct UIWindow *window, int width, int height)
{
    struct UITextViewer *self = (struct UITextViewer *)window;
    ui_list_box_on_create((struct UIWindow *)(&self->list), width, height);
}
