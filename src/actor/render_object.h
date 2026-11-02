#include "actor/object_ops.h"
#ifndef ACTOR_RENDER_OBJECT_H
#define ACTOR_RENDER_OBJECT_H
#include "actor/types.h"

struct RenderObject *render_object_construct(struct RenderObject *self);
void render_object_dc_render_object(struct RenderObject *self);
struct AbleToMakeEffect *able_to_make_effect_construct(struct AbleToMakeEffect *self);
void able_to_make_effect_dc_able_to_make_effect(struct AbleToMakeEffect *self);
struct GameActor *game_actor_construct(struct GameActor *self);
unsigned char game_actor_update_sphere_count(struct GameActor *self, int64_t count);
struct Corpse *corpse_construct(struct Corpse *self);
void game_actor_dc_game_actor(struct GameActor *self);

void render_object_load_spr_res(struct RenderObject *self, const char *name);
void render_object_load_act_res(struct RenderObject *self, const char *name);
void *render_object_get_act_res(struct RenderObject *self);
void render_object_set_motion_delay(struct RenderObject *self, float delay);
void render_object_set_argb(struct RenderObject *self, int a, int r, int g, int b);
void render_object_process_sound(struct RenderObject *self);
float render_object_calc_dist(struct RenderObject *self, float x, float y, float z);
float render_object_calc_ground_dist(struct RenderObject *self, float x, float z);
void render_object_turn_toward(struct RenderObject *self, float x, float z);
int render_object_camera_angle(float roty);
int render_object_camera_dir(float roty, int rounded);
unsigned char render_object_get_actor_pos(struct RenderObject *self, int aid, Vec3 *out);
struct RagEffect *game_actor_launch_effect(struct GameActor *self, int effect_id,
                                         Vec3 offset, float rotation);
#endif
