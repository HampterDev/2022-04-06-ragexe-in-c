#include "navi/navi_manager.h"
#include "navi/navi_search.h"
#include "render/texture_image.h"
#include "render/render_api.h"
#include "world_state_memory.h"
#include <stdio.h>

#define NAVI_RESULT_NONE    (-1)

static unsigned char update_result;
static int update_calls;
unsigned char navi_manager_update_window(struct NaviManager *view)
{
    assert(view == &g_navi);
    ++update_calls;
    return update_result;
}

int main(void)
{
    struct NaviLink links[2] = {0};
    struct NaviLink *route[] = {&links[0], &links[1]};
    struct NaviPoint point, *at;
    struct Vec points = {0};
    struct PathInfo path;
    struct Texture texture = {0};
    const char *name = "A destination name long enough to require owned storage";
    int i;
    assert(!g_navi.results.first && !g_navi.windowHidden && !g_navi.roadIcon);
    search_result_construct(&g_navi.destination, NAVI_RESULT_PLACE, &links[0]);
    search_result_construct(&g_navi.guided, NAVI_RESULT_NONE, NULL);
    string_assign_cstr(&g_navi.destination.name, name);
    g_navi.destination.x = 17; g_navi.destination.y = 29;
    g_navi.routeLinks.first = route; g_navi.routeLinks.last = route + 2; g_navi.routeLinks.end = route + 2;
    update_result = 1;
    assert(navi_manager_guide_link(&g_navi, 1) == 1 && update_calls == 1);
    assert(g_navi.guided.kind == NAVI_RESULT_ROUTE_LINK && g_navi.guided.obj == &links[1]);
    assert(g_navi.guided.x == 17 && g_navi.guided.y == 29);
    assert(strcmp(string_cstr(&g_navi.guided.name), name) == 0);
    assert(string_cstr(&g_navi.guided.name) != string_cstr(&g_navi.destination.name));
    update_result = 0;
    assert(navi_manager_guide_link(&g_navi, -1) == 0 && update_calls == 2);
    assert(g_navi.guided.kind == NAVI_RESULT_PLACE && g_navi.guided.obj == &links[0]);
    assert(navi_manager_guide_link(&g_navi, 2) == 0 && update_calls == 3);
    g_navi.routeLinks.first = g_navi.routeLinks.last = g_navi.routeLinks.end = NULL;
    assert(navi_manager_guide_link(&g_navi, 0) == 0 && update_calls == 4);
    search_result_destroy(&g_navi.guided); search_result_destroy(&g_navi.destination);
    assert(!live_blocks);

    for (i = 0; i < 300; ++i) {
        point.x = i; point.y = -i;
        vec_push(&points, &point, sizeof point);
    }
    at = points.first;
    for (i = 0; i < 300; ++i) assert(at[i].x == i && at[i].y == -i);
    vec_free(&points); assert(!live_blocks);
    memset(&path, 0xa5, sizeof path);
    path_info_init(&path);
    assert(!path.cells && !path.count && !path.current);
    path.current = 14; path_info_rewind(&path); assert(!path.current);
    path_info_destroy(&path); assert(!live_blocks);
    texture.reqWidth = 320; texture.reqHeight = 200;
    assert(texture_image_width(&texture) == 320 && texture_image_height(&texture) == 200);
    assert(!texture_image_width(NULL) && !texture_image_height(NULL));
    puts("Navigation: explicit guide result, owned copies, typed point growth, shared path state and texture queries passed");
    return 0;
}
