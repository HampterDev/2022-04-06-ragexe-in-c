#include "render/texture_image.h"
#include "ui/window_manager_state.h"
#include "navi/navi_manager.h"
#include "game/cells.h"
#include "render/texture_cache.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "world/pathfinder_state.h"
#include "navi/navi_search.h"
#include "resources/attr.h"
#include "session/state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "platform/win_types.h"
#include "fileio/resource_api.h"
#include <string.h>
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"

#include <stdlib.h>
#include "ui/social/guild_member_sort.h"

struct NaviManager g_navi;

struct NaviManager *navi_manager_construct(struct NaviManager *self)
{
    int i;

    navi_manager_base_construct(self);
    path_finder_construct(&self->pathFinder);
    self->results.first = self->results.last = self->results.end = NULL;
    self->guided.x = 0;
    self->guided.y = 0;
    string_init(&self->guided.name);
    self->guided.kind = -1;
    self->guided.obj = NULL;
    self->guided.x = -1;
    self->guided.y = -1;
    self->destination.x = 0;
    self->destination.y = 0;
    string_init(&self->destination.name);
    self->destination.kind = -1;
    self->destination.obj = NULL;
    self->destination.x = -1;
    self->destination.y = -1;
    self->routeLinks.first = self->routeLinks.last = self->routeLinks.end = NULL;
    path_info_init(&self->path);
    for (i = 0; i < 8; i++)
        string_init(&self->gridImages[i]);
    string_init(&self->searchText);
    self->minimapX = 0;
    self->minimapY = 0;
    self->pathDots.first = self->pathDots.last = self->pathDots.end = NULL;
    self->infoLines.first = self->infoLines.last = self->infoLines.end = NULL;
    self->resultGroups.first = self->resultGroups.last = self->resultGroups.end = NULL;
    for (i = 0; i < 5; i++)
        string_init(&self->texts[i]);
    self->roadIcon = 1;
    self->routeUseKafra = 0;
    self->routeUseScrolls = 0;
    self->routeUseAirship = 0;
    self->window_y = -1;
    self->window_x = -1;
    self->windowHidden = 0;
    self->windowCollapsed = 0;
    navi_manager_reset(self, 1);
    self->pathFinder.useHeapNodes = 1;
    self->currentMap = NULL;
    navi_manager_load_grid_images(self);
    self->showMarker = 0;
    return self;
}

void navi_manager_destruct(struct NaviManager *self)
{
    int i;

    navi_manager_reset(self, 1);
    self->currentMap = NULL;
    for (i = 4; i >= 0; i--)
        string_free(&self->texts[i]);
    navi_groups_free(&self->resultGroups);
    navi_ui_records_free(&self->infoLines);
    vec_free(&self->pathDots);
    string_free(&self->searchText);
    for (i = 7; i >= 0; i--)
        string_free(&self->gridImages[i]);
    path_info_destroy(&self->path);
    vec_free(&self->routeLinks);
    search_result_destroy(&self->destination);
    search_result_destroy(&self->guided);
    search_results_free(&self->results);
    path_finder_destruct(&self->pathFinder);
    navi_manager_base_destruct(self);
}

struct NaviMap *navi_map_named(struct NaviManager *self, const char *name)
{
    struct NaviMap *map = navi_manager_map_by_name(self, name);

    if (!map) {
        char *alias = res_mgr_resolve_map_name(g_resMgr(), name);
        map = navi_manager_map_by_name(self, alias);
        free(alias);
    }
    return map;
}

unsigned char navi_manager_set_target(struct NaviManager *self,
        struct NaviMap *map, int x, int y)
{
    if (!map)
        return 0;
    self->base.targetMap = map;
    self->base.targetX = x;
    self->base.targetY = y;
    return 1;
}

int navi_manager_map_count(struct NaviManager *self)
{
    return (int)VEC_COUNT(&self->base.maps, void *);
}

void navi_manager_reset_link_depths(struct NaviManager *self)
{
    struct NaviMap **map;
    struct NaviMap **maps_end = (struct NaviMap **)self->base.maps.last;

    for (map = (struct NaviMap **)self->base.maps.first; map != maps_end; map++) {
        struct NaviLink **endSlot, **beginSlot, **end, **it;
        unsigned count, i;

        end = *navi_map_links_end(*map, &endSlot);
        it = *navi_map_links_begin(*map, &beginSlot);
        count = (unsigned)(end - it);
        if (it > end)
            count = 0;
        for (i = 0; i < count; i++, it++)
            navi_link_set_depth(*it, -1);
    }
}

void navi_manager_check_arrival(struct NaviManager *self, int x, int y)
{
    struct NaviPoint target;

    if (self->guideState != NAVI_GUIDE_ACTIVE)
        return;
    ui_navigation_invalidate();
    if (!self->arriveNearTarget)
        return;
    navi_manager_target_pos(self, &target);
    if (abs(target.x - x) > 5)
        return;
    if (abs(target.y - y) > 5)
        return;
    navi_manager_arrive(self);
}

unsigned char navi_manager_set_guide_state(struct NaviManager *self, int state)
{
    if (state != NAVI_GUIDE_IDLE && state != NAVI_GUIDE_ACTIVE && state != NAVI_GUIDE_OFF)
        return 0;
    self->guideState = state;
    return 1;
}
