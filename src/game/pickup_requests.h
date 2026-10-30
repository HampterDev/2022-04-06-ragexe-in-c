#ifndef GAMEMODE_PICKUP_REQUESTS_H
#define GAMEMODE_PICKUP_REQUESTS_H

struct GameMode;

void game_mode_add_pickup_request(struct GameMode *self, unsigned item_aid);

char game_mode_is_pickup_requested(struct GameMode *self, unsigned item_aid);

void game_mode_remove_pickup_request(struct GameMode *self, unsigned item_aid);

#endif
