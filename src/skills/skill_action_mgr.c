#include "skills/skill_action.h"

unsigned char skill_action_mgr_efst_on(struct SkillActionMgr *self, int efst, int value,
                                     SkillActor *actor)
{
    const struct StatusAction *action = skill_action_mgr_find_efst(self, efst);

    if (!action)
        return 0;
    if (action->on)
        action->on(efst, value, actor);
    return 1;
}

unsigned char skill_action_mgr_efst_off(struct SkillActionMgr *self, int efst, int value,
                                      SkillActor *actor)
{
    const struct StatusAction *action = skill_action_mgr_find_efst(self, efst);

    if (!action)
        return 0;
    if (action->off)
        action->off(efst, value, actor);
    return 1;
}

unsigned char skill_action_mgr_efst_reapply(struct SkillActionMgr *self, int efst, int value,
                                          SkillActor *actor)
{
    const struct StatusAction *action = skill_action_mgr_find_efst(self, efst);

    if (!action)
        return 0;
    if (action->reapply)
        action->reapply(efst, value, actor);
    return 1;
}
