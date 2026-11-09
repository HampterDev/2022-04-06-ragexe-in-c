#include "game/gamemode.h"
#include "actor/weather.h"
#include "game/map_entry.h"
#include "game/camera.h"
#include "world/load.h"
#include "world/player.h"
#include <assert.h>
#include <stdio.h>

struct Weather g_weather;
static struct GameMode mode;
static struct World world;
static struct View view;
static struct Player player;
static int load_ok, create_ok, loads, clears, creates, views, pointers, effects, weather, logins;

int world_load(struct World *value)
{
    assert(value == &world);
    ++loads;
    world.player = load_ok ? &player : NULL;
    return load_ok;
}

void world_clear_objects(struct World *value)
{
    assert(value == &world);
    ++clears;
    world.player = NULL;
}

int world_create_player(struct World *value)
{
    assert(value == &world && !world.player);
    ++creates;
    world.player = create_ok ? &player : NULL;
    return create_ok;
}

void world_load_effects_and_sounds(struct World *value)
{
    assert(value == &world && world.player == &player);
    ++effects;
}

void weather_forget_effect(struct Weather *value)
{
    assert(value == &g_weather && world.player == &player);
    ++weather;
}

void game_mode_init_view(struct GameMode *value)
{
    assert(value == &mode && world.player == &player);
    ++views;
}

void game_mode_make_mouse_pointer(struct GameMode *value)
{
    assert(value == &mode && world.player == &player);
    ++pointers;
}

int game_mode_to_login(void)
{
    assert(mode.base.loop_cond == 0 && !world.player);
    ++logins;
    return 0;
}

int main(void)
{
    mode.world = &world;
    mode.view = &view;
    mode.base.loop_cond = 1;
    player.pos.x = 12;
    player.pos.y = -24;
    player.pos.z = 36;

    assert(!game_mode_load_world(&mode));
    assert(loads == 1 && logins == 1 && !mode.base.loop_cond);
    assert(views == 0 && pointers == 0);

    mode.base.loop_cond = 1;
    load_ok = 1;
    assert(game_mode_load_world(&mode));
    assert(loads == 2 && logins == 1 && mode.base.loop_cond);
    assert(views == 1 && pointers == 1);

    view.at[0] = 1;
    view.at[1] = 2;
    view.at[2] = 3;
    assert(!game_mode_reset_for_warp(&mode));
    assert(clears == 1 && creates == 1 && logins == 2 && !mode.base.loop_cond);
    assert(!world.player && effects == 0 && weather == 0 && pointers == 1);
    assert(view.at[0] == 1 && view.at[1] == 2 && view.at[2] == 3);

    mode.base.loop_cond = 1;
    create_ok = 1;
    assert(game_mode_reset_for_warp(&mode));
    assert(clears == 2 && creates == 2 && logins == 2 && mode.base.loop_cond);
    assert(world.player == &player && effects == 1 && weather == 1 && pointers == 2);
    assert(view.at[0] == 12 && view.at[1] == -24 && view.at[2] == 36);
    puts("map entry: failed load/warp stop initialization and request login; successful camera setup passed");
    return 0;
}
