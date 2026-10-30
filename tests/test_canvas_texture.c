#include <assert.h>
#include <stdio.h>
#include "render/texture.h"
#include "ui/draw.h"

static struct Texture textures[4];
static int canvas_draws;

void dc_bitmap_draw_texels(struct DcBitmap *canvas, int x, int y, struct Texture *texture,
                        int sx, int sy, int w, int h, int mirror, int cw, int ch)
{
    assert(canvas && texture == &textures[0]);
    assert(x == 2 && y == 3 && sx == 4 && sy == 5 && w == 6 && h == 7);
    assert(mirror == 1 && cw == 2 && ch == 1);
    ++canvas_draws;
}

int main(void)
{
    struct DcBitmap canvas = {0};
    ui_canvas_draw_texture(NULL, 2, 3, &textures[0], 4, 5, 6, 7, 1, 2, 1);
    ui_canvas_draw_texture(&canvas, 2, 3, NULL, 4, 5, 6, 7, 1, 2, 1);
    assert(!canvas_draws);
    ui_canvas_draw_texture(&canvas, 2, 3, &textures[0], 4, 5, 6, 7, 1, 2, 1);
    assert(canvas_draws == 1);
    puts("Canvas texture drawing: null inputs, clipping, mirroring and dimensions passed");
    return 0;
}
