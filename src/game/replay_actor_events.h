#ifndef GAMEMODE_REPLAY_ACTOR_EVENTS_H
#define GAMEMODE_REPLAY_ACTOR_EVENTS_H
#include <stdint.h>
#include <stddef.h>
#include "base.h"
#include "replay/event.h"
struct GameMode;
#pragma pack(push, 1)
struct ReplayWhisper { struct ReplayEventHeader header; char receiver[25], text[223]; };
struct ReplayModeMessage { struct ReplayEventHeader header; int32_t message, a, b, c; };
struct ReplayOwnRoom { struct ReplayEventHeader header; int32_t title_size, password_size, type, users, max_users, room_id; };
#pragma pack(pop)
SIZE_ASSERT(ReplayWhisper, 254);
SIZE_ASSERT(ReplayModeMessage, 22);
SIZE_ASSERT(ReplayOwnRoom, 30);

void game_mode_record_whisper(struct GameMode *mode, const char *receiver, const char *text);
char game_mode_record_mode_msg(struct GameMode *mode, int message, int a, int b, int c);
void game_mode_record_own_chat_room(struct GameMode *mode);
int game_replay_whisper(struct GameMode *mode, const void *data, size_t size);
int game_replay_chat_room(struct GameMode *mode, const void *data, size_t size);
#endif
