#ifndef GAMEMODE_DRAG_H
#define GAMEMODE_DRAG_H

#include "game/barter_drag.h"

struct GameMode;

struct ItemInfo;

void game_mode_reset_drag(struct GameMode *self);
void game_mode_set_drag_source(struct GameMode *self, int source, int index);
void game_mode_drag_item(struct GameMode *self, int source, const struct ItemInfo *item, int arg3,
                       int count);
void game_mode_drag_by_name(struct GameMode *self, int source, int index, const char *name, int level,
                         int count);
char game_mode_refuse_held_item_drag(struct GameMode *self);

const char *game_mode_count_item_id_text(struct GameMode *self);
void game_mode_draw_dragged(struct GameMode *self);

#endif
