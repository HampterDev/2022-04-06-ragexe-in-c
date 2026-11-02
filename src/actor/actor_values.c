#include "actor/render_object.h"
#include "std/integer_map.h"
#include "std/tree.h"

unsigned char game_actor_update_sphere_count(struct GameActor *self, int64_t count)
{
    unsigned char above = count > self->sphere_count;
    self->sphere_count = count;
    return above;
}
