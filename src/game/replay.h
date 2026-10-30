#include "replay/replay.h"
#ifndef GAMEMODE_REPLAY_H
#define GAMEMODE_REPLAY_H

#include "game/gamemode.h"

void game_mode_apply_replay_speed(struct GameMode *self);
char game_mode_on_replay_pause(struct GameMode *self, char pausing);
void game_mode_end_replay(struct GameMode *self);

#endif
