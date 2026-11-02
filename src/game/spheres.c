#include "game/packet_handlers.h"
#include "effects/rag/rag_effect.h"
#include "actor/render_object.h"
#include "session/state.h"
#include "game/gamemode.h"
#include "game/session_bridge.h"
#include "game/actor_packets.h"
#include "game/ui_bridge.h"
#include "effects/effect_controls.h"
#include "effects/rag/rag_effect_types.h"
#include "actor/actor_common.h"
#include "actor/game_actor.h"
#include "session/session_names.h"
#include "session/session_skills.h"
#include "world/world.h"

enum {
    EF_MILLENNIUMSHIELD = 0xe4,
    EF_SPHERES_BABY_MONK = 0x1f8,
    EF_COINS = 0x275,
    EF_TARGET_SPIRITS = 0x2a5,
    EF_REBELLION_COINS = 0x46f,
    EF_SOUL_ENERGY = 0x4c9,
    EFFECT_MSG_SET_COUNT = 0x0e,
};

enum {
    JOB_GUNSLINGER = 24,
    JOB_BABY_MONK = 4038,
    JOB_REBELLION = 4215,
    JOB_BABY_GUNSLINGER = 4228,
    JOB_BABY_REBELLION = 4229,
    JOB_SOUL_REAPER = 4240,
    JOB_BABY_SOUL_REAPER = 4242,
};

static void effect_refresh(struct Effect *effect)
{
    effect->ops->init(effect);
}

#pragma pack(push, 1)
struct PacketMillenniumShield {
    unsigned short packet_type;
    int aid;
    short num;
    unsigned short state;
};
#pragma pack(pop)

void game_mode_on_millennium_shield(struct GameMode *self, const struct PacketMillenniumShield *packet)
{
    struct GameActor *actor;
    void *effect;
    Vec3 at;
    float count[3];

    if (g_session1.effectOn == 0)
        return;
    actor = world_get_game_actor_by_aid(self->world, packet->aid);
    if (g_session1.ownAid == packet->aid)
        actor = (struct GameActor *)self->world->player;
    if (actor == 0)
        return;
    if (is_effect_state_burrow(actor_get_effect_state(actor)) == 1 || is_effect_state_star2(actor_get_effect_state(actor)) == 1)
        render_object_send(actor, NULL, ACTOR_MSG_REMOVE_EFFECT, EF_MILLENNIUMSHIELD, 0, 0, 0);

    if (packet->state == 0) {
        render_object_send(actor, NULL, ACTOR_MSG_REMOVE_EFFECT, EF_MILLENNIUMSHIELD, 0, 0, 0);
    } else if (packet->state == 1) {
        if (able_to_make_effect_is_effect_exist(actor, EF_MILLENNIUMSHIELD))
            return;
    } else {
        return;
    }
    at.x = at.y = at.z = 0.0f;
    effect = game_actor_launch_effect(actor, EF_MILLENNIUMSHIELD, at, 0.0f);
    if (effect == 0)
        return;
    count[0] = (float)packet->num;
    count[1] = 0.0f;
    count[2] = 0.0f;
    rag_effect_send_msg(effect, NULL, EFFECT_MSG_SET_COUNT, MSG_ARG_PTR(count), 0, 0);
}

static void remove_effect(struct GameActor *actor, int effect_id)
{
    render_object_send(actor, NULL, ACTOR_MSG_REMOVE_EFFECT, effect_id, 0, 0, 0);
}

static int is_hidden(struct GameActor *actor)
{
    return is_effect_state_burrow(actor_get_effect_state(actor)) || is_effect_state_star2(actor_get_effect_state(actor));
}

static struct Effect *running_effect(int aid, int effect_id)
{
    struct Effect *effect = effect_mgr_find_effect(g_effectMgr, aid, effect_id);

    if (effect == 0) {
        Vec3 at = { 0.0f, 0.0f, 0.0f };
        effect = effect_mgr_spawn(g_effectMgr, aid, effect_id, at, 0, -1, 0, 0);
    }
    return effect;
}

static void launch_spheres(struct GameActor *actor, int effect_id, short num)
{
    Vec3 at = { 0.0f, 0.0f, 0.0f };
    struct RagEffect *effect = game_actor_launch_effect(actor, effect_id, at, 0.0f);
    float count[3];

    if (effect == 0)
        return;
    if (effect->effect_id != EF_MILLENNIUMSHIELD && effect->effect_id != EF_SPHERES_BABY_MONK
        && effect->effect_id != EF_COINS)
        return;
    count[0] = (float)num;
    count[1] = 0.0f;
    count[2] = 0.0f;
    rag_effect_send_msg(effect, NULL, EFFECT_MSG_SET_COUNT, MSG_ARG_PTR(count), 0, 0);
}

static void show_rebellion_coins(struct GameActor *actor, short num)
{
    Vec3 at;
    struct Effect *effect;

    at.x = actor->pos.x;
    at.y = actor->pos.y;
    at.z = actor->pos.z;
    effect = effect_mgr_spawn(g_effectMgr, actor->aid, EF_REBELLION_COINS, at, 0, -1, 0, 0);
    if (effect) {
        richs_coin_effect_set_coin_count(effect, num);
        effect_refresh(effect);
    }
}

void game_mode_on_spirits(struct GameMode *self, int aid, short num)
{
    struct GameActor *actor;
    int job;

    if (aid == 0)
        return;
    actor = world_get_actor_or_player(self->world, aid);
    if (actor == 0)
        return;
    job = game_actor_get_job(actor);

    switch (job) {
    case JOB_BABY_MONK:
        remove_effect(actor, EF_SPHERES_BABY_MONK);
        break;
    case JOB_GUNSLINGER:
    case JOB_BABY_GUNSLINGER:
        remove_effect(actor, EF_COINS);
        break;
    case JOB_REBELLION:
    case JOB_BABY_REBELLION:
        effect_mgr_remove_type(g_effectMgr, actor->aid, EF_REBELLION_COINS);
        break;
    default:
        remove_effect(actor, EF_MILLENNIUMSHIELD);
        break;
    }
    if (num == 0)
        return;

    switch (job) {
    case JOB_BABY_MONK:
        launch_spheres(actor, EF_SPHERES_BABY_MONK, num);
        break;
    case JOB_GUNSLINGER:
    case JOB_BABY_GUNSLINGER:
        launch_spheres(actor, EF_COINS, num);
        break;
    case JOB_REBELLION:
    case JOB_BABY_REBELLION:
        show_rebellion_coins(actor, num);
        break;
    default:
        launch_spheres(actor, EF_MILLENNIUMSHIELD, num);
        break;
    }
}

#pragma pack(push, 1)
struct PacketSpirits2 {
    unsigned short packet_type;
    int aid;
    short num;
};
#pragma pack(pop)

void game_mode_on_spirits2(struct GameMode *self, const struct PacketSpirits2 *packet)
{
    struct GameActor *actor;
    Vec3 at = { 0.0f, 0.0f, 0.0f };
    struct RagEffect *effect;
    float count[3];
    int effect_id;

    if (g_session1.effectOn == 0)
        return;
    if (packet->aid == g_session1.ownAid)
        actor = (struct GameActor *)self->world->player;
    else
        actor = world_get_game_actor_by_aid(self->world, packet->aid);
    if (actor == 0)
        return;

    if (is_hidden(actor)) {
        int job = game_actor_get_job(actor);

        if (job == JOB_BABY_MONK)
            effect_id = EF_SPHERES_BABY_MONK;
        else if (job == JOB_GUNSLINGER || job == JOB_REBELLION || job == JOB_BABY_GUNSLINGER
                 || job == JOB_BABY_REBELLION)
            effect_id = EF_COINS;
        else
            effect_id = EF_MILLENNIUMSHIELD;
        remove_effect(actor, effect_id);
        return;
    }

    if (game_actor_get_job(actor) == JOB_BABY_MONK)
        effect_id = EF_SPHERES_BABY_MONK;
    else if (game_actor_get_job(actor) == JOB_GUNSLINGER || game_actor_get_job(actor) == JOB_REBELLION
             || game_actor_get_job(actor) == JOB_BABY_GUNSLINGER
             || game_actor_get_job(actor) == JOB_BABY_REBELLION)
        effect_id = EF_COINS;
    else
        effect_id = EF_MILLENNIUMSHIELD;
    if (able_to_make_effect_is_effect_exist(actor, effect_id))
        return;
    if (packet->aid == 0)
        return;
    effect = game_actor_launch_effect(actor, effect_id, at, 0.0f);
    count[0] = (float)packet->num;
    count[1] = 0.0f;
    count[2] = 0.0f;
    if (effect == 0)
        return;
    rag_effect_send_msg(effect, NULL, EFFECT_MSG_SET_COUNT, MSG_ARG_PTR(count), 0, 0);
}

#pragma pack(push, 1)
struct PacketSoulEnergy {
    unsigned short packet_type;
    int aid;
    short num;
};
struct PacketSpiritsAttribute {
    unsigned short packet_type;
    int aid;
    short attribute;
    short num;
};
#pragma pack(pop)

void game_mode_on_soul_energy(struct GameMode *self, const struct PacketSoulEnergy *packet)
{
    struct GameActor *actor = world_get_actor_or_player(self->world, packet->aid);
    struct Effect *effect;
    int job;

    if (actor == 0)
        return;
    job = game_actor_get_job(actor);
    if (job != JOB_SOUL_REAPER && job != JOB_BABY_SOUL_REAPER)
        return;
    if (is_hidden(actor))
        effect_mgr_remove_type(g_effectMgr, actor->aid, EF_SOUL_ENERGY);
    effect = running_effect(actor->aid, EF_SOUL_ENERGY);
    if (effect == 0)
        return;
    effect->level = packet->num;
    effect_refresh(effect);
}

void game_mode_on_spirits_attribute(struct GameMode *self, const struct PacketSpiritsAttribute *packet)
{
    struct GameActor *actor;

    if (packet->aid == g_session1.ownAid) {
        actor = self->world ? (struct GameActor *)self->world->player : NULL;
    } else {
        actor = session_get_actor_by_aid(&g_session1, packet->aid);
    }
    if (actor)
        game_actor_show_orbs(actor, packet->attribute, packet->num);
}

void game_mode_on_target_spirits(struct GameMode *self, int aid, short num)
{
    struct GameActor *actor;
    Vec3 at = { 0.0f, 0.0f, 0.0f };

    if (aid == 0 || g_session1.effectOn == 0)
        return;
    if (aid == g_session1.ownAid)
        actor = (struct GameActor *)self->world->player;
    else
        actor = world_get_game_actor_by_aid(self->world, aid);
    if (actor == 0)
        return;
    if (game_actor_update_sphere_count(actor, num) == 1)
        game_actor_launch_effect(actor, EF_TARGET_SPIRITS, at, 0.0f);
}
