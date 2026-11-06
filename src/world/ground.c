#include "render/texture_cache.h"
#include "world/ground.h"
#include "world/camera.h"

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

static float unit(float v)
{
    return 0.0f > v ? 0.0f : v > 1.0f ? 1.0f : v;
}

int gnd_lightmap_set_build(struct GndLightmapSet *self, const struct GndRes *gnd)
{
    DWORD pixels[256 * 256];
    BYTE rec[0x100];
    size_t base, i;

    gnd_lightmap_set_reset(self);
    if (gnd->lightmapCount < 0 || (size_t)gnd->lightmapCount > SIZE_MAX / sizeof *self->lightmaps)
        return 0;
    self->count = (size_t)gnd->lightmapCount;
    if (!self->count)
        return 1;
    self->pageCount = self->count / 1024 + (self->count % 1024 != 0);
    self->lightmaps = calloc(self->count, sizeof *self->lightmaps);
    self->pages = calloc(self->pageCount, sizeof *self->pages);
    if (!self->lightmaps || !self->pages)
        goto fail;

    for (base = 0; base < self->count; base += 0x400) {
        size_t page = base / 0x400;
        struct Texture *tex;
        int j;

        memset(pixels, 0, sizeof pixels);
        for (j = 0; j < 0x400; j++) {
            struct GndLightmap *lm;
            int local, col, row, r;
            DWORD *px;

            if (j + base >= self->count)
                break;
            gnd_res_get_lightmap(gnd, (int)(j + base), rec);
            lm = self->lightmaps + base + j;
            local = j;
            col = local % 32;
            row = local / 32;
            px = &pixels[(row * 256 + col) * 8];
            for (r = 0; r < 8; r++) {
                int k;

                for (k = 0; k < 8; k++) {
                    const BYTE *rgb = &rec[0x40 + (r * 8 + k) * 3];
                    px[k] = ((((((DWORD)rec[r * 8 + k] << 8) + rgb[0]) << 8) + rgb[1]) << 8) + rgb[2];
                }
                px += 256;
            }

            {
                float u0 = (float)(unsigned)(col * 8 + 1) * 0.00390625f;
                float v0 = (float)(unsigned)(row * 8 + 1) * 0.00390625f;
                float u1 = (float)(unsigned)(col * 8 + 7) * 0.00390625f;
                float v1 = (float)(unsigned)(row * 8 + 7) * 0.00390625f;

                lm->uv[0][0] = unit(u0);
                lm->uv[0][1] = unit(v0);
                lm->uv[1][0] = unit(u1);
                lm->uv[1][1] = unit(v0);
                lm->uv[2][0] = unit(u0);
                lm->uv[2][1] = unit(v1);
                lm->uv[3][0] = unit(u1);
                lm->uv[3][1] = unit(v1);
            }
            lm->centre[0] = rec[0x40 + 36 * 3 + 0];
            lm->centre[1] = rec[0x40 + 36 * 3 + 1];
            lm->centre[2] = rec[0x40 + 36 * 3 + 2];
            lm->quarter[0][3] = rec[9];
            lm->quarter[0][2] = rec[0x40 + 9 * 3 + 0];
            lm->quarter[0][1] = rec[0x40 + 9 * 3 + 1];
            lm->quarter[0][0] = rec[0x40 + 9 * 3 + 2];
            lm->quarter[1][3] = rec[13];
            lm->quarter[1][2] = rec[0x40 + 13 * 3 + 0];
            lm->quarter[1][1] = rec[0x40 + 13 * 3 + 1];
            lm->quarter[1][0] = rec[0x40 + 13 * 3 + 2];
            lm->quarter[2][3] = rec[41];
            lm->quarter[2][2] = rec[0x40 + 41 * 3 + 0];
            lm->quarter[2][1] = rec[0x40 + 41 * 3 + 1];
            lm->quarter[2][0] = rec[0x40 + 41 * 3 + 2];
            lm->quarter[3][3] = rec[45];
            lm->quarter[3][2] = rec[0x40 + 45 * 3 + 0];
            lm->quarter[3][1] = rec[0x40 + 45 * 3 + 1];
            lm->quarter[3][0] = rec[0x40 + 45 * 3 + 2];
        }

        tex = texture_from_pixels(0x100, 0x100, pixels, 4, 0, 0);
        if (!tex)
            goto fail;
        self->pages[page] = tex;
    }
    for (i = 0; i < self->count; i++)
        self->lightmaps[i].page = self->pages[i / 0x400];
    return 1;
fail:
    gnd_lightmap_set_reset(self);
    return 0;
}

void c3d_ground_get_cell_pos(const struct C3dGround15 *ground, float x, float z, int *cx, int *cy)
{
    const struct C3dGround15 *self = ground;

    *cx = (int)(float)floor((double)(x / self->zoom)) + self->width / 2;
    *cy = (int)(float)floor((double)(z / self->zoom)) + self->height / 2;
}

void c3d_ground15_get_cell_colour(struct C3dGround15 *self, float x, float z, int *rgb)
{
    int width = self->width;
    int cx = (int)(float)floor((double)(x / self->zoom)) + width / 2;
    int cy = (int)(float)floor((double)(z / self->zoom)) + self->height / 2;

    if (cx >= 0 && cy >= 0 && cx < width && cy < self->height) {
        const struct GndCell *cell = GND_CELL(self, cx, cy);
        rgb[0] = cell->colour[0];
        rgb[1] = cell->colour[1];
        rgb[2] = cell->colour[2];
        return;
    }
    rgb[1] = 0xff;
    rgb[0] = 0xff;
    rgb[2] = 0xff;
}

static BYTE quad_heights(struct C3dAttr *attr, int ax, int ay, float *quad)
{
    struct AttrCell *c = c3d_attr_get_cell(attr, ax, ay);

    if (!c)
        return 0;
    quad[1] = c->height[0];
    quad[4] = c->height[1];
    quad[7] = c->height[2];
    quad[10] = c->height[3];
    return 1;
}

BYTE c3d_ground15_get_attr_quad(struct C3dGround15 *self, int ax, int ay, float *quad)
{
    int cx, cy;

    if (ax < 0)
        return 0;
    cx = ax / 2;
    if (cx >= self->width)
        return 0;
    if (ay < 0)
        return 0;
    cy = ay / 2;
    if (cy >= self->height)
        return 0;

    quad[1] = 0.0f;
    quad[0] = (float)(cx - self->width / 2) * self->zoom;
    quad[2] = (float)(cy - self->height / 2) * self->zoom;
    quad[4] = 0.0f;
    quad[5] = (float)(cy - self->height / 2) * self->zoom;
    quad[3] = (float)(cx - self->width / 2 + 1) * self->zoom;
    quad[7] = 0.0f;
    quad[8] = (float)(cy - self->height / 2 + 1) * self->zoom;
    quad[6] = (float)(cx - self->width / 2) * self->zoom;
    quad[10] = 0.0f;
    quad[11] = (float)(cy - self->height / 2 + 1) * self->zoom;
    quad[9] = (float)(cx - self->width / 2 + 1) * self->zoom;
    return quad_heights(self->attr, ax, ay, quad);
}

BYTE c3d_ground15_get_attr_quad_half(struct C3dGround15 *self, int ax, int ay, float *quad)
{
    int width, height;
    float half, x0, x1, z0, z1;

    if (ax < 0)
        return 0;
    width = self->width;
    if (2 * width < ax)
        return 0;
    if (ay < 0)
        return 0;
    height = self->height;
    if (2 * height < ay)
        return 0;

    half = self->zoom * 0.5f;
    x0 = (float)(ax - width) * half;
    x1 = (float)(ax - width + 1) * half;
    z0 = (float)(ay - height) * half;
    z1 = (float)(ay - height + 1) * half;
    quad[1] = 0.0f;
    quad[0] = x0;
    quad[2] = z0;
    quad[4] = 0.0f;
    quad[3] = x1;
    quad[5] = z0;
    quad[7] = 0.0f;
    quad[6] = x0;
    quad[8] = z1;
    quad[9] = x1;
    quad[10] = 0.0f;
    quad[11] = z1;
    return quad_heights(self->attr, ax, ay, quad);
}

static void surface_height_range(const struct GndSurface *top, float *min, float *max)
{
    float a = top->v[0].pos[1], b = top->v[1].pos[1], c = top->v[2].pos[1], d = top->v[3].pos[1];

    if (a > *max)
        *max = a;
    if (b > *max)
        *max = b;
    if (c > *max)
        *max = c;
    if (d > *max)
        *max = d;
    if (*min > a)
        *min = a;
    if (*min > b)
        *min = b;
    if (*min > c)
        *min = c;
    if (*min > d)
        *min = d;
}

void c3d_ground15_get_height_min_max(struct C3dGround15 *self, const int *cells,
                                            float *min, float *max)
{
    BYTE outside = 0;
    int x, y;

    if (cells[0] < 0 || cells[0] >= self->width)
        outside = 1;
    if (cells[2] < 0 || cells[2] >= self->width)
        outside = 1;
    if (cells[1] < 0 || cells[1] >= self->height)
        outside = 1;
    if (cells[3] < 0 || cells[3] >= self->height)
        outside = 1;
    if (cells[2] <= cells[0] || cells[3] <= cells[1] || outside) {
        *min = 0.0f;
        *max = 0.0f;
        return;
    }
    *min = 999999.0f;
    *max = -999999.0f;
    for (x = cells[0]; x <= cells[2]; x++)
        for (y = cells[1]; y <= cells[3]; y++) {
            const struct GndSurface *top = GND_CELL(self, x, y)->top;
            if (top)
                surface_height_range(top, min, max);
        }
}

void c3d_ground15_get_cell_height_min_max(struct C3dGround15 *self, int x, int y,
                                                float *min, float *max)
{
    const struct GndSurface *top;

    if (x < 0 || x >= self->width || y < 0 || y >= self->height) {
        *min = 0.0f;
        *max = 0.0f;
        return;
    }
    *min = 999999.0f;
    *max = -999999.0f;
    top = GND_CELL(self, x, y)->top;
    if (top)
        surface_height_range(top, min, max);
}

int c3d_ground15_get_vertex_colour(struct C3dGround15 *self, int ax, int ay,
                                           float *r, float *g, float *b)
{
    if (ax >= 0 && ax / 2 < self->width && ay >= 0 && ay / 2 < self->height
        && self->colours != NULL) {
        const struct GndColour *c = self->colours + (ax + 2 * self->width * ay);
        *r = c->r;
        *g = c->g;
        *b = c->b;
        return 1;
    }
    *r = 1.0f;
    *g = 1.0f;
    *b = 1.0f;
    return 1;
}

static const struct GndLightmap *lightmap_at(struct C3dGround15 *self, int ax, int ay)
{
    struct GndCell *cell;

    if (ax < 0 || ax / 2 >= self->width || ay < 0 || ay / 2 >= self->height
        || self->cells == NULL)
        return NULL;
    cell = GND_CELL(self, ax / 2, ay / 2);
    if (!cell->top)
        return NULL;
    return cell->top->lightmap;
}

void c3d_ground15_get_lightmap_colour(struct C3dGround15 *self, int ax, int ay,
                                              BYTE *c0, BYTE *c1, BYTE *c2, BYTE *c3)
{
    const struct GndLightmap *lm = lightmap_at(self, ax, ay);
    int q;

    if (!lm) {
        *c0 = 0xff;
        *c1 = 0xff;
        *c2 = 0xff;
        *c3 = 0xff;
        return;
    }
    if (ax % 2 == 0 && ay % 2 == 0)
        q = 0;
    else if (ax % 2 == 0 && ay % 2 == 1)
        q = 2;
    else if (ax % 2 == 1 && ay % 2 == 0)
        q = 1;
    else
        q = 3;
    *c0 = lm->quarter[q][3];
    *c1 = lm->quarter[q][2];
    *c2 = lm->quarter[q][1];
    *c3 = lm->quarter[q][0];
}
