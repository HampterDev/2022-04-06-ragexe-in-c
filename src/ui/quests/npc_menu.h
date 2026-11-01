#ifndef UI_QUESTS_NPC_MENU_H
#define UI_QUESTS_NPC_MENU_H
#include "ui/window.h"

struct UIListBox;

struct UIChooseWnd {
    struct UIFrameWnd frame;
    int line_index;
    struct Vec line_values;
    struct UIListBox *list;
    int npc_id;
    void *info;
};
SIZE_ASSERT(UIChooseWnd, 0xb0);

struct UIChooseWarpWnd {
    struct UIFrameWnd frame;
    struct UIListBox *list;
    int skill_id;
    struct Vec destinations;
};
SIZE_ASSERT(UIChooseWarpWnd, 0xa8);

struct UIChooseWarpWnd *ui_choose_warp_wnd_construct(struct UIChooseWarpWnd *self);
struct UIWindow *ui_choose_warp_wnd_new(void);
struct UIChooseWnd *ui_choose_wnd_construct(struct UIChooseWnd *self);
struct UIWindow *ui_choose_wnd_new(void);

#endif
