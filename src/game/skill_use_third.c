#include "render/math3d.h"
#include "session/state.h"
#include "game/context.h"
#include <stdlib.h>

#include "game/skill_use.h"
#include "ui/misc/rolling_cutter.h"
#include <stdint.h>

enum {
    ACTOR_VALUE_FACING = 0,
    ACTOR_VALUE_COUNT = 1,
};

static const char WAVE_SOUL_STRIKE[] = "effect\\ef_soulstrike.wav";
static const char WAVE_HALLUCINATION_WALK[] = "effect\\HALLUCINATIONWALK.wav";
static const char WAVE_JACK_FROST[] = "jack_frost.wav";
static const char WAVE_WL_JACK_FROST[] = "effect\\wl_jackfrost.wav";
static const char WAVE_DRAGON[] = "dragon_h.wav";
static const char WAVE_HEAL_EFFECT[] = "_heal_effect.wav";
static const char WAVE_FURY[] = "effect\\mon_맹룡과강.wav";
static const char WAVE_ANCILLA[] = "effect\\AB_ancilla.wav";
static const char WAVE_LAUDAAGNUS[] = "lauagnus.wav";
static const char WAVE_RENOVATIO[] = "effect\\AB_renovatio.wav";
static const char WAVE_DUPLE_LIGHT[] = "effect\\AB_duplelight.wav";
static const char WAVE_RECOGNIZED_SPELL[] = "effect\\wl_recognizedspell.wav";
static const char WAVE_WOLF_ATTACK[] = "wolf_attack.wav";
static const char WAVE_WOLF_STAND[] = "wolf_stand.wav";
static const char WAVE_METEOR[] = "effect\\wizard_meteo.wav";
static const char WAVE_BLAST_MINE[] = "effect\\hunter_blastmine.wav";
static const char WAVE_TETRA_VORTEX[] = "effect\\wl_tetravortex.wav";

static const struct SimpleLook third_looks[] = {
    { 0x7d3, 0x1b9, ON_CASTER }, { 0x7e0, 0x135, ON_CASTER }, { 0x7eb, 0x303, ON_CASTER },
    { 0x7f9, 0x02a, ON_TARGET }, { 0x7fa, 0x025, ON_TARGET }, { 0x7fd, 0x070, ON_TARGET },
    { 0x7fe, 0x2d5, ON_CASTER }, { 0x809, 0x057, ON_TARGET }, { 0x89f, 0x31d, ON_TARGET },
    { 0x8a1, 0x31f, ON_CASTER }, { 0x8d6, 0x309, ON_CASTER },
};

enum {
    SKILL_RK_WINDCUTTER = 0x7d5,
    SKILL_RK_IGNITIONBREAK = 0x7d6,
    SKILL_RK_DRAGONHOWLING = 0x7d9,
    SKILL_RK_MILLENNIUMSHIELD = 0x7db,
    SKILL_RK_REFRESH = 0x7dd,
    SKILL_RK_GIANTGROWTH = 0x7de,
    SKILL_RK_STONEHARDSKIN = 0x7df,
    SKILL_RK_STORMBLAST = 0x7e1,
    SKILL_RK_FIGHTINGSPIRIT = 0x7e2,
    SKILL_RK_ABUNDANCE = 0x7e3,
    SKILL_RK_PHANTOMTHRUST = 0x7e4,
    SKILL_GC_VENOMIMPRESS = 0x7e5,
    SKILL_GC_ANTIDOTE = 0x7ea,
    SKILL_GC_WEAPONBLOCKING = 0x7ec,
    SKILL_GC_COUNTERSLASH = 0x7ed,
    SKILL_HALLUCINATION_WALK = 0x7f3,
    SKILL_GC_ROLLINGCUTTER = 0x7f4,
    SKILL_ANCILLA = 0x7f7,
    SKILL_AB_CHEAL = 0x7fb,
    SKILL_LAUDAAGNUS = 0x7ff,
    SKILL_LAUDARAMUS = 0x800,
    SKILL_RENOVATIO = 0x802,
    SKILL_AB_HIGHNESSHEAL = 0x803,
    SKILL_AB_CLEARANCE = 0x804,
    SKILL_AB_EXPIATIO = 0x805,
    SKILL_DUPLE_LIGHT = 0x806,
    SKILL_JACK_FROST_RING = 0x89c,
    SKILL_WL_MARSHOFABYSS = 0x89d,
    SKILL_RECOGNIZED_SPELL = 0x89e,
    SKILL_WL_TETRAVORTEX_FIRE = 0x8aa,
    SKILL_WL_TETRAVORTEX_GROUND = 0x8ad,
    SKILL_WL_RELEASE = 0x8b6,
    SKILL_RA_FEARBREEZE = 0x8ba,
    SKILL_RA_WUGMASTERY = 0x8c0,
    SKILL_RA_WUGRIDER = 0x8c1,
};

void skill_use_ninja_throw_look(const struct SkillCast *cast)
{
    if (!cast->caster)
        return;
    actor_launch_effect(cast->caster, 0x2f4);
    play_skill_wave("effect\\닌자_던지기.wav");
}

void skill_use_soul_strike_look(const struct SkillCast *cast)
{
    if (!cast->caster)
        return;
    actor_launch_effect(cast->caster, 0x1b9);
    actor_launch_effect(cast->caster, 0x2ed);
    play_skill_wave(WAVE_SOUL_STRIKE);
}

void skill_use_water_look(const struct SkillCast *cast)
{
    if (!cast->target)
        return;
    actor_launch_effect(cast->target, 0x300);
    play_skill_wave("effect\\수둔.wav");
}

void skill_use_glow_both_look(const struct SkillCast *cast)
{
    if (!cast->caster || !cast->target)
        return;
    actor_launch_effect(cast->caster, 0x21a);
    actor_launch_effect(cast->caster, 0x1b9);
    actor_launch_effect(cast->target, 0x21a);
    actor_launch_effect(cast->target, 0x1b9);
}

void skill_use_hallucination_walk_look(const struct SkillCast *cast)
{
    if (cast->caster)
        play_skill_wave(WAVE_HALLUCINATION_WALK);
}

void skill_use_jack_frost_ring_look(const struct SkillCast *cast)
{
    struct GameActor *caster = cast->caster;
    int i;

    if (!caster || !cast->target)
        return;
    for (i = 0; i < 10; i++) {
        Vec3 here = { 0.0f, 0.0f, 0.0f };
        struct RagEffect *effect = game_actor_launch_effect(caster, 0x321, here, 0.0f);
        Vec3 at;
        int step, k;

        if (!effect)
            continue;
        at = actor_pos(caster);
        step = rand() % 36;
        if (step < 0)
            step = 0;
        k = (short)(step + i * 36);
        at.x = g_cosTable[k] * 80.0f + at.x;
        at.z = g_sinTable[k] * 80.0f + at.z;
        effect_set_pos(effect, &at);
    }
    play_skill_wave(WAVE_JACK_FROST);
}

static void meteor_look(struct GameActor *caster, struct GameActor *target, int which)
{
    Vec3 origin = { 0.0f, 0.0f, 0.0f };
    int i;

    if (caster) {
        play_skill_wave_at(WAVE_METEOR, target->pos.x - caster->pos.x, 0.0f, target->pos.z - caster->pos.z);
        play_skill_wave_at(WAVE_BLAST_MINE, target->pos.x - caster->pos.x, 0.0f, target->pos.z - caster->pos.z);
        play_skill_wave_at(WAVE_TETRA_VORTEX, target->pos.x - caster->pos.x, 0.0f, target->pos.z - caster->pos.z);
    }
    if (g_session1.map_rule_flags[0] == 1) {
        spawn_effect(target->aid, 0x59c + which, origin);
        spawn_effect(target->aid, 0x598 + which, origin);
    } else {
        spawn_effect(target->aid, 0x4f6 + which, origin);
        spawn_effect(target->aid, 0x4f2 + which, origin);
    }
    for (i = 0; i < 4; i++) {
        Vec3 at = actor_pos(target);
        int dx, dz;

        at.z = at.z - 5.0f;
        dx = rand() % 35;
        if (dx < 10)
            dx = 10;
        dz = rand() % 35;
        if (dz < 10)
            dz = 10;
        if (i == 0 || i == 1)
            at.x = (float)dx + at.x;
        else
            at.x = at.x - (float)dx;
        if (i == 0 || i == 3)
            at.z = (float)dz + at.z;
        else
            at.z = at.z - (float)dz;
        spawn_effect(0, 0x4fe + which, at);
    }
}

static void heading_look(struct GameActor *caster)
{
    Vec3 origin = { 0.0f, 0.0f, 0.0f };
    float heading = actor_screen_heading(game_current_mode(), caster);
    int effect_id;

    if (heading > 0.0f && heading < 90.0f)
        effect_id = 0x69c;
    else if (heading >= 90.0f && heading < 180.0f)
        effect_id = 0x69b;
    else if (heading >= 180.0f && heading < 270.0f)
        effect_id = 0x69c;
    else
        effect_id = 0x69b;
    spawn_effect(caster->aid, 0x69a, origin);
    spawn_effect(caster->aid, effect_id, origin);
}

int skill_use_third(const struct SkillCast *cast)
{
    struct GameActor *caster = cast->caster, *target = cast->target;
    Vec3 origin = { 0.0f, 0.0f, 0.0f };

    if (play_simple_look(cast, third_looks, SIMPLE_LOOK_COUNT(third_looks)))
        return 1;

    switch (cast->skill_id) {
    case 0x8ae: case 0x8af: case 0x8b0: case 0x8b5:
        return 1;

    case SKILL_RK_WINDCUTTER:
        if (caster)
            heading_look(caster);
        return 1;

    case SKILL_RK_IGNITIONBREAK:
        play_skill_wave(WAVE_WL_JACK_FROST);
        if (!caster)
            return 1;
        spawn_effect(caster->aid, 0x658, origin);
        spawn_effect(caster->aid, 0x659, origin);
        return 1;

    case SKILL_RK_DRAGONHOWLING:
        if (!caster)
            return 1;
        send_quake();
        actor_launch_effect(caster, 0x2db);
        play_skill_wave(WAVE_DRAGON);
        return 1;

    case SKILL_RK_MILLENNIUMSHIELD:
        skill_use_soul_strike_look(cast);
        return 1;

    case SKILL_RK_REFRESH:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x21a);
        play_skill_wave(WAVE_HEAL_EFFECT);
        return 1;

    case SKILL_RK_GIANTGROWTH:
        skill_use_ninja_throw_look(cast);
        return 1;

    case SKILL_RK_STONEHARDSKIN:
    case SKILL_RK_FIGHTINGSPIRIT:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x2f4);
        actor_launch_effect(caster, 0x1b9);
        return 1;

    case SKILL_RK_STORMBLAST:
        if (caster)
            actor_launch_effect(caster, 0x11);
        if (!target)
            return 1;
        send_quake();
        play_skill_wave(WAVE_FURY);
        actor_launch_effect(target, 0x107);
        actor_launch_effect(target, 0x14a);
        return 1;

    case SKILL_RK_ABUNDANCE:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x21a);
        play_skill_wave("effect\\흡기.wav");
        return 1;

    case SKILL_RK_PHANTOMTHRUST:
        skill_use_glow_both_look(cast);
        return 1;

    case SKILL_GC_VENOMIMPRESS:
        skill_use_water_look(cast);
        return 1;

    case SKILL_GC_ANTIDOTE:
        if (!target)
            return 1;
        actor_launch_effect(target, 0x15);
        actor_launch_effect(target, 0x300);
        return 1;

    case SKILL_GC_WEAPONBLOCKING:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x300);
        play_skill_wave("effect\\t_낙법.wav");
        return 1;

    case SKILL_GC_COUNTERSLASH:
        if (!caster)
            return 1;
        send_quake();
        play_skill_wave(WAVE_FURY);
        spawn_effect(caster->aid, 0x570, origin);
        if (g_session1.map_rule_flags[0] != 0)
            return 1;
        spawn_effect(caster->aid, 0x571, origin);
        return 1;

    case SKILL_HALLUCINATION_WALK:
        skill_use_hallucination_walk_look(cast);
        return 1;

    case SKILL_GC_ROLLINGCUTTER:
        if (!caster)
            return 1;
        spawn_effect(caster->aid, g_session1.map_rule_flags[0] == 1 ? 0x5ba : 0x513, origin);
        skill_use_counted_effect(caster);
        return 1;

    case SKILL_ANCILLA:
        if (!target)
            return 1;
        play_skill_wave(WAVE_ANCILLA);
        actor_launch_effect(target, 0x1b9);
        return 1;

    case SKILL_AB_CHEAL:
    case SKILL_AB_HIGHNESSHEAL:
        skill_use_heal(cast);
        return 1;

    case SKILL_LAUDAAGNUS:
    case SKILL_LAUDARAMUS:
        if (!target)
            return 1;
        actor_launch_effect(target, 0x42);
        play_skill_wave(WAVE_LAUDAAGNUS);
        return 1;

    case SKILL_RENOVATIO:
        if (!target)
            return 1;
        play_skill_wave(WAVE_RENOVATIO);
        actor_launch_effect(target, 0x2cf);
        return 1;

    case SKILL_AB_CLEARANCE:
        if (!target)
            return 1;
        spawn_effect(target->aid, 0x615, origin);
        spawn_effect(target->aid, 0x616, origin);
        return 1;

    case SKILL_AB_EXPIATIO:
        if (caster)
            play_skill_wave("effect\\바실리카.wav");
        return 1;

    case SKILL_DUPLE_LIGHT: {
        struct RagEffect *effect;
        Vec3 at;

        if (!target)
            return 1;
        play_skill_wave(WAVE_DUPLE_LIGHT);
        if (able_to_make_effect_is_effect_exist(target, 0x341))
            return 1;
        effect = actor_launch_effect(target, 0x341);
        at.x = 2.0f;
        at.y = 0.0f;
        at.z = 0.0f;
        if (effect)
            effect_set_pos(effect, &at);
        return 1;
    }

    case SKILL_JACK_FROST_RING:
        skill_use_jack_frost_ring_look(cast);
        return 1;

    case SKILL_WL_MARSHOFABYSS:
        if (!target)
            return 1;
        actor_launch_effect(target, 0x2d9);
        actor_launch_effect(target, 0x1d2);
        return 1;

    case SKILL_RECOGNIZED_SPELL:
        if (caster)
            play_skill_wave(WAVE_RECOGNIZED_SPELL);
        return 1;

    case 0x8aa: case 0x8ab: case 0x8ac: case SKILL_WL_TETRAVORTEX_GROUND:
        if (target)
            meteor_look(caster, target, cast->skill_id - SKILL_WL_TETRAVORTEX_FIRE);
        return 1;

    case SKILL_WL_RELEASE: {
        struct RagEffect *effect;
        Vec3 at;

        if (!caster || !target)
            return 1;
        effect = actor_launch_effect(caster, 0x2ef);
        if (!effect)
            return 1;
        at = actor_pos(target);
        effect_set_pos(effect, &at);
        return 1;
    }

    case SKILL_RA_FEARBREEZE:
        if (caster)
            spawn_effect(caster->aid, 0x60d, origin);
        return 1;

    case SKILL_RA_WUGMASTERY:
        play_skill_wave(WAVE_WOLF_ATTACK);
        return 1;

    case SKILL_RA_WUGRIDER:
        play_skill_wave(WAVE_WOLF_STAND);
        return 1;
    }
    return 0;
}

void skill_use_counted_effect(struct GameActor *actor)
{
    if (!actor)
        return;
    if (!actor_launch_effect(actor, 0x345))
        return;
    if (game_actor_get_counter(actor, ACTOR_VALUE_COUNT) <= 0) {
        game_actor_set_counter(actor, ACTOR_VALUE_COUNT, 0);
        game_actor_set_counter(actor, ACTOR_VALUE_FACING, (int64_t)(actor->roty * 1000.0f));
    }
    game_actor_set_counter(actor, ACTOR_VALUE_COUNT,
                 (int)((unsigned)game_actor_get_counter(actor, ACTOR_VALUE_COUNT) + 1));
}
