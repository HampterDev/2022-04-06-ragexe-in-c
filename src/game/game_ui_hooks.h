#ifndef GAME_GAME_UI_HOOKS_H
#define GAME_GAME_UI_HOOKS_H

/* The game mode's side of g_uiHooks (ui/controls/ui_hooks.h): opens the quest window at
   quest `id`, as game_mode_ui_open_v2 type 6 does for the server. */
void game_open_quest_link(int id);
/* The game mode's side of g_uiHooks.click_actor: game_mode_click_actor on the running game
   mode, if there is one. */
void game_click_current_actor(unsigned aid);

#endif
