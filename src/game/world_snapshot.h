#ifndef GAMEMODE_WORLD_SNAPSHOT_H
#define GAMEMODE_WORLD_SNAPSHOT_H
#include <stddef.h>
struct GameMode;
struct ReplayBuffer;
struct ReplayRecorder;

int game_ground_items_save(struct GameMode *mode, struct ReplayBuffer *out);
int game_ground_items_load(struct GameMode *mode, const void *data, size_t size);
int game_skill_units_save(struct GameMode *mode, struct ReplayBuffer *out);
int game_skill_units_load(struct GameMode *mode, const void *data, size_t size);
int game_npc_dialog_save(struct GameMode *mode, struct ReplayBuffer *out);
int game_npc_dialog_load(struct GameMode *mode, const void *data, size_t size);
int game_world_snapshot_restore(struct GameMode *mode, struct ReplayRecorder *recorder);
#endif
