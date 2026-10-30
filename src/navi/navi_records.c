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
#include <string.h>
#include "fileio/res_mgr.h"
#include "ui/navigation_view.h"

#include <stdlib.h>

struct SearchResult *search_result_construct(struct SearchResult *self, int kind, void *obj)
{
    string_init(&self->name);
    self->kind = kind;
    self->obj = obj;
    self->x = -1;
    self->y = -1;
    return self;
}

struct SearchResult *search_result_assign(struct SearchResult *self,
        const struct SearchResult *other)
{
    self->kind = other->kind;
    self->obj = other->obj;
    self->x = other->x;
    self->y = other->y;
    string_assign(&self->name, string_cstr(&other->name), other->name.size);
    return self;
}

unsigned char navi_manager_add_place(struct NaviManager *self, const struct Vec *words)
{
    const struct String *w = (const struct String *)words->first;
    struct NaviMap *map;
    struct NaviPoint pos;
    struct String title, copy;
    struct SearchResult place;
    unsigned i;

    if (VEC_COUNT(words, struct String) < 2)
        return 0;
    map = navi_manager_map_by_name(self, string_cstr(&w[0]));
    if (!map)
        return 0;
    navi_manager_parse_pos(self, &pos, string_cstr(&w[1]));
    if (pos.x == -1 || pos.y == -1)
        return 0;

    string_init(&title);
    for (i = 2; i < VEC_COUNT(words, struct String); i++) {
        w = (const struct String *)words->first;
        string_append_cstr(&title, " ");
        string_append(&title, string_cstr(&w[i]), w[i].size);
    }
    string_copy(&copy, &title);
    string_init(&place.name);
    place.obj = map;
    place.x = pos.x;
    place.y = pos.y;
    place.kind = NAVI_RESULT_PLACE;
    string_assign(&place.name, string_cstr(&copy), copy.size);
    string_free(&copy);
    search_results_push_copy(&self->results, &place);
    search_result_destroy(&place);
    string_free(&title);
    return 1;
}

unsigned char navi_manager_parse_flags(struct NaviManager *self, const char *text,
        unsigned char *useScrolls, unsigned char *useKafra, unsigned char *useAirship)
{
    const char *slash1, *slash2;
    struct String first, second, third;
    unsigned char ok;

    slash1 = strstr(text, "/");
    if (!slash1)
        return 0;
    slash2 = strstr(slash1 + 1, "/");
    if (!slash2)
        return 0;

    string_init(&first);
    string_assign(&first, text, (size_t)(slash1 - text));
    if (first.size == 0) {
        ok = 0;
    } else {
        string_init(&second);
        string_assign(&second, slash1 + 1, (size_t)(slash2 - slash1 + 1));
        if (second.size == 0) {
            ok = 0;
        } else {
            string_init(&third);
            string_assign(&third, slash2 + 1, (size_t)(text + strlen(text) - slash2 + 1));
            if (third.size == 0) {
                ok = 0;
            } else {
                int scrolls = atoi(string_cstr(&first));
                int kafra = atoi(string_cstr(&second));
                int airship = atoi(string_cstr(&third));

                if ((scrolls == 0 || scrolls == 1) && (kafra == 0 || kafra == 1)
                        && (airship == 0 || airship == 1)) {
                    ok = 1;
                    *useScrolls = scrolls == 1;
                    *useKafra = kafra == 1;
                    *useAirship = airship == 1;
                } else {
                    ok = 0;
                }
            }
            string_free(&third);
        }
        string_free(&second);
    }
    string_free(&first);
    return ok;
}

struct NaviPoint *navi_manager_parse_pos(struct NaviManager *self,
        struct NaviPoint *out, const char *text)
{
    const char *slash;
    struct String xs, ys;

    if (!text || !(slash = strstr(text, "/"))) {
        out->x = -1;
        out->y = -1;
        return out;
    }
    string_init(&xs);
    string_assign(&xs, text, (size_t)(slash - text));
    if (xs.size == 0) {
        out->x = -1;
        out->y = -1;
    } else {
        string_init(&ys);
        string_assign(&ys, slash + 1, (size_t)(text + strlen(text) - slash + 1));
        if (ys.size != 0) {
            int x = atoi(string_cstr(&xs));
            int y = atoi(string_cstr(&ys));

            if ((unsigned)x <= 500 && (unsigned)y <= 500) {
                out->x = x;
                out->y = y;
            } else {
                out->x = -1;
                out->y = -1;
            }
        } else {
            out->x = -1;
            out->y = -1;
        }
        string_free(&ys);
    }
    string_free(&xs);
    return out;
}
