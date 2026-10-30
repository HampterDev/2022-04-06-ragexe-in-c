#include "game/context.h"
#include "skills/skill_action_internal.h"
#include <stdlib.h>
#include "effects/skills/ez2str_effect_ex.h"

#define NW_CAST_GUN 0x93f
#define NW_CAST_GRENADE 0x33e
#define NW_CAST_CARD 0x348

static void nw_intensive_aim_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("intensiveaim_helix", caster);
}

static void efst_intensive_aim_count_on(int efst, int value, SkillActor *actor)
{
    int i;

    if (!actor)
        return;
    for (i = 0; i <= 10; i++)
        effect_mgr_remove_type(g_effectMgr, actor->aid,
                               skill_format_effect_id("intensive_aim_%02d", i));
    skill_remove_named("intensiveaim_full", actor);
    effect_mgr_spawn(g_effectMgr, actor->aid, skill_format_effect_id("intensive_aim_%02d", value),
                      skill_vec_zero(), 0, -1, 0, 0);
    if (value == 10)
        skill_spawn_on("intensiveaim_full", actor);
}

static void efst_intensive_aim_off(int efst, int value, SkillActor *actor)
{
    int i;

    if (!actor)
        return;
    for (i = 0; i <= 10; i++)
        effect_mgr_remove_type(g_effectMgr, actor->aid,
                               skill_format_effect_id("intensive_aim_%02d", i));
    skill_remove_named("intensiveaim_full", actor);
}

static void nw_grenade_fragment_use(int level, SkillActor *caster, SkillActor *target)
{
    const char *name;

    if (!caster)
        return;
    switch (level) {
    case 1:
        name = "grenade_fragment_water";
        break;
    case 2:
        name = "grenade_fragment_wind";
        break;
    case 3:
        name = "grenade_fragment_earth";
        break;
    case 4:
        name = "grenade_fragment_fire";
        break;
    case 5:
        name = "grenade_fragment_darkness";
        break;
    case 6:
        name = "grenade_fragment_saint";
        break;
    case 7:
        name = "grenade_fragment_reset";
        break;
    default:
        return;
    }
    skill_spawn_on(name, caster);
}

static void nw_the_vigilante_at_night_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, NW_CAST_GUN, cast_time);
}

static void vigilante_cracks(SkillActor *caster, SkillActor *target, int count, int spacing,
                             int scale_min, int scale_range)
{
    struct Effect *effect;
    int i, r1, r2, r;

    if (caster)
        skill_spawn_on("the_vigilante_at_night_bottom", caster);
    if (!target)
        return;
    for (i = 0; i < count; i++) {
        effect = skill_spawn_at("the_vigilante_at_night_crack", target->pos);
        if (!effect)
            return;
        r1 = rand() % spacing;
        r2 = rand() % spacing;
        effect->pos.x = (float)(i % 2 * spacing - (r2 < 0 ? 0 : r2)) + target->pos.x;
        effect->pos.z = target->pos.z + 0.0f;
        effect->pos.y = (float)(i % 4 / 2 * spacing - (r1 < 0 ? 0 : r1)) + target->pos.y;
        effect->ops->set_start_delay(effect, i << 5);
        r = rand() % (scale_range + 1);
        effect->ops->set_zoom(effect, (float)((r < 0 ? 0 : r) + scale_min) * 0.01f);
    }
}

static void nw_vigilante_gatling_use(int level, SkillActor *caster, SkillActor *target)
{
    vigilante_cracks(caster, target, 7, 15, 50, 50);
}

static void nw_vigilante_shotgun_use(int level, SkillActor *caster, SkillActor *target)
{
    vigilante_cracks(caster, target, 4, 10, 70, 30);
}

static void nw_only_one_bullet_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, NW_CAST_GUN, cast_time);
}

static void nw_only_one_bullet_cast_on_target(SkillActor *actor)
{
    if (actor)
        skill_spawn_on("only_one_bullet", actor);
}

static void nw_only_one_bullet_damage_hit(SkillActor *source, SkillActor *target)
{
    float view[12], angle;
    struct Effect *effect;

    if (!source || !target)
        return;
    if (!game_copy_view_matrix(view))
        return;
    angle = effect_screen_angle(source->pos, target->pos, view);
    effect = skill_spawn_on("only_one_bullet_hit", target);
    if (effect)
        effect->ops->set_spin(effect, angle);
}

static void nw_spiral_shooting_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, NW_CAST_GUN, cast_time);
}

static void nw_spiral_shooting_use(int level, SkillActor *caster, SkillActor *target)
{
    if (target)
        skill_spawn_on("spiral_shooting", target);
}

static void nw_magazine_for_one_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, NW_CAST_GUN, cast_time);
}

static void nw_magazine_for_one_use(int level, SkillActor *caster, SkillActor *target)
{
    if (target)
        skill_spawn_on("magazine_for_one", target);
}

static void nw_wild_fire_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, NW_CAST_GUN, cast_time);
}

static void nw_wild_fire_use_on_ground(Vec3 pos, int level, SkillActor *caster)
{
    skill_spawn_at("wild_fire", pos);
    skill_spawn_at("wild_fire_bottom", pos);
}

static void nw_basic_grenade_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, NW_CAST_GRENADE, cast_time);
}

static void nw_basic_grenade_use_on_ground(Vec3 pos, int level, SkillActor *caster)
{
    skill_spawn_at("basic_grenade", pos);
    skill_spawn_at("basic_grenade_bottom", pos);
}

static void nw_hasty_fire_in_the_hole_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, NW_CAST_GRENADE, cast_time);
}

static void nw_hasty_fire_in_the_hole_use_on_ground(Vec3 pos, int level, SkillActor *caster)
{
    struct Effect *effect;
    int i, delay;

    for (i = 0, delay = 0; delay < 0x360; i++, delay += 0x120) {
        effect = effect_mgr_spawn(g_effectMgr, 0,
                                   skill_format_effect_id("hasty_fire_in_the_hole_%d", i), pos, 0,
                                   -1, 0, 0);
        if (effect)
            effect->ops->set_start_delay(effect, delay);
        effect = effect_mgr_spawn(g_effectMgr, 0,
                                   skill_format_effect_id("hasty_fire_in_the_hole_bottom_%d", i),
                                   pos, 0, -1, 0, 0);
        if (effect)
            effect->ops->set_start_delay(effect, delay);
    }
}

static void nw_grenades_dropping_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, NW_CAST_GRENADE, cast_time);
}

static void nw_grenades_dropping_appear(int level, SkillActor *unit, SkillActor *owner)
{
    if (unit)
        effect_mgr_spawn(g_effectMgr, unit->aid, 0x92f, unit->pos, 0, level, 0, 0);
}

static void nw_auto_firing_launcher_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, NW_CAST_GRENADE, cast_time);
}

static void nw_auto_firing_launcher_use(int level, SkillActor *caster, SkillActor *target)
{
    if (!target)
        return;
    skill_spawn_on("auto_firing_launcher", target);
    skill_spawn_on("auto_firing_launcher_bottom", target);
}

static void nw_hidden_card_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, NW_CAST_CARD, cast_time);
}

static void nw_hidden_card_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("hiddencard", caster);
}

static void efst_hidden_card_on(int efst, int value, SkillActor *actor)
{
    if (actor)
        skill_spawn_on("hiddencard_thunder", actor);
}

static void efst_hidden_card_off(int efst, int value, SkillActor *actor)
{
    if (actor)
        skill_remove_named("hiddencard_thunder", actor);
}

static void nw_mission_bombard_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, NW_CAST_CARD, cast_time);
}

static void nw_mission_bombard_appear(int level, SkillActor *unit, SkillActor *owner)
{
    if (!unit)
        return;
    skill_spawn_on("mission_bombard", unit);
    skill_spawn_on("mission_bombard_fire", unit);
    skill_spawn_on("mission_bombard_fire_bottom", unit);
    effect_mgr_spawn(g_effectMgr, unit->aid, 0x932, unit->pos, 0, -1, 0, 0);
}

const struct StatusAction efst_action_hidden_card_ops = {
    .on = efst_hidden_card_on,
    .off = efst_hidden_card_off,
    .reapply = efst_hidden_card_on,
};

const struct StatusAction efst_action_intensive_aim_ops = {
    .off = efst_intensive_aim_off,
};

const struct StatusAction efst_action_intensive_aim_count_ops = {
    .on = efst_intensive_aim_count_on,
    .reapply = efst_intensive_aim_count_on,
};

const struct SkillAction skill_action_nw_auto_firing_launcher_ops = {
    .cast_action = 8,
    .use_action = 2,
    .cast = nw_auto_firing_launcher_cast,
    .use = nw_auto_firing_launcher_use,
};

const struct SkillAction skill_action_nw_basic_grenade_ops = {
    .cast_action = 8,
    .use_action = 2,
    .area = 3,
    .cast = nw_basic_grenade_cast,
    .use_on_ground = nw_basic_grenade_use_on_ground,
};

const struct SkillAction skill_action_nw_grenades_dropping_ops = {
    .cast_action = 8,
    .use_action = 2,
    .skips_use_motion = 1,
    .area = 3,
    .cast = nw_grenades_dropping_cast,
};

const struct SkillAction skill_action_nw_grenade_fragment_ops = {
    .cast_action = 8,
    .use_action = 7,
    .use = nw_grenade_fragment_use,
};

const struct SkillAction skill_action_nw_hasty_fire_in_the_hole_ops = {
    .cast_action = 8,
    .use_action = 2,
    .skips_use_motion = 1,
    .area = 5,
    .cast = nw_hasty_fire_in_the_hole_cast,
    .use_on_ground = nw_hasty_fire_in_the_hole_use_on_ground,
};

const struct SkillAction skill_action_nw_hidden_card_ops = {
    .cast_action = 8,
    .use_action = 2,
    .cast = nw_hidden_card_cast,
    .use = nw_hidden_card_use,
};

const struct SkillAction skill_action_nw_intensive_aim_ops = {
    .cast_action = 8,
    .use_action = 2,
    .use = nw_intensive_aim_use,
};

const struct SkillAction skill_action_nw_magazine_for_one_ops = {
    .cast_action = 8,
    .use_action = 2,
    .cast = nw_magazine_for_one_cast,
    .use = nw_magazine_for_one_use,
};

const struct SkillAction skill_action_nw_mission_bombard_ops = {
    .cast_action = 8,
    .use_action = 2,
    .skips_use_motion = 1,
    .area = 9,
    .cast = nw_mission_bombard_cast,
};

const struct SkillAction skill_action_nw_only_one_bullet_ops = {
    .cast_action = 8,
    .use_action = 2,
    .cast = nw_only_one_bullet_cast,
    .cast_on_target = nw_only_one_bullet_cast_on_target,
    .damage_hit = nw_only_one_bullet_damage_hit,
};

const struct SkillAction skill_action_nw_spiral_shooting_ops = {
    .cast_action = 8,
    .use_action = 2,
    .cast = nw_spiral_shooting_cast,
    .use = nw_spiral_shooting_use,
};

const struct SkillAction skill_action_nw_the_vigilante_at_night_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = nw_the_vigilante_at_night_cast,
};

const struct SkillAction skill_action_nw_the_vigilante_at_night_gun_gatling_ops = {
    .cast_action = 8,
    .use_action = 2,
    .use = nw_vigilante_gatling_use,
};

const struct SkillAction skill_action_nw_the_vigilante_at_night_gun_shotgun_ops = {
    .cast_action = 8,
    .use_action = 2,
    .use = nw_vigilante_shotgun_use,
};

const struct SkillAction skill_action_nw_wild_fire_ops = {
    .cast_action = 8,
    .use_action = 2,
    .area = 5,
    .cast = nw_wild_fire_cast,
    .use_on_ground = nw_wild_fire_use_on_ground,
};

const struct SkillObjectAction skill_object_action_nw_grenades_dropping_ops = {
    .appear = nw_grenades_dropping_appear,
};

const struct SkillObjectAction skill_object_action_nw_mission_bombard_ops = {
    .appear = nw_mission_bombard_appear,
};
