#include "game/skill_use.h"
#include "actor/game_actor.h"

struct SongLook {
    unsigned short skill_id;
    unsigned short effect_id;
    const char *wave;
};

static const struct SongLook songs[] = {
    { 0x132, 0x34d, "effect\\자장가.wav" },
    { 0x133, 0x34a, "effect\\김서방돈.wav" },
    { 0x134, 0x34c, "effect\\영원의 혼돈.wav" },
    { 0x135, 0x34a, "effect\\전장의.wav" },
    { 0x136, 0x34a, "effect\\니벨룽겐의 반지.wav" },
    { 0x137, 0x34d, "effect\\로키.wav" },
    { 0x138, 0x34a, "effect\\심연속으로.wav" },
    { 0x139, 0x34a, "effect\\불사신.wav" },
    { 0x140, 0x348, "effect\\석양의 어쌔신.wav" },
    { 0x141, 0x34b, "effect\\브라기의 시.wav" },
    { 0x142, 0x349, "effect\\이둔의 사과.wav" },
    { 0x147, 0x34c, "effect\\흥얼거림.wav" },
    { 0x148, 0x34c, "effect\\나를잊지말아요.wav" },
    { 0x149, 0x348, "effect\\행운의.wav" },
    { 0x14a, 0x348, "effect\\당신을 위한 서비스.wav" },
    { 0x1e7, 0x34b, "effect\\ac_concentration.wav" },
};

static const struct SimpleLook second_looks[] = {
    { 0x13d, 0x34b, ON_CASTER }, { 0x13f, 0x34c, ON_CASTER }, { 0x145, 0x34b, ON_CASTER },
    { 0x156, 0x1c5, ON_TARGET }, { 0x157, 0x10d, ON_TARGET }, { 0x15e, 0x1c8, ON_CASTER },
    { 0x175, 0x17f, ON_CASTER }, { 0x17a, 0x1ed, ON_CASTER }, { 0x17c, 0x182, ON_CASTER },
    { 0x17f, 0x185, ON_CASTER }, { 0x180, 0x186, ON_CASTER }, { 0x186, 0x188, ON_CASTER },
    { 0x192, 0x193, ON_TARGET }, { 0x193, 0x1f9, ON_CASTER }, { 0x196, 0x199, ON_CASTER },
    { 0x198, 0x198, ON_TARGET }, { 0x19d, 0x1b3, ON_CASTER }, { 0x1cb, 0x062, ON_CASTER },
    { 0x1db, 0x1f0, ON_CASTER }, { 0x1f8, 0x271, ON_CASTER }, { 0x1f9, 0x272, ON_CASTER },
    { 0x1fa, 0x1c8, ON_CASTER }, { 0x204, 0x27d, ON_CASTER }, { 0x205, 0x272, ON_CASTER },
    { 0x218, 0x27b, ON_TARGET }, { 0x21b, 0x27c, ON_CASTER }, { 0x21d, 0x26e, ON_CASTER },
};

static const char WAVE_MAGIC_AMPLIFICATION[] = "effect\\마법력 증폭.wav";
static const char WAVE_INC_AGILITY[] = "effect\\EF_IncAgility.wav";
static const char WAVE_RESURRECTION[] = "effect\\priest_resurrection.wav";
static const char WAVE_SLOW_POISON[] = "effect\\priest_slowpoison.wav";

enum {
    SKILL_BA_FROSTJOKE = 0x13e,
    SKILL_DC_SCREAM = 0x146,
    SKILL_WE_MALE = 0x14e,
    SKILL_WE_FEMALE = 0x14f,
    SKILL_NPC_POWERUP = 0x15d,
    SKILL_AURA_BLADE = 0x163,
    SKILL_LK_PARRYING = 0x164,
    SKILL_LK_CONCENTRATION = 0x165,
    SKILL_BERSERK = 0x167,
    SKILL_LK_FURY = 0x168,
    SKILL_HP_ASSUMPTIO = 0x169,
    SKILL_HW_SOULDRAIN = 0x16c,
    SKILL_HW_MAGICPOWER = 0x16e,
    SKILL_PF_SOULCHANGE = 0x176,
    SKILL_PF_SOULBURN = 0x177,
    SKILL_WS_CARTBOOST = 0x183,
    SKILL_ST_CHASEWALK = 0x185,
    SKILL_MOONLIGHT = 0x18b,
    SKILL_CG_MARIONETTE = 0x18c,
    SKILL_TK_RUN = 0x19b,
    SKILL_BREAKFALL = 0x1a4,
    SKILL_TK_SEVENWIND = 0x1a9,
    SKILL_TK_HIGHJUMP = 0x1aa,
    SKILL_SG_SUN_WARM = 0x1ac,
    SKILL_SG_MOON_WARM = 0x1ad,
    SKILL_SG_STAR_WARM = 0x1ae,
    SKILL_SG_SUN_COMFORT = 0x1af,
    SKILL_SG_MOON_COMFORT = 0x1b0,
    SKILL_SG_STAR_COMFORT = 0x1b1,
    SKILL_SG_HATE = 0x1b2,
    SKILL_SG_FUSION = 0x1bc,
    SKILL_AM_BERSERKPITCHER = 0x1be,
    SKILL_SL_KAIZEL = 0x1ce,
    SKILL_SL_KAAHI = 0x1cf,
    SKILL_SL_KAUPE = 0x1d0,
    SKILL_SL_KAITE = 0x1d1,
    SKILL_SL_SWOO = 0x1d6,
    SKILL_SL_SKE = 0x1d7,
    SKILL_SL_SKA = 0x1d8,
    SKILL_ST_FULLSTRIP = 0x1dc,
    SKILL_CR_FULLPROTECTION = 0x1df,
    SKILL_PF_DOUBLECASTING = 0x1e2,
    SKILL_WS_OVERTHRUSTMAX = 0x1e6,
    SKILL_HERMODE = 0x1e8,
    SKILL_CG_TAROTCARD = 0x1e9,
    SKILL_TK_MISSION = 0x1ed,
    SKILL_KN_ONEHAND = 0x1ef,
    SKILL_FLIP = 0x1f4,
    SKILL_CRACKER = 0x1fc,
    SKILL_GS_DISARM = 0x201,
    SKILL_TATAMI = 0x20f,
    SKILL_MB_M_TELEPORT = 0x22b,
    ACTOR_MSG_GUARD_EFFECT = 0x7e,
};

static void target_mark_look(struct GameActor *target)
{
    Vec3 at = aid_as_pos(target);

    effect_place(actor_launch_effect(target, 0x1f7), at);
    actor_restart_effect(target, 0x1a8);
}

static int is_target_mark_skill(int skill_id)
{
    if (skill_id >= 0x23c && skill_id <= 0x23f)
        return 1;
    switch (skill_id) {
    case 0x1bd: case 0x1bf: case 0x1c0: case 0x1c1: case 0x1c2: case 0x1c3: case 0x1c4: case 0x1c5:
    case 0x1c6: case 0x1c7: case 0x1c8: case 0x1c9: case 0x1ca: case 0x1cc: case 0x1cd: case 0x1ee:
        return 1;
    }
    return 0;
}

static void caster_to_target_look(struct GameActor *caster, struct GameActor *target, int effect_id, int effect_2,
                                  int effect_3)
{
    Vec3 at = aid_as_pos(target);

    effect_place(actor_launch_effect(caster, effect_id), at);
    actor_launch_effect(caster, effect_2);
    actor_launch_effect(target, effect_2);
    if (effect_3) {
        actor_launch_effect(caster, effect_3);
        actor_launch_effect(target, effect_3);
    }
}

static void wind_release_look(const struct SkillCast *cast, struct GameActor *caster)
{
    int by_level = cast->effect_level + 0x1d2;

    play_skill_wave("effect\\T_바람방출.wav");
    actor_remove_all_effects(caster, 0x1cd);
    actor_launch_effect(caster, 0x1cd);
    actor_remove_all_effects(caster, by_level);
    actor_launch_effect(caster, by_level);
}

int skill_use_second(const struct SkillCast *cast)
{
    struct GameActor *caster = cast->caster, *target = cast->target;
    int i;

    for (i = 0; i < (int)(sizeof(songs) / sizeof(songs[0])); i++) {
        if (songs[i].skill_id == cast->skill_id) {
            play_skill_wave(songs[i].wave);
            caster_effect(cast, songs[i].effect_id);
            return 1;
        }
    }
    if (play_simple_look(cast, second_looks, SIMPLE_LOOK_COUNT(second_looks)))
        return 1;
    if (is_target_mark_skill(cast->skill_id)) {
        if (target)
            target_mark_look(target);
        return 1;
    }

    switch (cast->skill_id) {
    case SKILL_BA_FROSTJOKE:
        if (!caster)
            return 1;
        render_object_set_script_step_frames((struct RenderObject *)caster, 0xa);
        render_object_play_motion_range((struct RenderObject *)caster, 0x60, 4, 5, 0x60, 4, 1, 1800, 1);
        if (caster == (struct GameActor *)cast->mode->world->player)
            actor_launch_effect(caster, 0x127);
        return 1;

    case SKILL_DC_SCREAM:
        if (!caster)
            return 1;
        render_object_play_motion_range((struct RenderObject *)caster, 0x60, 0, 0, 0x60, 0, 1, 1200, 0);
        if (caster == (struct GameActor *)cast->mode->world->player)
            actor_launch_effect(caster, 0x128);
        return 1;

    case SKILL_WE_MALE:
        skill_use_heal(cast);
        return 1;

    case SKILL_WE_FEMALE:
        if (!target)
            return 1;
        actor_launch_effect(target, 0xfd);
        actor_show_number(target, cast->level, 0xff0000ff);
        return 1;

    case SKILL_NPC_POWERUP:
        if (!caster)
            return 1;
        play_skill_wave("effect\\mon_폭기.wav");
        actor_launch_effect(caster, 0x1c7);
        return 1;

    case SKILL_AURA_BLADE:
        if (!caster)
            return 1;
        actor_restart_effect(caster, 0x16f);
        actor_restart_effect(caster, 0x1da);
        render_object_play_motion_range((struct RenderObject *)caster, 0x20, 0, 5, 0x20, 5, 1, 1000, 0);
        play_skill_wave("effect\\오라 블레이드.wav");
        return 1;

    case SKILL_LK_PARRYING:
        if (caster)
            actor_send(caster, ACTOR_MSG_GUARD_EFFECT);
        return 1;

    case SKILL_LK_CONCENTRATION:
        if (caster)
            actor_restart_effect(caster, 0x171);
        return 1;

    case SKILL_BERSERK:
    case SKILL_LK_FURY:
        if (!caster)
            return 1;
        actor_restart_effect(caster, 0x170);
        send_quake();
        play_skill_wave("effect\\버서크.wav");
        return 1;

    case SKILL_HP_ASSUMPTIO:
        if (!target)
            return 1;
        actor_launch_effect(target, 0x177);
        actor_launch_effect(target, 0x1b8);
        return 1;

    case SKILL_HW_SOULDRAIN: {
        struct RagEffect *effect;

        if (!caster || !target)
            return 1;
        render_object_play_motion_range((struct RenderObject *)caster, 0x58, 0, 3, 0x58, 3, 1, 1000, 0);
        effect = actor_launch_effect(caster, 0x17a);
        if (effect) {
            Vec3 at = actor_pos(target);
            effect_set_pos(effect, &at);
        }
        actor_launch_effect(target, 0x17b);
        return 1;
    }

    case SKILL_HW_MAGICPOWER:
        if (!target)
            return 1;
        actor_restart_effect(target, 0x17e);
        play_skill_wave(WAVE_MAGIC_AMPLIFICATION);
        return 1;

    case SKILL_PF_SOULCHANGE:
        if (caster && target)
            caster_to_target_look(caster, target, 0x180, 0x181, 0x197);
        return 1;

    case SKILL_PF_SOULBURN:
        if (!target)
            return 1;
        actor_launch_effect(target, 0x196);
        actor_launch_effect(target, 0x17c);
        return 1;

    case SKILL_WS_CARTBOOST:
        if (!caster)
            return 1;
        play_skill_wave(WAVE_INC_AGILITY);
        actor_launch_effect(caster, 0x187);
        return 1;

    case SKILL_ST_CHASEWALK:
        if (caster)
            actor_send(caster, ACTOR_MSG_SMOKE);
        return 1;

    case SKILL_MOONLIGHT:
        if (caster && !able_to_make_effect_is_effect_exist(caster, 0x18a))
            actor_launch_effect(caster, 0x18a);
        play_skill_wave("effect\\달빛.wav");
        return 1;

    case SKILL_CG_MARIONETTE:
        if (caster && target)
            caster_to_target_look(caster, target, 0x18b, 0x18c, 0);
        return 1;

    case SKILL_TK_RUN:
        if (!caster)
            return 1;
        if (able_to_make_effect_is_effect_exist(caster, 0x1bb))
            actor_launch_effect(caster, 0x1bc);
        else if (!able_to_make_effect_is_effect_exist(caster, 0x1ba))
            actor_launch_effect(caster, 0x1ba);
        return 1;

    case SKILL_BREAKFALL:
        play_skill_wave_at("effect\\T_낙법.wav", 0.0f, -150.0f, 0.0f);
        return 1;

    case SKILL_TK_SEVENWIND:
        if (caster)
            wind_release_look(cast, caster);
        return 1;

    case SKILL_TK_HIGHJUMP:
        if (!caster)
            return 1;
        if (able_to_make_effect_is_effect_exist(caster, 0x1be))
            actor_remove_effect(caster, 0x1be);
        actor_launch_effect(caster, 0x1be);
        actor_remove_all_effects(caster, 0x1bd);
        return 1;

    case SKILL_SG_SUN_WARM:
    case SKILL_SG_MOON_WARM:
    case SKILL_SG_STAR_WARM:
        if (!caster || !target)
            return 1;
        actor_restart_effect(caster, 0x1a2);
        actor_restart_effect(caster, 0x220);
        actor_launch_effect(caster, 0x23c);
        return 1;

    case SKILL_SG_SUN_COMFORT:
    case SKILL_SG_MOON_COMFORT:
    case SKILL_SG_STAR_COMFORT:
        if (!caster)
            return 1;
        actor_restart_effect(caster, 0x1e8);
        actor_launch_effect(caster, 0x21f);
        return 1;

    case SKILL_SG_HATE:
        if (!target)
            return 1;
        play_skill_wave("effect\\T_등록.wav");
        actor_restart_effect(target, 0x21f);
        return 1;

    case SKILL_SG_FUSION:
        if (!caster)
            return 1;
        play_skill_wave("effect\\T_변신.wav");
        send_quake();
        return 1;

    case SKILL_AM_BERSERKPITCHER:
        if (!caster || !target)
            return 1;
        game_mode_launch_effect_at(cast->mode, 0x21d, caster, target, 4);
        actor_launch_effect(target, 0xdc);
        return 1;

    case SKILL_SL_KAIZEL:
        if (!target)
            return 1;
        play_skill_wave(WAVE_RESURRECTION);
        actor_launch_effect(target, 0x21f);
        actor_launch_effect(target, 0x24e);
        return 1;

    case SKILL_SL_KAAHI:
        if (!target)
            return 1;
        play_skill_wave("effect\\T_보조마법.wav");
        actor_launch_effect(target, 0x21f);
        return 1;

    case SKILL_SL_KAUPE:
        if (!target)
            return 1;
        play_skill_wave("effect\\T_치잉.wav");
        actor_launch_effect(target, 0x21e);
        return 1;

    case SKILL_SL_KAITE:
        if (!target)
            return 1;
        play_skill_wave("effect\\T_마법반사.wav");
        actor_restart_effect(target, 0x1a3);
        actor_launch_effect(target, 0x21e);
        return 1;

    case SKILL_SL_SWOO:
        if (!target)
            return 1;
        play_skill_wave_at("effect\\T_슈웃.wav", 0.0f, -50.0f, 0.0f);
        actor_restart_effect(target, 0x1a4);
        actor_launch_effect(target, 0x24d);
        return 1;

    case SKILL_SL_SKE:
        if (!target)
            return 1;
        play_skill_wave("effect\\T_공격력.wav");
        actor_restart_effect(target, 0x1ab);
        return 1;

    case SKILL_SL_SKA:
        if (!target)
            return 1;
        play_skill_wave("effect\\T_방어형.wav");
        send_quake();
        actor_remove_effect(target, 0xfe);
        actor_remove_effect(target, 0x46c);
        actor_launch_effect(target, 0xfe);
        actor_launch_effect(target, 0x46c);
        actor_launch_effect(target, 0x105);
        return 1;

    case SKILL_ST_FULLSTRIP: {
        struct RagEffect *effect;
        Vec3 at;

        if (!target)
            return 1;
        effect = actor_launch_effect(target, 0x1ef);
        if (!effect)
            return 1;
        at = actor_pos(target);
        effect_set_pos(effect, &at);
        return 1;
    }

    case SKILL_CR_FULLPROTECTION:
        if (!target)
            return 1;
        actor_launch_effect(target, 0x12c);
        actor_launch_effect(target, 0x1f4);
        return 1;

    case SKILL_PF_DOUBLECASTING:
        if (!target)
            return 1;
        actor_launch_effect(target, 0x209);
        play_skill_wave(WAVE_MAGIC_AMPLIFICATION);
        return 1;

    case SKILL_WS_OVERTHRUSTMAX:
        if (caster)
            actor_restart_effect(caster, 0x80);
        return 1;

    case SKILL_HERMODE:
        play_skill_wave("effect\\헤르모드의 지팡이.wav");
        return 1;

    case SKILL_CG_TAROTCARD:
        if (!target)
            return 1;
        play_skill_wave(WAVE_SLOW_POISON);
        actor_launch_effect(target, 0x1f4);
        return 1;

    case SKILL_TK_MISSION:
        play_skill_wave_at("effect\\T_피링.wav", 0.0f, -100.0f, 0.0f);
        return 1;

    case SKILL_KN_ONEHAND:
        if (caster)
            actor_restart_effect(caster, 0x82);
        return 1;

    case SKILL_FLIP:
        play_skill_wave("effect\\플립.wav");
        return 1;

    case SKILL_CRACKER:
        play_skill_wave("effect\\크래커.wav");
        return 1;

    case SKILL_GS_DISARM: {
        struct RagEffect *effect;
        Vec3 at;

        if (!target)
            return 1;
        effect = actor_launch_effect(target, 0x273);
        if (!effect)
            return 1;
        at = actor_pos(target);
        effect_set_pos(effect, &at);
        return 1;
    }

    case SKILL_TATAMI:
        play_skill_wave("effect\\다다미뒤집기.wav");
        return 1;

    case SKILL_MB_M_TELEPORT:
        if (caster)
            actor_send(caster, ACTOR_MSG_SMOKE);
        if (!target)
            return 1;
        actor_send(target, ACTOR_MSG_SMOKE);
        actor_launch_effect(target, 0x158);
        return 1;
    }
    return 0;
}
