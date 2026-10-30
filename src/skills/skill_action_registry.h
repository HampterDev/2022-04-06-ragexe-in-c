#ifndef SKILLS_SKILL_ACTION_REGISTRY_H
#define SKILLS_SKILL_ACTION_REGISTRY_H

struct SkillActionMgr;
struct SkillAction;
struct StatusAction;
struct SkillObjectAction;

struct SkillActionMgr *skill_action_mgr_instance(void);
void skill_action_mgr_destroy_instance(void);
void skill_action_mgr_add_skill(struct SkillActionMgr *self, int id,
                              const struct SkillAction *action);
void skill_action_mgr_add_efst(struct SkillActionMgr *self, int id,
                             const struct StatusAction *action);
void skill_action_mgr_add_object_action(struct SkillActionMgr *self, int id,
                                        const struct SkillObjectAction *action);
const struct SkillAction *skill_action_mgr_find_skill(struct SkillActionMgr *self, int id);
const struct StatusAction *skill_action_mgr_find_efst(struct SkillActionMgr *self, int id);
const struct SkillObjectAction *skill_action_mgr_find_object(struct SkillActionMgr *self, int id);
unsigned char skill_action_mgr_has_efst_action(struct SkillActionMgr *self, int id);

#endif
