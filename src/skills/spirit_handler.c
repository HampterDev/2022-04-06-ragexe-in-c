#include "game/context.h"
#include "skills/skill_action_internal.h"
#include "effects/skills/ez2str_effect_ex.h"

static void sh_chul_ho_sonic_claw_use(int level, SkillActor *caster, SkillActor *target)
{
    static const int waves[12] = {12, 1, 1, 3, 5, 5, 6, 7, 7, 9, 11, 11};
    float view[12], angle;
    int sector;

    if (!caster)
        return;
    skill_spawn_on("chul_ho_sonic_claw", caster);
    if (!target)
        return;
    if (!game_copy_view_matrix(view))
        return;
    angle = effect_screen_angle(caster->pos, target->pos, view);
    sector = (int)(angle + 105.0f) % 360 / 30;
    if (sector < 0 || (unsigned)sector >= 12)
        return;
    effect_mgr_spawn(g_effectMgr, caster->aid,
                      skill_format_effect_id("chul_ho_sonic_claw_wave_%d", waves[sector]),
                      skill_vec_zero(), 0, -1, 0, 0);
}

static void sh_howling_of_chul_ho_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x348, cast_time);
}

static void sh_howling_of_chul_ho_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("howling_of_chul_ho", caster);
}

static void efst_hogogong_on(int efst, int value, SkillActor *actor)
{
    if (actor)
        skill_spawn_on("howling_of_chul_ho_scared", actor);
}

static void efst_hogogong_off(int efst, int value, SkillActor *actor)
{
    if (actor)
        skill_remove_named("howling_of_chul_ho_scared", actor);
}

static void sh_hogogong_strike_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x348, cast_time);
}

static void sh_hogogong_strike_hit(SkillActor *caster, SkillActor *target, SkillActor *target_unit)
{
    if (target)
        skill_spawn_on("hogogong_strike", target);
}

static void sh_ki_sul_water_spraying_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x3a, cast_time);
}

static void sh_ki_sul_water_spraying_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("ki_sul_water_spraying", caster);
}

static void sh_marine_festival_of_ki_sul_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x34d, cast_time);
}

static void sh_marine_festival_of_ki_sul_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("marine_festival_of_ki_sul_bottom", caster);
}

static void efst_marine_festival_on(int efst, int value, SkillActor *actor)
{
    if (actor)
        skill_spawn_on("marine_festival_of_ki_sul", actor);
}

static void sh_sandy_festival_of_ki_sul_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x34a, cast_time);
}

static void sh_sandy_festival_of_ki_sul_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("sandy_festival_of_ki_sul_bottom", caster);
}

static void efst_sandy_festival_on(int efst, int value, SkillActor *actor)
{
    if (actor)
        skill_spawn_on("sandy_festival_of_ki_sul", actor);
}

static void sh_ki_sul_rampage_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x33f, cast_time);
}

static void sh_ki_sul_rampage_use(int level, SkillActor *caster, SkillActor *target)
{
    if (!caster)
        return;
    skill_spawn_on("ki_sul_rampage_bottom", caster);
    skill_spawn_on("ki_sul_rampage", caster);
}

static void sh_colors_of_hyun_rok_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x3a, cast_time);
}

static void sh_colors_of_hyun_rok_use(int level, SkillActor *caster, SkillActor *target)
{
    const char *name;

    if (!caster)
        return;
    switch (level) {
    case 1:
        name = "colors_of_hyun_rok_water";
        break;
    case 2:
        name = "colors_of_hyun_rok_wind";
        break;
    case 3:
        name = "colors_of_hyun_rok_earth";
        break;
    case 4:
        name = "colors_of_hyun_rok_fire";
        break;
    case 5:
        name = "colors_of_hyun_rok_darkness";
        break;
    case 6:
        name = "colors_of_hyun_rok_saint";
        break;
    case 7:
        name = "colors_of_hyun_rok_reset";
        break;
    default:
        return;
    }
    skill_spawn_on(name, caster);
}

static void sh_hyun_roks_breeze_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x349, cast_time);
}

static void sh_hyun_roks_breeze_appear(int level, SkillActor *unit, SkillActor *owner)
{
    if (!unit)
        return;
    skill_spawn_on("hyun_roks_breeze_bottom", unit);
    skill_spawn_on("hyun_roks_breeze", unit);
}

static void sh_hyun_rok_cannon_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x349, cast_time);
}

static void sh_hyun_rok_cannon_use(int level, SkillActor *caster, SkillActor *target)
{
    float view[12], angle;
    struct Effect *effect;

    if (!caster || !target)
        return;
    if (!game_copy_view_matrix(view))
        return;
    angle = effect_screen_angle(caster->pos, target->pos, view);
    effect = skill_spawn_at("hyun_rok_cannon", caster->pos);
    if (effect)
        effect->ops->set_spin(effect, angle);
}

static void sh_temporary_communion_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x93f, cast_time);
}

static void sh_temporary_communion_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("temporary_communion", caster);
}

static void efst_temporary_communion_on(int efst, int value, SkillActor *actor)
{
    if (!actor)
        return;
    skill_spawn_on("temporary_communion_buff_bottom", actor);
    skill_spawn_on("temporary_communion_buff", actor);
}

static void efst_temporary_communion_off(int efst, int value, SkillActor *actor)
{
    if (!actor)
        return;
    skill_remove_named("temporary_communion_buff_bottom", actor);
    skill_remove_named("temporary_communion_buff", actor);
}

static void sh_blessing_of_mystical_creatures_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x93f, cast_time);
}

static void sh_blessing_of_mystical_creatures_use(int level, SkillActor *caster, SkillActor *target)
{
    if (target)
        skill_spawn_on("blessing_of_mystical_creatures", target);
}

const struct StatusAction efst_action_hogogong_ops = {
    .on = efst_hogogong_on,
    .off = efst_hogogong_off,
    .reapply = efst_hogogong_on,
};

const struct StatusAction efst_action_marine_festival_ops = {
    .on = efst_marine_festival_on,
};

const struct StatusAction efst_action_sandy_festival_ops = {
    .on = efst_sandy_festival_on,
};

const struct StatusAction efst_action_temporary_communion_ops = {
    .on = efst_temporary_communion_on,
    .off = efst_temporary_communion_off,
    .reapply = efst_temporary_communion_on,
};

const struct SkillAction skill_action_sh_blessing_of_mystical_creatures_ops = {
    .is_good_for_actor = 1,
    .cast_action = 8,
    .use_action = 7,
    .cast = sh_blessing_of_mystical_creatures_cast,
    .use = sh_blessing_of_mystical_creatures_use,
};

const struct SkillAction skill_action_sh_chul_ho_sonic_claw_ops = {
    .cast_action = 8,
    .use_action = 54,
    .use = sh_chul_ho_sonic_claw_use,
};

const struct SkillAction skill_action_sh_colors_of_hyun_rok_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = sh_colors_of_hyun_rok_cast,
    .use = sh_colors_of_hyun_rok_use,
};

const struct SkillAction skill_action_sh_hogogong_strike_ops = {
    .cast_action = 8,
    .use_action = 54,
    .cast = sh_hogogong_strike_cast,
    .hit = sh_hogogong_strike_hit,
};

const struct SkillAction skill_action_sh_howling_of_chul_ho_ops = {
    .cast_action = 8,
    .use_action = 54,
    .cast = sh_howling_of_chul_ho_cast,
    .use = sh_howling_of_chul_ho_use,
};

const struct SkillAction skill_action_sh_hyun_roks_breeze_ops = {
    .cast_action = 8,
    .use_action = 7,
    .skips_use_motion = 1,
    .area = 9,
    .cast = sh_hyun_roks_breeze_cast,
};

const struct SkillAction skill_action_sh_hyun_rok_cannon_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = sh_hyun_rok_cannon_cast,
    .use = sh_hyun_rok_cannon_use,
};

const struct SkillAction skill_action_sh_ki_sul_rampage_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = sh_ki_sul_rampage_cast,
    .use = sh_ki_sul_rampage_use,
};

const struct SkillAction skill_action_sh_ki_sul_water_spraying_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = sh_ki_sul_water_spraying_cast,
    .use = sh_ki_sul_water_spraying_use,
};

const struct SkillAction skill_action_sh_marine_festival_of_ki_sul_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = sh_marine_festival_of_ki_sul_cast,
    .use = sh_marine_festival_of_ki_sul_use,
};

const struct SkillAction skill_action_sh_sandy_festival_of_ki_sul_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = sh_sandy_festival_of_ki_sul_cast,
    .use = sh_sandy_festival_of_ki_sul_use,
};

const struct SkillAction skill_action_sh_temporary_communion_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = sh_temporary_communion_cast,
    .use = sh_temporary_communion_use,
};

const struct SkillObjectAction skill_object_action_sh_hyun_roks_breeze_ops = {
    .appear = sh_hyun_roks_breeze_appear,
};
