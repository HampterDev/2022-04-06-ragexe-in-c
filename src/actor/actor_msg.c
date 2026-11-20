#include "actor/actor_ops.h"
#include "effects/rag/rag_effect.h"
#include "session/state.h"
#include "game/context.h"
#include "game/gamemode.h"
#include "actor/actor.h"
#include "session/session_jobs.h"
#include "actor/actor_msg.h"
#include "platform/clock.h"
#include "std/swing_pace.h"

int g_multiHitInterval = 200;

int g_maxAttackDelayMs = 432;

#define SEND_HIT(target, self, hit, total) \
    render_object_send((target), (self), 0x1b, (intptr_t)hit, (total), 0, 0)

static int hit_job_mark(int job)
{
    if ((job >= 0xfce && job <= 0xfd0) || job == 0x1081 || job == 0x1082 || job == 0x108e)
        return 0x1b2;
    return 0;
}

void game_actor_msg_attack(struct GameActor *self,
                                     const struct ActorAttack *attack)
{
    struct GameMode *mode = game_current_mode();
    int type = attack->action;
    int damage = attack->damage;
    int damage2 = attack->left_damage;
    int count = attack->count;
    int cell_x = attack->cell_x;
    int cell_y = attack->cell_y;
    int attack_mt = attack->attack_mt;
    int attacked_mt = attack->attacked_mt;
    struct GameActor *target;
    float tx, tz;
    int hits, kind, flag, mark, job;
    unsigned delay;
    struct ActorHit hit;

    self->attack_target_aid = attack->target_aid;
    if (g_session1.ownAid == attack->target_aid) {
        target = (struct GameActor *)mode->world->player;
    } else {
        target = world_get_game_actor_by_aid(mode->world, attack->target_aid);
        if (!target)
            target = (struct GameActor *)world_get_skill_by_aid(mode->world, self->attack_target_aid);
    }
    if (target) {
        tx = target->pos.x;
        tz = target->pos.z;
        game_actor_get_job(target);
    } else {
        struct GameActor *player = (struct GameActor *)mode->world->player;

        tx = self->pos.x - (player->pos.x - self->pos.x);
        tz = self->pos.z - (player->pos.z - self->pos.z);
    }
    if (count <= 0 || count > 0x24)
        count = 1;
    hits = count;
    if (type != 8 && type != 9 && type != 0xd)
        hits = 1;
    damage /= hits;
    switch (type) {
    case 1:
        actor_set_state(self, 5);
        return;
    case 2:
        actor_set_state(self, 6);
        return;
    case 3:
        actor_set_state(self, 0);
        return;
    case 0: case 4: case 8: case 9: case 10: case 11: case 13:
        break;
    default:
        return;
    }

    if (self->casting) {
        Vec3 zero = { 0.0f, 0.0f, 0.0f };
        struct RagEffect *effect = game_actor_launch_effect(self, 0x83, zero, 0.0f);

        if (effect)
            rag_effect_send_msg(effect, (struct RenderObject *)self, 0x32, 0, 0, 0);
        self->casting = 0;
    }
    if (attack->impact) {
        delay = 0;
    } else {
        actor_set_attack_speed(self, attack_mt);
        if (attack->target_aid != self->aid)
            actor_set_state(self, 2);
        render_object_turn_toward((struct RenderObject *)self, tx, tz);
        delay = (unsigned)(self->attack_motion_count * self->motion_delay * 24.0f);
        if (game_actor_get_job(self) == 0x3f8 || game_actor_get_job(self) == 0x58c)
            delay += 0xc0;
        else if (game_actor_get_job(self) == 0x505 || game_actor_get_job(self) == 0x726)
            delay += 0x390;
        else if (game_actor_get_job(self) == 0x506 || game_actor_get_job(self) == 0x725 ||
                 game_actor_get_job(self) == 0x507)
            delay += 0x198;
    }

    flag = -1;
    switch (type) {
    case 4:
    case 9:
        kind = 0x27;
        break;
    case 10:
        kind = 0x5d;
        flag = 1;
        break;
    case 11:
        kind = self->casting ? 0x1a : 0x5e;
        break;
    case 13:
        kind = 0x94;
        flag = 1;
        break;
    default:
        kind = 0x1a;
        break;
    }
    if (!target)
        return;
    mark = hit_job_mark(game_actor_get_job(self));
    {
        int player_cell_x, player_cell_y;
        struct GameActor *player = (struct GameActor *)mode->world->player;

        game_mode_world_to_cell(mode, player->pos.x, player->pos.y, &player_cell_x, &player_cell_y);
    }
    hit.target_aid = self->attack_target_aid;
    hit.kind = kind;
    hit.damage = damage;
    hit.tick = platform_ticks_ms() + delay;
    hit.attacker_aid = self->aid;
    hit.cell_x = cell_x;
    hit.cell_y = cell_y;
    hit.effect = flag;
    hit.hit_effect = attack->target_aid == self->aid ? -2 : mark;
    hit.value = 1;
    hit.motion = (float)attacked_mt / 288.0f;
    hit.is_sp_damage = attack->is_sp_damage;
    hit.skill_id = 0;
    if (hits == 1)
        SEND_HIT(target, self, &hit, 0);
    else
        SEND_HIT(target, self, &hit, damage);

    job = game_actor_get_job(self);
    if (damage2 && (session_get_pet_acc_act_name(&g_session1, job) || job > 0xfa0 ||
                    job == 0xc)) {
        int total = damage;
        unsigned tick;

        if (hits > 1) {
            int half = g_multiHitInterval / 2;
            int n;

            tick = hit.tick;
            for (n = hits - 1; n; n--) {
                tick += half;
                hit.damage = damage;
                total += damage;
                hit.tick = tick;
                SEND_HIT(target, self, &hit, total);
                half = g_multiHitInterval / 2;
                tick = hit.tick - half;
            }
        } else {
            tick = hit.tick;
        }
        hit.kind = kind;
        hit.damage = damage2;
        hit.tick = tick + (g_multiHitInterval / 2 + (g_multiHitInterval / 4 + g_multiHitInterval));
        if (hits == 1)
            SEND_HIT(target, self, &hit, 0);
        else
            SEND_HIT(target, self, &hit, total + damage2);
        return;
    }
    if (hits > 1) {
        int total = damage;
        int n;

        for (n = hits - 1; n; n--) {
            hit.tick += g_multiHitInterval;
            total += damage;
            hit.damage = damage;
            SEND_HIT(target, self, &hit, total);
        }
    }
}

/* The start of a swing whose hit is not decided yet: the attack animation plays, and the hit
   comes later as an attack marked `impact`. The animation is paced so that the frame its hit is
   on shows `lands_ms` from now, which is when the swing lands (std/swing_pace.h): however long
   the server has the swing take, the attack on the screen connects when the swing does. The
   attack's setup calls game_actor_pace_attack where it has that frame, before it times anything
   by the animation (an arrow's release). */
void game_actor_swing_begin(struct GameActor *self, int target_aid, int attack_mt, int lands_ms)
{
    struct GameMode *mode = game_current_mode();
    struct GameActor *target;

    self->attack_target_aid = target_aid;
    if (g_session1.ownAid == target_aid)
        target = (struct GameActor *)mode->world->player;
    else
        target = world_get_game_actor_by_aid(mode->world, target_aid);
    actor_set_attack_speed(self, attack_mt);
    if (target_aid != self->aid) {
        self->swing_lands_ms = lands_ms;
        self->swing_own_delay = 0.0f;
        self->swing_paces = 1;
        actor_set_state(self, 2);
        self->swing_paces = 0;
    }
    if (target)
        render_object_turn_toward((struct RenderObject *)self, target->pos.x, target->pos.z);
}

/* Where the setup of an attack has the frame its hit is on (attack_motion_count): paces the
   animation of the swing the server announced, and does nothing for any other attack. A setup
   that goes through another's (a player's through the common one) calls it once for each
   frame it comes to know, and the last one holds: every call paces the delay the attack had
   before the first. */
void game_actor_pace_attack(struct GameActor *self)
{
    if (!self->swing_paces)
        return;
    if (!(self->swing_own_delay > 0.0f))
        self->swing_own_delay = self->motion_delay;
    self->motion_delay = swing_pace_frame_delay(self->swing_own_delay, self->attack_motion_count, self->swing_lands_ms);
}

/* The server dropped the swing before its impact: the attack animation ends. */
void game_actor_swing_cancel(struct GameActor *self)
{
    if (self->state == 2)
        actor_set_state(self, 0);
}

void game_actor_set_attack_speed(struct GameActor * self, int ms)
{
    float f;

    if (ms > g_maxAttackDelayMs)
        ms = g_maxAttackDelayMs;
    f = (float)ms / 432.0f;
    self->delay_factor = f;
    self->attack_delay_factor = f;
}
