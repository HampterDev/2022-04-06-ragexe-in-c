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
    MODE_MSG_QUAKE = 0x7a,
};

enum {
    SKILL_ARROW_SHOWER = 0x2f,
    SKILL_BASILICA = 0x16a,
    SKILL_GOSPEL = 0x171,
    SKILL_NPC_PULSESTRIKE2 = 0x2c8,
    SKILL_NJ_RAIGEKISAI = 0x21d,
    SKILL_NPC_PSYCHIC_WAVE = 0x2e0,
    SKILL_NPC_CLOUD_KILL = 0x2e3,
    SKILL_WL_FROSTMISTY = 0x89b,
    SKILL_CHAOS_PANIC = 0x8fd,
    SKILL_MAELSTROM = 0x8fe,
    SKILL_BLOODY_LUST = 0x8ff,
    SKILL_OVERBRAND = 0x90d,
    SKILL_WM_DOMINION_IMPULSE = 0x971,
    SKILL_LULLABY = 0x976,
    SKILL_SO_EARTHGRAVE = 0x98e,
    SKILL_PSYCHIC_WAVE = 0x991,
    SKILL_CLOUD_KILL = 0x992,
    SKILL_GN_CRAZYWEED = 0x9b3,
    SKILL_RIDE_IN_LIGHTNING = 0x9d6,
    SKILL_SJ_BOOKOFCREATINGSTAR = 0xa1b,
};

static const char WAVE_THORN_TRAP[] = "effect\\G가시나무덫.wav";

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

static void thorn_ring(struct GameActor *caster, int x, int y)
{
    struct GameMode *mode = game_current_mode();
    Vec3 centre, at;
    struct Effect *effect;
    float turn[12];
    int degrees, delay;

    play_skill_wave(WAVE_THORN_TRAP);
    actor_launch_effect(caster, 0x376);
    centre = cell_on_ground(mode, x, y);
    for (degrees = 0, delay = 0; degrees < 1080; degrees += 72, delay += 70) {
        int r = rand() % 20;
        float distance = (float)(r < 0 ? 0 : r) + 5.0f;

        mat_set_rotate_y(turn, (float)degrees / 180.0f * 3.141592f);
        at.x = turn[0] * distance + turn[3] * 0.0f + turn[6] * 0.0f + turn[9] + centre.x;
        at.y = turn[1] * distance + turn[4] * 0.0f + turn[7] * 0.0f + turn[10] + centre.y;
        at.z = turn[2] * distance + turn[5] * 0.0f + turn[8] * 0.0f + turn[11] + centre.z;

        effect = spawn_effect(0, 0x5b9, at);
        if (effect) {
            spawned_effect_zoom(effect, 0.5f);
            spawned_effect_delay(effect, delay);
        }
        effect = spawn_effect(0, 0x656, at);
        if (effect) {
            spawned_effect_zoom(effect, 0.5f);
            spawned_effect_delay(effect, delay);
        }
    }
}

static void overbrand(struct GameActor *caster)
{
    struct GameMode *mode = game_current_mode();
    Vec3 origin = { 0.0f, 0.0f, 0.0f };
    float heading;
    int effect_id = 0;

    if (!caster)
        return;
    heading = actor_screen_heading(mode, caster);
    if (heading >= 0.0f && heading < 23.0f)
        effect_id = 0x692;
    else if (heading >= 23.0f && heading < 67.0f)
        effect_id = 0x691;
    else if (heading >= 67.0f && heading < 112.0f)
        effect_id = 0x690;
    else if (heading >= 112.0f && heading < 157.0f)
        effect_id = 0x68f;
    else if (heading >= 157.0f && heading < 202.0f)
        effect_id = 0x696;
    else if (heading >= 202.0f && heading < 247.0f)
        effect_id = 0x695;
    else if (heading >= 247.0f && heading < 292.0f)
        effect_id = 0x694;
    else if (heading >= 292.0f && heading < 337.0f)
        effect_id = 0x693;
    else if (heading >= 337.0f && heading <= 360.0f)
        effect_id = 0x692;
    spawn_effect(caster->aid, effect_id, origin);
    play_skill_wave("effect\\LG_OVERBRAND.wav");
}

static struct RagEffect *caster_effect_on_cell(struct GameActor *caster, int effect_id, int x, int y)
{
    struct RagEffect *effect = actor_launch_effect(caster, effect_id);
    Vec3 at;

    if (!effect)
        return 0;
    at = cell_on_ground(game_current_mode(), x, y);
    effect_set_pos(effect, &at);
    return effect;
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
        render_object_send(self->world->player, NULL, ACTOR_MSG_GROUND_SKILL, (__int64)(int)&info, 0, 0, 0);
        caster = (struct GameActor *)self->world->player;
    } else {
        struct GameActor *actor = world_get_game_actor_by_aid(self->world, packet->aid);
        if (actor) {
            render_object_send(actor, NULL, ACTOR_MSG_GROUND_SKILL, (__int64)(int)&info, 0, 0, 0);
            caster = actor;
        }
    }
    if (caster)
        job = game_actor_get_job(caster);

    if (session_is_monster_actor(&g_session1, caster)) {
        monster_ground_skill(caster, job, packet);
        return;
    }
    mode_send_msg(self, MODE_MSG_GROUND_SKILL, packet->skill_id, (int)caster, packet->level, 0);

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
    case SKILL_WL_FROSTMISTY:
        at = cell_on_ground(game_current_mode(), packet->x, packet->y);
        game_mode_launch_dummy_effect(self, &at, 0x2d6, g_effectDurations[0x2d6] + 10, 0);
        break;
    case SKILL_NPC_PSYCHIC_WAVE:
    case SKILL_PSYCHIC_WAVE:
        psychic_wave(caster, packet->x, packet->y, packet->level);
        break;
    case SKILL_NPC_CLOUD_KILL:
    case SKILL_CLOUD_KILL:
        actor_launch_effect(caster, 0x376);
        play_skill_wave("effect\\S클라우드킬.wav");
        break;
    case SKILL_SO_EARTHGRAVE:
        thorn_ring(caster, packet->x, packet->y);
        break;
    case SKILL_GN_CRAZYWEED: {
        struct GameMode *mode = game_current_mode();
        play_skill_wave(WAVE_THORN_TRAP);
        at = cell_on_ground(mode, packet->x, packet->y);
        game_mode_launch_dummy_effect(self, &at, 0x393, 150, 0);
        break;
    }
    case SKILL_OVERBRAND:
        overbrand(caster);
        break;
    case SKILL_RIDE_IN_LIGHTNING: {
        struct GameMode *mode = game_current_mode();
        actor_launch_effect(caster, 0x376);
        at = cell_on_ground(mode, packet->x, packet->y);
        game_mode_launch_dummy_effect(self, &at, 0x376, 300, 0);
        game_mode_launch_dummy_effect(self, &at, 0x375, 200, 0);
        game_mode_launch_dummy_effect(self, &at, 0x383, 999, 0);
        play_skill_wave("effect\\SR_RIDEINLIGHTNING.wav");
        break;
    }
    case SKILL_LULLABY:
        play_skill_wave("effect\\안식의자장가.wav");
        caster_effect_on_cell(caster, 0x35a, packet->x, packet->y);
        break;
    case SKILL_WM_DOMINION_IMPULSE:
        if (!caster_effect_on_cell(caster, 0x35f, packet->x, packet->y))
            break;
        game_send_message(MODE_MSG_QUAKE, 0, 0, 0, 0);
        play_skill_wave("effect\\invisibility.wav");
        break;
    case SKILL_CHAOS_PANIC:
        play_skill_wave("effect\\chaospanic.wav");
        break;
    case SKILL_MAELSTROM:
        play_skill_wave("effect\\maelstrom.wav");
        break;
    case SKILL_BLOODY_LUST:
        play_skill_wave("effect\\bloodylust.wav");
        break;
    case SKILL_SJ_BOOKOFCREATINGSTAR: {
        if (!caster)
            break;
        at.x = caster->pos.x;
        at.y = caster->pos.y;
        at.z = caster->pos.z;
        spawn_effect(caster->aid, 0x4d0, at);
        break;
    }
    }
}
