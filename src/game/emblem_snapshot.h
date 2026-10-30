#ifndef GAMEMODE_EMBLEM_SNAPSHOT_H
#define GAMEMODE_EMBLEM_SNAPSHOT_H
#include <stddef.h>
struct GameMode;
struct ReplayBuffer;
struct ReplayRecorder;
int game_emblems_save(struct GameMode *mode, struct ReplayBuffer *out);
int game_emblems_load(struct GameMode *mode, const void *data, size_t size);
int game_emblems_restore(struct GameMode *mode, struct ReplayRecorder *recorder);
#endif
