#include "actor/actor_path.h"
#include "session/state.h"
#include "actor/actor.h"
#include "session/session_state.h"

unsigned char game_actor_find_path(struct GameActor *self, int sx, int sy,
                                             int sub_x, int sub_y, int tx, int ty, int speed)
{
    float scaled = 0.0f;
    int first_speed;
    unsigned last, now;
    unsigned char found;

    if (g_replayPlaying) {
        scaled = replay_recorder_inverse_speed(replay_recorder_instance()) * (float)speed;
        first_speed = (int)scaled;
    } else {
        first_speed = speed;
    }
    found = path_finder_find_path(&g_pathFinder, self->path_start, sx, sy, sub_x, sub_y, tx, ty,
                                  first_speed, &self->path, self->object_type);
    if (!found)
        return found;
    last = path_info_end_time(&self->path);
    now = session_get_server_time(&g_session1);
    if (last > now)
        return found;
    self->path_start = now;
    return path_finder_find_path(&g_pathFinder, now, sx, sy, sub_x, sub_y, tx, ty,
                                 (g_replayPlaying ? (int)scaled : speed) * 10 / 13, &self->path, self->object_type);
}

int actor_path_position_at(struct GameActor *actor, uint32_t time,
                           float *x, float *y, int *direction)
{
    return path_info_position_at(&actor->path, time, x, y, direction);
}
