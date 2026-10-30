#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#ifndef ACTOR_GRANNY_ACTOR_H
#define ACTOR_GRANNY_ACTOR_H

#include "actor/types.h"
#include "resources/granny_res.h"
#include "resources/granny_playback.h"
#include "actor/granny_projection.h"
#include "actor/granny_model_storage.h"

struct GrannyLight {
    Vec3 dir;
    int intensity;
    int r, g, b;
};

struct C3dGrannyGameActor *c3d_granny_game_actor_construct(struct C3dGrannyGameActor *self, const char *name);
struct GrannyCorpse3d *granny_corpse3d_construct(struct GrannyCorpse3d *self, const char *name);
void granny_corpse3d_load_model(struct GrannyCorpse3d *self);
void granny_corpse3d_free_model(struct GrannyCorpse3d *self);

#endif
