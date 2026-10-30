#ifndef WORLD_MODE_ACTIONS_H
#define WORLD_MODE_ACTIONS_H

struct Mode;

void mode_run_fade_out(struct Mode *self, int logo);
void mode_set_cursor_action(struct Mode *self, int action);
void mode_draw_mouse_cursor(struct Mode *self);
void mode_process_fade_in(struct Mode *self);
void mode_screen_shot(struct Mode *self);
void mode_nothing(struct Mode *self);

#endif
