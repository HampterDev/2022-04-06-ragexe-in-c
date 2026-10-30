#include "skills/skill_action_internal.h"
#include "effects/skills/skill_view.h"

#define EFFECT_TALISMAN_MARK 0x933

#define SOA_TALISMAN(cls, cast, name)                                                              \
    static void cls##_Cast(unsigned cast_time, SkillActor *caster)                                 \
    {                                                                                              \
        skill_cast_effect(caster, (cast), cast_time);                                              \
    }                                                                                              \
    static void cls##_Use(int level, SkillActor *caster, SkillActor *target)                       \
    {                                                                                              \
        if (target)                                                                                \
            skill_spawn_on((name), target);                                                        \
    }

SOA_TALISMAN(SOA_TALISMAN_OF_PROTECTION, 0x33e, "talisman_of_protection")
SOA_TALISMAN(SOA_TALISMAN_OF_WARRIOR, 0x33e, "talisman_of_warrior")
SOA_TALISMAN(SOA_TALISMAN_OF_FIVE_ELEMENTS, 0x33e, "talisman_of_the_five_elements")

static void soa_talisman_of_magician_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x33e, cast_time);
}

static void soa_talisman_of_magician_use(int level, SkillActor *caster, SkillActor *target)
{
    if (!target)
        return;
    skill_spawn_on("talisman_of_magician", target);
    skill_spawn_on("talisman_of_magician_bottom", target);
}

#define SOA_SKILL(cls, cast, name)                                                                 \
    static void cls##_Cast(unsigned cast_time, SkillActor *caster)                                 \
    {                                                                                              \
        skill_cast_effect(caster, (cast), cast_time);                                              \
    }                                                                                              \
    static void cls##_Use(int level, SkillActor *caster, SkillActor *target)                       \
    {                                                                                              \
        if (target)                                                                                \
            skill_spawn_on((name), target);                                                        \
    }

SOA_SKILL(SOA_SOUL_GATHERING, 0x1c6, "soul_gathering")
SOA_SKILL(SOA_EXORCISM_OF_MALICIOUS_SOUL, 0x93f, "exorcism_of_malicuous_soul")
SOA_SKILL(SOA_TALISMAN_OF_BLUE_DRAGON, 0x34d, "talisman_of_blue_dragon")
SOA_SKILL(SOA_TALISMAN_OF_WHITE_TIGER, 0x34b, "talisman_of_white_tiger")
SOA_SKILL(SOA_TALISMAN_OF_FOUR_BEARING_GOD, 0x941, "talisman_of_four_bearing_god")
SOA_SKILL(SOA_CIRCLE_OF_DIRECTIONS_AND_ELEMENTALS, 0x61d, "circle_directions_elements")

static void soa_talisman_of_red_phoenix_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x348, cast_time);
}

static void soa_talisman_of_red_phoenix_use(int level, SkillActor *caster, SkillActor *target)
{
    if (!target)
        return;
    skill_spawn_on("talisman_of_red_phoenix_bottom", target);
    skill_spawn_on("talisman_of_red_phoenix", target);
}

static void soa_talisman_of_black_tortoise_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x940, cast_time);
}

static void soa_talisman_of_black_tortoise_use_on_ground(Vec3 pos, int level, SkillActor *caster)
{
    skill_spawn_at("talisman_of_black_tortoise_bottom", pos);
    skill_spawn_at("talisman_of_black_tortoise", pos);
}

static void soa_talisman_of_soul_stealing_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x1b9, cast_time);
}

static void soa_talisman_of_soul_stealing_use(int level, SkillActor *caster, SkillActor *target)
{
    if (!target)
        return;
    skill_launch(target, 0xfd);
    render_object_send(target, 0, 0x58, (RenderMsgArg)level, 0xff0000ffu, 0x72, 0);
}

static void soa_talisman_of_soul_stealing_hit(SkillActor *caster, SkillActor *target,
                                              SkillActor *target_unit)
{
    if (caster)
        skill_spawn_on("talisman_of_soul_stealing", caster);
    if (target)
        skill_spawn_on("talisman_of_magician", target);
}

static void soa_totem_of_tutelary_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x38f, cast_time);
}

static void soa_totem_of_tutelary_appear(int level, SkillActor *unit, SkillActor *owner)
{
    if (!unit)
        return;
    skill_spawn_on("totem_of_tutelary_bottom_start", unit);
    skill_spawn_on("totem_of_tutelary_bottom_loop", unit);
    skill_spawn_on("totem_of_tutelary_start", unit);
    skill_spawn_on("totem_of_tutelary_loop", unit);
}

static void soa_totem_of_tutelary_leave(SkillActor *unit)
{
    if (!unit)
        return;
    skill_remove_named("totem_of_tutelary_bottom_loop", unit);
    skill_spawn_at("totem_of_tutelary_bottom_end", unit->pos);
    skill_remove_named("totem_of_tutelary_loop", unit);
    skill_spawn_at("totem_of_tutelary_end", unit->pos);
}

#define SOA_GOD(name, god)                                                                         \
    static void name##_On(int efst, int value, SkillActor *actor)                                  \
    {                                                                                              \
        if (!actor)                                                                                \
            return;                                                                                \
        effect_mgr_remove_type(g_effectMgr, actor->aid, EFFECT_TALISMAN_MARK);                     \
        effect_mgr_spawn(g_effectMgr, actor->aid, EFFECT_TALISMAN_MARK, actor->pos, 0, (god), 0,  \
                          0);                                                                      \
    }                                                                                              \
    static void name##_Off(int efst, int value, SkillActor *actor)                                 \
    {                                                                                              \
        struct Effect *mark;                                                                      \
        if (!actor)                                                                                \
            return;                                                                                \
        mark = effect_mgr_find_effect(g_effectMgr, actor->aid, EFFECT_TALISMAN_MARK);              \
        if (mark)                                                                                  \
            talisman_mark_effect_finish((struct TalismanMarkEffect *)mark);                                                  \
    }

SOA_GOD(EfstFirstGod, 1)
SOA_GOD(EfstSecondGod, 2)
SOA_GOD(EfstThirdGod, 3)
SOA_GOD(EfstFourthGod, 4)

#define SOA_BUFF(name, effect)                                                                     \
    static void name##_On(int efst, int value, SkillActor *actor)                                  \
    {                                                                                              \
        if (actor)                                                                                 \
            skill_spawn_on((effect), actor);                                                       \
    }                                                                                              \
    static void name##_Off(int efst, int value, SkillActor *actor)                                 \
    {                                                                                              \
        if (actor)                                                                                 \
            skill_remove_named((effect), actor);                                                   \
    }

SOA_BUFF(EfstFifthGod, "circle_directions_elements_loop")
SOA_BUFF(EfstHeavenAndEarth, "soul_of_heaven_and_earth_buff")

static void soa_soul_of_heaven_and_earth_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x614, cast_time);
}

static void soa_soul_of_heaven_and_earth_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("soul_of_heaven_and_earth", caster);
}

const struct StatusAction efst_action_heaven_and_earth_ops = {
    .on = EfstHeavenAndEarth_On,
    .off = EfstHeavenAndEarth_Off,
    .reapply = EfstHeavenAndEarth_On,
};

const struct StatusAction efst_action_t_first_god_ops = {
    .on = EfstFirstGod_On,
    .off = EfstFirstGod_Off,
    .reapply = EfstFirstGod_On,
};

const struct StatusAction efst_action_t_fiveth_god_ops = {
    .on = EfstFifthGod_On,
    .off = EfstFifthGod_Off,
    .reapply = EfstFifthGod_On,
};

const struct StatusAction efst_action_t_fourth_god_ops = {
    .on = EfstFourthGod_On,
    .off = EfstFourthGod_Off,
    .reapply = EfstFourthGod_On,
};

const struct StatusAction efst_action_t_second_god_ops = {
    .on = EfstSecondGod_On,
    .off = EfstSecondGod_Off,
    .reapply = EfstSecondGod_On,
};

const struct StatusAction efst_action_t_third_god_ops = {
    .on = EfstThirdGod_On,
    .off = EfstThirdGod_Off,
    .reapply = EfstThirdGod_On,
};

const struct SkillAction skill_action_soa_circle_of_directions_and_elementals_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = SOA_CIRCLE_OF_DIRECTIONS_AND_ELEMENTALS_Cast,
    .use = SOA_CIRCLE_OF_DIRECTIONS_AND_ELEMENTALS_Use,
};

const struct SkillAction skill_action_soa_exorcism_of_malicious_soul_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = SOA_EXORCISM_OF_MALICIOUS_SOUL_Cast,
    .use = SOA_EXORCISM_OF_MALICIOUS_SOUL_Use,
};

const struct SkillAction skill_action_soa_soul_gathering_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = SOA_SOUL_GATHERING_Cast,
    .use = SOA_SOUL_GATHERING_Use,
};

const struct SkillAction skill_action_soa_soul_of_heaven_and_earth_ops = {
    .is_good_for_actor = 1,
    .cast_action = 8,
    .use_action = 7,
    .cast = soa_soul_of_heaven_and_earth_cast,
    .use = soa_soul_of_heaven_and_earth_use,
};

const struct SkillAction skill_action_soa_talisman_of_black_tortoise_ops = {
    .cast_action = 8,
    .use_action = 7,
    .area = 9,
    .cast = soa_talisman_of_black_tortoise_cast,
    .use_on_ground = soa_talisman_of_black_tortoise_use_on_ground,
};

const struct SkillAction skill_action_soa_talisman_of_blue_dragon_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = SOA_TALISMAN_OF_BLUE_DRAGON_Cast,
    .use = SOA_TALISMAN_OF_BLUE_DRAGON_Use,
};

const struct SkillAction skill_action_soa_talisman_of_five_elements_ops = {
    .is_good_for_actor = 1,
    .cast_action = 8,
    .use_action = 7,
    .cast = SOA_TALISMAN_OF_FIVE_ELEMENTS_Cast,
    .use = SOA_TALISMAN_OF_FIVE_ELEMENTS_Use,
};

const struct SkillAction skill_action_soa_talisman_of_four_bearing_god_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = SOA_TALISMAN_OF_FOUR_BEARING_GOD_Cast,
    .use = SOA_TALISMAN_OF_FOUR_BEARING_GOD_Use,
};

const struct SkillAction skill_action_soa_talisman_of_magician_ops = {
    .is_good_for_actor = 1,
    .cast_action = 8,
    .use_action = 7,
    .cast = soa_talisman_of_magician_cast,
    .use = soa_talisman_of_magician_use,
};

const struct SkillAction skill_action_soa_talisman_of_protection_ops = {
    .is_good_for_actor = 1,
    .cast_action = 8,
    .use_action = 7,
    .cast = SOA_TALISMAN_OF_PROTECTION_Cast,
    .use = SOA_TALISMAN_OF_PROTECTION_Use,
};

const struct SkillAction skill_action_soa_talisman_of_red_phoenix_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = soa_talisman_of_red_phoenix_cast,
    .use = soa_talisman_of_red_phoenix_use,
};

const struct SkillAction skill_action_soa_talisman_of_soul_stealing_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = soa_talisman_of_soul_stealing_cast,
    .use = soa_talisman_of_soul_stealing_use,
    .hit = soa_talisman_of_soul_stealing_hit,
};

const struct SkillAction skill_action_soa_talisman_of_warrior_ops = {
    .is_good_for_actor = 1,
    .cast_action = 8,
    .use_action = 7,
    .cast = SOA_TALISMAN_OF_WARRIOR_Cast,
    .use = SOA_TALISMAN_OF_WARRIOR_Use,
};

const struct SkillAction skill_action_soa_talisman_of_white_tiger_ops = {
    .cast_action = 8,
    .use_action = 7,
    .cast = SOA_TALISMAN_OF_WHITE_TIGER_Cast,
    .use = SOA_TALISMAN_OF_WHITE_TIGER_Use,
};

const struct SkillAction skill_action_soa_totem_of_tutelary_ops = {
    .cast_action = 8,
    .use_action = 7,
    .area = 7,
    .cast = soa_totem_of_tutelary_cast,
};

const struct SkillObjectAction skill_object_action_soa_totem_of_tutelary_ops = {
    .appear = soa_totem_of_tutelary_appear,
    .leave = soa_totem_of_tutelary_leave,
};
