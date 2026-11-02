#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/shops/shops.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define WND_ID_BANK 0x113
#define ZENY_MAX 0x7fffffffLL

struct UIBank_NewWnd;
struct UIWindow;
void ui_bank_new_wnd_destroy(struct UIWindow *window);
void ui_bank_new_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_bank_new_wnd_on_draw(struct UIWindow *self);
int ui_bank_new_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_bank_new_wnd_ops = {
    .destroy = ui_bank_new_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
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
    .on_create = ui_bank_new_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_bank_new_wnd_on_draw,
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
    .send_msg = ui_bank_new_wnd_send_msg,
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

enum {
    BANK_ID_STEP_UP = 0x10f,
    BANK_ID_STEP_DOWN = 0x110,
    BANK_ID_MAX = 0x17d,
    BANK_ID_ADD_10M = 0x17e,
    BANK_ID_ADD_1M = 0x17f,
    BANK_ID_ADD_100K = 0x180,
    BANK_ID_DEPOSIT = 0x1c6,
    BANK_ID_WITHDRAW = 0x1c7,
};

#define MSG_BANK_TITLE 0xad3
#define MSG_BANK_BALANCE 0xad4
#define MSG_BANK_PURSE 0xad5
#define MSG_BANK_DEPOSIT 0xad6
#define MSG_BANK_WITHDRAW 0xad7
#define MSG_BANK_STEP_UP 0xad8
#define MSG_BANK_STEP_DOWN 0xad9
#define MSG_BANK_MAX 0xada
#define MSG_BANK_NOT_ENOUGH_ZENY 0x998
#define MSG_BANK_NOTE_NOT_ENOUGH_ZENY 0xae1
#define MSG_BANK_NOT_ENOUGH_BANKED 0x997
#define MSG_BANK_NOTE_NOT_ENOUGH_BANKED 0xae2
#define MSG_BANK_PURSE_FULL 0x99b
#define MSG_BANK_FULL 0xae3
#define MSG_BANK_NOTE_FULL 0xadf
#define MSG_BANK_EMPTY 0xad2
#define MSG_BANK_NOTE_EMPTY 0xadc
#define MSG_BANK_NOTHING_LEFT 0x9a1
#define MSG_BANK_NO_AMOUNT 0xadb
#define MSG_BANK_NOTE_NO_AMOUNT 0xadd
#define MSG_BANK_NOT_A_NUMBER 0x9b8
#define MSG_BANK_NOTE_NOT_A_NUMBER 0xade
#define MSG_BANK_TOO_MUCH 0xad0
#define MSG_BANK_TOO_LITTLE 0xad1
#define MSG_BANK_NOTE_TOO_LITTLE 0xae0

#define PKT_CZ_REQ_BANKING_DEPOSIT 0x09a7
#define PKT_CZ_REQ_BANKING_WITHDRAW 0x09a9
#pragma pack(push, 1)
struct PacketBanking {
    unsigned short id;
    int account_id;
    int money;
};
#pragma pack(pop)
SIZE_ASSERT(PacketBanking, 10);

#define CHAT_RED RGB(0xff, 0, 0)

struct UIBank_NewWnd {
    struct UIFrameWnd frame;
    struct UIWindow *amount_edit;
    struct String notice;
};
OFFSET_ASSERT(UIBank_NewWnd, amount_edit, 0x94);
OFFSET_ASSERT(UIBank_NewWnd, notice, 0x98);

struct UIBank_NewWnd *ui_bank_new_wnd_construct(struct UIBank_NewWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_bank_new_wnd_ops;
    string_init(&self->notice);
    self->amount_edit = NULL;
    return self;
}

struct UIWindow *ui_bank_new_wnd_new(void)
{
    return (struct UIWindow *)ui_bank_new_wnd_construct(mem_alloc(sizeof(struct UIBank_NewWnd)));
}

static void ui_bank_new_wnd_destruct(struct UIBank_NewWnd *self)
{
    self->frame.base.ops = &ui_bank_new_wnd_ops;
    string_free(&self->notice);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_bank_new_wnd_destroy(struct UIWindow *window)
{
    struct UIBank_NewWnd *self = (struct UIBank_NewWnd *)window;
    ui_bank_new_wnd_destruct(self);
    mem_free(self);
}

void ui_bank_new_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIBank_NewWnd *self = (struct UIBank_NewWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *edit;

    ui_window_add_close_button(wnd, "닫기", 0);
    ui_window_add_bitmap_button(wnd, BANK_ID_DEPOSIT, 0xd1, 0x4d, "bank\\btn_deposit", msg_str(MSG_BANK_DEPOSIT));
    ui_window_add_bitmap_button(wnd, BANK_ID_WITHDRAW, 0xd1, 0x66, "bank\\btn_withdraw", msg_str(MSG_BANK_WITHDRAW));
    ui_window_add_bitmap_button(wnd, BANK_ID_STEP_UP, 0xb1, 0x59, "bank\\btn_upper", msg_str(MSG_BANK_STEP_UP));
    ui_window_add_bitmap_button(wnd, BANK_ID_STEP_DOWN, 0xb1, 0x63, "bank\\btn_buttom", msg_str(MSG_BANK_STEP_DOWN));
    ui_window_add_bitmap_button(wnd, BANK_ID_ADD_100K, 0xbd, 0x84, "bank\\btn_10mil", "+ 100,000");
    ui_window_add_bitmap_button(wnd, BANK_ID_ADD_1M, 0x8d, 0x84, "bank\\btn_100mil", "+ 1,000,000");
    ui_window_add_bitmap_button(wnd, BANK_ID_ADD_10M, 0x5d, 0x84, "bank\\btn_1000mil", "+ 10,000,000");
    ui_window_add_bitmap_button(wnd, BANK_ID_MAX, 0x2d, 0x84, "bank\\btn_max", msg_str(MSG_BANK_MAX));

    edit = mem_alloc(sizeof(struct UIEditCtrl));
    if (edit)
        edit = ui_edit_ctrl_construct(edit);
    self->amount_edit = edit;
    ui_window_create(edit, 0x63, 0x11);
    UI_EDIT_MAX_CHARS(self->amount_edit) = 10;
    ui_window_move(self->amount_edit, 0x4e, 0x5a);
    ui_edit_ctrl_set_back_colour(self->amount_edit, 0xf7, 0xf7, 0xf7);
    ui_edit_ctrl_set_border_colour(self->amount_edit, 0xc6, 0xc6, 0xc6);
    UI_EDIT_NO_BORDER(self->amount_edit) = 0;
    UI_EDIT_TEXT_TOP(self->amount_edit) = 1;
    ui_window_add_child(wnd, self->amount_edit);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->amount_edit);
}

void ui_bank_new_wnd_on_draw(struct UIWindow *window)
{
    struct UIBank_NewWnd *self = (struct UIBank_NewWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    void *background = ui_skin_bitmap(UI_SKIN "bank\\bg_bank.bmp");
    char money[0x104], line[0x104];
    int value_x;

    ui_window_draw_bitmap(wnd, 0, ui_window_draw_title_bar(wnd, 1, msg_str(MSG_BANK_TITLE), 0), background, 0);
    memset(money, 0, sizeof money);
    memset(line, 0, sizeof line);

    ui_window_text_out(wnd, 0x1e, 0x1d, msg_str(MSG_BANK_BALANCE), 0, 0, 12, 0, 0, 0);
    value_x = ui_window_vertical_text_length(wnd, msg_str(MSG_BANK_BALANCE), 0, 0, 12, 0) + 0x2b;
    format_money64(g_session1.bankZeny, money, sizeof money);
    sprintf(line, "%s z", money);
    ui_window_text_out(wnd, value_x, 0x1d, line, 0, 0, 11, 0, 0, 0);

    ui_window_text_out(wnd, 0x1e, 0x2e, msg_str(MSG_BANK_PURSE), 0, 0, 12, 0, 0, 0);
    format_money(g_session1.zeny, money);
    sprintf(line, "%s z", money);
    ui_window_text_out(wnd, value_x, 0x2e, line, 0, 0, 11, 0, 0, 0);

    if (self->notice.size) {
        const char *s = string_cstr(&self->notice);
        int width = ui_window_vertical_text_length(wnd, s, 0, 0, 12, 0);

        ui_window_text_out(wnd, (int)((float)wnd->width * 0.5f - (float)width * 0.5f), 0x70, s, 0, 0, 11,
                         CHAT_RED, 0, 0);
    }
}

static void set_notice(struct UIBank_NewWnd *self, const char *text)
{
    string_assign_cstr(&self->notice, text);
}

static void refuse(struct UIBank_NewWnd *self, int chat_msg, int notice_msg)
{
    ui_chat_line(msg_str(chat_msg), CHAT_RED);
    set_notice(self, msg_str(notice_msg));
}

static BYTE is_digits(const char *text, unsigned len)
{
    unsigned i;

    if (len == 0)
        return 0;
    for (i = 0; i < len; i++)
        if ((unsigned char)(text[i] - '0') > 9)
            return 0;
    return 1;
}

static void take_amount_text(struct UIBank_NewWnd *self, struct String *text)
{
    const char *s = ui_edit_ctrl_get_text(self->amount_edit);

    string_init(text);
    string_assign_cstr(text, s);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->amount_edit, "");
}

static long long bank_step_amount(struct UIBank_NewWnd *self, int delta)
{
    struct String text;
    long long amount;

    take_amount_text(self, &text);
    if (text.size == 0 || !is_digits(string_cstr(&text), text.size))
        amount = delta;
    else
        amount = _atoi64(string_cstr(&text)) + delta;
    if (amount > ZENY_MAX)
        amount = ZENY_MAX;
    else if (amount < 0)
        amount = 0;
    string_free(&text);
    return amount;
}

static long long bank_checked_amount(struct UIBank_NewWnd *self, BYTE deposit)
{
    struct String text;
    const char *max_word;
    long long amount = 0;

    take_amount_text(self, &text);
    max_word = msg_str(MSG_BANK_MAX);
    if (text.size == strlen(max_word) && memcmp(max_word, string_cstr(&text), text.size) == 0) {
        long long purse = g_session1.zeny, bank = g_session1.bankZeny;
        long long over = purse + bank > ZENY_MAX ? purse + bank - ZENY_MAX : 0;

        if (deposit) {
            if (bank >= ZENY_MAX)
                refuse(self, MSG_BANK_FULL, MSG_BANK_NOTE_FULL);
            else if (g_session1.zeny == 0)
                refuse(self, MSG_BANK_EMPTY, MSG_BANK_NOTE_EMPTY);
            else if (purse - over < 0)
                refuse(self, MSG_BANK_NOTHING_LEFT, MSG_BANK_NOTE_EMPTY);
            else
                amount = purse - over;
        } else {
            if (bank == 0)
                refuse(self, MSG_BANK_EMPTY, MSG_BANK_NOTE_EMPTY);
            else if (purse + (bank - over) > ZENY_MAX)
                amount = (int)(0x7fffffffu - (unsigned)g_session1.zeny);
            else
                amount = bank - over;
        }
    } else if (text.size == 0) {
        refuse(self, MSG_BANK_NO_AMOUNT, MSG_BANK_NOTE_NO_AMOUNT);
    } else if (!is_digits(string_cstr(&text), text.size)) {
        refuse(self, MSG_BANK_NOT_A_NUMBER, MSG_BANK_NOTE_NOT_A_NUMBER);
    } else {
        amount = _atoi64(string_cstr(&text));
        if (amount > ZENY_MAX) {
            refuse(self, MSG_BANK_TOO_MUCH, MSG_BANK_NOTE_FULL);
            amount = 0;
        } else if (amount < 1) {
            refuse(self, MSG_BANK_TOO_LITTLE, MSG_BANK_NOTE_TOO_LITTLE);
            amount = 0;
        }
    }
    string_free(&text);
    return amount;
}

static void send_banking(int packet_id, int money)
{
    struct PacketBanking pkt;

    pkt.id = (unsigned short)packet_id;
    pkt.account_id = g_session1.ownAid;
    pkt.money = money;
    send_packet(&pkt, rag_connection_get_packet_length(rag_connection_instance(), packet_id));
}

static void bank_deposit(struct UIBank_NewWnd *self)
{
    long long amount;

    amount = bank_checked_amount(self, 1);
    if (amount == 0)
        return;
    if ((long long)g_session1.zeny < amount)
        refuse(self, MSG_BANK_NOT_ENOUGH_ZENY, MSG_BANK_NOTE_NOT_ENOUGH_ZENY);
    else if (g_session1.bankZeny + amount > ZENY_MAX)
        refuse(self, MSG_BANK_FULL, MSG_BANK_NOTE_FULL);
    else {
        send_banking(PKT_CZ_REQ_BANKING_DEPOSIT, (int)amount);
        string_clear(&self->notice);
    }
}

static void bank_withdraw(struct UIBank_NewWnd *self)
{
    long long amount;

    amount = bank_checked_amount(self, 0);
    if (amount == 0)
        return;
    if (g_session1.bankZeny < amount)
        refuse(self, MSG_BANK_NOT_ENOUGH_BANKED, MSG_BANK_NOTE_NOT_ENOUGH_BANKED);
    else if ((unsigned)(g_session1.zeny + (int)amount) > 0x7fffffffu)
        refuse(self, MSG_BANK_PURSE_FULL, MSG_BANK_FULL);
    else {
        send_banking(PKT_CZ_REQ_BANKING_WITHDRAW, (int)amount);
        string_clear(&self->notice);
    }
}

static void bank_show_amount(struct UIBank_NewWnd *self, long long amount)
{
    char text[0x104];

    memset(text, 0, sizeof text);
    _i64toa(amount, text, 10);
    if (!self->amount_edit)
        return;
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->amount_edit, text);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->amount_edit);
}

int ui_bank_new_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                     int val2, int val3, int val4)
{
    struct UIBank_NewWnd *self = (struct UIBank_NewWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    switch (val1) {
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_BANK);
        break;
    case BANK_ID_STEP_UP:
        bank_show_amount(self, bank_step_amount(self, 1));
        break;
    case BANK_ID_STEP_DOWN:
        bank_show_amount(self, bank_step_amount(self, -1));
        break;
    case BANK_ID_ADD_10M:
        bank_show_amount(self, bank_step_amount(self, 10000000));
        break;
    case BANK_ID_ADD_1M:
        bank_show_amount(self, bank_step_amount(self, 1000000));
        break;
    case BANK_ID_ADD_100K:
        bank_show_amount(self, bank_step_amount(self, 100000));
        break;
    case BANK_ID_MAX:
        if (self->amount_edit) {
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->amount_edit, msg_str(MSG_BANK_MAX));
            ui_window_mgr_set_focus_edit(&g_windowMgr, self->amount_edit);
        }
        break;
    case BANK_ID_DEPOSIT:
        bank_deposit(self);
        break;
    case BANK_ID_WITHDRAW:
        bank_withdraw(self);
        break;
    }
    return 0;
}
