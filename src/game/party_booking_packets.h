#ifndef GAMEMODE_PARTY_BOOKING_PACKETS_H
#define GAMEMODE_PARTY_BOOKING_PACKETS_H

#include <stddef.h>

struct GameMode;
void game_mode_zc_party_booking_ack_register(const unsigned char *packet);
void game_mode_zc_party_booking_ack_search(const unsigned char *packet, size_t size);
void game_mode_zc_party_booking_ack_delete(const unsigned char *packet);
void game_mode_zc_party_booking_notify_insert(const unsigned char *packet);
void game_mode_zc_party_booking_notify_update(const unsigned char *packet);
void game_mode_zc_party_booking_notify_delete(const unsigned char *packet);

#endif
