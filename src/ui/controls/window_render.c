#include "render/rp_pool.h"
#include "render/texture_types.h"
#include "ui/window.h"
#include "ui/render/render.h"

#include "render/cdc.h"
#include "render/render_api.h"

struct TexDrawPrim *ui_window_make_quad_base(struct UIWindow *self)
{
    struct TexDrawPrim *quad;
    struct Texture *tex;
    float tw, th;
    int i;

    if (self->composites == 0) {
        if (!self->canvas)
            return NULL;
        dc_bitmap_to_texture(self->canvas, &self->texture);
    }
    quad = renderer_new_prim(g_renderer);
    tex = self->texture;
    quad->base.texture = tex;
    tw = (float)tex->base.width;
    th = (float)tex->base.height;
    quad->quad[0].x = (float)self->x;
    quad->quad[0].y = (float)self->y;
    quad->quad[0].u = g_texelOffsetU / tw;
    quad->quad[0].v = g_texelOffsetV / th;
    quad->quad[1].x = (float)(self->width + self->x);
    quad->quad[1].y = (float)self->y;
    quad->quad[1].u = ((float)self->width + g_texelOffsetU) / tw;
    quad->quad[1].v = g_texelOffsetV / th;
    quad->quad[2].x = (float)self->x;
    quad->quad[2].y = (float)(self->y + self->height);
    quad->quad[2].u = g_texelOffsetU / tw;
    quad->quad[2].v = ((float)self->height + g_texelOffsetV) / th;
    quad->quad[3].x = (float)(self->width + self->x);
    quad->quad[3].y = (float)(self->y + self->height);
    quad->quad[3].u = ((float)self->width + g_texelOffsetU) / tw;
    quad->quad[3].v = ((float)self->height + g_texelOffsetV) / th;
    for (i = 0; i < 4; i++) {
        *(DWORD *)&quad->quad[i].z = UI_QUAD_Z_BITS;
        *(DWORD *)&quad->quad[i].rhw = UI_QUAD_RHW_BITS;
        quad->quad[i].diffuse = ui_window_get_color(self);
    }
    return quad;
}
