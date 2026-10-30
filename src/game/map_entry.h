#ifndef GAMEMODE_MAP_ENTRY_H
#define GAMEMODE_MAP_ENTRY_H

struct GameMode;

int game_mode_load_world(struct GameMode *mode);
int game_mode_reset_for_warp(struct GameMode *mode);

#endif
