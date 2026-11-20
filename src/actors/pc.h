#include "actors/actor_ops.h"
#include "actors/object_ops.h"
#include "actors/render_ops.h"
#ifndef ACTOR_PC_H
#define ACTOR_PC_H
#include "actors/appearance.h"

#include "actors/game_actor.h"

void pc_destruct(struct Pc *self);

void pc_set_layer_act(struct Pc *self, int layer, const char *name);
void pc_set_layer_spr(struct Pc *self, int layer, const char *name);

void player_refresh_status_effects(struct Player *self);

#endif
