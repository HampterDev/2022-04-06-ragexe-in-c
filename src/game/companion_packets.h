#ifndef GAMEMODE_COMPANION_PACKETS_H
#define GAMEMODE_COMPANION_PACKETS_H

struct GameMode;

void game_mode_zc_property_homun(struct GameMode *mode, const unsigned char *packet);
void game_mode_zc_property_homun_no_accessory(struct GameMode *mode, const unsigned char *packet);
void game_mode_zc_property_homun_no_accessory2(struct GameMode *mode, const unsigned char *packet);
void game_mode_zc_property_homun_no_accessory3(struct GameMode *mode, const unsigned char *packet);

#endif
