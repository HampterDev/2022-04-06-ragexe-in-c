#include "ui/controls/edit.h"
#include "ui/controls/buttons.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include "core/markup_tags.h"
#include "ui/mail/rodex_ui.h"
#include "game/rodex_mail.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include <stdint.h>

#define ID_TAKE_ITEMS 0xb5
#define ID_TAKE_ZENY 0xb6
#define ID_DELETE 0xcb
#define ID_REPLY 0x129
#define ID_RETURN 0x12e
#define ID_BODY 0x1f0

#define MSG_NO_REPLY 0x6a

#define MODE_RODEX_DELETE 0xc4
#define MODE_RODEX_REPLY 0x10c

#define PACKET_CZ_REQ_ZENY_FROM_RODEX 0x9f1
#define PACKET_CZ_REQ_ITEM_FROM_RODEX 0x9f3
#define PACKET_CZ_RODEX_RETURN 0xb98

#define MSG_DELETE_QUESTION 0x164
#define MSG_TITLE 0xaa7
#define MSG_TAKE_FIRST 0xa34
#define MSG_DELETE 0xdd6
#define MSG_REPLY 0xdd7
#define MSG_RETURN 0xddc
#define MSG_NAME_OPTIONS 0xa28

#define COLOUR_SENDER 0x95330f
#define COLOUR_TEXT 0x313131
#define COLOUR_GREY 0x737373

#define ITEM_ROW_LEFT 0x1e
#define ITEM_ROW_TOP 0x135
#define ITEM_STEP 0x22
#define ITEM_SIZE 0x18

struct UIRodexContentsWnd {
    struct UIFrameWnd frame;
    struct UIEditCtrl3 *body;
    struct String sender;
    struct String title;
    int64_t zeny;
    struct List items;
    int hover;
    struct UIBitmapButton *btn_items;
    struct UIBitmapButton *btn_zeny;
    struct UIBasicButton *btn_delete;
    struct UIBasicButton *btn_reply;
    BYTE no_reply;
};
struct UIWindow;
void ui_rodex_contents_wnd_destroy(struct UIWindow *window);
void ui_rodex_contents_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_rodex_contents_wnd_on_draw(struct UIWindow *self);
void ui_rodex_contents_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_rodex_contents_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_rodex_contents_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_rodex_contents_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_rodex_contents_wnd_ops = {
    .destroy = ui_rodex_contents_wnd_destroy,
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
    .on_create = ui_rodex_contents_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_rodex_contents_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_rodex_contents_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_rodex_contents_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_rodex_contents_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_rodex_contents_wnd_send_msg,
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

#define UI_MSG_PARSE_LINKS 0x99
#define WND(self) (&(self)->frame.base)
#define EDIT(e) (&(e)->edit.base)

struct UIRodexContentsWnd *ui_rodex_contents_wnd_construct(struct UIRodexContentsWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    WND(self)->ops = &ui_rodex_contents_wnd_ops;
    self->body = NULL;
    string_init(&self->sender);
    string_init(&self->title);
    self->zeny = 0;
    list_init(&self->items);
    self->btn_items = NULL;
    self->btn_zeny = NULL;
    self->btn_delete = NULL;
    self->btn_reply = NULL;
    list_clear(&self->items, item_info_destroy);
    self->hover = 0;
    self->no_reply = 0;
    return self;
}

struct UIWindow *ui_rodex_contents_wnd_new(void)
{
    return (struct UIWindow *)ui_rodex_contents_wnd_construct(mem_alloc(sizeof(struct UIRodexContentsWnd)));
}

void ui_rodex_contents_wnd_destruct(struct UIRodexContentsWnd *self)
{
    WND(self)->ops = &ui_rodex_contents_wnd_ops;
    list_free(&self->items, item_info_destroy);
    string_free(&self->title);
    string_free(&self->sender);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_rodex_contents_wnd_destroy(struct UIWindow *window)
{
    struct UIRodexContentsWnd *self = (struct UIRodexContentsWnd *)window;
    ui_rodex_contents_wnd_destruct(self);
    mem_free(self);
}

struct ItemInfo *ui_rodex_contents_wnd_item_at(struct UIRodexContentsWnd *self, struct ItemInfo *out, int x,
                                                      int y)
{
    if ((unsigned)(x - ITEM_ROW_LEFT) <= 0xa0 && (unsigned)(y - ITEM_ROW_TOP) <= ITEM_SIZE) {
        struct ListNode *node;
        int left = ITEM_ROW_LEFT;

        self->hover = 0;
        for (node = self->items.head->next; node != self->items.head; node = node->next) {
            if (x > left && x < left + ITEM_SIZE)
                return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
            self->hover++;
            left += ITEM_STEP;
        }
    }
    return item_info_construct_with_arg(out);
}

void rodex_contents_wnd_items_taken(struct UIRodexContentsWnd *self)
{
    ui_button_show(&self->btn_items->button, 0);
    ui_window_invalidate_base(WND(self));
}

void rodex_contents_wnd_zeny_taken(struct UIRodexContentsWnd *self)
{
    ui_button_show(&self->btn_zeny->button, 0);
    self->zeny = 0;
    ui_window_invalidate_base(WND(self));
}

void rodex_contents_wnd_set_items_flag(struct UIRodexContentsWnd *self, BYTE disabled)
{
    self->btn_items->button.disabled = disabled;
    ui_window_invalidate_base(&self->btn_items->button.base);
    ui_window_invalidate_base(WND(self));
}

void rodex_contents_wnd_set_zeny_flag(struct UIRodexContentsWnd *self, BYTE disabled)
{
    self->btn_zeny->button.disabled = disabled;
    ui_window_invalidate_base(&self->btn_zeny->button.base);
    ui_window_invalidate_base(WND(self));
}

static struct UIBasicButton *basic_button(int msg, int width, int x, int y, int id)
{
    struct UIBasicButton *mem = mem_alloc(sizeof *mem);
    struct UIBasicButton *btn = mem ? ui_basic_button_construct(mem) : NULL;

    ui_basic_button_set_label(&btn->button.base, width, msg_str(msg));
    ui_window_move_base(&btn->button.base, x, y);
    btn->button.base.id = id;
    return btn;
}

void ui_rodex_contents_wnd_add_return_button(struct UIRodexContentsWnd *self)
{
    struct UIBasicButton *btn = basic_button(MSG_RETURN, 0x46, 0xe0, 0x1d, ID_RETURN);

    btn->button.disabled = 0;
    ui_window_invalidate_base(&btn->button.base);
    ui_window_add_child(WND(self), &btn->button.base);
}

static struct UIBitmapButton *receive_button(struct UIRodexContentsWnd *self, int y, int id)
{
    struct UIBitmapButton *mem = mem_alloc(sizeof *mem);
    struct UIBitmapButton *btn = mem ? ui_bitmap_button_construct(mem) : NULL;

    ui_bitmap_button_set_bitmap(btn, RODEX_DIR "btn_receive_out.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(btn, RODEX_DIR "btn_receive_over.bmp", UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(btn, RODEX_DIR "btn_receive_press.bmp", UI_BTN_DOWN, 0);
    ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
    ui_window_move_base(&btn->button.base, 0xfc, y);
    btn->button.base.id = id;
    ui_window_add_child(WND(self), &btn->button.base);
    return btn;
}

void ui_rodex_contents_wnd_set_mail(struct UIRodexContentsWnd *self, struct String sender,
                                           struct String title, struct String text, BYTE item_count,
                                           int64_t zeny, BYTE type)
{
    struct String nr = {0};
    struct String body_text;
    struct UIEditCtrl3 *mem;

    string_assign(&self->sender, string_cstr(&sender), sender.size);
    string_assign(&self->title, string_cstr(&title), title.size);
    string_copy(&body_text, &text);
    self->no_reply = (BYTE)navi_links_strip_nr(&nr, string_view(&self->title));
    string_assign(&self->title, string_cstr(&nr), nr.size);
    rodex_expand_msg_tags(&self->sender);
    rodex_expand_msg_tags(&self->title);
    rodex_expand_msg_tags(&body_text);

    mem = mem_alloc(sizeof *mem);
    self->body = mem ? ui_rodex_edit_ctrl3_construct(mem) : NULL;
    ui_window_create(EDIT(self->body), 0x10b, 0xc0);
    self->body->edit.multiline = 1;
    ui_window_move_base(EDIT(self->body), 0xf, 0x51);
    self->body->edit.max_chars = g_rodexBodyMaxChars;
    ui_edit_ctrl_set_back_colour(EDIT(self->body), 0xbd, 0xce, 0xf7);
    ui_edit_ctrl_set_text_colour(EDIT(self->body), 0x31, 0x31, 0x31, -1);
    self->body->edit.disabled = 1;
    ui_window_invalidate_base(EDIT(self->body));
    self->body->extra_line = 1;
    EDIT(self->body)->id = ID_BODY;
    if (text_view_find(string_view(&body_text), string_view(&text_tag_NAVIL), 0) == STRING_NPOS
        && text_view_find(string_view(&body_text), string_view(&text_tag_NAVIL_END), 0) == STRING_NPOS)
        ui_edit_ctrl_set_text(&self->body->edit, string_cstr(&body_text));
    else
        ui_window_send_msg(EDIT(self->body), NULL, UI_MSG_PARSE_LINKS, (int)string_cstr(&body_text), 0, 0, 0);
    ui_edit_ctrl3_set_tag_colour(self->body, RODEX_TAG_COLOUR);
    ui_window_add_child(WND(self), EDIT(self->body));

    if (item_count) {
        self->btn_items = receive_button(self, 0x138, ID_TAKE_ITEMS);
    }
    if (zeny > 0) {
        self->zeny = zeny;
        self->btn_zeny = receive_button(self, 0x159, ID_TAKE_ZENY);
    }
    if (g_rodex->open_type == 0 && !(type & MAIL_FROM_SERVER)) {
        self->btn_reply = basic_button(MSG_REPLY, 0x46, 0xe0, 0x178, ID_REPLY);
        self->btn_reply->button.disabled = 0;
        ui_window_invalidate_base(&self->btn_reply->button.base);
        ui_window_add_child(WND(self), &self->btn_reply->button.base);
    }
    if (self->no_reply == 1)
        ui_rodex_contents_wnd_send_msg(WND(self), NULL, MSG_NO_REPLY, 0, 0, 0, 0);
    string_free(&nr);
    string_free(&body_text);
    string_free(&sender);
    string_free(&title);
    string_free(&text);
}

static struct MailPair *open_mail(void)
{
    struct Rodex *box = g_rodex;
    struct Tree *map = &box->tabs[box->open_type == 0 ? 0 : box->open_type == 1 ? 1 : 2];
    long long id = box->open_mail_id;

    return tree_find(map, &id, mail_id_greater);
}

void ui_rodex_contents_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIRodexContentsWnd *self = (struct UIRodexContentsWnd *)window;
    ui_window_add_sys_button(WND(self));
    ui_window_add_close_button(WND(self), "", 0);
    if (g_rodex->open_type <= 2) {
        struct MailPair *node = open_mail();

        if (node) {
            struct Mail *mail = &node->mail;
            struct String sender, title, text;

            string_copy(&text, &mail->text);
            string_copy(&title, &mail->title);
            string_copy(&sender, &mail->sender);
            ui_rodex_contents_wnd_set_mail(self, sender, title, text, mail->item_count, mail->zeny, mail->type);
            if (g_rodex->open_type == 0 && !(mail->type & MAIL_FROM_SERVER) && mail->sender_trusted == 0)
                ui_rodex_contents_wnd_add_return_button(self);
        }
    }
    self->btn_delete = basic_button(MSG_DELETE, 0x32, 6, 0x178, ID_DELETE);
    ui_window_add_child(WND(self), &self->btn_delete->button.base);
}

void ui_rodex_contents_wnd_on_draw(struct UIWindow *window)
{
    struct UIRodexContentsWnd *self = (struct UIRodexContentsWnd *)window;
    struct UIWindow *wnd = WND(self);
    struct ListNode *node;
    struct String text;
    char money[0x104];
    int x;

    ui_window_clear_canvas(wnd, 0xff00ff);
    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(RODEX_DIR "bg_rodex_read.bmp"), 0);
    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_TITLE), 0);
    ui_window_text_out(wnd, 8, 0x1d, string_cstr(&self->sender), 0, 0, 0xc, COLOUR_SENDER, 1, 0);
    ui_window_text_out(wnd, 9, 0x34, string_cstr(&self->title), 0, 0, 0xc, COLOUR_TEXT, 0, 0);
    ui_window_draw_bitmap(wnd, 0x13, 0x156, res_mgr_get(g_resMgr(), RODEX_DIR "icon_zeny.bmp"), 1);
    if (self->zeny > 0) {
        memset(money, 0, sizeof money);
        format_money64(self->zeny, money, sizeof money);
        string_init(&text);
        string_format(&text, "%s Zeny", money);
        ui_window_text_out(wnd, 0x32, 0x15e, string_cstr(&text), 0, 0, 0xb, COLOUR_GREY, 1, 0);
        string_free(&text);
    }
    for (x = 0x1a; x < 0xc4; x += ITEM_STEP)
        ui_window_draw_bitmap(wnd, x, 0x13f, res_mgr_get(g_resMgr(), RODEX_DIR "shadow_item_off.bmp"), 1);
    for (node = self->items.head->next, x = ITEM_ROW_LEFT; node != self->items.head; node = node->next, x += ITEM_STEP) {
        struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);

        ui_window_draw_bitmap(wnd, x - 4, 0x13f, res_mgr_get(g_resMgr(), RODEX_DIR "shadow_item_on.bmp"),
                            1);
        ui_window_draw_item_icon_count(wnd, x, ITEM_ROW_TOP, item, 0, 0);
        if (item->grade > 0) {
            string_init(&text);
            string_format(&text, UI_SKIN "grade_enchant\\grade_icon%d.bmp", item->grade);
            ui_window_draw_bitmap(wnd, x, 0x141, res_mgr_get(g_resMgr(), string_cstr(&text)), 1);
            string_free(&text);
        }
    }
}

void ui_rodex_contents_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIRodexContentsWnd *self = (struct UIRodexContentsWnd *)window;
    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
}

void ui_rodex_contents_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIRodexContentsWnd *self = (struct UIRodexContentsWnd *)window;
    struct ItemInfo item;
    struct String text;
    struct String name = {0};
    int tx, ty;

    ui_rodex_contents_wnd_item_at(self, &item, x, y);
    if (atoi(string_cstr(&item.id_text))) {
        tx = self->hover * ITEM_STEP + ITEM_ROW_LEFT;
        ty = 0x122;
        ui_window_to_screen(WND(self), &tx, &ty);
        item_info_get_display_name(&item, &name);
        if (item.identified == 1 && item.option_count) {
            string_init(&text);
            string_format(&text, msg_str(MSG_NAME_OPTIONS), string_cstr(&name), item.option_count);
            mode_show_tooltip(client_current_mode(), string_cstr(&text), tx, ty,
                              session_get_item_dis_color(&item), 0);
            string_free(&text);
        } else {
            mode_show_tooltip(client_current_mode(), string_cstr(&name), tx, ty,
                              session_get_item_dis_color(&item), 0);
        }
        string_free(&name);
    }
    item_info_destruct(&item);
}

void ui_rodex_contents_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIRodexContentsWnd *self = (struct UIRodexContentsWnd *)window;
    struct ItemInfo item;

    ui_rodex_contents_wnd_item_at(self, &item, x, y);
    if (atoi(string_cstr(&item.id_text)))
        ui_window_send(ui_window_open(ITEM_WND_ID), NULL, UI_MSG_SHOW_ITEM, (int)&item, 0, 0, 0);
    item_info_destruct(&item);
}

static void take(struct UIRodexContentsWnd *self, WORD id, struct UIBitmapButton *btn)
{
    struct Mode *mode = game_current_base_mode();
    struct PacketRodexTake packet;
    int len;

    if (mode && game_mode_is_blocking_window_open() == 1)
        return;
    packet.id = id;
    packet.mail_id = g_rodex->open_mail_id;
    packet.open_type = g_rodex->open_type;
    len = rag_connection_get_packet_length(rag_connection_instance(), id);
    rag_connection_send_packet(rag_connection_instance(), len, &packet);
    btn->button.disabled = 1;
    ui_window_invalidate_base(&btn->button.base);
    ui_window_invalidate_base(WND(self));
}

static void delete_mail(void)
{
    struct MailPair *node = open_mail();

    if (!node)
        return;
    if (node->mail.type & (MAIL_HAS_ZENY | MAIL_HAS_ITEMS)) {
        rodex_chat(MSG_TAKE_FIRST);
        return;
    }
    if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_DELETE_QUESTION), UI_MSGBOX_QUESTION, 0, 0, 0,
                               "메시지", 0x118, 0x78, 0)
        != UI_MSGBOX_ANSWER_YES)
        return;
    game_send_message(MODE_RODEX_DELETE, (int)g_rodex->open_mail_id,
                  g_rodex->open_type, 0, 0);
    if (g_windowMgr.rodex_contents_wnd)
        ui_close_window(WID_RODEX_CONTENTS);
}

static void reload_items(struct UIRodexContentsWnd *self)
{
    struct ItemInfo item;
    int slot;

    list_clear(&self->items, item_info_destroy);
    for (slot = 0; slot < RODEX_ITEM_SLOTS; slot++) {
        session_get_mail_item(&g_session1, &item, slot);
        if (item.count > 0) {
            item_info_copy(list_push_back(&self->items, NULL, sizeof(struct ItemInfo)), &item);
        }
        item_info_destruct(&item);
    }
    ui_window_invalidate_base(WND(self));
}

int ui_rodex_contents_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                          int val2, int val3, int val4)
{
    struct UIRodexContentsWnd *self = (struct UIRodexContentsWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case ID_TAKE_ITEMS:
            if (self->items.size)
                take(self, PACKET_CZ_REQ_ITEM_FROM_RODEX, self->btn_items);
            break;
        case ID_TAKE_ZENY:
            if (self->zeny > 0)
                take(self, PACKET_CZ_REQ_ZENY_FROM_RODEX, self->btn_zeny);
            break;
        case UI_ID_CLOSE:
            ui_close_window(WID_RODEX_CONTENTS);
            g_rodex->open_mail_id = 0;
            if (g_windowMgr.rodex_inbox_wnd)
                ui_window_send_msg(g_windowMgr.rodex_inbox_wnd, NULL, UIM_RODEX_INBOX_REFRESH, 0, 0, 0, 0);
            break;
        case ID_DELETE:
            delete_mail();
            break;
        case ID_REPLY:
            if (self->no_reply != 1)
                mode_send_message(game_current_base_mode(), MODE_RODEX_REPLY,
                              (int)string_cstr(&self->sender), 0, 0, 0);
            break;
        case ID_RETURN: {
            struct PacketRodexReturn packet;
            int len;

            packet.id = PACKET_CZ_RODEX_RETURN;
            packet.mail_id = (int)g_rodex->open_mail_id;
            len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_CZ_RODEX_RETURN);
            rag_connection_send_packet(rag_connection_instance(), len, &packet);
            break;
        }
        }
        return 0;
    case UIM_RODEX_REFRESH:
        reload_items(self);
        return 0;
    case MSG_NO_REPLY:
        if (self->no_reply && self->btn_reply) {
            self->btn_reply->button.disabled = 1;
            ui_window_invalidate_base(&self->btn_reply->button.base);
        }
        return 0;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
}
