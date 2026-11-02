#include "navi/navi_manager.h"
#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "ui/wndmgr/wndmgr.h"
#include "session/session_items.h"
#include "session/session_state.h"
#include "ui/items/items.h"
#include "ui/shops/storage.h"
#include "ui/quests/navigation.h"

struct UIWindow *ui_window_mgr_get_window(struct UIWindowMgr *self, int window_id)
{
    struct WmWindowEntry *entry;

    switch (window_id) {
    case 0x1: return self->new_chat_wnd;
    case 0x3: return self->login_wnd;
    case 0x1b: return self->chat_room_make_wnd;
    case 0x1c: return self->chat_room_wnd;
    case 0x1e: return self->chat_room_change_wnd;
    case 0x21: return self->item_store_wnd;
    case 0x22: return self->messenger_group_wnd;
    case 0x25: return self->new_skill_list_wnd;
    case 0x3c: return self->guild_info_manage_wnd;
    case 0x3d: return self->guild_member_manage_wnd;
    case 0x3f: return self->guild_skill_wnd;
    case 0x58: return self->pet_info_wnd;
    case 0x67: return self->prohibit_list_wnd;
    case 0x71: return self->homun_info_wnd;
    case 0x72: return self->homun_skill_list_wnd;
    case 0x7d: return self->mer_info_wnd;
    case 0x7e: return self->mer_skill_list_wnd;
    case 0xb4: return self->login_otpwnd;
    case 0xc4: return self->collection_system_wnd;
    case 0xcb: return self->navigation_v4_wnd;
    case 0xcc: return self->quest_display_wnd;
    case 0xfd: return self->guild_storage_log_wnd;
    case 0x111: return self->mini_map_wnd;
    case 0x113: return self->bank_new_wnd;
    case 0x115: return self->new_select_char_wnd;
    case 0x12d:
        if (self->plates.size)
            return *LIST_VALUE(self->plates.head->next, struct UIWindow *);
        return NULL;
    case 0x145: return self->rpimage_wnd2;
    }
    entry = tree_find(&self->windows_by_id, &window_id, tree_less_int);
    return entry ? entry->wnd : NULL;
}

BYTE ui_window_mgr_is_untracked_id(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd;

    (void)self;
    switch (window_id) {
    case 0x0: case 0x1: case 0x2: case 0x3: case 0x6: case 0x7: case 0xe: case 0xf:
    case 0x10: case 0x11: case 0x12: case 0x15: case 0x16: case 0x17: case 0x18: case 0x1a:
    case 0x1c: case 0x1d: case 0x1f: case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x27:
    case 0x2a: case 0x2d: case 0x31: case 0x32: case 0x38: case 0x3a: case 0x3c: case 0x3d: case 0x3e:
    case 0x3f: case 0x40: case 0x41: case 0x42: case 0x43: case 0x46: case 0x49: case 0x4a: case 0x4e:
    case 0x4f: case 0x50: case 0x5a: case 0x5b: case 0x5e: case 0x5f: case 0x63: case 0x64:
    case 0x65: case 0x68: case 0x69: case 0x6b: case 0x6d: case 0x6f: case 0x73: case 0x74: case 0x7b:
    case 0x7c: case 0x7f: case 0x80: case 0x88: case 0x89: case 0x8a:
    case 0x90: case 0x9a: case 0x9c: case 0x9f: case 0xa0: case 0xa1: case 0xaf:
    case 0xb0: case 0xb1: case 0xb3: case 0xb4: case 0xba:  case 0xbd:
    case 0xc4:  case 0xcc: case 0xe3: case 0xfd:
    case 0x101: case 0x102:
    case 0x106: case 0x10f: case 0x115: case 0x116:
    case 0x121: case 0x127: case 0x133: case 0x145: case 0x14c: case 0x157:
    case UI_ID_NONE:
        return 1;
    case 0x91:
        wnd = ui_window_find(0x91);
        return wnd == NULL || !wnd->visible;
    }
    return 0;
}

void ui_window_mgr_forget_window_id(struct UIWindowMgr *self, int window_id)
{
    struct ListNode *node;

    if (ui_window_mgr_is_untracked_id(self, window_id))
        return;
    for (node = self->id_list.head->next; node != self->id_list.head; node = node->next) {
        if (*LIST_VALUE(node, int) == window_id) {
            list_erase(&self->id_list, node, NULL);
            return;
        }
    }
}

void ui_window_mgr_touch_window_id(struct UIWindowMgr *self, int window_id)
{
    if (ui_window_mgr_is_untracked_id(self, window_id))
        return;
    ui_window_mgr_forget_window_id(self, window_id);
    if (window_id == 0xcb)
        list_push_back(&self->id_list, &window_id, sizeof window_id);
    else
        list_push_front(&self->id_list, &window_id, sizeof window_id);
}

void ui_window_mgr_post_quit2(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    if (wnd == self->new_select_char_wnd)
        self->new_select_char_wnd = NULL;
    if (wnd == self->sel_char_for_userver_wnd)
        self->sel_char_for_userver_wnd = NULL;
    if (wnd == self->change_name_wnd)
        self->change_name_wnd = NULL;
    if (wnd == self->mouse_wnd)
        self->mouse_wnd = NULL;
    list_push_back(&self->quit_list, &wnd, sizeof wnd);
}

static void close_drag_plate(struct UIWindowMgr *self)
{
    if (self->drag_plate) {
        ui_window_mgr_post_quit2(self, self->drag_plate);
        self->drag_plate = NULL;
    }
}

static int close_slot(struct UIWindowMgr *self, struct UIWindow **slot)
{
    if (*slot == NULL)
        return 0;
    ui_window_mgr_post_quit2(self, *slot);
    *slot = NULL;
    return 1;
}

static int close_slot_stored(struct UIWindowMgr *self, struct UIWindow **slot)
{
    if (*slot == NULL)
        return 0;
    ui_window_store_info(*slot);
    ui_window_mgr_post_quit2(self, *slot);
    *slot = NULL;
    return 1;
}

static void send_packet_id(short id)
{
    short packet = id;

    rag_connection_send_packet(rag_connection_instance(),
                              rag_connection_get_packet_length(rag_connection_instance(), id), &packet);
}

BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *self, int window_id)
{
    struct UIWindow *wnd;
    struct WmClosedRect *saved;
    struct WindowInfo *rect;
    int closed = 0;

    wnd = ui_window_mgr_get_window(self, window_id);
    if (wnd) {
        saved = tree_find(&self->closed_rects, &window_id, tree_less_int);
        if (saved) {
            rect = saved->rect;
            rect->x = wnd->x;
            rect->y = wnd->y;
            rect->width = wnd->width;
            rect->height = wnd->height;
        } else {
            rect = (struct WindowInfo *)mem_alloc(sizeof *rect);
            if (rect) {
                rect->x = wnd->x;
                rect->y = wnd->y;
                rect->width = wnd->width;
                rect->height = wnd->height;
            }
            saved = tree_get(&self->closed_rects, &window_id, sizeof window_id, sizeof *saved, tree_less_int);
            saved->rect = rect;
        }
    }
    game_current_base_mode();
    if (self->modal_wnd)
        return 0;
    if (ui_window_mgr_get_window(self, window_id) == self->active_top_wnd)
        self->active_top_wnd = NULL;

    switch (window_id) {
    case 0x0: closed = close_slot(self, &self->basic_info_wnd); break;
    case 0x1:
        ui_window_mgr_post_quit1(self, 0x1a);
        if (close_slot(self, &self->new_chat_wnd)) {
            self->chat_wnd_show = 0;
            closed = 1;
        }
        break;
    case 0x8:
        if (self->item_wnd) {
            ui_item_wnd_clear_new_marks((struct UIItemWnd *)self->item_wnd, 0);
            ui_window_mgr_post_quit2(self, self->item_wnd);
            self->item_wnd = NULL;
            self->item_info.show = 0;
            closed = 1;
        }
        break;
    case 0xa:
        if (close_slot(self, &self->equip_wnd)) {
            self->equip_info.show = 0;
            g_session1.newTitles.last = g_session1.newTitles.first;
            closed = 1;
        }
        break;
    case 0xb:
        if (close_slot(self, &self->status_wnd)) {
            self->status_info.show = 0;
            closed = 1;
        }
        break;
    case 0xc:
        if (close_slot(self, &self->item_collection_wnd)) {
            closed = 1;
            if (self->item_collection_comparison_wnd)
                ui_window_mgr_post_quit1(self, 0xea);
        }
        break;
    case 0xea:
        if (close_slot(self, &self->item_collection_comparison_wnd)) {
            closed = 1;
            if (self->item_collection_wnd)
                ui_window_mgr_post_quit1(self, 0xc);
        }
        break;
    case 0xe: closed = close_slot(self, &self->minimap_zoom_wnd); break;
    case 0xf: closed = close_slot(self, &self->item_drop_cnt_wnd); break;
    case 0x10: closed = close_slot(self, &self->say_dialog_wnd); break;
    case 0x11: closed = close_slot(self, &self->choose_wnd); break;
    case 0x12: closed = close_slot(self, &self->menu_wnd); break;
    case 0x15: closed = close_slot(self, &self->notify_level_up_wnd); break;
    case 0x16: closed = close_slot(self, &self->item_shop_wnd); break;
    case 0x17:
        if (self->item_purchase_wnd) {
            mode_send_message(client_current_mode(), 0x123, 0, 0, 0, 0);
            closed = close_slot(self, &self->item_purchase_wnd);
        }
        break;
    case 0x18:
        if (self->item_sell_wnd) {
            mode_send_message(client_current_mode(), 0x123, 0, 0, 0, 0);
            closed = close_slot(self, &self->item_sell_wnd);
        }
        break;
    case 0x19:
        if (self->choose_sell_buy_wnd) {
            mode_send_message(client_current_mode(), 0x123, 0, 0, 0, 0);
            closed = close_slot(self, &self->choose_sell_buy_wnd);
        }
        break;
    case 0x1a: closed = close_slot(self, &self->combo_box_wnd); break;
    case 0x1b: closed = close_slot(self, &self->chat_room_make_wnd); break;
    case 0x1c: closed = close_slot(self, &self->chat_room_wnd); break;
    case 0x1d: closed = close_slot(self, &self->password_wnd); break;
    case 0x1e: closed = close_slot(self, &self->chat_room_change_wnd); break;
    case 0x1f: closed = close_slot(self, &self->exchange_wnd); break;
    case 0x20: closed = close_slot(self, &self->exchange_accept_wnd); break;
    case 0x21: closed = close_slot(self, &self->item_store_wnd); break;
    case 0x22:
        if (close_slot(self, &self->messenger_group_wnd)) {
            self->messenger_group_info.show = 0;
            ui_window_mgr_post_quit1(&g_windowMgr, 0x35);
            ui_window_mgr_post_quit1(&g_windowMgr, 0x44);
            close_drag_plate(&g_windowMgr);
            closed = 1;
        }
        break;
    case 0x23: closed = close_slot(self, &self->join_party_accept_wnd); break;
    case 0x24: closed = close_slot(self, &self->short_cut_wnd); break;
    case 0x25:
        ui_window_mgr_post_quit1(self, 0x2e);
        if (close_slot(self, &self->new_skill_list_wnd)) {
            self->skill_list_info.show = 0;
            closed = 1;
        }
        break;
    case 0x27: closed = close_slot(self, &self->choose_warp_wnd); break;
    case 0x28:
        if (close_slot(self, &self->merchant_item_wnd)) {
            self->merchant_item_info.show = 0;
            closed = 1;
        }
        break;
    case 0x29:
        ui_window_mgr_post_quit1(self, 0x2a);
        closed = close_slot(self, &self->merchant_shop_make_wnd);
        break;
    case 0x2a: closed = close_slot(self, &self->merchant_mirror_item_wnd); break;
    case 0x2b: closed = close_slot(self, &self->merchant_item_shop_wnd); break;
    case 0x2c: closed = close_slot(self, &self->merchant_item_purchase_wnd); break;
    case 0x2d: closed = close_slot(self, &self->merchant_item_my_shop_wnd); break;
    case 0x2e:
        if (close_slot(self, &self->skill_describe_wnd)) {
            self->skill_describe_show = 0;
            closed = 1;
        }
        break;
    case 0x2f: closed = close_slot(self, &self->card_item_illust_wnd); break;
    case 0x31: closed = close_slot(self, &self->notify_job_level_up_wnd); break;
    case 0x33: closed = close_slot(self, &self->candidate_wnd); break;
    case 0x35: closed = close_slot(self, &self->party_setting_wnd); break;
    case 0xcf:
        ui_window_mgr_post_quit1(self, 0x35);
        closed = 1;
        break;
    case 0x37: closed = close_slot(self, &self->skill_name_change_wnd); break;
    case 0x38: closed = close_slot(self, &self->npc_edit_dialog_wnd); break;
    case 0x64: closed = close_slot(self, &self->npc_text_edit_dialog_wnd); break;
    case 0x3a: closed = close_slot(self, &self->notify_item_obtain_wnd); break;
    case 0x3b: {
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
        break;
    }
    case 0x3c: closed = close_slot_stored(self, &self->guild_info_manage_wnd); break;
    case 0x3d: closed = close_slot_stored(self, &self->guild_member_manage_wnd); break;
    case 0x3e: closed = close_slot_stored(self, &self->guild_position_manage_wnd); break;
    case 0x3f: closed = close_slot_stored(self, &self->guild_skill_wnd); break;
    case 0x40: closed = close_slot_stored(self, &self->guild_banished_member_wnd); break;
    case 0x41: closed = close_slot_stored(self, &self->guild_notice_wnd); break;
    case 0x42: closed = close_slot_stored(self, &self->guild_total_info_wnd); break;
    case 0x44: closed = close_slot(self, &self->friend_option_wnd); break;
    case 0x45:
        if (self->messenger_group_wnd)
            ui_window_store_info(self->messenger_group_wnd);
        closed = ui_window_mgr_post_quit1(self, 0x22) != 0;
        close_drag_plate(&g_windowMgr);
        break;
    case 0x46: closed = close_slot(self, &self->join_guild_accept_wnd); break;
    case 0x48: closed = close_slot(self, &self->ally_guild_accept_wnd); break;
    case 0x49: closed = close_slot(self, &self->item_identify_wnd); break;
    case 0x4a: closed = close_slot(self, &self->item_composition_wnd); break;
    case 0x4b: closed = close_slot(self, &self->guild_leave_reason_desc_wnd); break;
    case 0x4c: closed = close_slot(self, &self->guild_ban_reason_desc_wnd); break;
    case 0x4e: closed = close_slot(self, &self->illust_wnd); break;
    case 0x4f: close_slot(self, &self->make_target_list_wnd); break;
    case 0x50: close_slot(self, &self->make_target_process_wnd); break;
    case 0x55: close_slot(self, &self->key_stroke_wnd); break;
    case 0x6b: close_slot(self, &self->item_repair_wnd); break;
    case 0x6f: close_slot(self, &self->weapon_refine_wnd); break;
    case 0x52: closed = close_slot(self, &self->combined_card_item_collection_wnd); break;
    case 0xfa: closed = close_slot(self, &self->combined_card_item_collection_comparison_wnd); break;
    case 0x56: closed = close_slot(self, &self->emotion_wnd); break;
    case 0x57: closed = close_slot(self, &self->emotion_list_wnd); break;
    case 0x58: closed = close_slot(self, &self->pet_info_wnd); break;
    case 0x5a:
        if (close_slot(self, &self->pet_egg_list_wnd)) {
            closed = 1;
            mode_send_message(client_current_mode(), 0x92, -1, 0, 0, 0);
        }
        break;
    case 0x5b: closed = close_slot(self, &self->pet_taming_deceive_wnd); break;
    case 0x5e: closed = close_slot(self, &self->making_arrow_list_wnd); break;
    case 0x5f: closed = close_slot(self, &self->select_cart_wnd); break;
    case 0x63: closed = close_slot(self, &self->spell_list_wnd); break;
    case 0x67: closed = close_slot(self, &self->prohibit_list_wnd); break;
    case 0x68: closed = close_slot(self, &self->couple_accept_wnd); break;
    case 0x69: closed = close_slot(self, &self->baby_accept_wnd); break;
    case 0x6d: closed = close_slot(self, &self->join_friend_accept_wnd); break;
    case 0x6e: closed = close_slot(self, &self->baby_accept_wnd2); break;
    case 0x70: closed = close_slot(self, &self->command_list_wnd); break;
    case 0x71: closed = close_slot(self, &self->homun_info_wnd); break;
    case 0x72:
        if (close_slot(self, &self->homun_skill_list_wnd)) {
            self->homun_skill_list_info.show = 0;
            closed = 1;
        }
        break;
    case 0x74: closed = close_slot(self, &self->auto_message_wnd); break;
    case 0x77: closed = close_slot(self, &self->star_place_accept_wnd); break;
    case 0x7d: closed = close_slot(self, &self->mer_info_wnd); break;
    case 0x7e:
        if (close_slot(self, &self->mer_skill_list_wnd)) {
            self->mer_skill_list_info.show = 0;
            closed = 1;
        }
        break;
    case 0x80: closed = close_slot(self, &self->change_name_wnd); break;
    case 0x84: closed = close_slot(self, &self->battle_msg_option_wnd); break;
    case 0x89:
        if (self->memorial_dun_wnd) {
            memorial_dun_info_reset();
            ui_window_mgr_post_quit2(self, self->memorial_dun_wnd);
            self->memorial_dun_wnd = NULL;
            g_session1.mdWndShown = 0;
            closed = 1;
        }
        break;
    case 0x8a: closed = close_slot(self, &self->your_item_wnd); break;
    case 0x8b: closed = close_slot(self, &self->other_equip_wnd); break;
    case 0x8c:
        if (close_slot(self, &self->kafra_move_map_service_info_wnd)) {
            closed = 1;
            if (self->navigation_v4_wnd) {
                g_navi.windowHidden = 0;
                ui_navigation_v4_wnd_relayout((struct UINavigationV4Wnd *)self->navigation_v4_wnd);
            }
        }
        break;
    case 0x8f: closed = close_slot(self, &self->notify_quest_wnd); break;
    case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98:
        if (close_slot(self, &self->item_store_sub_wnd[window_id - 0x92])) {
            closed = 1;
            if (self->item_store_wnd)
                ui_item_store_wnd_set_page_check((struct UIItemStoreWnd *)self->item_store_wnd, window_id - 0x92, 0);
        }
        break;
    case 0x135:
        if (close_slot(self, &self->item_store_sub_wnd[7])) {
            closed = 1;
            if (self->item_store_wnd)
                ui_item_store_wnd_set_page_check((struct UIItemStoreWnd *)self->item_store_wnd, 7, 0);
        }
        break;
    case 0x99: closed = close_slot(self, &self->item_store_find_wnd); break;
    case 0x9b: closed = close_slot(self, &self->esc_option_wnd); break;
    case 0x9c: case 0x9f: case 0xab: case 0xb5:
        if (ui_window_mgr_get_window(self, window_id) == NULL)
            break;
        wnd = ui_window_mgr_get_window(self, window_id);
        closed = 1;
        if (wnd == NULL)
            break;
        if (window_id == 0x9c) {
            ui_window_mgr_post_quit2(self, self->hot_key_wnd);
            self->hot_key_wnd = NULL;
        } else if (window_id == 0xb5) {
            ui_window_mgr_post_quit2(self, wnd);
            session_reset_store_search(&g_session1);
        } else {
            ui_window_mgr_post_quit2(self, wnd);
        }
        break;
    case 0xa0: closed = close_slot(self, &self->item_shop_wnd2); break;
    case 0xa1: closed = close_slot(self, &self->item_sell_wnd2); break;
    case 0xae:
        ui_window_mgr_post_quit1(self, 0xaf);
        closed = close_slot(self, &self->buying_store_make_wnd);
        break;
    case 0xaf: closed = close_slot(self, &self->buying_store_mirror_item_wnd); break;
    case 0xb0: closed = close_slot(self, &self->buying_store_item_my_shop_wnd); break;
    case 0xb1: closed = close_slot(self, &self->buying_store_item_shop_wnd); break;
    case 0xb2: closed = close_slot(self, &self->buying_store_item_sell_wnd); break;
    case 0xb3: closed = close_slot(self, &self->buying_store_mirror_sell_list_wnd); break;
    case 0xb4:
        wnd = ui_window_mgr_get_window(self, 0xb4);
        if (wnd) {
            ui_window_mgr_post_quit2(self, wnd);
            self->login_otpwnd = NULL;
            closed = 1;
        }
        break;

    case 0xc4: closed = close_slot(self, &self->collection_system_wnd); break;

    case 0xcb: closed = close_slot(self, &self->navigation_v4_wnd); break;
    case 0xcc: closed = close_slot(self, &self->quest_display_wnd); break;
    case 0xd0: closed = close_slot(self, &self->party_invitation_to_wnd); break;
    case 0xd4: closed = close_slot_stored(self, &self->guild_helper_wnd); break;
    case 0xd5: closed = close_slot(self, &self->create_guild_wnd); break;
    case 0xd7: closed = close_slot(self, &self->disband_guild_wnd); break;
    case 0xfd: closed = close_slot(self, &self->guild_storage_log_wnd); break;
    case 0xfe: closed = close_slot(self, &self->para_item_shop_wnd); break;
    case 0xff:
        if (self->para_item_purchase_wnd) {
            mode_send_message(client_current_mode(), 0x124, 0, 0, 0, 0);
            closed = close_slot(self, &self->para_item_purchase_wnd);
        }
        break;
    case 0x100:
        if (self->para_result_wnd) {
            struct Mode *game = game_current_base_mode();
            if (game)
                mode_send_message(game, 0x124, 0, 0, 0, 0);
            closed = close_slot(self, &self->para_result_wnd);
        }
        break;
    case 0x103: closed = close_slot(self, &self->second_costume_wnd); break;
    case 0x104: closed = close_slot(self, &self->combo_box_idwnd); break;
    case 0x106: closed = close_slot(self, &self->open_mail_box_wnd); break;
    case 0x107:
        if (self->rodex_inbox_wnd) {
            send_packet_id(0x9e9);
            ui_window_mgr_post_quit2(self, self->rodex_inbox_wnd);
            self->rodex_inbox_wnd = NULL;
            closed = 1;
            if (self->open_mail_box_wnd)
                ui_window_mgr_post_quit1(self, 0x106);
        }
        break;
    case 0x108:
        if (self->rodex_compose_wnd) {
            send_packet_id(0xa03);
            session_reset_mail_items(&g_session1);
            session_clear_mail_item_counts(&g_session1);
            closed = close_slot(self, &self->rodex_compose_wnd);
        }
        break;
    case 0x109:
        if (self->rodex_contents_wnd) {
            session_clear_mail_item_counts(&g_session1);
            closed = close_slot(self, &self->rodex_contents_wnd);
        }
        break;
    case 0x111: closed = close_slot(self, &self->mini_map_wnd); break;
    case 0x113: closed = close_slot(self, &self->bank_new_wnd); break;
    case 0x114:
        if (close_slot(self, &self->round_box_tooltip_wnd)) {
            self->round_box_tooltip_show = 0;
            closed = 1;
        }
        break;
    case 0x131:
        wnd = ui_window_mgr_get_window(self, 0x131);
        if (wnd) {
            if (g_windowMgr.combo_box_wnd)
                ui_window_mgr_post_quit1(&g_windowMgr, 0x1a);
            ui_window_mgr_post_quit2(self, wnd);
            closed = 1;
        }
        break;
    case 0x141:
        wnd = ui_window_mgr_get_window(self, 0x141);
        if (wnd) {
            ui_window_mgr_post_quit2(self, wnd);
            self->quest_info.show = 0;
            closed = 1;
        }
        break;
    case 0x143: closed = close_slot_stored(self, &self->register_party_wnd); break;
    case 0x144: closed = close_slot_stored(self, &self->adven_party_board_wnd); break;
    case 0x146: closed = close_slot(self, &self->request_join_party_wnd); break;
    case 0x14e: closed = close_slot(self, &self->bartermarket_wnd); break;
    case 0x14f:
        if (self->barter_item_purchase_wnd) {
            if (client_current_mode())
                mode_send_message(client_current_mode(), 0x135, 0, 0, 0, 0);
            closed = close_slot(self, &self->barter_item_purchase_wnd);
        }
        break;
    case 0x155:
        if (close_slot(self, &self->expanded_bartermarket_wnd)) {
            closed = 1;
            if (self->expanded_barter_item_purchase_wnd)
                ui_window_mgr_post_quit1(self, 0x156);
        }
        break;
    case 0x156:
        if (self->expanded_barter_item_purchase_wnd) {
            if (client_current_mode())
                mode_send_message(client_current_mode(), 0x142, 0, 0, 0, 0);
            ui_window_mgr_post_quit2(self, self->expanded_barter_item_purchase_wnd);
            self->expanded_barter_item_purchase_wnd = NULL;
            closed = 1;
            if (self->expanded_bartermarket_wnd)
                ui_window_mgr_post_quit1(self, 0x155);
        }
        break;
    case 0x159:
        if (self->apply_for_party_wnd) {
            ui_window_store_info(self->apply_for_party_wnd);
            ui_window_mgr_post_quit2(self, self->apply_for_party_wnd);
            self->apply_for_party_wnd = NULL;
            closed = 1;
        }
        break;
    default:
        wnd = ui_window_mgr_get_window(self, window_id);
        if (wnd) {
            ui_window_mgr_post_quit2(self, wnd);
            closed = 1;
        }
        break;
    }

    tree_erase_key(&self->windows_by_id, &window_id, tree_less_int, NULL);
    if (closed)
        ui_window_mgr_forget_window_id(self, window_id);
    return (BYTE)closed;
}

void ui_window_mgr_destroy_window(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    struct ListNode *node;

    if (wnd == NULL)
        return;
    for (node = self->windows.head->next; node != self->windows.head; node = node->next)
        if (*LIST_VALUE(node, struct UIWindow *) == wnd)
            break;
    if (node == self->windows.head)
        return;
    if (g_session1.windowSnap)
        tree_erase_key(&self->snap_rects, &wnd, tree_less_uint, NULL);
    snap_mgr_unregister(&g_snapMgr, wnd);
    ui_window_store_info(wnd);
    if (wnd)
        ui_window_destroy(wnd);
    list_erase(&self->windows, node, NULL);
}
