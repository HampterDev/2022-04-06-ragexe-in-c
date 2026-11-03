#include "game/game_ui_hooks.h"
#include "game/context.h"
#include "game/packet_handlers.h"

void game_open_quest_link(int id)
{
    game_mode_ui_open_v2(game_current_mode(), 6, id);
}
