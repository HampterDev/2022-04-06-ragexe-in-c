#include "core/object_type_registry.h"
#include "core/object_type_tables.h"

/* The object types of src/ui/quests/: the quest, NPC and navigation windows and their
   controls. core/object_type.h describes the table. In the order of the class names. */
static const struct ObjectTypeRow k_rows[] = {
    {&ui_ach_list_wnd_ops, 0, "UIAchListWnd"},
    {&ui_ach_scroll_bar_ops, 0, "UIAchScrollBar"},
    {&ui_ach_summary_wnd_ops, 0, "UIAchSummaryWnd"},
    {&ui_ach_tv_ops, 0, "UIAchTV"},
    {&ui_ach_tab_ops, 0, "UIAchTab"},
    {&ui_ach_tracing_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIAchTracingWnd"},
    {&ui_ach_upper_wnd_ops, 0, "UIAchUpperWnd"},
    {&ui_ach_view_wnd_ops, 0, "UIAchViewWnd"},
    {&ui_achievement_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIAchievementWnd"},
    {&ui_check_attendance_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UICheckAttendanceWnd"},
    {&ui_choose_warp_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIChooseWarpWnd"},
    {&ui_choose_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIChooseWnd"},
    {&ui_custom_gage_bar_ops, 0, "UICustomGageBar"},
    {&ui_list_box_navi_search_ops, 0, "UIListBox_NaviSearch"},
    {&ui_minimap_zoom_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIMinimapZoomWnd"},
    {&ui_navigation_help_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UINavigationHelpWnd"},
    {&ui_navigation_ruide_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UINavigationRuideWnd"},
    {&ui_navigation_v4_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UINavigationV4Wnd"},
    {&ui_navigationroadicon_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UINavigationroadiconWnd"},
    {&ui_note_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UINoteWnd"},
    {&ui_note_wnd_scroll_ops, 0, "UINoteWnd_Scroll"},
    {&ui_notify_job_level_up_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UINotifyJobLevelUpWnd"},
    {&ui_notify_level_up_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UINotifyLevelUpWnd"},
    {&ui_notify_quest_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UINotifyQuestWnd"},
    {&ui_npc_edit_dialog_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UINpcEditDialogWnd"},
    {&ui_npc_spr_wnd_ops, 0, "UINpcSprWnd"},
    {&ui_npc_text_edit_dialog_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UINpcTextEditDialogWnd"},
    {&ui_ongoing_quest_scroll_wnd_ops, 0, "UIOngoingQuestScrollWnd"},
    {&ui_open_mail_box_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIOpenMailBoxWnd"},
    {&ui_private_airplane_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIPrivateAirplaneWnd"},
    {&ui_quest_display_ops, 0, "UIQuestDisplay"},
    {&uirp_image_wnd_ops, 0, "UIRPImageWnd"},
    {&uirp_image_wnd2_ops, OBJECT_TYPE(FRAME_WINDOW), "UIRPImageWnd2"},
    {&ui_renew_detail_quest_info_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIRenewDetailQuestInfoWnd"},
    {&ui_renew_quest_scroll_bar_ops, 0, "UIRenewQuestScrollBar"},
    {&ui_renew_quest_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UIRenewQuestWnd"},
    {&ui_say_dialog_wnd_ops, OBJECT_TYPE(FRAME_WINDOW), "UISayDialogWnd"},
    {&ui_tag_text_viewer_ops, 0, "UITagTextViewer"},
    {&ui_tool_tip_bitmap_wnd_ops, 0, "UIToolTipBitmapWnd"},
};

const struct ObjectTypeRows g_questsObjectTypes = OBJECT_TYPE_ROWS(k_rows);
