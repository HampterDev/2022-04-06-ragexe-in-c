#ifndef GAMEMODE_WHISPER_H
#define GAMEMODE_WHISPER_H

#include "game/packet_handlers.h"
#include "game/gamemode.h"
#include "core/text_view.h"

enum {
    WHISPER_ACK_SENT = 0,
    WHISPER_ACK_NOT_THERE = 1,
    WHISPER_ACK_IGNORED = 2,
    WHISPER_ACK_IGNORING_ALL = 3,
};

void game_mode_whisper(struct GameMode *self, int sender_cid, struct TextView sender,
                                 struct TextView text, unsigned char from_admin);

#endif
