#ifndef GAMEMODE_CELLS_H
#define GAMEMODE_CELLS_H
#include "core/compiler.h"

struct GameMode;

void game_mode_cell_to_world(struct GameMode *self, int x, int y, float *world_x, float *world_z);
void game_mode_sub_cell_to_world(struct GameMode *self, int x, int y, int sub_x, int sub_y,
                             float *world_x, float *world_z);
void game_mode_world_to_cell(struct GameMode *self, float world_x, float world_z, int *x, int *y);
void game_mode_world_to_sub_cell(struct GameMode *self, float world_x, float world_z, int *x, int *y,
                             int *sub_x, int *sub_y);
void game_mode_get_player_cell(struct GameMode *self, int *x, int *y);
int game_mode_get_map_width(struct GameMode *self);
int game_mode_get_map_height(struct GameMode *self);
int game_mode_angle_to_direction(float degrees);

RAGE_NOINLINE char game_mode_is_cell_blocked(struct GameMode *self, int x, int y);
RAGE_NOINLINE char game_mode_is_cell_occupied(struct GameMode *self, int x, int y);
char game_mode_can_find_path(struct GameMode *self, int from_x, int from_y, int from_sub_x,
                          int from_sub_y, int to_x, int to_y);
char game_mode_is_valid_cell(struct GameMode *self, int x, int y);
char game_mode_get_closest_valid_cell(struct GameMode *self, int *x, int *y);

#endif
