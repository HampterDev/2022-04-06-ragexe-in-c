#include "render/renderer.h"
#include "ui/social/party_booking.h"
#include "ui/social/guild_windows.h"
#include "session/state.h"
#include "ui/window_access.h"
#include "ui/wndmgr/make_window_internal.h"
#include "ui/wndmgr/window_registry.h"
#include "contents/connection_api.h"
#include "session/session_items.h"
#include "ui/character/character.h"
#include "ui/chat/chat.h"
#include "ui/chat/chat_room_wnd.h"
#include "ui/items/items.h"
#include "ui/mail/rodex_compose.h"
#include "ui/mail/rodex_ui.h"
#include "ui/quests/quests.h"
#include "ui/social/guild_requests.h"
#include "ui/social/party_board.h"
#include "ui/social/party_windows.h"

static struct UIWindow *make_guild_tab(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow **slot;
    struct UIWindow *wnd;

    switch (window_id) {
    case 0x3c: slot = &self->guild_info_manage_wnd; break;
    case 0x3d: slot = &self->guild_member_manage_wnd; break;
    case 0x3e: slot = &self->guild_position_manage_wnd; break;
    case 0x3f: slot = &self->guild_skill_wnd; break;
    case 0x40: slot = &self->guild_banished_member_wnd; break;
    case 0x41: slot = &self->guild_notice_wnd; break;
    default: slot = &self->guild_total_info_wnd; break;
    }
    wnd = *slot;
    if (wnd) {
        wm_show(wnd);
        return wnd;
    }
    switch (window_id) {
    case 0x3c: {
        struct UIGuildInfoManageWnd *tab = mem_alloc(sizeof *tab);
        if (tab)
            ui_guild_info_manage_wnd_construct(tab);
        wnd = tab ? &tab->base.frame.base : NULL;
        break;
    }
    case 0x3d: {
        struct UIGuildMemberManageWnd *tab = mem_alloc(sizeof *tab);
        if (tab)
            ui_guild_member_manage_wnd_construct(tab);
        wnd = tab ? &tab->base.frame.base : NULL;
        break;
    }
    case 0x3e: {
        struct UIGuildPositionManageWnd *tab = mem_alloc(sizeof *tab);
        if (tab)
            ui_guild_position_manage_wnd_construct(tab);
        wnd = tab ? &tab->base.frame.base : NULL;
        break;
    }
    case 0x3f: wnd = ui_guild_skill_wnd_new(); break;
    case 0x40: wnd = ui_guild_banished_member_wnd_new(); break;
    case 0x41: wnd = ui_guild_notice_wnd_new(); break;
    default: wnd = ui_guild_total_info_wnd_new(); break;
    }
    *slot = wnd;
    ui_window_create(wnd, g_guildWndWidth, g_guildWndHeight);
    wm_move(wnd, ui_scale_x640(0), 0x93);
    ui_window_mgr_add_window(self, wnd);
    wm_set_info(wnd, &self->guild_info);
    wm_show(wnd);
    return wnd;
}

static void close_drag_plate(struct UIWindowMgr *self)
{
    if (self->drag_plate) {
        ui_window_mgr_post_quit2(self, self->drag_plate);
        self->drag_plate = NULL;
    }
}

static void send_packet_id(short id)
{
    short packet = id;

    rag_connection_send_packet(rag_connection_instance(),
                              rag_connection_get_packet_length(rag_connection_instance(), id), &packet);
}

/* 0x12d: get_window's answer is the first party plate. */
static struct UIWindow *get_first_plate(struct UIWindowMgr *self, int window_id)
{
    if (self->plates.size)
        return *LIST_VALUE(self->plates.head->next, struct UIWindow *);
    return NULL;
}

/* 0x1c */
static struct UIWindow *make_chat_room_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->chat_room_wnd;
    if (wnd == NULL) {
        wnd = ui_chat_room_wnd_new();
        self->chat_room_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_640(wnd, 0xc8, 0x12c);
        ui_window_mgr_add_window(self, wnd);
        ui_window_send_msg(self->chat_room_wnd, NULL, 0x35, (intptr_t)&self->chat_room_info, 0, 0, 0);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x22 */
static struct UIWindow *make_messenger_group_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    ui_window_mgr_post_quit1(self, 0x45);
    wnd = self->messenger_group_wnd;
    if (wnd == NULL) {
        wnd = ui_messenger_group_wnd_new();
        self->messenger_group_wnd = wnd;
        ui_window_create(wnd, 0x111, 0x146);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->messenger_group_wnd, &self->messenger_group_info);
    }
    wm_show(wnd);
    self->messenger_group_info.show = 1;
    return wnd;
}

/* 0x22: post_quit1 */
static int close_messenger_group_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (wm_close_slot(self, &self->messenger_group_wnd)) {
        self->messenger_group_info.show = 0;
        ui_window_mgr_post_quit1(&g_windowMgr, 0x35);
        ui_window_mgr_post_quit1(&g_windowMgr, 0x44);
        close_drag_plate(&g_windowMgr);
        closed = 1;
    }
    return closed;
}

/* 0x35, 0xcf */
static struct UIWindow *make_party_setting_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->party_setting_wnd;
    if (wnd == NULL) {
        int height;

        wnd = ui_party_setting_wnd_new();
        self->party_setting_wnd = wnd;
        if (window_id == 0x35)
            height = session_get_party_member_count(&g_session1) ? 0xf5 : 0xb4;
        else
            height = 0xdc;
        ui_window_create(wnd, 0x82, height);
        wm_move_640(wnd, 0xc8, 0x78);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(wnd, &self->party_setting_info);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x3b */
static struct UIWindow *make_guild_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    if (!guild_wnd_info_is_valid_info(&self->guild_info))
        guild_wnd_info_make_default(&self->guild_info);
    if ((unsigned)self->guild_info.tab > 6)
        return NULL;
    wnd = ui_window_open(0x3c + self->guild_info.tab);
    return wnd;
}

/* 0x3b: post_quit1 */
static int close_guild_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;
    int none = 1;

    if (ui_window_mgr_post_quit1(&g_windowMgr, 0x3c)) none = 0;
    if (ui_window_mgr_post_quit1(&g_windowMgr, 0x3d)) none = 0;
    if (ui_window_mgr_post_quit1(&g_windowMgr, 0x3e)) none = 0;
    if (ui_window_mgr_post_quit1(&g_windowMgr, 0x3f)) none = 0;
    if (ui_window_mgr_post_quit1(&g_windowMgr, 0x40)) none = 0;
    if (ui_window_mgr_post_quit1(&g_windowMgr, 0x41)) none = 0;
    if (ui_window_mgr_post_quit1(&g_windowMgr, 0x42)) none = 0;
    closed = none == 0;
    if (g_windowMgr.combo_box_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, 0x1a);
    return closed;
}

/* 0x44 */
static struct UIWindow *make_friend_option_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->friend_option_wnd;
    if (wnd == NULL) {
        wnd = ui_friend_option_wnd_new();
        self->friend_option_wnd = wnd;
        ui_window_create(wnd, 0xd5, 0x5a);
        wm_move_640(wnd, 0xc8, 0x78);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(wnd, &self->friend_option_info);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x45 */
static struct UIWindow *make_messenger_tab(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = ui_window_mgr_make_window(self, 0x22);
    ui_window_send_msg(wnd, NULL, UI_MSG_COMMAND, 0xcf, self->messenger_group_info.radio ? 2 : 1, 0, 0);
    return NULL;
}

/* 0x45: post_quit1 */
static int close_messenger_tab(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->messenger_group_wnd)
        ui_window_store_info(self->messenger_group_wnd);
    closed = ui_window_mgr_post_quit1(self, 0x22) != 0;
    close_drag_plate(&g_windowMgr);
    return closed;
}

/* 0x4b */
static struct UIWindow *make_guild_leave_reason_desc_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->guild_leave_reason_desc_wnd;
    if (wnd == NULL) {
        wnd = ui_guild_leave_reason_desc_wnd_new();
        self->guild_leave_reason_desc_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x50);
        wm_move_640(wnd, 0xc8, 0x78);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x4c */
static struct UIWindow *make_guild_ban_reason_desc_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->guild_ban_reason_desc_wnd;
    if (wnd == NULL) {
        wnd = ui_guild_ban_reason_desc_wnd_new();
        self->guild_ban_reason_desc_wnd = wnd;
        ui_window_create(wnd, 0x118, 0x50);
        wm_move_640(wnd, 0xc8, 0x78);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0xcf: post_quit1 */
static int close_party_setting_cf(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    ui_window_mgr_post_quit1(self, 0x35);
    closed = 1;
    return closed;
}

/* 0xd4 */
static struct UIWindow *make_guild_helper_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->guild_helper_wnd;
    if (wnd == NULL) {
        wnd = ui_guild_helper_wnd_new();
        self->guild_helper_wnd = wnd;
        ui_window_create(wnd, 0xc8, 0x64);
        wm_move_center(wnd, 0xdc, 0x78);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(wnd, &self->guild_helper_info);
    }
    wm_show(wnd);
    return wnd;
}

/* 0xd5, 0xd7 */
static struct UIWindow *make_create_guild_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = window_id == 0xd5 ? self->create_guild_wnd : self->disband_guild_wnd;
    if (wnd == NULL) {
        wnd = ui_create_guild_wnd_new(window_id == 0xd7);
        if (window_id == 0xd5)
            self->create_guild_wnd = wnd;
        else
            self->disband_guild_wnd = wnd;
        ui_window_create(wnd, 0x96, 0x64);
        wm_move_center(wnd, 0xc8, 0x78);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0xfd */
static struct UIWindow *make_guild_storage_log_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->guild_storage_log_wnd;
    if (wnd == NULL) {
        wnd = ui_guild_storage_log_new();
        self->guild_storage_log_wnd = wnd;
        ui_window_create(wnd, 0x190, 0xfa);
        {
            int y = (int)((double)g_renderer->height * 0.5 - 225.0);

            wm_move(wnd, (int)((double)g_renderer->width * 0.5 - 300.0), y);
        }
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x106 */
static struct UIWindow *make_open_mail_box_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->open_mail_box_wnd;
    if (wnd == NULL) {
        wnd = ui_open_mail_box_wnd_new();
        self->open_mail_box_wnd = wnd;
        ui_window_create(wnd, 0x2b, 0x2b);
        wm_move(wnd, g_renderer->width - 0xea, 0x10);
        ui_window_mgr_add_window(self, wnd);
    }
    wm_show(wnd);
    return wnd;
}

/* 0x107: post_quit1 */
static int close_rodex_inbox_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->rodex_inbox_wnd) {
        send_packet_id(0x9e9);
        ui_window_mgr_post_quit2(self, self->rodex_inbox_wnd);
        self->rodex_inbox_wnd = NULL;
        closed = 1;
        if (self->open_mail_box_wnd)
            ui_window_mgr_post_quit1(self, 0x106);
    }
    return closed;
}

/* 0x108: post_quit1 */
static int close_rodex_compose_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->rodex_compose_wnd) {
        send_packet_id(0xa03);
        session_reset_mail_items(&g_session1);
        session_clear_mail_item_counts(&g_session1);
        closed = wm_close_slot(self, &self->rodex_compose_wnd);
    }
    return closed;
}

/* 0x109: post_quit1 */
static int close_rodex_contents_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->rodex_contents_wnd) {
        session_clear_mail_item_counts(&g_session1);
        closed = wm_close_slot(self, &self->rodex_contents_wnd);
    }
    return closed;
}

/* 0x143 */
static struct UIWindow *make_register_party_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->register_party_wnd;
    if (wnd)
        return wnd;
    wnd = ui_register_party_wnd_new();
    self->register_party_wnd = wnd;
    ui_window_create(wnd, 0xdc, 0xd7);
    wm_move(wnd, g_renderer->centreX + 0xf0, g_renderer->centreY + 0xaa);
    ui_window_mgr_add_window(self, wnd);
    wm_set_info(self->register_party_wnd, &self->adven_register_info);
    return wnd;
}

/* 0x144 */
static struct UIWindow *make_adven_party_board_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->adven_party_board_wnd;
    if (wnd)
        return wnd;
    wnd = ui_adven_party_board_wnd_new();
    self->adven_party_board_wnd = wnd;
    ui_window_create(wnd, 0x1e0, 0x154);
    wm_move_on_centre(wnd);
    ui_window_mgr_add_window(self, wnd);
    wm_set_info(self->adven_party_board_wnd, &self->adven_party_board_info);
    return wnd;
}

/* 0x146 */
static struct UIWindow *make_request_join_party_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->request_join_party_wnd;
    if (wnd)
        return wnd;
    wnd = ui_request_join_party_wnd_new();
    self->request_join_party_wnd = wnd;
    ui_window_create(wnd, 0xdc, 0xa0);
    wm_move_on_centre(wnd);
    ui_window_mgr_add_window(self, wnd);
    return wnd;
}

/* 0x159 */
static struct UIWindow *make_apply_for_party_wnd(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    wnd = self->apply_for_party_wnd;
    if (wnd)
        return wnd;
    wnd = ui_apply_for_party_wnd_new();
    self->apply_for_party_wnd = wnd;
    ui_window_create(wnd, 0xdc, 0xbe);
    wm_move(wnd, g_renderer->centreX + 0xf0, g_renderer->centreY - 0xaa);
    ui_window_mgr_add_window(self, wnd);
    wm_set_info(self->apply_for_party_wnd, &self->adven_join_info);
    return wnd;
}

/* 0x159: post_quit1 */
static int close_apply_for_party_wnd(struct UIWindowMgr *self, int window_id)
{
    int closed = 0;

    if (self->apply_for_party_wnd) {
        ui_window_store_info(self->apply_for_party_wnd);
        ui_window_mgr_post_quit2(self, self->apply_for_party_wnd);
        self->apply_for_party_wnd = NULL;
        closed = 1;
    }
    return closed;
}

/* The rows of the guild, party, friend, chat-room and mail windows.
   window_table.h describes the columns. In ID order; slots that no window ID opens
   come last. */
static const struct WmWindowRow k_social_rows[] = {
    WM_ROW_OPEN(0x1b, chat_room_make_wnd, WM_GET_SLOT, WM_CLOSE_SLOT, ui_chat_room_make_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c),
    WM_ROW(0x1c, WM_SLOT(chat_room_wnd), WM_UNTRACKED | WM_GET_SLOT, make_chat_room_wnd, WM_CLOSE_SLOT),
    WM_ROW_OPEN(0x1d, password_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_password_wnd_new, 0xb0, 0x2e, WM_AT_640, 0xc8, 0xc8),
    WM_ROW_OPEN(0x1e, chat_room_change_wnd, WM_GET_SLOT, WM_CLOSE_SLOT, ui_chat_room_change_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c),
    WM_ROW_OPEN_INFO(0x20, exchange_accept_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_exchange_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8, exchange_accept_info),
    WM_ROW(0x22, WM_SLOT(messenger_group_wnd), WM_UNTRACKED | WM_GET_SLOT, make_messenger_group_wnd, close_messenger_group_wnd),
    WM_ROW_OPEN(0x23, join_party_accept_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_join_party_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8),
    WM_ROW(0x35, WM_SLOT(party_setting_wnd), 0, make_party_setting_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x3b, WM_NO_SLOT, 0, make_guild_wnd, close_guild_wnd),
    WM_ROW(0x3c, WM_SLOT(guild_info_manage_wnd), WM_UNTRACKED | WM_GET_SLOT, make_guild_tab, WM_CLOSE_STORED),
    WM_ROW(0x3d, WM_SLOT(guild_member_manage_wnd), WM_UNTRACKED | WM_GET_SLOT, make_guild_tab, WM_CLOSE_STORED),
    WM_ROW(0x3e, WM_SLOT(guild_position_manage_wnd), WM_UNTRACKED, make_guild_tab, WM_CLOSE_STORED),
    WM_ROW(0x3f, WM_SLOT(guild_skill_wnd), WM_UNTRACKED | WM_GET_SLOT, make_guild_tab, WM_CLOSE_STORED),
    WM_ROW(0x40, WM_SLOT(guild_banished_member_wnd), WM_UNTRACKED, make_guild_tab, WM_CLOSE_STORED),
    WM_ROW(0x41, WM_SLOT(guild_notice_wnd), WM_UNTRACKED, make_guild_tab, WM_CLOSE_STORED),
    WM_ROW(0x42, WM_SLOT(guild_total_info_wnd), WM_UNTRACKED, make_guild_tab, WM_CLOSE_STORED),
    WM_ROW(0x44, WM_SLOT(friend_option_wnd), 0, make_friend_option_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x45, WM_NO_SLOT, 0, make_messenger_tab, close_messenger_tab),
    WM_ROW_OPEN(0x46, join_guild_accept_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_join_guild_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8),
    WM_ROW_OPEN(0x48, ally_guild_accept_wnd, 0, WM_CLOSE_SLOT, ui_ally_guild_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8),
    WM_ROW(0x4b, WM_SLOT(guild_leave_reason_desc_wnd), 0, make_guild_leave_reason_desc_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x4c, WM_SLOT(guild_ban_reason_desc_wnd), 0, make_guild_ban_reason_desc_wnd, WM_CLOSE_SLOT),
    WM_ROW_OPEN(0x68, couple_accept_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_couple_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8),
    WM_ROW_OPEN(0x69, baby_accept_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_baby_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8),
    WM_ROW_OPEN(0x6d, join_friend_accept_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_join_friend_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8),
    WM_ROW_OPEN(0x6e, baby_accept_wnd2, 0, WM_CLOSE_SLOT, ui_baby_accept_wnd2_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8),
    WM_ROW_OPEN(0x77, star_place_accept_wnd, 0, WM_CLOSE_SLOT, ui_star_place_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8),
    WM_ROW_OPEN(0x8a, your_item_wnd, WM_UNTRACKED, WM_CLOSE_SLOT, ui_your_item_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8),
    WM_ROW(0xcf, WM_SLOT(party_setting_wnd), 0, make_party_setting_wnd, close_party_setting_cf),
    WM_ROW_OPEN(0xd0, party_invitation_to_wnd, 0, WM_CLOSE_SLOT, ui_party_invitation_to_wnd_new, 0x96, 0x64, WM_AT_640, 0xc8, 0x78),
    WM_ROW(0xd4, WM_SLOT(guild_helper_wnd), 0, make_guild_helper_wnd, WM_CLOSE_STORED),
    WM_ROW(0xd5, WM_SLOT(create_guild_wnd), 0, make_create_guild_wnd, WM_CLOSE_SLOT),
    WM_ROW(0xd7, WM_SLOT(disband_guild_wnd), WM_KEPT_BY_CLEAR, make_create_guild_wnd, WM_CLOSE_SLOT),
    WM_ROW(0xfd, WM_SLOT(guild_storage_log_wnd), WM_UNTRACKED | WM_GET_SLOT, make_guild_storage_log_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x106, WM_SLOT(open_mail_box_wnd), WM_UNTRACKED, make_open_mail_box_wnd, WM_CLOSE_SLOT),
    WM_ROW_OPEN(0x107, rodex_inbox_wnd, 0, close_rodex_inbox_wnd, ui_rodex_inbox_wnd_new, 0x135, 0x1a0, WM_AT_640, 0x64, 0x64),
    WM_ROW_OPEN(0x108, rodex_compose_wnd, 0, close_rodex_compose_wnd, ui_rodex_compose_wnd_new, 0x12c, 0x190, WM_AT_640, 0x78, 0x78),
    WM_ROW_OPEN(0x109, rodex_contents_wnd, 0, close_rodex_contents_wnd, ui_rodex_contents_wnd_new, 0x12c, 0x190, WM_AT_640, 0x78, 0x78),
    /* The party plates; plates.c creates them. */
    WM_ROW_GET(0x12d, get_first_plate),
    WM_ROW(0x143, WM_SLOT(register_party_wnd), 0, make_register_party_wnd, WM_CLOSE_STORED),
    WM_ROW(0x144, WM_SLOT(adven_party_board_wnd), 0, make_adven_party_board_wnd, WM_CLOSE_STORED),
    WM_ROW(0x146, WM_SLOT(request_join_party_wnd), 0, make_request_join_party_wnd, WM_CLOSE_SLOT),
    WM_ROW(0x159, WM_SLOT(apply_for_party_wnd), 0, make_apply_for_party_wnd, close_apply_for_party_wnd),
    /* The party plate being dragged (plates.c). */
    WM_ROW_SLOT(drag_plate, WM_KEPT_BY_FORGET),
};

const struct WmWindowRows wm_social_windows = WM_WINDOW_ROWS(k_social_rows);
