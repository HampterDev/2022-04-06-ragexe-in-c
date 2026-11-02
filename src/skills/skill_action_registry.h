#ifndef SKILLS_SKILL_ACTION_REGISTRY_H
#define SKILLS_SKILL_ACTION_REGISTRY_H

struct SkillActionMgr;
struct StatusAction;

struct SkillActionMgr *skill_action_mgr_instance(void);
void skill_action_mgr_destroy_instance(void);
void skill_action_mgr_add_efst(struct SkillActionMgr *self, int id,
                             const struct StatusAction *action);
const struct StatusAction *skill_action_mgr_find_efst(struct SkillActionMgr *self, int id);
unsigned char skill_action_mgr_has_efst_action(struct SkillActionMgr *self, int id);

#endif
