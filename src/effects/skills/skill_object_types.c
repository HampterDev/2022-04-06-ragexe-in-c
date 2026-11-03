#include "core/object_type_registry.h"
#include "core/object_type_tables.h"

/* The object types of src/effects/skills/: the skill and effect actor classes.
   core/object_type.h describes the table. In the order of the class names. */
static const struct ObjectTypeRow k_rows[] = {
    {&graffi_skill_ops, OBJECT_TYPE(GAME_ACTOR), "CGraffiSkill"},
    {&skill_ops, OBJECT_TYPE(GAME_ACTOR), "CSkill"},
    {&tetra_ball_effect_ops, OBJECT_TYPE(TETRA_BALL_EFFECT), "CTetraBallEffect"},
};

const struct ObjectTypeRows g_skillObjectTypes = OBJECT_TYPE_ROWS(k_rows);
