#ifndef GAMEMODE_SHORTCUT_KEYS_H
#define GAMEMODE_SHORTCUT_KEYS_H

#include "game/packet_handlers.h"
#include "game/gamemode.h"

struct PACKET_ZC_SHORTCUT_KEY_LIST_V4;
void session_set_shortcut_keys(int rotate, const unsigned char *keys, unsigned short tab);

#endif
