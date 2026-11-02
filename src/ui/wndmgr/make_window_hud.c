#include "navi/navi_manager.h"
#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "contents/mouse.h"
#include "ui/window_access.h"
#include "ui/wndmgr/make_window_internal.h"
#include "core/object_type_tables.h"
#include "ui/character/character.h"
#include "ui/character/popup_menu.h"
#include "ui/chat/chat.h"
#include "ui/chat/chat_window.h"
#include "ui/chat/sub_chat.h"
#include "ui/hotkeys/hotkey_bars.h"
#include "ui/hotkeys/hotkey_guide.h"
#include "ui/items/items.h"
#include "ui/map/map_std.h"
#include "ui/quests/navigation.h"
#include "ui/quests/npc_input.h"
#include "ui/quests/npc_menu.h"
#include "ui/quests/quests.h"
#include "ui/quests/say_dialog.h"
#include "ui/render/character_render.h"
#include "ui/render/render_std.h"
#include "ui/social/social.h"
#include "ui/worldmap/romap.h"

struct UIWindow *wm_make_hud(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    switch (window_id) {
    case 0x1:
        wnd = self->new_chat_wnd;
        if (wnd == NULL) {
            wnd = ui_new_chat_wnd_new();
            self->new_chat_wnd = wnd;
            ui_window_create(wnd, 0x258, self->chat_wnd_height);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_set_info(self->new_chat_wnd, NULL);
        break;
    case 0xcc:
        if (self->quest_display_wnd)
            return NULL;
        self->quest_display_wnd = ui_quest_display_new();
        ui_quest_display_create((struct UIQuestDisplay *)self->quest_display_wnd, 0xfa, 0x190);
        wm_move(self->quest_display_wnd, self->screen_width - 0x140, 0x96);
        self->quest_display_wnd->list_rank = 5;
        ui_window_mgr_add_window(self, self->quest_display_wnd);
        wm_window_list_sort(&self->windows);
        wm_set_info(self->quest_display_wnd, &self->quest_display);
        return NULL;
    case 0x24:
        wnd = self->short_cut_wnd;
        if (wnd)
            break;
        wnd = ui_short_cut_wnd_new();
        self->short_cut_wnd = wnd;
        ui_window_create(wnd, 0x122, 0x22);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->short_cut_wnd, &self->shortcut_info);
        snap_mgr_register(&g_snapMgr, wnd);
        break;
    case 0x38:
        wnd = self->npc_edit_dialog_wnd;
        if (wnd == NULL) {
            wnd = ui_npc_edit_dialog_wnd_new();
            self->npc_edit_dialog_wnd = wnd;
            ui_window_create(wnd, 0xb0, 0x2e);
            wm_move_640(wnd, 0xc8, 0xc8);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x64:
        wnd = self->npc_text_edit_dialog_wnd;
        if (wnd == NULL) {
            wnd = ui_npc_text_edit_dialog_wnd_new();
            self->npc_text_edit_dialog_wnd = wnd;
            ui_window_create(wnd, 0x15a, 0x2e);
            wm_move_640(wnd, 0xc8, 0xc8);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x4e:
        wnd = self->illust_wnd;
        if (wnd) {
            ui_window_send_msg(wnd, NULL, UI_MSG_SET_INFO, -1, 0, 0, 0);
            wm_show(wnd);
            break;
        }
        wnd = ui_illust_wnd_new();
        self->illust_wnd = wnd;
        ui_window_create(wnd, 4, 4);
        wm_move_640(wnd, 0xc8, 0x78);
        list_push_front(&self->windows, &wnd, sizeof wnd);
        wm_window_list_sort(&self->windows);
        wm_show(wnd);
        break;
    case 0x33:
        wnd = self->candidate_wnd;
        if (wnd == NULL) {
            int width = self->candidate_box ? 0x78 : 0x124, height = self->candidate_box ? 0xd4 : 0x22;

            wnd = ui_candidate_wnd_new();
            self->candidate_wnd = wnd;
            ui_window_create(wnd, width, height);
            wm_move(wnd, g_renderer->width - width, g_renderer->height - height);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x10:
        wnd = WM_OPEN_INFO(say_dialog_wnd, ui_say_dialog_wnd_new, 0x118, 0xb4, WM_AT_640, 0xc8, 0x64, say_dialog_info);
        break;
    case 0x27:
        wnd = WM_OPEN(choose_warp_wnd, ui_choose_warp_wnd_new, 0x118, 0x78, WM_AT_640, 0xc8, 0x12c);
        break;
    case 0x11:
        wnd = self->choose_wnd;
        if (wnd == NULL) {
            wnd = ui_choose_wnd_new();
            self->choose_wnd = wnd;
            ui_window_create(wnd, 0x118, 0x78);
            wm_move_640(wnd, 0xc8, 0x12c);
            ui_window_mgr_add_window(self, wnd);
            wm_set_info(self->choose_wnd, &self->choose_info);
        }
        wm_show(wnd);
        break;
    case 0x1a:
        wnd = WM_OPEN(combo_box_wnd, ui_combo_box_wnd_new, 0x10, 0x10, WM_AT_640, 0xc8, 0x12c);
        break;
    case 0x104:
        wnd = WM_OPEN(combo_box_idwnd, ui_combo_box_id_wnd_new, 0x10, 0x10, WM_AT_640, 0xc8, 0x12c);
        break;
    case 0x12:
        wnd = WM_OPEN(menu_wnd, ui_menu_wnd_new, 4, 4, WM_AT_640, 0xc8, 0x12c);
        break;
    case 0xe:
        wnd = self->minimap_zoom_wnd;
        if (wnd == NULL) {
            wnd = ui_minimap_zoom_wnd_new();
            self->minimap_zoom_wnd = wnd;
            ui_window_create(wnd, 0x80, 0x8c);
            wm_move(wnd, g_renderer->width - 0x91, 0x11);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x31:
        wnd = self->notify_job_level_up_wnd;
        if (wnd == NULL) {
            wnd = ui_notify_job_level_up_wnd_new();
            self->notify_job_level_up_wnd = wnd;
            ui_window_create(wnd, 0x2b, 0x2b);
            wm_move(wnd, 0, g_renderer->height - 0x2b);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x15:
    case 0x8f:
        wnd = window_id == 0x15 ? self->notify_level_up_wnd : self->notify_quest_wnd;
        if (wnd == NULL) {
            if (window_id == 0x15) {
                wnd = (struct UIWindow *)ui_notify_level_up_wnd_new();
                self->notify_level_up_wnd = wnd;
            } else {
                wnd = ui_notify_quest_wnd_new();
                self->notify_quest_wnd = wnd;
            }
            ui_window_create(wnd, 0x2b, 0x2b);
            wm_move(wnd, g_renderer->width - 0x2b, g_renderer->height - 0x2b);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x54:
        wnd = ui_talkbox_trap_input_wnd_new();
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_640_up(wnd, 0xb9, 0x12c, 0x3c);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x65:
        wnd = ui_graffi_strbox_wnd_new();
        ui_window_create(wnd, 0x118, 0x78);
        wm_move_640_up(wnd, 0xb9, 0x12c, 0x3c);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x55:
        wnd = WM_OPEN(key_stroke_wnd, ui_key_stroke_wnd_new, 0x3c, 0x12, WM_AT_640_UP, 0xb9, 0x12c);
        break;
    case 0x56:
        wnd = self->emotion_wnd;
        if (wnd == NULL) {
            wnd = ui_emotion_wnd_new();
            self->emotion_wnd = wnd;
            ui_window_create(wnd, 0x177, 0x104);
            wm_move_640_up(wnd, 0xb9, 0x12c, 0x3c);
            wm_set_info(wnd, &self->emotion_info);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x57:
        wnd = self->emotion_list_wnd;
        if (wnd == NULL) {
            wnd = ui_emotion_list_wnd_new();
            self->emotion_list_wnd = wnd;
            ui_window_create(wnd, 0xf6, 0x14a);
            wm_move_640_up(wnd, 0xb9, 0x190, 0x3c);
            wm_set_info(wnd, &self->emotion_list_info);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x67:
        wnd = WM_OPEN(prohibit_list_wnd, ui_prohibit_list_wnd_new, 0x1f4, 0x190, WM_AT_640, 0x64, 0x64);
        break;
    case 0x70:
        wnd = WM_OPEN(command_list_wnd, ui_command_list_wnd_new, 0xfa, 0xc8, WM_AT_640, 0x190, 0xc8);
        break;
    case 0xcb:
        if (self->navigation_v4_wnd)
            return NULL;

        g_navi.windowHidden = 0;
        wnd = ui_navigation_v4_wnd_new();
        self->navigation_v4_wnd = wnd;
        ui_window_create(wnd, 0x110, 0x152);
        wm_set_info(self->navigation_v4_wnd, &self->navigation_info);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0xe5:
        if (ui_window_mgr_get_window(self, 0xe5))
            return NULL;
        wnd = ui_navigation_help_wnd_new();
        ui_window_create(wnd, 0x136, 0xc8);
        wm_move(wnd, 0x64, 0x64);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x132:
    case 0x13a:
    {
        int x = 0xc8, y = 0xc8;

        if (ui_window_mgr_get_window(self, window_id))
            return NULL;
        if (self->navigation_v4_wnd) {
            x = self->navigation_v4_wnd->x;
            y = self->navigation_v4_wnd->y;
        }
        if (window_id == 0x132) {
            wnd = ui_navigationroadicon_wnd_new();
            ui_window_create(wnd, 0x9e, 0x66);
            wm_move(wnd, x + 0x71, y + 0xd3);
        } else {
            wnd = ui_navigation_ruide_wnd_new();
            ui_window_create(wnd, 0x96, 0x152);
            wm_move(wnd, x + 0x110, y);
        }
        ui_window_mgr_add_window(self, wnd);
        break;
    }
    case 0x74:
        wnd = WM_OPEN(auto_message_wnd, ui_auto_message_wnd_new, 0x118, 0x30, WM_AT_CENTER, 0xb9, 0xb4);
        break;
    case 0x84:
        wnd = WM_OPEN(battle_msg_option_wnd, ui_battle_msg_option_wnd_new, 0x118, 0xc8, WM_AT_640, 0x190, 0xc8);
        break;
    case 0x89:
        if (string_equal_cstr(&g_session1.mdName, ""))
            return NULL;
        wnd = self->memorial_dun_wnd;
        if (wnd == NULL) {
            wnd = ui_memorial_dun_wnd_new();
            self->memorial_dun_wnd = wnd;
            ui_window_create(wnd, 0xc8, 0x96);
            if (g_session1.mdWndX >= 0 && g_session1.mdWndY >= 0)
                wm_move(wnd, g_session1.mdWndX, g_session1.mdWndY);
            else
                wm_move(wnd, 0, 0x136);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        g_session1.mdWndShown = 1;
        break;
    case 0x8c:
        wnd = self->kafra_move_map_service_info_wnd;
        if (wnd == NULL) {
            wnd = ui_ro_map_wnd_new();
            self->kafra_move_map_service_info_wnd = wnd;
            ui_window_create(wnd, g_renderer->width, g_renderer->height);
            wm_move(wnd, 0, 0);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        ui_window_mgr_raise_window(&g_windowMgr, self->kafra_move_map_service_info_wnd);
        ui_window_mgr_raise_window(&g_windowMgr, self->navigation_v4_wnd);
        break;
    case 0x9b:
        wnd = WM_OPEN(esc_option_wnd, ui_esc_option_wnd_new, 0x118, 0x8f, WM_AT_640, 0xb9, 0x12c);
        break;
    case 0x111:
        if (self->mini_map_wnd)
            return NULL;
        wnd = ui_mini_map_wnd_new();
        self->mini_map_wnd = wnd;
        ui_window_create(wnd, self->minimap_info.width, self->minimap_info.height);
        wm_set_info(wnd, &self->minimap_info);
        wm_show(wnd);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x114:
        if (ui_window_mgr_get_window(self, 0x114)) {
            wnd = self->round_box_tooltip_wnd;
            wm_show(wnd);
            break;
        }
        wnd = ui_round_box_tooltip_wnd_new();
        self->round_box_tooltip_wnd = wnd;
        ui_window_create(wnd, 0x64, 0xc8);
        wm_move_640_up(wnd, 0xb9, 0x12c, 0x78);
        ui_window_mgr_add_window(self, wnd);
        wm_show(wnd);
        break;
    case 0x121:
        wnd = ui_window_mgr_get_window(self, 0x121);
        if (wnd == NULL) {
            int x = g_renderer->width - 0xc8, y = 0xc8;

            wnd = ui_view_camera_info_wnd_new();
            ui_window_create(wnd, 0x96, 0x64);
            if (self->mini_map_wnd) {
                x = self->mini_map_wnd->x;
                y = self->mini_map_wnd->height + 0x14 + self->mini_map_wnd->y;
            }
            wm_move(wnd, x, y);
            wnd->list_rank = 1;
            ui_window_mgr_add_window(self, wnd);
            wm_window_list_sort(&self->windows);
        }
        ui_window_set_show(wnd, g_session1.cameraInfoShown);
        break;
    case 0x12f:
        wnd = ui_window_mgr_get_window(self, 0x12f);
        if (wnd)
            break;
        wnd = ui_hotkey_guide_wnd_new();
        ui_window_create(wnd, 0x3a6, 0x1f2);
        wm_move(wnd, wm_center_x(0x3a6), wm_center_y(0x1f2));
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x133:
        wnd = ui_window_mgr_get_window(self, 0x133);
        if (wnd)
            break;
        wnd = ui_menu_icon_wnd_new();
        ui_window_create(wnd, 0xdc, 0xc5);
        wm_move(wnd, 0, 0xe1);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x131:
        wnd = ui_window_mgr_get_window(self, 0x131);
        if (wnd == NULL) {
            wnd = ui_graphic_setting_wnd_new();
            ui_window_create(wnd, 0x190, 0x15e);
            wm_move_640(wnd, 0xc8, 0xc8);
            ui_window_mgr_add_window(self, wnd);
            wm_set_info(wnd, &self->graphic_setting_info);
        }
        wm_show(wnd);
        break;
    case 0x134:
        wnd = ui_window_mgr_get_window(self, 0x134);
        if (wnd == NULL) {
            wnd = ui_show_warp_wnd_new();
            ui_window_create(wnd, 0x118, 0x78);
            wm_move_640(wnd, 0xc8, 0x12c);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x141:

        wnd = ui_window_mgr_get_window(self, 0x141);
        if (wnd == NULL) {
            wnd = ui_renew_quest_wnd_new();
            ui_window_create(wnd, 0x17d, 0x1d2);
            wm_move(wnd, wm_center_x(0x17d), wm_center_y(0x1d2));
            ui_window_mgr_add_window(self, wnd);
            wm_set_info(wnd, &self->quest_info);
            snap_mgr_register(&g_snapMgr, wnd);
        }
        wm_show(wnd);
        ui_window_mgr_post_quit1(self, 0x8f);
        self->quest_info.show = 1;
        break;
    case 0x142:

        wnd = ui_window_mgr_get_window(self, 0x142);
        if (wnd == NULL) {
            struct UIWindow *quest;

            wnd = ui_renew_detail_quest_info_wnd_new();
            ui_window_create(wnd, 0x156, 0x19c);
            quest = ui_window_mgr_get_window(self, 0x141);
            if (quest)
                wm_move(wnd, quest->x + quest->width, quest->y);
            else
                wm_move(wnd, wm_center_x(0x156), wm_center_y(0x19c));
            ui_window_mgr_add_window(self, wnd);
            snap_mgr_register(&g_snapMgr, wnd);
        }
        wm_show(wnd);
        break;
    case 0x148:
        wnd = ui_window_mgr_get_window(self, 0x148);
        if (wnd)
            break;
        wnd = ui_check_attendance_wnd_new();
        ui_window_create(wnd, 0x1e8, 0x19d);
        {
            int y = wm_times_half(g_renderer->height - 0x19d);

            wm_move(wnd, wm_times_half(g_renderer->width - 0x1e8), y);
        }
        ui_window_mgr_add_window(self, wnd);
        break;
    default:
        return WM_NOT_MINE;
    }
    return wm_register(self, window_id, wnd);
}
