#include "game/actor_names.h"
#ifndef GAMEMODE_NAME_BALLOON_H
#define GAMEMODE_NAME_BALLOON_H

#include "game/client.h"
#include "game/gamemode.h"

void game_mode_make_name_balloon(struct GameMode *self, struct UIWindow **slot);
void game_mode_show_name_balloon(struct GameMode *self, struct GameActor *actor);
void game_mode_show_name_balloon2(struct GameMode *self, struct GameActor *actor);
void game_mode_move_balloon_text_window(struct GameMode *self, struct UIWindow *balloon,
                                                struct GameActor *actor);
void game_mode_process_monster_pick(struct GameMode *self, unsigned aid);
unsigned game_mode_name_color(struct GameMode *self, int job, unsigned aid);
#endif
