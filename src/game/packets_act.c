#include <stdlib.h>
#include "game/packet_handlers.h"
#include "game/packets.h"
#include "game/mode_msg.h"
#include "session/state.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "world/actor_lookup.h"
#include "world/object_lookup.h"

#define ACTOR_MSG_ATTACK        0xb
#define DAMAGE_NONE             (-30000)
#define DEFAULT_MOTION_TIME     1440
#define PET_EMOTION_CHANCE         25
#define PET_EMOTION_INTERVAL       60000

static struct GameActor *attacker(struct GameMode *self, unsigned aid)
{
    struct GameActor *actor;

    if (aid == (unsigned)g_session1.ownAid)
        actor = (struct GameActor *)self->world->player;
    else
        actor = world_get_game_actor_by_aid(self->world, aid);
    if (!actor)
        actor = (struct GameActor *)world_get_skill_by_aid(self->world, (int)aid);
    return actor;
}

static void pet_attack_emotion(struct GameMode *self, unsigned aid)
{
    if (aid == (unsigned)g_session1.ownAid) {
        if (world_get_game_actor_by_aid(self->world, (unsigned)g_session1.aid)) {
            int hunger = session_pet_hunger_level(g_session1.petFullness);
            int intimacy = session_pet_intimacy_level(g_session1.petIntimacy);
            int emotion = session_get_pet_emotion(&g_session1, hunger, intimacy, 1);

            if (rand() % 100 < PET_EMOTION_CHANCE && session_pet_emotion_due(&g_session1, PET_EMOTION_INTERVAL) == 1) {
                mode_send_msg(self, MODE_MSG_PET_ACT, emotion, 0, 0, 0);
            }
        }
    }
}

void game_mode_notify_act(struct GameMode *self, unsigned aid, unsigned target, int start_time,
                                       int attack_mt, int attacked_mt, int damage, int count, int action,
                                       int left_damage, int is_sp_damage)
{
    struct ActorAttack attack;
    struct GameActor *actor;

    if (damage == DAMAGE_NONE)
        return;
    attack.damage = damage;
    attack.left_damage = left_damage;
    attack.action = (unsigned char)action;
    attack.target_aid = (int)target;
    attack.start_time = start_time;
    attack.count = (short)count;
    attack.cell_x = -1;
    attack.cell_y = -1;
    attack.attack_mt = attack_mt ? attack_mt : DEFAULT_MOTION_TIME;
    attack.attacked_mt = attacked_mt;
    if (attacked_mt == 0 && !session_is_attack(&g_session1, attack.action))
        attack.attacked_mt = DEFAULT_MOTION_TIME;
    attack.is_sp_damage = (unsigned char)is_sp_damage != 0;

    actor = attacker(self, aid);
    if (!actor)
        return;
    if (actor->state == 0 && (unsigned)actor->attack_target_aid != target
        && attack.action != 1 && attack.action != 2 && attack.action != 3)
        pet_attack_emotion(self, aid);
    actor_send(actor, ACTOR_MSG_ATTACK, (intptr_t)&attack, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_ACT_POSITION {
    unsigned short id;
    unsigned aid;
    unsigned target;
    int start_time;
    short x, y;
    short damage;
    short count;
    unsigned char action;
};
#pragma pack(pop)

#define ATTACK_POSITION_MT      0x1b0
#define ATTACKED_POSITION_MT    0x120

void game_mode_zc_notify_act_position(struct GameMode *self, const struct PACKET_ZC_NOTIFY_ACT_POSITION *packet)
{
    struct ActorAttack attack;
    struct GameActor *actor;

    if (packet->damage == DAMAGE_NONE)
        return;
    attack.damage = packet->damage;
    attack.left_damage = 0;
    attack.action = packet->action;
    attack.target_aid = (int)packet->target;
    attack.start_time = packet->start_time;
    attack.count = packet->count;
    attack.cell_x = packet->x;
    attack.cell_y = packet->y;
    attack.attack_mt = ATTACK_POSITION_MT;
    attack.attacked_mt = ATTACKED_POSITION_MT;
    attack.is_sp_damage = 0;
    if (packet->aid == (unsigned)g_session1.ownAid)
        actor = (struct GameActor *)self->world->player;
    else
        actor = world_get_game_actor_by_aid(self->world, packet->aid);
    if (actor)
        actor_send(actor, ACTOR_MSG_ATTACK, (intptr_t)&attack, 0);
}
