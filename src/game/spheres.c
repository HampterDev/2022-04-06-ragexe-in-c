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
    EFFECT_MSG_SET_COUNT = 0x0e,
};

enum {
    JOB_GUNSLINGER = 24,
    JOB_BABY_MONK = 4038,
    JOB_BABY_GUNSLINGER = 4228,
};

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
    rag_effect_send_msg(effect, NULL, EFFECT_MSG_SET_COUNT, (intptr_t)count, 0, 0);
}

static void remove_effect(struct GameActor *actor, int effect_id)
{
    render_object_send(actor, NULL, ACTOR_MSG_REMOVE_EFFECT, effect_id, 0, 0, 0);
}

static int is_hidden(struct GameActor *actor)
{
    return is_effect_state_burrow(actor_get_effect_state(actor)) || is_effect_state_star2(actor_get_effect_state(actor));
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
    rag_effect_send_msg(effect, NULL, EFFECT_MSG_SET_COUNT, (intptr_t)count, 0, 0);
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
        else if (job == JOB_GUNSLINGER || job == JOB_BABY_GUNSLINGER)
            effect_id = EF_COINS;
        else
            effect_id = EF_MILLENNIUMSHIELD;
        remove_effect(actor, effect_id);
        return;
    }

    if (game_actor_get_job(actor) == JOB_BABY_MONK)
        effect_id = EF_SPHERES_BABY_MONK;
    else if (game_actor_get_job(actor) == JOB_GUNSLINGER || game_actor_get_job(actor) == JOB_BABY_GUNSLINGER)
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
    rag_effect_send_msg(effect, NULL, EFFECT_MSG_SET_COUNT, (intptr_t)count, 0, 0);
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
