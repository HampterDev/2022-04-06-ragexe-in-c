#include "session/state.h"
#include "game/gamemode.h"
#include "world/world.h"
#include "actor/player.h"
#include "std/list.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct Session g_session1;
static struct World world;
static struct C3dAttr attr;
static struct GameMode mode;
static int player_storage, old_player;
static struct ListNode head;
static struct {
    struct ListNode links;
    void *value;
} node;
static int conversions, creations, allocations, fail_creation;

void game_mode_cell_to_world(struct GameMode *owner, int x, int y, float *out_x, float *out_z)
{
    assert(owner == &mode && x == 12 && y == 34);
    *out_x = -17.5f;
    *out_z = 32.5f;
    ++conversions;
}

float c3d_attr_get_height(struct C3dAttr *grid, float x, float z)
{
    assert(grid == &attr && x == -17.5f && z == 32.5f);
    return -8.0f;
}

void game_mode_world_to_cell(struct GameMode *owner, float x, float z, int *cell_x, int *cell_y)
{
    assert(owner == &mode && x == -17.5f && z == 32.5f);
    *cell_x = 12;
    *cell_y = 34;
    ++conversions;
}

struct Player *player_create(const float position[3], int cell_x, int cell_y)
{
    assert(position[0] == -17.5f && position[1] == -8.0f && position[2] == 32.5f);
    assert(cell_x == 12 && cell_y == 34);
    assert(world.player == (struct Player *)&old_player && world.gameObjects.size == 0);
    ++creations;
    return fail_creation ? NULL : (struct Player *)&player_storage;
}

void *list_insert(struct List *list, struct ListNode *where, const void *value, size_t size)
{
    assert(list == &world.gameObjects && where == &head && head.prev == &head);
    assert(size == sizeof node.value && *(void *const *)value == &player_storage);
    assert(world.player == (struct Player *)&old_player);
    node.links.next = where;
    node.links.prev = where->prev;
    where->prev->next = &node.links;
    where->prev = &node.links;
    list->size++;
    node.value = *(void *const *)value;
    ++allocations;
    return &node.value;
}

int main(void)
{
    g_session1.playerPosX = 12;
    g_session1.playerPosY = 34;
    world.player = (struct Player *)&old_player;
    world.gameObjects.head = &head;
    head.next = head.prev = &head;
    assert(!world_create_player(NULL));
    assert(!world_create_player(&world));
    world.mode = &mode;
    assert(!world_create_player(&world));
    world.mode = NULL;
    world.attr = &attr;
    assert(!world_create_player(&world));
    assert(conversions == 0 && creations == 0 && allocations == 0);
    assert(world.player == (struct Player *)&old_player && world.gameObjects.size == 0);

    world.mode = &mode;
    fail_creation = 1;
    assert(!world_create_player(&world));
    assert(conversions == 2 && creations == 1 && allocations == 0);
    assert(world.player == (struct Player *)&old_player && world.gameObjects.size == 0);
    assert(head.next == &head && head.prev == &head);

    fail_creation = 0;
    assert(world_create_player(&world));
    assert(conversions == 4 && creations == 2 && allocations == 1);
    assert(world.player == (struct Player *)&player_storage && world.gameObjects.size == 1);
    assert(head.next == &node.links && head.prev == &node.links);
    assert(node.links.next == &head && node.links.prev == &head);
    assert(*LIST_VALUE(&node.links, void *) == &player_storage);
    puts("world player: owning map, absent context, allocation failure and list ownership passed");
    return 0;
}
