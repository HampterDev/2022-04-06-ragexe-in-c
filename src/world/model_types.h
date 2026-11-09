#ifndef WORLD_MODEL_TYPES_H
#define WORLD_MODEL_TYPES_H

#include "base.h"
#include "render/keyframes.h"
#include "world/frustum.h"

struct C3dActor;
struct C3dNode;
struct C3dModelRes;
struct ModelResNode;
struct AnimMesh;
struct Ray3;

struct ModelActorOps {
    void (*destroy)(struct C3dActor *actor);
    int (*create)(struct C3dActor *actor, struct C3dModelRes *resource);
    void (*light)(struct C3dActor *actor, const float *direction, const float *diffuse, const float *ambient);
    void (*animate)(struct C3dActor *actor, double time);
    void (*tick)(struct C3dActor *actor);
    void (*render)(struct C3dActor *actor, const float *view, BYTE clip);
    void (*set_speed)(struct C3dActor *actor, float speed);
};

struct FaceInfo {
    DWORD colour[3];
    float uv[3][2];
};
SIZE_ASSERT(FaceInfo, 0x24);

/* custom: a node's faces as a static mesh the device projects (render/gfx.h), built on its first
   draw (world/model_node_render.c). Only faces whose texture inMesh marks are in it: not alpha-keyed
   (format 1) and, on version 2 nodes, without a texture-coordinate animation. A draw is a run of
   faces with one texture, and with or without twoSided. The other faces are in casterMesh, which
   only casts shadows: a draw there is a run of alpha-keyed faces with their texture, which the
   shadow pass tests, or of the rest without one. */
struct ModelGpuDraw {
    struct Texture *texture;
    int first, count;
    BYTE twoSided;
};

struct ModelGpu {
    struct gfx_mesh *mesh, *casterMesh;
    struct ModelGpuDraw *draws, *casterDraws;
    int drawCount, casterDrawCount;
    BYTE *inMesh;
    int inMeshCount;
    int allInMesh;
    int groundLighting;
};

struct C3dNode {
    const char *name;
    struct ModelResNode *resource;
    struct C3dActor *actor;
    struct C3dNode *parent;
    struct C3dNode **children;
    size_t childCount;
    struct AnimMesh *mesh;
    float matrix[12];
    struct Texture *const *textures;
    size_t textureCount;
    float alpha;
    struct FaceInfo *faceInfo;
    int *lightInts;
    BYTE highlight;
    struct ModelGpu *gpu;
    int gpuState;
};

void model_node_gpu_free(struct C3dNode *node);

struct C3dNodeV1 {
    struct C3dNode base;
    struct KeyPos restPos;
    struct KeyQuat restRot;
    struct KeyScaleAxis restScale;
};

struct UvTransform {
    int texture;
    float offset[2], scale[2], angle;
};

struct C3dNodeV2 {
    struct C3dNode base;
    float offset[12], local[12];
    struct UvTransform *uvXforms;
    size_t uvXformCount;
};

struct C3dNode *model_node_create(int version, struct C3dActor *actor,
                                struct C3dNode *parent, struct ModelResNode *resource);
void model_node_destroy(struct C3dNode *node, int version);

#include "resources/model_volume.h"

struct C3dActor {
    const struct ModelActorOps *ops;
    float pos[3];
    float rot[3];
    float scale[3];
    float offset[3];
    float matrix[12];
    float inverse[12];
    BYTE dirty;
    struct FrustumObb obb;
    float corners[8][3];
    float bboxMin[3];
    float bboxMax[3];
    int lastFrame;
    BYTE pickable;
    BYTE picked;
    BYTE alpha;
    BYTE layer;
    struct VolumeBox *volumeBoxes;
    size_t volumeBoxCount;
    int animType;
    float speed;
    int shadeType;
};

struct C3dActorV1 {
    struct C3dActor base;
    struct C3dNode *root;
    int frame;
    int frameCount;
    DWORD lastTick;
};

struct C3dActorV2 {
    struct C3dActor base;
    struct C3dNode **nodes;
    size_t nodeCount;
    float fps;
    double duration;
    DWORD startTick;
};

struct C3dActor *model_actor_create(int version);
int c3d_actor_add_volume_boxes(struct C3dActor *actor, const struct VolumeBox *boxes, size_t count);
void c3d_actor_pick(struct C3dActor *actor, const struct Ray3 *ray);

#endif
