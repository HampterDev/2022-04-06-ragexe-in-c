#include "actor/actor_path.h"
#include "actor/actor.h"

/* A walk that arrives late is not searched again: its record is joined by the drawn point as
   any other place is (plans/drawn-place.md). */
unsigned char game_actor_find_path(struct GameActor *self, int sx, int sy,
                                             int sub_x, int sub_y, int tx, int ty, int speed)
{
    return path_finder_find_path(&g_pathFinder, self->path_start, sx, sy, sub_x, sub_y, tx, ty,
                                 speed, &self->path, self->object_type);
}
