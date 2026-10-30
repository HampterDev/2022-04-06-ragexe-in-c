#include "actor/actor_ops.h"

struct ActorHeadExtent game_actor_get_head_extent(struct GameActor *self)
{
    struct ActorHeadExtent extent = {0.0f, 0.0f};
    (void)self;
    return extent;
}

struct ActorHeadExtent pc_get_head_extent(struct GameActor *base_actor)
{
    struct Pc *self = (struct Pc *)base_actor;
    struct ActorHeadExtent extent = {
        self->head_clip[2] - self->head_clip[0],
        self->head_clip[3] - self->head_clip[1]
    };
    return extent;
}
