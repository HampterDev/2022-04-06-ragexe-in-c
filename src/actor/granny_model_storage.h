#ifndef ACTOR_GRANNY_MODEL_STORAGE_H
#define ACTOR_GRANNY_MODEL_STORAGE_H

#include "resources/granny_res.h"
#include "resources/granny_playback.h"
#include "actor/granny_projection.h"

struct GrannyMeshState {
    struct GrannyProjection *projection;
    struct GrannyVertexBuffer *vertices;
    granny_mesh_binding *binding;
    struct Texture *texture;
    unsigned short *indices;
    int triangles;
    float matrix[12];
};

struct GrannyModelState {
    struct C3dGrannyModelRes *resource;
    struct C3dGrannyModelRes *base_resource;
    struct GrannyPlayback *playback;
    int mesh_count;
    struct GrannyBounds bounds;
    struct GrannyMeshState meshes[];
};

struct GrannyModelState *granny_model_state_create(struct C3dGrannyModelRes *resource,
                                                   struct C3dGrannyModelRes *base);
void granny_model_state_destroy(struct GrannyModelState *model);
const struct GrannyMeshVertex *granny_model_prepare_mesh(struct GrannyModelState *model, int mesh,
                                                        granny_world_pose *pose,
                                                        struct GrannyHairPivot *hair);
const struct GrannyMeshVertex *granny_model_cached_vertices(struct GrannyModelState *model, int mesh);
int granny_model_find_hair_pivot(const struct GrannyModelState *model);

#endif
