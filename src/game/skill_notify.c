#include "game/packet_handlers.h"
#include "session/state.h"
#include "game/skill_fx.h"
#include "actor/job.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_talk.h"

enum {
    ACTOR_MSG_SKILL_HIT = 0x25,
    MODE_MSG_SKILL_USED = 0x56,
};

static struct GameActor *tell_caster(struct GameMode *mode, int aid, const struct ActorSkillUse *info, int *job,
                                      struct GameActor **other_actor)
{
    struct GameActor *actor;
    void *unit;

    *job = -1;
    *other_actor = 0;
    if ((unsigned)aid == (unsigned)g_session1.ownAid) {
        render_object_send(mode->world->player, NULL, ACTOR_MSG_SKILL_HIT, MSG_ARG_PTR(info), 0, 0, 0);
        actor = (struct GameActor *)mode->world->player;
        *job = game_actor_get_job(actor);
        return actor;
    }
    actor = world_get_game_actor_by_aid(mode->world, aid);
    if (actor) {
        render_object_send(actor, NULL, ACTOR_MSG_SKILL_HIT, MSG_ARG_PTR(info), 0, 0, 0);
        *other_actor = actor;
        *job = game_actor_get_job(actor);
        return actor;
    }
    unit = (struct GameActor *)world_get_skill_by_aid(mode->world, aid);
    if (unit)
        render_object_send(unit, NULL, ACTOR_MSG_SKILL_HIT, MSG_ARG_PTR(info), 0, 0, 0);
    return 0;
}

static void play_monster_skill_motion(struct GameActor *other_actor, int job, int skill_id)
{
    int action, loop_type;

    session_get_monster_skill_info(&g_session1, session_get_job_name(&g_session1, job, -1), skill_id, &action, &loop_type);
    if (action == -1 || !other_actor)
        return;
    render_object_set_action((struct RenderObject *)other_actor, action * 8 - 8, loop_type == 0);
}

#pragma pack(push, 1)
struct PacketNotifySkill {
    unsigned short packet_type;
    unsigned short skill_id;
    int src_aid;
    int target_aid;
    int start_time;
    int attack_mt;
    int attacked_mt;
    short damage;
    short level;
    short count;
    unsigned char action;
};
#pragma pack(pop)

void game_mode_on_notify_skill(struct GameMode *self, const struct PacketNotifySkill *packet)
{
    struct ActorSkillUse info;
    struct GameActor *caster, *other_actor;
    int job;

    info.skill_id = packet->skill_id;
    info.action = packet->action;
    info.damage = packet->damage;
    info.start_time = packet->start_time;
    info.target_aid = packet->target_aid;
    info.level = packet->level;
    info.count = packet->count;
    info.attack_mt = packet->attack_mt;
    info.attacked_mt = packet->attacked_mt;
    info.cell_x = -1;
    info.cell_y = -1;
    caster = tell_caster(self, packet->src_aid, &info, &job, &other_actor);

    if (!session_is_monster_actor(&g_session1, caster)) {
        mode_send_msg(self, MODE_MSG_SKILL_USED, packet->skill_id, (int)caster, packet->level, 0);
        return;
    }
    play_monster_skill_motion(other_actor, job, info.skill_id);
}

#pragma pack(push, 1)
struct PacketNotifySkillPosition {
    unsigned short packet_type;
    unsigned short skill_id;
    int src_aid;
    int target_aid;
    int start_time;
    int attack_mt;
    int attacked_mt;
    short x, y;
    short damage;
    short level;
    short count;
    unsigned char action;
};
#pragma pack(pop)

void game_mode_on_notify_skill_position(struct GameMode *self, const struct PacketNotifySkillPosition *packet)
{
    struct ActorSkillUse info;
    struct GameActor *caster, *other_actor;
    int job, action, loop_type;

    info.skill_id = packet->skill_id;
    info.action = packet->action;
    info.damage = packet->damage;
    info.start_time = packet->start_time;
    info.target_aid = packet->target_aid;
    info.level = packet->level;
    info.count = packet->count;
    info.cell_x = packet->x;
    info.cell_y = packet->y;
    info.attack_mt = packet->attack_mt;
    info.attacked_mt = packet->attacked_mt;
    caster = tell_caster(self, packet->src_aid, &info, &job, &other_actor);

    if (!session_is_monster_actor(&g_session1, caster)) {
        mode_send_msg(self, MODE_MSG_SKILL_USED, packet->skill_id, (int)caster, packet->level, 0);
        return;
    }
    session_get_monster_skill_info(&g_session1, session_get_job_name(&g_session1, job, -1), info.skill_id, &action,
                                &loop_type);
}

#pragma pack(push, 1)
struct PacketNotifySkill2 {
    unsigned short packet_type;
    unsigned short skill_id;
    int src_aid;
    int target_aid;
    int start_time;
    int attack_mt;
    int attacked_mt;
    int damage;
    short level;
    short count;
    unsigned char action;
};
#pragma pack(pop)

void game_mode_on_notify_skill2(struct GameMode *self, const struct PacketNotifySkill2 *packet)
{
    struct ActorSkillUse info;
    struct GameActor *caster, *target, *other_actor;
    int job;

    info.skill_id = packet->skill_id;
    info.action = packet->action;
    info.damage = packet->damage;
    info.start_time = packet->start_time;
    info.level = packet->level;
    info.count = packet->count;
    info.attack_mt = packet->attack_mt;
    info.attacked_mt = packet->attacked_mt;
    info.target_aid = packet->target_aid;
    info.cell_x = -1;
    info.cell_y = -1;
    if ((unsigned)packet->target_aid == (unsigned)g_session1.ownAid)
        target = (struct GameActor *)self->world->player;
    else
        target = world_get_game_actor_by_aid(self->world, packet->target_aid);
    caster = tell_caster(self, packet->src_aid, &info, &job, &other_actor);

    if (target && caster)
        skill_hit_caster_effects(self, packet->skill_id, caster, target);
    if (target)
        skill_hit_target_effects(packet->skill_id, packet->level, caster, target);

    if (caster) {
        if (!session_is_monster_actor(&g_session1, caster)) {
            mode_send_msg(self, MODE_MSG_SKILL_USED, packet->skill_id, (int)caster, packet->level, 0);
            return;
        }
        if (caster->object_type == 10)
            mode_send_msg(self, MODE_MSG_SKILL_USED, packet->skill_id, (int)caster, packet->level, 0);
    }
    play_monster_skill_motion(other_actor, job, info.skill_id);
}
