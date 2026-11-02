#include "session/state.h"
#include "ui/wndmgr/wndmgr.h"

#include "ui/chat/sub_chat_map.h"

#define SUB_CHAT_WNDS(self) (&(self)->sub_chat_wnds)

#define WM_MODAL_END_TEARDOWN 0xb4
#define WHISPER_WND_ID 0x43

static void destroy_window(struct UIWindow *wnd)
{
    ui_window_store_info(wnd);
    if (wnd)
        ui_window_destroy(wnd);
}

static void destroy_plates(struct UIWindowMgr *self)
{
    struct ListNode *node;

    for (node = self->plates.head->next; node != self->plates.head; node = node->next) {
        struct UIWindow **plate = LIST_VALUE(node, struct UIWindow *);

        ui_window_mgr_destroy_window(self, *plate);
        *plate = NULL;
    }
    list_clear(&self->plates, NULL);
}

void ui_window_mgr_close_all_windows(struct UIWindowMgr *self)
{
    struct ListNode *node;

    if (self->modal_wnd)
        self->modal_result = WM_MODAL_END_TEARDOWN;
    for (node = self->windows.head->next; node != self->windows.head; node = node->next) {
        struct UIWindow **wnd = LIST_VALUE(node, struct UIWindow *);

        if (self->modal_wnd && self->capture_root == *wnd) {
            self->close_capture_root = 1;
            continue;
        }
        destroy_window(*wnd);
        *wnd = NULL;
    }
    list_clear(&self->windows, NULL);
    list_clear(&self->quit_list, NULL);
    tree_clear(&self->snap_rects, NULL);
    self->message_boxes.last = self->message_boxes.first;
    ui_window_mgr_clear_slots(self);
    destroy_plates(self);
}

static int not_a_tab(struct UIWindowMgr *self, struct UIWindow *wnd)
{
    int i, other = 1;

    for (i = 0; other && (unsigned)i < self->sub_chat_wnds.size; i++) {
        struct SubChatWnd *sub = tree_get(SUB_CHAT_WNDS(self), &i, sizeof i, sizeof *sub, tree_less_int);

        other = wnd != sub->wnd;
    }
    return other;
}

void ui_window_mgr_close_all_but_chat(struct UIWindowMgr *self)
{
    struct ListNode *node, *next;
    struct TreeNode *it;
    struct Tree tabs;
    struct UIWindow *chat;

    if (self->modal_wnd)
        self->modal_result = WM_MODAL_END_TEARDOWN;
    if (self->new_chat_wnd == NULL) {
        ui_window_mgr_close_all_windows(self);
        return;
    }
    for (node = self->windows.head->next; node != self->windows.head; node = next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        next = node->next;
        if (!not_a_tab(self, wnd) || wnd == self->new_chat_wnd || wnd->id == WHISPER_WND_ID)
            continue;
        if (wnd == self->memorial_dun_wnd && g_session1.mdWndShown) {
            g_session1.mdWndX = wnd->x;
            g_session1.mdWndY = wnd->y;
        }
        if (self->modal_wnd && self->capture_root == wnd) {
            self->close_capture_root = 1;
            continue;
        }
        destroy_window(wnd);
        list_erase(&self->windows, node, NULL);
    }
    for (node = self->quit_list.head->next; node != self->quit_list.head; node = next) {
        struct UIWindow *wnd = *LIST_VALUE(node, struct UIWindow *);

        next = node->next;
        if (not_a_tab(self, wnd) && self->new_chat_wnd != wnd)
            list_erase(&self->quit_list, node, NULL);
    }
    for (it = tree_first(&self->snap_rects); it != self->snap_rects.head;) {
        if (TREE_VALUE(it, struct WmSnapRect)->wnd != self->new_chat_wnd)
            it = tree_erase(&self->snap_rects, it, NULL);
        else
            it = tree_next(it);
    }
    chat = self->new_chat_wnd;
    tree_copy(&tabs, SUB_CHAT_WNDS(self), sizeof(struct SubChatWnd), NULL);
    ui_window_mgr_clear_slots(self);
    self->new_chat_wnd = chat;
    tree_free(SUB_CHAT_WNDS(self), NULL);
    tree_copy(SUB_CHAT_WNDS(self), &tabs, sizeof(struct SubChatWnd), NULL);
    destroy_plates(self);
    tree_free(&tabs, NULL);
}

void ui_window_mgr_clear_slots(struct UIWindowMgr *self)
{
    int i;

    tree_clear(&self->windows_by_id, NULL);
    tree_clear(SUB_CHAT_WNDS(self), NULL);
    self->message_boxes.last = self->message_boxes.first;
    list_clear(&self->hidden_wnds, NULL);
    self->hide_ui_state = 0;

    self->capture = NULL;
    self->focus_edit = NULL;
    self->modal_wnd = NULL;
    self->mouse_wnd = NULL;
    self->loading_wnd = NULL;
    self->minimap_zoom_wnd = NULL;
    self->status_wnd = NULL;
    self->new_chat_wnd = NULL;
    self->login_wnd = NULL;
    self->item_wnd = NULL;
    self->orphan_char_wnd = NULL;
    self->basic_info_wnd = NULL;
    self->equip_wnd = NULL;
    self->short_cut_wnd = NULL;
    self->item_drop_cnt_wnd = NULL;
    self->say_dialog_wnd = NULL;
    self->choose_wnd = NULL;
    self->item_identify_wnd = NULL;
    self->item_repair_wnd = NULL;
    self->weapon_refine_wnd = NULL;
    self->item_composition_wnd = NULL;
    self->choose_warp_wnd = NULL;
    self->menu_wnd = NULL;
    self->combo_box_wnd = NULL;
    self->item_collection_wnd = NULL;
    self->combo_box_idwnd = NULL;
    self->item_collection_comparison_wnd = NULL;
    self->combined_card_item_collection_wnd = NULL;
    self->skill_describe_wnd = NULL;
    self->card_item_illust_wnd = NULL;
    self->notify_level_up_wnd = NULL;
    self->notify_job_level_up_wnd = NULL;
    self->notify_quest_wnd = NULL;
    self->item_shop_wnd = NULL;
    self->item_shop_wnd2 = NULL;
    self->item_purchase_wnd = NULL;
    self->item_sell_wnd = NULL;
    self->item_sell_wnd2 = NULL;
    self->choose_sell_buy_wnd = NULL;
    self->para_item_shop_wnd = NULL;
    self->para_item_purchase_wnd = NULL;
    self->chat_room_make_wnd = NULL;
    self->chat_room_change_wnd = NULL;
    self->chat_room_wnd = NULL;
    self->password_wnd = NULL;
    self->exchange_wnd = NULL;
    self->exchange_accept_wnd = NULL;
    self->item_store_wnd = NULL;
    for (i = 0; i < ITEM_STORE_SUB_WNDS; i++)
        self->item_store_sub_wnd[i] = NULL;
    self->item_store_find_wnd = NULL;
    self->new_skill_list_wnd = NULL;
    self->messenger_group_wnd = NULL;
    self->join_party_accept_wnd = NULL;
    self->couple_accept_wnd = NULL;
    self->star_place_accept_wnd = NULL;
    self->your_item_wnd = NULL;
    self->baby_accept_wnd = NULL;
    self->baby_accept_wnd2 = NULL;
    self->join_guild_accept_wnd = NULL;
    self->ally_guild_accept_wnd = NULL;
    self->merchant_item_wnd = NULL;
    self->merchant_mirror_item_wnd = NULL;
    self->merchant_shop_make_wnd = NULL;
    self->merchant_item_shop_wnd = NULL;
    self->merchant_item_my_shop_wnd = NULL;
    self->merchant_item_purchase_wnd = NULL;
    self->party_setting_wnd = NULL;
    self->skill_name_change_wnd = NULL;
    self->npc_edit_dialog_wnd = NULL;
    self->npc_text_edit_dialog_wnd = NULL;
    self->notify_item_obtain_wnd = NULL;
    self->guild_info_manage_wnd = NULL;
    self->guild_member_manage_wnd = NULL;
    self->guild_position_manage_wnd = NULL;
    self->guild_skill_wnd = NULL;
    self->guild_banished_member_wnd = NULL;
    self->guild_notice_wnd = NULL;
    self->guild_total_info_wnd = NULL;
    self->friend_option_wnd = NULL;
    self->guild_leave_reason_desc_wnd = NULL;
    self->guild_ban_reason_desc_wnd = NULL;
    self->illust_wnd = NULL;
    self->make_target_list_wnd = NULL;
    self->make_target_process_wnd = NULL;
    self->emotion_wnd = NULL;
    self->emotion_list_wnd = NULL;
    self->pet_info_wnd = NULL;
    self->pet_egg_list_wnd = NULL;
    self->pet_taming_deceive_wnd = NULL;
    self->making_arrow_list_wnd = NULL;
    self->select_cart_wnd = NULL;
    self->candidate_wnd = NULL;
    self->key_stroke_wnd = NULL;
    self->join_friend_accept_wnd = NULL;
    self->command_list_wnd = NULL;
    self->homun_info_wnd = NULL;
    self->homun_skill_list_wnd = NULL;
    self->mer_info_wnd = NULL;
    self->mer_skill_list_wnd = NULL;
    self->auto_message_wnd = NULL;
    self->new_select_char_wnd = NULL;
    self->sel_char_for_userver_wnd = NULL;
    self->battle_msg_option_wnd = NULL;
    self->memorial_dun_wnd = NULL;
    self->other_equip_wnd = NULL;
    self->kafra_move_map_service_info_wnd = NULL;
    self->hot_key_wnd = NULL;
    self->esc_option_wnd = NULL;
    self->buying_store_make_wnd = NULL;
    self->buying_store_mirror_item_wnd = NULL;
    self->buying_store_item_my_shop_wnd = NULL;
    self->buying_store_item_shop_wnd = NULL;
    self->buying_store_item_sell_wnd = NULL;
    self->buying_store_mirror_sell_list_wnd = NULL;
    self->quest_display_wnd = NULL;
    self->party_invitation_to_wnd = NULL;
    self->guild_helper_wnd = NULL;
    self->create_guild_wnd = NULL;
    self->orphan_guild_wnd = NULL;
    self->bank_new_wnd = NULL;
    self->guild_storage_log_wnd = NULL;
    self->open_mail_box_wnd = NULL;
    self->rodex_inbox_wnd = NULL;
    self->rodex_compose_wnd = NULL;
    self->rodex_contents_wnd = NULL;
    self->second_costume_wnd = NULL;
    self->navigation_v4_wnd = NULL;
    self->collection_system_wnd = NULL;
    self->mini_map_wnd = NULL;
    self->active_top_wnd = NULL;
    self->drag_plate = NULL;
    self->private_airplane_wnd = NULL;
    self->rpimage_wnd2 = NULL;
    self->bartermarket_wnd = NULL;
    self->barter_item_purchase_wnd = NULL;
    self->expanded_bartermarket_wnd = NULL;
    self->expanded_barter_item_purchase_wnd = NULL;
    self->register_party_wnd = NULL;
    self->adven_party_board_wnd = NULL;
    self->request_join_party_wnd = NULL;
    self->apply_for_party_wnd = NULL;
}
