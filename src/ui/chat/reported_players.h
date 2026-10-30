#ifndef UI_CHAT_REPORTED_PLAYERS_H
#define UI_CHAT_REPORTED_PLAYERS_H

#include "std/string.h"
#include "std/vec.h"
#include <stdint.h>

struct ReportedPlayer {
    struct String name;
    int aid;
    uint16_t job;
};

struct ReportedPlayer *reported_player_init(struct ReportedPlayer *player,
    const struct String *name, int aid, uint16_t job);

void reported_players_clear(struct Vec *players);
void reported_players_free(struct Vec *players);

#endif
