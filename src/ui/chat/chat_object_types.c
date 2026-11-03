#include "core/object_type_registry.h"
#include "core/object_type_tables.h"

/* The object types of src/ui/chat/: the chat windows and their controls. core/object_type.h
   describes the table. In the order of the class names. */
static const struct ObjectTypeRow k_rows[] = {
    {&ui_ach_balloon_text_ops, 0, "UIAchBalloonText"},
    {&ui_auto_message_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIAutoMessageWnd"},
    {&ui_balloon_text_ops, 0, "UIBalloonText"},
    {&ui_battle_msg_option_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIBattleMsgOptionWnd"},
    {&ui_char_info_balloon_text_ops, 0, "UICharInfoBalloonText"},
    {&ui_chat_edit_ctrl_ops, 0, "UIChatEditCtrl"},
    {&ui_chat_his_box_ops, 0, "UIChatHisBox"},
    {&ui_chat_his_box2_ops, 0, "UIChatHisBox2"},
    {&ui_chat_room_change_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIChatRoomChangeWnd"},
    {&ui_chat_room_make_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIChatRoomMakeWnd"},
    {&ui_chat_room_title_ops, 0, "UIChatRoomTitle"},
    {&ui_chat_room_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIChatRoomWnd"},
    {&ui_command_list_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UICommandListWnd"},
    {&ui_emotion_list_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIEmotionListWnd"},
    {&ui_emotion_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIEmotionWnd"},
    {&ui_friend_option_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIFriendOptionWnd"},
    {&ui_graffi_strbox_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGraffiStrboxWnd"},
    {&ui_guild_notice_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIGuildNoticeWnd"},
    {&ui_join_friend_accept_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIJoinFriendAcceptWnd"},
    {&ui_name_balloon_text_ops, 0, "UINameBalloonText"},
    {&ui_new_chat_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UINewChatWnd"},
    {&ui_prohibit_list_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIProhibitListWnd"},
    {&ui_sub_chat_his_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UISubChatHisWnd"},
    {&ui_talkbox_trap_input_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UITalkboxTrapInputWnd"},
    {&ui_trans_balloon_text_ops, 0, "UITransBalloonText"},
    {&ui_vertical_name_balloon_text_ops, 0, "UIVerticalNameBalloonText"},
    {&ui_whisper_edit_ctrl_ops, 0, "UIWhisperEditCtrl"},
    {&ui_whisper_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIWhisperWnd"},
};

const struct ObjectTypeRows g_chatObjectTypes = OBJECT_TYPE_ROWS(k_rows);
