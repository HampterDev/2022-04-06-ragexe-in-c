#ifndef GAMEMODE_DISPLAY_SETTINGS_H
#define GAMEMODE_DISPLAY_SETTINGS_H

struct GameMode;

void game_mode_apply_fog(struct GameMode *self, int on);

void game_mode_refresh_actor_effects(struct GameMode *self, char player_only);

#endif
