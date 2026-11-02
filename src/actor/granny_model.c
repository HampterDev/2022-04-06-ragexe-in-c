#include "actor/actor_ops.h"
#include "actor/render_ops.h"
#include <string.h>
#include "actor/granny_actor.h"
#include "actor/game_actor.h"

void granny_corpse3d_load_model(struct GrannyCorpse3d *self)
{
    struct C3dGrannyModelRes *resource = res_mgr_get(g_resMgr(), self->name);
    struct GrannyModelState *model = granny_model_state_create(resource, NULL);

    if (!model)
        return;
    granny_corpse3d_free_model(self);
    self->model = model;
    *(GrannyClock *)self->clock_last = _GrannyGetSystemSeconds();
    self->anim_time = 0.0f;
    self->frames = 0;
    granny_playback_play(model->playback, 3, 1);
}

void granny_corpse3d_free_model(struct GrannyCorpse3d *self)
{
    granny_model_state_destroy(self->model);
    self->model = NULL;
}

void c3d_granny_game_actor_set_job_model(struct GameActor * base_actor, int job, int sex)
{
    struct C3dGrannyGameActor * self = (struct C3dGrannyGameActor *)base_actor;

    struct C3dGrannyModelRes *resource = res_mgr_get(g_resMgr(), self->name);
    struct GrannyModelState *model = granny_model_state_create(resource, NULL);

    if (!model)
        return;
    game_actor_set_job((struct GameActor *)self, job);
    game_actor_set_sex((struct GameActor *)self, sex);
    granny_model_state_destroy(self->model);
    self->model = model;
    *(GrannyClock *)self->clock_last = _GrannyGetSystemSeconds();
    self->anim_time = 0.0f;
    if (job == 0x49d || job == 0x504) {
        self->model_kind = 1;
        model->bounds.half_width = 30.0f;
        model->bounds.half_height = 30.0f;
    } else {
        self->model_kind = job == 0x2d2 ? 2 : 0;
    }
}
