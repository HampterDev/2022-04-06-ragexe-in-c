#include "ui/controls/edit.h"
#include "ui/window_records.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "session/number_codec.h"
#include "ui/record_tags.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include <string.h>
#include "ui/shops/shops.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/wndmgr/wndmgr.h"

struct DragInfo;
struct UIExchangeWnd;
struct UIWindow;
void ui_exchange_wnd_destroy(struct UIWindow *window);
void ui_exchange_wnd_on_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_exchange_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_exchange_wnd_on_process(struct UIWindow *self);
void ui_exchange_wnd_on_draw(struct UIWindow *self);
void ui_exchange_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_exchange_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_exchange_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_exchange_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_exchange_wnd_ops = {
    .destroy = ui_exchange_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_exchange_wnd_on_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_exchange_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_exchange_wnd_on_process,
    .on_draw = ui_exchange_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_exchange_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_exchange_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_exchange_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_exchange_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_exchange_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define MODE_REQ_TRADE_ADD 0x33
#define MODE_REQ_TRADE_LOCK 0x34
#define MODE_REQ_TRADE_CANCEL 0x35
#define MODE_REQ_TRADE_COMMIT 0x36
#define MODE_MSG_SCREENSHOT 0x44

#define UI_MSG_WHEEL_SCROLL 7
#define UI_MSG_TRADE_REFRESH 0x17
#define UI_MSG_TRADE_PARTNER 0x32
#define UI_MSG_TRADE_LOCKED_MINE 0x33
#define UI_MSG_TRADE_LOCKED_PARTNER 0x34

#define UI_ID_DA 0xda
#define UI_ID_EXCHANGE 0xde

#define TRADE_SLOTS 10
#define TRADE_MAX_ZENY_TYPED 1000000000
#define TRADE_RIGHT_COLUMN_X 0x118

struct UIExchangeWnd {
    struct UIFrameWnd frame;
    struct UIWindow *buttons[3];
    struct UIWindow *money_edit;
    struct UIWindow *scroll_bar;
    int locked_mine;
    int locked_partner;
    int pressed_ok;
    int pressed_exchange;
    int pressed_cancel;
    struct List my_items;
    struct List partner_items;
    struct String partner_name;
    int partner_cid;
    short partner_level;
    short row_height;
    short hover_x;
    struct UIWindow *notify_check;
    BYTE notify;
    int money_len;
    struct String money_text;
};
OFFSET_ASSERT(UIExchangeWnd, buttons, 0x94);

static const int g_buttonX[3] = { 3, 0x104, 0x1fe };

static void button_drops(const struct UIExchangeWnd *self, int drop[3])
{
    drop[0] = self->locked_mine * 0x78;
    drop[1] = self->locked_mine && self->locked_partner && !self->pressed_exchange ? 0 : 0x78;
    drop[2] = 0;
}

struct UIExchangeWnd *ui_exchange_wnd_construct(struct UIExchangeWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_exchange_wnd_ops;
    list_init(&self->my_items);
    list_init(&self->partner_items);
    string_init(&self->partner_name);
    self->notify = 0;
    self->row_height = 0x1e;
    string_init(&self->money_text);
    self->money_len = 0;
    string_clear(&self->money_text);
    return self;
}

struct UIWindow *ui_exchange_wnd_new(void)
{
    return (struct UIWindow *)ui_exchange_wnd_construct(mem_alloc(sizeof(struct UIExchangeWnd)));
}

void ui_exchange_wnd_destruct(struct UIExchangeWnd *self)
{
    self->frame.base.ops = &ui_exchange_wnd_ops;
    string_free(&self->money_text);
    string_free(&self->partner_name);
    list_free(&self->partner_items, item_info_destroy);
    list_free(&self->my_items, item_info_destroy);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_exchange_wnd_destroy(struct UIWindow *window)
{
    struct UIExchangeWnd *self = (struct UIExchangeWnd *)window;
    ui_exchange_wnd_destruct(self);
    mem_free(self);
}

static void layout_buttons(struct UIExchangeWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    int drop[3], i;

    button_drops(self, drop);
    for (i = 0; i < 3; i++)
        ui_window_move_base(self->buttons[i], g_buttonX[i], wnd->height - 0x17 + drop[i]);
    ui_window_invalidate_base(wnd);
}

static void hide_money_edit(struct UIExchangeWnd *self)
{
    if (!self->locked_mine)
        return;
    if (self->money_edit) {
        ui_window_mgr_set_focus_edit(&g_windowMgr, NULL);
        ui_window_move(self->money_edit, 0x81, 0x1f4);
        self->money_edit = NULL;
        if (g_windowMgr.new_chat_wnd)
            ui_window_send_msg(g_windowMgr.new_chat_wnd, NULL, 0x36, 0, 0, 0, 0);
    }
    ui_window_invalidate_base(&self->frame.base);
}

static struct ItemInfo *item_at(struct UIExchangeWnd *self, struct ItemInfo *out, int x, int y)
{
    struct UIWindow *wnd = &self->frame.base;
    struct List *list;
    struct ListNode *node;
    int row, i;

    if (x < 4 || x >= wnd->width - 3 || y < 0x14 || y >= wnd->height - 0x28)
        return item_info_construct_with_arg(out);
    row = (y - 0x11) / self->row_height;
    list = x < TRADE_RIGHT_COLUMN_X ? &self->my_items : &self->partner_items;
    node = list->head->next;
    for (i = 0; i < row && node != list->head; i++)
        node = node->next;
    if (node == list->head)
        return item_info_construct_with_arg(out);
    return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
}

static void money_input(struct UIExchangeWnd *self, char buf[24])
{
    sprintf(buf, "%s ", ui_edit_ctrl_get_text(self->money_edit));
}

static void keep_money_text(struct UIExchangeWnd *self)
{
    const char *text = ui_edit_ctrl_get_text(self->money_edit);

    string_assign_cstr(&self->money_text, text);
}

static void bad_amount_box(void)
{
    ui_window_mgr_message_box(&g_windowMgr, msg_str(0x25a), UI_MSGBOX_OK, 1, 0, 0, "Message", 0x118, 0x78, 0);
}

void ui_exchange_wnd_on_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIExchangeWnd *self = (struct UIExchangeWnd *)window;
    struct Mode *mode = game_current_base_mode();

    if (self->locked_mine || drag->source != DRAG_INVENTORY) {
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (drag->drop_count == 1) {
        ui_exchange_wnd_send_msg(&self->frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, 5, 0, 0, 0);
}

void ui_exchange_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIExchangeWnd *self = (struct UIExchangeWnd *)window;
    static const int ids[3] = { UI_ID_OK, UI_ID_EXCHANGE, UI_ID_CANCEL };
    static const char *const bitmaps[3][3] = {
        { UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp" },
        { UI_SKIN "btn_exchange.bmp", UI_SKIN "btn_exchange_a.bmp", UI_SKIN "btn_exchange_b.bmp" },
        { UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp", UI_SKIN "btn_cancel_b.bmp" },
    };
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *edit, *check;
    int drop[3], i;

    ui_exchange_wnd_send_msg(wnd, NULL, UI_MSG_TRADE_REFRESH, 0, 0, 0, 0);
    self->locked_mine = 0;
    self->locked_partner = 0;
    self->pressed_ok = 0;
    self->pressed_exchange = 0;
    self->pressed_cancel = 0;

    button_drops(self, drop);
    for (i = 0; i < 3; i++) {
        struct UIBitmapButton *button = mem_alloc(sizeof *button);

        if (button)
            button = ui_bitmap_button_construct(button);
        self->buttons[i] = &button->button.base;
        ui_bitmap_button_set_bitmap(button, bitmaps[i][0], UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(button, bitmaps[i][1], UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(button, bitmaps[i][2], UI_BTN_DOWN, 0);
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, g_buttonX[i], wnd->height + drop[i] - 0x17);
        button->button.base.id = ids[i];
        ui_window_add_child(wnd, &button->button.base);
    }

    edit = mem_alloc(sizeof(struct UIEditCtrl));
    if (edit)
        edit = ui_edit_ctrl_construct(edit);
    self->money_edit = edit;
    ui_window_create(edit, 0x58, 0x10);
    ui_window_move(self->money_edit, 0x91, 0x14a);
    UI_EDIT_MAX_CHARS(self->money_edit) = 0xd;
    ui_edit_ctrl_set_back_colour(self->money_edit, 0xe8, 0xe8, 0xe8);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->money_edit, "0");
    ui_window_add_child(wnd, self->money_edit);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->money_edit);
    ui_edit_ctrl_select_all(self->money_edit);

    check = mem_alloc(sizeof(struct UICheckBox));
    if (check)
        check = (struct UIWindow *)ui_check_box_construct(check);
    self->notify_check = check;
    ui_window_create(check, 0x30, 0xc);
    ui_check_box_set_bitmaps_fit((struct UICheckBox *)self->notify_check, UI_SKIN "checkbox_1.bmp",
                             UI_SKIN "checkbox_0.bmp");
    ui_window_move_base(self->notify_check, 0x69, wnd->height - 0x13);
    ui_check_box_set_check(self->notify_check, self->notify);
    ui_window_add_child(wnd, self->notify_check);
    self->frame.default_id = 0x239;
}

void ui_exchange_wnd_on_process(struct UIWindow *window)
{
    struct UIExchangeWnd *self = (struct UIExchangeWnd *)window;
    struct UIWindow *edit = self->money_edit;
    struct AmountColours colours;
    long long amount;
    char buf[24];
    int len;

    if (!edit)
        return;
    len = ui_edit_ctrl_get_text_size(edit);
    if (len == self->money_len)
        return;
    self->money_len = len;
    if (len == 0)
        return;

    money_input(self, buf);
    if (!session_is_number_text(&g_session1, buf)) {
        bad_amount_box();
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->money_edit, string_cstr(&self->money_text));
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->money_edit);
        return;
    }
    amount = _atoi64(buf);
    if (amount <= 0) {
        amount = 0;
        format_money64(amount, buf, sizeof buf);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->money_edit, buf);
        keep_money_text(self);
    } else if (amount > TRADE_MAX_ZENY_TYPED) {
        if (ui_window_mgr_message_box(&g_windowMgr, msg_str(0x9a3), UI_MSGBOX_QUESTION, 1, 0, 0, "Message",
                                   0x118, 0x78, 0) == UI_MSGBOX_ANSWER_YES) {
            amount = TRADE_MAX_ZENY_TYPED;
            format_money64(amount, buf, sizeof buf);
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->money_edit, buf);
        } else {
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->money_edit, string_cstr(&self->money_text));
        }
    } else {
        format_money64(amount, buf, sizeof buf);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->money_edit, buf);
        keep_money_text(self);
    }

    ui_window_amount_colours(&self->frame.base, &colours, amount);
    ui_edit_ctrl_set_text_colour(self->money_edit, colours.colour & 0xff, (colours.colour >> 8) & 0xff,
                             (colours.colour >> 16) & 0xff, colours.double_colour);
    self->money_len = ui_edit_ctrl_get_text_size(self->money_edit);
    ui_edit_ctrl_set_caret(self->money_edit, ui_edit_ctrl_get_text_size(self->money_edit));
}

static void draw_items(struct UIExchangeWnd *self, struct List *list, int icon_x, int name_x)
{
    struct UIWindow *wnd = &self->frame.base;
    struct ListNode *node;
    int row = 0;

    for (node = list->head->next; node != list->head; node = node->next) {
        struct ItemInfo *info = LIST_VALUE(node, struct ItemInfo);
        int y;

        if (item_grid_hidden(info))
            continue;
        y = self->row_height * row;
        ui_window_draw_item_icon_count(wnd, icon_x, y + 0x15, info, 0, 0);
        ui_window_draw_item_name(wnd, name_x, y + 0x1d, info, 0, 1);
        if (info->grade > 0) {
            struct String path;

            string_init(&path);
            string_format(&path, UI_SKIN "grade_enchant\\grade_icon%d.bmp", (int)info->grade);
            ui_window_draw_bitmap(wnd, icon_x, y + 0x21, res_mgr_get(g_resMgr(), string_cstr(&path)),
                                1);
            string_free(&path);
        }
        if (row == TRADE_SLOTS - 1)
            break;
        row++;
    }
}

static void draw_zeny(struct UIExchangeWnd *self, int amount, int right_x)
{
    struct UIWindow *wnd = &self->frame.base;
    struct AmountColours colours;
    char money[32], text[128];
    COLORREF colour;
    int len;

    len = sprintf(text, "%s", format_money(amount, money));
    colour = ui_window_amount_colours(wnd, &colours, amount)->colour & 0xffffff;
    ui_window_text_out_right(wnd, right_x, 0x14b, text, len, 1, 0x10, colour, 0);
    ui_window_text_out_right(wnd, right_x + 1, 0x14b, text, len, 1, 0x10, colour, 0);
    ui_window_text_out_right(wnd, right_x + 1, 0x14c, text, len, 1, 0x10, 0x323232, 0);
}

void ui_exchange_wnd_on_draw(struct UIWindow *window)
{
    struct UIExchangeWnd *self = (struct UIExchangeWnd *)window;
    static const int disabled_x[2] = { 3, 0x104 };
    static const char *const disabled[2] = {
        UI_SKIN "btn_ok_dis.bmp", UI_SKIN "btn_exchange_dis.bmp",
    };
    struct UIWindow *wnd = &self->frame.base;
    char label[128], line[128];
    const char *name = string_cstr(&self->partner_name);
    int i;

    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\titlebar_fix2.bmp"), 0);
    ui_window_draw_bitmap(wnd, 0, 0x11, ui_skin_bitmap(UI_SKIN "basic_interface\\exchange_bg2.bmp"), 0);
    ui_window_text_out_outlined(wnd, 0x7b, wnd->height - 0x14, msg_str(0x727), 0, 0x0000ff, 0xffffff, 0, 12, 1,
                             0);

    sprintf(label, "%s : ", msg_str(0xce));
    ui_window_text_out(wnd, 0xd, 4, label, 0, 0, 12, 0xffffff, 0, 0);
    ui_window_text_out(wnd, 0xc, 3, label, 0, 0, 12, 0, 0, 0);
    if (self->partner_cid > 0) {
        if (g_serviceType == 6)
            sprintf(line, "%s  Lv%d", name, (int)self->partner_level);
        else {
            struct String code;

            session_scramble_number(&g_session1, &code, self->partner_cid);
            sprintf(line, "%s  Lv%d (%s)", name, (int)self->partner_level, string_cstr(&code));
            string_free(&code);
        }
    } else
        sprintf(line, "%s", name);
    ui_window_text_out(wnd, 0x3f, 4, line, 0, 0, 12, 0xb4b4b4, 0, 0);
    ui_window_text_out(wnd, 0x3e, 3, line, 0, 0, 12, 0, 0, 0);

    for (i = 0; i < 2; i++)
        ui_window_draw_bitmap(wnd, disabled_x[i], wnd->height - 0x17, ui_skin_bitmap(disabled[i]), 0);

    for (i = 0; i < TRADE_SLOTS; i++) {
        ui_window_draw_item_win_mid(wnd, 0xc, self->row_height * i + 0x14);
        ui_window_draw_item_win_mid(wnd, 0x123, self->row_height * i + 0x14);
    }
    if (self->locked_mine)
        ui_window_fill_rect(wnd, 4, 0x15, 0x110, 0x136, ui_skin_colour(10, 6));
    if (self->locked_partner)
        ui_window_fill_rect(wnd, 0x11b, 0x15, 0x110, 0x136, ui_skin_colour(10, 6));

    draw_items(self, &self->my_items, 0x10, 0x30);
    draw_items(self, &self->partner_items, 0x127, 0x147);
    draw_zeny(self, g_session1.myTradeZeny, 0xdb);
    draw_zeny(self, g_session1.partnerTradeZeny, 0x1f3);
}

void ui_exchange_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIExchangeWnd *self = (struct UIExchangeWnd *)window;
    struct ItemInfo item;

    item_at(self, &item, x, y);
    self->hover_x = (short)x;
    if (item.index != 0) {
        int row = (y - 0x11) / self->row_height;
        int tip_x = x < TRADE_RIGHT_COLUMN_X ? 0x2a : 0x141;
        int tip_y = row * self->row_height + 4;
        struct Mode *mode;
        struct String text;
        struct String name = {0};
        COLORREF colour;

        ui_window_to_screen(&self->frame.base, &tip_x, &tip_y);
        mode = g_modeMgr.curMode;
        item.type = 0x15;
        item_info_get_display_name(&item, &name);
        if (item.identified == 1 && item.option_count != 0) {
            string_init(&text);
            string_format(&text, msg_str(0xa28), string_cstr(&name), item.option_count);
            colour = session_get_item_dis_color(&item);
            mode_show_tooltip(mode, string_cstr(&text), tip_x, tip_y, colour, 0);
            string_free(&text);
        } else {
            colour = session_get_item_dis_color(&item);
            mode_show_tooltip(mode, string_cstr(&name), tip_x, tip_y, colour, 0);
        }
        string_free(&name);
    }
    item_info_destruct(&item);
}

void ui_exchange_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIExchangeWnd *self = (struct UIExchangeWnd *)window;
    struct ItemInfo item;
    struct UIWindow *info_wnd;

    item_at(self, &item, x, y);
    if (item.index != 0) {
        info_wnd = ui_window_open(0xc);
        if (info_wnd)
            ui_window_send_msg(info_wnd, NULL, 0x18, (int)&item, 0, 0, 0);
    }
    item_info_destruct(&item);
}

void ui_exchange_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIExchangeWnd *self = (struct UIExchangeWnd *)window;
    ui_exchange_wnd_send_msg(&self->frame.base, self->scroll_bar, UI_MSG_WHEEL_SCROLL, delta < 0 ? 1 : -1, 0, 0, 0);
}

int ui_exchange_wnd_get_records(struct UIWindow *window, struct ReplayBuffer *out)
{
    struct UIExchangeWnd *self = (struct UIExchangeWnd *)window;
    struct SnapshotWriter writer = {0};
    snapshot_write(&writer, REC_TRADE_BEGIN, NULL, 0);
    snapshot_write(&writer, REC_TRADE_PARTNER_CID, &self->partner_cid, 4);
    snapshot_write(&writer, REC_TRADE_PARTNER_LEVEL, &self->partner_level, 2);
    snapshot_write(&writer, REC_TRADE_PARTNER_NAME, string_cstr(&self->partner_name), self->partner_name.size);
    snapshot_write(&writer, REC_TRADE_LOCKED_MINE, &self->locked_mine, 4);
    snapshot_write(&writer, REC_TRADE_LOCKED_PARTNER, &self->locked_partner, 4);
    snapshot_write(&writer, REC_TRADE_PRESSED_OK, &self->pressed_ok, 4);
    snapshot_write(&writer, REC_TRADE_PRESSED_EXCHANGE, &self->pressed_exchange, 4);
    snapshot_write(&writer, REC_TRADE_PRESSED_CANCEL, &self->pressed_cancel, 4);
    snapshot_write(&writer, REC_TRADE_END, NULL, 0);
    return snapshot_finish(&writer, out);
}

static void load_records(struct UIExchangeWnd *self, const char *buf, unsigned size)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIRecordHeader hdr = { 0 };
    unsigned pos = sizeof hdr;
    int cid, level;

    memcpy(&hdr, buf, sizeof hdr);
    if (hdr.tag != REC_TRADE_BEGIN)
        return;
    cid = 0;
    level = 0;
    while (pos < size) {
        int *flag = NULL;
        int msg = 0, val1 = 0;

        memcpy(&hdr, buf + pos, sizeof hdr);
        pos += sizeof hdr;
        switch (hdr.tag) {
        case REC_TRADE_END:
            ui_exchange_wnd_send_msg(wnd, NULL, UI_MSG_TRADE_REFRESH, 0, 0, 0, 0);
            return;
        case REC_TRADE_PARTNER_NAME: {
            char name[0x104];

            memset(name, 0, sizeof name);
            memcpy(name, buf + pos, hdr.len);
            pos += hdr.len;
            ui_exchange_wnd_send_msg(wnd, NULL, UI_MSG_TRADE_PARTNER, (int)name, cid, (short)level, 0);
            continue;
        }
        case REC_TRADE_PARTNER_CID:
            memcpy(&cid, buf + pos, hdr.len);
            pos += hdr.len;
            continue;
        case REC_TRADE_PARTNER_LEVEL:
            memcpy(&level, buf + pos, hdr.len);
            pos += hdr.len;
            continue;
        case REC_TRADE_LOCKED_MINE:
            flag = &self->locked_mine, msg = UI_MSG_TRADE_LOCKED_MINE;
            break;
        case REC_TRADE_LOCKED_PARTNER:
            flag = &self->locked_partner, msg = UI_MSG_TRADE_LOCKED_PARTNER;
            break;
        case REC_TRADE_PRESSED_OK:
            flag = &self->pressed_ok, msg = UI_MSG_COMMAND, val1 = UI_ID_OK;
            break;
        case REC_TRADE_PRESSED_EXCHANGE:
            flag = &self->pressed_exchange, msg = UI_MSG_COMMAND, val1 = UI_ID_EXCHANGE;
            break;
        case REC_TRADE_PRESSED_CANCEL:
            flag = &self->pressed_cancel, msg = UI_MSG_COMMAND, val1 = UI_ID_CANCEL;
            break;
        default:
            pos += hdr.len;
            continue;
        }
        memcpy(flag, buf + pos, hdr.len);
        pos += hdr.len;
        if (*flag)
            ui_exchange_wnd_send_msg(wnd, NULL, msg, val1, 0, 0, 0);
    }
}

static int on_command(struct UIExchangeWnd *self, int id, int state)
{
    struct Mode *mode = game_current_base_mode();
    char buf[24];

    switch (id) {
    case UI_ID_OK:
        if (self->pressed_ok)
            return 0;
        if (self->money_edit) {
            long long typed;
            int amount;

            money_input(self, buf);
            if (!session_is_number_text(&g_session1, buf)) {
                bad_amount_box();
                return 0;
            }
            typed = _atoi64(buf);
            amount = typed >= 0x7fffffff ? 0x7fffffff : typed > 0 ? (int)typed : 0;
            if (amount < 0)
                amount = 0;
            if (amount > g_session1.zeny)
                amount = g_session1.zeny;
            mode_send_message(mode, MODE_REQ_TRADE_ADD, 0, g_session1.myTradeZeny + amount, 0, 0);
            hide_money_edit(self);
        }
        mode_send_message(mode, MODE_REQ_TRADE_LOCK, 0, 0, 0, 0);
        self->pressed_ok = 1;
        return 0;
    case UI_ID_CANCEL:
        if (self->pressed_cancel)
            return 0;
        mode_send_message(mode, MODE_REQ_TRADE_CANCEL, 0, 0, 0, 0);
        self->pressed_cancel = 1;
        return 0;
    case UI_ID_CHECKBOX:
        self->notify = state != 0;
        return 0;
    case UI_ID_DA: {
        int amount;

        if (!self->money_edit)
            return 0;
        amount = atoi(ui_edit_ctrl_get_text(self->money_edit));
        if (amount < 0)
            amount = 0;
        if (amount > g_session1.zeny)
            amount = g_session1.zeny;
        mode_send_message(mode, MODE_REQ_TRADE_ADD, 0, g_session1.myTradeZeny + amount, 0, 0);
        hide_money_edit(self);
        return 0;
    }
    case UI_ID_EXCHANGE:
        if (self->pressed_exchange)
            return 0;
        if (self->notify) {
            struct String text;

            string_init(&text);
            string_format(&text, msg_str(0x728), string_cstr(&self->partner_name));
            mode_send_message(g_modeMgr.curMode, MODE_MSG_SCREENSHOT, (int)string_cstr(&text), 0, 0, 0);
            string_free(&text);
        }
        mode_send_message(mode, MODE_REQ_TRADE_COMMIT, 0, 0, 0, 0);
        self->pressed_exchange = 1;
        layout_buttons(self);
        return 0;
    }
    return 0;
}

int ui_exchange_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                     int val2, int val3, int val4)
{
    struct UIExchangeWnd *self = (struct UIExchangeWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        return on_command(self, val1, val2);
    case UI_MSG_WHEEL_SCROLL:
        return 0;
    case UI_MSG_TRADE_REFRESH: {
        int i;

        list_clear(&self->my_items, item_info_destroy);
        list_clear(&self->partner_items, item_info_destroy);
        for (i = 0; i < TRADE_SLOTS; i++) {
            struct ItemInfo mine, theirs;

            session_get_my_trade_item(&g_session1, &mine, i);
            if (mine.count != 0)
                item_info_copy(list_push_back(&self->my_items, NULL, sizeof(struct ItemInfo)), &mine);
            session_get_partner_trade_item(&g_session1, &theirs, i);
            if (theirs.count != 0)
                item_info_copy(list_push_back(&self->partner_items, NULL, sizeof(struct ItemInfo)), &theirs);
            item_info_destruct(&theirs);
            item_info_destruct(&mine);
        }
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    }
    case UI_MSG_ADD_DRAGGED: {
        struct Mode *mode = game_current_base_mode();
        const struct DragInfo *drag = game_current_drag();

        mode_send_message(mode, MODE_REQ_TRADE_ADD, drag->index, drag->drop_count, 0, 0);
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return 0;
    }
    case UI_MSG_TRADE_PARTNER: {
        const char *name = (const char *)val1;

        string_assign_cstr(&self->partner_name, name);
        self->partner_cid = val2;
        self->partner_level = (short)val3;
        return 0;
    }
    case UI_MSG_TRADE_LOCKED_MINE:
        self->locked_mine = 1;
        hide_money_edit(self);
        layout_buttons(self);
        return 0;
    case UI_MSG_TRADE_LOCKED_PARTNER:
        self->locked_partner = 1;
        hide_money_edit(self);
        layout_buttons(self);
        return 0;
    case UI_MSG_LOAD_RECORDS:
        {
            int record_bytes = ui_window_records_validate((const void *)val1, (size_t)(unsigned)val2);
            if (record_bytes <= 0) return 0;
            val2 = record_bytes;
        }
        load_records(self, (const char *)val1, (unsigned)val2);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
