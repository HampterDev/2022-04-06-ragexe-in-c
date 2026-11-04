#include "game/packet_handlers.h"
#include "actor/render_object.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/skill_use.h"
#include "actor/job.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_skills.h"
#include "session/session_state.h"
#include "session/session_talk.h"

enum {
    SKILL_MG_SRECOVERY = 0x9,
    SKILL_CR_AUTOGUARD = 0xf9,
    SKILL_VAPORIZE = 0xf4,
    SKILL_WE_MALE = 0x14e,
    SKILL_WE_FEMALE = 0x14f,
    SKILL_ML_AUTOGUARD = 0x201c,
};

static int keeps_its_state(int skill_id)
{
    switch (skill_id) {
    case 0x107: case 0x110: case 0x111: case 0x173:
        return 1;
    }
    return 0;
}

static int is_cast_on_cell(int skill_id)
{
    switch (skill_id) {
    case 0xc: case 0x12: case 0x15: case 0x46: case 0x4f: case 0x53: case 0x55: case 0x59: case 0x6e:
    case 0x1de: case 0x2d3: case 0x2dd: case 0x301:
        return 1;
    }
    return 0;
}

static void play_cast_motion(struct GameMode *mode, unsigned short skill_id, int level, int target_aid,
                             struct GameActor *caster)
{
    int action = -1, loop_type, motion, state;

    if (skill_id == SKILL_CR_AUTOGUARD || skill_id == SKILL_ML_AUTOGUARD)
        actor_set_state(caster, 2, 1);

    if (!session_is_monster_actor(&g_session1, caster) && skill_id != SKILL_MG_SRECOVERY) {
        if (skill_id == SKILL_WE_MALE || skill_id == SKILL_WE_FEMALE)
            mode_send_msg(mode, MODE_MSG_SKILL_CAST, skill_id, (intptr_t)caster, target_aid, 0);
        else
            mode_send_msg(mode, MODE_MSG_SKILL_CAST, skill_id, (intptr_t)caster, level, 0);
    } else {
        session_get_monster_skill_info(&g_session1, session_get_job_name(&g_session1, game_actor_get_job(caster), -1),
                                    skill_id, &action, &loop_type);
        if (action != -1)
            render_object_set_action((struct RenderObject *)caster, action * 8 - 8, loop_type == 0);
    }
    if (session_is_monster_actor(&g_session1, caster) == 1)
        mode_send_msg(mode, MODE_MSG_SKILL_CAST, skill_id, (intptr_t)caster, level, 0);

    if (action != -1 || keeps_its_state(skill_id))
        return;
    session_get_use_effect(&g_session1, skill_id, &motion, &state);
    if (caster->state != 1 && state == 1)
        actor_set_state(caster, 7, 1);
    else
        actor_set_state(caster, state, 1);
}

static void face_the_target(struct GameMode *mode, unsigned short skill_id, int src_aid, unsigned target_aid,
                            struct GameActor *caster, struct GameActor *target)
{
    if (is_cast_on_cell(skill_id)) {
        float world_x, world_z;

        if (!caster)
            return;
        game_mode_cell_to_world(mode, target_aid >> 16, (unsigned short)target_aid, &world_x, &world_z);
        render_object_turn_toward((struct RenderObject *)caster, world_x, world_z);
        return;
    }
    if (skill_id == 0x7c || skill_id == 0x83) {
        struct GameActor *unit = (struct GameActor *)world_get_skill_by_aid(mode->world, target_aid);

        if (unit && caster)
            render_object_turn_toward((struct RenderObject *)caster, unit->pos.x, unit->pos.z);
        return;
    }
    if (skill_id == SKILL_VAPORIZE && (unsigned)src_aid == (unsigned)g_session1.ownAid) {
        if (g_windowMgr.homun_info_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, 0x71);
        if (g_windowMgr.homun_skill_list_wnd)
            ui_window_mgr_post_quit1(&g_windowMgr, 0x72);
        session_clear_homun_commands(&g_session1);
        g_session1.homun_ai_flags[0] = 0;
    }
    if (caster && target && caster != target)
        render_object_turn_toward((struct RenderObject *)caster, target->pos.x, target->pos.z);
}

static void play_table_look(const struct SkillCast *cast)
{
    skill_effect_on_caster(cast->skill_id, cast->caster, cast->target);
}

static int play_known_look(const struct SkillCast *cast)
{
    int id = cast->skill_id;

    if (id <= 0x130)
        return skill_use_basic(cast);
    if (id <= 0x23f)
        return skill_use_second(cast);
    if (id <= 0x7d1)
        return skill_use_extra(cast);
    if (id <= 0x146e)
        return skill_use_late(cast);
    return skill_use_companion(cast);
}

int play_simple_look(const struct SkillCast *cast, const struct SimpleLook *looks, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        if (looks[i].skill_id != cast->skill_id)
            continue;
        if (looks[i].on == ON_TARGET)
            target_effect(cast, looks[i].effect_id);
        else
            caster_effect(cast, looks[i].effect_id);
        return 1;
    }
    return 0;
}

void skill_use_heal(const struct SkillCast *cast)
{
    int effect_id;

    if (!cast->target)
        return;
    if (cast->level >= 4000)
        effect_id = 0x145;
    else if (cast->level >= 2000)
        effect_id = 0x139;
    else if (cast->level >= 200)
        effect_id = 0x138;
    else
        effect_id = 0x7;
    actor_launch_effect(cast->target, effect_id);
    actor_show_number(cast->target, cast->level, 0xff00ff00);
}

void game_mode_show_skill_use(struct GameMode *self, unsigned short skill_id, int level, int src_aid,
                                      int target_aid, char result)
{
    struct SkillCast cast;
    struct GameActor *caster, *target;

    if ((unsigned)src_aid == (unsigned)g_session1.ownAid)
        caster = (struct GameActor *)self->world->player;
    else
        caster = world_get_game_actor_by_aid(self->world, src_aid);
    if (caster)
        play_cast_motion(self, skill_id, level, target_aid, caster);

    if ((unsigned)target_aid == (unsigned)g_session1.ownAid)
        target = (struct GameActor *)self->world->player;
    else
        target = world_get_game_actor_by_aid(self->world, target_aid);
    face_the_target(self, skill_id, src_aid, target_aid, caster, target);

    if (!result)
        return;

    cast.mode = self;
    cast.skill_id = skill_id;
    cast.level = level;
    cast.effect_level = level;
    if (level < 1)
        cast.effect_level = 1;
    else if (level > 10 && skill_id != SKILL_MG_SRECOVERY)
        cast.effect_level = 10;
    cast.src_aid = src_aid;
    cast.target_aid = target_aid;
    cast.caster = caster;
    cast.target = target;
    if (!play_known_look(&cast))
        play_table_look(&cast);
}
