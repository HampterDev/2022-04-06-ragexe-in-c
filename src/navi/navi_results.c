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
#include <windows.h>
#include <mmsystem.h>
#include "fileio/resource_api.h"
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"

#include <string.h>

void navi_manager_clear_info_lines(struct NaviManager *self)
{
    struct NaviUiRecord *rec;
    struct Vec *v = &self->infoLines;

    for (rec = (struct NaviUiRecord *)v->first; rec != (struct NaviUiRecord *)v->last; rec++)
        string_free(&rec->text);
    vec_clear(v);
}

void navi_manager_clear_groups(struct NaviManager *self)
{
    struct Vec *g, *end;
    unsigned i;

    for (i = 0; i < VEC_COUNT(&self->resultGroups, struct Vec); i++) {
        g = &((struct Vec *)self->resultGroups.first)[i];
        g->last = g->first;
    }
    end = (struct Vec *)self->resultGroups.last;
    for (g = (struct Vec *)self->resultGroups.first; g != end; g++)
        vec_free(g);
    self->resultGroups.last = self->resultGroups.first;
}

int navi_manager_group_of(struct NaviManager *self, int index)
{
    struct SearchResult *results = (struct SearchResult *)self->results.first;
    unsigned i;

    if (results[index].kind == NAVI_RESULT_MAP || results[index].kind == NAVI_RESULT_PLACE)
        return -1;
    for (i = 0; i < VEC_COUNT(&self->resultGroups, struct Vec); i++) {
        struct Vec *g = &((struct Vec *)self->resultGroups.first)[i];
        unsigned head;
        struct SearchResult *r, *h;

        if ((((char *)g->last - (char *)g->first) & ~3) == 0)
            return -1;
        head = *(unsigned *)g->first;
        results = (struct SearchResult *)self->results.first;
        if (head >= VEC_COUNT(&self->results, struct SearchResult))
            return -1;
        r = &results[index];
        h = &results[head];
        if (r->kind == NAVI_RESULT_MOB && r->x == NAVI_POS_UNDER_MAP && r->y == NAVI_POS_UNDER_MAP) {
            struct NaviMap *hmap;
            struct NaviObject *mob;

            if (h->kind != NAVI_RESULT_MAP)
                continue;
            hmap = (struct NaviMap *)h->obj;
            if (!hmap)
                return -1;
            mob = (struct NaviObject *)r->obj;
            if (!mob)
                return -1;
            if (!navi_object_map(mob))
                return -1;
            {
                const char *mobMap = navi_map_name(navi_object_map(mob));

                if (strcmp(navi_map_name(hmap), mobMap) == 0)
                    return (int)i;
            }
            results = (struct SearchResult *)self->results.first;
            r = &results[index];
            h = &results[head];
        }
        if (h->kind == r->kind) {
            const char *rname;

            if (!h->obj)
                return -1;
            if (!r->obj)
                return -1;
            rname = navi_object_name(r->obj);
            if (strcmp(navi_object_name(h->obj), rname) == 0)
                return (int)i;
        }
    }
    return -1;
}

struct SearchResult *navi_manager_result_at(struct NaviManager *self,
        struct SearchResult *out, int index)
{
    if (index >= 0 && (unsigned)index < VEC_COUNT(&self->results, struct SearchResult)) {
        const struct SearchResult *r = &((struct SearchResult *)self->results.first)[index];

        out->kind = r->kind;
        out->obj = r->obj;
        out->x = r->x;
        out->y = r->y;
        string_copy(&out->name, &r->name);
        return out;
    }
    search_result_init(out, -1);
    return out;
}

struct String *navi_manager_result_map_name(struct NaviManager *self,
        struct String *out, int index, int *sprite)
{
    struct String name;

    string_init(&name);
    if (index >= 0 && (unsigned)index < VEC_COUNT(&self->results, struct SearchResult)) {
        struct SearchResult *r = &((struct SearchResult *)self->results.first)[index];
        void *obj = r->obj;

        if (obj) {
            struct NaviMap *map = NULL;
            int ok = 1;

            switch (r->kind) {
            case NAVI_RESULT_PLACE:
            case NAVI_RESULT_MAP:
                map = (struct NaviMap *)obj;
                break;
            case NAVI_RESULT_NPC:
            case NAVI_RESULT_MOB:
            case NAVI_RESULT_ROUTE_LINK:
                map = navi_object_map(obj);
                break;
            default:
                ok = 0;
                break;
            }
            if (ok && map) {
                const char *text = navi_map_name(map);

                string_assign_cstr(&name, text);
            }
        }
        r = &((struct SearchResult *)self->results.first)[index];
        if (r->kind == NAVI_RESULT_MOB)
            *sprite = navi_object_sprite_high(r->obj);
        else
            *sprite = r->kind == NAVI_RESULT_NPC ? NAVI_SPRITE_NPC : 0;
    }
    *out = name;
    string_init(&name);
    string_free(&name);
    return out;
}

struct NaviMap *navi_manager_result_map(const struct SearchResult *r)
{
    void *obj = r->obj;

    if (!obj)
        return NULL;
    switch (r->kind) {
    case NAVI_RESULT_PLACE:
    case NAVI_RESULT_MAP:
        return (struct NaviMap *)obj;
    case NAVI_RESULT_NPC:
    case NAVI_RESULT_MOB:
    case NAVI_RESULT_ROUTE_LINK:
        return navi_object_map(obj);
    default:
        return NULL;
    }
}

static void new_group(struct NaviManager *self, int *index)
{
    struct Vec g;

    vec_init(&g);
    vec_push(&g, index, sizeof *index);
    vec_push(&self->resultGroups, &g, sizeof g);
}

void navi_manager_build_groups(struct NaviManager *self)
{
    int i;

    navi_manager_clear_groups(self);
    if (self->searchFilter == NAVI_SEARCH_ALL || self->searchFilter == NAVI_RESULT_MAP) {
        for (i = 0; (unsigned)i < VEC_COUNT(&self->results, struct SearchResult); i++)
            if (((struct SearchResult *)self->results.first)[i].kind == NAVI_RESULT_MAP)
                new_group(self, &i);
    }
    for (i = 0; (unsigned)i < VEC_COUNT(&self->results, struct SearchResult); i++) {
        int kind = ((struct SearchResult *)self->results.first)[i].kind;
        int group;

        if (self->searchFilter != NAVI_SEARCH_ALL && kind != self->searchFilter)
            continue;
        if (kind == NAVI_RESULT_MAP)
            continue;
        group = navi_manager_group_of(self, i);
        if (group == -1) {
            new_group(self, &i);
        } else {
            struct Vec *g = &((struct Vec *)self->resultGroups.first)[group];

            vec_push(g, &i, sizeof i);
        }
    }
}
