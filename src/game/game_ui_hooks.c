#include "game/game_ui_hooks.h"
#include "game/context.h"
#include "game/packet_handlers.h"
#include "game/actor_actions.h"

void game_open_quest_link(int id)
{
    game_mode_ui_open_v2(game_current_mode(), 6, id);
}

void game_click_current_actor(unsigned aid)
{
    struct GameMode *mode = game_current_mode();

    if (mode)
        game_mode_click_actor(mode, aid);
}
