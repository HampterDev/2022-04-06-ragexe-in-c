#include "effects/rag/rag_effect.h"
#include "game/skill_use.h"
#include "effects/effect_controls.h"
#include <stdint.h>

enum {
    EF_SERVANT_WEAPON = 0x69e,
    EFFECT_MSG_PRESET = 0x2c,
};

static const char WAVE_UNLIMIT[] = "effect\\ra_unlimit.wav";
static const char WAVE_TELEKINESIS_INTENSE[] = "effect\\Wl_Telekinesis_Intense.wav";
static const char WAVE_SANCTUARY[] = "effect\\priest_sanctuary.wav";
static const char WAVE_CHARGING_PIERCE[] = "effect\\DK_CHARGINGPIERCE2.wav";

static const struct SimpleLook late_looks[] = {
    { 0x138d, 0x414, ON_CASTER }, { 0x1390, 0x416, ON_CASTER }, { 0x1391, 0x413, ON_TARGET },
    { 0x1393, 0x417, ON_CASTER }, { 0x1396, 0x412, ON_CASTER }, { 0x13cc, 0x350, ON_CASTER },
    { 0x13cf, 0x412, ON_CASTER }, { 0x13d0, 0x055, ON_TARGET },
};

static const struct {
    unsigned short skill_id;
    unsigned short effect_id;
} target_place_looks[] = {
    { 0x139b, 0x451 }, { 0x139d, 0x44f }, { 0x13a8, 0x453 }, { 0x13a9, 0x455 }, { 0x13ab, 0x454 },
    { 0x13ae, 0x44e }, { 0x13b1, 0x44a }, { 0x13b5, 0x467 }, { 0x13b7, 0x466 }, { 0x13b9, 0x463 },
    { 0x13ba, 0x463 }, { 0x13bf, 0x464 },
};

enum {
    SKILL_RL_MASS_SPIRAL = 0x9f9,
    SKILL_RL_QD_SHOT = 0x9ff,
    SKILL_RL_P_ALTER = 0xa03,
    SKILL_RL_FALLEN_ANGEL = 0xa04,
    SKILL_RL_R_TRIP = 0xa05,
    SKILL_RL_B_FLICKER_ATK = 0xa0d,
    SKILL_SJ_STAREMPEROR = 0xa15,
    SKILL_SJ_PROMINENCEKICK = 0xa21,
    SKILL_SP_SOULGOLEM = 0xa24,
    SKILL_SP_SOULFAIRY = 0xa27,
    SKILL_SP_SOULCURSE = 0xa29,
    SKILL_SP_SOULUNITY = 0xa2d,
    SKILL_SP_SOULREVOLVE = 0xa30,
    SKILL_SP_KAUTE = 0xa34,
    SKILL_KO_HUUMARANKA = 0xbc1,
    SKILL_UNLIMIT = 0x138a,
    SKILL_WM_FRIGG_SONG = 0x138f,
    SKILL_SC_ESCAPE = 0x1392,
    SKILL_TELEKINESIS_INTENSE = 0x1394,
    SKILL_SU_LOPE = 0x139f,
    SKILL_SU_LUNATICCARROTBEAT = 0x13ac,
    SKILL_SU_BUNCHOFSHRIMP = 0x13b0,
    SKILL_SU_LUNATICCARROTBEAT2 = 0x13b3,
    SKILL_WE_ONEFOREVER = 0x13c8,
    SKILL_WE_CHEERUP = 0x13c9,
    SKILL_NV_HELPANGEL = 0x13d4,
    SKILL_DK_SERVANT_W_PHANTOM = 0x1454,
    SKILL_DK_SERVANT_W_DEMOL = 0x1455,
    SKILL_CHARGING_PIERCE = 0x1456,
    SKILL_DK_HACKANDSLASHER = 0x1458,
    SKILL_DK_DRAGONIC_AURA = 0x145a,
    SKILL_DK_VIGOR = 0x145c,
    SKILL_DK_STORMSLASH = 0x145d,
    SKILL_AG_DEADLY_PROJECTION = 0x145e,
    SKILL_AG_DESTRUCTIVE_HURRICANE = 0x145f,
    SKILL_AG_RAIN_OF_CRYSTAL = 0x1460,
    SKILL_AG_SOUL_VC_STRIKE = 0x1464,
    SKILL_AG_CRYSTAL_IMPACT = 0x1469,
    SKILL_AG_ASTRAL_STRIKE = 0x146e,
};

static void spawn_pair_on(struct GameActor *actor, int effect_1, int effect_2)
{
    Vec3 origin = { 0.0f, 0.0f, 0.0f };

    spawn_effect(actor->aid, effect_1, origin);
    spawn_effect(actor->aid, effect_2, origin);
}

static void wave_from(const char *wave, struct GameActor *actor, struct Player *player)
{
    if (actor && player)
        play_skill_wave_at(wave, actor->pos.x - player->pos.x, 0.0f, actor->pos.z - player->pos.z);
}

static void target_mark_by_skill(const struct SkillCast *cast, struct GameActor *target)
{
    static const unsigned short by_skill[4] = { 0x4cc, 0x4d3, 0x4d5, 0x4d4 };
    Vec3 origin = { 0.0f, 0.0f, 0.0f };
    struct RagEffect *effect;
    unsigned which;

    effect = actor_launch_effect(target, 0x1f7);
    if (effect) {
        Vec3 ids = aid_as_pos(target);
        effect_set_pos(effect, &ids);
    }
    actor_restart_effect(target, 0x1a8);
    which = (unsigned)(cast->skill_id - SKILL_SP_SOULGOLEM);
    if (which > 3)
        return;
    spawn_effect(target->aid, by_skill[which], origin);
}

static void square_look(const struct SkillCast *cast, struct GameActor *caster)
{
    float step = (float)cast->mode->world->attr->zoom * (float)(cast->level + 2);
    int row, col;

    if (!effect_mgr_is_registered(g_effectMgr, 0x423))
        return;
    for (row = 1; row >= -1; row--) {
        float dz = (float)row * step;

        for (col = -1; col <= 1; col++) {
            Vec3 at = actor_pos(caster);

            at.x = (float)col * step + at.x;
            at.z = dz + at.z;
            spawn_effect(0, 0x423, at);
        }
    }
}

static void spend_servant_weapons(const struct SkillCast *cast, struct GameActor *caster)
{
    struct Effect *effect = effect_mgr_find_effect(g_effectMgr, caster->aid, EF_SERVANT_WEAPON);
    int left;

    if (!effect)
        return;
    for (left = servant_weapon_effect_count_armed(effect); left > 0; left--)
        servant_weapon_effect_fire(effect, cast->skill_id);
}

int skill_use_late(const struct SkillCast *cast)
{
    struct GameActor *caster = cast->caster, *target = cast->target;
    Vec3 origin = { 0.0f, 0.0f, 0.0f };
    int i;

    if (play_simple_look(cast, late_looks, SIMPLE_LOOK_COUNT(late_looks)))
        return 1;
    for (i = 0; i < (int)(sizeof(target_place_looks) / sizeof(target_place_looks[0])); i++) {
        if (target_place_looks[i].skill_id == cast->skill_id) {
            if (target)
                spawn_effect(target->aid, target_place_looks[i].effect_id, actor_pos(target));
            return 1;
        }
    }

    switch (cast->skill_id) {
    case SKILL_RL_MASS_SPIRAL:
        if (target)
            spawn_effect(0, 0x427, actor_pos(target));
        return 1;

    case SKILL_RL_QD_SHOT:
        if (target)
            spawn_effect(0, 0x434, actor_pos(target));
        return 1;

    case SKILL_RL_P_ALTER:
        if (caster)
            spawn_effect(caster->aid, 0x435, origin);
        return 1;

    case SKILL_RL_FALLEN_ANGEL:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x1be);
        actor_remove_all_effects(caster, 0x430);
        return 1;

    case SKILL_RL_R_TRIP:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x27d);
        spawn_effect(0, 0x1b3, actor_pos(caster));
        return 1;

    case SKILL_RL_B_FLICKER_ATK: {
        struct GameActor *unit = (struct GameActor *)world_get_skill_by_aid(cast->mode->world, cast->target_aid);

        if (unit)
            spawn_effect(0, 0x432, actor_pos(unit));
        return 1;
    }

    case SKILL_SJ_STAREMPEROR:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x1be);
        spawn_effect(caster->aid, 0x4b3, origin);
        actor_remove_all_effects(caster, 0x1bd);
        return 1;

    case SKILL_SJ_PROMINENCEKICK:
        if (target)
            spawn_effect(target->aid, 0x4d7, origin);
        return 1;

    case SKILL_SP_SOULGOLEM: case 0xa25: case 0xa26: case SKILL_SP_SOULFAIRY:
        if (target)
            target_mark_by_skill(cast, target);
        return 1;

    case SKILL_SP_SOULCURSE:
        if (target)
            spawn_effect(target->aid, 0x4d6, origin);
        return 1;

    case SKILL_SP_SOULUNITY:
        if (!target)
            return 1;
        if (caster) {
            Vec3 ids = aid_as_pos(caster);

            effect_place(actor_launch_effect(target, 0x4ba), ids);
        }
        actor_launch_effect(target, 0x197);
        return 1;

    case SKILL_SP_SOULREVOLVE:
        if (!target)
            return 1;
        spawn_effect(target->aid, 0x4db, origin);
        wave_from("effect\\t_방어형.wav", target, cast->mode->world->player);
        return 1;

    case SKILL_SP_KAUTE:
        if (caster)
            actor_launch_effect(caster, 0x4b8);
        if (!target)
            return 1;
        wave_from("effect\\흡기.wav", target, cast->mode->world->player);
        actor_launch_effect(target, 0x4b7);
        return 1;

    case SKILL_KO_HUUMARANKA:
        if (!target)
            return 1;
        spawn_effect(target->aid, 0x3d8, origin);
        spawn_effect(target->aid, 0x3e8, origin);
        spawn_effect(target->aid, 0x3ea, origin);
        return 1;

    case SKILL_UNLIMIT:
        if (!caster)
            return 1;
        wave_from(WAVE_UNLIMIT, caster, game_current_player());
        spawn_pair_on(caster, 0x674, 0x675);
        return 1;

    case SKILL_WM_FRIGG_SONG:
        if (caster)
            square_look(cast, caster);
        return 1;

    case SKILL_SC_ESCAPE: {
        Vec3 at;
        struct RagEffect *effect;

        if (!caster || able_to_make_effect_is_effect_exist(caster, 0x439))
            return 1;
        at = actor_pos(caster);
        effect = actor_launch_effect(caster, 0x439);
        if (effect)
            rag_effect_send_msg(effect, NULL, EFFECT_MSG_PRESET, 0xffff, 0x422, (int64_t)(int)&at);
        return 1;
    }

    case SKILL_TELEKINESIS_INTENSE: {
        struct Player *player;

        if (!caster)
            return 1;
        player = game_current_player();
        if (player)
            play_skill_wave_at(WAVE_TELEKINESIS_INTENSE, player->pos.x - caster->pos.x, 0.0f,
                               player->pos.z - caster->pos.z);
        return 1;
    }

    case SKILL_SU_LOPE:
        if (!caster)
            return 1;
        actor_launch_effect(caster, 0x1be);
        actor_remove_all_effects(caster, 0x1bd);
        actor_launch_effect(caster, 0x19b);
        return 1;

    case SKILL_SU_LUNATICCARROTBEAT:
        if (target)
            effect_mgr_spawn(g_effectMgr, 0, 0x44b, actor_pos(target), 0, cast->level, 0, 0);
        return 1;

    case SKILL_SU_BUNCHOFSHRIMP:
        if (caster)
            spawn_effect(caster->aid, 0x45c, actor_pos(caster));
        return 1;

    case SKILL_SU_LUNATICCARROTBEAT2:
        if (target)
            effect_mgr_spawn(g_effectMgr, 0, 0x457, actor_pos(target), 0, cast->level, 0, 0);
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

    case SKILL_DK_SERVANT_W_PHANTOM:
    case SKILL_DK_SERVANT_W_DEMOL:
        if (caster)
            spend_servant_weapons(cast, caster);
        return 1;

    case SKILL_CHARGING_PIERCE:
        if (!caster)
            return 1;
        spawn_effect(caster->aid, 0x6e5, actor_pos(caster));
        spawn_effect(caster->aid, 0x6e6, actor_pos(caster));
        wave_from(WAVE_CHARGING_PIERCE, caster, cast->mode->world->player);
        return 1;

    case SKILL_DK_HACKANDSLASHER:
        if (caster)
            spawn_pair_on(caster, 0x67d, 0x67e);
        return 1;

    case SKILL_DK_DRAGONIC_AURA: {
        struct Effect *effect;

        if (!caster || !target)
            return 1;
        spawn_pair_on(caster, 0x6ba, 0x6bb);
        effect = spawn_effect(target->aid, 0x6bc, origin);
        if (effect)
            spawned_effect_delay(effect, 100);
        effect = spawn_effect(target->aid, 0x6bd, origin);
        if (effect)
            spawned_effect_delay(effect, 100);
        return 1;
    }

    case SKILL_DK_VIGOR:
        if (caster)
            spawn_effect(caster->aid, 0x6d6, origin);
        return 1;

    case SKILL_DK_STORMSLASH: {
        struct Effect *effect;
        int delay;

        if (!caster)
            return 1;
        delay = (int)(caster->motion_delay * 50.0f + 30.0f);
        effect = spawn_effect(caster->aid, 0x685 + cast->level, origin);
        if (effect)
            spawned_effect_delay(effect, delay);
        return 1;
    }

    case SKILL_AG_DEADLY_PROJECTION:
    case SKILL_AG_SOUL_VC_STRIKE:
        if (caster)
            render_object_play_motion_range((struct RenderObject *)caster, 0x60, 1, 1, 0x60, 1, 1, 500, 0);
        return 1;

    case SKILL_AG_DESTRUCTIVE_HURRICANE:
        spawn_pair_on(caster, 0x6c4, 0x6c5);
        return 1;

    case SKILL_AG_RAIN_OF_CRYSTAL:
    case SKILL_AG_ASTRAL_STRIKE:
        if (caster)
            render_object_play_motion_range((struct RenderObject *)caster, 0x60, 1, 3, 0x60, 3, 1, 500, 0);
        return 1;

    case SKILL_AG_CRYSTAL_IMPACT:
        if (target)
            spawn_pair_on(target, 0x73e, 0x73f);
        return 1;
    }
    return 0;
}
