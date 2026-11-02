#include "render/render_api.h"
#include "render/texture_cache.h"
#include "render/sprite_atlas.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "render/cdc.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>

static struct SprImg *spr_image(struct SprRes *spr, const struct SprClip *clip)
{
    return clip ? sprite_resource_image(spr, clip->clipType, clip->sprIndex) : NULL;
}

static void delete_textures(struct DcBitmap *self)
{
    size_t i;
    for (i = 0; i < self->texture_count; ++i)
        texture_destroy(self->textures[i]);
    free(self->textures);
    self->textures = NULL;
    self->texture_count = 0;
}

static void opaque(struct DcBitmap *self)
{
    int n = self->width * self->height * 4, i;

    for (i = 0; i < n; i += 4)
        ((BYTE *)self->bits)[i + 3] = 0xff;
}

struct DcBitmap *dc_bitmap_construct(struct DcBitmap *self, int w, int h)
{
    memset(self, 0, sizeof *self);
    self->hdc = CreateCompatibleDC(NULL);
    if (!self->hdc)
        return NULL;
    dc_bitmap_alloc(self, w, h);
    if (!self->dib) {
        DeleteDC(self->hdc);
        self->hdc = NULL;
        return NULL;
    }
    return self;
}

struct DcBitmap *dc_bitmap_create(int w, int h)
{
    struct DcBitmap *bitmap = calloc(1, sizeof *bitmap);
    if (!bitmap) return NULL;
    if (!dc_bitmap_construct(bitmap, w, h)) {
        free(bitmap);
        return NULL;
    }
    dc_bitmap_fill(bitmap, NULL, 0xffff00ff);
    return bitmap;
}

void dc_bitmap_destruct(struct DcBitmap *self)
{
    delete_textures(self);
    if (self->hdc) {
        if (self->oldBitmap) SelectObject(self->hdc, self->oldBitmap);
        if (self->dib) DeleteObject(self->dib);
        DeleteDC(self->hdc);
    }
    self->hdc = NULL;
    self->dib = NULL;
    self->oldBitmap = NULL;
    self->bits = NULL;
    self->width = self->height = 0;
    self->changed = 0;
}

void dc_bitmap_destroy(struct DcBitmap *self)
{
    if (!self) return;
    dc_bitmap_destruct(self);
    free(self);
}

void dc_bitmap_alloc(struct DcBitmap *self, int w, int h)
{
    BITMAPINFO info = {0};
    HBITMAP dib;
    HGDIOBJ previous;
    DWORD *bits;
    if (!self->hdc || w <= 0 || h <= 0 || (size_t)w > (size_t)INT_MAX / 4 / (size_t)h)
        return;
    info.bmiHeader.biSize = sizeof info.bmiHeader;
    info.bmiHeader.biWidth = w;
    info.bmiHeader.biHeight = -h;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biXPelsPerMeter = info.bmiHeader.biYPelsPerMeter = 100;
    dib = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
    if (!dib) return;
    previous = SelectObject(self->hdc, dib);
    if (!previous || previous == HGDI_ERROR) {
        DeleteObject(dib);
        return;
    }
    delete_textures(self);
    if (self->dib) DeleteObject(self->dib);
    else self->oldBitmap = previous;
    self->dib = dib;
    self->bits = bits;
    self->width = w;
    self->height = h;
    self->changed = 1;
}

char dc_bitmap_get_dc(struct DcBitmap *self, HDC *out)
{
    *out = NULL;
    if (!self || !self->hdc) return 0;
    self->changed = 1;
    *out = self->hdc;
    return 1;
}

void dc_bitmap_update(struct DcBitmap *self, int x, int y, int w, int h, const DWORD *src, char keyed)
{
    int total, row, col;

    if (!self->bits || !src)
        return;
    total = self->height * self->width;
    self->changed = 1;
    for (row = 0; row < h; row++) {
        for (col = 0; col < w; col++) {
            DWORD p = src[col];
            int i;

            if (keyed && p == CDC_COLOR_KEY)
                continue;
            i = (row + y) * self->width + col + x;
            if (i > -1 && total > i)
                self->bits[i] = p;
        }
        src += w;
    }
}

void dc_bitmap_copy_bitmap(struct DcBitmap *self, unsigned dx, unsigned dy, unsigned w, unsigned h,
                                      struct DcBitmap *src)
{
    DWORD *d, *s;
    int row, k, srcW;

    if (!self->bits || dx >= (unsigned)self->width || dy >= (unsigned)self->height)
        return;
    if (dx + w > (unsigned)self->width)
        w = self->width - dx;
    if (dy + h > (unsigned)self->height)
        h = self->height - dy;
    self->changed = 1;
    d = self->bits + (self->width * dy + dx);
    srcW = src->width;
    s = src->bits;
    for (row = 0; row < (int)h; row++) {
        if (!d || !s)
            return;
        for (k = 0; k < (int)w; k++) {
            if (src->singleTexture == 1) {
                DWORD D = d[k], S = s[k];
                unsigned sa = S >> 24, da = D >> 24, inv = 0xff - (WORD)sa;
                unsigned r, g, b, a;

                r = (BYTE)((int)(((D >> 16) & 0xff) * inv) / 0xff + ((S >> 16) & 0xff) * (WORD)sa / 0xff);
                a = (WORD)sa > (WORD)da ? (WORD)sa : (WORD)da;
                g = (BYTE)((int)(((WORD)D >> 8) * inv) / 0xff + ((WORD)S >> 8) * (WORD)sa / 0xff);
                b = (BYTE)((int)((BYTE)D * inv) / 0xff + (BYTE)S * (WORD)sa / 0xff);
                d[k] = (((((a << 8) + r) << 8) + g) << 8) + b;
            } else {
                DWORD S = s[k];
                if (S != CDC_COLOR_KEY)
                    d[k] = S + 0xff000000u;
            }
        }
        s += srcW;
        d += self->width;
    }
}

void dc_bitmap_fill(struct DcBitmap *self, const RECT *r, DWORD colour)
{
    DWORD c;
    unsigned ux, uy;
    int x, y;

    if (!self->bits)
        return;
    c = ((colour >> 16) & 0xff) + ((colour & 0xff) << 16) + (colour & 0xff000000u) + (colour & 0xff00);
    self->changed = 1;
    if (r) {
        for (y = r->top; y < r->bottom; y++)
            for (x = r->left; x < r->right; x++)
                self->bits[self->width * y + x] = c;
        return;
    }
    for (uy = 0; uy < (unsigned)self->height; uy++)
        for (ux = 0; ux < (unsigned)self->width; ux++)
            self->bits[self->width * uy + ux] = c;
}

void dc_bitmap_upload(struct DcBitmap *self)
{
    size_t columns, rows, count, i;
    int x, y, w, h;
    if (!self->changed || !self->bits || self->width <= 0 || self->height <= 0)
        return;
    columns = self->singleTexture == 1 ? 1 : ((size_t)self->width + CDC_TILE - 1) / CDC_TILE;
    rows = self->singleTexture == 1 ? 1 : ((size_t)self->height + CDC_TILE - 1) / CDC_TILE;
    count = columns * rows;
    if (self->texture_count != count) {
        struct Texture **textures = calloc(count, sizeof *textures);
        if (!textures) return;
        for (i = 0; i < count; ++i) {
            x = self->singleTexture == 1 ? 0 : (int)(i % columns) * CDC_TILE;
            y = self->singleTexture == 1 ? 0 : (int)(i / columns) * CDC_TILE;
            w = self->width - x; h = self->height - y;
            if (self->singleTexture != 1) {
                if (w > CDC_TILE) w = CDC_TILE;
                if (h > CDC_TILE) h = CDC_TILE;
            }
            textures[i] = texture_create(w, h, self->singleTexture == 1 ? 4 : 0);
            if (!textures[i]) {
                while (i) texture_destroy(textures[--i]);
                free(textures);
                return;
            }
        }
        delete_textures(self);
        self->textures = textures;
        self->texture_count = count;
    }
    if (self->singleTexture != 1) opaque(self);
    for (i = 0; i < count; ++i) {
        x = self->singleTexture == 1 ? 0 : (int)(i % columns) * CDC_TILE;
        y = self->singleTexture == 1 ? 0 : (int)(i / columns) * CDC_TILE;
        w = self->width - x; h = self->height - y;
        if (self->singleTexture != 1) {
            if (w > CDC_TILE) w = CDC_TILE;
            if (h > CDC_TILE) h = CDC_TILE;
        }
        if (!texture_upload(self->textures[i], 0, 0, w, h,
                            self->bits + self->width * y + x, 0, self->width))
            return;
    }
    self->changed = 0;
}

void dc_bitmap_draw(struct DcBitmap *self, int x, int y, int w, int h, DWORD colour)
{
    size_t columns, i;
    int tx, ty, tw, th;
    if (!self->bits || w <= 0 || h <= 0) return;
    dc_bitmap_upload(self);
    if (!self->texture_count || self->changed) return;
    if (self->singleTexture == 1) {
        texture_draw_surface(self->textures[0], x, y, w, h, colour);
        return;
    }
    if (w > self->width) w = self->width;
    if (h > self->height) h = self->height;
    columns = ((size_t)self->width + CDC_TILE - 1) / CDC_TILE;
    for (ty = 0; ty < h; ty += CDC_TILE)
        for (tx = 0; tx < w; tx += CDC_TILE) {
            i = (size_t)(ty / CDC_TILE) * columns + (size_t)(tx / CDC_TILE);
            tw = w - tx > CDC_TILE ? CDC_TILE : w - tx;
            th = h - ty > CDC_TILE ? CDC_TILE : h - ty;
            texture_draw_surface(self->textures[i], x + tx, y + ty, tw, th, colour);
        }
}

void dc_bitmap_draw_sprite(struct DcBitmap *self, int a, int b, int c, int d, void *sprv, void *motionv,
                                      int x, int y, float scale, DWORD *palette)
{
    struct SprRes *spr = (struct SprRes *)sprv;
    struct Motion *motion = (struct Motion *)motionv;
    struct SpriteRegion place = {0};
    size_t i;

    if (!spr || !motion)
        return;
    if (!palette)
        palette = spr->palette;
    memset(&place, 0, sizeof(place));
    for (i = 0; i < motion->clip_count; i++) {
        struct SprClip *clip = motion_get_clip(motion, i);
        struct SprImg *img;
        struct Texture *tex;
        float zx, zy;
        DWORD colour;

        if (!clip || clip->sprIndex == -1)
            continue;
        img = spr_image(spr, clip);
        if (!img)
            continue;
        zx = clip->zoomX * scale;
        zy = clip->zoomY * scale;
        place.u = 0.0f;
        place.v = 0.0f;
        if (clip->clipType == 0) {
            tex = sprite_atlas_resolve(g_renderer, img, palette, &place);
        } else {
            tex = img->texture;
            place.x = place.y = 0;
        }
        if (!tex)
            continue;
        colour = (((((((DWORD)clip->color[3] << 8) + clip->color[0]) << 8) + clip->color[1]) << 8) + clip->color[2]);
        dc_bitmap_draw_texels_scaled(self, a, b, c, d, tex, img->width, img->height, zx, zy, colour,
                         clip->flags != 0, (int)((float)clip->x * zx + (float)x),
                         (int)((float)clip->y * zy + (float)y), place.x,
                         place.y);
    }
}

void dc_bitmap_brighten(struct DcBitmap *self, int x0, int y0, int w, int h, int delta)
{
    unsigned xe, ye;
    int x, yy;

    if (x0 < 0 || y0 < 0 || (unsigned)self->width < (unsigned)x0 || (unsigned)self->height < (unsigned)y0
        || w < 1 || h < 1)
        return;
    self->changed = 1;
    xe = (unsigned)(w + x0) < (unsigned)self->width ? (unsigned)(w + x0) : (unsigned)self->width;
    ye = (unsigned)(h + y0) < (unsigned)self->height ? (unsigned)(h + y0) : (unsigned)self->height;
    for (yy = y0; yy < (int)ye; yy++)
        for (x = x0; x < (int)xe; x++) {
            DWORD *p = &self->bits[self->width * yy + x];
            DWORD v = *p;
            int r, g, b;

            if (v == CDC_COLOR_KEY)
                continue;
            r = (int)(((v >> 16) & 0xff) + (unsigned)delta);
            r = r < 0 ? 0 : r;
            r = r > 0xff ? 0xff : r;
            g = (int)(((v >> 8) & 0xff) + (unsigned)delta);
            g = g < 0 ? 0 : g;
            g = g > 0xff ? 0xff : g;
            b = (int)((v & 0xff) + (unsigned)delta);
            b = b < 0 ? 0 : b;
            b = b > 0xff ? 0xff : b;
            *p = ((((unsigned)r | 0xffffff00u) << 8 | (BYTE)g) << 8) | (BYTE)b;
        }
}

void dc_bitmap_copy_rect(struct DcBitmap *self, int dx, int dy, int sx, int sy, int w, int h,
                                    struct DcBitmap *src)
{
    DWORD *d, *s;
    int row, k;

    if (!self->bits)
        return;
    self->changed = 1;
    d = self->bits + (self->width * dy + dx);
    s = src->bits + (src->width * sy + sx);
    for (row = 0; row < h; row++) {
        if (!d || !s)
            return;
        for (k = 0; k < w; k++)
            if (s[k] != CDC_COLOR_KEY)
                d[k] = s[k];
        s += src->width;
        d += self->width;
    }
}

void dc_bitmap_to_texture(struct DcBitmap *self, struct Texture **tex)
{
    opaque(self);
    if (!*tex) {
        *tex = texture_from_pixels(self->width, self->height, self->bits, 0, 0, 1);
        return;
    }
    texture_upload_pixels(*tex, 0, 0, self->width, self->height, self->bits, 0, 0);
}
