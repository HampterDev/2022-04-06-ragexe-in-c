#ifndef WORLD_WORLD_H
#define WORLD_WORLD_H

#include "platform/error_message.h"
#include "world/actor_lookup.h"
#include "world/lifecycle.h"
#include "world/object_lookup.h"
#include "world/ground_api.h"
#include "world/models.h"
#include "contents/guild_info.h"
#include "game/context.h"
#include "world/scene_graph.h"
#include "resources/world_res.h"
#include "world/player.h"
#include "world/load.h"

struct CellIds {
    float cell;
    struct List ids;
};

#include "world/state.h"

struct GameModeView {
    struct World *world;
};

#define WORLD_NEAR_MODEL_DIST   200.0f

int cell_ids_less(const void *a, const void *b);
void cell_ids_destroy(void *value);

void world_forget_id_in_cell_map(struct World *self, int id, BYTE notify);
void world_forget_id_in_list_ec(struct World *self, int id, BYTE notify);
void world_update_near_flag_for_id(struct World *self, int id, const float *pos);
void world_update_near_flag_at(struct World *self, const float *x, const float *y);
BYTE world_any_matching_actor_near(struct World *self, const float *x, const float *y);
struct C3dActor;
struct C3dActorV1;
struct C3dActorV2;
#include "actor/render_ops.h"

void *world_get_actor_or_player(struct World *self, int aid);
BYTE world_actor_matches_global(struct World *self, int aid);
void world_remove_object(struct World *self, void *object);
BYTE world_remove_item(struct World *self, int aid);
BYTE world_remove_skill(struct World *self, int aid);
void world_place_id_at_cell(struct World *self, int id, const float *cell);
void world_place_id_at_client_cell(struct World *self, int id, const int *cell);
void world_place_id_nowhere(struct World *self, int id, const float *pos);
void world_send_msg_to_object(struct World *self, void *sender, int key1, int key2,
                                       int a, int b, int c, int d, int e);

void world_load_effects_and_sounds(struct World *self);

void world_process_actors(struct World *self);
char world_remove_actor_by_aid(struct World *self, unsigned aid);
void world_clear_objects(struct World *self);

void world_free_scene_graph(struct World *self);
int world_build_scene_graph(struct World *self);

#endif
