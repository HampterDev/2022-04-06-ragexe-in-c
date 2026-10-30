#ifndef RESOURCES_MODEL_RES_H
#define RESOURCES_MODEL_RES_H

#include "platform/error_message.h"
#include "std/string.h"
#include "resources/resource_ops.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "resources/resource_factories.h"
#include "resources/model_node_names.h"
#include "resources/model_volume.h"
#include "render/anim_mesh.h"
#include "render/keyframes.h"
#include "core/text_array.h"

struct C3dModelRes;

struct ModelResNode {
    void (*destroy)(struct ModelResNode *node);
    struct OwnedText name, parentName;
    struct C3dModelRes *model;
    struct ModelResNode *parent;
    struct ModelResNode **children;
    size_t childCount;
    struct Texture **textures;
    size_t textureCount;
    struct AnimMesh *mesh;
    unsigned char alpha;
};

struct ModelResNodeV1 {
    struct ModelResNode base;
    float pos[3], axis[3], angle, scale[3];
    struct KeyQuat *rotKeys;
    size_t rotKeyCount;
};

struct ModelResNodeV2 {
    struct ModelResNode base;
    float matrix[12];
    struct KeyVec *scaleKeys, *posKeys;
    struct KeyQuat *rotKeys;
    size_t scaleKeyCount, posKeyCount, rotKeyCount;
    struct UvAnimation uvKeys;
};

struct C3dModelRes {
    struct Res res;
    int version;
    struct ModelResNode **nodes;
    size_t nodeCount;
    struct TextArray mainNodes;
    struct VolumeBox *blocks;
    size_t blockCount;
    struct OwnedText fileName;
    int shadeType;
    unsigned char alpha;
    struct TextArray videoNames;
    float fps;
    int frameCount;
};

void model_res_init(struct C3dModelRes *model);
void model_res_clear(struct C3dModelRes *model);
struct ModelResNode *model_res_node_create(int version);
struct ModelResNode *model_res_find_node(const struct C3dModelRes *model, const char *name);
void model_res_place_videos(const struct C3dModelRes *model, Vec3 position);
BYTE model_resource_load_v1(struct C3dModelRes *model, const char *name);
BYTE model_resource_load_v2(struct C3dModelRes *model, const char *name);
void model_res_node_v1_destroy(struct ModelResNode *node);
void model_res_node_v2_destroy(struct ModelResNode *node);

extern const char g_model_res_not_a_model[];

#endif
