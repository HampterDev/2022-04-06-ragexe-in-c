#include "core/object_type_registry.h"
#include "core/object_type_tables.h"

/* The object types of src/ui/social/: the guild, party and friend windows and their controls.
   core/object_type.h describes the table. In the order of the class names. */
static const struct ObjectTypeRow k_rows[] = {
    {&ui_adven_party_board_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIAdvenPartyBoardWnd"},
    {&ui_agit_message_box_ops, OBJECT_TYPE(FRAME_WINDOW), "UIAgitMessageBox"},
    {&ui_ally_guild_accept_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIAllyGuildAcceptWnd"},
    {&ui_apply_for_party_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIApplyForPartyWnd"},
    {&ui_baby_accept_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIBabyAcceptWnd"},
    {&ui_baby_accept_wnd2_ops, OBJECT_TYPE(FRAME_WINDOW), "UIBabyAcceptWnd2"},
    {&ui_choose_sell_buy_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIChooseSellBuyWnd"},
    {&ui_couple_accept_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UICoupleAcceptWnd"},
    {&ui_create_guild_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UICreateGuildWnd"},
    {&ui_drag_mini_party_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIDragMiniPartyWnd"},
    {&ui_emblem_wnd_ops, 0, "UIEmblemWnd"},
    {&ui_exchange_accept_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIExchangeAcceptWnd"},
    {&ui_guild_ban_reason_desc_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGuildBanReasonDescWnd"},
    {&ui_guild_banished_member_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGuildBanishedMemberWnd"},
    {&ui_guild_helper_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGuildHelperWnd"},
    {&ui_guild_info_manage_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGuildInfoManageWnd"},
    {&ui_guild_leave_reason_desc_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGuildLeaveReasonDescWnd"},
    {&ui_guild_member_manage_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGuildMemberManageWnd"},
    {&ui_guild_position_manage_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGuildPositionManageWnd"},
    {&ui_guild_skill_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGuildSkillWnd"},
    {&ui_guild_total_info_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGuildTotalInfoWnd"},
    {&ui_guild_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGuildWnd"},
    {&ui_guild_storage_log_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGuild_Storage_Log"},
    {&ui_job_slot_button_ops, 0, "UIJobSlotButton"},
    {&ui_join_guild_accept_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIJoinGuildAcceptWnd"},
    {&ui_join_party_accept_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIJoinPartyAcceptWnd"},
    {&ui_member_list_box_ops, 0, "UIMemberListBox"},
    {&ui_memorial_dun_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMemorialDunWnd"},
    {&ui_messenger_group_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMessengerGroupWnd"},
    {&ui_mini_party_info_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMiniPartyInfoWnd"},
    {&ui_mini_party_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMiniPartyWnd"},
    {&ui_party_info_wnd_ops, 0, "UIPartyInfoWnd"},
    {&ui_party_invitation_to_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIPartyInvitationToWnd"},
    {&ui_party_setting_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIPartySettingWnd"},
    {&ui_register_party_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIRegisterPartyWnd"},
    {&ui_request_join_party_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIRequestJoinPartyWnd"},
    {&ui_star_place_accept_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIStarPlaceAcceptWnd"},
};

const struct ObjectTypeRows g_socialObjectTypes = OBJECT_TYPE_ROWS(k_rows);
