#ifndef GAMEMODE_CAPTCHA_PACKETS_H
#define GAMEMODE_CAPTCHA_PACKETS_H
#include <stddef.h>
struct GameMode;
void game_mode_dispatch_captcha_packet(struct GameMode *game, int id, const void *packet, size_t size);
#endif
