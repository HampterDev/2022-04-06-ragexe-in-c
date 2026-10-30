#ifndef GAMEMODE_BARTER_DRAG_H
#define GAMEMODE_BARTER_DRAG_H

struct GameMode;
struct BarterGoods;
struct BarterGoodsEx;

void game_mode_drag_barter_item(struct GameMode *self, int source,
    const struct BarterGoods *item, int drop_count, int count);
void game_mode_drag_expanded_barter_item(struct GameMode *self, int source,
    const struct BarterGoodsEx *item, int drop_count, int count);

#endif
