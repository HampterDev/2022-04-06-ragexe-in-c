#include "ui/window_manager_state.h"
#include "actor/liveness.h"
#include <stddef.h>
#include "ui/window_mgr.h"

#define SLOT(field) offsetof(struct UIWindowMgr, field)

static const unsigned short window_slots[] = {
    SLOT(loading_wnd), SLOT(minimap_zoom_wnd), SLOT(status_wnd), SLOT(new_chat_wnd), SLOT(login_wnd),
    SLOT(login_otpwnd), SLOT(item_wnd), SLOT(basic_info_wnd), SLOT(equip_wnd), SLOT(n_cash_wnd),
    SLOT(short_cut_wnd), SLOT(item_drop_cnt_wnd), SLOT(say_dialog_wnd), SLOT(choose_wnd),
    SLOT(item_identify_wnd), SLOT(item_repair_wnd), SLOT(weapon_refine_wnd),
    SLOT(item_composition_wnd), SLOT(choose_warp_wnd), SLOT(menu_wnd), SLOT(combo_box_wnd),
    SLOT(item_collection_wnd), SLOT(combo_box_idwnd), SLOT(item_collection_comparison_wnd),
    SLOT(combined_card_item_collection_comparison_wnd), SLOT(combined_card_item_collection_wnd),
    SLOT(item_param_change_display_wnd), SLOT(skill_describe_wnd), SLOT(quit_wnd),
    SLOT(restart_wnd), SLOT(card_item_illust_wnd), SLOT(notify_level_up_wnd), SLOT(notify_job_level_up_wnd),
    SLOT(notify_quest_wnd), SLOT(n_cash_call_wnd), SLOT(item_shop_wnd), SLOT(item_shop_wnd2),
    SLOT(item_purchase_wnd), SLOT(item_sell_wnd), SLOT(item_sell_wnd2), SLOT(choose_sell_buy_wnd),
    SLOT(para_item_shop_wnd), SLOT(para_item_purchase_wnd), SLOT(para_result_wnd),
    SLOT(chat_room_make_wnd), SLOT(chat_room_change_wnd), SLOT(chat_room_wnd), SLOT(password_wnd),
    SLOT(exchange_wnd), SLOT(exchange_accept_wnd), SLOT(item_store_wnd), SLOT(item_store_find_wnd),
    SLOT(seek_party_wnd), SLOT(seek_party_mbwnd), SLOT(job_list_wnd), SLOT(seek_party_list_wnd),
    SLOT(party_booking_help_wnd), SLOT(new_skill_list_wnd), SLOT(messenger_group_wnd),
    SLOT(join_party_accept_wnd), SLOT(couple_accept_wnd), SLOT(star_place_accept_wnd),
    SLOT(mix_accept_wnd), SLOT(your_item_wnd), SLOT(baby_accept_wnd), SLOT(baby_accept_wnd2),
    SLOT(join_guild_accept_wnd), SLOT(ally_guild_accept_wnd), SLOT(tip_of_the_day_wnd),
    SLOT(merchant_item_wnd), SLOT(merchant_mirror_item_wnd), SLOT(merchant_shop_make_wnd),
    SLOT(merchant_item_shop_wnd), SLOT(merchant_item_my_shop_wnd),
    SLOT(merchant_item_purchase_wnd), SLOT(party_setting_wnd), SLOT(skill_name_change_wnd),
    SLOT(npc_edit_dialog_wnd), SLOT(npc_text_edit_dialog_wnd), SLOT(notify_item_obtain_wnd),
    SLOT(guild_info_manage_wnd), SLOT(guild_member_manage_wnd), SLOT(guild_position_manage_wnd),
    SLOT(guild_skill_wnd), SLOT(guild_banished_member_wnd), SLOT(guild_notice_wnd),
    SLOT(guild_total_info_wnd), SLOT(friend_option_wnd), SLOT(guild_leave_reason_desc_wnd),
    SLOT(guild_ban_reason_desc_wnd), SLOT(illust_wnd), SLOT(make_target_list_wnd),
    SLOT(make_target_process_wnd), SLOT(make_target_result_wnd), SLOT(emotion_wnd),
    SLOT(emotion_list_wnd), SLOT(prohibit_list_wnd), SLOT(pet_info_wnd), SLOT(pet_egg_list_wnd),
    SLOT(pet_taming_deceive_wnd), SLOT(making_arrow_list_wnd), SLOT(select_cart_wnd),
    SLOT(spell_list_wnd), SLOT(candidate_wnd), SLOT(composition_wnd), SLOT(key_stroke_wnd),
    SLOT(book_wnd), SLOT(join_friend_accept_wnd), SLOT(command_list_wnd),
    SLOT(second_password_input_wnd), SLOT(second_password_edit_wnd), SLOT(homun_info_wnd),
    SLOT(homun_skill_list_wnd), SLOT(mer_info_wnd), SLOT(mer_skill_list_wnd), SLOT(auto_message_wnd),
    SLOT(new_select_char_wnd), SLOT(sel_char_for_userver_wnd), SLOT(change_name_wnd),
    SLOT(rebirth_wnd), SLOT(sub_chat_st_wnd), SLOT(sub_chat_bt_wnd),
    SLOT(battle_msg_option_wnd), SLOT(sub_chat_mini_wnd), SLOT(memorial_dun_wnd),
    SLOT(other_equip_wnd), SLOT(kafra_move_map_service_info_wnd), SLOT(esc_option_wnd),
    SLOT(buying_store_make_wnd), SLOT(buying_store_mirror_item_wnd),
    SLOT(buying_store_item_my_shop_wnd), SLOT(buying_store_item_shop_wnd),
    SLOT(buying_store_item_sell_wnd), SLOT(buying_store_mirror_sell_list_wnd),
    SLOT(monster_display_wnd), SLOT(party_invitation_to_wnd), SLOT(guild_helper_wnd),
    SLOT(guild_tip_wnd), SLOT(create_guild_wnd), SLOT(disband_guild_wnd),
    SLOT(clan_info_manage_wnd), SLOT(bank_new_wnd), SLOT(guild_storage_log_wnd),
    SLOT(open_mail_box_wnd), SLOT(rodex_inbox_wnd), SLOT(rodex_compose_wnd),
    SLOT(rodex_contents_wnd), SLOT(open_roullette_wnd), SLOT(roullette_wnd), SLOT(tipbox_wnd),
    SLOT(replay_reccontrol_wnd), SLOT(replay_control_wnd), SLOT(second_costume_wnd),
    SLOT(navigation_v4_wnd), SLOT(collection_system_wnd), SLOT(mini_map_wnd), SLOT(editctrl_wnd),
    SLOT(round_box_tooltip_wnd), SLOT(captcha_register_wnd), SLOT(macro_detector_wnd),
    SLOT(macro_reporter_wnd), SLOT(macro_user_info_wnd), SLOT(captcha_preview_wnd),
    SLOT(cash_shop_wnd), SLOT(rpimage_wnd2), SLOT(bartermarket_wnd), SLOT(barter_item_purchase_wnd),
    SLOT(expanded_bartermarket_wnd), SLOT(expanded_barter_item_purchase_wnd),
    SLOT(register_party_wnd), SLOT(adven_party_board_wnd), SLOT(request_join_party_wnd),
    SLOT(apply_for_party_wnd),
};

static void forget(struct UIWindow **ref, const struct UIWindow *wnd)
{
    if (*ref == wnd)
        *ref = NULL;
}

void ui_window_mgr_forget_window(struct UIWindowMgr *self, const struct UIWindow *wnd)
{
    size_t i;

    for (i = 0; i < sizeof window_slots / sizeof window_slots[0]; i++)
        forget((struct UIWindow **)((BYTE *)self + window_slots[i]), wnd);
    for (i = 0; i < ITEM_STORE_SUB_WNDS; i++)
        forget(&self->item_store_sub_wnd[i], wnd);
    forget(&self->capture_root, wnd);
    forget(&self->capture, wnd);
    forget(&self->focus_edit, wnd);
    forget(&self->modal_wnd, wnd);
    forget(&self->mouse_wnd, wnd);
    forget(&self->replay_wnd, wnd);
    forget(&self->active_top_wnd, wnd);
    actors_forget_window(wnd);
}
