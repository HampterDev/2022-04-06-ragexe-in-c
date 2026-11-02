#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/social/social.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define GUILD_LEAVE_REASON_WND_ID 0x4b
#define GUILD_BAN_REASON_WND_ID 0x4c

#define MSG_GUILD_LEAVE_REASON 0x20b
#define MSG_GUILD_BAN_REASON 0x20c

#define REQ_GUILD_LEAVE 0x6e
#define REQ_GUILD_EXPEL 0x6f

#define MSG_GUILD_BAN_MEMBER UI_MSG_SET_INFO

struct UIGuildLeaveReasonDescWnd {
    struct UIFrameWnd frame;
    struct UIWindow *reason;
    struct UIWindow *caption;
};

struct UIGuildBanReasonDescWnd {
    struct UIGuildLeaveReasonDescWnd leave;
    struct GuildBanTarget member;
};

void ui_guild_leave_reason_desc_wnd_destroy(struct UIWindow *window);
void ui_guild_leave_reason_desc_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_guild_reason_wnd_on_draw(struct UIWindow *self);
intptr_t ui_guild_leave_reason_desc_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_guild_leave_reason_desc_wnd_ops = {
    .destroy = ui_guild_leave_reason_desc_wnd_destroy,
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
    .on_create = ui_guild_leave_reason_desc_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_guild_reason_wnd_on_draw,
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
    .send_msg = ui_guild_leave_reason_desc_wnd_send_msg,
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
void ui_guild_ban_reason_desc_wnd_destroy(struct UIWindow *window);
void ui_guild_ban_reason_desc_wnd_on_create(struct UIWindow *self, int width, int height);
intptr_t ui_guild_ban_reason_desc_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_guild_ban_reason_desc_wnd_ops = {
    .destroy = ui_guild_ban_reason_desc_wnd_destroy,
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
    .on_create = ui_guild_ban_reason_desc_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_guild_reason_wnd_on_draw,
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
    .send_msg = ui_guild_ban_reason_desc_wnd_send_msg,
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

struct UIGuildLeaveReasonDescWnd *ui_guild_leave_reason_desc_wnd_construct(
    struct UIGuildLeaveReasonDescWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_guild_leave_reason_desc_wnd_ops;
    self->reason = NULL;
    self->caption = NULL;
    return self;
}

struct UIWindow *ui_guild_leave_reason_desc_wnd_new(void)
{
    return (struct UIWindow *)ui_guild_leave_reason_desc_wnd_construct(mem_alloc(sizeof(struct UIGuildLeaveReasonDescWnd)));
}

void ui_guild_leave_reason_desc_wnd_destruct(struct UIGuildLeaveReasonDescWnd *self)
{
    self->frame.base.ops = &ui_guild_leave_reason_desc_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_guild_leave_reason_desc_wnd_destroy(struct UIWindow *window)
{
    struct UIGuildLeaveReasonDescWnd *self = (struct UIGuildLeaveReasonDescWnd *)window;
    ui_guild_leave_reason_desc_wnd_destruct(self);
    mem_free(self);
}

void ui_guild_leave_reason_desc_wnd_on_create(struct UIWindow *window, int width_unused,
                                                   int height_unused)
{
    struct UIGuildLeaveReasonDescWnd *self = (struct UIGuildLeaveReasonDescWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *reason, *caption;

    reason = mem_alloc(sizeof(struct UIEditCtrl));
    if (reason)
        reason = ui_edit_ctrl_construct(reason);
    self->reason = reason;
    ui_window_create(reason, wnd->width - 0x20, 0x10);
    ui_window_move(reason, 0xf, 0x16);
    UI_EDIT_MAX_CHARS(reason) = 0x27;
    ui_edit_ctrl_set_back_colour(reason, 0xe8, 0xe8, 0xe8);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)reason, "");
    ui_window_add_child(wnd, reason);
    ui_window_mgr_set_focus_edit(&g_windowMgr, reason);

    caption = mem_alloc(sizeof(struct UIStaticText));
    if (caption)
        caption = ui_static_text_construct(caption);
    self->caption = caption;
    ui_window_create(caption, wnd->width - 0x1e, 0xf);
    ui_window_move(caption, 0xf, 6);
    ui_static_text_set_text(caption, msg_str(MSG_GUILD_LEAVE_REASON), 0);
    ui_window_add_child(wnd, caption);

    ui_add_bitmap_button(wnd, UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp",
                         wnd->width - 0x65, wnd->height - 0x20, UI_ID_OK);
    ui_add_bitmap_button(wnd, UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp", UI_SKIN "btn_cancel_b.bmp",
                         wnd->width - 0x37, wnd->height - 0x20, UI_ID_CANCEL);
    UI_FRAME_DEFAULT_ID(&self->frame) = UI_ID_OK;
}

void ui_guild_reason_wnd_on_draw(struct UIWindow *self)
{
    ui_window_draw_sys_box(self, 0, 0, 0, 0);
}

intptr_t ui_guild_leave_reason_desc_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                                 int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIGuildLeaveReasonDescWnd *self = (struct UIGuildLeaveReasonDescWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    if (val1 == UI_ID_OK) {
        mode_send_message(client_current_mode(), REQ_GUILD_LEAVE, (intptr_t)ui_edit_ctrl_get_text(self->reason), 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, GUILD_LEAVE_REASON_WND_ID);
    } else if (val1 == UI_ID_CANCEL) {
        ui_window_mgr_post_quit1(&g_windowMgr, GUILD_LEAVE_REASON_WND_ID);
    }
    return 0;
}

struct UIGuildBanReasonDescWnd *ui_guild_ban_reason_desc_wnd_construct(struct UIGuildBanReasonDescWnd *self)
{
    ui_guild_leave_reason_desc_wnd_construct(&self->leave);
    self->leave.frame.base.ops = &ui_guild_ban_reason_desc_wnd_ops;
    return self;
}

struct UIWindow *ui_guild_ban_reason_desc_wnd_new(void)
{
    return (struct UIWindow *)ui_guild_ban_reason_desc_wnd_construct(mem_alloc(sizeof(struct UIGuildBanReasonDescWnd)));
}

static void ui_guild_ban_reason_desc_wnd_destruct(struct UIGuildBanReasonDescWnd *self)
{
    self->leave.frame.base.ops = &ui_guild_ban_reason_desc_wnd_ops;
    ui_guild_leave_reason_desc_wnd_destruct(&self->leave);
}

void ui_guild_ban_reason_desc_wnd_destroy(struct UIWindow *window)
{
    struct UIGuildBanReasonDescWnd *self = (struct UIGuildBanReasonDescWnd *)window;
    ui_guild_ban_reason_desc_wnd_destruct(self);
    mem_free(self);
}

void ui_guild_ban_reason_desc_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIGuildBanReasonDescWnd *self = (struct UIGuildBanReasonDescWnd *)window;
    ui_guild_leave_reason_desc_wnd_on_create((struct UIWindow *)(&self->leave), width, height);
    if (self->leave.caption)
        ui_static_text_set_text(self->leave.caption, msg_str(MSG_GUILD_BAN_REASON), 0);
}

intptr_t ui_guild_ban_reason_desc_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                               int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIGuildBanReasonDescWnd *self = (struct UIGuildBanReasonDescWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == UI_ID_OK) {
            ui_window_mgr_post_quit1(&g_windowMgr, GUILD_BAN_REASON_WND_ID);
            mode_send_message(client_current_mode(), REQ_GUILD_EXPEL, (intptr_t)&self->member,
                          (intptr_t)ui_edit_ctrl_get_text(self->leave.reason), 0, 0);
        } else if (val1 == UI_ID_CANCEL) {
            ui_window_mgr_post_quit1(&g_windowMgr, GUILD_BAN_REASON_WND_ID);
        }
        return 0;
    }
    if (msg == MSG_GUILD_BAN_MEMBER) {
        self->member = *(const struct GuildBanTarget *)val1;
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->leave.frame), sender, msg, val1, val2, val3, val4);
}
