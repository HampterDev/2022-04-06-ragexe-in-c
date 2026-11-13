#ifndef CONTENTS_BODY_LAYER_H
#define CONTENTS_BODY_LAYER_H

#include "base.h"
#include "std/std.h"
#include "resources/sprite_pair.h"
#include "actor/pc.h"
#include "actor/sprite_render.h"
#include "resources/animation.h"
#include "resources/sprite.h"

#define BODY_LAYER_COUNT 9
#define BODY_LAYER_BODY 0
#define BODY_LAYER_HEAD 1

struct QueuedLayer {
    int priority;
    struct SqQuad quad;
    void *palette;
    int flags;
};

struct LayerSprite {
    char present;
    struct SpriteResPair pair;
};

/* One player's layers drawn in one frame, on the stack: the sprites each layer draws, the view
   (item) of the worn layers, and the worn layers' quads queued by priority, at most one per
   priority (the first queued keeps it) and at most one per drawn layer order. */
struct BodyLayerDraw {
    float matrix[12];
    int opaque_override;
    signed char opaque_override_kind;
    struct Pc *pc;
    float pos[3];
    float screen_x, screen_y, screen_z;
    float screen_scale;
    float screen_shift;
    const struct ImfRes *imf;
    int layers_drawn;
    int offset_x, offset_y;
    char head_drawn;
    struct LayerSprite sprites[BODY_LAYER_COUNT];
    int views[BODY_LAYER_COUNT];
    struct ScreenBounds bounds;
    char any_drawn;
    struct QueuedLayer queued[BODY_LAYER_COUNT];
    int queued_count;
};

struct LayerClip {
    void *image;
    struct SprClip *clip;
    int dy, dx;
};

/* layer is 0 to BODY_LAYER_COUNT - 1. */
struct LayerSprite *body_layer_sprite_slot(struct BodyLayerDraw *self, int layer);
void body_layer_queue(struct BodyLayerDraw *self, int priority, const struct SqQuad *quad, void *palette,
                      int flags);

#define QUAD_BUILDER(pc, quad, palette, flags, x, y)                                               \
    render_object_submit_spr_quad((struct RenderObject *)(pc), (quad), (palette), (flags), (x), (y))

#endif
