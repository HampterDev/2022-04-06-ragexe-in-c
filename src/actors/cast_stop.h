#ifndef ACTOR_CAST_STOP_H
#define ACTOR_CAST_STOP_H

struct GameActor;

/* Ends the cast an actor shows without its skill: the cast effect, the target's effect and the
   progress bar go, then for another actor (`own` 0) the warning plane of a ground cast, and an
   actor in the casting state leaves it. It is what ZC_DISPEL does, and with tracks what an
   idle record does to an actor that casts (plans/core-loop.md). */
void game_actor_stop_cast(struct GameActor *actor, int own);

#endif
