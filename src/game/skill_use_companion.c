#include "game/skill_use.h"

static const char WAVE_DEFENCE[] = "effect\\h_defence.wav";
static const char WAVE_CASTLING[] = "effect\\h_castling.wav";

static const struct SimpleLook companion_looks[] = {
    { 0x1f44, 0x1f9, ON_CASTER }, { 0x1f48, 0x23b, ON_CASTER }, { 0x1f4a, 0x234, ON_CASTER },
    { 0x1f4b, 0x234, ON_CASTER }, { 0x1f50, 0x0b7, ON_CASTER },
    { 0x201b, 0x0de, ON_CASTER }, { 0x201e, 0x04c, ON_CASTER },
    { 0x202e, 0x070, ON_CASTER }, { 0x202f, 0x02a, ON_TARGET }, { 0x2030, 0x025, ON_TARGET },
};

enum {
    SKILL_HLIF_HEAL = 0x1f41,
    SKILL_HLIF_AVOID = 0x1f42,
    SKILL_CASTLING = 0x1f45,
    SKILL_DEFENCE = 0x1f46,
    SKILL_MS_PARRYING = 0x200c,
    SKILL_BERSERK = 0x200e,
    SKILL_ML_AUTOGUARD = 0x201c,
    ACTOR_MSG_GUARD_EFFECT = 0x7e,
};

static void wave_and_both(const struct SkillCast *cast, const char *wave, int effect_id)
{
    play_skill_wave(wave);
    caster_effect(cast, effect_id);
    target_effect(cast, effect_id);
}

int skill_use_companion(const struct SkillCast *cast)
{
    struct GameActor *caster = cast->caster;

    if (play_simple_look(cast, companion_looks, SIMPLE_LOOK_COUNT(companion_looks)))
        return 1;

    switch (cast->skill_id) {
    case SKILL_HLIF_HEAL:
        skill_use_heal(cast);
        return 1;

    case SKILL_HLIF_AVOID:
        caster_effect(cast, 0x1c8);
        target_effect(cast, 0x1c8);
        return 1;

    case SKILL_CASTLING:
        wave_and_both(cast, WAVE_CASTLING, 0x23a);
        return 1;

    case SKILL_DEFENCE:
        wave_and_both(cast, WAVE_DEFENCE, 0x239);
        return 1;

    case SKILL_MS_PARRYING:
    case SKILL_ML_AUTOGUARD:
        if (caster)
            actor_send(caster, ACTOR_MSG_GUARD_EFFECT);
        return 1;

    case SKILL_BERSERK:
        if (!caster)
            return 1;
        actor_restart_effect(caster, 0x170);
        send_quake();
        play_skill_wave("effect\\버서크.wav");
        return 1;
    }
    return 0;
}
