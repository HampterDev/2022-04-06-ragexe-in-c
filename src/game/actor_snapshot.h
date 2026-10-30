#ifndef GAMEMODE_ACTOR_SNAPSHOT_H
#define GAMEMODE_ACTOR_SNAPSHOT_H
#include <stddef.h>
struct GameMode;
struct ReplayBuffer;
struct ReplayRecorder;
int game_actors_save(struct GameMode *mode, struct ReplayBuffer *out);
int game_actors_load(struct GameMode *mode, const void *data, size_t size);
int game_actors_restore(struct GameMode *mode, struct ReplayRecorder *recorder);
#endif
