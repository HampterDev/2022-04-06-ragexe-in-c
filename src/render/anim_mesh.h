#ifndef RENDER_ANIM_MESH_H
#define RENDER_ANIM_MESH_H

#include <stddef.h>
#include <stdint.h>
#include "render/vec3.h"

struct AnimMeshTexVertex {
    uint32_t colour;
    float u, v;
};

struct AnimMeshFace {
    uint16_t v[3];
    uint16_t tv[3];
    uint16_t texture;
    int twoSided;
    int *groups;
    size_t groupCount;
};

struct AnimMesh {
    int vertexCount;
    Vec3 *positions;
    int faceCount;
    struct AnimMeshFace *faces;
    int texVertCount;
    struct AnimMeshTexVertex *texVerts;
    Vec3 *faceNormals;
    Vec3 *vertexNormals;
};

void anim_mesh_destroy(struct AnimMesh *mesh);
int anim_mesh_smooth_normals(struct AnimMesh *mesh);

#endif
