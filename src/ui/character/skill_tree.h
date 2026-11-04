#include "core/thread_text.h"
#include "game/drag.h"
#include "session/option_pairs.h"
#include "resources/animation.h"
#include "skills/skill_bundle.h"
#include "skills/skill_info.h"
#include "ui/window_messages.h"
#include "session/state.h"
#ifndef UI_SKILL_TREE_H
#define UI_SKILL_TREE_H

#include "ui/character/character.h"
#include "std/list.h"

#define SKILL_INFO_UPGRADABLE(info) ((info)->upgradable)
#define SKILL_INFO_ID_NAME(info) ((info)->id_name)
#define SKILL_INFO_SLOT(info) ((info)->cell_index)
#define SKILL_INFO_MAX_LEVEL(info) ((info)->max_level)
#define SKILL_INFO_LEVEL_LIMIT(info) ((info)->level_limit)
#define SKILL_INFO_BASE_LEVEL(info) ((short)(info)->level2)
#define SKILL_INFO_REQUIREMENTS(info) (&(info)->requirements)

int skill_list_info_is_valid(struct WindowInfo *self);
void skill_list_info_set_default(struct WindowInfo *self);

extern int g_skillWndHeadDx, g_skillWndHeadDy;

#define SKILL_TREE_TABS 4
#define SKILL_TREE_CELLS 42
#define SKILL_TREE_UP_BUTTONS 10

struct UINewSkillListWnd {
    struct UIFrameWnd frame;
    struct UITabControl *tabs;
    struct UIScrollBar *scroll;
    struct UIWindow *sys_button;
    struct UIWindow *mini_button;
    struct UIWindow *close_button;
    struct UIBitmapButton *up_buttons[SKILL_TREE_UP_BUTTONS];
    struct UIBitmapButton *lower_buttons[SKILL_TREE_CELLS];
    struct UIBitmapButton *raise_buttons[SKILL_TREE_CELLS];
    struct UIWindow *info_check;
    struct UIResizer *resizer;
    struct WindowInfo *info;
    int hover;
    int tab;
    int top;
    int columns;
    int rows;
    int top_margin;
    int bottom_margin;
    BYTE scrollable;
    BYTE dirty;
    DWORD anim_time;
    struct List required;
    BYTE show_required;
    int required_cell;
    int anim_frame_counts[2];
    int anim_frames[2];
    unsigned long long drawn_key;
};

void ui_new_skill_list_wnd_on_draw(struct UIWindow *window);

struct UINewSkillListWnd *ui_new_skill_list_wnd_construct(struct UINewSkillListWnd *self);
struct UIWindow *ui_new_skill_list_wnd_new(void);

#endif
