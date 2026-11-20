#include <stdlib.h>
#include "actors/granny_model_storage.h"
#include "fileio/resource_api.h"

struct GrannyModelState *granny_model_state_create(struct C3dGrannyModelRes *resource)
{
    struct GrannyModelState *model;

    if (!resource || !resource->model || !resource->skeleton)
        return NULL;
    model = calloc(1, sizeof *model);
    if (!model)
        return NULL;
    model->resource = resource;
    model->bounds = resource->bounds;
    model->playback = granny_playback_create(resource);
    if (!model->playback) {
        free(model);
        return NULL;
    }
    res_lock(&resource->res);
    return model;
}

void granny_model_state_destroy(struct GrannyModelState *model)
{
    if (!model)
        return;
    granny_playback_destroy(model->playback);
    res_unlock(&model->resource->res);
    free(model);
}
