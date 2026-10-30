#include "render/texture.h"
#include "render/texture_cache.h"
#include "iteminfo/item_api.h"
#include "navi/navi_manager.h"
#ifndef UI_QUESTS_H
#define UI_QUESTS_H

#include <string.h>
#include "contents/renew_quest.h"
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"

#include "ui/quests/tag_text_viewer.h"

#define QUEST_HUNT_MAX RENEW_QUEST_HUNT_MAX
#define QUEST_HUNT_ANY_MOB RENEW_QUEST_ANY_MOB

struct UINpcSprWnd;
struct UINpcSprWnd *ui_npc_spr_wnd_new(void);
void ui_npc_spr_wnd_create(struct UINpcSprWnd *self, int width, int height, struct String name);
void ui_npc_spr_wnd_set_target(struct UINpcSprWnd *self, struct String map, int x, int y);
struct UIOngoingQuestScrollWnd;
struct UIRecommededQuestScrollWnd;
struct UIOngoingQuestScrollWnd *ui_ongoing_quest_scroll_wnd_new(int width, int height);
void ui_ongoing_quest_scroll_wnd_set_quest(struct UIOngoingQuestScrollWnd *self, int quest_id);
struct UIRecommededQuestScrollWnd *ui_recommeded_quest_scroll_wnd_new(int width, int height);
void ui_recommeded_quest_scroll_wnd_set_quest(struct UIRecommededQuestScrollWnd *self, int quest_id);
struct UIScrollBar *ui_renew_quest_scroll_bar_construct(struct UIScrollBar *self);
struct UIRenewDetailQuestInfoWnd;
void ui_renew_detail_quest_info_wnd_show_quest(struct UIRenewDetailQuestInfoWnd *self,
                                                    int quest_id, BYTE recommended);
#define QUEST_DETAIL_WND_ID 0x142

int say_dialog_info_is_valid(struct WindowInfo *self);
void say_dialog_info_set_default(struct WindowInfo *self);

void ui_check_attendance_wnd_add_banner(struct UIWindow *window, struct String bitmap, struct String url,
                                        int window_id, int value);
struct UICheckAttendanceWnd *ui_check_attendance_wnd_construct(struct UICheckAttendanceWnd *self);
struct UIWindow *ui_check_attendance_wnd_new(void);

struct UIRPImageWnd2 *uirp_image_wnd2_construct(struct UIRPImageWnd2 *self);
struct UIWindow *uirp_image_wnd2_new(void);
void uirp_image_wnd2_set_image(struct UIRPImageWnd2 *self, struct String path, int draw_count);
struct UIRPImageWnd *uirp_image_wnd_new(const char *image, int draw_count);

struct UINavigationHelpWnd *ui_navigation_help_wnd_construct(struct UINavigationHelpWnd *self);
struct UIWindow *ui_navigation_help_wnd_new(void);
struct UINavigationRuideWnd *ui_navigation_ruide_wnd_construct(struct UINavigationRuideWnd *self);
struct UIWindow *ui_navigation_ruide_wnd_new(void);
struct UINavigationroadiconWnd *ui_navigationroadicon_wnd_construct(struct UINavigationroadiconWnd *self);
struct UIWindow *ui_navigationroadicon_wnd_new(void);

struct UINoteWnd *ui_note_wnd_construct(struct UINoteWnd *self);
struct UIWindow *ui_note_wnd_new(void);

struct UINotifyLevelUpWnd *ui_notify_level_up_wnd_new(void);
struct UIWindow *ui_notify_job_level_up_wnd_new(void);
struct UIWindow *ui_notify_quest_wnd_new(void);
struct UIOpenMailBoxWnd *ui_open_mail_box_wnd_construct(struct UIOpenMailBoxWnd *self);
struct UIWindow *ui_open_mail_box_wnd_new(void);

struct UIRenewDetailQuestInfoWnd *ui_renew_detail_quest_info_wnd_construct(struct UIRenewDetailQuestInfoWnd *self);
struct UIWindow *ui_renew_detail_quest_info_wnd_new(void);

struct UIQuestDisplay *ui_quest_display_construct(struct UIQuestDisplay *self);
struct UIWindow *ui_quest_display_new(void);
void ui_quest_display_create(struct UIQuestDisplay *self, int width, int height);

struct UIRenewQuestWnd *ui_renew_quest_wnd_construct(struct UIRenewQuestWnd *self);
struct UIWindow *ui_renew_quest_wnd_new(void);

struct SignBoardWnd;
void sign_board_wnd_apply_text_colour(struct SignBoardWnd *self);

#endif
