#include "skills/skill_action_internal.h"
#include <stdlib.h>

#define SKE_STANCE(name, first, second)                                                            \
    static void name##_On(int efst, int value, SkillActor *actor)                                  \
    {                                                                                              \
        if (!actor)                                                                                \
            return;                                                                                \
        skill_spawn_on(first, actor);                                                              \
        skill_spawn_on(second, actor);                                                             \
    }                                                                                              \
    static void name##_Off(int efst, int value, SkillActor *actor)                                 \
    {                                                                                              \
        if (actor)                                                                                 \
            skill_remove_named(second, actor);                                                     \
    }                                                                                              \
    static void name##_Reapply(int efst, int value, SkillActor *actor)                             \
    {                                                                                              \
        if (actor)                                                                                 \
            skill_spawn_on(second, actor);                                                         \
    }

SKE_STANCE(EfstRisingSun, "ske_rising_sun_00", "ske_rising_sun_01")
SKE_STANCE(EfstNoonSun, "ske_rising_sun_02", "ske_rising_sun_03")
SKE_STANCE(EfstSunsetSun, "ske_rising_sun_04", "ske_rising_sun_05")
SKE_STANCE(EfstRisingMoon, "ske_rising_moon_00", "ske_rising_moon_01")
SKE_STANCE(EfstMidnightMoon, "ske_rising_moon_02", "ske_rising_moon_03")
SKE_STANCE(EfstDawnMoon, "ske_rising_moon_04", "ske_rising_moon_05")
SKE_STANCE(EfstSkyEnchant, "ske_enchanting_sky_00", "ske_enchanting_sky_01")

static void ske_noon_blast_use(int level, SkillActor *caster, SkillActor *target)
{
    if (target)
        skill_spawn_on("ske_noon_blast", target);
}

static void ske_sunset_blast_use(int level, SkillActor *caster, SkillActor *target)
{
    struct Effect *effect;
    int delay, r1, r2;

    if (!target)
        return;
    skill_spawn_on("sunset_blast", target);
    for (delay = 0; delay < 0xc0; delay += 0x20) {
        effect = skill_spawn_at("sunset_blast_small", target->pos);
        if (!effect)
            return;
        r1 = rand() % 30;
        r2 = rand() % 30;
        effect->pos.x = (float)(15 - (r2 < 0 ? 0 : r2)) + target->pos.x;
        effect->pos.y = target->pos.y + 0.0f;
        effect->pos.z = (float)(15 - (r1 < 0 ? 0 : r1)) + target->pos.z;
        effect->ops->set_start_delay(effect, delay);
    }
}

static void ske_midnight_kick_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x3a, cast_time);
}

static void ske_midnight_kick_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("ske_midnight_kick", caster);
}

static void ske_dawn_break_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x3a, cast_time);
}

static void ske_dawn_break_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("ske_dawn_break", caster);
}

#define SKE_CAST_STAR 0x1b9

static void ske_twinkling_galaxy_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, SKE_CAST_STAR, cast_time);
}

static void ske_twinkling_galaxy_appear(int level, SkillActor *unit, SkillActor *owner)
{
    if (unit)
        effect_mgr_spawn(g_effectMgr, unit->aid, 0x92d, unit->pos, 0, level, 0, 0);
}

static void ske_star_burst_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, SKE_CAST_STAR, cast_time);
}

static void efst_star_burst_on(int efst, int value, SkillActor *actor)
{
    if (actor)
        effect_mgr_spawn(g_effectMgr, actor->aid, 0x92c, skill_vec_zero(), 0, -1, 0, 0);
}

static void efst_star_burst_off(int efst, int value, SkillActor *actor)
{
    if (actor)
        effect_mgr_remove_type(g_effectMgr, actor->aid, 0x92c);
}

static void ske_star_cannon_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, SKE_CAST_STAR, cast_time);
}

static void ske_star_cannon_appear(int level, SkillActor *unit, SkillActor *owner)
{
    if (unit)
        effect_mgr_spawn(g_effectMgr, unit->aid, 0x92e, unit->pos, 0, level, 0, 0);
}

static void ske_all_in_the_sky_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x34d, cast_time);
}

static void ske_all_in_the_sky_cast_on_target(SkillActor *actor)
{
    if (actor)
        skill_spawn_on("ske_all_in_the_sky", actor);
}

static void ske_enchanting_sky_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x33f, cast_time);
}

const struct StatusAction efst_action_dawn_moon_ops = {
    .on = EfstDawnMoon_On,
    .off = EfstDawnMoon_Off,
    .reapply = EfstDawnMoon_Reapply,
};

const struct StatusAction efst_action_midnight_moon_ops = {
    .on = EfstMidnightMoon_On,
    .off = EfstMidnightMoon_Off,
    .reapply = EfstMidnightMoon_Reapply,
};

const struct StatusAction efst_action_noon_sun_ops = {
    .on = EfstNoonSun_On,
    .off = EfstNoonSun_Off,
    .reapply = EfstNoonSun_Reapply,
};

const struct StatusAction efst_action_rising_moon_ops = {
    .on = EfstRisingMoon_On,
    .off = EfstRisingMoon_Off,
    .reapply = EfstRisingMoon_Reapply,
};

const struct StatusAction efst_action_rising_sun_ops = {
    .on = EfstRisingSun_On,
    .off = EfstRisingSun_Off,
    .reapply = EfstRisingSun_Reapply,
};

const struct StatusAction efst_action_sky_enchant_ops = {
    .on = EfstSkyEnchant_On,
    .off = EfstSkyEnchant_Off,
    .reapply = EfstSkyEnchant_Reapply,
};

const struct StatusAction efst_action_star_burst_ops = {
    .on = efst_star_burst_on,
    .off = efst_star_burst_off,
    .reapply = efst_star_burst_on,
};

const struct StatusAction efst_action_sunset_sun_ops = {
    .on = EfstSunsetSun_On,
    .off = EfstSunsetSun_Off,
    .reapply = EfstSunsetSun_Reapply,
};

const struct SkillAction skill_action_ske_all_in_the_sky_ops = {
    .cast_action = 8,
    .use_action = 53,
    .cast = ske_all_in_the_sky_cast,
    .cast_on_target = ske_all_in_the_sky_cast_on_target,
};

const struct SkillAction skill_action_ske_dawn_break_ops = {
    .cast_action = 8,
    .use_action = 53,
    .cast = ske_dawn_break_cast,
    .use = ske_dawn_break_use,
};

const struct SkillAction skill_action_ske_enchanting_sky_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = ske_enchanting_sky_cast,
};

const struct SkillAction skill_action_ske_midnight_kick_ops = {
    .cast_action = 8,
    .use_action = 53,
    .cast = ske_midnight_kick_cast,
    .use = ske_midnight_kick_use,
};

const struct SkillAction skill_action_ske_noon_blast_ops = {
    .cast_action = 8,
    .use_action = 52,
    .use = ske_noon_blast_use,
};

const struct SkillAction skill_action_ske_rising_moon_ops = {
    .cast_action = 8,
    .use_action = 53,
};

const struct SkillAction skill_action_ske_rising_sun_ops = {
    .cast_action = 8,
    .use_action = 52,
};

const struct SkillAction skill_action_ske_star_burst_ops = {
    .cast_action = 8,
    .use_action = 7,
    .skips_use_motion = 1,
    .cast = ske_star_burst_cast,
};

const struct SkillAction skill_action_ske_star_cannon_ops = {
    .cast_action = 8,
    .use_action = 7,
    .skips_use_motion = 1,
    .skips_attack_delay = 1,
    .cast = ske_star_cannon_cast,
};

const struct SkillAction skill_action_ske_sunset_blast_ops = {
    .cast_action = 8,
    .use_action = 52,
    .use = ske_sunset_blast_use,
};

const struct SkillAction skill_action_ske_twinkling_galaxy_ops = {
    .cast_action = 8,
    .use_action = 7,
    .skips_use_motion = 1,
    .skips_attack_delay = 1,
    .cast = ske_twinkling_galaxy_cast,
};

const struct SkillObjectAction skill_object_action_ske_star_cannon_ops = {
    .appear = ske_star_cannon_appear,
};

const struct SkillObjectAction skill_object_action_ske_twinkling_galaxy_ops = {
    .appear = ske_twinkling_galaxy_appear,
};
