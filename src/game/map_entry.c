#include "game/gamemode.h"
#include "actor/weather.h"
#include "game/map_entry.h"
#include "game/camera.h"
#include "game/mode_msg.h"
#include "world/load.h"
#include "world/player.h"
#include "game/world_objects.h"
#include "world/world.h"

static int map_entry_failed(struct GameMode *mode)
{
    mode->base.loop_cond = 0;
    game_mode_to_login();
    return 0;
}

int game_mode_load_world(struct GameMode *mode)
{
    if (!world_load(mode->world))
        return map_entry_failed(mode);
    mode->ray_tests_cells = 0;
    game_mode_init_view(mode);
    game_mode_make_mouse_pointer(mode);
    return 1;
}

int game_mode_reset_for_warp(struct GameMode *mode)
{
    struct Player *player;

    world_clear_objects(mode->world);
    if (!world_create_player(mode->world))
        return map_entry_failed(mode);
    world_load_effects_and_sounds(mode->world);
    weather_forget_effect(&g_weather);
    game_mode_make_mouse_pointer(mode);
    player = mode->world->player;
    mode->view->at[0] = player->pos.x;
    mode->view->at[1] = player->pos.y;
    mode->view->at[2] = player->pos.z;
    return 1;
}
