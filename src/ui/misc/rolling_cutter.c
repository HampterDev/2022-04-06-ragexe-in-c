#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "std/tree.h"
#include "core/object_type.h"
#include "effects/rag/rag_effect.h"
#include "render/render_api.h"
#include "ui/ui_std.h"
#include "ui/misc/rolling_cutter.h"
#include "actor/game_actor.h"
#include "game/mode_callbacks.h"
#include <stdint.h>

struct GameObject;
struct RenderObject;
static const struct RenderObjectOps rolling_cutter_rag_effect_ops = {
    .base = {
        .destroy = rolling_cutter_rag_effect_delete,
        .on_process = rolling_cutter_rag_effect_on_process,
        .send_msg = render_object_send_msg,
        .render = rag_effect_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

#define SPIN_STEP 12.0f
#define COUNTER_ROLLING_CUTTER 1
#define COUNTER_FACING 0

static struct Tree *actor_counters(void *actor)
{
    return &((struct GameActor *)actor)->vars;
}

int game_actor_get_counter(struct GameActor *self, int key)
{
    struct IntInt64Pair *var = tree_find(actor_counters(self), &key, tree_less_int);

    if (!var)
        return 0;
    return (int)var->value;
}

static int64_t *actor_counter_at(void *actor, int key)
{
    struct IntInt64Pair *var = tree_get(actor_counters(actor), &key, sizeof key, sizeof *var, tree_less_int);

    return &var->value;
}

void game_actor_set_counter(struct GameActor *self, int key, int64_t value)
{
    *actor_counter_at(self, key) = value;
}

struct RollingCutterRagEffect *rolling_cutter_rag_effect_construct(struct RollingCutterRagEffect *self)
{
    rag_effect_construct(&self->base);
    self->base.base.ops = &rolling_cutter_rag_effect_ops.base;
    self->spin_frames = 60;
    self->frames = 0;
    return self;
}

void rolling_cutter_rag_effect_delete(struct GameObject *object)
{
    struct RollingCutterRagEffect *self = (struct RollingCutterRagEffect *)object;

    self->base.base.ops = &rolling_cutter_rag_effect_ops.base;
    rag_effect_destruct(&self->base);
    mem_free(self);
}

void rag_effect_set_duration(struct RagEffect *self, int frame)
{
    self->duration = frame;
}

int render_object_return_zero(struct RenderObject *self)
{
    return 0;
}

unsigned char rolling_cutter_rag_effect_on_process(struct GameObject *object)
{
    struct RollingCutterRagEffect *self = (struct RollingCutterRagEffect *)object;

    struct RenderObject *master = self->base.master;
    struct GameActor *actor;
    int64_t left;

    if (!master)
        return 0;
    if (g_isSkipFrame > 0 && master->drawn == 1) {
        master->overlay_flags = 0;
        self->base.master->drawn = 0;
        master = self->base.master;
    }
    self->base.base.pos = master->pos;
    if (self->frames == 0) {
        rag_play_wave(&self->base, "effect\\EF_BeginSpell.wav", -100.0f);
    }
    self->base.master->roty += SPIN_STEP;
    if (self->base.master->roty >= 360.0f)
        self->base.master->roty -= 360.0f;
    self->frames++;
    if (self->frames < self->spin_frames)
        return 1;
    actor = object_as_type(self->base.master, OBJECT_GAME_ACTOR);
    if (actor) {
        left = (int64_t)(game_actor_get_counter(actor, COUNTER_ROLLING_CUTTER) - 1);
        *actor_counter_at(actor, COUNTER_ROLLING_CUTTER) = left;
        if (game_actor_get_counter(actor, COUNTER_ROLLING_CUTTER) <= 0)
            actor->roty = (float)game_actor_get_counter(actor, COUNTER_FACING) / 1000.0f;
    }
    return 0;
}
