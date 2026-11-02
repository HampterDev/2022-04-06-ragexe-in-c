#ifndef SKILLS_SKILL_ACTION_H
#define SKILLS_SKILL_ACTION_H

#include "render/vec3.h"
#include "skills/skill_answer.h"
#include "skills/skill_action_registry.h"

typedef struct RenderObject SkillActor;

struct StatusAction {
    void (*on)(int status, int value, SkillActor *actor);
    void (*off)(int status, int value, SkillActor *actor);
    void (*reapply)(int status, int value, SkillActor *actor);
};

unsigned char skill_action_mgr_efst_on(struct SkillActionMgr *self, int efst, int value,
                                     SkillActor *actor);
unsigned char skill_action_mgr_efst_off(struct SkillActionMgr *self, int efst, int value,
                                      SkillActor *actor);
unsigned char skill_action_mgr_efst_reapply(struct SkillActionMgr *self, int efst, int value,
                                          SkillActor *actor);

void skill_actions_register(void);

#endif
