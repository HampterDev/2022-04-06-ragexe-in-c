#include "session/state.h"
#include "game/context.h"
#include "skills/skill_action_internal.h"
#include <stdlib.h>
#include "effects/skills/ez2str_effect_ex.h"
#include "actor/actor.h"

#define SS_CAST_KUNAI 0x1c6
#define SS_CAST_KUNAI_RING 0x3da
#define SS_CAST_SPELL_ACT 0x32c
#define SS_CAST_SPELL 0x678

static void kunai_cast(SkillActor *caster, unsigned cast_time)
{
    if (!caster)
        return;
    effect_mgr_spawn(g_effectMgr, caster->aid, SS_CAST_KUNAI_RING, skill_vec_zero(), 0, -1, 0, 0);
    game_actor_launch_effect_timed((struct GameActor *)caster, SS_CAST_KUNAI, cast_time >> 4);
}

static void akumu_cast(SkillActor *caster, unsigned cast_time)
{
    if (!caster)
        return;
    skill_launch(caster, SS_CAST_SPELL_ACT);
    game_actor_launch_effect_timed((struct GameActor *)caster, SS_CAST_SPELL, cast_time >> 4);
}

static void spawn_turned_quadrant(const char *name, SkillActor *caster, Vec3 to)
{
    float angle = effect_screen_angle_quadrant(caster->pos, to);
    struct Effect *effect = skill_spawn_at(name, caster->pos);

    if (effect)
        effect->ops->set_spin(effect, angle);
}

static void spawn_turned(const char *name, SkillActor *caster, Vec3 to)
{
    float view[12], angle;
    struct Effect *effect;

    if (!game_copy_view_matrix(view))
        return;
    angle = effect_screen_angle(caster->pos, to, view);
    effect = skill_spawn_at(name, caster->pos);
    if (effect)
        effect->ops->set_spin(effect, angle);
}

static void efst_nightmare_on(int efst, int value, SkillActor *actor)
{
    if (actor)
        skill_spawn_on("efst_nightmare", actor);
}

static void efst_nightmare_off(int efst, int value, SkillActor *actor)
{
    if (actor)
        skill_remove_named("efst_nightmare", actor);
}

static void efst_shadow_clock_on(int efst, int value, SkillActor *actor)
{
    skill_spawn_at("kagekakusi", actor->pos);
    skill_launch(actor, 0x3c3);
}

static void efst_shadow_clock_off(int efst, int value, SkillActor *actor)
{
    if (!actor)
        return;
    while (able_to_make_effect_is_effect_exist(actor, 0x3c3))
        render_object_send(actor, 0, 0x6d, 0x3c3, 0, 0, 0);
}

static void efst_shadow_clock_reapply(int efst, int value, SkillActor *actor)
{
    if (actor)
        skill_launch(actor, 0x3c3);
}

static void ss_sbunshin_appear(int level, SkillActor *unit, SkillActor *owner)
{
    if (!unit)
        return;
    skill_spawn_on("shinkirou_idle_start", unit);
    skill_spawn_on("shinkirou_idle_loop", unit);
}

static void ss_sbunshin_leave(SkillActor *unit)
{
    if (!unit)
        return;
    skill_remove_named("shinkirou_idle_start", unit);
    skill_remove_named("shinkirou_idle_loop", unit);
    skill_spawn_at("shinkirou_idle_end", unit->pos);
}

static void ss_kunaitrap_appear(int level, SkillActor *unit, SkillActor *owner)
{
    if (!unit)
        return;
    skill_spawn_on("kunaiarea_01_start", unit);
    skill_spawn_on("kunaiarea_01_loop", unit);
}

static void ss_kunaitrap_leave(SkillActor *unit)
{
    if (!unit)
        return;
    skill_remove_named("kunaiarea_01_loop", unit);
    skill_spawn_at("kunaiarea_01_end", unit->pos);
}

static void npc_fuuma_appear(int level, SkillActor *unit, SkillActor *owner)
{
    if (!unit || !owner)
        return;
    effect_mgr_spawn(g_effectMgr, unit->aid, 0x930, owner->pos, unit->aid, -1, 0, 0);
}

static void ss_tokedasu_use_on_ground(Vec3 pos, int level, SkillActor *caster)
{
    skill_spawn_at("tokedasu", pos);
    if (caster)
        skill_spawn_on("tokedasu_shadow", caster);
}

static void ss_akumukesu_cast(unsigned cast_time, SkillActor *caster)
{
    if (!caster)
        return;
    skill_launch(caster, 0x3a6);
    game_actor_launch_effect_timed((struct GameActor *)caster, 0xc, cast_time >> 4);
}

static void ss_akumukesu_use(int level, SkillActor *caster, SkillActor *target)
{
    if (target)
        skill_spawn_on("akumukesu", target);
}

static void ss_shinkirou_cast(unsigned cast_time, SkillActor *caster)
{
    struct Effect *effect;

    if (!caster)
        return;
    game_actor_launch_effect_timed((struct GameActor *)caster, 0x945, cast_time >> 4);
    effect = effect_mgr_spawn(g_effectMgr, caster->aid, 0x6ee, skill_vec_zero(), 0, -1, 0, 0);
    if (effect)
        effect->ops->set_stop_frame(effect, (float)(double)cast_time);
}

static void ss_kagegari_use_on_ground(Vec3 pos, int level, SkillActor *caster)
{
    skill_spawn_at("kagegari", pos);
}

static void ss_kagenomai_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, SS_CAST_KUNAI, cast_time);
}

static void ss_kagenomai_use(int level, SkillActor *caster, SkillActor *target)
{
    if (target)
        skill_spawn_on("kagenomai", target);
}

static void ss_kagegissen_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, SS_CAST_KUNAI, cast_time);
}

static void ss_kagegissen_use(int level, SkillActor *caster, SkillActor *target)
{
    if (!caster || !target)
        return;
    spawn_turned_quadrant("kagegissen", caster, target->pos);
}

static void ss_fuumashouaku_cast(unsigned cast_time, SkillActor *caster)
{
    kunai_cast(caster, cast_time);
}

static void ss_fuumashouaku_hit_hit(SkillActor *caster, SkillActor *target, SkillActor *target_unit)
{
    if (target_unit)
        skill_spawn_at("fuumakouchiku_blasting", target_unit->pos);
}

static void ss_fuumakouchiku_cast(unsigned cast_time, SkillActor *caster)
{
    kunai_cast(caster, cast_time);
}

static void ss_fuumakouchiku_use_on_ground(Vec3 pos, int level, SkillActor *caster)
{
    if (caster)
        spawn_turned_quadrant("fuumakouchiku", caster, pos);
}

static void ss_kunaiwaikyoku_cast(unsigned cast_time, SkillActor *caster)
{
    kunai_cast(caster, cast_time);
}

static void ss_kunaiwaikyoku_use_on_ground(Vec3 pos, int level, SkillActor *caster)
{
    if (caster)
        spawn_turned("kunaiwaikyoku", caster, pos);
}

static void ss_kunaikaiten_use(int level, SkillActor *caster, SkillActor *target)
{
    static const char *const names[4] = {"kunaikaiten_bottom", "kunaikaiten_00", "kunaikaiten_01",
                                         "kunaikaiten_02"};
    struct Effect *effect;
    int round, i;

    if (!caster)
        return;
    for (round = 0; round < 4; round++) {
        for (i = 0; i < 4; i++) {
            effect = skill_spawn_at(names[i], caster->pos);
            if (effect)
                effect->ops->set_start_delay(effect, round * 0x1e0);
        }
    }
}

static void ss_kunaikussetsu_cast(unsigned cast_time, SkillActor *caster)
{
    kunai_cast(caster, cast_time);
}

static void ss_kunaikussetsu_use(int level, SkillActor *caster, SkillActor *target)
{
    if (!target)
        return;
    skill_spawn_on("kunaikussetsu", target);
    skill_spawn_on("kunaikussetsu_end", target);
    if (g_session1.map_rule_flags[0])
        return;
    skill_spawn_on("kunaikussetsu_bottom", target);
    skill_spawn_on("kunaikussetsu_bottom_end", target);
}

static void ss_sekienhou_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x37, cast_time);
}

static void ss_sekienhou_use_on_ground(Vec3 pos, int level, SkillActor *caster)
{
    if (caster)
        spawn_turned_quadrant("sekienhou", caster, pos);
}

static void ss_reiketsuhou_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x36, cast_time);
}

static void ss_reiketsuhou_use_on_ground(Vec3 pos, int level, SkillActor *caster)
{
    int counts[11] = {0, 5, 5, 5, 5, 5, 6, 6, 6, 7, 7};
    unsigned index = (unsigned)level;
    int count, span, i, delay, r;
    float half;
    Vec3 at;
    struct Effect *effect;

    skill_spawn_at("reiketsuhou_bottom", pos);
    if (index >= 10)
        index = 10;
    count = counts[index];
    if (count <= 0)
        return;
    half = (float)(count * 50) * 0.5f;
    span = count * 50 + 1;
    delay = 0;
    for (i = 0; i < count; i++) {
        at = pos;
        r = rand() % span;
        at.x = ((float)(r < 0 ? 0 : r) - half) * 0.1f + at.x;
        r = rand() % span;
        at.z = ((float)(r < 0 ? 0 : r) - half) * 0.1f + at.z;
        effect = skill_spawn_at("reiketsuhou", at);
        if (effect)
            effect->ops->set_start_delay(effect, delay);
        delay += 0x50;
    }
}

static void ss_raidenpou_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x39, cast_time);
}

static void ss_raidenpou_use_on_ground(Vec3 pos, int level, SkillActor *caster)
{
    if (caster)
        spawn_turned_quadrant("raidenpou", caster, pos);
}

static void ss_kinryuuhou_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x3a, cast_time);
}

static void ss_kinryuuhou_use(int level, SkillActor *caster, SkillActor *target)
{
    if (!caster || !target)
        return;
    spawn_turned("kinryuuhou", caster, target->pos);
}

static void ss_kinryuuhou_damage_hit(SkillActor *source, SkillActor *target)
{
    if (target)
        skill_spawn_on("kinryuuhou_hit", target);
}

static void ss_antenpou_cast(unsigned cast_time, SkillActor *caster)
{
    skill_cast_effect(caster, 0x3b, cast_time);
}

static void ss_antenpou_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_at("antenpou", caster->pos);
}

static void ss_kageakumu_cast(unsigned cast_time, SkillActor *caster)
{
    akumu_cast(caster, cast_time);
}

static void ss_hitouakumu_cast(unsigned cast_time, SkillActor *caster)
{
    akumu_cast(caster, cast_time);
}

static void ss_ankokuryuuakumu_cast(unsigned cast_time, SkillActor *caster)
{
    akumu_cast(caster, cast_time);
}

static void ss_kageakumu_hit(SkillActor *caster, SkillActor *target, SkillActor *target_unit)
{
    if (target)
        skill_spawn_on("kageakumu", target);
}

static void ss_hitouakumu_use(int level, SkillActor *caster, SkillActor *target)
{
    struct Vec order;
    struct Effect *effect;
    int ring, base, n, k, r, whole;
    float step;

    if (!caster)
        return;
    skill_spawn_at("hitouakumu", caster->pos);
    for (ring = 0, base = 0; base < 30; ring++, base += 10) {
        r = rand() % 5;
        n = (r < 0 ? 0 : r) + 15;
        step = 360.0f / (float)n;
        skill_shuffled_indices(&order, n);
        for (k = 0; k < n; k++) {
            effect =
                effect_mgr_spawn(g_effectMgr, 0, skill_format_effect_id("hitouakumu_%02d", ring),
                                  caster->pos, 0, -1, 0, 0);
            if (!effect)
                continue;
            whole = (int)step;
            if (whole) {
                r = rand() % whole;
                r = r < 0 ? 0 : r;
            } else {
                r = 0;
            }
            effect->ops->set_spin(effect, (float)k * step + (float)r);
            effect->ops->set_start_delay(effect, (((int *)order.first)[k] + base) << 5);
        }
        vec_free(&order);
    }
}

static void ss_ankokuryuuakumu_use(int level, SkillActor *caster, SkillActor *target)
{
    if (caster)
        skill_spawn_on("ankokuryuuakumu_00", caster);
}

static void ss_ankokuryuuakumu_hit(SkillActor *caster, SkillActor *target, SkillActor *target_unit)
{
    if (target)
        skill_spawn_on("ankokuryuuakumu_01", target);
}

const struct StatusAction efst_action_nightmare_ops = {
    .on = efst_nightmare_on,
    .off = efst_nightmare_off,
    .reapply = efst_nightmare_on,
};

const struct StatusAction efst_action_shadow_clock_ops = {
    .on = efst_shadow_clock_on,
    .off = efst_shadow_clock_off,
    .reapply = efst_shadow_clock_reapply,
};

const struct SkillAction skill_action_ss_akumukesu_ops = {
    .cast_action = 55,
    .use_action = 56,
    .cast = ss_akumukesu_cast,
    .use = ss_akumukesu_use,
};

const struct SkillAction skill_action_ss_ankokuryuuakumu_ops = {
    .cast_action = 55,
    .use_action = 57,
    .cast = ss_ankokuryuuakumu_cast,
    .use = ss_ankokuryuuakumu_use,
    .hit = ss_ankokuryuuakumu_hit,
};

const struct SkillAction skill_action_ss_antenpou_ops = {
    .cast_action = 55,
    .use_action = 57,
    .cast = ss_antenpou_cast,
    .use = ss_antenpou_use,
};

const struct SkillAction skill_action_ss_fuumakouchiku_ops = {
    .cast_action = 8,
    .use_action = 9,
    .cast = ss_fuumakouchiku_cast,
    .use_on_ground = ss_fuumakouchiku_use_on_ground,
};

const struct SkillAction skill_action_ss_fuumashouaku_ops = {
    .cast_action = 8,
    .use_action = 9,
    .cast = ss_fuumashouaku_cast,
};

const struct SkillAction skill_action_ss_fuumashouaku_hit_ops = {
    .cast_action = 8,
    .use_action = 7,
    .hit = ss_fuumashouaku_hit_hit,
};

const struct SkillAction skill_action_ss_hitouakumu_ops = {
    .cast_action = 55,
    .use_action = 57,
    .cast = ss_hitouakumu_cast,
    .use = ss_hitouakumu_use,
};

const struct SkillAction skill_action_ss_kageakumu_ops = {
    .cast_action = 55,
    .use_action = 57,
    .cast = ss_kageakumu_cast,
    .hit = ss_kageakumu_hit,
};

const struct SkillAction skill_action_ss_kagegari_ops = {
    .cast_action = 8,
    .use_action = 7,
    .use_on_ground = ss_kagegari_use_on_ground,
};

const struct SkillAction skill_action_ss_kagegissen_ops = {
    .cast_action = 8,
    .use_action = 2,
    .cast = ss_kagegissen_cast,
    .use = ss_kagegissen_use,
};

const struct SkillAction skill_action_ss_kagenomai_ops = {
    .cast_action = 8,
    .use_action = 2,
    .cast = ss_kagenomai_cast,
    .use = ss_kagenomai_use,
};

const struct SkillAction skill_action_ss_kinryuuhou_ops = {
    .cast_action = 55,
    .use_action = 57,
    .cast = ss_kinryuuhou_cast,
    .use = ss_kinryuuhou_use,
    .damage_hit = ss_kinryuuhou_damage_hit,
};

const struct SkillAction skill_action_ss_kunaikaiten_ops = {
    .cast_action = 8,
    .use_action = 9,
    .use = ss_kunaikaiten_use,
};

const struct SkillAction skill_action_ss_kunaikussetsu_ops = {
    .cast_action = 8,
    .use_action = 56,
    .cast = ss_kunaikussetsu_cast,
    .use = ss_kunaikussetsu_use,
};

const struct SkillAction skill_action_ss_kunaiwaikyoku_ops = {
    .cast_action = 8,
    .use_action = 9,
    .cast = ss_kunaiwaikyoku_cast,
    .use_on_ground = ss_kunaiwaikyoku_use_on_ground,
};

const struct SkillAction skill_action_ss_raidenpou_ops = {
    .cast_action = 55,
    .use_action = 57,
    .cast = ss_raidenpou_cast,
    .use_on_ground = ss_raidenpou_use_on_ground,
};

const struct SkillAction skill_action_ss_reiketsuhou_ops = {
    .cast_action = 55,
    .use_action = 57,
    .cast = ss_reiketsuhou_cast,
    .use_on_ground = ss_reiketsuhou_use_on_ground,
};

const struct SkillAction skill_action_ss_sekienhou_ops = {
    .cast_action = 55,
    .use_action = 57,
    .cast = ss_sekienhou_cast,
    .use_on_ground = ss_sekienhou_use_on_ground,
};

const struct SkillAction skill_action_ss_shinkirou_ops = {
    .cast_action = 55,
    .use_action = 56,
    .cast = ss_shinkirou_cast,
};

const struct SkillAction skill_action_ss_tokedasu_ops = {
    .cast_action = 8,
    .use_action = 7,
    .use_on_ground = ss_tokedasu_use_on_ground,
};

const struct SkillObjectAction skill_object_action_npc_fuuma_ops = {
    .appear = npc_fuuma_appear,
};

const struct SkillObjectAction skill_object_action_ss_kunaitrap_ops = {
    .appear = ss_kunaitrap_appear,
    .leave = ss_kunaitrap_leave,
};

const struct SkillObjectAction skill_object_action_ss_sbunshin_ops = {
    .appear = ss_sbunshin_appear,
    .leave = ss_sbunshin_leave,
};
