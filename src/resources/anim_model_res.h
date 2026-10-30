#ifndef RESOURCES_ANIM_MODEL_RES_H
#define RESOURCES_ANIM_MODEL_RES_H

#include "fileio/resource_types.h"
#include "core/owned_text.h"
#include "render/math3d.h"

struct RsxTextureVertex { unsigned char data[12]; };
struct RsxFace { unsigned char data[24]; };
SIZE_ASSERT(RsxTextureVertex, 12);
SIZE_ASSERT(RsxFace, 24);

struct AnimatedModelFrame {
    Vec3 *vertices;
    struct RsxTextureVertex *textureVertices;
};
struct AnimatedModelNode {
    struct OwnedText name, parentName;
    struct AnimatedModelNode *parent;
    struct AnimatedModelNode **children;
    size_t childCount;
    struct AnimatedModelFrame *frames;
    size_t frameCount, vertexCount, textureVertexCount;
    struct RsxFace *faces;
    size_t faceCount;
    struct Texture **textures;
    size_t textureCount;
    int sharedTextureVertices;
};

struct AnimatedModelResource {
    struct Res res;
    struct AnimatedModelNode **nodes;
    size_t nodeCount, frameCount;
    struct OwnedText mainNode;
    int headerValue0, headerValue1;
    float version12Scalar;
    int version12Values[3], version13Value;
};

struct Res *animated_model_resource_create(void);
#endif
