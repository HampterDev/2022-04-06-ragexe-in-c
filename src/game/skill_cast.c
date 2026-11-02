#include "game/skill_cast.h"
#include "skills/skill_action.h"
#include "actor/actor_ops.h"
#include "std/string.h"
#include "skills/skill_info.h"
#include "actor/render_object.h"
#include "skills/skill_action_registry.h"
#include "session/state.h"
#include "core/memory.h"
#include "game/context.h"
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "game/skill_use.h"
#include "effects/effect_controls.h"
#include "actor/actor_common.h"
#include "effects/skills/skill_view.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_skills.h"
#include "session/session_talk.h"
#include "world/world.h"

enum {
    ACTOR_MSG_CAST_TIME = 0x52,
    ACTOR_MSG_ATTACK_TARGET = 0x51,
    ACTOR_MSG_TARGET_EFFECT = 0x56,
    ACTOR_MSG_TARGET_EFFECT_STOP = 0x57,
    ACTOR_MSG_EFFECT_STAGE = 0x8f,
    GROUND_MSG_OWNER = 0x16,
    GROUND_MSG_CELL = 0xe,
    GROUND_MSG_END_TIME = 0x73,
    GROUND_MSG_SKILL = 0x75,
};

static __inline void effect_set_time(struct Effect *effect, float value)
{
    effect->ops->set_stop_frame(effect, value);
}

char skill_cast_special(struct GameActor *caster, struct GameActor *target, int skill_id,
                               unsigned cast_time, int attack_motion)
{
    unsigned short skill = (unsigned short)skill_id;

    if (skill == 0x10f)
        return 0;
    if (skill == 0x49) {
        if (target && caster != target)
            render_object_turn_toward((struct RenderObject *)caster, target->pos.x, target->pos.z);
        game_send_message(MODE_MSG_SKILL_CAST, 0x49, (int)caster, 0, 0);
    }
    return cast_time ? 0 : 1;
}

void skill_cast_caster_tint(struct GameActor *caster, int skill_id, unsigned cast_time)
{
    switch ((unsigned short)skill_id) {
    case 0x191:
        render_object_play_motion_range((struct RenderObject *)caster, 0x60, 0, 0, 0x60, 0, 0, 0, 0);
        actor_launch_effect(caster, 0x192);
        break;
    case 0x39:
    case 0x3e:
        caster->motion_held = 1;
        caster->motion_hold_until = timeGetTime() + cast_time;
        break;
    case 0x105:
    case 0x3f7:
        render_object_play_motion_range((struct RenderObject *)caster, 0x60, 0, 0, 0x60, 0, 0, 0, 0);
        break;
    case 0x10c:
        render_object_play_motion_range((struct RenderObject *)caster, 0x60, 0, 0, 0x60, 0, 1, cast_time + 1000, 0);
        break;
    case 0x8a9:
        render_object_send(caster, NULL, ACTOR_MSG_EFFECT_STAGE, 0x2df, 1, 0, 0);
        render_object_send(caster, NULL, ACTOR_MSG_EFFECT_STAGE, 0x2e1, 1, 0, 0);
        render_object_send(caster, NULL, ACTOR_MSG_EFFECT_STAGE, 0x2e0, 1, 0, 0);
        render_object_send(caster, NULL, ACTOR_MSG_EFFECT_STAGE, 0x2e2, 1, 0, 0);
        break;
    }
}

void skill_cast_target_look(struct GameMode *mode, struct GameActor *caster, struct GameActor *target,
                                  unsigned target_aid, int skill_id, int x, int y, unsigned cast_time)
{
    unsigned short skill = (unsigned short)skill_id;
    struct Effect *effect;

    if (target_aid == 0) {
        float world_x = 0.0f, world_z = 0.0f;
        void *scope;

        game_mode_cell_to_world(game_current_mode(), (short)x, (short)y, &world_x, &world_z);
        render_object_turn_toward((struct RenderObject *)caster, world_x, world_z);
        scope = ground_scope_new();
        if (!scope)
            return;
        render_object_send(scope, NULL, GROUND_MSG_OWNER, (unsigned)caster->aid, 0, 0, 0);
        render_object_send(scope, NULL, GROUND_MSG_CELL, (short)x, (short)y, 0, 0);
        render_object_send(scope, NULL, GROUND_MSG_END_TIME, (unsigned)(timeGetTime() + cast_time), 0, 0, 0);
        render_object_send(scope, NULL, GROUND_MSG_SKILL, skill, 0, 0, 0);
        list_push_back(&mode->world->gameObjects, &scope, sizeof scope);
        return;
    }
    if (!target || caster == target)
        return;
    if (session_is_mercenary_or_homun_s_job(&g_session1, game_actor_get_job(target)) || target->object_type == 9) {
        struct World *world = mode->world;

        if (world->is_pvp || world->is_siege || !skill_info_is_good_skill_for_actor(skill))
            render_object_send(caster, NULL, ACTOR_MSG_ATTACK_TARGET, (unsigned)target->aid, 0, 0, 0);
    }
    render_object_turn_toward((struct RenderObject *)caster, target->pos.x, target->pos.z);
    render_object_send(target, NULL, ACTOR_MSG_TARGET_EFFECT, 0x3c, cast_time >> 4, 0, 0);
    switch (skill) {
    case 0x1ea:
        game_mode_launch_effect_at(mode, 0x21b, caster, target, -1);
        break;
    case 0x89a: {
        unsigned wait = (unsigned)(int)(target->motion_delay * 150.0f + 200.0f) + cast_time;

        effect = spawn_on(target->aid, 0x635);
        if (effect)
            effect_set_time(effect, (float)wait);
        effect = spawn_on(target->aid, 0x636);
        if (effect)
            effect_set_time(effect, (float)wait);
        break;
    }
    case 0x89c:
        actor_send(target, ACTOR_MSG_TARGET_EFFECT_STOP);
        render_object_send(target, NULL, ACTOR_MSG_TARGET_EFFECT, 0x5a8, cast_time >> 4, 0, 0);
        break;
    case 0x8a6:
        actor_send(target, ACTOR_MSG_TARGET_EFFECT_STOP);
        spawn_on(target->aid, 0x665);
        break;
    case 0x8bc:
        actor_send(target, ACTOR_MSG_TARGET_EFFECT_STOP);
        effect = spawn_on(target->aid, 0x673);
        if (effect)
            effect_set_time(effect, (float)cast_time);
        break;
    case 0x8d3: {
        struct RagEffect *launched = actor_launch_effect(caster, 0x313);

        if (launched) {
            Vec3 at = actor_pos(target);

            effect_set_pos(launched, &at);
        }
        break;
    }
    }
}

static __inline int is_skill_39_3e(unsigned short skill)
{
    return skill == 0x3e || skill == 0x39;
}

void game_mode_zc_useskill_ack(struct GameMode *mode, unsigned src_aid,
                                           unsigned target_aid, int x, int y, int skill_id, int property,
                                           unsigned delay_ms, unsigned char is_disposable, int attack_motion)
{
    unsigned short skill = (unsigned short)skill_id;
    struct GameActor *caster, *target;
    int cast_effect, cast_state;

    caster = world_get_actor_or_player(mode->world, src_aid);
    if (!caster)
        return;
    target = world_get_actor_or_player(mode->world, target_aid);
    skill_cast_chat_line(caster, target, skill_id, is_disposable);

    if (delay_ms == 0 && skill == 0xa04) {
        struct Effect *effect = effect_mgr_find_effect(g_effectMgr, caster->aid, 0x474);

        if (!effect)
            spawn_effect(caster->aid, 0x474, actor_pos(caster));
        else
            effect->ops->init(effect);
    }
    if (!is_disposable && !is_skill_39_3e(skill) && !session_is_monster_actor(&g_session1, caster)) {
        struct GameMode *gm = game_current_mode();

        if (skill == 0x14e || skill == 0x14f)
            mode_send_msg(gm, MODE_MSG_SKILL_CAST, skill, (int)caster, (int)target_aid, 0);
        else
            mode_send_msg(gm, MODE_MSG_SKILL_CAST, skill, (int)caster, 0, 0);
    }
    if (skill_cast_special(caster, target, skill_id, delay_ms, attack_motion) == 1)
        return;

    cast_effect = 0;
    cast_state = 0;
    {
        int job = game_actor_get_job(caster);

        session_get_cast_effect(&g_session1, skill, &cast_effect, &cast_state, property, job);
    }
    actor_set_state(caster, cast_state, 1);
    if (!is_skill_39_3e(skill) && delay_ms)
        render_object_send(caster, NULL, ACTOR_MSG_CAST_TIME, delay_ms, 0, 0, 0);
    skill_cast_play_cast_look(caster, x, y, skill_id, delay_ms, cast_effect);

    if (!is_skill_39_3e(skill) && session_is_monster_actor(&g_session1, caster)) {
        int action = 0, loop = 0;
        const char *monster = session_get_job_name(&g_session1, game_actor_get_job(caster), -1);

        session_get_monster_skill_info(&g_session1, monster, skill, &action, &loop);
        if (action != -1) {
            render_object_set_action((struct RenderObject *)caster, action * 8 - 8, loop == 0);
            if (skill == 0xc1)
                render_object_set_motion_delay((struct RenderObject *)caster, (float)(delay_ms / 300));
        }
    }
    skill_cast_caster_tint(caster, skill_id, delay_ms);
    skill_cast_target_look(mode, caster, target, target_aid, skill_id, x, y, delay_ms);
}
