#ifndef RENDER_PRIMITIVE_TYPES_H
#define RENDER_PRIMITIVE_TYPES_H

#include <stdint.h>
#include "render/gfx.h"

struct Texture;

/* A list of triangles (a triangle of each three vertices) or a strip of four (a quad), RPQuad.prim */
#define RP_LIST 4
#define RP_STRIP 5

/* A quad, a triangle or a list of triangles the renderer queues: vertices (struct TexDrawVertex,
   render/quads.h), pre-transformed or in world coordinates, or an instance the client made
   (render/gfx.h, gfx_draw_quad). The renderer's pools hand out their entries as the last user left
   them (rp_pool.h): a field a user does not set keeps that user's value. */
struct RPQuad {
    struct TexDrawVertex *verts;
    int nverts;
    struct Texture *texture;
    unsigned char alpha_mode;
    /* custom: the vertices' x, y and z are world coordinates, a list of triangles (Granny's;
       render/gfx.h, gfx_draw_vertices) */
    unsigned char world;
    uint32_t srcBlend, destBlend;
    int stageMode;
    int prim;
    /* custom: drawn as this instance instead of vertices; the lists sort it by the rhw of its first
       two corners, as they sort vertices by theirs */
    struct GfxQuad *instance;
    float rhw[2];
};

/* custom: a quad of four vertices or, in their place, an instance (base.instance points to it) */
struct TexDrawPrim {
    struct RPQuad base;
    union {
        struct TexDrawVertex quad[4];
        struct GfxQuad instance;
    };
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
