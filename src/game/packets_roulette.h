#ifndef GAMEMODE_PACKETS_ROULETTE_H
#define GAMEMODE_PACKETS_ROULETTE_H

#include "game/packet_handlers.h"
#include "game/objects.h"
#include "game/roulette_state.h"

#define ROULETTE_PRIZES 42

struct RoulettePrize;
void roulette_add_prize(struct Roulette *r, struct RoulettePrize prize);
void roulette_place_marker(struct Roulette *r, int a, int b, int x, int y);
void roulette_begin_spin(struct Roulette *r, int a, int b, int x, int y);
void ui_window_mgr_allow_roulette_spin(struct UIWindowMgr *mgr);

struct PACKET_ZC_ACK_OPEN_ROULETTE;
struct PACKET_ZC_ACK_ROULEITTE_INFO;
struct PACKET_ZC_ACK_CLOSE_ROULETTE;
struct PACKET_ZC_ACK_GENERATE_ROULETTE;

#endif
