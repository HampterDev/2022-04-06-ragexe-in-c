#ifndef GAMEMODE_SCREEN_COUNTERS_H
#define GAMEMODE_SCREEN_COUNTERS_H

#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "game/client.h"

#include "render/sprite_draw.h"

enum {
    TIMER_FOR_5S = 0,
    TIMER_SINCE = 1,
    TIMER_UNTIL = 2,
    TIMER_COUNTDOWN = 3,
};

void game_mode_draw_pk_ranking(struct GameMode *self);
void game_mode_draw_ranking_time_attack(struct GameMode *self);
void game_mode_draw_time_attack(struct GameMode *self, DWORD value, int type);
void game_mode_draw_skill_level_at_cursor(struct GameMode *self);
struct PACKET_ZC_SHOWDIGIT;

#endif
