#include "session/state.h"
#include "game/skill_use.h"
#include "actor/game_actor.h"

static const char WAVE_REPARATIO_1[] = "effect\\CD_REPARATIO1.wav";
static const char WAVE_REPARATIO_2[] = "effect\\CD_REPARATIO2.wav";
static const char WAVE_DILECTIO_HEAL[] = "effect\\CD_DILECTIO_HEAL.wav";
static const char WAVE_CHAIN_REACTION_SHOT[] = "effect\\ABC_CHAIN_REACTION_SHOT.wav";
static const char WAVE_JUDGEMENT_CROSS[] = "effect\\IG_JUDGEMENT_CROSS.wav";
static const char WAVE_SECOND_JUDGEMENT[] = "effect\\IQ_SECOND_JUDGEMENT.wav";
static const char WAVE_UNLUCKY_RUSH[] = "effect\\ABC_UNLUCKY_RUSH.wav";
static const char WAVE_ABYSS_SLAYER[] = "effect\\ABC_ABYSS_SLAYER.wav";

static const struct SpawnLook {
    unsigned short skill_id;
    unsigned char on;
    unsigned short first, second;
} spawn_looks[] = {
    { 0x1470, ON_CASTER, 0x709, 0 },     { 0x1471, ON_TARGET, 0x6d1, 0x6d2 },
    { 0x1479, ON_CASTER, 0x7c5, 0 },     { 0x1483, ON_TARGET, 0x840, 0x841 },
    { 0x1484, ON_TARGET, 0x825, 0x826 }, { 0x1485, ON_TARGET, 0x831, 0x8e7 },
    { 0x1487, ON_CASTER, 0x879, 0 },     { 0x1488, ON_CASTER, 0x88d, 0x88c },
    { 0x1489, ON_CASTER, 0x894, 0x895 }, { 0x148c, ON_CASTER, 0x87c, 0 },
    { 0x148d, ON_CASTER, 0x898, 0x899 }, { 0x148f, ON_TARGET, 0x87f, 0x880 },
    { 0x1492, ON_CASTER, 0x86d, 0x86e }, { 0x1495, ON_TARGET, 0x710, 0 },
    { 0x1497, ON_TARGET, 0x713, 0 },     { 0x1498, ON_TARGET, 0x715, 0 },
    { 0x149b, ON_TARGET, 0x717, 0 },     { 0x149d, ON_TARGET, 0x736, 0x737 },
    { 0x14a1, ON_TARGET, 0x744, 0 },     { 0x14a2, ON_TARGET, 0x75f, 0 },
    { 0x14a3, ON_TARGET, 0x777, 0 },     { 0x14a4, ON_TARGET, 0x789, 0x78b },
    { 0x14aa, ON_CASTER, 0x78d, 0 },     { 0x14ac, ON_CASTER, 0x767, 0x768 },
    { 0x14b2, ON_CASTER, 0x730, 0 },     { 0x14b6, ON_CASTER, 0x78e, 0 },
    { 0x14b7, ON_CASTER, 0x77b, 0 },     { 0x14b8, ON_CASTER, 0x76f, 0x770 },
    { 0x14b9, ON_CASTER, 0x772, 0 },     { 0x14c8, ON_TARGET, 0x854, 0 },
    { 0x14cc, ON_TARGET, 0x76b, 0 },     { 0x14ce, ON_TARGET, 0x771, 0 },
    { 0x14d0, ON_CASTER, 0x7a8, 0 },     { 0x14da, ON_TARGET, 0x88a, 0 },
    { 0x14db, ON_TARGET, 0x871, 0x872 }, { 0x14e3, ON_CASTER, 0x8c1, 0 },
    { 0x14f6, ON_TARGET, 0x7e9, 0 },     { 0x14f7, ON_TARGET, 0x7d3, 0 },
    { 0x14f8, ON_TARGET, 0x7c8, 0 },     { 0x150a, ON_CASTER, 0x8bf, 0 },
    { 0x150c, ON_CASTER, 0x8d1, 0 },     { 0x150d, ON_CASTER, 0x85b, 0x85c },
    { 0x150e, ON_CASTER, 0x864, 0x865 }, { 0x150f, ON_CASTER, 0x867, 0x868 },
    { 0x1510, ON_CASTER, 0x85e, 0x85f }, { 0x1511, ON_CASTER, 0x861, 0x862 },
};

static const struct LinkLook {
    unsigned short skill_id;
    unsigned short effect_id;
    unsigned char by_place;
} link_looks[] = {
    { 0x1472, 0x6db, 1 }, { 0x1473, 0x6dc, 0 }, { 0x14a7, 0x72d, 0 }, { 0x14ab, 0x7a1, 0 },
    { 0x14ca, 0x842, 0 }, { 0x14d1, 0x7b3, 1 }, { 0x14d6, 0x79f, 0 }, { 0x14dc, 0x8a6, 0 },
    { 0x14dd, 0x8b6, 0 }, { 0x14de, 0x8ae, 0 }, { 0x14df, 0x89e, 0 }, { 0x14e9, 0x802, 0 },
    { 0x1508, 0x8cc, 0 }, { 0x1509, 0x8ca, 0 },
};

static const struct TintLook {
    unsigned short skill_id;
    unsigned char shade, tone;
    unsigned short first, second;
} tint_looks[] = {
    { 0x1476, 1, 1, 0x7c3, 0 }, { 0x1477, 1, 1, 0x7cf, 0 }, { 0x147a, 1, 1, 0x7ac, 0x7ad },
    { 0x14e6, 1, 3, 0, 0 },     { 0x14e7, 1, 3, 0x7d0, 0 }, { 0x14e8, 1, 3, 0x7d6, 0 },
    { 0x14ee, 1, 3, 0x844, 0 }, { 0x14ef, 1, 3, 0x847, 0 }, { 0x14f0, 1, 3, 0x84c, 0 },
    { 0x14f1, 1, 3, 0x84a, 0 }, { 0x14f2, 1, 3, 0x811, 0 }, { 0x14f3, 1, 3, 0x81b, 0 },
    { 0x14f4, 1, 3, 0x814, 0 },
};

#define COUNT(table) ((int)(sizeof(table) / sizeof((table)[0])))

enum {
    SKILL_AG_FROZEN_SLASH = 0x1475,
    SKILL_IQ_MASSIVE_F_BLASTER = 0x147b,
    SKILL_IQ_EXPOSION_BLASTER = 0x147c,
    SKILL_IQ_FIRST_FAITH_POWER = 0x147e,
    SKILL_IQ_JUDGE = 0x147f,
    SKILL_IQ_SECOND_FLAME = 0x1480,
    SKILL_IQ_SECOND_JUDGEMENT = 0x1482,
    SKILL_IQ_THIRD_EXOR_FLAME = 0x1486,
    SKILL_JUDGEMENT_CROSS = 0x1490,
    SKILL_REPARATIO = 0x1494,
    SKILL_CD_COMPETENTIA = 0x149e,
    SKILL_DILECTIO_HEAL = 0x14a0,
    SKILL_SHC_ETERNAL_SLASH = 0x14a9,
    SKILL_MT_AXE_STOMP = 0x14af,
    SKILL_MT_RUSH_QUAKE = 0x14b0,
    SKILL_MT_M_MACHINE = 0x14b1,
    SKILL_MT_D_MACHINE = 0x14b3,
    SKILL_AG_DESTRUCTIVE_HURRICANE_CLIMAX = 0x14ba,
    SKILL_ABC_STRIP_SHADOW = 0x14c1,
    SKILL_ABC_ABYSS_DAGGER = 0x14c2,
    SKILL_UNLUCKY_RUSH = 0x14c3,
    SKILL_CHAIN_REACTION_SHOT = 0x14c4,
    SKILL_ABC_FROM_THE_ABYSS = 0x14c5,
    SKILL_ABYSS_SLAYER = 0x14c6,
    SKILL_BO_BIONIC_PHARMACY = 0x14d8,
};

static void tint_caster(struct GameActor *caster, int shade, int tone, int duration_ms)
{
    render_object_play_motion_range((struct RenderObject *)caster, 0x60, shade, tone, 0x60, tone, 1, duration_ms, 0);
}

static void wave_toward_player(const char *wave, struct GameActor *actor)
{
    struct Player *player = game_current_player();

    if (player && actor)
        play_skill_wave_at(wave, player->pos.x - actor->pos.x, 0.0f, player->pos.z - actor->pos.z);
}

static void link_effect(int aid, int effect_id, struct GameActor *caster, struct GameActor *target)
{
    effect_mgr_spawn(g_effectMgr, aid, effect_id, actor_pos(caster), target->aid, -1, 0, 0);
}

static int play_table_looks(const struct SkillCast *cast)
{
    struct GameActor *caster = cast->caster, *target = cast->target;
    int i;

    for (i = 0; i < COUNT(spawn_looks); i++) {
        const struct SpawnLook *look = &spawn_looks[i];
        struct GameActor *on;

        if (look->skill_id != cast->skill_id)
            continue;
        on = look->on == ON_CASTER ? caster : target;
        if (!on)
            return 1;
        spawn_on(on->aid, look->first);
        if (look->second)
            spawn_on(on->aid, look->second);
        return 1;
    }
    for (i = 0; i < COUNT(link_looks); i++) {
        if (link_looks[i].skill_id != cast->skill_id)
            continue;
        if (caster && target)
            link_effect(link_looks[i].by_place ? 0 : (int)caster->aid, link_looks[i].effect_id, caster, target);
        return 1;
    }
    for (i = 0; i < COUNT(tint_looks); i++) {
        const struct TintLook *look = &tint_looks[i];

        if (look->skill_id != cast->skill_id)
            continue;
        if (!caster)
            return 1;
        tint_caster(caster, look->shade, look->tone, 500);
        if (look->first)
            spawn_on(caster->aid, look->first);
        if (look->second)
            spawn_on(caster->aid, look->second);
        return 1;
    }
    return 0;
}

static void tint_and_delayed_pair(struct GameActor *caster, int tone, int first, int delayed_1, int delayed_2)
{
    tint_caster(caster, tone, tone, 500);
    spawn_on(caster->aid, first);
    spawn_on_delayed(caster->aid, delayed_1, 1000);
    spawn_on_delayed(caster->aid, delayed_2, 1000);
}

static void spread_look(struct GameActor *caster)
{
    int i;

    tint_caster(caster, 1, 3, 500);
    spawn_effect(caster->aid, 0x6f5, actor_pos(caster));
    for (i = 0; i < 8; i++)
        spawn_effect(caster->aid, 0x6f9 + i, actor_pos(caster));
}

static void reparatio_look(struct GameActor *caster, struct GameActor *target)
{
    struct Effect *effect;

    if (caster) {
        if (g_session1.map_rule_flags[0] == 0)
            spawn_on(caster->aid, 0x727);
        spawn_on(caster->aid, 0x728);
        spawn_on_delayed(caster->aid, 0x729, 500);
        effect = spawn_on(caster->aid, 0x72a);
        if (effect) {
            spawned_effect_delay(effect, 500);
            effect_add_sound_cue(effect, 0, WAVE_REPARATIO_1, 0x1f, 0.0f);
        }
    }
    if (!target)
        return;
    effect = spawn_on(target->aid, 0x72b);
    if (!effect)
        return;
    spawned_effect_delay(effect, 800);
    effect_add_sound_cue(effect, 0, WAVE_REPARATIO_2, 0x32, 0.0f);
}

static void dilectio_heal_look(struct GameActor *caster, struct GameActor *target)
{
    struct Effect *effect = spawn_on_delayed(caster->aid, 0x784, 400);

    if (!target)
        return;
    if (caster->aid == target->aid) {
        if (effect)
            effect_add_sound_cue(effect, 0, WAVE_DILECTIO_HEAL, 0x19, 0.0f);
        return;
    }
    if (!effect_mgr_find_effect(g_effectMgr, target->aid, 0x787))
        spawn_on(target->aid, 0x787);
    if (!effect_mgr_find_effect(g_effectMgr, target->aid, 0x788))
        spawn_on(target->aid, 0x788);
}

static void scaled_trio_look(const struct SkillCast *cast, struct GameActor *caster)
{
    float scale = 1.0f;
    struct Effect *effect;

    if (cast->level == 1 || cast->level == 2)
        scale = 0.7f;
    else if (cast->level == 5)
        scale = 1.2f;
    if (!caster)
        return;
    effect = spawn_on(caster->aid, 0x794);
    if (effect)
        spawned_effect_zoom(effect, scale);
    effect = spawn_on(caster->aid, 0x795);
    if (effect)
        spawned_effect_zoom(effect, scale);
    spawn_on(caster->aid, 0x796 + (caster->action - caster->base_action));
}

static void link_and_wave(struct GameActor *caster, struct GameActor *target, int effect_id, const char *wave)
{
    if (caster) {
        if (!target)
            return;
        link_effect(caster->aid, effect_id, caster, target);
    }
    if (target)
        wave_toward_player(wave, target);
}

int skill_use_fourth(const struct SkillCast *cast)
{
    struct GameActor *caster = cast->caster, *target = cast->target;

    if (play_table_looks(cast))
        return 1;

    switch (cast->skill_id) {
    case SKILL_AG_FROZEN_SLASH:
        if (caster)
            spread_look(caster);
        return 1;

    case SKILL_IQ_MASSIVE_F_BLASTER:
        if (!caster)
            return 1;
        spawn_effect(0, 0x82e, actor_pos(caster));
        render_object_set_script_step_frames((struct RenderObject *)caster, 5);
        render_object_play_motion_range((struct RenderObject *)caster, 0x60, 1, 2, 0x60, 2, 1, 1100, 0);
        return 1;

    case SKILL_IQ_EXPOSION_BLASTER:
        if (!caster)
            return 1;
        spawn_on(caster->aid, 0x7cb);
        spawn_on(caster->aid, 0x7cc);
        render_object_set_script_step_frames((struct RenderObject *)caster, 5);
        render_object_play_motion_range((struct RenderObject *)caster, 0x60, 1, 2, 0x60, 2, 1, 1100, 0);
        return 1;

    case SKILL_IQ_FIRST_FAITH_POWER:
        if (!caster)
            return 1;
        spawn_on(caster->aid, 0x7b9);
        spawn_on_delayed(caster->aid, 0x7b7, 1000);
        return 1;

    case SKILL_IQ_JUDGE:
        if (caster)
            tint_and_delayed_pair(caster, 5, 0x7bc, 0x7ba, 0x7bd);
        return 1;

    case SKILL_IQ_THIRD_EXOR_FLAME:
        if (caster)
            tint_and_delayed_pair(caster, 2, 0x7c0, 0x7be, 0x7c1);
        return 1;

    case SKILL_IQ_SECOND_FLAME:
        if (!target)
            return 1;
        spawn_on_delayed(target->aid, 0x823, 500);
        spawn_on_delayed(target->aid, 0x824, 500);
        return 1;

    case SKILL_IQ_SECOND_JUDGEMENT:
        if (target)
            wave_toward_player(WAVE_SECOND_JUDGEMENT, target);
        return 1;

    case SKILL_JUDGEMENT_CROSS:
        link_and_wave(caster, target, 0x893, WAVE_JUDGEMENT_CROSS);
        return 1;

    case SKILL_CHAIN_REACTION_SHOT:
        link_and_wave(caster, target, 0x7e2, WAVE_CHAIN_REACTION_SHOT);
        return 1;

    case SKILL_REPARATIO:
        reparatio_look(caster, target);
        return 1;

    case SKILL_CD_COMPETENTIA:
        if (!target)
            return 1;
        spawn_on(target->aid, 0x739);
        if (g_session1.map_rule_flags[0] == 0)
            spawn_on(target->aid, 0x73a);
        return 1;

    case SKILL_DILECTIO_HEAL:
        if (caster)
            dilectio_heal_look(caster, target);
        return 1;

    case SKILL_SHC_ETERNAL_SLASH:
        if (!caster)
            return 1;
        skill_use_counted_effect(caster);
        spawn_on(caster->aid, 0x778);
        return 1;

    case SKILL_MT_AXE_STOMP:
        scaled_trio_look(cast, caster);
        return 1;

    case SKILL_MT_RUSH_QUAKE:
        spawn_on(cast->target_aid, 0x75c);
        spawn_on(cast->target_aid, 0x75d);
        return 1;

    case SKILL_MT_M_MACHINE:
        target_effect(cast, 0x65);
        return 1;

    case SKILL_MT_D_MACHINE:
        spawn_on(cast->target_aid, 0x71a);
        spawn_on(cast->target_aid, 0x71b);
        spawn_on_delayed(cast->target_aid, 0x71c, 500);
        spawn_on_delayed(cast->target_aid, 0x719, 1430);
        return 1;

    case SKILL_AG_DESTRUCTIVE_HURRICANE_CLIMAX:
        spawn_on(caster->aid, 0x6c8);
        return 1;

    case SKILL_ABC_STRIP_SHADOW: {
        struct RagEffect *effect;

        if (!target)
            return 1;
        effect = actor_launch_effect(target, 0x1ef);
        if (effect)
            effect_set_pos(effect, (const Vec3 *)&target->pos.x);
        return 1;
    }

    case SKILL_ABC_ABYSS_DAGGER:
        if (!caster)
            return 1;
        spawn_on(caster->aid, 0x7ae);
        actor_set_state(caster, 2, 1);
        return 1;

    case SKILL_UNLUCKY_RUSH:
        if (target)
            wave_toward_player(WAVE_UNLUCKY_RUSH, target);
        return 1;

    case SKILL_ABC_FROM_THE_ABYSS:
        if (!caster)
            return 1;
        spawn_on(caster->aid, 0x83e);
        tint_caster(caster, 0, 0, 700);
        return 1;

    case SKILL_ABYSS_SLAYER:
        if (caster)
            wave_toward_player(WAVE_ABYSS_SLAYER, caster);
        return 1;

    case SKILL_BO_BIONIC_PHARMACY:
        caster_effect(cast, 0x3a);
        return 1;
    }
    return 0;
}
