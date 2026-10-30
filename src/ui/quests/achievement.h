#include "contents/achievement.h"
#include "iteminfo/item_api.h"
#include "session/state.h"
#include "session/session_achievements.h"
#include "contents/game_session.h"
#ifndef UI_QUESTS_ACHIEVEMENT_H
#define UI_QUESTS_ACHIEVEMENT_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/controls/controls.h"
#include "ui/ui_items.h"

#define ACHIEVEMENT_WND_ID 0x10e
#define ACH_TRACING_WND_ID 0x10f

struct AchPair {
    int id;
    struct AchievementInfo info;
};

void ach_pairs_clear(struct Vec *v);
void ach_pairs_free(struct Vec *v);
struct AchPair *ach_pair_copy_construct(struct AchPair *dest, const int *id, const struct AchievementInfo *info);
void ach_sort_ids(int *first, int *last);

#define ACH_MSG_SELECT_TAB 0x16
#define ACH_MSG_REFRESH 0x3c
#define ACH_MSG_TAB_DRAWN 0x74
#define ACH_MSG_SHOW 0x87
#define ACH_MSG_GO_TO 0x91
#define ACH_ID_RECEIVE 0x1d2
#define PACKET_CZ_REQ_ACH_REWARD 0xa25

struct UIAchScrollBar {
    struct UIScrollBar bar;
};

struct UICustomGageBar {
    struct UIWindow base;
    int value;
    int max;
    struct String label;
    COLORREF bar_colour;
    COLORREF back_colour;
    COLORREF outer_colour;
    COLORREF inner_colour;
};
OFFSET_ASSERT(UICustomGageBar, label, 0x84);

struct UIToolTipBitmapWnd {
    struct UIWindow base;
    struct String tooltip;
    struct BitmapRes *bitmap;
    int item_id;
    int item_x, item_y;
};

struct UIAchTV {
    struct UITextViewer viewer;
    BYTE no_click;
    COLORREF colour;
    COLORREF outline;
    BYTE underline;
};

struct UIAchListWnd {
    struct UIWindow base;
    struct UIAchScrollBar *scroll;
    struct UIButton **rows;
    int row_height;
    int row_count;
    int top;
    int selected;
    int scroll_width;
    BYTE scrollable;
    int hover_row;
    struct Vec ids;
    struct Tree *achs;
};
OFFSET_ASSERT(UIAchListWnd, ids, 0xa0);
SIZE_ASSERT(UIAchListWnd, 0xb0);

struct UIAchTab {
    struct UIWindow base;
    int sel_main, sel_sub;
    int tab_width;
    int tab_height;
    int sub_height;
    int font_height;
    int hover_main, hover_sub;
    COLORREF sel_colour;
    COLORREF colour;
    COLORREF sel_sub_colour;
    COLORREF sub_colour;
    BYTE flag_ac;
    struct Tree tabs;
    struct UIAchScrollBar *scroll;
    int top;
    int scroll_x;
    int visible;
    BYTE scrollable;
};
OFFSET_ASSERT(UIAchTab, tabs, 0xb0);
SIZE_ASSERT(UIAchTab, 0xcc);

struct UIAchSummaryWnd {
    struct UIWindow base;
    int gauge_count;
    struct Vec gauges;
    struct Vec recent;
    struct UITextButton *recent_buttons[2];
};
SIZE_ASSERT(UIAchSummaryWnd, 0xa0);

struct UIAchUpperWnd {
    struct UIWindow base;
    struct String caption;
    int points, points_max;
    int count, count_max;
    struct UICustomGageBar *points_gauge;
    struct UICustomGageBar *count_gauge;
    struct UIRadioBtn *radios[2];
    int order;
};

struct UIAchViewWnd {
    struct UIWindow base;
    struct UIBitmapButton *receive;
    struct UIAchTV *text;
    struct UIAchTV *reqs[10];
    struct UICustomGageBar *gauges[10];
    struct UIToolTipBitmapWnd *rewards[3];
    int id;
    struct AchievementInfo *info;
};
SIZE_ASSERT(UIAchViewWnd, 0xe8);

struct UIAchievementWnd {
    struct UIFrameWnd frame;
    struct UIAchUpperWnd *upper;
    struct UIAchTab *tab;
    struct UIAchListWnd *list;
    struct UIAchViewWnd *view;
    struct UIAchSummaryWnd *summary;
    int main, sub;
    struct Tree achs;
};
SIZE_ASSERT(UIAchievementWnd, 0xb8);

struct UIAchTracingWnd {
    struct UIFrameWnd frame;
    POINT *info;
    struct UITextButton *lines[3];
    struct UITextButton *arrows[3];
};
SIZE_ASSERT(UIAchTracingWnd, 0xb0);


void ui_ach_list_wnd_init(struct UIAchListWnd *self, int width, int height, int rows);
void ui_ach_list_wnd_fill(struct UIAchListWnd *self, struct Tree *achs, int order);
void ui_ach_tab_init(struct UIAchTab *self, int tab_width, int tab_height,
                              int sub_height, BYTE flag, COLORREF sel_colour, COLORREF colour,
                              COLORREF sel_sub_colour, COLORREF sub_colour);
void ui_ach_tab_set_name(struct UIAchTab *self, const char *name, int main, int sub);
struct String *ui_ach_tab_get_name(struct UIAchTab *self, struct String *out,
                                              int main, int sub);
void ui_ach_summary_wnd_refresh(struct UIAchSummaryWnd *self);
void ui_ach_summary_wnd_set_show(struct UIWindow *window, int visible);
void ui_ach_view_wnd_refresh(struct UIAchViewWnd *self);
void ui_ach_upper_wnd_set_count(struct UIAchUpperWnd *self, int count, int max);
void ui_ach_upper_wnd_set_points(struct UIAchUpperWnd *self, int points, int max);
void ui_tool_tip_bitmap_wnd_set(struct UIToolTipBitmapWnd *self, const char *path,
                                       const char *tooltip, int item_id);
struct UIAchListWnd *ui_ach_list_wnd_construct(struct UIAchListWnd *self);
struct UIAchScrollBar *ui_ach_scroll_bar_construct(struct UIAchScrollBar *self);
struct UIAchSummaryWnd *ui_ach_summary_wnd_construct(struct UIAchSummaryWnd *self);
struct UIAchTV *ui_ach_tv_construct(struct UIAchTV *self);
struct UIAchTab *ui_ach_tab_construct(struct UIAchTab *self);
struct UIAchUpperWnd *ui_ach_upper_wnd_construct(struct UIAchUpperWnd *self);
struct UIAchViewWnd *ui_ach_view_wnd_construct(struct UIAchViewWnd *self);
struct UICustomGageBar *ui_custom_gage_bar_construct(struct UICustomGageBar *self);
struct UIToolTipBitmapWnd *ui_tool_tip_bitmap_wnd_construct(struct UIToolTipBitmapWnd *self);
void ui_achievement_wnd_go_to(struct UIAchievementWnd *self, int id);
void ui_ach_list_wnd_select_id(struct UIAchListWnd *self, int id);

struct UIAchTracingWnd *ui_ach_tracing_wnd_construct(struct UIAchTracingWnd *self);
struct UIWindow *ui_ach_tracing_wnd_new(void);
struct UIAchievementWnd *ui_achievement_wnd_construct(struct UIAchievementWnd *self);
struct UIWindow *ui_achievement_wnd_new(void);

#endif
