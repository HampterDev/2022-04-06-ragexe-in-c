#include "game/world_objects.h"
#include "session/state.h"
#include "game/context.h"
#include "skills/skill_action_internal.h"
#include <stdlib.h>
#include "effects/skills/ez2str_effect_ex.h"
#include "actor/actor.h"
#include "core/text_util.h"

static void hn_doublebowlingbash_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0xc, cast_time);
}

static void hn_doublebowlingbash_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("double_bowlingbash", caster);
}

static void hn_doublebowlingbash_damage_hit(SkillActor *source, SkillActor *target)
{
    if (target)
        game_actor_launch_effect((struct GameActor *)target, 0x49, skill_vec_zero(), 0);
}

static void hn_mega_sonic_blow_use(int level, SkillActor *caster, SkillActor *target)
{
    if (!caster)
        return;
    skill_launch(caster, 0x79);
    skill_spawn_on("mega_sonic_blow", caster);
}

static void hn_shield_chain_rush_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x3a, cast_time);
}

static void hn_shield_chain_rush_use(int level, SkillActor *caster, SkillActor *target)
{
    if (target)
        skill_spawn_on("shield_chain_rush", target);
}

static void hn_spiral_pierce_max_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x1f6, cast_time);
}

static void hn_spiral_pierce_max_use(int level, SkillActor *caster, SkillActor *target)
{
    float view[12], angle;
    struct Effect *effect;

    if (!caster || !target)
        return;
    if (!g_session1.map_rule_flags[0])
        skill_spawn_on("spiral_pierce_max_aura_bottom", caster);
    skill_spawn_on("spiral_pierce_max_aura", caster);
    if (!game_copy_view_matrix(view))
        return;
    angle = effect_screen_angle(caster->pos, target->pos, view);
    effect = skill_spawn_at("spiral_pierce_max", caster->pos);
    if (effect)
        effect->ops->set_spin(effect, angle);
}

static void hn_meteor_storm_buster_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x37, cast_time);
}

static void hn_meteor_storm_buster_use_on_ground(Vec3 pos, int level, SkillActor *caster)
{
    skill_spawn_at("meteor_storm_buster_meteor_bottom", pos);
    skill_spawn_at("meteor_storm_buster_meteor", pos);
    skill_spawn_at("meteor_storm_buster_bottom", pos);
    skill_spawn_at("meteor_storm_buster", pos);
}

static void hn_jupitel_thunder_storm_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x39, cast_time);
}

static void hn_jupitel_thunder_storm_use(int level, SkillActor *caster, SkillActor *target)
{
    if (!caster || !target)
        return;
    game_actor_launch_effect((struct GameActor *)caster, 0xc, skill_vec_zero(), 0);
    effect_mgr_spawn(g_effectMgr, caster->aid, 0x93e, caster->pos, target->aid, level, 0, 0);
}

static void hn_jack_frost_nova_cast(unsigned cast_time, SkillActor *caster)
{
    struct Effect *effect;

    if (!caster)
        return;
    game_actor_launch_effect_timed((struct GameActor *)caster, 0x5a9, cast_time >> 4);
    effect = effect_mgr_spawn(g_effectMgr, caster->aid, g_session1.map_rule_flags[0] ? 0x607 : 0x5a6,
                               skill_vec_zero(), 0, -1, 0, 0);
    if (effect)
        effect->ops->set_stop_frame(effect, (float)(double)cast_time);
}

static void hn_jack_frost_nova_cast_at_cell(unsigned time_ms, Vec3 pos)
{
    if (game_current_mode())
        game_mode_launch_dummy_effect(game_current_mode(), &pos, 0x5a8, time_ms / 24, 0);
}

static void hn_jack_frost_nova_appear(int level, SkillActor *unit, SkillActor *owner)
{
    if (!unit)
        return;
    skill_spawn_on("jack_frost_nova_start_bottom", unit);
    skill_spawn_on("jack_frost_nova_start", unit);
    skill_spawn_on("jack_frost_nova_loop_bottom", unit);
    skill_spawn_on("jack_frost_nova_loop", unit);
}

static void hn_jack_frost_nova_leave(SkillActor *unit)
{
    if (unit)
        skill_spawn_at("jack_frost_nova_end", unit->pos);
}

static void hn_hells_drive_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x6ad, cast_time);
}

static float hells_drive_take(struct Vec *v, int count)
{
    int *first = (int *)v->first;
    float offset;

    if (first == (int *)v->last)
        return 0.0f;
    offset = (float)((double)(*first - count / 2) * 5.0);
    memmove(first, first + 1, (char *)v->last - (char *)(first + 1));
    v->last = (int *)v->last - 1;
    return offset;
}

static void hn_hells_drive_use(int level, SkillActor *caster, SkillActor *target)
{
    struct Vec xs, zs;
    struct String name;
    struct Effect *effect;
    int count, i, delay, r;
    float dz, dx;

    if (!caster)
        return;
    count = (level - 1) / 3 * 2 + 3;
    skill_shuffled_indices(&xs, count);
    skill_shuffled_indices(&zs, count);
    delay = 0;
    for (i = 0; i < count; i++) {
        dz = hells_drive_take(&zs, count);
        dx = hells_drive_take(&xs, count);

        effect = skill_spawn_at("hells_drive_bottom", caster->pos);
        if (effect) {
            effect->pos.y = caster->pos.y + 0.0f;
            effect->pos.x = caster->pos.x + dx;
            effect->pos.z = caster->pos.z + dz;
            effect->ops->set_start_delay(effect, delay);
        }

        r = rand() % 3;
        if (r < 0)
            r = 0;
        string_init(&name);
        string_format(&name, "hells_drive_%02d", r);
        effect = effect_mgr_spawn(
            g_effectMgr, 0, ez2_str_effect_mgr_get_effect_id(ez2str_effect_mgr_instance(), &name),
            caster->pos, 0, -1, 0, 0);
        string_free(&name);
        if (effect) {
            effect->pos.y = caster->pos.y + 0.0f;
            effect->pos.x = dx + caster->pos.x;
            effect->pos.z = caster->pos.z + dz;
            effect->ops->set_start_delay(effect, delay);
        }
        delay += 0x20;
    }
    vec_free(&zs);
    vec_free(&xs);
}

static void hn_ground_gravitation_cast(unsigned cast_time, SkillActor *caster)
{
    if (!caster)
        return;
    game_actor_launch_effect_timed((struct GameActor *)caster, 9, cast_time >> 4);
    game_actor_launch_effect_timed((struct GameActor *)caster, 0xc, cast_time >> 4);
}

static void hn_ground_gravitation_appear(int level, SkillActor *unit, SkillActor *owner)
{
    skill_spawn_on("ground_gravitation_bottom_start", unit);
    skill_spawn_on("ground_gravitation_start", unit);
    skill_spawn_on("ground_gravitation_bottom_loop", unit);
    skill_spawn_on("ground_gravitation_loop", unit);
}

static void hn_ground_gravitation_leave(SkillActor *unit)
{
    struct Effect *effect;

    effect = effect_mgr_find_effect(g_effectMgr, unit->aid,
                                    skill_named_effect_id("ground_gravitation_bottom_loop"));
    if (effect)
        effect->ops->fade_out(effect);
    effect = effect_mgr_find_effect(g_effectMgr, unit->aid,
                                    skill_named_effect_id("ground_gravitation_loop"));
    if (effect)
        effect->ops->fade_out(effect);
}

static void hn_napalm_vulcan_strike_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x3a, cast_time);
}

static void hn_napalm_vulcan_strike_hit(SkillActor *caster, SkillActor *target,
                                        SkillActor *target_unit)
{
    if (target)
        skill_spawn_on("napalm_vulcan_strike", target);
}

static void hn_breakinglimit_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x941, cast_time);
}

static void hn_breakinglimit_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("breakinglimit", caster);
}

static void efst_breaking_limit_on(int efst, int value, SkillActor *actor)
{
    if (actor)
        skill_spawn_on("breakinglimit_buff", actor);
}

static void efst_breaking_limit_off(int efst, int value, SkillActor *actor)
{
    if (actor)
        effect_mgr_remove_type(g_effectMgr, actor->aid,
                               skill_named_effect_id("breakinglimit_buff"));
}

static void hn_rulebreak_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x941, cast_time);
}

static void hn_rulebreak_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("rulebreak", caster);
}

static void efst_rule_break_on(int efst, int value, SkillActor *actor)
{
    if (actor)
        skill_spawn_on("rulebreak_buff", actor);
}

static void efst_rule_break_off(int efst, int value, SkillActor *actor)
{
    if (actor)
        effect_mgr_remove_type(g_effectMgr, actor->aid, skill_named_effect_id("rulebreak_buff"));
}

const struct StatusAction efst_action_breakinglimit_ops = {
    .on = efst_breaking_limit_on,
    .off = efst_breaking_limit_off,
    .reapply = efst_breaking_limit_on,
};

const struct StatusAction efst_action_rulebreak_ops = {
    .on = efst_rule_break_on,
    .off = efst_rule_break_off,
    .reapply = efst_rule_break_on,
};

const struct SkillAction skill_action_hn_breakinglimit_ops = {
    .cast_action = 8,
    .use_action = 58,
    .cast = hn_breakinglimit_cast,
    .use = hn_breakinglimit_use,
};

const struct SkillAction skill_action_hn_doublebowlingbash_ops = {
    .cast_action = 8,
    .use_action = 58,
    .cast = hn_doublebowlingbash_cast,
    .use = hn_doublebowlingbash_use,
    .damage_hit = hn_doublebowlingbash_damage_hit,
};

const struct SkillAction skill_action_hn_ground_gravitation_ops = {
    .cast_action = 8,
    .use_action = 59,
    .skips_use_motion = 1,
    .cast = hn_ground_gravitation_cast,
};

const struct SkillAction skill_action_hn_hells_drive_ops = {
    .cast_action = 8,
    .use_action = 59,
    .cast = hn_hells_drive_cast,
    .use = hn_hells_drive_use,
};

const struct SkillAction skill_action_hn_jack_frost_nova_ops = {
    .cast_action = 8,
    .use_action = 59,
    .skips_use_motion = 1,
    .area = -2,
    .cast = hn_jack_frost_nova_cast,
    .cast_at_cell = hn_jack_frost_nova_cast_at_cell,
};

const struct SkillAction skill_action_hn_jupitel_thunder_storm_ops = {
    .cast_action = 8,
    .use_action = 59,
    .cast = hn_jupitel_thunder_storm_cast,
    .use = hn_jupitel_thunder_storm_use,
};

const struct SkillAction skill_action_hn_mega_sonic_blow_ops = {
    .cast_action = 8,
    .use_action = 58,
    .use = hn_mega_sonic_blow_use,
};

const struct SkillAction skill_action_hn_meteor_storm_buster_ops = {
    .cast_action = 8,
    .use_action = 7,
    .skips_use_motion = 1,
    .holds_ground_pose = 1,
    .area = 7,
    .cast = hn_meteor_storm_buster_cast,
    .use_on_ground = hn_meteor_storm_buster_use_on_ground,
};

const struct SkillAction skill_action_hn_napalm_vulcan_strike_ops = {
    .cast_action = 8,
    .use_action = 59,
    .cast = hn_napalm_vulcan_strike_cast,
    .hit = hn_napalm_vulcan_strike_hit,
};

const struct SkillAction skill_action_hn_rulebreak_ops = {
    .cast_action = 8,
    .use_action = 58,
    .cast = hn_rulebreak_cast,
    .use = hn_rulebreak_use,
};

const struct SkillAction skill_action_hn_shield_chain_rush_ops = {
    .cast_action = 8,
    .use_action = 59,
    .cast = hn_shield_chain_rush_cast,
    .use = hn_shield_chain_rush_use,
};

const struct SkillAction skill_action_hn_spiral_pierce_max_ops = {
    .cast_action = 8,
    .use_action = 58,
    .cast = hn_spiral_pierce_max_cast,
    .use = hn_spiral_pierce_max_use,
};

const struct SkillObjectAction skill_object_action_hn_ground_gravitation_ops = {
    .appear = hn_ground_gravitation_appear,
    .leave = hn_ground_gravitation_leave,
};

const struct SkillObjectAction skill_object_action_hn_jack_frost_nova_ops = {
    .appear = hn_jack_frost_nova_appear,
    .leave = hn_jack_frost_nova_leave,
};
