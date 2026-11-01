#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include <string.h>
#include "ui/chat/chat.h"
#include "ui/accept_wnd.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define JOIN_FRIEND_MSG_ASKER 0x5f
#define REQ_JOIN_FRIEND_ANSWER 0xaf
#define MSG_JOIN_FRIEND_QUESTION 0x332

struct UIJoinFriendAcceptWnd;
struct UIWindow;
void ui_join_friend_accept_wnd_destroy(struct UIWindow *window);
int ui_join_friend_accept_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_join_friend_accept_wnd_ops = {
    .destroy = ui_join_friend_accept_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_exchange_accept_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_exchange_accept_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_accept_wnd_on_draw,
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
    .send_msg = ui_join_friend_accept_wnd_send_msg,
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

struct UIJoinFriendAcceptWnd {
    struct UIAcceptWnd accept;
    int asker_id1;
    int asker_id2;
};
OFFSET_ASSERT(UIJoinFriendAcceptWnd, asker_id1, 0xa0);
SIZE_ASSERT(UIJoinFriendAcceptWnd, 0xa8);

struct UIJoinFriendAcceptWnd *ui_join_friend_accept_wnd_construct(struct UIJoinFriendAcceptWnd *self)
{
    accept_construct(&self->accept, &ui_join_friend_accept_wnd_ops);
    return self;
}

struct UIWindow *ui_join_friend_accept_wnd_new(void)
{
    return (struct UIWindow *)ui_join_friend_accept_wnd_construct(mem_alloc(sizeof(struct UIJoinFriendAcceptWnd)));
}

void ui_join_friend_accept_wnd_destroy(struct UIWindow *window)
{
    struct UIJoinFriendAcceptWnd *self = (struct UIJoinFriendAcceptWnd *)window;
    accept_destruct(&self->accept, &ui_join_friend_accept_wnd_ops);
    mem_free(self);
}

int ui_join_friend_accept_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             int val1, int val2, int val3, int val4)
{
    struct UIJoinFriendAcceptWnd *self = (struct UIJoinFriendAcceptWnd *)window;
    char line[0x80];

    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_JOIN_FRIEND_ACCEPT);
            mode_send_message(g_modeMgr.curMode, REQ_JOIN_FRIEND_ANSWER, self->asker_id1, self->asker_id2, 1, 0);
        } else if (val1 == UI_ID_CANCEL) {
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_JOIN_FRIEND_ACCEPT);
            mode_send_message(g_modeMgr.curMode, REQ_JOIN_FRIEND_ANSWER, self->asker_id1, self->asker_id2, 0, 0);
        }
        return 0;
    case JOIN_FRIEND_MSG_ASKER:
        self->asker_id1 = val1;
        self->asker_id2 = val2;
        sprintf(line, msg_str(MSG_JOIN_FRIEND_QUESTION), (const char *)val3);
        ui_list_box_clear((struct UIListBox *)(self->accept.text));
        ui_window_send_msg(self->accept.text, NULL, UI_MSG_ADD_TEXT, (int)line, 0x26, 0, 0);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->accept.frame), sender, msg, val1, val2, val3, val4);
}
