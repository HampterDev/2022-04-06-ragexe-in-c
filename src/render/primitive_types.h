#ifndef RENDER_PRIMITIVE_TYPES_H
#define RENDER_PRIMITIVE_TYPES_H

#include <stdint.h>
#include "render/gfx.h"

struct Texture;

struct TexDrawVertex {
    float x, y, z, rhw;
    uint32_t diffuse;
    float u, v;
};

struct RPQuad {
    struct TexDrawVertex *verts;
    int nverts;
    struct Texture *texture;
    void *indices;
    int nindices;
    unsigned char alpha_mode;
    /* custom: the vertices' x, y and z are world coordinates (GFX_VTX_WORLD, render/gfx.h) */
    unsigned char world;
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
    uint32_t diffuse;
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

/* custom: a draw of the static world (render/gfx.h) queued among the primitives above: one of its
   records (RP_WORLD_RECORD, an alpha-blended model's, drawn with `draw`), a caster's triangles
   (RP_WORLD_CAST: count indices from firstIndex after baseVertex, of textureArray, whose records'
   ids lie below ids), or a cell of its water (RP_WORLD_WATER, drawn in `colour`). */
enum { RP_WORLD_RECORD, RP_WORLD_CAST, RP_WORLD_WATER };

struct RPWorldDraw {
    int kind;
    struct SwRecord record;
    struct GfxWorldDraw draw;
    uint32_t firstIndex, count, baseVertex, ids;
    uint8_t textureArray;
    uint32_t cell;
    uint32_t colour;
};

struct PrimTriangle {
    struct TexDrawVertex v[3];
    struct Texture *texture;
    int src_blend, dest_blend;
};

#endif
