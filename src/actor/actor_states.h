#ifndef ACTOR_ACTOR_STATES_H
#define ACTOR_ACTOR_STATES_H

/* The values of an actor's `state` (game_actor_set_state) that the code names. */
enum {
    ACTOR_STATE_STAND = 0,
    ACTOR_STATE_WALKING = 1,
    ACTOR_STATE_ATTACK = 2,
    ACTOR_STATE_DEAD = 3,
    ACTOR_STATE_HURT = 4,
    ACTOR_STATE_CASTING = 8
};

#endif
