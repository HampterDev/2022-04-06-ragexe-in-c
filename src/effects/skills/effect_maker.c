#include "actor/object_ops.h"
#include "actor/render_ops.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include <stdlib.h>
#include <string.h>
#include "resources/world_records.h"
#include "effects/render_object.h"
#include "effects/skills/skill_view.h"
#include "actor/game_actor.h"
#include "effects/rag/rag_effect.h"

#define MSG_SETUP 0xe
#define RAG_MSG_EMITTER_PARAMS 0x2e
#define EFFECT_SPREAD_SPACING 0x2d
#define IN_RANGE 130.0f

struct EffectMaker {
    struct AbleToMakeEffect base;
    unsigned frames_since_launch;
    unsigned interval;
    int effect_id;
    float params[4];
};

struct GameObject;
struct RenderObject;

const struct RenderObjectOps effect_maker_ops = {
    .base = {
        .destroy = effect_maker_delete,
        .on_process = effect_maker_on_process,
        .send_msg = effect_maker_send_msg,
        .render = effect_maker_render,
        .get_direction = game_object_dir_of,
    },
    .render_body = game_object_set_render_info,
    .set_action = render_object_set_action_base,
    .update_motion = render_object_update_motion_base,
    .get_effect_state = render_object_return_zero,
};

static void effect_maker_reset(struct EffectMaker *self)
{
    self->effect_id = -1;
    self->interval = 360;
    self->frames_since_launch = 360;
    self->params[0] = 0.0f;
    self->params[1] = 0.0f;
    self->params[2] = 0.0f;
    self->params[3] = 0.0f;
}

static struct EffectMaker *effect_maker_construct(struct EffectMaker *self)
{
    able_to_make_effect_construct(&self->base);
    self->base.ops = &effect_maker_ops.base;
    effect_maker_reset(self);
    return self;
}

struct EffectMaker *effect_maker_new(void)
{
    struct EffectMaker *self = mem_alloc(sizeof *self);

    return self ? effect_maker_construct(self) : NULL;
}

void effect_maker_destruct(struct EffectMaker *self)
{
    self->base.ops = &effect_maker_ops.base;
    able_to_make_effect_dc_able_to_make_effect(&self->base);
}

void effect_maker_delete(struct GameObject *object)
{
    struct EffectMaker *self = (struct EffectMaker *)object;

    effect_maker_destruct(self);
    mem_free(self);
}

unsigned char effect_maker_on_process(struct GameObject *object)
{
    struct EffectMaker *self = (struct EffectMaker *)object;

    struct RenderObject *me = (struct RenderObject *)&self->base;
    const struct RenderObject *player = (const struct RenderObject *)game_current_player();

    if (player && IN_RANGE > render_object_calc_ground_dist(me, player->pos.x, player->pos.z) && self->frames_since_launch >= self->interval) {
        Vec3 here;
        struct RagEffect *effect;

        here.x = 0.0f;
        here.y = 0.0f;
        here.z = 0.0f;
        self->frames_since_launch = 0;
        effect = game_actor_launch_effect((struct GameActor *)me, self->effect_id, here, 0);
        if (effect)
            rag_effect_send_msg(effect, 0, RAG_MSG_EMITTER_PARAMS, (intptr_t)&self->params, 0, 0);
    }
    self->frames_since_launch++;
    return 1;
}

static int random_below(int n)
{
    int r;
    if (!n)
        return 0;
    r = rand() % n;
    return r < 0 ? 0 : r;
}

void effect_maker_send_msg(struct GameObject *object, struct GameObject *source,
    RenderMsgArg msg, RenderMsgArg par1, RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    struct EffectMaker *self = (struct EffectMaker *)object;

    const struct RswEffect *rsw = (const struct RswEffect *)MSG_PTR(par1);
    float interval;

    if (msg != MSG_SETUP) {
        able_to_make_effect_send_msg((struct GameObject *)&self->base, source, msg, par1, par2, par3, par4);
        return;
    }
    self->effect_id = rsw->id;
    interval = rsw->emitSpeed;
    if (self->effect_id != EFFECT_SPREAD_SPACING) {
        int r;
        self->interval = (unsigned)interval;
        r = rand() % 24;
        if (r < 0)
            r = 0;
        self->frames_since_launch = (unsigned)(rsw->emitSpeed - (float)r);
    } else {
        int spread = (int)(interval / 10.0f);
        int r = random_below(spread);
        self->interval = (unsigned)((float)r + interval - interval / 20.0f);
        self->frames_since_launch = (unsigned)random_below((int)(rsw->emitSpeed + 1.0f));
    }
    memcpy(&self->base.pos, rsw->pos, sizeof rsw->pos);
    memcpy(self->params, rsw->param, sizeof self->params);
}

void effect_maker_render(struct GameObject *object, const float *view)
{
}
