#include "session/state.h"
#include "world/world.h"
#include "actor/player.h"
#include "std/list.h"

int world_create_player(struct World *self)
{
    float position[3];
    int cell_x, cell_y;
    void *player;

    if (!self || !self->mode || !self->attr)
        return 0;

    game_mode_cell_to_world(self->mode, g_session1.playerPosX, g_session1.playerPosY, &position[0], &position[2]);
    position[1] = c3d_attr_get_height(self->attr, position[0], position[2]);
    game_mode_world_to_cell(self->mode, position[0], position[2], &cell_x, &cell_y);
    player = player_create(position, cell_x, cell_y);
    if (!player)
        return 0;

    list_push_back(&self->gameObjects, &player, sizeof player);
    self->player = player;
    return 1;
}
