#ifndef ACTOR_HALLUCINATION_H
#define ACTOR_HALLUCINATION_H

struct GameActor;

void actor_spawn_hallu_copy(const struct GameActor *source, const float position[3], int alpha);

#endif
