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
    /* custom: the vertices' x, y and z are world coordinates (render/gfx.h, gfx_draw_vertices) */
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

/* custom: a draw of part of a static mesh (render/gfx.h), queued among the primitives above; view
   takes the mesh's own coordinates to view space (render/rp_project.c) */
struct RPMeshDraw {
    struct gfx_mesh *mesh;
    float view[12];
    struct GfxMeshDraw draw;
};

struct PrimTriangle {
    struct TexDrawVertex v[3];
    struct Texture *texture;
    int src_blend, dest_blend;
};

#endif
