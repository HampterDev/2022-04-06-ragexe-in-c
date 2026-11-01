#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include <string.h>
#include "ui/social/social.h"
#include "ui/accept_wnd.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define JOIN_PARTY_ACCEPT_WND_ID 0x23
#define JOIN_GUILD_ACCEPT_WND_ID 0x46
#define ALLY_GUILD_ACCEPT_WND_ID 0x48
#define COUPLE_ACCEPT_WND_ID 0x68
#define BABY_ACCEPT_WND2_ID 0x6e
#define JOIN_GUILD_CLOSES_TOO_1 0xd6
#define JOIN_GUILD_CLOSES_TOO_2 0xd5
#define JOIN_GUILD_CLOSES_TOO_3 0xd4

#define MSG_JOIN_PARTY_ASKER 0x38
#define MSG_JOIN_GUILD_ASKER 0x49
#define MSG_ALLY_GUILD_ASKER 0x4a
#define MSG_COUPLE_ASKER 0x5a
#define MSG_BABY2_ASKER 0x5b

#define REQ_JOIN_PARTY_ANSWER 0x3c
#define REQ_JOIN_GUILD_ANSWER 0x72
#define REQ_ALLY_GUILD_ANSWER 0x78
#define REQ_COUPLE_ANSWER 0xa7
#define REQ_BABY2_ANSWER 0xad

#define MSG_JOIN_PARTY_QUESTION 0x5e
#define MSG_JOIN_GUILD_QUESTION 0x179
#define MSG_ALLY_GUILD_QUESTION 0x189
#define MSG_BABY_QUESTION 0x31a
#define MSG_COUPLE_QUESTION 0x2e1
#define MSG_BABY2_QUESTION 0xb95

struct UIWindow;
void ui_join_party_accept_wnd_destroy(struct UIWindow *window);
int ui_join_party_accept_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_join_party_accept_wnd_ops = {
    .destroy = ui_join_party_accept_wnd_destroy,
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
    .send_msg = ui_join_party_accept_wnd_send_msg,
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
void ui_couple_accept_wnd_destroy(struct UIWindow *window);
int ui_couple_accept_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_couple_accept_wnd_ops = {
    .destroy = ui_couple_accept_wnd_destroy,
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
    .send_msg = ui_couple_accept_wnd_send_msg,
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
void ui_baby_accept_wnd_destroy(struct UIWindow *window);
int ui_baby_accept_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_baby_accept_wnd_ops = {
    .destroy = ui_baby_accept_wnd_destroy,
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
    .send_msg = ui_baby_accept_wnd_send_msg,
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
void ui_baby_accept_wnd2_destroy(struct UIWindow *window);
int ui_baby_accept_wnd2_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_baby_accept_wnd2_ops = {
    .destroy = ui_baby_accept_wnd2_destroy,
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
    .send_msg = ui_baby_accept_wnd2_send_msg,
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
void ui_join_guild_accept_wnd_destroy(struct UIWindow *window);
int ui_join_guild_accept_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_join_guild_accept_wnd_ops = {
    .destroy = ui_join_guild_accept_wnd_destroy,
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
    .send_msg = ui_join_guild_accept_wnd_send_msg,
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
void ui_ally_guild_accept_wnd_destroy(struct UIWindow *window);
int ui_ally_guild_accept_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_ally_guild_accept_wnd_ops = {
    .destroy = ui_ally_guild_accept_wnd_destroy,
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
    .send_msg = ui_ally_guild_accept_wnd_send_msg,
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

static void accept_show(struct UIAcceptWnd *self, const char *name, int question)
{
    struct String text;
    const char *q;

    string_init(&text);
    if (name) {
        string_assign_cstr(&text, "(");
        string_append_cstr(&text, name);
        string_append_cstr(&text, ")");
        q = msg_str(question);
        string_append_cstr(&text, q);
    } else {
        q = msg_str(question);
        string_assign_cstr(&text, q);
    }
    ui_list_box_clear((struct UIListBox *)self->text);
    ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)string_cstr(&text), 0x26, 0, 0);
    string_free(&text);
}

static void accept_answer(struct UIAcceptWnd *self, int id, int wnd_id, int request)
{
    if (id == UI_ID_OK) {
        ui_window_mgr_post_quit1(&g_windowMgr, wnd_id);
        mode_send_message(client_current_mode(), request, self->requester, 1, 0, 0);
    } else if (id == UI_ID_CANCEL) {
        ui_window_mgr_post_quit1(&g_windowMgr, wnd_id);
        mode_send_message(client_current_mode(), request, self->requester, 0, 0, 0);
    }
}

struct UIAcceptWnd *ui_join_party_accept_wnd_construct(struct UIAcceptWnd *self)
{
    return accept_construct(self, &ui_join_party_accept_wnd_ops);
}

struct UIWindow *ui_join_party_accept_wnd_new(void)
{
    return (struct UIWindow *)ui_join_party_accept_wnd_construct(mem_alloc(sizeof(struct UIAcceptWnd)));
}

static void ui_join_party_accept_wnd_destruct(struct UIAcceptWnd *self)
{
    accept_destruct(self, &ui_join_party_accept_wnd_ops);
}

void ui_join_party_accept_wnd_destroy(struct UIWindow *window)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    ui_join_party_accept_wnd_destruct(self);
    mem_free(self);
}

int ui_join_party_accept_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                            int val2, int val3, int val4)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:

        accept_answer(self, val1, JOIN_PARTY_ACCEPT_WND_ID, REQ_JOIN_PARTY_ANSWER);
        return 0;
    case MSG_JOIN_PARTY_ASKER:
        self->requester = val1;
        accept_show(self, (const char *)val2, MSG_JOIN_PARTY_QUESTION);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UIAcceptWnd *ui_join_guild_accept_wnd_construct(struct UIAcceptWnd *self)
{
    return accept_construct(self, &ui_join_guild_accept_wnd_ops);
}

struct UIWindow *ui_join_guild_accept_wnd_new(void)
{
    return (struct UIWindow *)ui_join_guild_accept_wnd_construct(mem_alloc(sizeof(struct UIAcceptWnd)));
}

static void ui_join_guild_accept_wnd_destruct(struct UIAcceptWnd *self)
{
    accept_destruct(self, &ui_join_guild_accept_wnd_ops);
}

void ui_join_guild_accept_wnd_destroy(struct UIWindow *window)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    ui_join_guild_accept_wnd_destruct(self);
    mem_free(self);
}

int ui_join_guild_accept_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                            int val2, int val3, int val4)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            ui_window_mgr_post_quit1(&g_windowMgr, JOIN_GUILD_ACCEPT_WND_ID);
            ui_window_mgr_post_quit1(&g_windowMgr, JOIN_GUILD_CLOSES_TOO_1);
            ui_window_mgr_post_quit1(&g_windowMgr, JOIN_GUILD_CLOSES_TOO_2);
            ui_window_mgr_post_quit1(&g_windowMgr, JOIN_GUILD_CLOSES_TOO_3);
            mode_send_message(client_current_mode(), REQ_JOIN_GUILD_ANSWER, self->requester, 1, 0, 0);
        } else if (val1 == UI_ID_CANCEL) {
            ui_window_mgr_post_quit1(&g_windowMgr, JOIN_GUILD_ACCEPT_WND_ID);
            mode_send_message(client_current_mode(), REQ_JOIN_GUILD_ANSWER, self->requester, 0, 0, 0);
        }
        return 0;
    case MSG_JOIN_GUILD_ASKER: {
        char name[0x104];

        memset(name, 0, sizeof name);
        strcpy_s(name, sizeof name, (const char *)val2);
        self->requester = val1;
        accept_show(self, name, MSG_JOIN_GUILD_QUESTION);
        return 0;
    }
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UIAcceptWnd *ui_ally_guild_accept_wnd_construct(struct UIAcceptWnd *self)
{
    return accept_construct(self, &ui_ally_guild_accept_wnd_ops);
}

struct UIWindow *ui_ally_guild_accept_wnd_new(void)
{
    return (struct UIWindow *)ui_ally_guild_accept_wnd_construct(mem_alloc(sizeof(struct UIAcceptWnd)));
}

static void ui_ally_guild_accept_wnd_destruct(struct UIAcceptWnd *self)
{
    accept_destruct(self, &ui_ally_guild_accept_wnd_ops);
}

void ui_ally_guild_accept_wnd_destroy(struct UIWindow *window)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    ui_ally_guild_accept_wnd_destruct(self);
    mem_free(self);
}

int ui_ally_guild_accept_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                            int val2, int val3, int val4)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        accept_answer(self, val1, ALLY_GUILD_ACCEPT_WND_ID, REQ_ALLY_GUILD_ANSWER);
        return 0;
    }
    if (msg == MSG_ALLY_GUILD_ASKER) {
        self->requester = val1;
        accept_show(self, (const char *)val2, MSG_ALLY_GUILD_QUESTION);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UIAcceptWnd *ui_couple_accept_wnd_construct(struct UIAcceptWnd *self)
{
    return accept_construct(self, &ui_couple_accept_wnd_ops);
}

struct UIWindow *ui_couple_accept_wnd_new(void)
{
    return (struct UIWindow *)ui_couple_accept_wnd_construct(mem_alloc(sizeof(struct UIAcceptWnd)));
}

static void ui_couple_accept_wnd_destruct(struct UIAcceptWnd *self)
{
    accept_destruct(self, &ui_couple_accept_wnd_ops);
}

void ui_couple_accept_wnd_destroy(struct UIWindow *window)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    ui_couple_accept_wnd_destruct(self);
    mem_free(self);
}

int ui_couple_accept_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        accept_answer(self, val1, COUPLE_ACCEPT_WND_ID, REQ_COUPLE_ANSWER);
        return 0;
    }
    if (msg == MSG_COUPLE_ASKER) {
        self->requester = val1;
        accept_show(self, (const char *)val2, MSG_COUPLE_QUESTION);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UIAcceptWnd *ui_baby_accept_wnd_construct(struct UIAcceptWnd *self)
{
    return accept_construct(self, &ui_baby_accept_wnd_ops);
}

struct UIWindow *ui_baby_accept_wnd_new(void)
{
    return (struct UIWindow *)ui_baby_accept_wnd_construct(mem_alloc(sizeof(struct UIAcceptWnd)));
}

static void ui_baby_accept_wnd_destruct(struct UIAcceptWnd *self)
{
    accept_destruct(self, &ui_baby_accept_wnd_ops);
}

void ui_baby_accept_wnd_destroy(struct UIWindow *window)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    ui_baby_accept_wnd_destruct(self);
    mem_free(self);
}

struct UIAcceptWnd *ui_baby_accept_wnd2_construct(struct UIAcceptWnd *self)
{
    return accept_construct(self, &ui_baby_accept_wnd2_ops);
}

struct UIWindow *ui_baby_accept_wnd2_new(void)
{
    return (struct UIWindow *)ui_baby_accept_wnd2_construct(mem_alloc(sizeof(struct UIAcceptWnd)));
}

static void ui_baby_accept_wnd2_destruct(struct UIAcceptWnd *self)
{
    accept_destruct(self, &ui_baby_accept_wnd2_ops);
}

void ui_baby_accept_wnd2_destroy(struct UIWindow *window)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    ui_baby_accept_wnd2_destruct(self);
    mem_free(self);
}

int ui_baby_accept_wnd2_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                        int val2, int val3, int val4)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:

        accept_answer(self, val1, BABY_ACCEPT_WND2_ID, REQ_BABY2_ANSWER);
        return 0;
    case MSG_BABY2_ASKER:
        self->requester = val1;
        accept_show(self, NULL, MSG_BABY2_QUESTION);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

#define BABY_ACCEPT_WND_ID 0x69
#define MSG_BABY_ASKER 0x5b

int ui_baby_accept_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            ui_close_window(BABY_ACCEPT_WND_ID);
            ui_window_send(ui_window_open(BABY_ACCEPT_WND2_ID), NULL, MSG_BABY2_ASKER, UI_ID_OK, val2, 0, 0);
        } else if (val1 == UI_ID_CANCEL) {
            ui_close_window(BABY_ACCEPT_WND_ID);
            mode_send_message(client_current_mode(), REQ_BABY2_ANSWER, self->requester, 0, 0, 0);
        }
        return 0;
    case MSG_BABY_ASKER:
        self->requester = val1;
        accept_show(self, (const char *)val2, MSG_BABY_QUESTION);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

#define STAR_PLACE_ACCEPT_WND_ID 0x77
#define MSG_STAR_PLACE_KIND 0x5b
#define REQ_STAR_PLACE_ANSWER 0xbe
#define MSG_STAR_PLACE_QUESTION 0x404
#define MSG_STAR_PLACE_GEMSTONE_QUESTION 0x4ef

struct UIStarPlaceAcceptWnd {
    struct UIAcceptWnd accept;
    char kind;
};
OFFSET_ASSERT(UIStarPlaceAcceptWnd, kind, 0xa0);

struct UIStarPlaceAcceptWnd *ui_star_place_accept_wnd_construct(struct UIStarPlaceAcceptWnd *self)
{
    accept_construct(&self->accept, &ui_star_place_accept_wnd_ops);
    return self;
}

struct UIWindow *ui_star_place_accept_wnd_new(void)
{
    return (struct UIWindow *)ui_star_place_accept_wnd_construct(mem_alloc(sizeof(struct UIStarPlaceAcceptWnd)));
}

int ui_star_place_accept_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                            int val1, int val2, int val3, int val4)
{
    struct UIStarPlaceAcceptWnd *self = (struct UIStarPlaceAcceptWnd *)window;
    struct UIAcceptWnd *accept = &self->accept;

    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            struct Mode *mode = game_current_base_mode();

            ui_close_window(STAR_PLACE_ACCEPT_WND_ID);
            mode_send_message(mode, REQ_STAR_PLACE_ANSWER, self->kind, 0, 0, 0);
        } else if (val1 == UI_ID_CANCEL) {
            ui_close_window(STAR_PLACE_ACCEPT_WND_ID);
        }
        return 0;
    case MSG_STAR_PLACE_KIND:
        self->kind = (char)val1;
        accept_show(accept, NULL, self->kind == 10 ? MSG_STAR_PLACE_GEMSTONE_QUESTION : MSG_STAR_PLACE_QUESTION);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&accept->frame), sender, msg, val1, val2, val3, val4);
}

struct UIAcceptWnd;
struct UIWindow;
void ui_star_place_accept_wnd_destroy(struct UIWindow *window);
int ui_star_place_accept_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_star_place_accept_wnd_ops = {
    .destroy = ui_star_place_accept_wnd_destroy,
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
    .send_msg = ui_star_place_accept_wnd_send_msg,
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

void ui_star_place_accept_wnd_destroy(struct UIWindow *window)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    accept_destruct(self, &ui_star_place_accept_wnd_ops);
    mem_free(self);
}
