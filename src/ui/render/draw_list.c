#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "render/texture_types.h"
#include "ui/render/render.h"
#include "render/render_api.h"

static void quad_set_depth(struct TexDrawVertex *v)
{
    *(DWORD *)&v->z = UI_QUAD_Z_BITS;
    *(DWORD *)&v->rhw = UI_QUAD_RHW_BITS;
}

static void quad_finish(struct TexDrawPrim *quad, DWORD argb)
{
    int i;

    for (i = 0; i < 4; i++) {
        quad->quad[i].diffuse = argb;
        quad_set_depth(&quad->quad[i]);
    }
}

static void quad_place(struct TexDrawPrim *quad, float x0, float y0, float x1, float y1)
{
    quad->quad[0].x = x0;
    quad->quad[0].y = y0;
    quad->quad[1].x = x1;
    quad->quad[1].y = y0;
    quad->quad[2].x = x0;
    quad->quad[2].y = y1;
    quad->quad[3].x = x1;
    quad->quad[3].y = y1;
}

static void quad_map(struct TexDrawPrim *quad, float u0, float v0, float u1, float v1)
{
    quad->quad[0].u = u0;
    quad->quad[0].v = v0;
    quad->quad[1].u = u1;
    quad->quad[1].v = v0;
    quad->quad[2].u = u0;
    quad->quad[2].v = v1;
    quad->quad[3].u = u1;
    quad->quad[3].v = v1;
}

static struct TexDrawPrim *borrow_quad(void)
{
    return renderer_new_prim(g_renderer);
}

void ui_window_queue_bitmap(struct UIWindow *self, int x, int y, struct String name,
                                     DWORD argb, int width, int height)
{
    struct Texture *tex = texture_cache_get(&g_textureCache, string_cstr(&name), 0, 1);

    if (tex) {
        struct TexDrawPrim *quad = borrow_quad();
        int image_w = ((unsigned)tex->reqWidth), image_h = ((unsigned)tex->reqHeight);
        float tw, th, u1, v1;

        quad->base.texture = tex;
        if (!width)
            width = image_w;
        if (!height)
            height = image_h;
        tw = (float)tex->base.width;
        th = (float)tex->base.height;
        quad_place(quad, (float)x, (float)y, (float)(x + width), (float)(y + height));
        u1 = ((float)(width > image_w ? image_w : width) + g_texelOffsetU) / tw;
        if (height > image_h)
            v1 = ((float)image_h + g_texelOffsetV) / th;
        else
            v1 = ((float)height + g_texelOffsetU) / th;
        quad_map(quad, g_texelOffsetU / tw, g_texelOffsetV / th, u1, v1);
        quad_finish(quad, argb);
        list_push_back(&self->draws, &quad, sizeof quad);
    }
    string_free(&name);
}

void ui_window_reset_draws(struct UIWindow *self, int width, int height, DWORD argb)
{
    struct TexDrawPrim *quad;

    list_clear(&self->draws, NULL);
    quad = borrow_quad();
    quad->base.texture = NULL;
    quad_place(quad, 0.0f, 0.0f, (float)width, (float)height);
    quad_map(quad, 0.0f, 0.0f, 0.0f, 0.0f);
    quad_finish(quad, argb);
    list_push_back(&self->draws, &quad, sizeof quad);
}

void *ui_list_iterator_self(void *self)
{
    return self;
}

void ui_empty_method(void *self)
{
}
