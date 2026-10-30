#ifndef ACTOR_PATH_H
#define ACTOR_PATH_H
#include <stdint.h>
struct GameActor;
unsigned char game_actor_find_path(struct GameActor *self, int sx, int sy,
                                 int sub_x, int sub_y, int tx, int ty, int speed);
int actor_path_position_at(struct GameActor *actor, uint32_t time,
                           float *x, float *y, int *direction);
#endif
