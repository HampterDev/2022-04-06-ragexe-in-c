#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include "ui/items/items.h"
#include "ui/accept_wnd.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"

#define UI_WND_YOUR_ITEM 0x8a
#define UI_WND_MIX_ACCEPT 0xa3
#define UI_WND_MIX_CLOSES_TOO_1 0xa0
#define UI_WND_MIX_CLOSES_TOO_2 0xa1

#define UI_MSG_ASK 0x5b
#define ACCEPT_TEXT_STYLE 0x26
#define MIX_ACCEPT_MSG_QUESTION 0x630
#define YOUR_ITEM_MSG_QUESTION 0x52f
#define REQ_MIX_ACCEPT 0x10a
#define PACKET_YOUR_ITEM_ANSWER 0x998

static void accept_show_question(struct UIAcceptWnd *self, int msg_id)
{
    struct String text;
    const char *line;

    string_init(&text);
    line = msg_str(msg_id);
    string_assign_cstr(&text, line);
    ui_list_box_clear((struct UIListBox *)self->text);
    ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (intptr_t)string_cstr(&text), ACCEPT_TEXT_STYLE, 0, 0);
    string_free(&text);
}

struct UIMixAcceptWnd {
    struct UIAcceptWnd accept;
    BYTE ask_kind;
};

struct UIWindow;
void ui_mix_accept_wnd_destroy(struct UIWindow *window);
intptr_t ui_mix_accept_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

const struct UIWindowOps ui_mix_accept_wnd_ops = {
    .destroy = ui_mix_accept_wnd_destroy,
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
    .send_msg = ui_mix_accept_wnd_send_msg,
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

struct UIMixAcceptWnd *ui_mix_accept_wnd_construct(struct UIMixAcceptWnd *self)
{
    accept_construct(&self->accept, &ui_mix_accept_wnd_ops);
    return self;
}

struct UIWindow *ui_mix_accept_wnd_new(void)
{
    return (struct UIWindow *)ui_mix_accept_wnd_construct(mem_alloc(sizeof(struct UIMixAcceptWnd)));
}

void ui_mix_accept_wnd_destruct(struct UIMixAcceptWnd *self)
{
    accept_destruct(&self->accept, &ui_mix_accept_wnd_ops);
}

void ui_mix_accept_wnd_destroy(struct UIWindow *window)
{
    struct UIMixAcceptWnd *self = (struct UIMixAcceptWnd *)window;
    ui_mix_accept_wnd_destruct(self);
    mem_free(self);
}

intptr_t ui_mix_accept_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                      intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMixAcceptWnd *self = (struct UIMixAcceptWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_OK) {
            struct Mode *mode = g_modeMgr.curMode;

            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_MIX_CLOSES_TOO_1);
            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_MIX_CLOSES_TOO_2);
            mode_send_message(mode, REQ_MIX_ACCEPT, 0, 0, 0, 0);
            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_MIX_ACCEPT);
            session_clear_lists(&g_session1);
        } else if (val1 == UI_ID_CANCEL) {
            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_MIX_ACCEPT);
        }
        return 0;

    case UI_MSG_ASK:
        self->ask_kind = (BYTE)val1;
        accept_show_question(&self->accept, MIX_ACCEPT_MSG_QUESTION);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->accept.frame), sender, msg, val1, val2, val3, val4);
}

struct UIYourItemWnd {
    struct UIAcceptWnd accept;
    int value1;
    int value2;
};

#pragma pack(push, 1)
struct YourItemAnswer {
    unsigned short packet_id;
    unsigned short value1;
    int value2;
};
#pragma pack(pop)
SIZE_ASSERT(YourItemAnswer, 8);

void ui_your_item_wnd_destroy(struct UIWindow *window);
intptr_t ui_your_item_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_your_item_wnd_ops = {
    .destroy = ui_your_item_wnd_destroy,
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
    .send_msg = ui_your_item_wnd_send_msg,
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

struct UIYourItemWnd *ui_your_item_wnd_construct(struct UIYourItemWnd *self)
{
    accept_construct(&self->accept, &ui_your_item_wnd_ops);
    return self;
}

struct UIWindow *ui_your_item_wnd_new(void)
{
    return (struct UIWindow *)ui_your_item_wnd_construct(mem_alloc(sizeof(struct UIYourItemWnd)));
}

void ui_your_item_wnd_destruct(struct UIYourItemWnd *self)
{
    accept_destruct(&self->accept, &ui_your_item_wnd_ops);
}

void ui_your_item_wnd_destroy(struct UIWindow *window)
{
    struct UIYourItemWnd *self = (struct UIYourItemWnd *)window;
    ui_your_item_wnd_destruct(self);
    mem_free(self);
}

intptr_t ui_your_item_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                     intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIYourItemWnd *self = (struct UIYourItemWnd *)window;
    struct YourItemAnswer answer;

    switch (msg) {
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_OK) {
            game_current_base_mode();
            answer.packet_id = PACKET_YOUR_ITEM_ANSWER;
            answer.value1 = (unsigned short)self->value1;
            answer.value2 = self->value2;
            rag_connection_send_packet(rag_connection_instance(),
                                      rag_connection_get_packet_length(rag_connection_instance(),
                                                                     PACKET_YOUR_ITEM_ANSWER),
                                      &answer);
            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_YOUR_ITEM);
        } else if (val1 == UI_ID_CANCEL) {
            ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_YOUR_ITEM);
        }
        return 0;

    case UI_MSG_ASK:
        accept_show_question(&self->accept, YOUR_ITEM_MSG_QUESTION);
        self->value1 = val1;
        self->value2 = val2;
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->accept.frame), sender, msg, val1, val2, val3, val4);
}
