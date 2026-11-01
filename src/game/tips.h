#ifndef GAMEMODE_TIPS_H
#define GAMEMODE_TIPS_H

struct GameMode;
struct String;
struct Vec;

struct RenewQuestB;

struct String *state_icon_get_time_left_text(struct GameMode *self, struct String *out, int icon,
                                            int *kind);
void game_mode_update_state_icon_tip(struct GameMode *self, int x, int y);
struct String *quest_hunt_get_line(struct String *out, const struct RenewQuestB *quest, int i);
void game_mode_show_quest_hunt_list(struct GameMode *self, const struct Vec *lines);

#endif
