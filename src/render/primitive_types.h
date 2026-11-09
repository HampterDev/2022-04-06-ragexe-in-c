#ifndef RENDER_PRIMITIVE_TYPES_H
#define RENDER_PRIMITIVE_TYPES_H

#include <stdint.h>
#include "render/gfx.h"

struct Texture;

struct TexDrawVertex {
    float x, y, z, rhw;
    uint32_t diffuse, specular;
    float u, v;
};

struct RPQuad {
    struct TexDrawVertex *verts;
    int nverts;
    struct Texture *texture;
    void *indices;
    int nindices;
    unsigned char alpha_mode;
    uint32_t srcBlend, destBlend;
    int stageMode;
    int prim;
};

struct TexDrawPrim {
    struct RPQuad base;
    struct TexDrawVertex quad[4];
};

struct RPTwoTexVertex {
    float x, y, z, rhw;
    uint32_t diffuse, specular;
    float u0, v0, u1, v1;
};

struct RPTwoTex {
    struct RPTwoTexVertex *verts;
    int nverts;
    unsigned short *indices;
    int nindices;
    struct Texture *tex0, *tex1;
    int prim;
};

struct RPTwoTexQuad {
    struct RPTwoTex prim;
    struct RPTwoTexVertex quad[4];
};

/* custom: a draw of part of a static mesh (render/gfx.h), queued among the primitives above; view
   takes the mesh's own coordinates to view space (render/rp_project.c), and cull is the cull mode
   it is drawn with, 0 for the one its queue's flush has set */
struct RPMeshDraw {
    struct gfx_mesh *mesh;
    float view[12];
    int cull;
    struct GfxMeshDraw draw;
};

struct PrimTriangle {
    struct TexDrawVertex v[3];
    struct Texture *texture;
    int src_blend, dest_blend;
};

#endif
