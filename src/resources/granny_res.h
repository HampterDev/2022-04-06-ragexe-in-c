#include "render/screen_bounds.h"
#ifndef RESOURCES_GRANNY_RES_H
#define RESOURCES_GRANNY_RES_H

#include "fileio/resource_types.h"
#include "resources/granny_sdk.h"
#include "render/vec3.h"

struct Texture;

struct GrannyMeshVertex {
    Vec3 pos;
    float nx, ny, nz;
    float u, v;
};
SIZE_ASSERT(GrannyMeshVertex, 32);

struct GrannyVertexBuffer {
    size_t count;
    struct GrannyMeshVertex vertices[];
};

struct GrannyResTexture {
    const char *name;
    struct Texture *texture;
};

struct GrannyResMesh {
    granny_mesh *mesh;
    granny_mesh_deformer *deformer;
    unsigned short *indices;
    struct GrannyMeshVertex *rigidVerts;
    int materialCount;
    struct GrannyResTexture **textures;
};

struct GrannyResModel {
    int meshCount;
    struct GrannyResMesh *meshes;
};

struct GrannyBounds {
    float center[3];
    float half_width;
    float half_height;
};

struct GrannyHairPivot {
    int part;
    int mesh;
    float matrix[12];
};
SIZE_ASSERT(GrannyHairPivot, 0x38);

struct GrannyMeshPose {
    granny_world_pose *world;
    granny_mesh_binding *binding;
    struct GrannyHairPivot *hair;
};

struct C3dGrannyModelRes {
    struct Res res;
    granny_file *file;
    int textureCount;
    struct GrannyResTexture *textures;
    struct GrannyResModel *model;
    char name[260];
    granny_model *grannyModel;
    struct GrannyBounds bounds;
    struct C3dGrannyBoneRes *boneRes;
    int id;
    granny_skeleton *skeleton;
};

enum { GRANNY_BONE_JOB_COUNT = 10, GRANNY_BONE_ANIMATION_COUNT = 5 };
struct GrannyAnimationEntry {
    granny_file *file;
    granny_animation *animation;
};
struct C3dGrannyBoneRes {
    struct GrannyAnimationEntry entries[GRANNY_BONE_JOB_COUNT][GRANNY_BONE_ANIMATION_COUNT];
};
extern struct C3dGrannyBoneRes g_grannyBoneRes;

BYTE c3d_granny_model_res_load(struct C3dGrannyModelRes *self, const char *name);
void granny_model_resource_clear(struct C3dGrannyModelRes *self);
void granny_model_resource_finish(struct C3dGrannyModelRes *self);
struct GrannyVertexBuffer *granny_vertex_buffer_create(const struct C3dGrannyModelRes *model, int mesh);
void granny_vertex_buffer_destroy(struct GrannyVertexBuffer *buffer);
int granny_mesh_vertex_position(const struct C3dGrannyModelRes *model, int mesh,
                               const struct GrannyMeshVertex *vertices, int vertex, Vec3 *out);
const struct GrannyMeshVertex *c3d_granny_model_res_rigid_mesh_vertices(struct C3dGrannyModelRes *self, int mesh);
const struct GrannyMeshVertex *c3d_granny_model_res_mesh_to_draw(struct C3dGrannyModelRes *self, int mesh,
                                              struct Texture **tex, unsigned short **indices, int *tris,
                                              float *matrix, const struct GrannyMeshPose *pose,
                                              struct GrannyVertexBuffer *buffer);
BYTE granny_bounds_screen_box(const struct GrannyBounds *bounds, struct ScreenBounds *box,
                                            const float *view, const float *r, const float *eye);

void granny_animation_cache_init(struct C3dGrannyBoneRes *self);
void granny_animation_cache_clear(struct C3dGrannyBoneRes *self);
BYTE granny_animation_cache_load(struct C3dGrannyBoneRes *self, const char *name, int job, int part);
granny_animation *granny_animation_cache_get(struct C3dGrannyBoneRes *self, int job, int part);

#endif
