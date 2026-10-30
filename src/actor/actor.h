#ifndef ACTOR_ACTOR_H
#define ACTOR_ACTOR_H

#include "actor/render_object.h"

#include "actor/actor_common.h"

#include "actor/types.h"

struct GameObject *game_object_construct(struct GameObject *self);
void render_object_on_init(struct RenderObject *self);
void game_actor_on_init(struct GameActor *self);
void game_actor_release_attached(struct GameActor *self);

struct RagEffect *able_to_make_effect_launch_show_effect(struct GameActor *self, int effect_id, int show_id);
struct RagEffect *game_actor_launch_effect_timed(struct GameActor *self, int effect_id, int arg);

#endif
