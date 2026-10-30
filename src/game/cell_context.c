#include "game/gamemode.h"
#include "game/context.h"
#include "game/cells.h"

int game_world_to_cell(float x, float z, int *cell_x, int *cell_y)
{
    struct GameMode *mode = game_current_mode();
    if (!mode || !mode->world || !mode->world->attr)
        return 0;
    game_mode_world_to_cell(mode, x, z, cell_x, cell_y);
    return 1;
}

int game_cell_to_world(int x, int y, float *world_x, float *world_z)
{
    struct GameMode *mode = game_current_mode();
    if (!mode || !mode->world || !mode->world->attr)
        return 0;
    game_mode_cell_to_world(mode, x, y, world_x, world_z);
    return 1;
}

int game_can_walk_to_cell(int x, int y)
{
    struct GameMode *mode = game_current_mode();
    return mode && mode->world && mode->world->attr && mode->world->player
        && game_mode_is_valid_cell(mode, x, y);
}

int game_closest_walkable_cell(int *x, int *y)
{
    struct GameMode *mode = game_current_mode();
    return mode && mode->world && mode->world->attr && mode->world->player
        && game_mode_get_closest_valid_cell(mode, x, y);
}
