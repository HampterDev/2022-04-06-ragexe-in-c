#include "world/pick_node.h"
#ifndef GAMEMODE_INPUT_H
#define GAMEMODE_INPUT_H

#include "game/gamemode.h"

enum { BUTTON_PRESSED = 1, BUTTON_HELD = 2, BUTTON_RELEASED = 3 };

char game_mode_pick_ground_cell(struct GameMode *self, int *cell_x, int *cell_y);
void game_mode_process_input(struct GameMode *self);

extern int g_shiftDown;

#endif
