#ifndef ACTOR_GRANNY_MODEL_STORAGE_H
#define ACTOR_GRANNY_MODEL_STORAGE_H

#include "resources/granny_res.h"
#include "resources/granny_playback.h"

/* An actor's model: the resource it shows, held while it does, its animations' playback, and its
   bounds (custom: the meshes' draws are the resource's, render/gfx.h, gfx_draw_skin). */
struct GrannyModelState {
    struct C3dGrannyModelRes *resource;
    struct GrannyPlayback *playback;
    struct GrannyBounds bounds;
};

struct GrannyModelState *granny_model_state_create(struct C3dGrannyModelRes *resource);
void granny_model_state_destroy(struct GrannyModelState *model);

#endif
