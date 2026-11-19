#include "core/compiler.h"
#include "game/gamemode.h"
#include "actor/actor_ops.h"
#include "world/pathfinder_state.h"
#include "world/actor_lookup.h"
#include <stdlib.h>
#include "platform/win_types.h"
#include "game/cells.h"
#include "session/session_skills.h"
#include "platform/clock.h"

static const struct { int dx, dy; } kNeighbours[9] = {
    { 0, 0 },
    { 1, 0 }, { 1, 1 }, { 1, -1 },
    { -1, 0 }, { -1, 1 }, { -1, -1 },
    { 0, 1 }, { 0, -1 },
};

static int cell_is_walkable(const struct C3dAttr *attr, int x, int y)
{
    const struct AttrCell *cell;

    if (x < 0 || x >= attr->width || y < 0 || y >= attr->height)
        return 0;
    cell = c3d_attr_get_cell(attr, x, y);
    if (cell == 0)
        return 0;
    return cell->type != ATTR_CELL_BLOCKED && cell->type != ATTR_CELL_CLIFF;
}

void game_mode_get_player_cell(struct GameMode *self, int *x, int *y)
{
    const struct Player *player = self->world->player;

    game_mode_world_to_cell(self, player->pos.x, player->pos.z, x, y);
}

int game_mode_get_map_width(struct GameMode *self)
{
    if (self->world == 0 || self->world->attr == 0)
        return 0;
    return self->world->attr->width;
}

int game_mode_get_map_height(struct GameMode *self)
{
    if (self->world == 0 || self->world->attr == 0)
        return 0;
    return self->world->attr->height;
}

int game_mode_angle_to_direction(float degrees)
{
    return ((int)degrees + 180) / 45 % 8;
}

RAGE_NOINLINE char game_mode_is_cell_blocked(struct GameMode *self, int x, int y)
{
    return !cell_is_walkable(self->world->attr, x, y);
}

RAGE_NOINLINE char game_mode_is_cell_occupied(struct GameMode *self, int x, int y)
{
    struct TreeNode *node;
    const struct UnitCell *cell;
    struct GameActor *actor;

    for (node = tree_first(&self->unit_cells); node != self->unit_cells.head; node = tree_next(node)) {
        cell = TREE_VALUE(node, const struct UnitCell);
        if (cell->x != x || cell->y != y)
            continue;
        actor = world_get_game_actor_by_aid(self->world, (unsigned)cell->aid);
        if (actor == 0)
            return 1;
        if (!is_effect_state_hiding(actor_get_effect_state(actor)))
            return 1;
    }
    return 0;
}

char game_mode_can_find_path(struct GameMode *self, int from_x, int from_y, int from_sub_x,
                                     int from_sub_y, int to_x, int to_y)
{
    struct PathInfo path;
    char found;

    (void)self;
    path_info_init(&path);
    found = path_finder_find_path(&g_pathFinder, platform_ticks_ms(), from_x, from_y, from_sub_x, from_sub_y,
                                  to_x, to_y, 150, &path, 15);
    path_info_destroy(&path);
    return found;
}

char game_mode_is_valid_cell(struct GameMode *self, int x, int y)
{
    const struct Player *player;
    int from_x, from_y, from_sub_x, from_sub_y;

    if (!cell_is_walkable(self->world->attr, x, y))
        return 0;
    player = self->world->player;
    if (player->requested_cell_x == x && player->requested_cell_y == y)
        return 0;
    game_mode_world_to_sub_cell(self, player->pos.x, player->pos.z, &from_x, &from_y, &from_sub_x, &from_sub_y);
    if (!game_mode_can_find_path(self, from_x, from_y, from_sub_x, from_sub_y, x, y))
        return 0;
    if (game_mode_is_cell_occupied(self, x, y))
        return 0;
    return 1;
}

static int find_free_neighbour(struct GameMode *self, int x, int y, int *out_x, int *out_y)
{
    int i;

    for (i = 0; i < 9; i++) {
        int cand_x = x + kNeighbours[i].dx;
        int cand_y = y + kNeighbours[i].dy;
        if (cell_is_walkable(self->world->attr, cand_x, cand_y)
            && !game_mode_is_cell_occupied(self, cand_x, cand_y)) {
            *out_x = cand_x;
            *out_y = cand_y;
            return 1;
        }
    }
    return 0;
}

static int find_free_cell_on_line(struct GameMode *self, int x, int y, int to_x, int to_y, int *out_x, int *out_y)
{
    int span_x = to_x - x, span_y = to_y - y;
    int step, along, across;

    if (abs(span_x) >= abs(span_y)) {
        if (span_x == 0)
            return 0;
        step = span_x > 0 ? 1 : -1;
        for (along = x; along != to_x; along += step) {
            across = (along - x) * span_y / span_x + y;
            if (find_free_neighbour(self, along, across, out_x, out_y))
                return 1;
        }
        return 0;
    }

    if (span_y == 0)
        return 0;
    step = span_y > 0 ? 1 : -1;
    for (along = y; along != to_y; along += step) {
        across = (along - y) * span_x / span_y + x;
        if (find_free_neighbour(self, across, along, out_x, out_y))
            return 1;
    }
    return 0;
}

char game_mode_get_closest_valid_cell(struct GameMode *self, int *x, int *y)
{
    const struct Player *player = self->world->player;
    int player_x, player_y, player_sub_x, player_sub_y;
    int free_x, free_y;

    game_mode_world_to_sub_cell(self, player->pos.x, player->pos.z,
                            &player_x, &player_y, &player_sub_x, &player_sub_y);
    if (!find_free_cell_on_line(self, *x, *y, player_x, player_y, &free_x, &free_y))
        return 0;
    if (!game_mode_can_find_path(self, player_x, player_y, player_sub_x, player_sub_y, free_x, free_y))
        return 0;
    *x = free_x;
    *y = free_y;
    return 1;
}
