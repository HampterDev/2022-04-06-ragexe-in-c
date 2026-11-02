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
    case 0xb: case 0x2f: case 0x111: case 0x19d: case 0x2dc: case 0x8d2: case 0x8ec:
    case 0x903: case 0x90d: case 0x918: case 0x919: case 0x91c: case 0x921:
    case 0x9ac: case 0x9d7: case 0xa10: case 0xbbe: case 0xbbf:
    case 0x13a2: case 0x13b6: case 0x145b: case 0x146e:
    case 0x1473: case 0x147b: case 0x147c: case 0x1484: case 0x14a3: case 0x14a4: case 0x14c4:
        return 1;
    }
    return 0;
}

static int skill_skips_cast_state(int skill, int action)
{
    switch (skill) {
    case 0x8f: case 0x107: case 0x10f: case 0x110: case 0x111: case 0x173:
    case 0x90d: case 0x916: case 0x918: case 0x919: case 0x9d7: case 0xa13: case 0x1506:
        return 1;
    case 0x8d5:
        return action == 8;
    }
    return skill_action_mgr_skips_use_motion(skill_action_mgr_instance(), skill);
}

static int skill_skips_attack_speed(int skill)
{
    switch (skill) {
    case 0x1ea: case 0x8d0: case 0x8d1: case 0xbc1:
        return 1;
    }
    return skill_action_mgr_skips_attack_delay(skill_action_mgr_instance(), skill);
}

static int skill_msg_delay(const struct GameActor *self)
{
    return (int)(int64_t)(self->attack_motion_count * self->motion_delay * 24.0f);
}

static int skill_msg_band(int skill)
{
    switch (skill) {
    case 0x145f: case 0x1469: case 0x146a: case 0x1472: case 0x1473: case 0x1474: case 0x1475:
    case 0x1479: case 0x147c: case 0x147d: case 0x14a6: case 0x14ae: case 0x14af: case 0x14b0:
    case 0x14b2: case 0x14c2: case 0x14c7: case 0x14d1: case 0x14ea: case 0x1507: case 0x1508:
    case 0x1509:
        return 1;
    }
    return 0;
}

static int send_skill_hit_60(struct GameActor *self, struct GameObject *target,
                             const struct ActorSkillUse *use, int damage, int value)
{
    int skill = use->skill_id;
    struct ActorHit hit;

    if (!target)
        return skill == 0x992 || skill == 0x2e3 || skill == 0x13a6 || skill == 0x9ad;
    if (skill_sends_no_hit(skill))
        return 0;
    hit.effect = -1;
    hit.hit_effect = -1;
    if (skill_msg_band(skill)) {
        hit.effect = session_get_skill_effect_id(&g_session1, skill);
        hit.hit_effect = session_get_skill_hit_effect_id(&g_session1, skill);
    }
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
        return skill == 0x992 || skill == 0x2e3;
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
    Vec3 tpos, ppos;
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
        if ((skill == 0x8c3 || skill == 0x8c4) && self->cart != 0)
            render_object_send(self->cart, NULL, 0x1d, (intptr_t)&target->pos, 0, 0, 0);
        if ((skill == 0x14ce || skill == 0x14d1) && self->falcon != 0)
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
    if (skill == 0x8f0 || skill == 0x14ca || skill == 0x14c4)
        actor_set_skip_arrow(self, 1);

    switch (type) {
    case 4: case 5: case 6: case 8: case 9: case 0xd: case 0xe:
        break;
    default:
        return;
    }
    if (type == 5) {
        if (skill == 0x8ec || skill == 0x2dc)
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
    if (target != NULL && target != self && !skill_action_mgr_skips_use_motion(skill_action_mgr_instance(), skill))
        render_object_turn_toward((struct RenderObject *)self, tpos.x, tpos.z);

    player = (struct GameActor *)mode->world->player;
    ppos = player->pos;
    switch (skill) {
    case 0x2199:
        sot_wave("effect\\MT_SUMMON_ABR_BATTLE_WARIOR.wav", self->pos.x - ppos.x, self->pos.y, self->pos.z - ppos.z);
        break;
    case 0x219a:
        sot_wave("effect\\MT_SUMMON_ABR_DUAL_CANNON.wav", self->pos.x - ppos.x, self->pos.y, self->pos.z - ppos.z);
        break;
    case 0x219d:
        sot_wave("effect\\MT_SUMMON_ABR_INFINITY.wav", self->pos.x - ppos.x, self->pos.y, self->pos.z - ppos.z);
        break;
    }
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
    case 0xbe: case 0xa28: case 0xa2a: hit1 = 0xb4; break;
    case 0x1d3: case 0x1d4: hit1 = 0x225; break;
    case 0x7e6: hit1 = 0x61f; break;
    case 0x89a: hit1 = 0x637; hit2 = 0x638; break;
    case 0x903: hit2 = 0x583; break;
    case 0x90d: case 0x9d7: hit1 = 0x699; break;
    case 0x913:
        if (g_session1.ownAid != self->attack_target_aid)
            hit1 = 0x6ac;
        break;
    case 0x96e: hit1 = 0x639; break;
    case 0x98e: hit1 = 0x657; break;
    case 0x1452: hit1 = 0x69f; hit2 = 0x6a0; break;
    case 0x1458: case 0x1459: hit1 = 0x67f; break;
    case 0x147b: hit1 = 0x82f; hit2 = 0x830; break;
    case 0x1490: hit1 = 0x88f; hit2 = 0x890; break;
    case 0x149a: hit1 = 0x722; break;
    case 0x14ab: hit1 = 0x7a2; break;
    case 0x14c8: hit1 = 0x854; break;
    case 0x14ca: hit1 = 0x843; break;
    case 0x14d6: hit1 = 0x7a0; break;
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
    case 0x8ab: msg.tick += 500;  with_damage = 1; break;
    case 0x8ac: msg.tick += 1000; with_damage = 1; break;
    case 0x8ad: msg.tick += 1500; with_damage = 1; break;
    case 0x111: case 0x173: case 0x8aa: case 0x918: case 0x919: case 0x91a:
        with_damage = 1;
        break;
    case 0x19f: case 0x1a1: case 0xa20: case 0xa21:
        msg.tick += 300;
    case 0x19d: case 0x1a3: case 0x1a5: case 0x91c: case 0x920: case 0x926: case 0xa10: case 0xa14:
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
    case 0x10a: case 0x10b: case 0x9d6:
        render_object_set_script_step_frames((struct RenderObject *)self, 10);
        if (session_is_doram_job(&g_session1, game_actor_get_job(self)) == 1)
            sot_motion(self, 0x58, 4, 5, 0x44c);
        else
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
    case 0x916:
        render_object_set_script_step_frames((struct RenderObject *)self, 6);
        sot_motion(self, 0x60, 4, 4, sot_attack_duration(self));
        break;
    case 0x917: case 0x91a:
        render_object_set_script_step_frames((struct RenderObject *)self, 6);
        sot_motion(self, 0x60, 5, 5, sot_attack_duration(self));
        break;
    case 0x918:
        render_object_set_script_step_frames((struct RenderObject *)self, 6);
        sot_motion(self, 0x60, 1, 2, sot_attack_duration(self));
        if (!able_to_make_effect_is_effect_exist(self, 0x378)) {
            game_send_message(0x7a, 0, 0, 0, 0);
            sot_wave("effect\\SR_EARTHSHAKER.wav", 0.0f, 0.0f, 0.0f);
            sot_launch(self, 0x376);
            sot_launch(self, 0x378);
        }
        break;
    case 0x919: {
        Vec3 zero = { 0.0f, 0.0f, 0.0f };
        render_object_set_script_step_frames((struct RenderObject *)self, 6);
        sot_motion(self, 0x60, 3, 3, sot_attack_duration(self));
        game_send_message(0x7a, 0, 0, 0, 0);
        sot_wave("effect\\SR_FALLENEMPIRE.wav", 0.0f, 0.0f, 0.0f);
        effect_mgr_spawn(g_effectMgr, target->aid, 0x622, zero, 0, -1, 0, 0);
        effect_mgr_spawn(g_effectMgr, target->aid, 0x623, zero, 0, -1, 0, 0);
        break;
    }
    case 0x91c:
        if (target != self) {
            sot_launch(target, 0x225);
            sot_launch(target, 0x226);
            e = sot_launch(self, 0x37b);
            if (e != NULL) {
                Vec3 at = target->pos;
                sot_effect_at(e, &at);
            }
        }
        break;
    case 0x9ff:
        sot_launch(target, 0x106);
        sot_launch(target, 0x111);
        break;
    case 0xa04:
        render_object_set_script_step_frames((struct RenderObject *)self, 4);
        sot_motion(self, 0x58, 0, 7, 0xdac);
        break;
    case 0x145e: case 0x1464: case 0x146e: case 0x14e6: case 0x14e9: case 0x14ed:
        sot_motion(self, 0x60, 1, 3, 500);
        break;
    case 0x1483: case 0x1484: case 0x1485:
        render_object_set_script_step_frames((struct RenderObject *)self, 4);
        sot_motion(self, 0x60, 2, 2, 500);
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
            } else if (skill == 0x8a3) {
                msg.hit_effect = g_session1.map_rule_flags[0] == 1 ? -1 : 0x512;
            } else if (skill == 0x8a5) {
                if (i & 1) {
                    msg.effect = -1;
                } else if (g_session1.map_rule_flags[0] == 1) {
                    msg.effect = 0x601;
                    msg.hit_effect = 0x602;
                } else {
                    msg.effect = 0x588;
                    msg.hit_effect = 0x589;
                }
            } else if (skill == 0x911 || skill == 0x2e1) {
                msg.hit_effect = 0x593;
            }
            switch (skill) {
            case 0x1464: case 0x1481: case 0x1482: case 0x1491: case 0x1492: case 0x149d: case 0x14a7:
            case 0x14a9: case 0x14ac: case 0x14d2: case 0x14dc: case 0x14dd: case 0x14de: case 0x14df:
            case 0x14f9: case 0x14fd: case 0x1506: case 0x150d: case 0x150e: case 0x150f: case 0x1510:
            case 0x1511: case 0x1f6f: case 0x1f70: case 0x1f72:
                msg.hit_effect = session_get_skill_hit_effect_id(&g_session1, skill);
                break;
            case 0x1490:
                msg.hit_effect = -1;
                msg.effect = -1;
                break;
            case 0x14ab:
                msg.hit_effect = 0x7a2 | (i & 1);
                break;
            case 0x14c8:
                msg.effect = -1;
                msg.hit_effect = session_get_skill_hit_effect_id(&g_session1, skill);
                break;
            }
            msg.kind = kind;
            switch (skill) {
            case 0x19d: case 0x19f: case 0x1a1: case 0x1a3: case 0x1a5:
            case 0x91c: case 0x920: case 0x926: case 0xa10: case 0xa14: case 0xa20: case 0xa21:
                render_object_send(target, self, 0x85, (intptr_t)&msg, total, 0, 0);
                break;
            default:
                render_object_send(target, self, 0x1b, (intptr_t)&msg, total, 0, 0);
                break;
            }
        }
    }
}
