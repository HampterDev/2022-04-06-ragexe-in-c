#include "effects/rag/rag_effect.h"
#include "game/skill_use.h"
#include "effects/effect_controls.h"

static const char WAVE_SANCTUARY[] = "effect\\priest_sanctuary.wav";
static const char WAVE_REPRODUCE[] = "effect\\reproduce.wav";

static const struct SimpleLook late_looks[] = {
    { 0x13cc, 0x350, ON_CASTER },
    { 0x13cf, 0x412, ON_CASTER },
};

enum {
    SKILL_SC_REPRODUCE = 0x8ed,
    SKILL_WE_ONEFOREVER = 0x13c8,
    SKILL_WE_CHEERUP = 0x13c9,
    SKILL_NV_HELPANGEL = 0x13d4,
};

static void spawn_pair_on(struct GameActor *actor, int effect_1, int effect_2)
{
    Vec3 origin = { 0.0f, 0.0f, 0.0f };

    spawn_effect(actor->aid, effect_1, origin);
    spawn_effect(actor->aid, effect_2, origin);
}

int skill_use_late(const struct SkillCast *cast)
{
    struct GameActor *caster = cast->caster, *target = cast->target;

    if (play_simple_look(cast, late_looks, SIMPLE_LOOK_COUNT(late_looks)))
        return 1;

    switch (cast->skill_id) {
    case SKILL_SC_REPRODUCE:
        if (caster)
            spawn_pair_on(caster, 0x631, 0x632);
        play_skill_wave(WAVE_REPRODUCE);
        return 1;

    case SKILL_WE_ONEFOREVER:
        if (target)
            spawn_effect(target->aid, 0x475, actor_pos(target));
        if (caster)
            spawn_effect(caster->aid, 0x475, actor_pos(caster));
        return 1;

    case SKILL_WE_CHEERUP:
        if (target)
            actor_launch_effect(target, 0x198);
        if (caster)
            spawn_effect(caster->aid, 0x476, actor_pos(caster));
        return 1;

    case SKILL_NV_HELPANGEL:
        if (!caster)
            return 1;
        play_skill_wave(WAVE_SANCTUARY);
        spawn_pair_on(caster, 0x557, 0x558);
        return 1;
    }
    return 0;
}
