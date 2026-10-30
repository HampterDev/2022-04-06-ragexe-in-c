#ifndef CONTENTS_BODY_LAYER_H
#define CONTENTS_BODY_LAYER_H

#include "base.h"
#include "std/std.h"
#include "std/tree.h"
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
    int layer;
    struct SpriteResPair pair;
};

struct LayerView {
    int layer;
    int view;
};

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
    struct Tree sprites;
    struct Tree views;
    struct ScreenBounds bounds;
    char any_drawn;
    struct Tree queued;
};

struct LayerClip {
    void *image;
    struct SprClip *clip;
    int dy, dx;
};

struct LayerSprite *body_layer_sprite_slot(struct BodyLayerDraw *self, int layer);
int *body_layer_view_slot(struct BodyLayerDraw *self, int layer);

void body_layer_destroy(struct BodyLayerDraw *self);

#define QUAD_BUILDER(pc, quad, palette, flags, x, y)                                               \
    render_object_submit_spr_quad((struct RenderObject *)(pc), (quad), (palette), (flags), (x), (y))

#endif
