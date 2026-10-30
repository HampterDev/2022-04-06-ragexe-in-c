#ifndef ACTOR_PLAYER_H
#define ACTOR_PLAYER_H

struct Player;

struct Player *player_create(const float position[3], int cell_x, int cell_y);

#endif
