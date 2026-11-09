#ifndef WORLD_MODEL_H
#define WORLD_MODEL_H

#include "actor/weather.h"
#include "render/keyframes.h"
#include "world/mode_manager.h"

#include "world/camera.h"
#include "world/ground.h"

#include "resources/model_res.h"

#include "world/model_types.h"

extern DWORD g_nodeCornerColours[];
struct ModeMgr;

int c3d_node_v1_load(struct C3dNode *base, struct ModelResNode *resource);
void c3d_node_v1_animate(struct C3dNode *base, double t);
void c3d_node_v1_render(struct C3dNode *base, const float *view, BYTE mirrored, BYTE layer);
void c3d_node_v1_cast(struct C3dNode *base, const float *view);
int c3d_node_v2_load(struct C3dNode *base, struct ModelResNode *resource);
void c3d_node_v2_animate(struct C3dNode *base, double t);
void c3d_node_v2_render(struct C3dNode *base, const float *view, BYTE mirrored, BYTE layer);
void c3d_node_v2_cast(struct C3dNode *base, const float *view);
void c3d_node_get_bound(struct C3dNode *self, float *box, const float *parent);
void c3d_node_light(struct C3dNode *self, const float *parent, const float *dir,
                              const float *diffuse, const float *ambient, int shadeType);

void c3d_actor_build_matrix(struct C3dActor *self);
void c3d_actor_set_speed(struct C3dActor *self, float speed);
void model_actor_v1_destroy(struct C3dActor *base);
int c3d_actor_v1_create(struct C3dActor *base, struct C3dModelRes *res);
void c3d_actor_v1_update_bound(struct C3dActor *base);
void c3d_actor_v1_light(struct C3dActor *base, const float *dir, const float *diffuse, const float *ambient);
void c3d_actor_v1_animate(struct C3dActor *base, double t);
void c3d_actor_v1_tick(struct C3dActor *base);
void c3d_actor_v1_render(struct C3dActor *base, const float *view);
void c3d_actor_v1_cast(struct C3dActor *base, const float *view);
void model_actor_v2_destroy(struct C3dActor *base);
int c3d_actor_v2_create(struct C3dActor *base, struct C3dModelRes *res);
void c3d_actor_v2_build_matrix(struct C3dActor *base);
void c3d_actor_v2_update_bound(struct C3dActor *base);
void c3d_actor_v2_light(struct C3dActor *base, const float *dir, const float *diffuse, const float *ambient);
void c3d_actor_v2_animate(struct C3dActor *base, double t);
void c3d_actor_v2_tick(struct C3dActor *base);
void c3d_actor_v2_render(struct C3dActor *base, const float *view);
void c3d_actor_v2_cast(struct C3dActor *base, const float *view);
void c3d_actor_v2_set_speed(struct C3dActor *base, float speed);

#endif
