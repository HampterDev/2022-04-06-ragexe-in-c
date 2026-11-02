#include "ui/window.h"
#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include <stdlib.h>
#include "core/object_type_tables.h"
#include "session/session_items.h"
#include "ui/render/graphic_setting.h"
#include "platform/clock.h"

#define WND_ITEM_COLLECTION 0xc
#define WND_ITEM_COLLECTION_CMP 0xea
#define UI_MSG_COLLECTION_ITEM 0x18

struct ItemInfo;

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
    struct ItemInfo *item, *worn;
    struct UIWindow *wnd;
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
        if (!item)
            return 0;
        wnd = ui_window_open(WND_ITEM_COLLECTION);
        ui_window_send(wnd, NULL, UI_MSG_COLLECTION_ITEM, (int)item, 0, 0, 0);
        if (!g_windowMgr.item_collection_wnd)
            return 0;
        worn = session_get_worn_counterpart(&g_session1, item);
        if (!g_session1.itemCompare || !worn || val4 != 0)
            return 0;
        wnd = ui_window_open(WND_ITEM_COLLECTION_CMP);
        if (wnd)
            ui_window_send(wnd, NULL, UI_MSG_COLLECTION_ITEM, (int)worn, 1, 0, 0);
        else
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ITEM_COLLECTION_CMP);
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
    f->base.alpha_time = platform_ticks_ms();
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
    f->base.alpha_time = platform_ticks_ms();
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
