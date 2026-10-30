#ifndef SKILLS_SKILL_ACTION_INTERNAL_H
#define SKILLS_SKILL_ACTION_INTERNAL_H

#include "skills/skill_action.h"
#include "session/state.h"
#include <string.h>
#include "core/fatal.h"
#include "game/context.h"
#include "std/std.h"
#include "effects/effect.h"
#include "effects/effect_mgr.h"
#include "effects/render_object.h"
#include "effects/ez2str_effect_mgr.h"
#include "actor/actor.h"

struct Effect *skill_spawn_named(const char *name, int aid, Vec3 pos, int target_aid, int level,
                                  unsigned char reuse, int instance_key);
int skill_named_effect_id(const char *name);
int skill_format_effect_id(const char *fmt, int n);
static __inline void skill_remove_named(const char *name, SkillActor *actor)
{
    effect_mgr_remove_type(g_effectMgr, actor->aid, skill_named_effect_id(name));
}
static __inline Vec3 skill_vec_zero(void)
{
    Vec3 v;

    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 0.0f;
    return v;
}

static __inline struct Effect *skill_spawn_on(const char *name, SkillActor *actor)
{
    return skill_spawn_named(name, actor->aid, skill_vec_zero(), 0, -1, 0, 0);
}
static __inline struct Effect *skill_spawn_at(const char *name, Vec3 pos)
{
    return skill_spawn_named(name, 0, pos, 0, -1, 0, 0);
}

struct Vec *skill_shuffled_indices(struct Vec *out, int count);

static __inline struct RagEffect *skill_launch(SkillActor *actor, int effect_id)
{
    return game_actor_launch_effect((struct GameActor *)actor, effect_id, skill_vec_zero(), 0);
}
static __inline void skill_cast_effect(SkillActor *caster, int effect_id, unsigned cast_time)
{
    if (caster)
        game_actor_launch_effect_timed((struct GameActor *)caster, effect_id, cast_time >> 4);
}

#endif
