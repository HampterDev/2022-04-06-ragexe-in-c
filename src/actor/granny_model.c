#include "actor/actor_ops.h"
#include "actor/render_ops.h"
#include <string.h>
#include "actor/granny_pc.h"

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

void granny_pc_free_res(struct GrannyPc *self)
{
    int i;

    for (i = 0; i < 10; i++) {
        granny_model_state_destroy(self->parts[i]);
        self->parts[i] = NULL;
    }
    granny_model_state_destroy(self->model);
    self->model = NULL;
    self->body_pose = NULL;
    self->draw_phase = 0;
}

void granny_pc_play_state_anim(struct GrannyPc *self, int loop)
{
    self->anim = self->state;
    if (!self->model)
        return;
    granny_playback_stop(self->model->playback);
    self->anim_time = 0.0f;
    granny_playback_play(self->model->playback, self->anim, loop == 1 && self->state != 6);
}

granny_world_pose *granny_pc_pose(struct GrannyPc *self, struct GrannyModelState *model)
{
    float elapsed;

    if (!model)
        model = self->model;
    if (!model)
        return NULL;
    *(GrannyClock *)self->clock_now = _GrannyGetSystemSeconds();
    elapsed = _GrannyGetSecondsElapsed((const GrannyClock *)self->clock_last, (const GrannyClock *)self->clock_now);
    self->anim_time = self->anim_time + (elapsed + elapsed);
    memcpy(self->clock_last, self->clock_now, sizeof self->clock_last);
    if (model != self->model) {
        granny_playback_stop(model->playback);
        granny_playback_play(model->playback, self->anim, 0);
        return granny_playback_pose(model->playback, self->anim_time);
    }
    if (self->anim == 2 && granny_playback_finished(self->model->playback, 2)) {
        self->anim = 0;
        granny_playback_play(self->model->playback, self->anim, 0);
    }
    return granny_playback_pose(self->model->playback, self->anim_time * 0.5f);
}

int granny_pc_replace_models(struct GrannyPc *self)
{
    static const char *const part_prefixes[] = {"bd", "pt", "le"};
    struct GrannyModelState *body = NULL;
    struct GrannyModelState *parts[10] = {0};
    struct C3dGrannyModelRes *resource;
    struct GrannyHairPivot pivot = {0};
    char name[sizeof self->model_name], hair[sizeof self->model_name];
    char *at, *separator;
    size_t prefix;
    int i;

    strcpy(name, "SWM@hd기본_3.gr2");
    at = strstr(name, "@hd");
    separator = at ? strchr(at + 1, '_') : NULL;
    if (!separator)
        return 0;
    prefix = (size_t)(separator - (at + 1)) + 1;
    if (prefix > sizeof hair - sizeof "99.gr2")
        return 0;
    resource = res_mgr_get(g_resMgr(), name);
    body = granny_model_state_create(resource, NULL);
    if (!body)
        return 0;
    at[1] = 'h'; at[2] = 'r';
    memcpy(hair, at + 1, prefix);
    memcpy(hair + prefix, "99.gr2", sizeof "99.gr2");
    resource = res_mgr_get(g_resMgr(), hair);
    parts[1] = granny_model_state_create(resource, NULL);
    if (!parts[1])
        goto fail;
    for (i = 2; i <= 4; i++) {
        at[1] = part_prefixes[i - 2][0];
        at[2] = part_prefixes[i - 2][1];
        resource = res_mgr_get(g_resMgr(), name);
        parts[i] = granny_model_state_create(resource, body->resource);
        if (!parts[i])
            goto fail;
    }
    pivot.part = 1;
    pivot.mesh = granny_model_find_hair_pivot(body);
    pivot.matrix[0] = pivot.matrix[4] = pivot.matrix[8] = 1.0f;
    granny_pc_free_res(self);
    self->model = body;
    memcpy(self->parts, parts, sizeof parts);
    self->hair_pivot = pivot;
    strcpy(self->model_name, name);
    *(GrannyClock *)self->clock_last = _GrannyGetSystemSeconds();
    self->anim_time = 0.0f;
    self->model_kind = 0;
    return 1;

fail:
    for (i = 0; i < 10; i++)
        granny_model_state_destroy(parts[i]);
    granny_model_state_destroy(body);
    return 0;
}
