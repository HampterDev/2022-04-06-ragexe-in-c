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
#include <windows.h>
#include <mmsystem.h>
#include "fileio/resource_api.h"
#include <string.h>
#include "navi/navi_manager.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"

static unsigned char ptrs_push(struct Vec *v, void *item)
{
    if (!item)
        return 0;
    vec_push(v, &item, sizeof item);
    return 1;
}

static void *ptrs_get(const struct Vec *v, int key, unsigned char byId)
{
    if (byId) {
        struct NaviObject **it;
        for (it = (struct NaviObject **)v->first; it != (struct NaviObject **)v->last; it++)
            if ((*it)->id == key)
                return *it;
        return NULL;
    }
    if (v->first == v->last)
        return NULL;
    if ((unsigned)key >= VEC_COUNT(v, void *))
        return NULL;
    return ((void **)v->first)[key];
}

static void *ptrs_find_at(const struct Vec *v, int x, int y)
{
    struct NaviObject **it;
    for (it = (struct NaviObject **)v->first; it != (struct NaviObject **)v->last; it++)
        if ((*it)->x == x && (*it)->y == y)
            return *it;
    return NULL;
}

static void ptrs_delete_all(struct Vec *v)
{
    struct NaviObject **it;
    for (it = (struct NaviObject **)v->first; it != (struct NaviObject **)v->last; it++) {
        if (*it)
            navi_object_free(*it);
        *it = NULL;
    }
    v->last = v->first;
}

struct NaviMap *navi_map_construct(struct NaviMap *self, int id,
        const char *name, const char *displayName, int width, int height, int mapType)
{
    string_init(&self->name);
    string_init(&self->displayName);
    self->npcs.first = self->npcs.last = self->npcs.end = NULL;
    self->mobs.first = self->mobs.last = self->mobs.end = NULL;
    self->links.first = self->links.last = self->links.end = NULL;
    self->id = id;
    self->mapType = mapType;
    string_assign_cstr(&self->name, name);
    string_assign_cstr(&self->displayName, displayName);
    self->width = width;
    self->height = height;
    self->visited = 0;
    return self;
}

void navi_map_destruct(struct NaviMap *self)
{
    self->id = 0;
    self->mapType = NAVI_MAP_TYPE_DEFAULT;
    string_clear(&self->name);
    string_clear(&self->displayName);
    self->width = 0;
    self->height = 0;
    ptrs_delete_all(&self->npcs);
    ptrs_delete_all(&self->mobs);
    ptrs_delete_all(&self->links);
    vec_free(&self->links);
    vec_free(&self->mobs);
    vec_free(&self->npcs);
    string_free(&self->displayName);
    string_free(&self->name);
}

const char *navi_map_name(struct NaviMap *self)
{
    return string_cstr(&self->name);
}

const char *navi_map_display_name(struct NaviMap *self)
{
    return string_cstr(&self->displayName);
}

int navi_map_map_type(struct NaviMap *self)
{
    return self->mapType;
}

int navi_map_width(struct NaviMap *self)
{
    return self->width;
}

int navi_map_height(struct NaviMap *self)
{
    return self->height;
}

unsigned char navi_map_visited(struct NaviMap *self)
{
    return self->visited;
}

void navi_map_set_visited(struct NaviMap *self, unsigned char visited)
{
    if (self->mapType == NAVI_MAP_TYPE_DEFAULT)
        self->visited = visited;
}

unsigned char navi_map_name_equals(struct NaviMap *self, const struct String *name)
{
    return string_equal(&self->name, name);
}

unsigned char navi_map_add_npc(struct NaviMap *self, struct NaviObject *npc)
{
    return ptrs_push(&self->npcs, npc);
}

struct NaviObject *navi_map_npc(struct NaviMap *self, int key, unsigned char byId)
{
    return (struct NaviObject *)ptrs_get(&self->npcs, key, byId);
}

struct NaviObject *navi_map_npc_at(struct NaviMap *self, int x, int y)
{
    return (struct NaviObject *)ptrs_find_at(&self->npcs, x, y);
}

unsigned char navi_map_add_mob(struct NaviMap *self, struct NaviObject *mob)
{
    return ptrs_push(&self->mobs, mob);
}

struct NaviObject *navi_map_mob_by_name(struct NaviMap *self, const char *name, size_t size)
{
    struct NaviObject **it;
    if (!self || !name) return NULL;
    for (it = (struct NaviObject **)self->mobs.first; it != (struct NaviObject **)self->mobs.last; ++it)
        if ((*it)->name.size == size && !memcmp(string_cstr(&(*it)->name), name, size))
            return *it;
    return NULL;
}

struct NaviObject *navi_map_mob_by_sprite(struct NaviMap *self, int sprite)
{
    struct NaviObject **it;
    for (it = (struct NaviObject **)self->mobs.first; it != (struct NaviObject **)self->mobs.last; it++)
        if (sprite == navi_object_sprite(*it))
            return *it;
    return NULL;
}

unsigned char navi_map_add_link(struct NaviMap *self, struct NaviLink *link)
{
    return ptrs_push(&self->links, link);
}

struct NaviLink *navi_map_link(struct NaviMap *self, int key, unsigned char byId)
{
    return (struct NaviLink *)ptrs_get(&self->links, key, byId);
}

struct NaviLink *navi_map_find_link_route(struct NaviMap *self, int x, int y,
        const struct NaviMap *destination, int destination_x, int destination_y)
{
    struct NaviLink **it;

    if (!self || !destination) return NULL;
    for (it = (struct NaviLink **)self->links.first; it != (struct NaviLink **)self->links.last; it++) {
        struct NaviLink *l = *it;
        if (l->base.x != x || l->base.y != y)
            continue;
        if (l->destX != destination_x || l->destY != destination_y)
            continue;
        if (!l->base.map)
            continue;
        if (strcmp(string_cstr(&l->base.map->name), string_cstr(&self->name)) != 0)
            continue;
        if (!l->destMap)
            continue;
        if (strcmp(string_cstr(&l->destMap->name), string_cstr(&destination->name)) == 0)
            return l;
    }
    return NULL;
}

struct NaviLink ***navi_map_links_begin(struct NaviMap *self, struct NaviLink ***out)
{
    *out = (struct NaviLink **)self->links.first;
    return out;
}

struct NaviLink ***navi_map_links_end(struct NaviMap *self, struct NaviLink ***out)
{
    *out = (struct NaviLink **)self->links.last;
    return out;
}

int navi_map_link_count(struct NaviMap *self)
{
    return (int)VEC_COUNT(&self->links, void *);
}
