#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "render/texture_types.h"
#include "resources/sprite.h"
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/character/character.h"
#include "ui/render/render.h"
#include <stdlib.h>
#include <string.h>
#include "platform/clock.h"

static void fill_clipped(struct UIWindow *self, int left, int top, int right, int bottom, COLORREF colour)
{
    RECT rc;

    rc.left = left > 0 ? left : 0;
    rc.top = top > 0 ? top : 0;
    rc.right = right < self->width ? right : self->width;
    rc.bottom = bottom < self->height ? bottom : self->height;
    dc_bitmap_fill(self->canvas, &rc, colour);
}

void ui_window_fill_rect(struct UIWindow *self, int x, int y, int width, int height,
                                  COLORREF colour)
{
    fill_clipped(self, x, y, x + width, y + height, colour);
}

void ui_window_draw_outline(struct UIWindow *self, int x, int y, int width, int height,
                                  COLORREF colour)
{
    int right = x + width, bottom = y + height;

    fill_clipped(self, x, y, right, y + 1, colour);
    fill_clipped(self, x, y, x + 1, bottom, colour);
    fill_clipped(self, right - 1, y, right, bottom, colour);
    fill_clipped(self, x, bottom - 1, right, bottom, colour);
}

void ui_window_draw_round_frame(struct UIWindow *self, int x, int y, int width,
                                        int height, COLORREF colour)
{
    int right = x + width, bottom = y + height;

    fill_clipped(self, x + 3, y, right - 3, y + 1, colour);
    fill_clipped(self, x + 3, bottom - 1, right - 3, bottom, colour);
    fill_clipped(self, x, y + 3, x + 1, bottom - 3, colour);
    fill_clipped(self, right - 1, y + 3, right, bottom - 3, colour);
    fill_clipped(self, x + 1, y + 1, x + 3, y + 2, colour);
    fill_clipped(self, x + 1, bottom - 2, x + 3, bottom - 1, colour);
    fill_clipped(self, right - 3, y + 1, right - 1, y + 2, colour);
    fill_clipped(self, right - 3, bottom - 2, right - 1, bottom - 1, colour);
    fill_clipped(self, x + 1, y + 2, x + 2, y + 3, colour);
    fill_clipped(self, x + 1, bottom - 3, x + 2, bottom - 2, colour);
    fill_clipped(self, right - 2, y + 2, right - 1, y + 3, colour);
    fill_clipped(self, right - 2, bottom - 3, right - 1, bottom - 2, colour);
}

void ui_window_fill_round_rect(struct UIWindow *self, int x, int y, int width,
                                       int height, COLORREF colour)
{
    fill_clipped(self, x + 1, y + 1, x + 1 - 2 + width, y + 1 - 2 + height, colour);
    ui_window_draw_round_frame(self, x, y, width, height, colour);
}

void ui_window_fill_triangle(struct UIWindow *self, int x1, int y1, int x2, int y2,
                                      int x3, int y3, COLORREF colour)
{
    POINT pts[3];

    pts[0].x = x1;
    pts[0].y = y1;
    pts[1].x = x2;
    pts[1].y = y2;
    pts[2].x = x3;
    pts[2].y = y3;
    dc_bitmap_fill_triangle(self->canvas, pts, colour);
}

void ui_window_draw_bitmap(struct UIWindow *self, int x, int y, struct BitmapRes *bitmap,
                                    int keep_alpha)
{
    struct DcBitmap *canvas = self->canvas;

    if (!canvas || !bitmap)
        return;
    canvas->singleTexture = bitmap->hasAlpha != 0;
    dc_bitmap_update(self->canvas, x, y, bitmap->width, bitmap->height, bitmap->pixels, keep_alpha);
}

void ui_window_draw_bitmap_tinted(struct UIWindow *self, int tint, int x, int y,
                                          struct BitmapRes *bitmap, int keep_alpha, BYTE remake)
{
    if (!self->canvas || !bitmap)
        return;
    if (remake)
        bitmap_res_free_gray_copy(bitmap);
    bitmap_res_make_gray_copy(bitmap, tint);
    if (bitmap->grayPixels)
        dc_bitmap_update(self->canvas, x, y, bitmap->width, bitmap->height, bitmap->grayPixels,
                            keep_alpha);
}

#define CLIP_ZOOM_X(clip) ((clip)->zoomX)
#define CLIP_ZOOM_Y(clip) ((clip)->zoomY)
#define CLIP_MIRRORED(clip) ((clip)->flags & 1)

void ui_window_draw_spr_clip(struct UIWindow *self, int x, int y, struct SprImg *img, struct SprClip *clip, float scale,
                                     float angle, DWORD colour, DWORD *palette, float zoom2)
{
    struct SpriteRegion place = {0};
    struct TexDrawPrim *quad;
    float u0, v0, u1, v1;
    float left, right, top, bottom;
    float x0, y0, x1, y1, x2, y2, x3, y3;
    void *tex;
    int i;

    if (!img || !clip)
        return;
    x += self->x;
    y += self->y;
    memset(&place, 0, sizeof place);
    left = (float)clip->x * scale * CLIP_ZOOM_X(clip);
    right = (float)((img->widthHalved + 1) * img->width + clip->x - 1) * scale * CLIP_ZOOM_X(clip) * zoom2;
    top = (float)clip->y * scale * CLIP_ZOOM_Y(clip);
    bottom = (float)((img->heightHalved + 1) * img->height + clip->y - 1) * scale * CLIP_ZOOM_Y(clip) * zoom2;
    if (clip->clipType == 0) {
        tex = sprite_atlas_resolve(g_renderer, img, palette, &place);
        u0 = place.u;
        v0 = place.v;
        u1 = place.u1;
        v1 = place.v1;
    } else {
        struct Texture *t = img->texture;

        if (!t)
            return;
        tex = t;
        u0 = 0.0f;
        v0 = 0.0f;
        u1 = (float)img->width / (float)t->base.width;
        v1 = (float)img->height / (float)t->base.height;
    }
    if (!tex)
        return;
    quad = renderer_new_prim(g_renderer);
    quad->base.texture = tex;

    if (angle != 0.0f) {
        int deg = (int)angle;
        float c = get_cos_deg(deg), s = get_sin_deg(deg);
        float fx = (float)x, fy = (float)y;
        float l, r, t, b, hw, hh, cx, cy;
        int far_x = (img->widthHalved + 1) * img->width + clip->x;

        t = (float)clip->y * scale + fy;
        b = (float)((img->heightHalved + 1) * img->height + clip->y) * scale + fy + 1.0f;
        if (CLIP_MIRRORED(clip)) {
            r = (float)clip->x * scale + fx + 1.0f;
            l = (float)far_x * scale + fx;
        } else {
            r = (float)far_x * scale + fx + 1.0f;
            l = (float)clip->x * scale + fx;
        }
        hh = (b - t) * 0.5f;
        hw = (r - l) * 0.5f;
        cy = hh + t;
        cx = l + hw;
        x0 = cx - (hw * c - hh * s);
        y0 = cy - (hh * c + hw * s);
        x1 = hw * c + hh * s + cx;
        y1 = hw * s - hh * c + cy;
        x2 = cx - (hh * s + hw * c);
        y2 = hh * c - hw * s + cy;
        x3 = hw * c - hh * s + cx;
        y3 = hh * c + hw * s + cy;
    } else {
        float a = left, z = right;

        if (CLIP_MIRRORED(clip)) {
            a = right;
            z = left;
        }
        x0 = x2 = a + (float)x;
        x1 = x3 = z + (float)x;
        y0 = y1 = (float)y + top;
        y2 = y3 = (float)y + bottom;
    }
    quad->quad[0].x = x0 + 0.5f;
    quad->quad[0].y = y0 + 0.5f;
    quad->quad[1].x = x1 + 0.5f;
    quad->quad[1].y = y1 + 0.5f;
    quad->quad[2].x = x2 + 0.5f;
    quad->quad[2].y = y2 + 0.5f;
    quad->quad[3].x = x3 + 0.5f;
    quad->quad[3].y = y3 + 0.5f;
    for (i = 0; i < 4; i++) {
        *(DWORD *)&quad->quad[i].z = UI_QUAD_Z_BITS;
        *(DWORD *)&quad->quad[i].rhw = UI_QUAD_RHW_BITS;
        quad->quad[i].diffuse = colour;
    }
    quad->quad[0].u = u0;
    quad->quad[0].v = v0;
    quad->quad[1].u = u1;
    quad->quad[1].v = v0;
    quad->quad[2].u = u0;
    quad->quad[2].v = v1;
    quad->quad[3].u = u1;
    quad->quad[3].v = v1;
    vec_push(&self->quads, &quad, sizeof quad);
}

struct MotionBox {
    int left, top, right, bottom;
};

static struct SprImg *clip_image(struct SprRes *spr, const struct SprClip *clip)
{
    return clip ? sprite_resource_image(spr, clip->clipType, clip->sprIndex) : NULL;
}

static int motion_box(struct Motion *motion, struct SprRes *spr, struct MotionBox *box)
{
    struct SprClip *clip = motion_get_clip(motion, 0);
    struct SprImg *img;
    int i;

    if (!clip || clip->sprIndex == -1)
        return 0;
    img = clip_image(spr, clip);
    if (!img)
        return 0;
    box->left = (int)((float)clip->x * CLIP_ZOOM_X(clip));
    box->top = (int)((float)clip->y * CLIP_ZOOM_Y(clip));
    box->right = (int)((float)img->width * CLIP_ZOOM_X(clip)) + box->left;
    box->bottom = (int)((float)img->height * CLIP_ZOOM_Y(clip)) + box->top;
    for (i = 1; i < motion->clip_count; i++) {
        int cx, cy, w, h;

        clip = motion_get_clip(motion, i);
        if (!clip || clip->sprIndex == -1)
            continue;
        img = clip_image(spr, clip);
        if (!img)
            continue;
        cx = (int)((float)clip->x * CLIP_ZOOM_X(clip));
        w = (int)((float)img->width * CLIP_ZOOM_X(clip));
        cy = (int)((float)clip->y * CLIP_ZOOM_Y(clip));
        h = (int)((float)img->height * CLIP_ZOOM_Y(clip));
        if (box->left > cx)
            box->left = cx;
        if (box->right < cx + w)
            box->right = cx + w;
        if (box->top > cy)
            box->top = cy;
        if (box->bottom < cy + h)
            box->bottom = cy + h;
    }
    return 1;
}

static float fit_scale(int bw, int bh, int width, int height, BYTE *by_width)
{
    *by_width = 1;
    if (bw < bh) {
        *by_width = 0;
        return height < bh ? (float)height / (float)bh : 1.0f;
    }
    return width < bw ? (float)width / (float)bw : 1.0f;
}

void ui_window_draw_actor_sprite(struct UIWindow *self, void *spr, void *act, int x,
                                         int y, int width, int height, int period, char scale_down,
                                         void *backdrop)
{
    struct SpriteAction *motions;
    struct Motion *first;
    struct MotionBox box;
    struct BitmapRes *bg = backdrop;
    int bw, bh, ox, oy, count;
    unsigned frame;
    float scale, draw_scale;
    BYTE by_width;

    if (!spr || !act)
        return;
    motions = act_res_get_action(act, 0);
    if (!motions || !motions->motion_count || !width || !height)
        return;
    first = motions->motions;
    if (!first || !motion_box(first, spr, &box))
        return;
    ox = -box.left;
    oy = -box.top;
    bw = abs(box.left - box.right);
    bh = abs(box.top - box.bottom);
    scale = fit_scale(bw, bh, width, height, &by_width);
    draw_scale = scale;
    if ((double)scale == 1.0) {
        ox -= (int)((float)(width - bw) * -0.5f);
        oy -= (int)((float)(height - bh) * -0.5f);
    } else {
        ox = (int)((float)ox * scale);
        oy = (int)((float)oy * scale);
        if (by_width)
            oy += (int)(((float)height - (float)bh * scale) * 0.5f);
        else
            ox += (int)(((float)width - (float)bw * scale) * 0.5f);
    }
    if (!scale_down)
        draw_scale = 1.0f;
    if (bg)
        dc_bitmap_update(self->canvas, ox - bg->width / 2 + x, oy - bg->height / 2 + y, bg->width, bg->height,
                            bg->pixels, 1);

    count = (int)motions->motion_count;
    if (period <= 0) {
        frame = 0;
    } else {
        unsigned t = platform_ticks_ms() % (unsigned)period;
        int i = (int)((float)t * ((float)count / (float)period));

        if (i < 0)
            i = 0;
        frame = count > i ? (unsigned)i : (unsigned)(count - 1);
    }
    count = (int)motions->motion_count;
    if ((unsigned)count > frame)
        dc_bitmap_draw_sprite(self->canvas, x, y, width, height, spr,
                            &motions->motions[frame], ox, oy, draw_scale, NULL);
}

BYTE ui_window_fit_motion(void *motion, void *spr, int width,
                                   int height, int *ox, int *oy, float *scale)
{
    struct MotionBox box;
    int bw, bh, x0, y0;
    BYTE by_width;

    if (!width || !height || !motion || !spr || !ox || !oy || !scale)
        return 0;
    if (!motion_box(motion, spr, &box))
        return 0;
    x0 = -box.left;
    y0 = -box.top;
    bw = abs(box.left - box.right);
    bh = abs(box.top - box.bottom);
    *scale = fit_scale(bw, bh, width, height, &by_width);
    if ((double)*scale == 1.0) {
        *ox = x0 - (int)((float)(width - bw) * -0.5f);
        *oy = y0 - (int)((float)(height - bh) * -0.5f);
        return 1;
    }
    *ox = (int)((float)x0 * *scale);
    *oy = (int)((float)y0 * *scale);
    if (by_width)
        *oy -= (int)(((float)height - (float)bh * *scale) * -0.5f);
    else
        *ox += (int)(((float)width - (float)bw * *scale) * 0.5f);
    return 1;
}
