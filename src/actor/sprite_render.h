#include "render/screen_bounds.h"
#include "actor/render_ops.h"
#include "actor/layer_order.h"
#include "resources/sprite.h"
#ifndef ACTOR_SPRITE_RENDER_H
#define ACTOR_SPRITE_RENDER_H

#include "actor/actor.h"
#include "actor/sprite_draw.h"

struct SprImg;
struct BodyLayerDraw;

struct SqQuad {
    float top, x0, bottom, x1;
    float depth_a, depth_b;
    unsigned color;
    float angle;
    struct SprImg *image;
};

SIZE_ASSERT(ScreenBounds, 0x14);

void bounds_add_quad(struct ScreenBounds *b, const struct SqQuad *quad);

void render_object_render_mercenary(struct RenderObject *self, const float *matrix, int job);
void render_object_submit_flipped_quad(struct RenderObject *self, struct SqQuad *quad,
                                                void *palette, int flags, float x, float y);

void render_object_draw_tint_quad(struct RenderObject *self, struct SqQuad *quad, short r, short g, short b, void *palette);
unsigned short render_object_hover_height(struct RenderObject *self);
void render_object_apply_pulse_scale(struct RenderObject *self, float *scale);
void render_object_swing_point(struct RenderObject *self, int layer, int *x, int *y,
                                         int *offset_x, int *offset_y);
void render_object_draw_flashes(struct RenderObject *self, struct SqQuad quad, void *palette);

void pc_render_body_layers(struct Pc *pc, const float *matrix, int mode, char flag);
struct Vec *pc_layer_act_names(struct Pc *pc, struct Vec *out);
struct Vec *pc_layer_spr_names(struct Pc *pc, struct Vec *out);
void pc_place_head(struct Pc *pc);
struct SprRes *pc_layer_spr(struct Pc *pc, int layer);
struct ActRes *pc_layer_act(struct Pc *pc, int layer);
int layer_clip_index(int layer);
char action_is_ready_or_attack(int action);
char layer_is_ignored_riding_state(int view, int job, int base_action, char mounted);
int layer_body_attach_dy(struct Motion *head_motion, int action, int motion, int job, int sex, int body_style);
void pc_render_robe(struct Pc *pc, int on_top_pass,
                               int *layers_drawn, const float *pos, float x, float y, float screen_scale, float screen_shift);
void pc_render_body_alone(struct Pc *pc, int layer,
                                    const float *pos, float x, float y, float screen_scale, float screen_shift, float z);
struct Motion *pc_get_layer_motion(struct Pc *pc, int layer, struct ActRes *act,
                                              int *action_out, int *motion_out, char *use_out);
void pc_layer_depth_bias(int layer, struct SqQuad *quad, float a, float b);
void pc_layer_depth_bias_by_view(struct Pc *pc, int layer, struct SqQuad *quad, float a, float b);

void pc_draw_layer_overlays(struct Pc *pc, struct SqQuad *quad, struct SprClip *clip,
                                      int layer, const int *rect, float screen_shift, float z, float a, float b, float x, float y,
                                      void *palette, void *image, int *flags, int dx, int dy);
void render_object_draw_clip_overlays(struct RenderObject *self, struct SqQuad *quad, const struct SprClip *clip, const int *rect, float screen_shift, float depth_a, float depth_b, void *palette, void *image, int *flags, unsigned colour);
void render_object_draw_pulse_outline(struct RenderObject *self, const struct SprClip *clip, const int *rect, float screen_shift, float depth_a, float depth_b, void *palette, void *image, unsigned colour);

void body_layer_render(struct BodyLayerDraw *self, const float *matrix, int opaque_override, char opaque_override_kind);

#endif
