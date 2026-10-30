#ifndef UI_CHARACTER_SKILL_LIST_WND_H
#define UI_CHARACTER_SKILL_LIST_WND_H

struct UIWindow;

void ui_skill_list_set_kind(struct UIWindow *wnd, int kind);

struct UISkillListWnd *ui_skill_list_wnd_construct(struct UISkillListWnd *self);
struct UIWindow *ui_skill_list_wnd_new(void);

extern int g_skillPointPending;

#endif
