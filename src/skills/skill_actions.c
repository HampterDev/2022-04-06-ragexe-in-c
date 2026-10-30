#include "skills/skill_action.h"
#include "skills/skill_action_registry.h"

#define EFST_ACTION(id, cls) extern const struct StatusAction cls##_ops;
#define SKILL_ACTION(id, cls) extern const struct SkillAction cls##_ops;
#define SKILL_OBJECT_ACTION(id, cls) extern const struct SkillObjectAction cls##_ops;
#include "skills/skill_actions.def"
#undef EFST_ACTION
#undef SKILL_ACTION
#undef SKILL_OBJECT_ACTION

void skill_actions_register(void)
{
    struct SkillActionMgr *mgr = skill_action_mgr_instance();

#define EFST_ACTION(id, cls) skill_action_mgr_add_efst(mgr, id, &cls##_ops);
#define SKILL_ACTION(id, cls) skill_action_mgr_add_skill(mgr, id, &cls##_ops);
#define SKILL_OBJECT_ACTION(id, cls) skill_action_mgr_add_object_action(mgr, id, &cls##_ops);
#include "skills/skill_actions.def"
#undef EFST_ACTION
#undef SKILL_ACTION
#undef SKILL_OBJECT_ACTION
}
