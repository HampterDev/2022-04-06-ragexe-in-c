#ifndef GAMEMODE_PACKET_DISPATCH_H
#define GAMEMODE_PACKET_DISPATCH_H

#include <stddef.h>
struct GameMode;

void game_mode_dispatch_packet(struct GameMode *self, int packet_id, unsigned char *packet,
                             size_t packet_size);
void game_mode_dispatch_packet2(struct GameMode *self, int packet_id, unsigned char *packet,
                              size_t packet_size);

#endif
