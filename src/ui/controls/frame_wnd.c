#include "ui/window.h"
#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include <mmsystem.h>
#include <stdlib.h>
#include "core/object_type_tables.h"
#include "ui/controls/ui_hooks.h"
#include "ui/render/graphic_setting.h"

struct ItemInfo;

const struct UIWindowOps ui_frame_wnd_ops = {
    .destroy = ui_frame_wnd_destroy,
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
    .on_draw = ui_window_on_draw_base,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
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

struct UIFrameWnd *ui_frame_wnd_construct(struct UIFrameWnd *self, int composites)
{
    ui_window_construct(&self->base, composites);
    self->base.ops = &ui_frame_wnd_ops;
    self->press_x = 0;
    self->press_y = 0;
    self->default_id = UI_ID_NONE;
    self->cancel_id = UI_ID_NONE;
    return self;
}

void ui_frame_wnd_destruct(struct UIFrameWnd *self)
{
    self->base.ops = &ui_frame_wnd_ops;
    ui_window_destruct(&self->base);
}

int ui_frame_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                  int val1, int val2, int val3, int val4)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    struct ItemInfo *item;
    int id;

    switch (msg) {
    case UI_MSG_DEFAULT_BUTTON:
    case UI_MSG_CANCEL_BUTTON:
        id = msg == UI_MSG_DEFAULT_BUTTON ? self->default_id : self->cancel_id;
        if (id)
            ui_window_send_msg(&self->base, &self->base, UI_MSG_COMMAND, id, 0, 0, 0);
        return 0;
    case UI_MSG_TEXT_BUTTON:
        item = (struct ItemInfo *)val3;
        if (item && g_uiHooks.show_item_link)
            g_uiHooks.show_item_link(item, val4);
        return 0;
    }
    return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
}

void ui_frame_wnd_destroy(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    self->base.ops = &ui_frame_wnd_ops;
    ui_window_destruct(&self->base);
    mem_free(self);
}

void ui_frame_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    self->base.x = x;
    self->base.y = y;
    ui_frame_wnd_snap(&self->base);
}

void ui_frame_wnd_save_pos(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    struct UIFrameWnd *f = self;

    f->saved_x = self->base.x;
    f->saved_y = self->base.y;
}

void ui_frame_wnd_move_delta(struct UIWindow *window, int dx, int dy)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    struct UIFrameWnd *f = self;

    self->base.x = f->saved_x + dx;
    self->base.y = f->saved_y + dy;
}

void ui_frame_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    struct UIFrameWnd *f = self;
    int press_x, press_y;

    if (!self)
        return;
    if (g_windowMgr.last_id != self->base.id) {
        ui_window_mgr_touch_window_id(&g_windowMgr, self->base.id);
        g_windowMgr.last_id = self->base.id;
    }
    ui_window_mgr_set_capture(&g_windowMgr, &self->base);
    press_x = x;
    press_y = y;
    ui_window_to_screen(&self->base, &press_x, &press_y);
    f->press_x = press_x;
    f->press_y = press_y;
    f->saved_x = self->base.x;
    f->saved_y = self->base.y;
    f->base.alpha_target = UI_FRAME_DRAG_ALPHA;
    f->base.alpha_time = timeGetTime();
    snap_mgr_save_group_positions(&g_snapMgr, &self->base);
}

static int larger_shift(int a, int b)
{
    return abs(a) < abs(b) ? b : a;
}

void ui_frame_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    struct UIWindow *wnd = &self->base;
    struct UIFrameWnd *f = self;
    int sx, sy, dx, dy, snap_dx, snap_dy, group_dx, group_dy;

    if (ui_window_mgr_get_capture(&g_windowMgr) != wnd)
        return;
    if (x <= -g_displayWidth || y <= -g_displayHeight)
        return;
    sx = x;
    sy = y;
    ui_window_to_screen(wnd, &sx, &sy);
    dx = sx - f->press_x;
    dy = sy - f->press_y;
    if (!snap_mgr_is_registered_to_snap(&g_snapMgr, wnd)) {
        ui_window_move(wnd, f->saved_x + dx, f->saved_y + dy);
        return;
    }
    ui_frame_wnd_move_delta(wnd, dx, dy);
    ui_window_mgr_get_snap_delta(&g_windowMgr, wnd, &snap_dx, &snap_dy, 0);
    group_dx = dx;
    group_dy = dy;
    snap_mgr_drag_group(&g_snapMgr, wnd, &group_dx, &group_dy);
    group_dx = larger_shift(group_dx, snap_dx);
    group_dy = larger_shift(group_dy, snap_dy);
    ui_frame_wnd_move_delta(wnd, group_dx + dx, group_dy + dy);
    ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
    snap_mgr_move_group(&g_snapMgr, wnd, group_dx + dx, group_dy + dy);
}

void ui_frame_wnd_on_mouse_hover(struct UIWindow *self, int x_unused, int y_unused)
{
}

void ui_frame_wnd_on_l_btn_up(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    struct UIFrameWnd *f = self;

    if (ui_window_mgr_get_capture(&g_windowMgr) != &self->base)
        return;
    g_windowMgr.last_id = -1;
    f->base.alpha_target = 0xff;
    f->base.alpha_time = timeGetTime();
    ui_window_mgr_release_capture(&g_windowMgr);
    snap_mgr_relink_all(&g_snapMgr, &self->base);
}

void ui_frame_wnd_snap(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    int dx, dy;

    ui_window_mgr_get_snap_delta(&g_windowMgr, &self->base, &dx, &dy, 0);
    self->base.x += dx;
    self->base.y += dy;
    ui_window_mgr_update_snap_rect(&g_windowMgr, &self->base);
}
