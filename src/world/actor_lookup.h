#ifndef WORLD_ACTOR_LOOKUP_H
#define WORLD_ACTOR_LOOKUP_H

struct World;
struct GameActor;

struct GameActor *world_get_game_actor_by_aid(struct World *world, unsigned aid);
void *world_find_pc_by_char_id(struct World *world, int character_id);

#endif
