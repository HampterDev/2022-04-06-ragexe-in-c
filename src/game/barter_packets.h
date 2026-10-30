#ifndef GAMEMODE_BARTER_PACKETS_H
#define GAMEMODE_BARTER_PACKETS_H

struct GameMode;

void game_mode_zc_npc_barter_market_item_info(struct GameMode *self, const unsigned char *packet);
void game_mode_zc_npc_barter_market_item_info2(struct GameMode *self, const unsigned char *packet);
void game_mode_zc_npc_expanded_barter_market_item_info(struct GameMode *self, const unsigned char *packet);
void game_mode_zc_npc_expanded_barter_market_item_info2(struct GameMode *self, const unsigned char *packet);

void game_mode_poll_network_msg(struct GameMode *self);

#endif
