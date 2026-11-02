#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "navi/navi_manager.h"
#include "game/cells.h"
#include "render/texture_cache.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "world/pathfinder_state.h"
#include "navi/navi_search.h"
#include "resources/attr.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include "platform/win_types.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"

#include <stdlib.h>
#include <string.h>
#include "core/msg_str.h"

static void result_clear(struct SearchResult *r)
{
    struct SearchResult empty;

    string_init(&empty.name);
    empty.x = 0;
    empty.y = 0;
    empty.kind = -1;
    empty.obj = NULL;
    empty.x = -1;
    empty.y = -1;
    r->kind = -1;
    r->obj = NULL;
    r->x = -1;
    r->y = -1;
    string_clear(&r->name);
    search_result_destroy(&empty);
}

static unsigned char guide_result(struct NaviManager *self, int index)
{
    struct SearchResult *r;

    if (self->results.first == self->results.last)
        return 0;
    r = &((struct SearchResult *)self->results.first)[index];
    if (!r->obj)
        return 0;
    search_result_assign(&self->guided, r);
    return navi_manager_update_window(self);
}

static void guide_destination_or_link(struct NaviManager *self, int index)
{
    search_result_assign(&self->guided, &self->destination);
    if (index >= 0 && (unsigned)index < VEC_COUNT(&self->routeLinks, struct NaviLink *)) {
        self->guided.kind = NAVI_RESULT_ROUTE_LINK;
        self->guided.obj = ((struct NaviLink **)self->routeLinks.first)[index];
    }
}

unsigned char navi_manager_at_destination(struct NaviManager *self, int x, int y)
{
    void *obj = self->destination.obj;
    struct NaviMap *map;
    int destX, destY, px, py;
    unsigned char same;
    struct GameMode *gm;

    if (!obj)
        return 0;
    if (self->destination.kind == NAVI_RESULT_NPC) {
        struct NaviPoint scratch, *pos;

        map = navi_object_map(obj);
        pos = navi_object_position((struct NaviObject *)obj, &scratch);
        destX = pos->x;
        destY = pos->y;
    } else if (self->destination.kind != NAVI_RESULT_PLACE) {
        return 0;
    } else {
        map = (struct NaviMap *)obj;
        destX = x;
        destY = y;
    }

    same = strcmp(navi_map_name(map), g_session1.mapName) == 0;
    if (!same) {
        char *alias = res_mgr_resolve_map_name(g_resMgr(), g_session1.mapName);
        same = strcmp(navi_map_name(map), alias) == 0;
        free(alias);
    }
    if (!same)
        return 0;
    gm = game_current_mode();
    if (!gm)
        return 0;
    game_mode_get_player_cell(gm, &px, &py);
    if (destX + 1 < px || destX - 1 > px)
        return 0;
    if (destY + 1 < py || destY - 1 > py)
        return 0;
    return 1;
}

int navi_manager_destination_from_guided(struct NaviManager *self)
{
    int kind = self->guided.kind;

    if (kind == NAVI_RESULT_ROUTE_LINK)
        return NAVI_STATUS_ROUTE_LINK_GUIDED;
    if ((kind != NAVI_RESULT_MAP && kind != NAVI_RESULT_NPC && kind != NAVI_RESULT_MOB && kind != NAVI_RESULT_PLACE)
            || !self->guided.obj)
        return NAVI_STATUS_NO_WAY_IN;
    search_result_assign(&self->destination, &self->guided);
    return 0;
}

void navi_manager_reset_guide(struct NaviManager *self)
{
    result_clear(&self->destination);
    self->arriveNearTarget = 0;
    self->walkDistance = 0;
    self->routeLinks.last = self->routeLinks.first;
    self->routeStep = -1;
    self->showPath = 0;
    self->ground = NULL;
    self->pathDots.last = self->pathDots.first;
    self->guideState = NAVI_GUIDE_IDLE;
}

static int guide_on_target_map(struct NaviManager *self, int px, int py)
{
    int kind = self->destination.kind;
    struct NaviPoint a, b;
    int tx, ty, status;

    if (kind != NAVI_RESULT_NPC && kind != NAVI_RESULT_PLACE) {
        status = kind == NAVI_RESULT_MAP ? NAVI_STATUS_ARRIVED_MAP : NAVI_STATUS_NO_ROUTE;
        if (kind == NAVI_RESULT_MOB)
            status = NAVI_STATUS_ARRIVED_MOB_MAP;
        navi_manager_arrive(self);
        return status;
    }
    if (px == navi_manager_target_pos(self, &a)->x && py == navi_manager_target_pos(self, &a)->y) {
        navi_manager_arrive(self);
        return NAVI_STATUS_AT_DESTINATION;
    }
    ty = navi_manager_target_pos(self, &a)->y;
    tx = navi_manager_target_pos(self, &b)->x;
    if (!navi_manager_find_path(self, px, py, tx, ty, 1))
        return NAVI_STATUS_NO_ROUTE;

    self->routeLinks.last = self->routeLinks.first;
    self->routeStep = -1;
    self->walkDistance = abs(px - tx) + abs(py - ty);
    search_result_assign(&self->guided, &self->destination);
    navi_manager_update_window(self);
    self->showPath = 1;
    self->arriveNearTarget = 1;
    self->showMarker = 1;
    status = self->destination.kind == NAVI_RESULT_NPC ? NAVI_STATUS_WALK_TO_NPC : NAVI_STATUS_WALK_TO_PLACE;
    self->guideState = NAVI_GUIDE_ACTIVE;
    return status;
}

int navi_manager_start_guide(struct NaviManager *self)
{
    struct NaviMap *here = navi_map_named(self, g_session1.mapName);
    int status, px, py;
    struct GameMode *gm;

    self->showPath = 0;
    self->arriveNearTarget = 0;
    self->showMarker = 0;
    if (!here)
        return NAVI_STATUS_UNKNOWN_MAP;

    status = NAVI_STATUS_NO_ROUTE;
    px = 0;
    py = 0;
    gm = game_current_mode();
    if (gm)
        game_mode_get_player_cell(gm, &px, &py);

    if (navi_manager_target_map(self) == here) {
        status = guide_on_target_map(self, px, py);
        if (status != NAVI_STATUS_NO_ROUTE)
            return status;
    }

    if (navi_manager_plan_route(self, here, px, py)) {
        status = NAVI_STATUS_ROUTE;
        if (self->routeStep == 0) {
            struct NaviLink *first = *(struct NaviLink **)self->routeLinks.first;

            if (navi_object_type(&first->base) == NAVI_TYPE_WARP_SCROLL)
                status = NAVI_STATUS_ROUTE_BY_SCROLL;
        }
        self->guideState = NAVI_GUIDE_ACTIVE;
    }
    return status;
}

int navi_manager_guide_on_map(struct NaviManager *self, int x, int y)
{
    struct NaviMap *here, *target;
    int kind, tx, ty, px, py;
    struct GameMode *gm;

    self->routeStep = -1;
    here = navi_map_named(self, g_session1.mapName);
    if (!here)
        return NAVI_STATUS_UNKNOWN_MAP;

    kind = self->destination.kind;
    target = NULL;
    tx = x;
    ty = y;
    if (kind == NAVI_RESULT_MOB)
        return here == navi_object_map(self->destination.obj) ? NAVI_STATUS_ON_MOB_MAP : NAVI_STATUS_NOT_ON_MAP;
    if (kind == NAVI_RESULT_MAP)
        return here == (struct NaviMap *)self->destination.obj ? NAVI_STATUS_ON_MAP : NAVI_STATUS_NOT_ON_MAP;
    if (kind == NAVI_RESULT_PLACE) {
        if (x < 0 || y < 0)
            return NAVI_STATUS_NO_WAY_IN;
        target = (struct NaviMap *)self->destination.obj;
    } else if (kind == NAVI_RESULT_NPC) {
        struct NaviObject *npc = (struct NaviObject *)self->destination.obj;
        struct NaviPoint scratch, *pos;

        target = navi_object_map(npc);
        pos = navi_object_position(npc, &scratch);
        tx = pos->x;
        ty = pos->y;
    }
    if (here != target)
        return NAVI_STATUS_NOT_ON_MAP;

    px = 0;
    py = 0;
    gm = game_current_mode();
    if (!gm)
        return NAVI_STATUS_NO_GAME_MODE;
    game_mode_get_player_cell(gm, &px, &py);
    navi_manager_bind_world(self);
    if (!navi_manager_find_path(self, px, py, tx, ty, 0))
        return NAVI_STATUS_NOT_ON_MAP;
    navi_manager_set_target(self, target, tx, ty);
    self->showPath = 1;
    self->arriveNearTarget = 1;
    self->showMarker = 1;
    self->guideState = NAVI_GUIDE_ACTIVE;
    return NAVI_STATUS_WALK_TO_PLACE;
}

int navi_manager_guide_to(struct NaviManager *self, int x, int y)
{
    int kind;

    if (navi_manager_at_destination(self, x, y))
        return NAVI_STATUS_AT_DESTINATION;
    if (navi_manager_guide_on_map(self, x, y) != NAVI_STATUS_NOT_ON_MAP)
        return navi_manager_start_guide(self);
    if (!navi_manager_fill_target_passes(self, x, y))
        return NAVI_STATUS_NO_WAY_IN;
    if (!navi_manager_fill_from_passes(self))
        return NAVI_STATUS_NO_WAY_OUT;
    kind = self->destination.kind;
    self->walkDistance = navi_manager_find_route(self, &self->routeLinks,
            kind != NAVI_RESULT_NPC && kind != NAVI_RESULT_PLACE,
            self->routeUseKafra, self->routeUseScrolls, self->routeUseAirship, self->routePrNpcState);
    if (!self->walkDistance)
        return NAVI_STATUS_NO_ROUTE;
    return navi_manager_start_guide(self);
}

void navi_manager_stop_guide(struct NaviManager *self)
{
    result_clear(&self->guided);
    navi_manager_update_window(self);
    navi_manager_clear_info_lines(self);
}

void navi_manager_arrive(struct NaviManager *self)
{
    navi_manager_reset(self, 0);
    self->windowPage = 0;
    if (!self->windowHidden) {
        self->guideState = NAVI_GUIDE_IDLE;
        ui_navigation_relayout();
    }
    ui_navigation_chat(msg_str(0x905));
}

void navi_manager_resume(struct NaviManager *self)
{
    if (self->guideState == NAVI_GUIDE_IDLE) {
        navi_manager_update_window(self);
        ui_navigation_open();
        return;
    }
    if (self->guideState == NAVI_GUIDE_ACTIVE) {
        navi_manager_report_status(self, navi_manager_start_guide(self), NAVI_REPORT_CHAT);
        if (!self->windowHidden)
            ui_navigation_open();
    }
}

void navi_manager_reset(struct NaviManager *self, unsigned char all)
{
    if (all) {
        navi_manager_clear_groups(self);
        self->selectedGroup = -1;
        self->selectedMember = -1;
        search_results_clear(&self->results);
    }
    result_clear(&self->destination);
    result_clear(&self->guided);
    self->walkDistance = 0;
    self->routeLinks.last = self->routeLinks.first;
    self->arriveNearTarget = 0;
    self->routeStep = -1;
    self->showPath = 0;
    self->ground = NULL;
    self->guideState = NAVI_GUIDE_OFF;
    self->searchFilter = 0;
    string_clear(&self->searchText);
    self->routePrNpcState = 0;
    self->pictureKind = 0;
    self->picture = NULL;
    self->minimapX = -1;
    self->minimapY = -1;
    self->pathDots.last = self->pathDots.first;
    self->actRes = NULL;
    self->sprRes = NULL;
    navi_manager_clear_info_lines(self);
}

void navi_manager_clear_target_and_marker(struct NaviManager *self)
{
    self->showMarker = 0;
    navi_manager_clear_target(self);
}

static unsigned char link_walk(struct NaviManager *self, struct NaviLink *l, int x, int y)
{
    struct NaviObject *link = &l->base;
    struct NaviPoint a, b;
    int ty = navi_object_position(link, &a)->y;

    return navi_manager_find_path(self, x, y, navi_object_position(link, &b)->x, ty, 1);
}

static void take_step(struct NaviManager *self, int step, unsigned char showPath)
{
    self->routeStep = step;
    navi_manager_guide_link(self, step);
    self->showPath = showPath;
}

unsigned char navi_manager_plan_route(struct NaviManager *self,
        struct NaviMap *here, int x, int y)
{
    struct Vec others;
    struct NaviLink **links;
    unsigned char ok = 0;
    int i, n, k;

    if (self->routeLinks.first == self->routeLinks.last || !here)
        return 0;
    navi_manager_bind_world(self);

    others.first = others.last = others.end = NULL;
    for (i = (int)VEC_COUNT(&self->routeLinks, struct NaviLink *) - 1; i > 0; i--) {
        struct NaviPoint pos, *at;

        links = (struct NaviLink **)self->routeLinks.first;
        if (navi_object_map(&(links[i])->base) != here)
            continue;
        at = navi_link_dest_pos(links[i - 1], &pos);
        if (at->x == x && at->y == y) {
            if (link_walk(self, ((struct NaviLink **)self->routeLinks.first)[i], x, y)) {
                take_step(self, i, 1);
                ok = 1;
                break;
            }
        } else {
            vec_push(&others, &i, sizeof i);
        }
    }

    links = (struct NaviLink **)self->routeLinks.first;
    if (!ok && navi_object_map(&(links[0])->base) == here && link_walk(self, links[0], x, y)) {
        take_step(self, 0, 1);
        ok = 1;
    }
    n = (int)VEC_COUNT(&others, int);
    for (k = 0; !ok && k < n; k++) {
        int index = ((int *)others.first)[k];

        if (link_walk(self, ((struct NaviLink **)self->routeLinks.first)[index], x, y)) {
            take_step(self, index, 1);
            ok = 1;
        }
    }
    if (!ok) {
        self->showPath = 0;
        if (navi_object_type(&(*(struct NaviLink **)self->routeLinks.first)->base) == NAVI_TYPE_WARP_SCROLL) {
            take_step(self, 0, 0);
            ok = 1;
        }
    }
    vec_free(&others);
    return ok;
}

unsigned char navi_manager_guide_link(struct NaviManager *self, int index)
{
    struct SearchResult tmp;

    guide_destination_or_link(self, index);

    tmp.kind = self->guided.kind;
    tmp.obj = self->guided.obj;
    tmp.x = self->guided.x;
    tmp.y = self->guided.y;
    string_copy(&tmp.name, &self->guided.name);
    search_result_assign(&self->guided, &tmp);
    search_result_destroy(&tmp);
    return navi_manager_update_window(self);
}

unsigned char navi_manager_guide_result(struct NaviManager *self, int index)
{
    return guide_result(self, index);
}

unsigned char navi_manager_guide_selected(struct NaviManager *self)
{
    int group = self->selectedGroup, member = self->selectedMember;
    int index;

    if (group >= 0 && (unsigned)group < VEC_COUNT(&self->resultGroups, struct Vec) && member >= 0) {
        struct Vec *g = &((struct Vec *)self->resultGroups.first)[group];

        if ((unsigned)member < VEC_COUNT(g, int) && (index = ((int *)g->first)[member]) != -1)
            return guide_result(self, index);
    }
    result_clear(&self->guided);
    navi_manager_update_window(self);
    navi_manager_clear_info_lines(self);
    return 0;
}
