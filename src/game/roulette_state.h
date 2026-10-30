#include "ui/window_manager_state.h"
#ifndef GAMEMODE_ROULETTE_STATE_H
#define GAMEMODE_ROULETTE_STATE_H

#include "base.h"
#include "ui/misc/roulette_state.h"

struct UIWindow;

void roulette_set_addition_item(struct Roulette *r, int item_id);
void roulette_set_slot(struct Roulette *r, unsigned short slot, unsigned char has_slot,
                                 unsigned char flag);

#endif
