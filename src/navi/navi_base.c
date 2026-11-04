#include "navi/navi_manager.h"
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
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"
#include <stdlib.h>

struct NaviManager *navi_manager_base_construct(struct NaviManager *self)
{
    struct NaviManagerBase *b = &self->base;

    b->maps.first = b->maps.last = b->maps.end = NULL;
    b->links.first = b->links.last = b->links.end = NULL;
    b->targetX = 0;
    b->targetY = 0;
    b->routeNodes.first = b->routeNodes.last = b->routeNodes.end = NULL;
    b->fromPasses.first = b->fromPasses.last = b->fromPasses.end = NULL;
    b->targetPasses.first = b->targetPasses.last = b->targetPasses.end = NULL;
    b->targetMap = NULL;
    b->targetX = 0;
    b->targetY = 0;
    b->bestDistance = 50000;
    return self;
}

void navi_manager_clear_target(struct NaviManager *self)
{
    struct NaviManagerBase *b = &self->base;

    b->targetMap = NULL;
    b->targetX = 0;
    b->targetY = 0;
    b->bestDistance = 50000;
    b->fromPasses.last = b->fromPasses.first;
    b->targetPasses.last = b->targetPasses.first;
    vec_clear(&b->routeNodes);
}

static void base_clear(struct NaviManager *self)
{
    struct NaviManagerBase *b = &self->base;
    struct NaviLink **link;
    struct NaviMap **map;

    navi_manager_clear_target(self);
    self->currentMap = NULL;
    for (link = (struct NaviLink **)b->links.first; link != (struct NaviLink **)b->links.last; link++) {
        if (*link)
            navi_object_free(&(*link)->base);
        *link = NULL;
    }
    b->links.last = b->links.first;
    for (map = (struct NaviMap **)b->maps.first; map != (struct NaviMap **)b->maps.last; map++) {
        if (*map) {
            navi_map_destruct(*map);
            free(*map);
        }
        *map = NULL;
    }
    b->maps.last = b->maps.first;
}

void navi_manager_clear(struct NaviManager *self)
{
    base_clear(self);
}

void navi_manager_base_destruct(struct NaviManager *self)
{
    struct NaviManagerBase *b = &self->base;

    base_clear(self);
    vec_free(&b->targetPasses);
    vec_free(&b->fromPasses);
    vec_free(&b->routeNodes);
    vec_free(&b->links);
    vec_free(&b->maps);
}

unsigned char navi_manager_add_map(struct NaviManager *self, struct NaviMap *map)
{
    struct Vec *v = &self->base.maps;

    if (!map)
        return 0;
    vec_push(v, &map, sizeof map);
    return 1;
}

struct NaviMap *navi_manager_map_by_name(struct NaviManager *self, const char *name)
{
    return name ? navi_map_by_name(self, name, strlen(name)) : NULL;
}

struct NaviMap *navi_map_by_name(struct NaviManager *self, const char *name, size_t size)
{
    struct NaviMap **map;

    if (!self || !name) return NULL;
    for (map = (struct NaviMap **)self->base.maps.first; map != (struct NaviMap **)self->base.maps.last; map++)
        if (strlen(string_cstr(&(*map)->name)) == size &&
            !memcmp(string_cstr(&(*map)->name), name, size))
            return *map;
    return NULL;
}

unsigned char navi_manager_add_scroll_link(struct NaviManager *self,
                                                   struct NaviLink *link)
{
    struct Vec *v = &self->base.links;

    if (!link)
        return 0;
    vec_push(v, &link, sizeof link);
    return 1;
}

struct NaviLink *navi_manager_scroll_link(struct NaviManager *self, int key,
                                                   unsigned char byId)
{
    struct NaviLink **first = (struct NaviLink **)self->base.links.first;
    struct NaviLink **last = (struct NaviLink **)self->base.links.last;
    struct NaviLink **link;

    if (byId) {
        for (link = first; link != last; link++)
            if (navi_object_id_is(&(*link)->base, &key))
                return *link;
        return NULL;
    }
    if (first == last)
        return NULL;
    if ((unsigned)key >= (unsigned)(last - first))
        return NULL;
    return first[key];
}

struct NaviPoint *navi_manager_target_pos(struct NaviManager *self, struct NaviPoint *out)
{
    out->x = self->base.targetX;
    out->y = self->base.targetY;
    return out;
}

struct NaviMap *navi_manager_target_map(struct NaviManager *self)
{
    return self->base.targetMap;
}
