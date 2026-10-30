#ifndef GAMEMODE_TALK_TYPE_H
#define GAMEMODE_TALK_TYPE_H

#include "game/gamemode.h"
#include "session/talk_options.h"

void game_mode_process_talk_type(struct GameMode *self, int type, struct String *args);

enum {
    TALK_CHAT = 0x0,
    TALK_UNKNOWN = 0x3,
    TALK_WHISPER_BLOCK = 0x12,
    TALK_WHISPER_UNBLOCK = 0x13,
    TALK_CHAT_TO_GUILD = 0x32,
    TALK_SHOW_NAME = 0xe3,
};

void talk_option_toggle(int type);

void talk_cmd_send_chat(struct GameMode *self, int type);
void talk_cmd_whisper_block(struct GameMode *self, const struct String *name, int unblock);
void talk_cmd_whisper_block_all(struct GameMode *self, int unblock);
void game_mode_reblock_whispers(struct GameMode *self);
void talk_cmd_hi_friends(struct GameMode *self, const struct String *args);
void talk_cmd_chat_auto_save(struct GameMode *self);
void talk_cmd_where(struct GameMode *self);
void talk_cmd_snow(struct GameMode *self);
void talk_cmd_sit_or_stand(struct GameMode *self);
void talk_cmd_turn_by45(struct GameMode *self, int counter_clockwise);
void talk_cmd_help(struct GameMode *self);
void talk_cmd_trace_ai(struct GameMode *self);
void talk_cmd_volume(struct GameMode *self, const struct String *args, int effects);
void talk_cmd_fog(struct GameMode *self);
void talk_cmd_show_effects(struct GameMode *self, int type);
void talk_cmd_navigate(struct GameMode *self, const struct String *args, int with_flags);
void talk_cmd_build_info(struct GameMode *self);
void talk_cmd_send_camera(struct GameMode *self, const struct String *args, int reset);
void talk_cmd_on_off(struct GameMode *self, const struct String *args);
void talk_cmd_sound(struct GameMode *self, int type);
void talk_cmd_zoom_range(struct GameMode *self);

#define TALK_COLOR_NOTICE 0x64ffff
#define TALK_COLOR_YELLOW 0xffff
#define TALK_COLOR_HELP 0xffff64

void talk_print(const char *text, unsigned color);
void talk_print_msg(int msg_id, unsigned color);
void talk_refresh_option_window(void);

extern int g_isFixedCamera;

#endif
