#include "ui/controls/popups.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/controls/controls.h"
#include "ui/skin.h"
#include "ui/draw.h"
#include "ui/window_mgr.h"
#include "diag/perf.h"

#define UI_ID_MINI_TOGGLED 0x107

struct UIMiniBtnWnd {
    struct UIFrameWnd frame;
    struct String bitmap_shown;
    struct String bitmap_hidden;
    int target_id;
};

struct UIWindow;
const struct UIWindowOps ui_mini_btn_wnd_ops = {
    .destroy = ui_mini_btn_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_mini_btn_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_mini_btn_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_frame_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

void ui_mini_btn_wnd_destruct(struct UIMiniBtnWnd *self)
{
    self->frame.base.ops = &ui_mini_btn_wnd_ops;
    string_free(&self->bitmap_hidden);
    string_free(&self->bitmap_shown);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_mini_btn_wnd_destroy(struct UIWindow *window)
{
    struct UIMiniBtnWnd *self = (struct UIMiniBtnWnd *)window;
    ui_mini_btn_wnd_destruct(self);
    mem_free(self);
}

void ui_mini_btn_wnd_on_draw(struct UIWindow *window)
{
    struct UIMiniBtnWnd *self = (struct UIMiniBtnWnd *)window;
    struct UIWindow *target;
    const struct String *name;

    if (self->frame.base.id < 0)
        return;
    target = ui_get_window(self->target_id);
    name = target && target->visible ? &self->bitmap_shown : &self->bitmap_hidden;
    ui_window_draw_bitmap(&self->frame.base, 0, 0, ui_skin_bitmap(string_cstr(name)), 0);
}

void ui_mini_btn_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMiniBtnWnd *self = (struct UIMiniBtnWnd *)window;
    struct UIWindow *target = ui_get_window(self->target_id);
    int show;

    if (target) {
        show = target->visible == 0;
    } else {
        target = ui_make_window(self->target_id);
        show = 1;
    }
    if (target) {
        ui_window_set_show(target, show);
        ui_window_send_msg(target, &self->frame.base, UI_MSG_COMMAND, UI_ID_MINI_TOGGLED, show, 0, 0);
    }
    ui_window_invalidate_base(&self->frame.base);
}
