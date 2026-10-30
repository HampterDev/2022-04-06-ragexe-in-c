#include "world/camera.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int sky_destructions, sky_frees, view_frees;
static void *owned_sky, *owned_view;
void ui_edit_ctrl_wheel_on_wheel(void *obj)
{ assert(obj == owned_sky); ++sky_destructions; }
void mem_free(void *obj)
{
    if (obj == owned_sky) { ++sky_frees; owned_sky = NULL; }
    else { assert(obj == owned_view); ++view_frees; }
    free(obj);
}
int main(void)
{
    struct { unsigned before[4]; struct View view; unsigned after[4]; } guarded;
    struct View *view = &guarded.view;
    struct View expected = {0};
    struct World world = {0};
    int i;
    memset(&guarded, 0xa5, sizeof guarded);
    assert(view_construct(view) == view);
    assert(memcmp(view, &expected, sizeof *view) == 0);
    view->world = &world;
    view_reset(view);
    assert(view->world == &world);
    assert(view->latitude == -45 && view->destLatitude == -45);
    assert(view->longitude == 0 && view->destLongitude == 0);
    assert(view->distance == 300 && view->destDistance == 300);
    assert(view->up[0] == 0 && view->up[1] == 1 && view->up[2] == 0);
    owned_sky = malloc(0x32c); assert(owned_sky); view->sky = owned_sky;
    view->frustum.inside_count = 1; view->frustum.inside[0] = &world.root;
    view->frustum.crossing_count = 1; view->frustum.crossing[0] = &world.root;
    view_destruct(view);
    assert(!view->sky && !view->frustum.inside_count && !view->frustum.crossing_count);
    assert(sky_destructions == 1 && sky_frees == 1 && !view_frees);
    view_destruct(view);
    assert(sky_destructions == 1 && sky_frees == 1 && view->world == &world);
    for (i = 0; i < 4; ++i) assert(guarded.before[i] == 0xa5a5a5a5 && guarded.after[i] == 0xa5a5a5a5);
    owned_view = malloc(sizeof *view); assert(owned_view);
    view = view_construct(owned_view); view_reset(view);
    view_delete(view); assert(view_frees == 1);
    puts("camera lifecycle: complete initialization, shared fields, reset defaults, borrowed world, repeat cleanup and complete allocation size passed");
    return 0;
}
