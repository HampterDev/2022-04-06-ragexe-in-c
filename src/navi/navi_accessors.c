#include <stdlib.h>
#include "session/state.h"
#include "game/context.h"
#include "navi/navi_manager.h"
#include "game/cells.h"
#include "render/texture_cache.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "world/pathfinder_state.h"
#include "navi/navi_search.h"
#include "resources/attr.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "platform/win_types.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"

#include <string.h>

int navi_manager_guided_kind(struct NaviManager *self)
{
    return self->guided.kind;
}

struct NaviMap *navi_manager_current_map(struct NaviManager *self)
{
    struct String name;
    unsigned char same;

    string_init(&name);
    string_assign_cstr(&name, g_session1.mapName);
    same = self->currentMap && navi_map_name_equals(self->currentMap, &name);
    string_free(&name);
    if (!same)
        self->currentMap = navi_manager_map_by_name(self, g_session1.mapName);
    return self->currentMap;
}

unsigned char navi_manager_player_cell(struct NaviManager *self, int *x, int *y)
{
    struct GameMode *gm = game_current_mode();

    if (!gm)
        return 0;
    game_mode_get_player_cell(gm, x, y);
    return 1;
}

int navi_manager_route_step(struct NaviManager *self)
{
    return self->routeStep;
}

struct NaviLink *navi_manager_route_link_at(struct NaviManager *self, int index)
{
    if (index < 0 || VEC_COUNT(&self->routeLinks, struct NaviLink *) <= (unsigned)index)
        return NULL;
    return ((struct NaviLink **)self->routeLinks.first)[index];
}

static struct SearchResult *result_copy(struct SearchResult *out, const struct SearchResult *r)
{
    out->kind = r->kind;
    out->obj = r->obj;
    out->x = r->x;
    out->y = r->y;
    string_copy(&out->name, &r->name);
    return out;
}

struct SearchResult *navi_manager_guided(struct NaviManager *self, struct SearchResult *out)
{
    return result_copy(out, &self->guided);
}

struct String *navi_manager_guided_map_name(struct NaviManager *self, struct String *out)
{
    struct NaviMap *map;

    string_init(out);
    if (!self->guided.obj)
        return out;
    map = navi_manager_result_map(&self->guided);
    if (map) {
        const char *name = navi_map_name(map);

        string_assign_cstr(out, name);
    }
    return out;
}

struct SearchResult *navi_manager_destination(struct NaviManager *self, struct SearchResult *out)
{
    return result_copy(out, &self->destination);
}

struct NaviMap *navi_manager_target_map2(struct NaviManager *self)
{
    return navi_manager_target_map(self);
}

struct NaviPoint *navi_manager_target_pos2(struct NaviManager *self, struct NaviPoint *out)
{
    navi_manager_target_pos(self, out);
    return out;
}

int navi_manager_result_count(struct NaviManager *self)
{
    return (int)VEC_COUNT(&self->results, struct SearchResult);
}

int navi_manager_route_link_count(struct NaviManager *self)
{
    return (int)VEC_COUNT(&self->routeLinks, struct NaviLink *);
}
