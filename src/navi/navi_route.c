#include "navi/navi.h"
#include "game/cells.h"
#include "render/texture_cache.h"
#include "std/string.h"
#include "contents/client_lua_calls.h"
#include "world/pathfinder_state.h"
#include "navi/navi_search.h"
#include "resources/attr.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include "platform/win_types.h"
#include "fileio/resource_api.h"
#include <string.h>
#include "navi/navi_manager.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"

#define NAVI_ROUTE_NO_DISTANCE  50000
#define NAVI_ROUTE_MAX_DEPTH    100

#define ROUTE_NODES(self) ((struct NaviRouteNode *)(self)->base.routeNodes.first)
#define ROUTE_NODE_COUNT(self) ((int)VEC_COUNT(&self->base.routeNodes, struct NaviRouteNode))

static void reset_nodes(struct Vec *nodes)
{
    struct NaviRouteNode *it;
    for (it = (struct NaviRouteNode *)nodes->first; it != (struct NaviRouteNode *)nodes->last; it++) {
        it->prev = -1;
        it->link = NULL;
        it->distance = 0;
    }
    nodes->last = nodes->first;
}

static void push_node(struct Vec *nodes, int prev, struct NaviLink *link, int distance)
{
    struct NaviRouteNode node;

    node.prev = prev;
    node.link = link;
    node.distance = distance;
    vec_push(nodes, &node, sizeof node);
}

static unsigned char link_blocked(struct NaviLink *link, unsigned char useKafra, unsigned char useAirship, int prNpcState)
{
    int type = navi_object_type(&link->base);

    if (type == NAVI_TYPE_LINK_NPC_WARPER) {
        if (!useKafra)
            return 1;
    } else if (type == NAVI_TYPE_LINK_AIRPORT) {
        if (!useAirship)
            return 1;
    } else if (type == NAVI_TYPE_LINK_KAFRA_DUNGEON || type == NAVI_TYPE_LINK_COOL_EVENT_DUNGEON) {
        if (useKafra != 1)
            return 1;
        if (type == NAVI_TYPE_LINK_KAFRA_DUNGEON)
            return prNpcState != 2;
        return prNpcState != 1;
    }
    return navi_map_visited(navi_link_dest_map(link)) != 0;
}

static void clear_visited(struct NaviManager *self)
{
    struct NaviMap **it = (struct NaviMap **)self->base.maps.first;
    struct NaviMap **end = (struct NaviMap **)self->base.maps.last;
    unsigned count = ((unsigned)((char *)end - (char *)it) + 3) >> 2;
    unsigned i;

    if (it > end)
        count = 0;
    for (i = 0; i < count; i++, it++)
        navi_map_set_visited(*it, 0);
}

static unsigned char node_arrives(struct NaviManager *self, int index, unsigned char arriveIsEnough, int *bestIndex)
{
    struct NaviRouteNode *node = &ROUTE_NODES(self)[index];
    int distance;

    if (self->base.targetMap != navi_link_dest_map(node->link))
        return 0;
    if (arriveIsEnough) {
        distance = node->distance;
    } else {
        struct NaviPass *t = (struct NaviPass *)self->base.targetPasses.first;
        struct NaviPass *end = (struct NaviPass *)self->base.targetPasses.last;
        for (; t != end; t++)
            if (node->link == t->link)
                break;
        if (t == end)
            return 0;
        distance = t->distance + node->distance;
    }
    if (self->base.bestDistance > distance) {
        self->base.bestDistance = distance;
        *bestIndex = index;
    }
    return 1;
}

static unsigned char level_arrives(struct NaviManager *self, int from, int to, unsigned char arriveIsEnough,
        int *bestIndex)
{
    int i;

    for (i = from; i < to; i++)
        if (node_arrives(self, i, arriveIsEnough, bestIndex))
            return 1;
    return 0;
}

static void build_path(struct NaviManager *self, struct Vec *path, int bestIndex)
{
    struct Vec reversed;
    struct NaviLink **it;
    int idx, n;

    vec_init(&reversed);
    for (idx = bestIndex; idx != -1; idx = ROUTE_NODES(self)[idx].prev) {
        struct NaviLink *link = ROUTE_NODES(self)[idx].link;
        vec_push(&reversed, &link, sizeof link);
    }
    n = (int)VEC_COUNT(&reversed, struct NaviLink *) - 1;
    path->last = path->first;
    if (n >= 0) {
        it = (struct NaviLink **)reversed.first + n;
        do {
            vec_push(path, it, sizeof *it);
            it--;
        } while (--n >= 0);
    }
    vec_free(&reversed);
}

unsigned char navi_manager_links_into(struct NaviManager *self,
        struct NaviMap *destMap, struct Vec *out)
{
    struct NaviMap **map;
    struct NaviLink **it, **end;

    out->last = out->first;
    for (map = (struct NaviMap **)self->base.maps.first; map != (struct NaviMap **)self->base.maps.last; map++) {
        struct NaviLink **slot;
        it = *navi_map_links_begin(*map, &slot);
        while (it != *navi_map_links_end(*map, &slot)) {
            if (destMap == navi_link_dest_map(*it))
                vec_push(out, it, sizeof *it);
            it++;
        }
    }
    end = (struct NaviLink **)self->base.links.last;
    for (it = (struct NaviLink **)self->base.links.first; it != end; it++)
        if (destMap == navi_link_dest_map(*it))
            vec_push(out, it, sizeof *it);
    return (((char *)out->last - (char *)out->first) & ~3) != 0;
}

int navi_manager_find_route(struct NaviManager *self, struct Vec *path,
        unsigned char arriveIsEnough, unsigned char useKafra, unsigned char useScrolls,
        unsigned char useAirship, int prNpcState)
{
    struct Vec *nodes = &self->base.routeNodes;
    struct NaviPass *pass;
    int levelStart, count, newCount, depth, bestIndex, result, i, j;
    unsigned char found = 0;

    reset_nodes(nodes);
    clear_visited(self);

    if (useScrolls) {
        struct NaviLink **it;
        for (it = (struct NaviLink **)self->base.links.first; it != (struct NaviLink **)self->base.links.last; it++) {
            navi_link_set_depth(*it, 0);
            push_node(nodes, -1, *it, 1);
        }
    }
    for (pass = (struct NaviPass *)self->base.fromPasses.first;
         pass != (struct NaviPass *)self->base.fromPasses.last; pass++) {
        if (link_blocked(pass->link, useKafra, useAirship, prNpcState))
            continue;
        navi_link_set_depth(pass->link, 0);
        push_node(nodes, -1, pass->link, pass->distance);
    }

    count = ROUTE_NODE_COUNT(self);
    levelStart = 0;
    depth = 1;
    bestIndex = -1;
    for (;;) {
        if (levelStart >= count)
            break;
        found = level_arrives(self, levelStart, count, arriveIsEnough, &bestIndex);
        if (found)
            break;
        for (i = levelStart; i < count; i++) {
            struct NaviLink *link = ROUTE_NODES(self)[i].link;
            struct NaviPass *exit, *slot;

            exit = *navi_link_exits_begin(link, &slot);
            while (exit != *navi_link_exits_end(link, &slot)) {
                struct NaviLink *next = exit->link;
                if (next) {
                    if (depth == navi_link_depth(next)) {
                        int distance = exit->distance + ROUTE_NODES(self)[i].distance;
                        unsigned total = (unsigned)ROUTE_NODE_COUNT(self);
                        for (j = count; (unsigned)j < total; j++) {
                            struct NaviRouteNode *node = &ROUTE_NODES(self)[j];
                            if (node->link == next) {
                                if (node->distance > distance) {
                                    node->prev = i;
                                    node->link = next;
                                    node->distance = distance;
                                }
                                break;
                            }
                        }
                    } else if (!(depth < navi_link_depth(next))
                               && navi_link_depth(next) == -1
                               && !link_blocked(next, useKafra, useAirship, prNpcState)) {
                        navi_link_set_depth(next, depth);
                        push_node(nodes, i, next, ROUTE_NODES(self)[i].distance + exit->distance);
                    }
                }
                exit++;
            }
        }
        newCount = ROUTE_NODE_COUNT(self);
        levelStart = count;
        if (count == newCount)
            break;
        count = newCount;
        if (++depth >= NAVI_ROUTE_MAX_DEPTH)
            break;
    }

    result = 0;
    if (found) {
        result = self->base.bestDistance;
        build_path(self, path, bestIndex);
    }
    self->base.bestDistance = NAVI_ROUTE_NO_DISTANCE;
    reset_nodes(nodes);
    navi_manager_reset_link_depths(self);
    return result;
}
