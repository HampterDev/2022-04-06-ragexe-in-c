#ifndef GAMEMODE_LIFECYCLE_H
#define GAMEMODE_LIFECYCLE_H

struct GameMode;
struct Mode;

struct GameMode *game_mode_create(void);
void game_mode_destruct(struct GameMode *self);
void game_mode_destroy(struct Mode *mode);
void game_mode_on_init(struct Mode *mode, const char *map);
int game_mode_on_run(struct Mode *mode);
void game_mode_on_exit(struct Mode *mode);

#endif
