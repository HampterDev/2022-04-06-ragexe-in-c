#include "render/renderer.h"
#include "ui/social/party_booking.h"
#include "ui/social/guild_windows.h"
#include "session/state.h"
#include "ui/window_access.h"
#include "ui/wndmgr/make_window_internal.h"
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
#include "ui/social/seek_party.h"
#include "ui/social/seek_party_mb.h"

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

struct UIWindow *wm_make_social(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    switch (window_id) {
    case 0xad:
        if (self->party_booking_help_wnd)
            return NULL;
        wnd = ui_party_booking_help_wnd_new();
        self->party_booking_help_wnd = wnd;
        ui_window_create(wnd, 0x140, 0xc7);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0xa4:
        wnd = self->seek_party_wnd;
        if (wnd == NULL) {
            wnd = ui_seek_party_wnd_new();
            self->seek_party_wnd = wnd;
            ui_window_create(wnd, 0x98, 0xc7);
            wm_move_640(wnd, 0xc8, 0xc8);
            ui_window_mgr_add_window(self, wnd);
            wm_set_info(self->seek_party_wnd, &self->booking_info);
        }
        wm_show(wnd);
        g_session1.partyBooking.window_open = 1;
        break;
    case 0xa8:
        if (self->seek_party_mbwnd) {
            if (self->seek_party_wnd == NULL)
                ui_window_mgr_post_quit1(&g_windowMgr, 0xa8);
            return NULL;
        }
        if (self->seek_party_wnd)
            return NULL;
        wnd = ui_seek_party_mb_wnd_new();
        self->seek_party_mbwnd = wnd;
        ui_window_create(wnd, 0x168, 0x15e);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->seek_party_mbwnd, &self->booking_mb_info);
        break;
    case 0xa9:
        if (self->seek_party_list_wnd) {
            ui_window_mgr_post_quit1(&g_windowMgr, 0xa9);
            return NULL;
        }
        wnd = ui_seek_party_list_wnd_new();
        self->seek_party_list_wnd = wnd;
        ui_window_create(wnd, 0x226, 0x15e);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->seek_party_list_wnd, &self->booking_list_info);
        break;
    case 0x45:
        wnd = ui_window_mgr_make_window(self, 0x22);
        ui_window_send_msg(wnd, NULL, UI_MSG_COMMAND, 0xcf, self->messenger_group_info.radio ? 2 : 1, 0, 0);
        return NULL;
    case 0x22:

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
        break;
    case 0xd4:
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
        break;
    case 0xd5:
    case 0xd7:
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
        break;
    case 0x3b:
        if (!guild_wnd_info_is_valid_info(&self->guild_info))
            guild_wnd_info_make_default(&self->guild_info);
        if ((unsigned)self->guild_info.tab > 6)
            return NULL;
        wnd = ui_window_open(0x3c + self->guild_info.tab);
        break;
    case 0x3c:
    case 0x3d:
    case 0x3e:
    case 0x3f:
    case 0x40:
    case 0x41:
    case 0x42:
        wnd = make_guild_tab(self, window_id);
        break;
    case 0x4b:
        wnd = self->guild_leave_reason_desc_wnd;
        if (wnd == NULL) {
            wnd = ui_guild_leave_reason_desc_wnd_new();
            self->guild_leave_reason_desc_wnd = wnd;
            ui_window_create(wnd, 0x118, 0x50);
            wm_move_640(wnd, 0xc8, 0x78);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x4c:
        wnd = self->guild_ban_reason_desc_wnd;
        if (wnd == NULL) {
            wnd = ui_guild_ban_reason_desc_wnd_new();
            self->guild_ban_reason_desc_wnd = wnd;
            ui_window_create(wnd, 0x118, 0x50);
            wm_move_640(wnd, 0xc8, 0x78);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x44:
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
        break;
    case 0x35:
    case 0xcf:
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
        break;
    case 0xd0:
        wnd = WM_OPEN(party_invitation_to_wnd, ui_party_invitation_to_wnd_new, 0x96, 0x64, WM_AT_640, 0xc8, 0x78);
        break;
    case 0x46:
        wnd = WM_OPEN(join_guild_accept_wnd, ui_join_guild_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x48:
        wnd = WM_OPEN(ally_guild_accept_wnd, ui_ally_guild_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x23:
        wnd = WM_OPEN(join_party_accept_wnd, ui_join_party_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x68:
        wnd = WM_OPEN(couple_accept_wnd, ui_couple_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x77:
    case 0x166:
        wnd = WM_OPEN(star_place_accept_wnd, ui_star_place_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0xa3:
        wnd = WM_OPEN(mix_accept_wnd, ui_mix_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x8a:
        wnd = WM_OPEN(your_item_wnd, ui_your_item_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x69:
        wnd = WM_OPEN(baby_accept_wnd, ui_baby_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x6e:
        wnd = WM_OPEN(baby_accept_wnd2, ui_baby_accept_wnd2_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x20:
        wnd = WM_OPEN_INFO(exchange_accept_wnd, ui_exchange_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8, exchange_accept_info);
        break;
    case 0x1d:
        wnd = WM_OPEN(password_wnd, ui_password_wnd_new, 0xb0, 0x2e, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x1c:
        wnd = self->chat_room_wnd;
        if (wnd == NULL) {
            wnd = ui_chat_room_wnd_new();
            self->chat_room_wnd = wnd;
            ui_window_create(wnd, 0x118, 0x78);
            wm_move_640(wnd, 0xc8, 0x12c);
            ui_window_mgr_add_window(self, wnd);
            ui_window_send_msg(self->chat_room_wnd, NULL, 0x35, (int)(INT_PTR)&self->chat_room_info, 0, 0, 0);
        }
        wm_show(wnd);
        break;
    case 0x1e:
        wnd = WM_OPEN(chat_room_change_wnd, ui_chat_room_change_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c);
        break;
    case 0x1b:
        wnd = WM_OPEN(chat_room_make_wnd, ui_chat_room_make_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c);
        break;
    case 0x6d:
        wnd = WM_OPEN(join_friend_accept_wnd, ui_join_friend_accept_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0xfd:
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
        break;
    case 0x106:
        wnd = self->open_mail_box_wnd;
        if (wnd == NULL) {
            wnd = ui_open_mail_box_wnd_new();
            self->open_mail_box_wnd = wnd;
            ui_window_create(wnd, 0x2b, 0x2b);
            wm_move(wnd, g_renderer->width - 0xea, 0x10);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x107:
        wnd = WM_OPEN(rodex_inbox_wnd, ui_rodex_inbox_wnd_new, 0x135, 0x1a0, WM_AT_640, 0x64, 0x64);
        break;
    case 0x108:
        wnd = WM_OPEN(rodex_compose_wnd, ui_rodex_compose_wnd_new, 0x12c, 0x190, WM_AT_640, 0x78, 0x78);
        break;
    case 0x109:
        wnd = WM_OPEN(rodex_contents_wnd, ui_rodex_contents_wnd_new, 0x12c, 0x190, WM_AT_640, 0x78, 0x78);
        break;
    case 0x144:
        wnd = self->adven_party_board_wnd;
        if (wnd)
            break;
        wnd = ui_adven_party_board_wnd_new();
        self->adven_party_board_wnd = wnd;
        ui_window_create(wnd, 0x1e0, 0x154);
        wm_move_on_centre(wnd);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->adven_party_board_wnd, &self->adven_party_board_info);
        break;
    case 0x143:
        wnd = self->register_party_wnd;
        if (wnd)
            break;
        wnd = ui_register_party_wnd_new();
        self->register_party_wnd = wnd;
        ui_window_create(wnd, 0xdc, 0xd7);
        wm_move(wnd, g_renderer->centreX + 0xf0, g_renderer->centreY + 0xaa);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->register_party_wnd, &self->adven_register_info);
        break;
    case 0x146:
        wnd = self->request_join_party_wnd;
        if (wnd)
            break;
        wnd = ui_request_join_party_wnd_new();
        self->request_join_party_wnd = wnd;
        ui_window_create(wnd, 0xdc, 0xa0);
        wm_move_on_centre(wnd);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x159:
        wnd = self->apply_for_party_wnd;
        if (wnd)
            break;
        wnd = ui_apply_for_party_wnd_new();
        self->apply_for_party_wnd = wnd;
        ui_window_create(wnd, 0xdc, 0xbe);
        wm_move(wnd, g_renderer->centreX + 0xf0, g_renderer->centreY - 0xaa);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->apply_for_party_wnd, &self->adven_join_info);
        break;
    default:
        return WM_NOT_MINE;
    }
    return wm_register(self, window_id, wnd);
}
