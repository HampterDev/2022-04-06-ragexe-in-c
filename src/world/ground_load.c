#include "render/texture_cache.h"
#include "world/ground.h"
#include <stdlib.h>
#include <stdint.h>

#include <math.h>

static const int lightmap_sample_offsets[5][2] = {
    { 0, 0 },
    { 0, -1 },
    { 0, 1 },
    { 1, 0 },
    { -1, 0 },
};

struct GndNormals {
    float top[2][3];
    float front[3];
    float right[3];
};
OFFSET_ASSERT(GndNormals, front, 0x18);
SIZE_ASSERT(GndNormals, 0x30);

static float unsigned_to_float(DWORD v)
{
    return (float)(double)v;
}

static void corner(const struct C3dGround15 *self, float *pos, int x, int y, float h)
{
    pos[0] = (float)(x - self->width / 2) * self->zoom;
    pos[1] = h;
    pos[2] = (float)(y - self->height / 2) * self->zoom;
}

static int load_surface(struct C3dGround15 *self, const struct GndRes *gnd, struct GndSurface *s,
                         const struct GndSurfaceRec *rec)
{
    struct GndLightmap *lm;
    float su, sv;
    int i;

    if (rec->texture < 0 || rec->texture >= gnd->textureCount
            || !gnd->textureNames || rec->lightmap >= self->lightmaps.count)
        return 0;
    s->colour = rec->colour;
    s->texture = texture_cache_get(&g_textureCache, gnd->textureNames[rec->texture], 0, 0);
    if (!s->texture || s->texture->base.width <= 0 || s->texture->base.height <= 0)
        return 0;
    su = unsigned_to_float(s->texture->reqWidth) / unsigned_to_float(s->texture->base.width);
    sv = unsigned_to_float(s->texture->reqHeight) / unsigned_to_float(s->texture->base.height);
    for (i = 0; i < 4; i++) {
        s->v[i].u = rec->u[i] * su;
        s->v[i].v = rec->v[i] * sv;
    }
    lm = self->lightmaps.lightmaps + rec->lightmap;
    s->lightmap = lm;
    s->lightmapPage = lm->page;
    for (i = 0; i < 4; i++) {
        s->v[i].lu = lm->uv[i][0];
        s->v[i].lv = lm->uv[i][1];
    }
    return 1;
}

static void normalize(float *v)
{
    float inv = 1.0f / sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    v[0] *= inv;
    v[1] *= inv;
    v[2] *= inv;
}

static void add3(float *acc, const float *v)
{
    acc[0] += v[0];
    acc[1] += v[1];
    acc[2] += v[2];
}

static DWORD top_colour(const struct GndCell *cell)
{
    return cell && cell->top ? cell->top->colour : 0;
}

static DWORD lit(DWORD base, float r, float g, float b)
{
    BYTE *c = (BYTE *)&base;

    c[3] = 0xff;
    c[2] = (BYTE)(int)((float)c[2] * r);
    c[1] = (BYTE)(int)((float)c[1] * g);
    c[0] = (BYTE)(int)((float)c[0] * b);
    return base;
}

static void light(const float *diffuse, const float *ambient, float dot, float *rgb)
{
    float v;

    dot = dot > 0.0f ? dot : 0.0f;
    v = diffuse[0] * dot + ambient[0];
    rgb[0] = v < 1.0f ? v : 1.0f;
    v = diffuse[1] * dot + ambient[1];
    rgb[1] = v < 1.0f ? v : 1.0f;
    v = dot * diffuse[2] + ambient[2];
    rgb[2] = v < 1.0f ? v : 1.0f;
}

int c3d_ground15_load(struct C3dGround15 *self, const struct GndRes *gnd,
                                 const float *lightDir, const float *diffuse, const float *ambient)
{
    struct GndNormals *normals = NULL;
    size_t count;
    int x, y, i;

    c3d_ground15_clear(self);
    if (!gnd || gnd->surfaceCount < 0 || (size_t)gnd->surfaceCount > SIZE_MAX / sizeof *self->surfaces
            || !gnd->cubes || (gnd->surfaceCount && !gnd->surfaces)
            || !c3d_ground15_create(self, gnd->width, gnd->height))
        return 0;
    count = self->cellCount;
    normals = calloc(count, sizeof *normals);
    if (!normals)
        goto fail;
    self->zoom = gnd->zoom;
    if (!gnd_lightmap_set_build(&self->lightmaps, gnd))
        goto fail;
    if (gnd->surfaceCount) {
        self->surfaces = calloc((size_t)gnd->surfaceCount, sizeof *self->surfaces);
        if (!self->surfaces)
            goto fail;
    }
    self->surfaceCount = (size_t)gnd->surfaceCount;

    for (y = 0; y < self->height; y++) {
        for (x = 0; x < self->width; x++) {
            int index = y * self->width + x;
            const struct GndCellRec *rec = &gnd->cubes[index];
            struct GndCell *cell = self->cells + index;
            struct GndNormals *n = &normals[index];
            struct GndSurface *s;

            cell->height[0] = rec->height[0];
            cell->height[1] = rec->height[1];
            cell->height[2] = rec->height[2];
            cell->height[3] = rec->height[3];
            corner(self, cell->water[0], x, y, 0.0f);
            corner(self, cell->water[1], x + 1, y, 0.0f);
            corner(self, cell->water[2], x, y + 1, 0.0f);
            corner(self, cell->water[3], x + 1, y + 1, 0.0f);

            if (rec->top >= 0 && rec->top < gnd->surfaceCount) {
                float h0, h1, h2, h3, zoom, dx, dz, d21, d20, d31, d32, e;
                struct GndLightmap *lm;

                s = self->surfaces + rec->top;
                corner(self, s->v[0].pos, x, y, cell->height[0]);
                corner(self, s->v[1].pos, x + 1, y, cell->height[1]);
                corner(self, s->v[2].pos, x, y + 1, cell->height[2]);
                corner(self, s->v[3].pos, x + 1, y + 1, cell->height[3]);
                if (!load_surface(self, gnd, s, &gnd->surfaces[rec->top]))
                    goto fail;
                lm = s->lightmap;
                cell->colour[0] = (BYTE)(lm->centre[0] * 2 < 0xff ? lm->centre[0] * 2 : 0xff);
                cell->colour[1] = (BYTE)(lm->centre[1] * 2 < 0xff ? lm->centre[1] * 2 : 0xff);
                cell->colour[2] = (BYTE)(lm->centre[2] * 2 < 0xff ? lm->centre[2] * 2 : 0xff);

                h0 = cell->height[0];
                h1 = cell->height[1];
                h2 = cell->height[2];
                h3 = cell->height[3];
                zoom = self->zoom;
                dx = zoom - 0.0f;
                dz = 0.0f - zoom;
                d21 = h2 - h1;
                d20 = h2 - h0;
                n->top[0][0] = d21 * dx - dx * d20;
                n->top[0][1] = dx * 0.0f - dx * dz;
                n->top[0][2] = d20 * dz - d21 * 0.0f;
                normalize(n->top[0]);
                d31 = h3 - h1;
                d32 = h3 - h2;
                e = zoom - zoom;
                n->top[1][0] = d31 * e - dx * d32;
                n->top[1][1] = dx * dx - e * e;
                n->top[1][2] = e * d32 - dx * d31;
                normalize(n->top[1]);
                cell->top = s;
            }

            if (y < self->height - 1) {
                const struct GndCellRec *next = &gnd->cubes[(y + 1) * self->width + x];

                if (rec->front >= 0 && rec->front < gnd->surfaceCount) {
                    s = self->surfaces + rec->front;
                    corner(self, s->v[0].pos, x, y + 1, rec->height[2]);
                    corner(self, s->v[1].pos, x + 1, y + 1, rec->height[3]);
                    corner(self, s->v[2].pos, x, y + 1, next->height[0]);
                    corner(self, s->v[3].pos, x + 1, y + 1, next->height[1]);
                    if (!load_surface(self, gnd, s, &gnd->surfaces[rec->front]))
                        goto fail;
                    n->front[0] = 0.0f;
                    n->front[1] = 0.0f;
                    n->front[2] = next->height[0] > rec->height[2] ? -1.0f : 1.0f;
                    cell->frontWall = s;
                }
            }

            if (x < self->width - 1) {
                const struct GndCellRec *next = &gnd->cubes[y * self->width + x + 1];

                if (rec->right >= 0 && rec->right < gnd->surfaceCount) {
                    s = self->surfaces + rec->right;
                    corner(self, s->v[0].pos, x + 1, y + 1, rec->height[3]);
                    corner(self, s->v[1].pos, x + 1, y, rec->height[1]);
                    corner(self, s->v[2].pos, x + 1, y + 1, next->height[2]);
                    corner(self, s->v[3].pos, x + 1, y, next->height[0]);
                    if (!load_surface(self, gnd, s, &gnd->surfaces[rec->right]))
                        goto fail;
                    cell->lastFrame = -1;
                    n->right[0] = next->height[0] > rec->height[1] ? -1.0f : 1.0f;
                    n->right[1] = 0.0f;
                    n->right[2] = 0.0f;
                    cell->rightWall = s;
                }
            }
        }
    }

    for (y = 0; y < self->height; y++) {
        for (x = 0; x < self->width; x++) {
            int w = self->width;
            struct GndCell *cells = self->cells;
            struct GndCell *cell = &cells[y * w + x];
            const struct GndNormals *n = &normals[y * w + x];
            struct GndCell *ul = NULL, *u = NULL, *ur = NULL, *l = NULL, *r = NULL, *dl = NULL, *d = NULL, *dr = NULL;
            const struct GndNormals *nul = NULL, *nu = NULL, *nur = NULL, *nl = NULL, *nr = NULL, *ndl = NULL,
                                    *nd = NULL, *ndr = NULL;
            float c[4][3], first[3], both[3], rgb[3];
            int k;

            if (x >= 1 && y >= 1) {
                ul = &cells[(y - 1) * w + x - 1];
                nul = &normals[(y - 1) * w + x - 1];
            }
            if (y >= 1) {
                u = &cells[(y - 1) * w + x];
                nu = &normals[(y - 1) * w + x];
            }
            if (x < w - 1 && y >= 1) {
                ur = &cells[(y - 1) * w + x + 1];
                nur = &normals[(y - 1) * w + x + 1];
            }
            if (x >= 1) {
                l = &cells[y * w + x - 1];
                nl = &normals[y * w + x - 1];
            }
            if (x < w - 1) {
                r = &cells[y * w + x + 1];
                nr = &normals[y * w + x + 1];
            }
            if (x >= 1 && y < self->height - 1) {
                dl = &cells[(y + 1) * w + x - 1];
                ndl = &normals[(y + 1) * w + x - 1];
            }
            if (y < self->height - 1) {
                d = &cells[(y + 1) * w + x];
                nd = &normals[(y + 1) * w + x];
            }
            if (x < w - 1 && y < self->height - 1) {
                dr = &cells[(y + 1) * w + x + 1];
                ndr = &normals[(y + 1) * w + x + 1];
            }

            first[0] = n->top[0][0] + 0.0f;
            first[1] = n->top[0][1] + 0.0f;
            first[2] = n->top[0][2] + 0.0f;
            both[0] = n->top[1][0] + first[0];
            both[1] = n->top[1][1] + first[1];
            both[2] = n->top[1][2] + first[2];

            c[0][0] = first[0];
            c[0][1] = first[1];
            c[0][2] = first[2];
            if (u && u->height[2] == cell->height[0]) {
                add3(c[0], nu->top[0]);
                add3(c[0], nu->top[1]);
            }
            if (l && l->height[1] == cell->height[0]) {
                add3(c[0], nl->top[0]);
                add3(c[0], nl->top[1]);
            }
            if (ul && ul->height[3] == cell->height[0])
                add3(c[0], nul->top[1]);

            c[1][0] = both[0];
            c[1][1] = both[1];
            c[1][2] = both[2];
            if (r && r->height[0] == cell->height[1])
                add3(c[1], nr->top[0]);
            if (u && u->height[3] == cell->height[1])
                add3(c[1], nu->top[1]);
            if (ur && ur->height[2] == cell->height[1]) {
                for (k = 0; k < 3; k++)
                    c[1][k] = nur->top[0][k] + c[1][k] + nur->top[1][k];
            }

            c[2][0] = both[0];
            c[2][1] = both[1];
            c[2][2] = both[2];
            if (l && l->height[3] == cell->height[2])
                add3(c[2], nl->top[1]);
            if (d && d->height[0] == cell->height[2]) {
                for (k = 0; k < 3; k++)
                    c[2][k] = nd->top[0][k] + c[2][k];
            }
            if (dl && dl->height[1] == cell->height[2]) {
                for (k = 0; k < 3; k++)
                    c[2][k] = ndl->top[0][k] + c[2][k] + ndl->top[1][k];
            }

            c[3][0] = first[0];
            c[3][1] = first[1];
            c[3][2] = first[2];
            if (r && r->height[2] == cell->height[3]) {
                for (k = 0; k < 3; k++)
                    c[3][k] = nr->top[0][k] + c[3][k] + nr->top[1][k];
            }
            if (d && d->height[1] == cell->height[3]) {
                for (k = 0; k < 3; k++)
                    c[3][k] = nd->top[0][k] + c[3][k] + nd->top[1][k];
            }
            if (dr && dr->height[0] == cell->height[3]) {
                for (k = 0; k < 3; k++)
                    c[3][k] = ndr->top[0][k] + c[3][k];
            }

            if (cell->top) {
                for (i = 0; i < 4; i++) {
                    DWORD base = 0;

                    normalize(c[i]);
                    light(diffuse, ambient,
                          c[i][1] * lightDir[1] + c[i][0] * lightDir[0] + c[i][2] * lightDir[2], rgb);
                    switch (i) {
                    case 0: base = cell->top->colour; break;
                    case 1: base = top_colour(r); break;
                    case 2: base = top_colour(d); break;
                    case 3: base = top_colour(dr); break;
                    }
                    cell->top->v[i].colour = lit(base, rgb[0], rgb[1], rgb[2]);
                }
            }
            if (cell->frontWall) {
                for (i = 0; i < 4; i++) {
                    light(diffuse, ambient,
                          n->front[0] * lightDir[0] + lightDir[1] * n->front[1] + lightDir[2] * n->front[2], rgb);
                    cell->frontWall->v[i].colour = lit(i & 1 ? top_colour(dr) : top_colour(d), rgb[0], rgb[1], rgb[2]);
                }
            }
            if (cell->rightWall) {
                for (i = 0; i < 4; i++) {
                    light(diffuse, ambient,
                          lightDir[1] * n->right[1] + lightDir[0] * n->right[0] + lightDir[2] * n->right[2], rgb);
                    cell->rightWall->v[i].colour = lit(i & 1 ? top_colour(r) : top_colour(dr), rgb[0], rgb[1], rgb[2]);
                }
            }
        }
    }
    free(normals);
    return 1;
fail:
    free(normals);
    c3d_ground15_clear(self);
    return 0;
}

void c3d_ground15_build_vertex_colours(struct C3dGround15 *self)
{
    int ax, ay;

    for (ay = 0; ay < 2 * self->height; ay++) {
        int cy = ay / 2;

        for (ax = 0; ax < 2 * self->width; ax++) {
            struct AttrCell *c = c3d_attr_get_cell(self->attr, ax, ay);
            float avg = 0.0f, min, max, r, g, b;

            if (c)
                avg = (c->height[1] + c->height[0] + c->height[2] + c->height[3]) * 0.25f;
            c3d_ground15_get_cell_height_min_max(self, ax / 2,
                                          cy, &min, &max);
            r = 1.0f;
            g = 1.0f;
            b = 1.0f;
            if (avg >= min - 2.5f) {
                int sumI = 0, sumR = 0, sumG = 0, sumB = 0;
                const int (*off)[2];
                float base, top;

                for (off = lightmap_sample_offsets; off < lightmap_sample_offsets + 5; off++) {
                    BYTE ci, cr, cg, cb;

                    c3d_ground15_get_lightmap_colour(self, ax + (*off)[0], ay + (*off)[1], &ci, &cr, &cg, &cb);
                    sumI += ci;
                    sumR += cr;
                    sumG += cg;
                    sumB += cb;
                }
                base = (float)sumI / 5.0f / 255.0f;
                g = (float)sumG / 5.0f * 0.0078125f + base;
                r = (float)sumR / 5.0f * 0.0078125f + base;
                b = (float)sumB / 5.0f * 0.0078125f + base;
                top = g > r ? g : r;
                top = b > top ? b : top;
                if (top > 1.0f) {
                    r /= top;
                    g /= top;
                    b /= top;
                }
            }
            if (self->colours != NULL) {
                struct GndColour *col = self->colours + (ax + 2 * self->width * ay);
                col->r = r;
                col->g = g;
                col->b = b;
            }
        }
    }
}

static void surface_check_alpha(struct GndSurface *s, const struct Ray3 *ray, float maxT)
{
    Vec3 q[4];
    float u, v, t;
    int i;

    for (i = 0; i < 4; i++) {
        q[i].x = s->v[i].pos[0];
        q[i].y = s->v[i].pos[1];
        q[i].z = s->v[i].pos[2];
    }
    if (ray_triangle_hit(ray, &q[0], &q[1], &q[2], &u, &v, &t) && maxT > t)
        s->halfAlpha = 1;
    if (s->halfAlpha == 0 && ray_triangle_hit(ray, &q[1], &q[3], &q[2], &u, &v, &t) && maxT > t)
        s->halfAlpha = 1;
}

void c3d_ground15_check_alpha(struct C3dGround15 *self, const struct Ray3 *ray, int x, int y,
                                       float maxT)
{
    struct GndCell *cell;

    if (x < 0 || x >= self->width || y < 0 || y >= self->height)
        return;
    cell = GND_CELL(self, x, y);
    if (cell->top)
        surface_check_alpha(cell->top, ray, maxT);
    if (cell->frontWall)
        surface_check_alpha(cell->frontWall, ray, maxT);
    if (cell->rightWall)
        surface_check_alpha(cell->rightWall, ray, maxT);
}
