#include "session/state.h"
#include "ui/window_manager_state.h"
#include "game/context.h"
#include "navi/navi_manager.h"
#include "game/cells.h"
#include "render/texture_cache.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "world/pathfinder_state.h"
#include "navi/navi_search.h"
#include "resources/attr.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "platform/win_types.h"
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"

#include <stdlib.h>
#include <string.h>
#include "core/msg_str.h"

#define NAVI_WALK_SEARCH        1
#define NAVI_WALK_RANGE         8
#define NAVI_WALK_LIMIT         10
#define NAVI_WALK_FLAGS         0xf

static void pass_push(struct Vec *v, struct NaviLink *link, int distance)
{
    struct NaviPass pass;

    pass.link = link;
    pass.distance = distance;
    vec_push(v, &pass, sizeof pass);
}

static unsigned char find_walk(struct NaviManager *self, int sx, int sy, int tx, int ty, struct PathInfo *path)
{
    return path_finder_find_path(&self->pathFinder, NAVI_WALK_SEARCH, sx, sy, NAVI_WALK_RANGE, NAVI_WALK_RANGE,
                                 tx, ty, NAVI_WALK_LIMIT, path, NAVI_WALK_FLAGS);
}

unsigned char navi_manager_fill_from_passes(struct NaviManager *self)
{
    struct NaviMap *here = navi_map_named(self, g_session1.mapName);
    struct Vec *passes = &self->base.fromPasses;
    struct PathInfo path;
    struct NaviLink ***end, **it, **endIt;
    unsigned char found;
    int px, py;
    struct GameMode *gm;

    if (!here) {
        ui_navigation_message_box(msg_str(0x8f5));
        return 0;
    }
    if (!navi_map_link_count(here))
        return 0;
    gm = game_current_mode();
    if (!gm)
        return 0;
    game_mode_get_player_cell(gm, &px, &py);
    navi_manager_bind_world(self);
    path_info_init(&path);
    passes->last = passes->first;
    navi_map_links_begin(here, &it);
    for (end = navi_map_links_end(here, &endIt); it != *end; end = navi_map_links_end(here, &endIt)) {
        struct NaviObject *link = &(*it)->base;
        struct NaviPoint a, b;
        int ty = navi_object_position(link, &a)->y;
        int tx = navi_object_position(link, &b)->x;

        if (find_walk(self, px, py, tx, ty, &path))
            pass_push(passes, *it, ((int)path.count));
        path_finder_clear_search(&self->pathFinder);
        it++;
    }
    found = passes->first != passes->last;
    path_info_destroy(&path);
    return found;
}

static unsigned char npc_target_passes(struct NaviManager *self, struct NaviObject *npc)
{
    struct Vec *passes = &self->base.targetPasses;

    if (!(int)VEC_COUNT(&npc->passes, struct NaviPass))
        return 0;
    vec_free(passes);
    vec_copy(passes, &npc->passes, sizeof(struct NaviPass));
    return 1;
}

static unsigned char use_place_map(struct NaviManager *self, struct NaviMap *map, int x, int y)
{
    char *name = res_mgr_resolve_map_name(g_resMgr(), navi_map_name(map));
    unsigned char ok = name && path_finder_use_map_attr(&self->pathFinder, name, strlen(name))
         && path_finder_cell_walkable(&self->pathFinder, x, y);
    free(name);
    return ok;
}

static unsigned char map_target_passes(struct NaviManager *self, struct NaviMap *map, int tx, int ty)
{
    struct Vec *passes = &self->base.targetPasses;
    struct Vec links;
    struct PathInfo path;
    struct NaviLink **it;
    unsigned count, i;
    int distance;

    links.first = NULL;
    links.last = NULL;
    links.end = NULL;
    if (!navi_manager_links_into(self, map, &links)) {
        vec_free(&links);
        return 0;
    }
    distance = 0;
    path_info_init(&path);
    it = (struct NaviLink **)links.first;
    count = (void *)it > links.last ? 0 : (unsigned)((char *)links.last - (char *)it + 3) >> 2;
    for (i = 0; i != count; i++, it++) {
        if (self->destination.kind == NAVI_RESULT_PLACE) {
            struct NaviPoint a, b;

            path_finder_clear_search(&self->pathFinder);
            if (navi_link_dest_pos(*it, &a)->x == tx && navi_link_dest_pos(*it, &b)->y == ty) {
                distance = 1;
            } else {
                int sy = navi_link_dest_pos(*it, &a)->y;
                int sx = navi_link_dest_pos(*it, &b)->x;

                if (!find_walk(self, sx, sy, tx, ty, &path)) {
                    distance = 0;
                    continue;
                }
                distance = ((int)path.count);
            }
        }
        pass_push(passes, *it, distance);
    }
    path_info_destroy(&path);
    vec_free(&links);
    return 1;
}

unsigned char navi_manager_fill_target_passes(struct NaviManager *self, int x, int y)
{
    struct Vec *passes = &self->base.targetPasses;
    void *obj = self->destination.obj;
    struct NaviMap *map;
    int tx = -1, ty = -1;

    if (!obj)
        return 0;
    passes->last = passes->first;
    switch (self->destination.kind) {
    case NAVI_RESULT_NPC: {
        struct NaviPoint pos, *p = navi_object_position(obj, &pos);

        map = navi_object_map(obj);
        ty = p->y;
        tx = p->x;
        if (!npc_target_passes(self, obj))
            return 0;
        break;
    }
    case NAVI_RESULT_MOB:
        map = navi_object_map(obj);
        if (!map_target_passes(self, map, tx, ty))
            return 0;
        break;
    case NAVI_RESULT_MAP:
        map = obj;
        if (!map_target_passes(self, map, tx, ty))
            return 0;
        break;
    case NAVI_RESULT_PLACE:
        tx = x;
        ty = y;
        map = obj;
        if (!use_place_map(self, map, x, y) || !map_target_passes(self, map, tx, ty))
            return 0;
        break;
    default:
        return 0;
    }

    if (passes->first == passes->last)
        return 0;
    return navi_manager_set_target(self, map, tx, ty);
}
