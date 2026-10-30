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

void search_results_push_copy(struct Vec *v, const struct SearchResult *r)
{
    struct SearchResult copy;

    copy.kind = r->kind;
    copy.obj = r->obj;
    copy.x = r->x;
    copy.y = r->y;
    string_copy(&copy.name, &r->name);
    vec_push(v, &copy, sizeof copy);
}

void search_results_clear(struct Vec *v)
{
    struct SearchResult *r;

    for (r = (struct SearchResult *)v->first; r != (struct SearchResult *)v->last; r++)
        search_result_destroy(r);
    vec_clear(v);
}

void search_results_free(struct Vec *v)
{
    search_results_clear(v);
    vec_free(v);
}

static unsigned char results_name_greater(const void *a, const void *b, void *pred)
{
    return navi_result_name_greater((struct SearchResult *)a, (const struct SearchResult *)b);
}

void navi_results_sort(struct SearchResult *first, struct SearchResult *last)
{
    std_sort(first, last, sizeof(struct SearchResult), results_name_greater, NULL);
}

void navi_ui_records_free(struct Vec *v)
{
    struct NaviUiRecord *r;

    for (r = (struct NaviUiRecord *)v->first; r != (struct NaviUiRecord *)v->last; r++)
        string_free(&r->text);
    vec_free(v);
}

void navi_groups_free(struct Vec *v)
{
    struct Vec *g;

    for (g = (struct Vec *)v->first; g != (struct Vec *)v->last; g++)
        vec_free(g);
    vec_free(v);
}
