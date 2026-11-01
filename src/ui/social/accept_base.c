#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include <string.h>
#include "game/context.h"
#include "ui/social/social.h"
#include "ui/accept_wnd.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"

struct UIAcceptWnd;
struct UIWindow;
void ui_choose_sell_buy_wnd_on_create(struct UIWindow *self, int width, int height);
int ui_choose_sell_buy_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

void ui_choose_sell_buy_wnd_destroy(struct UIWindow *window);
const struct UIWindowOps ui_choose_sell_buy_wnd_ops = {
    .destroy = ui_choose_sell_buy_wnd_destroy,
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
    .on_create = ui_choose_sell_buy_wnd_on_create,
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
    .send_msg = ui_choose_sell_buy_wnd_send_msg,
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

int ui_exchange_accept_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

void ui_exchange_accept_wnd_destroy(struct UIWindow *window);
const struct UIWindowOps ui_exchange_accept_wnd_ops = {
    .destroy = ui_exchange_accept_wnd_destroy,
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
    .send_msg = ui_exchange_accept_wnd_send_msg,
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

#define CHOOSE_SELL_BUY_WND_ID 0x19
#define EXCHANGE_ACCEPT_WND_ID 0x20

#define NPC_SHOP_WND_A_ID 0x16
#define NPC_SHOP_WND_B_ID 0x17
#define NPC_SHOP_WND_C_ID 0x18

#define ID_BUY 0xd8
#define ID_SELL 0xd7

#define MSG_CHOOSER_REQUESTER 0x1c
#define MSG_EXCHANGE_ASKER 0x32

#define REQ_NPC_SELL 0x25
#define REQ_NPC_BUY 0x26
#define REQ_NPC_CANCEL 0x28
#define REQ_EXCHANGE_ANSWER 0x32
#define EXCHANGE_ACCEPTED 3
#define EXCHANGE_REFUSED 4

#define MSG_CHOOSE_SELL_BUY_QUESTION 0x5c
#define MSG_EXCHANGE_QUESTION 0x5d

#define ACCEPT_INFO(self) (*(struct WindowInfo **)&(self)->info)

static void accept_add_text(struct UIAcceptWnd *self, int width, int height)
{
    self->text = ui_add_text_viewer(&self->frame.base, 10, 0x16, width - 0x14, height - 0x37);
}

struct UIAcceptWnd *ui_choose_sell_buy_wnd_construct(struct UIAcceptWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_choose_sell_buy_wnd_ops;
    self->requester = 0;
    return self;
}

struct UIWindow *ui_choose_sell_buy_wnd_new(void)
{
    return (struct UIWindow *)ui_choose_sell_buy_wnd_construct(mem_alloc(sizeof(struct UIAcceptWnd)));
}

void ui_choose_sell_buy_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    static const char *const names[3][3] = {
        { "btn_buy", "btn_buy_a", "btn_buy_b" },
        { "btn_sell", "btn_sell_a", "btn_sell_b" },
        { "btn_cancel", "btn_cancel_a", "btn_cancel_b" },
    };
    static const int pos[3][2] = { { 0x90, 0x60 }, { 0xbd, 0x60 }, { 0xea, 0x60 } };
    static const int ids[3] = { ID_BUY, ID_SELL, UI_ID_CANCEL };
    struct String dir;
    int i;

    ui_skin_dir(&dir);
    for (i = 0; i < 3; i++)
        ui_add_named_skin_button(&self->frame.base, &dir, names[i], pos[i][0], pos[i][1], ids[i]);
    self->frame.default_id = ID_BUY;
    accept_add_text(self, width, height);
    ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)msg_str(MSG_CHOOSE_SELL_BUY_QUESTION), 0x26, 0, 0);
    string_free(&dir);
}

void ui_accept_wnd_on_draw(struct UIWindow *window)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    ui_window_draw_bitmap(&self->frame.base, 0, 0, ui_skin_bitmap(UI_SKIN "win_msgbox.bmp"), 0);
}

int ui_choose_sell_buy_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                          int val2, int val3, int val4)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:

        if (!self->frame.base.visible)
            return 0;
        if (val1 == UI_ID_CANCEL) {
            mode_send_message(client_current_mode(), REQ_NPC_CANCEL, 0, 0, 0, 0);
            ui_close_window(CHOOSE_SELL_BUY_WND_ID);
        } else if (val1 == ID_SELL || val1 == ID_BUY) {
            mode_send_message(client_current_mode(), val1 == ID_SELL ? REQ_NPC_SELL : REQ_NPC_BUY, self->requester, 0, 0, 0);
            ui_window_show(ui_window_find(CHOOSE_SELL_BUY_WND_ID), 0);
        } else {
            return 0;
        }
        session_clear_lists(&g_session1);
        return 0;
    case MSG_CHOOSER_REQUESTER:
        self->requester = val1;
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UIAcceptWnd *ui_exchange_accept_wnd_construct(struct UIAcceptWnd *self)
{
    return accept_construct(self, &ui_exchange_accept_wnd_ops);
}

struct UIWindow *ui_exchange_accept_wnd_new(void)
{
    return (struct UIWindow *)ui_exchange_accept_wnd_construct(mem_alloc(sizeof(struct UIAcceptWnd)));
}

void ui_exchange_accept_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    static const char *const names[2][3] = {
        { "btn_ok", "btn_ok_a", "btn_ok_b" },
        { "btn_cancel", "btn_cancel_a", "btn_cancel_b" },
    };
    static const int pos[2][2] = { { 0xbd, 0x60 }, { 0xea, 0x60 } };
    static const int ids[2] = { UI_ID_OK, UI_ID_CANCEL };
    struct String dir;
    int i;

    ui_skin_dir(&dir);
    for (i = 0; i < 2; i++)
        ui_add_named_skin_button(&self->frame.base, &dir, names[i], pos[i][0], pos[i][1], ids[i]);
    self->frame.default_id = UI_ID_OK;
    accept_add_text(self, width, height);
    ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)"deal", 0x26, 0, 0);
    string_free(&dir);
}

void ui_exchange_accept_wnd_store_info(struct UIWindow *window)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    struct WindowInfo *info = ACCEPT_INFO(self);

    if (!info)
        return;
    info->x = self->frame.base.x;
    info->y = self->frame.base.y;
    info->width = self->frame.base.width;
    info->height = self->frame.base.height;
}

static int exchange_info_usable(const struct WindowInfo *info)
{
    if (info->width != 0x118 || info->height != 0x78)
        return 0;
    if (info->x < 0 || info->x > g_renderer->width - 0x118)
        return 0;
    if (info->y < 0 || info->y > g_renderer->height - 0x78)
        return 0;
    return 1;
}

static void exchange_info_reset(struct WindowInfo *info)
{
    info->x = 200;
    info->y = 200;
    info->width = 0x118;
    info->height = 0x78;
}

static void trade_code_letters(struct String *out, int number)
{
    static const char letters[] = "ROHUTNASEW";
    int div = 100000000;
    int started = 0;

    do {
        int digit = 0;

        while (number >= div) {
            number -= div;
            digit++;
        }
        div /= 10;
        if (digit > 0)
            started = 1;
        if (digit == 0) {
            if (started > 0)
                string_append(out, &letters[0], 1);
        } else if (digit <= 9) {
            string_append(out, &letters[digit], 1);
        }
    } while (div > 0);
}

int ui_exchange_accept_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                           int val2, int val3, int val4)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            ui_close_window(EXCHANGE_ACCEPT_WND_ID);
            mode_send_message(client_current_mode(), REQ_EXCHANGE_ANSWER, EXCHANGE_ACCEPTED, 0, 0, 0);
        } else if (val1 == UI_ID_CANCEL) {
            ui_close_window(EXCHANGE_ACCEPT_WND_ID);
            mode_send_message(client_current_mode(), REQ_EXCHANGE_ANSWER, EXCHANGE_REFUSED, 0, 0, 0);
        }
        return 0;

    case UI_MSG_SET_INFO:
        ACCEPT_INFO(self) = (struct WindowInfo *)val1;
        if (!exchange_info_usable(ACCEPT_INFO(self)))
            exchange_info_reset(ACCEPT_INFO(self));
        ui_frame_wnd_move(&self->frame.base, ACCEPT_INFO(self)->x, ACCEPT_INFO(self)->y);
        return 0;

    case MSG_EXCHANGE_ASKER: {
        const char *name = (const char *)val1;
        const char *q;
        struct String text;

        string_init(&text);
        string_assign_cstr(&text, "(");
        string_append_cstr(&text, name);
        string_append_cstr(&text, ")");
        q = msg_str(MSG_EXCHANGE_QUESTION);
        string_append_cstr(&text, q);
        ui_list_box_clear((struct UIListBox *)self->text);
        ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)string_cstr(&text), 0x8a, 0, 0);
        if (val2 > 0 && (unsigned)(val3 - 1) <= 0x62) {
            struct String line;
            char level[10];

            string_init(&line);
            string_assign_cstr(&line, "PN : ");
            trade_code_letters(&line, val2);
            if (g_serviceType == 6)
                string_clear(&line);
            if (g_serviceType == 12)
                string_append_cstr(&line, "     Nv.");
            else
                string_append_cstr(&line, "     Lv.");
            memset(level, 0, sizeof level);
            sprintf(level, "%d", val3);
            string_append_cstr(&line, level);
            ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)string_cstr(&line), 0x8a, 0, 0);
            string_free(&line);
        }
        string_free(&text);
        return 0;
    }
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_choose_sell_buy_wnd_destroy(struct UIWindow *window)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    self->frame.base.ops = &ui_choose_sell_buy_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_exchange_accept_wnd_destroy(struct UIWindow *window)
{
    struct UIAcceptWnd *self = (struct UIAcceptWnd *)window;
    accept_destruct(self, &ui_exchange_accept_wnd_ops);
    mem_free(self);
}
