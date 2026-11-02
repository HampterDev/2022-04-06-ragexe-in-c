#include "skills/skill_info.h"
#ifndef UI_SKILL_TREE_INTERNAL_H
#define UI_SKILL_TREE_INTERNAL_H

#include "ui/character/skill_tree.h"

#define WND(self) (&(self)->frame.base)

#define BASIC_INTERFACE UI_SKIN "basic_interface\\"

#define CELL_W 0x44
#define CELL_H 0x38
#define ROW_H 0x20

int skill_tree_tab_count(const struct UINewSkillListWnd *self);
struct List *skill_tree_shown_list(struct UINewSkillListWnd *self);
struct SkillRequirement *skill_tree_level_list_find(struct List *list, int skill_id);
BYTE skill_tree_is_last_tab(struct UINewSkillListWnd *self, int tab);
struct SkillInfo *skill_tree_skill_at(struct UINewSkillListWnd *self, struct SkillInfo *out, int cell);
void skill_tree_layout_buttons(struct UINewSkillListWnd *self);
BYTE skill_tree_raise_skill(struct UINewSkillListWnd *self, int skill_id);
void skill_tree_mark_required(struct UINewSkillListWnd *self);
BYTE ui_new_skill_list_wnd_render(struct UIWindow *window, int force);

#endif
