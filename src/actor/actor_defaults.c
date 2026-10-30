#include "actor/actor_ops.h"

void actor_default_bool(struct GameActor *actor, unsigned char value0)
{
    (void)actor;
    (void)value0;
}

unsigned char actor_default_false(struct GameActor *actor)
{
    (void)actor;
    return 0;
}

void actor_default_int(struct GameActor *actor, int value0)
{
    (void)actor;
    (void)value0;
}

void actor_default_int_bool(struct GameActor *actor, int value0, unsigned char value1)
{
    (void)actor;
    (void)value0;
    (void)value1;
}

void actor_default_int_flags(struct GameActor *actor, int value0, unsigned int value1)
{
    (void)actor;
    (void)value0;
    (void)value1;
}

void actor_default_look(struct GameActor *actor, const struct ActorLook *value0)
{
    (void)actor;
    (void)value0;
}

void actor_default_look_state(struct GameActor *actor, const struct ActorLook *value0, int value1)
{
    (void)actor;
    (void)value0;
    (void)value1;
}

int64_t actor_default_no_weapon(struct GameActor *actor)
{
    (void)actor;
    return -1;
}

void actor_default_noop(struct GameActor *actor)
{
    (void)actor;
}

void actor_default_pair(struct GameActor *actor, int value0, int value1)
{
    (void)actor;
    (void)value0;
    (void)value1;
}

void actor_default_weapon(struct GameActor *actor, int64_t value0)
{
    (void)actor;
    (void)value0;
}

int actor_default_zero(struct GameActor *actor)
{
    (void)actor;
    return 0;
}
