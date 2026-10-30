#ifndef UI_CHARACTER_SKILL_DIALOGS_H
#define UI_CHARACTER_SKILL_DIALOGS_H

struct UIWindow;

int ui_skill_describe_skill(const struct UIWindow *describe);

struct UISkillDescribeWnd *ui_skill_describe_wnd_construct(struct UISkillDescribeWnd *self);
struct UIWindow *ui_skill_describe_wnd_new(void);
struct UISkillNameChangeWnd *ui_skill_name_change_wnd_construct(struct UISkillNameChangeWnd *self);
struct UIWindow *ui_skill_name_change_wnd_new(void);

#endif
