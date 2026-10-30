#ifndef GAMEMODE_MODE_CALLBACKS_H
#define GAMEMODE_MODE_CALLBACKS_H

#include "world/mode_types.h"

extern const struct ModeOps g_gameModeVtable_ops;
void game_mode_on_update(struct Mode *mode);
int game_mode_send_msg(struct Mode *mode, int message, int a, int b, int c, int d);
void game_mode_on_change_state(struct Mode *mode, int state);

extern unsigned g_followLastMoveTick;
extern unsigned g_followLostSinceTick;
extern int g_isSkipFrame;

#endif
