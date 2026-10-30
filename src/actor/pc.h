#include "actor/actor_ops.h"
#include "actor/object_ops.h"
#include "actor/render_ops.h"
#ifndef ACTOR_PC_H
#define ACTOR_PC_H
#include "actor/appearance.h"

#include "actor/game_actor.h"

struct GrannyPc;

void pc_destruct(struct Pc *self);

void pc_set_layer_act(struct Pc *self, int layer, const char *name);
void pc_set_layer_spr(struct Pc *self, int layer, const char *name);

void player_refresh_status_effects(struct Player *self);

#endif
