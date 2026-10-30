#ifndef GAMEMODE_REPUTE_H
#define GAMEMODE_REPUTE_H

#include <stdint.h>
struct GameMode;

#pragma pack(push, 1)
struct ReputeEntry {
    uint64_t id;
    int64_t points;
};
#pragma pack(pop)

void game_mode_on_repute_info(struct GameMode *self, unsigned char success,
                           unsigned count, const struct ReputeEntry *entries);
void game_mode_on_ui_open_repute(struct GameMode *self, uint64_t group_id,
                            uint64_t info_id);

#endif
