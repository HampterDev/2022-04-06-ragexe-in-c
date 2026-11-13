#ifndef RENDER_DRAW_QUEUE_H
#define RENDER_DRAW_QUEUE_H

#include <stddef.h>
#include <stdint.h>
#include "render/gfx.h"

enum RenderPass {
    RENDER_OPAQUE_CLAMP,
    RENDER_ALPHA_DEPTH,
    RENDER_ALPHA_CLAMP,
    RENDER_OVERLAY_ORDERED,
    RENDER_ALPHA_REMAINDER,
    RENDER_OPAQUE_WRAP,
    RENDER_ALPHA_WRAP,
    RENDER_ALPHA_WRITE_DEPTH,
    RENDER_ADDITIVE_FOG,
    RENDER_ADDITIVE,
    RENDER_ADDITIVE_OVERLAY,
    RENDER_SHADOW_CASTERS,
    RENDER_BLENDED_WRAP,
    RENDER_BLENDED_NO_DEPTH,
    RENDER_ALPHA_NO_DEPTH,
    RENDER_ALPHA_CUTOUT,
    RENDER_MULTITEXTURE,
    RENDER_BLENDED_OVERLAY,
    RENDER_PASS_COUNT
};

struct RenderTexture {
    gfx_surface *surface;
    unsigned sort_key;
};

/* What a queue's entry is: a struct RPQuad; custom, a draw of the static world (struct
   RPWorldDraw) or of a Granny model's mesh (struct RPSkinDraw), render/primitive_types.h */
enum { RP_ENTRY_QUAD, RP_ENTRY_WORLD, RP_ENTRY_SKIN };

struct RenderEntry {
    void *primitive;
    float key;
    unsigned order;
    struct RenderTexture textures[2];
    unsigned char kind;
};

struct RenderQueue {
    struct RenderEntry *items;
    size_t count, capacity;
};

#endif
