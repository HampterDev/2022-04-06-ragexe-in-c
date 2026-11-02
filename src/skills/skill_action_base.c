#include "skills/skill_action_internal.h"
#include <stdlib.h>
#include "core/text_util.h"
#include "effects/skills/skill_view.h"

int skill_named_effect_id(const char *name)
{
    struct String text;
    int id;

    string_init(&text);
    string_assign_cstr(&text, name);
    id = ez2_str_effect_mgr_get_effect_id(ez2str_effect_mgr_instance(), &text);
    string_free(&text);
    return id;
}

int skill_format_effect_id(const char *fmt, int n)
{
    struct String text;
    int id;

    string_init(&text);
    string_format(&text, fmt, n);
    id = ez2_str_effect_mgr_get_effect_id(ez2str_effect_mgr_instance(), &text);
    string_free(&text);
    return id;
}

struct Effect *skill_spawn_named(const char *name, int aid, Vec3 pos, int target_aid, int level,
                                  unsigned char reuse, int instance_key)
{
    struct EffectMgr *mgr = g_effectMgr;

    return effect_mgr_spawn(mgr, aid, skill_named_effect_id(name), pos, target_aid, level, reuse,
                             instance_key);
}

struct Vec *skill_shuffled_indices(struct Vec *out, int count)
{
    int i, *first, *last, *target;
    unsigned bmask, bits, index, n_targets, ret, mask, off;

    out->first = 0;
    out->last = 0;
    out->end = 0;
    for (i = 0; i < count; i++) {
        vec_push(out, &i, sizeof(int));
    }
    first = (int *)out->first;
    last = (int *)out->last;
    bits = 32;
    bmask = 0xffffffffu;
    do {
        bmask >>= 1;
        --bits;
    } while (bmask > 0x7fff);
    if (first == last)
        return out;
    target = first + 1;
    n_targets = target > last ? 0 : (unsigned)(((char *)last - (char *)target + 3) / 4);
    if (!n_targets)
        return out;
    for (index = 2; index - 2 != n_targets; index++, target++) {
        for (;;) {
            ret = 0;
            mask = 0;
            while (mask < index - 1) {
                ret <<= bits - 1;
                ret <<= 1;
                for (;;) {
                    unsigned v = (unsigned)rand() & 0x7fffu;

                    if (v <= bmask) {
                        ret |= v;
                        break;
                    }
                }
                mask <<= bits - 1;
                mask <<= 1;
                mask |= bmask;
            }
            if (ret / index < mask / index || mask % index == index - 1)
                break;
        }
        off = ret % index;
        if (off != index - 1) {
            int t = *target;

            *target = first[off];
            first[off] = t;
        }
    }
    return out;
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
