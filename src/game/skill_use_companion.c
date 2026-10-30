#include "game/skill_use.h"
#include "world/world.h"

static const char WAVE_DEFENCE[] = "effect\\h_defence.wav";
static const char WAVE_CASTLING[] = "effect\\h_castling.wav";
static const char WAVE_SERPENS[] = "effect\\EM_SUMMON_ELEMENTAL_SERPENS_1.wav";
static const char WAVE_DILUVIO[] = "effect\\EM_SUMMON_ELEMENTAL_DILUVIO_1.wav";
static const char WAVE_ARDOR[] = "effect\\EM_SUMMON_ELEMENTAL_ARDOR_1.wav";
static const char WAVE_PROCELLA[] = "effect\\EM_SUMMON_ELEMENTAL_PROCELLA_1.wav";
static const char WAVE_TERREMOTUS[] = "effect\\EM_SUMMON_ELEMENTAL_TERREMOTUS_1.wav";

static const struct SimpleLook companion_looks[] = {
    { 0x1f44, 0x1f9, ON_CASTER }, { 0x1f48, 0x23b, ON_CASTER }, { 0x1f4a, 0x234, ON_CASTER },
    { 0x1f4b, 0x234, ON_CASTER }, { 0x1f50, 0x0b7, ON_CASTER }, { 0x1f5a, 0x3c1, ON_CASTER },
    { 0x1f67, 0x3c2, ON_CASTER }, { 0x201b, 0x0de, ON_CASTER }, { 0x201e, 0x04c, ON_CASTER },
    { 0x202e, 0x070, ON_CASTER }, { 0x202f, 0x02a, ON_TARGET }, { 0x2030, 0x025, ON_TARGET },
};

static const struct {
    unsigned short skill_id;
    unsigned char on;
    unsigned short first, second;
} spawn_looks[] = {
    { 0x1f6d, ON_CASTER, 0x924, 0 },     { 0x1f75, ON_TARGET, 0x910, 0x911 },
    { 0x1f79, ON_TARGET, 0x91b, 0x91c }, { 0x1f7a, ON_CASTER, 0x91e, 0 },
    { 0x20fb, ON_TARGET, 0x8d6, 0x8d7 }, { 0x20fe, ON_TARGET, 0x8d4, 0x8d5 },
    { 0x2101, ON_TARGET, 0x8d8, 0x8d9 }, { 0x2104, ON_TARGET, 0x8da, 0x8db },
    { 0x2107, ON_TARGET, 0x8d2, 0x8d3 },
};

static const struct {
    unsigned short skill_id;
    unsigned short effect_id;
    const char *wave;
} summon_looks[] = {
    { 0x20fc, 0x7f8, WAVE_ARDOR },    { 0x20ff, 0x7e8, WAVE_DILUVIO },
    { 0x2102, 0x837, WAVE_PROCELLA }, { 0x2105, 0x83c, WAVE_TERREMOTUS },
    { 0x2108, 0x7dd, WAVE_SERPENS },
};

#define COUNT(table) ((int)(sizeof(table) / sizeof((table)[0])))

enum {
    SKILL_HLIF_HEAL = 0x1f41,
    SKILL_HLIF_AVOID = 0x1f42,
    SKILL_CASTLING = 0x1f45,
    SKILL_DEFENCE = 0x1f46,
    SKILL_MH_THE_ONE_FIGHTER_RISES = 0x1f73,
    SKILL_MS_PARRYING = 0x200c,
    SKILL_BERSERK = 0x200e,
    SKILL_ML_AUTOGUARD = 0x201c,
    SKILL_ABR_NET_REPAIR = 0x219b,
    SKILL_ABR_NET_SUPPORT = 0x219c,
    ACTOR_MSG_GUARD_EFFECT = 0x7e,
};

static void wave_toward_player(const char *wave, struct GameActor *actor)
{
    struct Player *player = game_current_player();

    if (player && actor)
        play_skill_wave_at(wave, player->pos.x - actor->pos.x, 0.0f, player->pos.z - actor->pos.z);
}

static void summon_look(const struct SkillCast *cast, int effect_id, const char *wave)
{
    struct GameActor *target = cast->target, *linked;

    if (!target || !cast->mode->world)
        return;
    spawn_on(target->aid, effect_id);
    linked = world_get_actor_or_player(cast->mode->world, target->owner_aid);
    if (!linked)
        return;
    spawn_on(linked->aid, effect_id);
    wave_toward_player(wave, linked);
}

static void target_and_caster_once(const struct SkillCast *cast, int target_effect_id, int caster_effect_id)
{
    if (!cast->target)
        return;
    actor_launch_effect(cast->target, target_effect_id);
    if (!cast->caster)
        return;
    if (effect_mgr_find_effect(g_effectMgr, cast->caster->aid, caster_effect_id))
        return;
    spawn_on(cast->caster->aid, caster_effect_id);
}

static void wave_and_both(const struct SkillCast *cast, const char *wave, int effect_id)
{
    play_skill_wave(wave);
    caster_effect(cast, effect_id);
    target_effect(cast, effect_id);
}

int skill_use_companion(const struct SkillCast *cast)
{
    struct GameActor *caster = cast->caster, *target = cast->target;
    int i;

    if (play_simple_look(cast, companion_looks, SIMPLE_LOOK_COUNT(companion_looks)))
        return 1;
    for (i = 0; i < COUNT(spawn_looks); i++) {
        struct GameActor *on;

        if (spawn_looks[i].skill_id != cast->skill_id)
            continue;
        on = spawn_looks[i].on == ON_CASTER ? caster : target;
        if (!on)
            return 1;
        spawn_on(on->aid, spawn_looks[i].first);
        if (spawn_looks[i].second)
            spawn_on(on->aid, spawn_looks[i].second);
        return 1;
    }
    for (i = 0; i < COUNT(summon_looks); i++) {
        if (summon_looks[i].skill_id == cast->skill_id) {
            summon_look(cast, summon_looks[i].effect_id, summon_looks[i].wave);
            return 1;
        }
    }

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

    case SKILL_MH_THE_ONE_FIGHTER_RISES:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x922);
        spawn_on(caster->aid, 0x923);
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

    case SKILL_ABR_NET_REPAIR:
        target_and_caster_once(cast, 0x139, 0x76f);
        return 1;

    case SKILL_ABR_NET_SUPPORT:
        target_and_caster_once(cast, 0xfd, 0x770);
        return 1;
    }
    return 0;
}
