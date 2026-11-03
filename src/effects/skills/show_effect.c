#include "effects/effect_allocator.h"
#include "session/state.h"
#include "world/actor_lookup.h"
#include "effects/effect_mgr.h"
#include "effects/skills/skill_view.h"
#include "actor/actor.h"
#include "std/tree.h"

#define EFFECT_SHOW_EFFECT 0x947
#define ACTOR_MSG_DROP_SHOWN_EFFECT 0x9f

struct ShownEffect {
    int show_id;
    int effect_type;
};

struct ShowEffect {
    struct Effect base;
    struct Tree shown;
};

void show_effect_release(struct Effect *self);
static void show_effect_destroy(struct Effect *effect);

static const struct EffectOps show_effect_ops = {
    .set_submit = effect_set_submit,
    .init = effect_init,
    .update_range = effect_update_range,
    .process = effect_process,
    .render = effect_render,
    .release = show_effect_release,
    .is_alive = effect_is_owner_alive,
    .restart = effect_restart,
    .set_color = effect_set_color,
    .set_texture = effect_set_texture,
    .after_init = effect_after_init,
    .skip = effect_skip,
    .set_stop_frame = effect_set_stop_frame,
    .set_start_delay = effect_set_start_delay,
    .set_spin = effect_set_spin,
    .set_str_name = effect_set_str_name,
    .set_list_flags = effect_set_list_flags,
    .set_zoom = effect_set_zoom,
    .set_alpha_scale = effect_set_alpha_scale,
    .destroy = show_effect_destroy,
    .fade_out = effect_fade_out,
};

#include "actor/render_ops.h"

static void actor_send(void *actor, void *sender, int show_id, int effect_type)
{
    render_object_send(actor, sender, ACTOR_MSG_DROP_SHOWN_EFFECT, show_id, effect_type, 0, 0);
}

struct ShowEffect *show_effect_construct(struct ShowEffect *self)
{
    effect_construct(&self->base, EFFECT_SHOW_EFFECT);
    self->base.ops = &show_effect_ops;
    tree_init(&self->shown);
    return self;
}

struct Effect *show_effect_new(void)
{
    struct ShowEffect *self = mem_try_alloc(sizeof *self);

    return self ? &show_effect_construct(self)->base : NULL;
}

/* Effect types created as ShowEffect that effects/effect_registry.c registers after the
   EZ2STR file types (see g_effectTypes there). A type missing here is not created. */
static const struct EffectTypeRow show_effect_type_rows[] = {
    EFFECT_TYPE(0x947, show_effect_new),
};
const struct EffectTypeTable show_effect_types = EFFECT_TYPE_TABLE(show_effect_type_rows);

static void show_effect_destroy(struct Effect *effect)
{
    struct ShowEffect *self = (struct ShowEffect *)effect;

    tree_free(&self->shown, NULL);
    effect_destruct(&self->base);
    mem_free(self);
}

void show_effect_add(struct ShowEffect *self, int show_id, int effect_type)
{
    int owner = self->base.owner_aid;
    Vec3 origin = { 0, 0, 0 };
    struct ShownEffect *shown;

    if (tree_find(&self->shown, &show_id, tree_less_int))
        return;
    if (effect_mgr_find_effect(g_effectMgr, owner, effect_type))
        return;
    if (!effect_mgr_spawn(g_effectMgr, owner, effect_type, origin, 0, -1, 0, show_id)) {
        void *actor = g_session1.ownAid == owner
            ? (void *)game_current_player() : (void *)game_find_actor(owner);
        if (!actor)
            return;
        if (!able_to_make_effect_launch_show_effect(actor, effect_type, show_id))
            return;
    }
    shown = tree_get(&self->shown, &show_id, sizeof show_id, sizeof *shown, tree_less_int);
    shown->effect_type = effect_type;
}

void show_effect_remove(struct ShowEffect *self, int show_id)
{
    struct ShownEffect *shown = tree_find(&self->shown, &show_id, tree_less_int);
    void *actor;
    int owner;

    if (!shown)
        return;
    owner = self->base.owner_aid;
    actor = game_find_actor(owner);
    if (!actor)
        return;
    actor_send(actor, actor, show_id, shown->effect_type);
    tree_erase_key(&self->shown, &show_id, tree_less_int, NULL);
}

int show_effect_count(struct ShowEffect *self)
{
    return (int)self->shown.size;
}

void show_effect_release(struct Effect *effect)
{
    struct ShowEffect *self = (struct ShowEffect *)effect;
    struct TreeNode *node;
    void *actor = 0;
    int owner;

    if (self->shown.size == 0)
        return;
    owner = self->base.owner_aid;
    for (node = tree_first(&self->shown); node != self->shown.head; node = tree_next(node)) {
        struct ShownEffect *shown = TREE_VALUE(node, struct ShownEffect);

        actor = game_find_actor(owner);
        if (actor)
            actor_send(actor, 0, shown->show_id, shown->effect_type);
        else if (effect_mgr_is_registered(g_effectMgr, shown->effect_type))
            effect_mgr_remove_keyed(g_effectMgr, owner, shown->show_id, shown->effect_type);
    }
    tree_clear(&self->shown, NULL);
}
