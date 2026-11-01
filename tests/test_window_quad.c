#include <assert.h>
#include <stdio.h>
#include "render/cdc.h"
#include "ui/render/render.h"

static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
float g_texelOffsetU, g_texelOffsetV;
static struct Texture texture;
static struct TexDrawPrim quad;
static unsigned uploads, allocations;

void dc_bitmap_to_texture(struct DcBitmap *canvas, struct Texture **out)
{
    assert(canvas);
    ++uploads;
    *out = &texture;
}

struct TexDrawPrim *renderer_new_prim(struct Renderer *self)
{
    assert(self == &renderer);
    ++allocations;
    return &quad;
}

DWORD ui_window_get_color(struct UIWindow *window)
{
    (void)window;
    return 0xffabcdef;
}

int main(void)
{
    struct UIWindow window = {0};
    struct DcBitmap canvas = {0};
    assert(ui_window_make_quad_base(&window) == NULL);
    assert(!uploads && !allocations);
    window.width = 20;
    window.height = 10;
    assert(ui_window_make_quad_base(&window) == NULL);
    assert(!uploads && !allocations);
    window.canvas = &canvas;
    texture.base.width = 32;
    texture.base.height = 16;
    assert(ui_window_make_quad_base(&window) == &quad);
    assert(uploads == 1 && allocations == 1);
    assert(quad.base.texture == &texture);
    assert(quad.quad[3].x == 20 && quad.quad[3].y == 10);
    assert(quad.quad[3].u == 0.625f && quad.quad[3].v == 0.625f);
    assert(quad.quad[0].diffuse == 0xffabcdef);
    window.composites = 1;
    window.canvas = NULL;
    assert(ui_window_make_quad_base(&window) == &quad);
    assert(uploads == 1 && allocations == 2);
    puts("Window quads: absent canvases draw nothing; bitmap and composited windows retain their geometry and color passed.");
    return 0;
}
