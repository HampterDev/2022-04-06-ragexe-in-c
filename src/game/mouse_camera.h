#ifndef GAMEMODE_MOUSE_CAMERA_H
#define GAMEMODE_MOUSE_CAMERA_H

#include "game/gamemode.h"

void game_mode_process_right_button(struct GameMode *self);
void game_mode_process_wheel(struct GameMode *self, int verdict);

#endif
