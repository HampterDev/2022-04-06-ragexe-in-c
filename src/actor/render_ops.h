#include "actor/object_ops.h"
#include "render/screen_bounds.h"
#ifndef ACTOR_RENDER_OPS_H
#define ACTOR_RENDER_OPS_H
#include "actor/types.h"

struct SprClip;
struct SprImg;
struct SqQuad;

void render_object_update_motion_base(struct RenderObject *object);
void c3d_granny_game_actor_update_motion(struct RenderObject *object);
unsigned char render_object_clip_to_quad(struct RenderObject *object, int index, int layer,
    float scale, const struct SprClip *clip, float x, float y, const float *view,
    float shift, float *depth_a, float *depth_b, const struct SprImg *image,
    struct SqQuad *quad, int *rect, unsigned *colour, int dx, int dy,
    unsigned char fixed_step);
void render_object_draw_echoes(struct RenderObject *object, const struct SprClip *clip,
    const int *rect, float screen_x, float depth_a, float depth_b, void *palette,
    void *image, float scale, short hover);
void render_object_draw_rings(struct RenderObject *object, struct SqQuad *quad,
    const struct SprClip *clip, const int *rect, float screen_x, float depth_a,
    float depth_b, void *palette, void *image, int flags, short hover);
void render_object_jitter_quad(struct RenderObject *object, struct SqQuad *quad,
    const int *rect, float screen_x, int range, int bias);
void render_object_set_screen_position(struct RenderObject *object, float x, float y);

void render_object_set_action_base(struct RenderObject *object, int action, int type);
void game_actor_set_action(struct RenderObject *object, int action, int type);
void c3d_granny_game_actor_set_action(struct RenderObject *object, int action, int type);

void game_object_set_render_info(struct RenderObject *object, const struct ScreenBounds *bounds, float x, float y);
void game_actor_set_render_info(struct RenderObject *object, const struct ScreenBounds *bounds, float x, float y);
void skill_render_body(struct RenderObject *object, const struct ScreenBounds *bounds, float x, float y);
void item_add_pick_box(struct RenderObject *object, const struct ScreenBounds *bounds, float x, float y);

int game_actor_get_effect_state(struct RenderObject *object);
int render_object_return_zero(struct RenderObject *object);

struct RenderObjectOps {
    struct GameObjectOps base;
    void (*render_body)(struct RenderObject *, const struct ScreenBounds *bounds, float x, float y);
    void (*set_action)(struct RenderObject *, int action, int type);
    void (*update_motion)(struct RenderObject *);
    int (*get_effect_state)(struct RenderObject *);
};

extern const struct RenderObjectOps render_object_ops;
extern const struct RenderObjectOps able_to_make_effect_ops;
extern const struct RenderObjectOps mouse_pointer_ops;
extern const struct RenderObjectOps sound_maker_ops;
extern const struct RenderObjectOps corpse_ops;
extern const struct RenderObjectOps arrow_effect_ops;
extern const struct RenderObjectOps effect_maker_ops;
extern const struct RenderObjectOps ground_scope_ops;
extern const struct RenderObjectOps mandra_attack_effect_ops;
extern const struct RenderObjectOps msg_effect_ops;
extern const struct RenderObjectOps item_ops;

static __inline const struct RenderObjectOps *render_object_ops_of(const struct RenderObject *object)
{
    return (const struct RenderObjectOps *)object->ops;
}

static __inline void render_object_update_motion(struct RenderObject *object)
{
    render_object_ops_of(object)->update_motion(object);
}

static __inline int render_object_direction(struct RenderObject *object, float roty)
{
    return game_object_direction((struct GameObject *)object, roty);
}

static __inline void render_object_set_action(struct RenderObject *object, int action, int type)
{
    render_object_ops_of(object)->set_action(object, action, type);
}

static __inline void render_object_render_body(struct RenderObject *object,
    const struct ScreenBounds *bounds, float x, float y)
{
    render_object_ops_of(object)->render_body(object, bounds, x, y);
}

static __inline void render_object_send(void *object, void *sender, RenderMsgArg msg, RenderMsgArg par1,
                                        RenderMsgArg par2, RenderMsgArg par3, RenderMsgArg par4)
{
    game_object_send((struct GameObject *)object, (struct GameObject *)sender, msg, par1, par2, par3, par4);
}

#endif
