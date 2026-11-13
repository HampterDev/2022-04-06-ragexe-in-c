#include "render/screen_bounds.h"
#ifndef RESOURCES_GRANNY_RES_H
#define RESOURCES_GRANNY_RES_H

#include "fileio/resource_types.h"
#include <stdint.h>
#include "resources/granny_sdk.h"
#include "render/vec3.h"

struct Texture;
struct gfx_skin;

struct GrannyResTexture {
    const char *name;
    struct Texture *texture;
};

/* custom: a mesh and what it draws with: the texture of its first triangle group's material
   (NULL: none, and it is not drawn, but for the guild flag's mesh 1, which actor/granny.c draws
   with the guild's emblem whatever its own texture), its triangles in the model's indices on the device
   (C3dGrannyModelRes.skin: firstIndex to firstIndex + indexCount, none where it has no triangle),
   and the bone that moves it where it is rigid, -1 where it is skinned. */
struct GrannyResMesh {
    granny_mesh *mesh;
    struct Texture *texture;
    uint32_t firstIndex, indexCount;
    int rigidBone;
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
    /* custom: its meshes' vertices and indices on the device, which skins them (render/gfx.h,
       gfx_create_skin); NULL where no mesh has a triangle or the skeleton has more bones than the
       device takes (render/skin_layout.h, SKIN_BONES_MAX), and nothing of it is drawn */
    struct gfx_skin *skin;
};

enum { GRANNY_BONE_JOB_COUNT = 10, GRANNY_BONE_ANIMATION_COUNT = 5 };
/* unavailable: the animation's file was looked for and is not there, or is there and failed to
   load; neither looked for nor loaded again until granny_animation_cache_clear. */
struct GrannyAnimationEntry {
    granny_file *file;
    granny_animation *animation;
    unsigned char unavailable;
};
struct C3dGrannyBoneRes {
    struct GrannyAnimationEntry entries[GRANNY_BONE_JOB_COUNT][GRANNY_BONE_ANIMATION_COUNT];
};
extern struct C3dGrannyBoneRes g_grannyBoneRes;

BYTE c3d_granny_model_res_load(struct C3dGrannyModelRes *self, const char *name);
void granny_model_resource_clear(struct C3dGrannyModelRes *self);
void granny_model_resource_finish(struct C3dGrannyModelRes *self);
/* custom: the model's meshes on the device (C3dGrannyModelRes.skin) and each mesh's triangles
   among them, from the meshes' vertices and indices as the file holds them (the whole triangles of
   each; indices past the last whole one are neither drawn nor checked); 0 without memory or where an
   index of a triangle names no vertex of its mesh (the load then fails), 1 where it made them or
   makes none: no triangle, a skeleton of more than SKIN_BONES_MAX bones, or more than 2^32 - 1
   vertices or indices in all (the model is then not drawn). */
int granny_model_res_skin(struct C3dGrannyModelRes *self);
/* Where Granny's deformer put vertex `vertex` of mesh `mesh` in the mesh's space, for the pose's
   bones: 0 where the model, mesh or vertex is not there. */
int granny_mesh_vertex_position(const struct C3dGrannyModelRes *model, int mesh, granny_world_pose *pose,
                               int vertex, Vec3 *out);
BYTE granny_bounds_screen_box(const struct GrannyBounds *bounds, struct ScreenBounds *box,
                                            const float *view, const float *r, const float *eye);

/* The model name ("model\\3dmob\\" + name) that job `job`'s bone animations belong to,
   or NULL. Defined in resources/granny_res.c; client_install_hooks()
   (core/client_init.c) installs session_granny_job_model (session/session_granny_hooks.c).
   NULL: no job has one, so no bone animation loads. */
extern const char *(*g_grannyJobModelName)(int job);

void granny_animation_cache_init(struct C3dGrannyBoneRes *self);
void granny_animation_cache_clear(struct C3dGrannyBoneRes *self);
BYTE granny_animation_cache_load(struct C3dGrannyBoneRes *self, const char *name, int job, int part);
granny_animation *granny_animation_cache_get(struct C3dGrannyBoneRes *self, int job, int part);

#endif
