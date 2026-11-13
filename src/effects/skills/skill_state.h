#ifndef EFFECTS_SKILLS_SKILL_STATE_H
#define EFFECTS_SKILLS_SKILL_STATE_H
#include "actor/types.h"
#include "effects/skills/skill_entry.h"

/* custom: unit_node is the unit's node on its world's skill units (struct World, skills, of struct
   SkillEntry; world/aid_index.h), NULL while it is not on it; unit_aid changes only through
   world_set_skill_aid once the unit may be on it. The original's vector of cells after `range`
   (0x410) was never filled, only walked in its release (CSkill::Release, 0x89bdc0), and is gone. */
struct Skill {
    struct GameActor base;
    int creator_aid;
    unsigned frames;
    int unit_aid;
    struct C3dActor *model;
    struct RagEffect *loop_effect;
    int effect_id;
    unsigned char launched;
    int range;
    struct ListNode *unit_node;
};

struct Skill *skill_construct(struct Skill *self, int unit_aid);
void skill_destruct(struct Skill *self);

#endif
