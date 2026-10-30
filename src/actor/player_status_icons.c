#include "actor/status_icons.h"
#include "actor/actor.h"
#include "game/context.h"

int game_hide_player_status_icons(void)
{
    struct Player *player = game_current_player();
    if (player)
        render_object_send(player, NULL, 0x6d, 0xa4, 0, 0, 0);

    player = game_current_player();
    if (!player)
        return 0;
    player->status_icons = NULL;
    return 1;
}

void game_show_player_status_icons(void)
{
    struct Player *player = game_current_player();
    if (player) {
        Vec3 origin = {0, 0, 0};
        player->status_icons = game_actor_launch_effect((struct GameActor *)player, 0xa4, origin, 0);
    }
}
