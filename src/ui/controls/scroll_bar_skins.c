#include "ui/controls/scroll.h"
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

#define BASIC_DIR UI_SKIN "basic_interface\\"

struct UIScrollBar;
struct UIWindow;
const struct UIWindowOps ui_scroll_bar2_ops = {
    .destroy = ui_scroll_bar2_destroy,
    .resize = ui_scroll_bar_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_scroll_bar_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_scroll_bar2_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_scroll_bar_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_scroll_bar_on_l_btn_dbl_clk,
    .on_mouse_move = ui_scroll_bar_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_scroll_bar_on_mouse_shape,
    .on_l_btn_up = ui_scroll_bar_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_scroll_bar_on_r_btn_up,
    .on_wheel = ui_scroll_bar_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
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
};
struct UIScrollBar4;
const struct UIWindowOps ui_scroll_bar4_ops = {
    .destroy = ui_scroll_bar4_destroy,
    .resize = ui_scroll_bar_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_scroll_bar_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_scroll_bar4_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_scroll_bar_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_scroll_bar_on_l_btn_dbl_clk,
    .on_mouse_move = ui_scroll_bar_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_scroll_bar_on_mouse_shape,
    .on_l_btn_up = ui_scroll_bar_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_scroll_bar_on_r_btn_up,
    .on_wheel = ui_scroll_bar_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
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
};

struct UIScrollBar4 {
    struct UIScrollBar bar;
    int thumb_size;
};
SIZE_ASSERT(UIScrollBar4, 0xb0);

void ui_scroll_bar2_destroy(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    ui_scroll_bar_destruct(self);
    mem_free(self);
}

void ui_scroll_bar2_on_draw(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    struct UIWindow *wnd = &self->base;
    int top;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    if (!self->max_pos || self->orientation != UI_SCROLL_VERT)
        return;
    ui_window_fill_rect(wnd, 0, 0xb, 0xa, wnd->height - 0x15, 0);
    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(BASIC_DIR "dialscr_up.bmp"), 0);
    top = ui_scroll_bar_thumb_pos(self) + self->button_size;
    ui_window_fill_rect(wnd, 0, top + 1, 0xa, ui_scroll_bar_get_thumb_size(self) - 1, 0xafaaa5);
    ui_window_draw_bitmap(wnd, 0, wnd->height - self->button_size - 1,
                        ui_skin_bitmap(BASIC_DIR "dialscr_down.bmp"), 0);
}

void ui_scroll_bar4_destroy(struct UIWindow *window)
{
    struct UIScrollBar4 *self = (struct UIScrollBar4 *)window;
    ui_scroll_bar_destruct(&self->bar);
    mem_free(self);
}

void ui_scroll_bar4_on_draw(struct UIWindow *window)
{
    struct UIScrollBar4 *self = (struct UIScrollBar4 *)window;
    struct UIWindow *wnd = &self->bar.base;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    if (!self->bar.max_pos)
        return;
    if (self->bar.orientation == UI_SCROLL_HORZ) {
        int x;

        ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(BASIC_DIR "scrollbar.bmp"), 1);
        x = ui_scroll_bar_thumb_pos(&self->bar);
        ui_window_draw_bitmap(wnd, x, 0, ui_skin_bitmap(BASIC_DIR "scroll_bar.bmp"), 1);
    }
    self->bar.min_thumb = 0;
}
