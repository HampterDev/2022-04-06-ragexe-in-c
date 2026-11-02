#include "skills/skill_action.h"
#include "effects/effect_mgr.h"
#include "effects/skills/skill_view.h"

static Vec3 skill_vec_zero(void)
{
    Vec3 v;

    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 0.0f;
    return v;
}

static struct Effect *status_effect(int aid, int type)
{
    struct Effect *effect = effect_mgr_find_effect(g_effectMgr, aid, type);

    if (effect)
        return effect;
    return effect_mgr_spawn(g_effectMgr, aid, type, skill_vec_zero(), 0, -1, 0, 0);
}

#define EFFECT_CHANGE_SIZE 0x946

static void efst_change_size_on(int efst, int value, SkillActor *actor)
{
    struct Effect *effect;

    if (!actor)
        return;
    effect = status_effect(actor->aid, EFFECT_CHANGE_SIZE);
    if (effect)
        effect->params.scale = (float)value * 0.01f;
}

static void efst_change_size_off(int efst, int value, SkillActor *actor)
{
    if (actor)
        effect_mgr_remove_type(g_effectMgr, actor->aid, EFFECT_CHANGE_SIZE);
}

const struct StatusAction efst_action_change_size_monster_ops = {
    .on = efst_change_size_on,
    .off = efst_change_size_off,
    .reapply = efst_change_size_on,
};

const struct StatusAction efst_action_change_size_npc_ops = {
    .on = efst_change_size_on,
    .off = efst_change_size_off,
    .reapply = efst_change_size_on,
};

#define EFFECT_SHOW_EFFECT 0x947

static void efst_show_effect_on(int efst, int value, SkillActor *actor)
{
    struct Effect *effect;

    if (!actor)
        return;
    effect = status_effect(actor->aid, EFFECT_SHOW_EFFECT);
    if (effect)
        show_effect_add((struct ShowEffect *)effect, efst, value);
}

static void efst_show_effect_off(int efst, int value, SkillActor *actor)
{
    int aid;
    struct Effect *effect;

    if (!actor)
        return;
    aid = actor->aid;
    effect = effect_mgr_find_effect(g_effectMgr, aid, EFFECT_SHOW_EFFECT);
    if (!effect)
        return;
    show_effect_remove((struct ShowEffect *)effect, efst);
    if (show_effect_count((struct ShowEffect *)effect) <= 0)
        effect_mgr_remove_type(g_effectMgr, aid, EFFECT_SHOW_EFFECT);
}

const struct StatusAction efst_action_show_effect1_ops = {
    .on = efst_show_effect_on,
    .off = efst_show_effect_off,
    .reapply = efst_show_effect_on,
};

const struct StatusAction efst_action_show_effect2_ops = {
    .on = efst_show_effect_on,
    .off = efst_show_effect_off,
    .reapply = efst_show_effect_on,
};

const struct StatusAction efst_action_show_effect3_ops = {
    .on = efst_show_effect_on,
    .off = efst_show_effect_off,
    .reapply = efst_show_effect_on,
};
