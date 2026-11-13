#ifndef WORLD_AID_INDEX_H
#define WORLD_AID_INDEX_H

/* custom: the world's actors and skill units found by their aid, each list with an index by aid
   (world/id_index.h) in place of the original's walk of the list: the actors (struct World, actors
   and actorIndex; GetGameActorByAID, 0x65c580, world_get_game_actor_by_aid in world/actor_lookup.h)
   and the skill units (skills and skillIndex; sub_65c6a0, world_get_skill_by_aid). Each node of a
   list holds the object, first, then its link in the index (struct ActorEntry, struct SkillEntry),
   so the index's entries go with the list's nodes; the object keeps its node (actor_node,
   unit_node). An index gives the first object with an aid in the list's order, as the walk did,
   also where several share it. Here the lists' storage, the actors' lookup and their aid's setter;
   the objects join and leave the lists, and the units are looked up and change their aid, in
   world/aid_lists.c. */

struct GameActor;
struct IdIndex;
struct World;

void world_init_aid_lists(struct World *self);
/* empties the lists, whose objects are gone */
void world_clear_aid_lists(struct World *self);
void world_free_aid_lists(struct World *self);
/* grows an index's table where one more aid would not fit */
void world_index_room(struct IdIndex *index);
/* An actor's aid changes only through this once it may be on the actors, with the world of that
   list (any world, NULL too, for an actor on none). */
void world_set_actor_aid(struct World *self, struct GameActor *actor, int aid);

#endif
