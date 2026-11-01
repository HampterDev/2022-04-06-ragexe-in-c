#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "core/object_type.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "ui/wndmgr/make_window_internal.h"
#include "ui/character/equip_wnd.h"
#include "ui/character/skill_list_wnd.h"
#include "session/session_jobs.h"
#include "ui/character/character.h"
#include "ui/character/dress_room.h"
#include "ui/character/gauge.h"
#include "ui/character/skill_dialogs.h"
#include "ui/character/skill_tree.h"
#include "ui/items/item_collection_internal.h"
#include "ui/items/item_list.h"
#include "ui/quests/achievement.h"
#include "ui/render/graphic_setting.h"

static void wm_clear_menu_mark(int val1)
{
    struct UIWindow *w = ui_window_find(0x133);

    if (w)
        ui_window_send_msg(w, NULL, 6, 0x116, val1, 0, 0);
}

static struct UIWindow *make_basic_info(struct UIWindowMgr *self)
{
    struct UIWindow *wnd = self->basic_info_wnd, *w;

    if (wnd == NULL) {
        wnd = ui_basic_info_wnd_new();
        self->basic_info_wnd = wnd;
        ui_window_create(wnd, 0xdc,
                         session_is_fourth_job(&g_session1, session_get_drawn_job(&g_session1)) == 1 ? 0x47 : 0x37);
        wm_move(wnd, 0, 0);
        ui_window_mgr_add_window(self, wnd);
        ui_window_send_msg(self->basic_info_wnd, self->basic_info_wnd, UI_MSG_SET_INFO,
                 (int)(INT_PTR)&self->basic_info_info, 0, 0, 0);
        snap_mgr_register_root(&g_snapMgr, wnd);
        w = ui_window_open(0x133);
        if (w) {
            int x = 0, y = 0;

            ui_window_to_screen(wnd, &x, &y);
            wm_move(w, x, wnd->height + y);
        }
    }
    {
        wm_show(wnd);
    }
    if (self->item_info.show)
        ui_window_mgr_make_window(self, 0x8);
    if (self->equip_info.show)
        ui_window_mgr_make_window(self, 0xa);
    if (self->status_info.show)
        ui_window_mgr_make_window(self, 0xb);
    if (self->option_info.show)
        ui_window_mgr_make_window(self, 0xd);
    if (self->skill_list_info.show)
        ui_window_mgr_make_window(self, 0x25);
    if (self->quest_info.show)
        ui_window_mgr_make_window(self, 0x141);
    if (game_camera_is_free())
        ui_window_mgr_make_window(self, 0xe);
    ui_window_mgr_show_option_window(self, 0x1, self->chat_wnd_show);
    ui_window_mgr_show_option_window(self, 0x24, self->shortcut_info.show);
    ui_window_mgr_show_option_window(self, 0xf, 0);
    if (self->messenger_group_info.show)
        ui_window_mgr_make_window(self, 0x45);
    if (g_session1.lowMemory && g_session1.autoOpenDetailWindow && g_spriteMode == 2 && g_textureMode == 2)
        ui_window_mgr_make_window(self, 0x131);
    if (session_has_tracked_achievements(&g_session1) == 1)
        ui_window_mgr_make_window(self, 0x10f);
    if (self->macro_reporter_info.open == 1)
        ui_window_mgr_make_window(self, 0x11e);
    ui_window_mgr_restore_plates(self);
    return wnd;
}

static struct UIWindow *wm_frame_cast(struct UIWindow *wnd)
{
    return object_as_type(wnd, OBJECT_FRAME_WINDOW);
}

struct UIWindow *wm_make_character(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd = NULL;

    switch (window_id) {
    case 0x0:
        wnd = make_basic_info(self);
        break;
    case 0x8:
        wnd = self->item_wnd;
        if (wnd == NULL) {
            wnd = ui_item_wnd_new();
            self->item_wnd = wnd;
            ui_window_create(wnd, 0x118, 0x78);
            wm_move(wnd, 0, 0x78);
            ui_window_mgr_add_window(self, wnd);
            wm_set_info(self->item_wnd, &self->item_info);
            snap_mgr_register(&g_snapMgr, wnd);
        }
        wm_show(wnd);
        self->item_info.show = 1;
        wm_clear_menu_mark(0xba);
        break;
    case 0xa:
        wnd = self->equip_wnd;
        if (wnd == NULL) {
            wnd = ui_equip_wnd_new();
            self->equip_wnd = wnd;
            ui_equip_wnd_set_viewing_other(wnd, 0);
            ui_window_create(wnd, 0x118, 0xa7);
            wm_move_640(wnd, 0xb9, 0x12c);
            ui_window_mgr_add_window(self, wnd);
            wm_set_info(self->equip_wnd, &self->equip_info);
            snap_mgr_register(&g_snapMgr, wnd);
        }
        wm_show(wnd);
        self->equip_info.show = 1;
        break;
    case 0xb:
        wnd = self->status_wnd;
        if (wnd == NULL) {
            wnd = ui_status_wnd_new();
            self->status_wnd = wnd;
            ui_window_create(wnd, 0x118, 0x8d);
            wm_move_640(wnd, 0xb9, 0x12c);
            ui_window_mgr_add_window(self, wnd);
            wm_set_info(self->status_wnd, &self->status_info);
            snap_mgr_register(&g_snapMgr, wnd);
        }
        wm_show(wnd);
        self->status_info.show = 1;
        ui_window_mgr_post_quit1(self, 0x15);
        wm_clear_menu_mark(0xb8);
        break;
    case 0x25:
        wnd = self->new_skill_list_wnd;
        if (wnd == NULL) {
            wnd = ui_new_skill_list_wnd_new();
            self->new_skill_list_wnd = wnd;
            ui_window_create(wnd, 0x230, 0x190);
            wm_move_640(wnd, 0x32, 0x32);
            ui_window_mgr_add_window(self, wnd);
            wm_set_info(self->new_skill_list_wnd, &self->skill_list_info);
        }
        wm_show(wnd);
        self->skill_list_info.show = 1;
        ui_window_mgr_post_quit1(self, 0x31);
        wm_clear_menu_mark(0xbc);
        break;
    case 0xaa:
        if (self->job_list_wnd)
            return NULL;
        wnd = ui_job_list_wnd_new();
        self->job_list_wnd = wnd;
        ui_window_create(wnd, 0x98, 0xc7);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        wm_set_info(self->job_list_wnd, &self->booking_mb_check_info);
        break;
    case 0x3a:
        wnd = self->notify_item_obtain_wnd;
        if (wnd == NULL) {
            wnd = ui_notify_item_obtain_wnd_new();
            self->notify_item_obtain_wnd = wnd;
            ui_window_create(wnd, 0xb4, 0x20);
            wm_move(wnd, ui_center_x640(0xdc), 0x4b);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x37:
        wnd = self->skill_name_change_wnd;
        if (wnd == NULL) {
            wnd = ui_skill_name_change_wnd_new();
            self->skill_name_change_wnd = wnd;
            ui_window_create(wnd, 0x118, 0x30);
            wm_move_center(wnd, 0xb9, 0xb4);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0xf:
        wnd = self->item_drop_cnt_wnd;
        if (wnd)
            break;
        wnd = ui_item_drop_cnt_wnd_new();
        self->item_drop_cnt_wnd = wnd;
        ui_window_create(wnd, 0xb6, 0x2e);
        wm_move_640(wnd, 0xc8, 0xc8);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x2f:
        wnd = WM_OPEN(card_item_illust_wnd, ui_card_item_illust_wnd_new, 0x12c, 0x1a1, WM_AT_640, 0, 0);
        break;
    case 0xc9:
        wnd = WM_OPEN(monster_display_wnd, ui_monster_display_wnd_new, 0x64, 0x96, WM_AT_640, 0, 0);
        break;
    case 0x2e:
        wnd = self->skill_describe_wnd;
        if (wnd == NULL) {
            wnd = ui_skill_describe_wnd_new();
            self->skill_describe_wnd = wnd;
            ui_window_create(wnd, 0x118, 0x78);
            wm_move_640_up(wnd, 0xb9, 0x12c, 0x78);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x52:
        wnd = WM_OPEN_INFO(combined_card_item_collection_wnd, ui_combined_card_item_collection_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c, combined_card_item_collection_info);
        break;
    case 0xc:
        wnd = WM_OPEN_INFO(item_collection_wnd, ui_item_collection_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c, item_collection_info);
        break;
    case 0xea:
        wnd = self->item_collection_comparison_wnd;
        if (wnd == NULL) {
            wnd = ui_item_collection_comparison_wnd_new();
            self->item_collection_comparison_wnd = wnd;
            ui_window_create(wnd, 0x118, 0x78);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_set_info(wnd, NULL);
        wm_show(wnd);
        break;
    case 0xfa:
        wnd = WM_OPEN_INFO(combined_card_item_collection_comparison_wnd, ui_combined_card_item_collection_comparison_wnd_new, 0x118, 0x78, WM_AT_640, 0xb9, 0x12c, combined_card_item_collection_info);
        break;
    case 0x4d:
        wnd = ui_monster_info_wnd_new();
        ui_window_create(wnd, 0x190, 0xc8);
        wm_move(wnd, 0, 0);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x4f:
        wnd = WM_OPEN(make_target_list_wnd, ui_make_target_list_wnd_new, 0x118, 0x96, WM_AT_640_UP, 0xb9, 0x12c);
        break;
    case 0x50:
        wnd = WM_OPEN(make_target_process_wnd, ui_make_target_process_wnd_new, 0x118, 0x96, WM_AT_640_UP, 0xb9, 0x12c);
        break;
    case 0x51:
        wnd = WM_OPEN(make_target_result_wnd, ui_make_target_result_wnd_new, 0x118, 0x78, WM_AT_640_UP, 0xb9, 0x12c);
        break;
    case 0x58:
        if (g_session1.petJob == -1)
            return NULL;
        wnd = self->pet_info_wnd;
        if (wnd == NULL) {
            wnd = ui_pet_info_wnd_new();
            self->pet_info_wnd = wnd;
            ui_window_create(wnd, 0x118, 0xa0);
            wm_move_640_up(wnd, 0xb9, 0x190, 0x3c);
            wm_set_info(wnd, &self->pet_info_info);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x5a:
        wnd = WM_OPEN(pet_egg_list_wnd, ui_pet_egg_list_wnd_new, 0xc8, 0xc8, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x5b:
        wnd = self->pet_taming_deceive_wnd;
        if (wnd == NULL) {
            wnd = ui_pet_taming_deceive_wnd_new();
            self->pet_taming_deceive_wnd = wnd;
            ui_window_create(wnd, 0xc4, 0x100);
            wm_move(wnd, g_renderer->width / 2 - 0x62, g_renderer->height / 2 - 0x80);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x5e:
        wnd = WM_OPEN(making_arrow_list_wnd, ui_making_arrow_list_wnd_new, 0xc8, 0xc8, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x5f:
        wnd = WM_OPEN(select_cart_wnd, ui_select_cart_wnd_new, 0x181, 0x62, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x63:
        if (self->spell_list_wnd) {
            ui_window_mgr_post_quit2(self, self->spell_list_wnd);
            self->spell_list_wnd = NULL;
        }
        wnd = WM_OPEN(spell_list_wnd, ui_spell_list_wnd_new, 0xc8, 0xc8, WM_AT_640, 0xc8, 0xc8);
        break;
    case 0x71:
        if (g_session1.homunJob == -1)
            return NULL;
        wnd = self->homun_info_wnd;
        if (wnd == NULL) {
            wnd = ui_homun_info_wnd_new();
            self->homun_info_wnd = wnd;
            ui_window_create(wnd, 0x118, 0xd0);
            wm_move_640_up(wnd, 0xb9, 0x190, 0x3c);
            wm_set_info(wnd, &self->homun_info_info);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x72:
        wnd = self->homun_skill_list_wnd;
        if (wnd == NULL) {
            wnd = ui_skill_list_wnd_new();
            self->homun_skill_list_wnd = wnd;
            ui_skill_list_set_kind(wnd, WM_SKILL_LIST_HOMUN);
            ui_window_create(wnd, 0x118, 0x78);
            wm_move_640(wnd, 0xc8, 0xc8);
            ui_window_mgr_add_window(self, wnd);
            wm_set_info(self->homun_skill_list_wnd, &self->homun_skill_list_info);
        }
        ui_skill_list_set_kind(wnd, WM_SKILL_LIST_HOMUN);
        wm_show(wnd);
        self->homun_skill_list_info.show = 1;
        break;
    case 0x7d:
        if (g_session1.mercJob == -1)
            return NULL;
        wnd = self->mer_info_wnd;
        if (wnd == NULL) {
            wnd = ui_mer_info_wnd_new();
            self->mer_info_wnd = wnd;
            ui_window_create(wnd, 0x118, 0xb4);
            wm_move_640_up(wnd, 0xb9, 0x190, 0x3c);
            wm_set_info(wnd, &self->mer_info_info);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x7e:
        wnd = self->mer_skill_list_wnd;
        if (wnd == NULL) {
            wnd = ui_skill_list_wnd_new();
            self->mer_skill_list_wnd = wnd;
            ui_skill_list_set_kind(wnd, WM_SKILL_LIST_MERCENARY);
            ui_window_create(wnd, 0x118, 0x78);
            wm_move_640(wnd, 0xc8, 0xc8);
            ui_window_mgr_add_window(self, wnd);
            wm_set_info(self->mer_skill_list_wnd, &self->mer_skill_list_info);
        }
        ui_skill_list_set_kind(wnd, WM_SKILL_LIST_MERCENARY);
        wm_show(wnd);
        self->mer_skill_list_info.show = 1;
        break;
    case 0x8b:
        wnd = self->other_equip_wnd;
        if (wnd == NULL) {
            wnd = ui_equip_wnd_new();
            self->other_equip_wnd = wnd;
            ui_equip_wnd_set_viewing_other(wnd, 1);
            ui_window_create(wnd, 0x118, 0xa7);
            wm_move_640(wnd, 0xb9, 0x12c);
            ui_window_mgr_add_window(self, wnd);
            snap_mgr_register(&g_snapMgr, wnd);
        }
        wm_show(wnd);
        break;
    case 0xc4:
        wnd = self->collection_system_wnd;
        if (wnd == NULL) {
            wnd = ui_collection_system_wnd_new();
            self->collection_system_wnd = wnd;
            ui_window_create(wnd, 0xc4, 0x100);
            wm_move(wnd, g_renderer->width / 2 - 0x62, g_renderer->height / 2 - 0x80);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_show(wnd);
        break;
    case 0x103:
        wnd = wm_frame_cast(ui_window_mgr_get_window(self, 0x103));
        if (wnd)
            break;
        wnd = ui_second_costume_wnd_new();
        if (!wnd)
            return NULL;
        self->second_costume_wnd = wnd;
        ui_window_create(wnd, 0x18b, 0xcd);
        {
            int y = scale_y_from480(0xc8) + 0x2e;

            wm_move(wnd, ui_center_x640(0xc8), y);
        }
        wm_show(wnd);
        ui_window_mgr_add_window(self, wnd);
        break;
    case 0x10e:
        wnd = wm_frame_cast(ui_window_mgr_get_window(self, 0x10e));
        if (wnd == NULL) {
            wnd = ui_achievement_wnd_new();
            if (!wnd)
                return NULL;
            ui_window_create(wnd, 0x2ee, 0x1d4);
            wm_move(wnd, wm_center_x(0x2ee), wm_center_y(0x1d4));
            wm_show(wnd);
            ui_window_mgr_add_window(self, wnd);
        }
        wm_clear_menu_mark(0x1d1);
        break;
    case 0x10f:
        wnd = wm_frame_cast(ui_window_mgr_get_window(self, 0x10f));
        if (wnd)
            break;
        wnd = ui_ach_tracing_wnd_new();
        if (!wnd)
            return NULL;
        ui_window_create(wnd, 0x12c, 0x96);
        wm_move(wnd, g_renderer->width - 0x15e, wm_center_y(0x96));
        wm_show(wnd);
        wnd->list_rank = 5;
        ui_window_mgr_add_window(self, wnd);
        wm_window_list_sort(&self->windows);
        wm_set_info(wnd, &self->ach_tracing_pos);
        break;
    case 0x126:
        wnd = ui_window_mgr_get_window(self, 0x126);
        if (wnd)
            break;
        wnd = ui_preview_equip_wnd_new();
        ui_window_create(wnd, 0x118, 0x104);
        {
            int y = (int)((float)(g_renderer->height - 0x104) * 0.5f);

            wm_move(wnd, (int)((float)(g_renderer->width - 0x118) * 0.5f), y);
        }
        wm_set_info(wnd, &self->preview_equip_pos);
        ui_window_mgr_add_window(self, wnd);
        break;
    default:
        return WM_NOT_MINE;
    }
    return wm_register(self, window_id, wnd);
}
