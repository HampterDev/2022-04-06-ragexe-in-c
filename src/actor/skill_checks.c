#include "actor/actor_ops.h"
#include "effects/rag/rag_effect.h"
#include "contents/sound.h"
#include "session/state.h"
#include "game/context.h"
#include "actor/actor.h"
#include "actor/game_actor.h"
#include "game/gamemode.h"
#include "effects/skills/msg_effect.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_skills.h"
#include "actor/actor_msg.h"
#include <stdint.h>
#include "platform/clock.h"

struct World;

static int skill_sends_no_hit(int skill)
{
    switch (skill) {
    case 0xb: case 0x2f: case 0x111: case 0x19d: case 0x2dc:
        return 1;
    }
    return 0;
}

static int skill_skips_cast_state(int skill, int action)
{
    switch (skill) {
    case 0x8f: case 0x107: case 0x10f: case 0x110: case 0x111: case 0x173:
        return 1;
    }
    return 0;
}

static int skill_skips_attack_speed(int skill)
{
    switch (skill) {
    case 0x1ea:
        return 1;
    }
    return 0;
}

static int skill_msg_delay(const struct GameActor *self)
{
    return (int)(int64_t)(self->attack_motion_count * self->motion_delay * 24.0f);
}

static int send_skill_hit_60(struct GameActor *self, struct GameObject *target,
                             const struct ActorSkillUse *use, int damage, int value)
{
    int skill = use->skill_id;
    struct ActorHit hit;

    if (!target)
        return skill == 0x2e3;
    if (skill_sends_no_hit(skill))
        return 0;
    hit.effect = -1;
    hit.hit_effect = -1;
    hit.target_aid = self->attack_target_aid;
    hit.kind = 0x60;
    hit.damage = damage;
    hit.tick = platform_ticks_ms() + skill_msg_delay(self);
    hit.attacker_aid = self->aid;
    hit.cell_x = use->cell_x;
    hit.cell_y = use->cell_y;
    hit.value = value;
    hit.motion = (float)use->attacked_mt / 288.0f;
    hit.is_sp_damage = 0;
    hit.skill_id = skill;
    render_object_send(target, self, 0x1b, (intptr_t)&hit, 0, 0, 0);
    return 1;
}

static int send_skill_hit_61(struct GameActor *self, struct GameObject *target,
                             const struct ActorSkillUse *use, int damage, int value)
{
    int skill = use->skill_id;
    struct ActorHit hit;

    if (!target)
        return skill == 0x2e3;
    if (skill_sends_no_hit(skill))
        return 0;
    hit.target_aid = self->attack_target_aid;
    hit.kind = 0x61;
    hit.damage = damage;
    hit.tick = platform_ticks_ms() + skill_msg_delay(self);
    hit.attacker_aid = self->aid;
    hit.cell_x = use->cell_x;
    hit.cell_y = use->cell_y;
    hit.effect = -1;
    hit.hit_effect = -1;
    hit.value = value;
    hit.motion = (float)use->attacked_mt / 288.0f;
    hit.is_sp_damage = 0;
    hit.skill_id = skill;
    render_object_send(target, self, 0x1b, (intptr_t)&hit, 0, 0, 0);
    return 1;
}

static struct RagEffect *sot_launch(void *actor, int effect)
{
    Vec3 zero = { 0.0f, 0.0f, 0.0f };
    return game_actor_launch_effect((struct GameActor *)actor, effect, zero, 0.0f);
}

static void sot_wave(const char *name, float x, float y, float z)
{
    sound_play_wave(name, x, y, z, 250, 40, 1.0f);
}

static void sot_motion(struct GameActor *self, int action, int motion, int last, int duration)
{
    render_object_play_motion_range((struct RenderObject *)self, (unsigned char)action, (unsigned char)motion,
                                   (unsigned char)last, (unsigned char)action, (unsigned char)last, 1, duration, 0);
}

static int sot_attack_duration(const struct GameActor *self)
{
    return session_find_combo_delay(&g_session1, self->aid);
}

static void sot_effect_at(struct RagEffect *e, const Vec3 *pos)
{
    rag_effect_send_msg(e, NULL, 0xe, (intptr_t)pos, 0, 0);
}

void game_actor_skill_on_target(struct GameActor *self, const struct ActorSkillUse *rec)
{
    struct GameMode *mode = game_current_mode();
    int skill = rec->skill_id, type = rec->action;
    int hits, value, per_hit, delay, hit_msg, with_damage;
    int use_effect, use_state, hit1, hit2, kind;
    struct GameActor *target;
    struct GameActor *player;
    struct RagEffect *e;
    struct ActorHit msg;
    Vec3 tpos;
    struct World *world;

    hits = rec->count;
    if (hits <= 0 || hits > 0x24)
        hits = 1;
    value = skill == 0x38 ? hits : rec->level;
    per_hit = rec->damage / hits;
    self->attack_target_aid = rec->target_aid;
    world = mode->world;

    if (g_session1.ownAid == rec->target_aid)
        target = (struct GameActor *)world->player;
    else
        target = world_get_game_actor_by_aid(world, rec->target_aid);
    if (target != NULL || g_session1.ownAid == rec->target_aid) {
        if ((skill == 0x81 || skill == 0x17d) && self->falcon != 0)
            render_object_send(self->falcon, NULL, 0x1d, (intptr_t)&target->pos, 0, 0, 0);
    } else {
        target = (struct GameActor *)world_get_skill_by_aid(mode->world, self->attack_target_aid);
    }
    if (target != NULL) {
        tpos = target->pos;
        game_actor_get_job(target);
    } else {
        player = (struct GameActor *)mode->world->player;
        tpos.x = self->pos.x - (player->pos.x - self->pos.x);
        tpos.y = self->pos.y - (player->pos.y - self->pos.y);
        tpos.z = self->pos.z - (player->pos.z - self->pos.z);
    }
    if (type != 8 && type != 9 && type != 0xd)
        hits = 1;

    switch (type) {
    case 4: case 5: case 6: case 8: case 9: case 0xd: case 0xe:
        break;
    default:
        return;
    }
    if (type == 5) {
        if (skill == 0x2dc)
            render_object_set_motion_delay((struct RenderObject *)self, (float)rec->attack_mt / 432.0f * 4.0f);
        game_mode_zc_use_skill_(&g_session1, skill, hits, self, target);
        if (send_skill_hit_60(self, (struct GameObject *)target, rec, per_hit, value))
            return;
    } else if (type == 0xe) {
        game_mode_zc_use_skill_(&g_session1, skill, hits, self, target);
        if (send_skill_hit_61(self, (struct GameObject *)target, rec, per_hit, value))
            return;
    }

    session_get_use_effect(&g_session1, skill, &use_effect, &use_state);
    if (!skill_skips_attack_speed(skill))
        actor_set_attack_speed(self, rec->attack_mt);
    if (!skill_skips_cast_state(skill, type))
        actor_set_state(self, use_state, 1);
    if (skill_info_shows_use_effect(skill) && value != -2) {
        e = sot_launch(self, use_effect);
        if (e != NULL)
            rag_effect_send_msg(e, NULL, 0x2c, hits, 0, 0);
    }
    if (type != 5)
        game_mode_zc_use_skill_(&g_session1, skill, hits, self, target);
    if (target != NULL && target != self)
        render_object_turn_toward((struct RenderObject *)self, tpos.x, tpos.z);

    delay = skill_msg_delay(self);
    if (game_actor_get_job(self) == 0x3f8 || game_actor_get_job(self) == 0x58c)
        delay += 0xc0;
    if (target == NULL)
        return;

    {
        int launch = session_get_skill_effect_id(&g_session1, skill);
        hit2 = session_get_skill_hit_effect_id(&g_session1, skill);
        if (hit2 == 0xba && value >= 2 && value <= 5)
            hit2 = 0xbb + (value - 2);
        e = sot_launch(self, launch);
        if (e != NULL) {
            rag_effect_send_msg(e, NULL, 0x2c, hits, 0, 0);
            sot_effect_at(e, &tpos);
        }
    }
    if (skill >= 0x19b && skill <= 0x1d8)
        hit1 = -1;
    else
        hit1 = skill >= 0x1ed ? -1 : 0;
    switch (skill) {
    case 0x7: case 0x11: case 0x13: hit1 = 0x31; break;
    case 0xe: case 0xf: hit1 = 0x33; break;
    case 0x14: hit1 = 0x34; break;
    case 0x36:
        if (per_hit != 0) {
            hit1 = 0x98;
            hit2 = -1;
        }
        break;
    case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x81:
    case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd:
    case 0x15b: case 0x18d: case 0x28e:
        hit1 = -1;
        break;
    case 0x5a: case 0x5b: case 0x302: hit1 = 0x93; break;
    case 0x88: hit1 = 0; break;
    case 0xaa: type = 0xa; break;
    case 0xbe: hit1 = 0xb4; break;
    case 0x1d3: case 0x1d4: hit1 = 0x225; break;
    }
    switch (type) {
    case 4: case 9: kind = 0x27; break;
    case 6: kind = 0x61; break;
    case 0xa: kind = 0x5d; break;
    case 0xd: kind = 0x94; break;
    default: kind = 0x60; break;
    }

    msg.target_aid = self->attack_target_aid;
    msg.kind = kind;
    msg.damage = per_hit;
    msg.tick = platform_ticks_ms() + delay;
    msg.attacker_aid = self->aid;
    msg.cell_x = rec->cell_x;
    msg.cell_y = rec->cell_y;
    msg.effect = hit1;
    msg.hit_effect = hit2;
    msg.value = value;
    msg.motion = (float)rec->attacked_mt / 288.0f;
    msg.is_sp_damage = 0;
    msg.skill_id = skill;
    hit_msg = 0x1b;
    with_damage = hits != 1;
    switch (skill) {
    case 0x4d:  msg.tick += 500; break;
    case 0x111: case 0x173:
        with_damage = 1;
        break;
    case 0x19f: case 0x1a1:
        msg.tick += 300;
    case 0x19d: case 0x1a3: case 0x1a5:
        hit_msg = 0x82;
        with_damage = 1;
        break;
    default:
        break;
    }
    if (with_damage)
        render_object_send(target, self, hit_msg, (intptr_t)&msg, per_hit, 0, 0);
    else
        render_object_send(target, self, hit_msg, (intptr_t)&msg, 0, 0, 0);
    msg.hit_effect = -1;

    switch (skill) {
    case 0x107:
        render_object_set_script_step_frames((struct RenderObject *)self, 4);
        sot_motion(self, 0x58, 0, 7, sot_attack_duration(self));
        break;
    case 0x10a: case 0x10b:
        render_object_set_script_step_frames((struct RenderObject *)self, 10);
        sot_motion(self, 0x60, 3, 4, 0x44c);
        break;
    case 0x10f:
        render_object_set_script_step_frames((struct RenderObject *)self, 4);
        sot_motion(self, 0x58, 0, 7, 0xdac);
        self->body_relocation = 1;
        break;
    case 0x110:
        sot_motion(self, 0x60, 5, 5, sot_attack_duration(self));
        sot_launch(target, 0x106);
        sot_launch(self, 0x107);
        sot_launch(target, 0x111);
        break;
    case 0x111:
        render_object_set_script_step_frames((struct RenderObject *)self, 6);
        sot_motion(self, 0x60, 1, 2, sot_attack_duration(self));
        game_send_message(0x7a, 0, 0, 0, 0);
        sot_wave("effect\\mon_맹룡과강.wav", 0.0f, 0.0f, 0.0f);
        sot_launch(self, 0x107);
        sot_launch(self, 0x14a);
        break;
    case 0x172:
        sot_motion(self, 0x60, 3, 4, 1);
        game_send_message(0x7a, 0, 0, 0, 0);
        sot_wave("effect\\맹호경파산.wav", 0.0f, 0.0f, 0.0f);
        sot_launch(target, 0x178);
        break;
    case 0x173:
        render_object_set_script_step_frames((struct RenderObject *)self, 6);
        sot_motion(self, 0x60, 4, 4, sot_attack_duration(self));
        game_send_message(0x7a, 0, 0, 0, 0);
        sot_launch(self, 0x179);
        sot_launch(self, 0x107);
        break;
    case 0x174:
        sot_launch(self, 0x107);
        e = sot_launch(self, 0x200);
        if (e != NULL) {
            Vec3 at = target->pos;
            sot_effect_at(e, &at);
        }
        break;
    }

    if (type == 6 || type == 8 || type == 9 || type == 0xd) {
        int i, n = hits - 1, total = per_hit;
        for (i = 0; i < n; i++) {
            unsigned at = msg.tick;
            if (skill == 0x38)
                at += 0x50;
            total += per_hit;
            msg.hit_effect = -1;
            msg.tick = at + g_multiHitInterval;
            msg.damage = per_hit;
            if (skill == 0x110) {
                msg.hit_effect = 0x7a;
            } else if (skill == 0x174) {
                msg.kind = 0x6c;
                render_object_send(self, self, 0x1b, (intptr_t)&msg, 0, 0, 0);
            } else if (skill == 0x2e1) {
                msg.hit_effect = 0x593;
            }
            msg.kind = kind;
            switch (skill) {
            case 0x19d: case 0x19f: case 0x1a1: case 0x1a3: case 0x1a5:
                render_object_send(target, self, 0x85, (intptr_t)&msg, total, 0, 0);
                break;
            default:
                render_object_send(target, self, 0x1b, (intptr_t)&msg, total, 0, 0);
                break;
            }
        }
    }
}
