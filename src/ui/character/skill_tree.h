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
    struct UIBitmapButton *apply_button;
    struct UIBitmapButton *reset_button;
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
    int points_left;
    BYTE scrollable;
    BYTE dirty;
    BYTE pending;
    DWORD anim_time;
    struct List tab_skills[SKILL_TREE_TABS];
    struct List required;
    struct List planned;
    BYTE show_required;
    int required_cell;
    int anim_frame_counts[2];
    int anim_frames[2];
    unsigned long long drawn_key;
};
OFFSET_ASSERT(UINewSkillListWnd, up_buttons, 0xa8);
OFFSET_ASSERT(UINewSkillListWnd, raise_buttons, 0x178);
OFFSET_ASSERT(UINewSkillListWnd, apply_button, 0x220);
OFFSET_ASSERT(UINewSkillListWnd, info, 0x230);
OFFSET_ASSERT(UINewSkillListWnd, points_left, 0x250);
OFFSET_ASSERT(UINewSkillListWnd, tab_skills, 0x25c);
OFFSET_ASSERT(UINewSkillListWnd, required, 0x27c);
OFFSET_ASSERT(UINewSkillListWnd, required_cell, 0x290);
SIZE_ASSERT(UINewSkillListWnd, 0x2b0);

void ui_new_skill_list_wnd_on_draw(struct UIWindow *window);

struct UINewSkillListWnd *ui_new_skill_list_wnd_construct(struct UINewSkillListWnd *self);
struct UIWindow *ui_new_skill_list_wnd_new(void);

#endif
