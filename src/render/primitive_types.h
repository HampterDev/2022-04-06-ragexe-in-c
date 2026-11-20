#ifndef RENDER_PRIMITIVE_TYPES_H
#define RENDER_PRIMITIVE_TYPES_H

#include <stdint.h>
#include "render/gfx.h"

struct Texture;

/* A list of triangles (a triangle of each three vertices) or a strip of four (a quad), RPQuad.prim */
#define RP_LIST 4
#define RP_STRIP 5

/* A quad, a triangle or a list of triangles the renderer queues: pre-transformed vertices (struct
   TexDrawVertex, render/quads.h), or an instance the client made (render/gfx.h, gfx_draw_quad). The
   renderer's pools hand out their entries as the last user left them (rp_pool.h): a field a user
   does not set keeps that user's value. */
struct RPQuad {
    struct TexDrawVertex *verts;
    int nverts;
    struct Texture *texture;
    unsigned char alpha_mode;
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
   records (RP_WORLD_RECORD, an alpha-blended model's, drawn with `draw`), or a cell of its water
   (RP_WORLD_WATER, drawn in `colour`). */
enum { RP_WORLD_RECORD, RP_WORLD_WATER };

struct RPWorldDraw {
    int kind;
    struct SwRecord record;
    struct GfxWorldDraw draw;
    uint32_t cell;
    uint32_t colour;
};

/* custom: a Granny actor's pose for the frame (render/skin_layout.h, skin_pose_rows): its rows as
   the skinning shader reads them, and the row the device gave them among the frame's quads when the
   first of its meshes was drawn (-1 before; render/gfx.h, gfx_skin_pose). */
struct RPSkinPose {
    float (*rows)[4];
    size_t rowCount;
    int row;
};

/* custom: one of a Granny model's meshes drawn in an actor's pose: `count` of the model's indices
   from `first` (render/gfx.h, gfx_draw_skin) */
struct RPSkinDraw {
    struct RPSkinPose *pose;
    gfx_skin *model;
    uint32_t first, count;
};

/* custom: a corner of a triangle in the world: its point and texture coordinates */
struct PrimCorner {
    float pos[3];
    float u, v;
};

/* One of an effect primitive's triangles, with its texture and blending: pre-transformed vertices
   (renderer_add_triangle), or, custom, corners in the world in one colour, which the device
   projects (effects/rag/effect_prim_draw.c, prim_add_world_triangles) */
struct PrimTriangle {
    union {
        struct TexDrawVertex v[3];
        struct {
            struct PrimCorner corner[3];
            uint32_t colour;
        } world;
    };
    struct Texture *texture;
    int src_blend, dest_blend;
};

#endif
