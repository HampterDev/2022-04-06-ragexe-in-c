#ifndef GAMEMODE_MODE_UTIL_H
#define GAMEMODE_MODE_UTIL_H

struct GameMode;

void renderer_run_every15s(void);
void follow_set_times(unsigned moved_at, unsigned lost_at);
void game_mode_remove_listed(struct GameMode *self, void *obj);
void game_mode_notify_listed(struct GameMode *self, int arg);
char item_id_is994_to997(int id);
char item_id_is990_to993(int id);
void game_mode_confirm_feed_homun(void);
void game_mode_confirm_feed_pet(void);
void game_mode_draw_shown_timer(struct GameMode *self);
void game_mode_on_own_aid(struct GameMode *self, int aid);

#endif
