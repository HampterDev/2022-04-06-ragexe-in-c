#ifndef EFFECTS_SKILLS_SKILL_STATE_H
#define EFFECTS_SKILLS_SKILL_STATE_H
#include "actor/types.h"
#include "effects/skills/skill_entry.h"

struct SkillCell {
    int x, y;
};

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
    struct Vec cells;
};

struct Skill *skill_construct(struct Skill *self, int unit_aid);
void skill_destruct(struct Skill *self);

#endif
