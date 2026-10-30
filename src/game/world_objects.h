#ifndef GAMEMODE_WORLD_OBJECTS_H
#define GAMEMODE_WORLD_OBJECTS_H

#include "render/vec3.h"

struct GameMode;
struct GameActor;
struct Corpse;
struct Corpse *game_mode_make_corpse(struct GameMode *mode, Vec3 pos, const char *act,
                                    const char *spr, int action, int motion, float facing);
unsigned long long game_mode_launch_dummy_effect(struct GameMode *mode, const Vec3 *at,
                                                int effect_id, int frames, int value);
void game_mode_make_granny_corpse(struct GameMode *mode, Vec3 pos, float facing, const char *name);

void game_mode_launch_arrow(struct GameMode *self, const struct Vec3 *from, unsigned target,
                          float speed, int job);

void game_mode_launch_plant_attack(struct GameMode *self, unsigned target_id, int job, float delay);

struct GameActor *game_mode_new_actor_for_job(struct GameMode *self, int job);

void game_mode_launch_arrow_to_point(struct GameMode *self, const float *from, const float *to, float speed);
void game_mode_make_mouse_pointer(struct GameMode *self);

#endif
