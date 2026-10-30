#include "navi/navi_manager.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#ifndef UI_WORLDMAP_NAVI_WND_H
#define UI_WORLDMAP_NAVI_WND_H

#include "ui/worldmap/romap.h"

struct UINavigationWnd {
    struct UIFrameWnd frame;
    struct UIWindow *close;
    struct UIWindow *mini;
    struct String title;
    struct String folded_text;
    struct UIWindow *mode_combo;
    struct UIWindow *edit;
    struct UIWindow *results;
    struct UIWindow *route;
    struct UIBitmapButton *buttons[6];
    struct UIWindow *check_kafra;
    struct UIWindow *check_scrolls;
    struct UIWindow *check_airship;
    struct UIBitmapButton *help;
    struct NavigationWndInfo *info;
    struct UIResizer *resizer;
    struct Vec files;
};
OFFSET_ASSERT(UINavigationWnd, title, 0x9c);

#define NAVI_ID_SEARCH 0x139
#define NAVI_ID_START 0xb0
#define NAVI_ID_ROUTE 0xb5
#define NAVI_ID_LIST 0xb6
#define NAVI_ID_WIDE 0x114
#define NAVI_ID_FOLDER 0xb7
#define NAVI_ID_FOLD 0xc2
#define NAVI_ID_CHECK 0xcd
#define NAVI_ID_HELP 0x14f
#define NAVI_ID_PICTURE 0x1a5
#define NAVI_ID_RESULTS 0x148
#define NAVI_ID_ROUTE_LIST 0x1b1
#define WND_NAVI_HELP 0xe5
#define NAVI_MSG_ADD_LINE 0x1f
#define NAVI_MSG_INFO 0x22
#define NAVI_MSG_SELECT 0x27

void ui_navigation_wnd_destruct(struct UINavigationWnd *self);
void ui_navigation_wnd_place(struct UINavigationWnd *self);
void ui_navigation_wnd_show_list(struct UINavigationWnd *self);
void ui_navigation_wnd_set_title(struct UINavigationWnd *self, int state);
void ui_navigation_wnd_fill_results(struct UINavigationWnd *self);
void ui_navigation_wnd_fill_route(struct UINavigationWnd *self);
BYTE ui_navigation_wnd_load_place_file(struct UINavigationWnd *self, int index);

#endif
