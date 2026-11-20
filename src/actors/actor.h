#ifndef ACTOR_ACTOR_H
#define ACTOR_ACTOR_H

#include "actors/render_object.h"

#include "actors/actor_common.h"

#include "actors/types.h"

struct GameObject *game_object_construct(struct GameObject *self);
void render_object_on_init(struct RenderObject *self);
void game_actor_on_init(struct GameActor *self);
void game_actor_release_attached(struct GameActor *self);

struct RagEffect *able_to_make_effect_launch_show_effect(struct GameActor *self, int effect_id, int show_id);

#endif
