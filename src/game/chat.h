#ifndef GAMEMODE_CHAT_H
#define GAMEMODE_CHAT_H

#include "game/gamemode.h"

struct UIWindowMgr;
void window_mgr_clear_chat_line(struct UIWindowMgr *mgr);
const char *window_mgr_get_chat_line(struct UIWindowMgr *mgr);
void game_mode_mute_self_for_flooding(void);
char chat_refuse_repeated_line(int repeats);
void game_mode_log_whisper(struct GameMode *self, const char *line);
void game_mode_send_npc_input_text(struct GameMode *self, int npc_id, const char *text,
                                          const char *override);

void game_mode_close_chat_log(void);

#endif
