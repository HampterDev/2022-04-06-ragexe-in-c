#ifndef RENDER_PRIMITIVE_TYPES_H
#define RENDER_PRIMITIVE_TYPES_H

#include <stdint.h>

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

struct gfx_mesh;

/* custom: a draw of part of a static world mesh (render/gfx.h), queued like the primitives above;
   view is the view matrix it is drawn with (render/rp_project.c) */
struct RPMeshDraw {
    struct gfx_mesh *mesh;
    int first, count;
    float view[12];
};

struct PrimTriangle {
    struct TexDrawVertex v[3];
    struct Texture *texture;
    int src_blend, dest_blend;
};

#endif
