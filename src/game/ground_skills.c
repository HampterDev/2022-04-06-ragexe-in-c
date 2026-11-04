#include "game/packet_handlers.h"
#include "render/math3d.h"
#include "session/state.h"
#include "game/context.h"
#include <stdlib.h>

#include "game/skill_fx.h"
#include "actor/job.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_talk.h"
#include <stdint.h>

#pragma pack(push, 1)
struct PacketNotifyGroundSkill {
    unsigned short packet_type;
    unsigned short skill_id;
    int aid;
    short level;
    short x, y;
    int start_time;
};
#pragma pack(pop)

enum {
    ACTOR_MSG_GROUND_SKILL = 0x26,
    MODE_MSG_GROUND_SKILL = 0x56,
};

enum {
    SKILL_ARROW_SHOWER = 0x2f,
    SKILL_BASILICA = 0x16a,
    SKILL_GOSPEL = 0x171,
    SKILL_NPC_PULSESTRIKE2 = 0x2c8,
    SKILL_NJ_RAIGEKISAI = 0x21d,
    SKILL_NPC_PSYCHIC_WAVE = 0x2e0,
    SKILL_NPC_CLOUD_KILL = 0x2e3,
};

static const int arrow_shower_offsets[8][2] = {
    { 1, 0 }, { 1, 1 }, { 1, -1 }, { -1, 0 }, { -1, 1 }, { -1, -1 }, { 0, 1 }, { 0, -1 },
};

static void arrow_shower(struct GameActor *caster, int x, int y)
{
    struct GameMode *mode = game_current_mode();
    const float *from;
    int i;

    if (!caster)
        return;
    from = &caster->pos.x;
    game_mode_launch_arrow_to_point(mode, from, from, 18.0f);
    for (i = 0; i < 8; i++) {
        Vec3 to = cell_on_ground(mode, x + arrow_shower_offsets[i][0], y + arrow_shower_offsets[i][1]);
        game_mode_launch_arrow_to_point(mode, from, &to.x, 18.0f);
    }
}

static void psychic_wave(struct GameActor *caster, int x, int y, int level)
{
    struct GameMode *mode = game_current_mode();
    Vec3 at;
    struct Effect *effect;
    int pulses, delay;

    play_skill_wave("effect\\S사이킥웨이브.wav");
    actor_launch_effect(caster, 0x376);
    at = cell_on_ground(mode, x, y);
    pulses = level + 2;
    if (pulses <= 0)
        return;
    for (delay = 0; pulses != 0; pulses--, delay += 500) {
        int second;

        if (g_session1.map_rule_flags[0] == 1) {
            effect = spawn_effect(0, 0x5ae, at);
            if (effect)
                spawned_effect_delay(effect, delay);
            second = 0x5af;
        } else {
            effect = spawn_effect(0, 0x508, at);
            if (effect)
                spawned_effect_delay(effect, delay);
            second = 0x509;
        }
        effect = spawn_effect(0, second, at);
        if (effect)
            spawned_effect_delay(effect, delay);
    }
}

static void monster_ground_skill(struct GameActor *caster, int job, const struct PacketNotifyGroundSkill *packet)
{
    int action, loop_type;

    session_get_monster_skill_info(&g_session1, session_get_job_name(&g_session1, job, -1), packet->skill_id,
                                &action, &loop_type);
    if (packet->skill_id == SKILL_NPC_PULSESTRIKE2)
        actor_launch_effect(caster, 0x199);
    if (packet->skill_id == SKILL_NPC_PSYCHIC_WAVE)
        psychic_wave(caster, packet->x, packet->y, packet->level);
}

void game_mode_on_notify_ground_skill(struct GameMode *self, const struct PacketNotifyGroundSkill *packet)
{
    struct GroundSkillInfo info;
    struct GameActor *caster = 0;
    Vec3 at;
    int job = -1;

    info.skill_id = packet->skill_id;
    info.start_time = packet->start_time;
    info.x = packet->x;
    info.y = packet->y;
    info.level = packet->level;
    if ((unsigned)packet->aid == (unsigned)g_session1.ownAid) {
        render_object_send(self->world->player, NULL, ACTOR_MSG_GROUND_SKILL, (intptr_t)&info, 0, 0, 0);
        caster = (struct GameActor *)self->world->player;
    } else {
        struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);
        if (actor) {
            render_object_send(actor, NULL, ACTOR_MSG_GROUND_SKILL, (intptr_t)&info, 0, 0, 0);
            caster = actor;
        }
    }
    if (caster)
        job = game_actor_get_job(caster);

    if (session_is_monster_actor(&g_session1, caster)) {
        monster_ground_skill(caster, job, packet);
        return;
    }
    mode_send_msg(self, MODE_MSG_GROUND_SKILL, packet->skill_id, (intptr_t)caster, packet->level, 0);

    switch (packet->skill_id) {
    case SKILL_GOSPEL:
        play_skill_wave("effect\\가스펠.wav");
        break;
    case SKILL_BASILICA:
        play_skill_wave("effect\\바실리카.wav");
        break;
    case SKILL_ARROW_SHOWER:
        arrow_shower(caster, packet->x, packet->y);
        break;
    case SKILL_NJ_RAIGEKISAI:
        at = cell_on_ground(game_current_mode(), packet->x, packet->y);
        game_mode_launch_dummy_effect(self, &at, 0x26e, 200, 0);
        break;
    case SKILL_NPC_PSYCHIC_WAVE:
        psychic_wave(caster, packet->x, packet->y, packet->level);
        break;
    case SKILL_NPC_CLOUD_KILL:
        actor_launch_effect(caster, 0x376);
        play_skill_wave("effect\\S클라우드킬.wav");
        break;
    }
}
