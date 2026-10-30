#ifndef UI_QUESTS_ACHIEVEMENT_INTERNAL_H
#define UI_QUESTS_ACHIEVEMENT_INTERNAL_H

#include "ui/quests/achievement.h"

struct UIWindow;

#define ACH_DIR UI_SKIN "achievement_re\\"
#define ACH_TEXT_COLOUR 0xa55239
#define ACH_DATE_COLOUR 0x635aa5
#define ACH_DONE_COLOUR 0xc9977d

#define ACH_MSG_SELECT_TAB 0x16
#define ACH_MSG_REFRESH 0x3c
#define ACH_MSG_TAB_DRAWN 0x74
#define ACH_MSG_SHOW 0x87
#define ACH_MSG_GO_TO 0x91
#define ACH_ID_RECEIVE 0x1d2
#define ACH_TRACING_WND_ID 0x10f
#define PACKET_CZ_REQ_ACH_REWARD 0xa25
#define ITEM_INFO_WND_ID 0xc
#define UI_MSG_SHOW_ITEM 0x18

void ach_draw(struct UIWindow *wnd, int x, int y, const char *path, int keep_alpha);
BYTE ach_has_reward(const struct AchievementInfo *info);
struct UIAchScrollBar *ach_new_scroll_bar(void);
struct UICustomGageBar *ach_new_gauge(int width, int height);
struct UIAchTV *ach_new_tv(int nothrow);

struct SessionAchievementState *ach_state(struct SessionAchievementState *out, int id);

#endif
