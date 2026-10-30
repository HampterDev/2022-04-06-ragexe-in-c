#include "actor/object_ops.h"
#include "effects/skills/skill_entry.h"
#ifndef EFFECTS_SKILLS_SKILL_H
#define EFFECTS_SKILLS_SKILL_H

#include "effects/render_object.h"

#include "effects/skills/skill_state.h"

const char *skill_trap_model(struct Skill *self);
void skill_load_model(struct Skill *self, const char *name);

struct GraffiSkill *graffi_skill_new(void);

#endif
