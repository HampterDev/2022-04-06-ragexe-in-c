#ifndef ACTOR_PATH_H
#define ACTOR_PATH_H
#include <stdint.h>
struct GameActor;
unsigned char game_actor_find_path(struct GameActor *self, int sx, int sy,
                                 int sub_x, int sub_y, int tx, int ty, int speed);

#endif
