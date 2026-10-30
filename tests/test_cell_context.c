#include "game/gamemode.h"
#include "game/context.h"
#include "game/cells.h"
#include <assert.h>
#include <stdio.h>

static struct GameMode *current;
static int conversions, reverse_conversions, checks, searches, valid, found;

struct GameMode *game_current_mode(void) { return current; }

void game_mode_world_to_cell(struct GameMode *mode, float x, float z, int *cx, int *cy)
{
    assert(mode == current && mode->world && mode->world->attr);
    assert(x == 2.5f && z == 6.5f);
    ++conversions;
    *cx = 2;
    *cy = 6;
}

void game_mode_cell_to_world(struct GameMode *mode, int x, int y, float *wx, float *wz)
{
    assert(mode == current && mode->world && mode->world->attr);
    assert(x == 2 && y == 6);
    ++reverse_conversions;
    *wx = 2.5f;
    *wz = 6.5f;
}

char game_mode_is_valid_cell(struct GameMode *mode, int x, int y)
{
    assert(mode == current && mode->world->attr && mode->world->player);
    assert(x == 2 && y == 6);
    ++checks;
    return (char)valid;
}

char game_mode_get_closest_valid_cell(struct GameMode *mode, int *x, int *y)
{
    assert(mode == current && mode->world->attr && mode->world->player);
    ++searches;
    if (found) { *x = 3; *y = 7; }
    return (char)found;
}

static void check_absent(void)
{
    int x = 90, y = 91;
    float wx = 80.0f, wz = 81.0f;
    assert(!game_cell_to_world(2, 6, &wx, &wz));
    assert(wx == 80.0f && wz == 81.0f);
    assert(!game_world_to_cell(2.5f, 6.5f, &x, &y));
    assert(!game_can_walk_to_cell(2, 6));
    assert(!game_closest_walkable_cell(&x, &y));
    assert(x == 90 && y == 91);
}

int main(void)
{
    static struct GameMode mode;
    static struct World world;
    static struct C3dAttr attr;
    static struct Player player;
    int x = 90, y = 91;
    float wx = 80.0f, wz = 81.0f;

    check_absent();
    current = &mode;
    check_absent();
    mode.world = &world;
    check_absent();
    assert(conversions == 0 && reverse_conversions == 0 && checks == 0 && searches == 0);
    world.attr = &attr;
    assert(game_world_to_cell(2.5f, 6.5f, &x, &y) && x == 2 && y == 6);
    assert(game_cell_to_world(x, y, &wx, &wz) && wx == 2.5f && wz == 6.5f);
    assert(!game_can_walk_to_cell(x, y));
    assert(!game_closest_walkable_cell(&x, &y));
    assert(checks == 0 && searches == 0);
    world.player = &player;
    assert(!game_can_walk_to_cell(x, y));
    assert(!game_closest_walkable_cell(&x, &y) && x == 2 && y == 6);
    valid = found = 1;
    assert(game_can_walk_to_cell(x, y));
    assert(game_closest_walkable_cell(&x, &y) && x == 3 && y == 7);
    world.attr = NULL;
    check_absent();
    assert(conversions == 1 && reverse_conversions == 1 && checks == 2 && searches == 2);
    current = NULL;
    check_absent();
    puts("cell context: loading, absent player, failed searches and teardown passed");
    return 0;
}
