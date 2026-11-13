#include "render/quads.h"
#include <string.h>

/* The record's header and kinds, as render/shaders/instances.hlsli reads them */
#define QUAD_SPRITE 1
#define QUAD_RECT 2
#define QUAD_SCREEN 3
#define QUAD_WORLD 4
#define QUAD_TRIANGLE 16u
#define QUAD_SNAP 32u
#define QUAD_TURNED 64u
#define QUAD_STAGE2 128u
#define QUAD_RHW_ADD 256u
#define QUAD_RHW_VALUE 512u
#define QUAD_Z_VALUE 1024u
#define QUAD_Z_ADD 2048u
#define QUAD_VIEW 4096u
#define QUAD_UPRIGHT 8192u
#define QUAD_RIGHT 16384u
#define QUAD_CORNERS 5

static uint32_t bits(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}

static void header(uint32_t *w, uint32_t kind, uint32_t flags, const struct QuadHeader *h, uint32_t colour,
                   uint32_t last)
{
    w[0] = kind | flags | (h->snap ? QUAD_SNAP : 0) | (h->layer & 0xffffu) << 16;
    w[1] = h->draw;
    w[2] = colour;
    w[3] = last;
}

static void row(uint32_t *w, float a, float b, float c, float d)
{
    w[0] = bits(a);
    w[1] = bits(b);
    w[2] = bits(c);
    w[3] = bits(d);
}

/* the flags of an rhw and a depth rule (GFX_RHW_*, GFX_DEPTH_*) */
static uint32_t rule_flags(int rhw, int depth)
{
    return (rhw == GFX_RHW_ADD ? QUAD_RHW_ADD : rhw == GFX_RHW_VALUE ? QUAD_RHW_VALUE : 0)
         | (depth == GFX_DEPTH_VALUE ? QUAD_Z_VALUE : depth == GFX_DEPTH_ADD ? QUAD_Z_ADD : 0);
}

int quad_corner_words(const struct GfxCorners *c, uint32_t matrix, uint32_t *w)
{
    uint32_t flags = rule_flags(c->rhw, c->depth) | (c->upright ? QUAD_UPRIGHT : 0);
    int n = 4, i;
    if (c->count < 1 || c->count > GFX_CORNERS_MAX)
        return 0;
    if (flags) {
        row(w + n, c->rhwValue, c->depthValue, c->lift, 0.0f);
        n += 4;
    }
    if (c->upright) {
        row(w + n, c->uprightPoint[0], c->uprightPoint[1], c->uprightPoint[2], 0.0f);
        n += 4;
    }
    w[0] = flags;
    w[1] = c->colour;
    w[2] = matrix;
    w[3] = (uint32_t)n;
    for (i = 0; i < c->count; i++) {
        w[n++] = bits(c->point[i][0]);
        w[n++] = bits(c->point[i][1]);
        w[n++] = bits(c->point[i][2]);
        w[n++] = bits(c->u[i]);
        w[n++] = bits(c->v[i]);
    }
    while (n % 4)
        w[n++] = 0;
    return n;
}

int quad_words(const struct GfxQuad *q, const struct QuadHeader *h, uint32_t *w)
{
    int i;
    if (q->kind == GFX_QUAD_CORNERS) {
        const unsigned char *c = q->corners.corner;
        header(w, QUAD_CORNERS, QUAD_TRIANGLE | (q->corners.right ? QUAD_RIGHT : 0), h, h->matrix,
               (uint32_t)c[0] | (uint32_t)c[1] << 8 | (uint32_t)c[2] << 16);
        return 4;
    }
    if (q->kind == GFX_QUAD_SPRITE) {
        header(w, QUAD_SPRITE, q->sprite.turned ? QUAD_TURNED : 0, h, q->sprite.colour, 0);
        row(w + 4, q->sprite.x0, q->sprite.x1, q->sprite.top, q->sprite.bottom);
        row(w + 8, q->sprite.rhwBottom, q->sprite.rhwTop, q->sprite.offsetX, q->sprite.offsetY);
        row(w + 12, q->sprite.cosine, q->sprite.sine, 0.0f, 0.0f);
        row(w + 16, q->sprite.u0, q->sprite.v0, q->sprite.u1, q->sprite.v1);
        return 20;
    }
    if (q->kind == GFX_QUAD_WORLD) {
        int n = q->world.stage2 ? 36 : 28;
        uint32_t flags = rule_flags(q->world.rhw, q->world.depth) | (q->world.stage2 ? QUAD_STAGE2 : 0)
                       | (q->world.viewSpace ? QUAD_VIEW : 0);
        header(w, QUAD_WORLD, flags, h, q->world.colour, h->matrix);
        for (i = 0; i < 4; i++) {
            const float *p = q->world.point[i];
            row(w + 4 + i * 4, p[0], p[1], p[2], i == 0 ? q->world.rhwValue : i == 1 ? q->world.depthValue : 0.0f);
        }
        row(w + 20, q->world.u[0], q->world.u[1], q->world.u[2], q->world.u[3]);
        row(w + 24, q->world.v[0], q->world.v[1], q->world.v[2], q->world.v[3]);
        if (q->world.stage2) {
            row(w + 28, q->world.u1[0], q->world.u1[1], q->world.u1[2], q->world.u1[3]);
            row(w + 32, q->world.v1[0], q->world.v1[1], q->world.v1[2], q->world.v1[3]);
        }
        return n;
    }
    return 0;
}

int quad_instance_count(int n, int strip)
{
    if (strip)
        return n == 4 ? 1 : 0;
    return n > 0 && n % 3 == 0 ? n / 3 : 0;
}

int quad_vertices_flat(const struct TexDrawVertex *v, int n)
{
    int i;
    for (i = 1; i < n; i++)
        if (bits(v[i].rhw) != bits(v[0].rhw))
            return 0;
    return 1;
}

int quad_rhw_flat(const float *rhw, int n)
{
    int i;
    for (i = 1; i < n; i++)
        if (bits(rhw[i]) != bits(rhw[0]))
            return 0;
    return 1;
}

static int same(float a, float b)
{
    return bits(a) == bits(b);
}

/* corners 0 to 3 of a strip of four make a rectangle with one rhw and colour, the same z along each
   row and the texture coordinates of its corners by column and row */
static int rectangle(const struct TexDrawVertex *v)
{
    return same(v[0].x, v[2].x) && same(v[1].x, v[3].x) && same(v[0].y, v[1].y) && same(v[2].y, v[3].y)
        && same(v[0].z, v[1].z) && same(v[2].z, v[3].z) && same(v[0].rhw, v[1].rhw) && same(v[0].rhw, v[2].rhw)
        && same(v[0].rhw, v[3].rhw) && v[0].diffuse == v[1].diffuse && v[0].diffuse == v[2].diffuse
        && v[0].diffuse == v[3].diffuse && same(v[0].u, v[2].u) && same(v[1].u, v[3].u) && same(v[0].v, v[1].v)
        && same(v[2].v, v[3].v);
}

int quad_vertex_words(int n, int strip, uint32_t first, int index, const struct QuadHeader *h, uint32_t *w)
{
    if (index < 0 || index >= quad_instance_count(n, strip))
        return 0;
    header(w, QUAD_SCREEN, strip ? 0 : QUAD_TRIANGLE, h, 0, first + (uint32_t)(strip ? 0 : index * 3 * 7));
    return 4;
}

int quad_rect_words(const struct TexDrawVertex *v, const struct QuadHeader *h, uint32_t *w)
{
    if (!rectangle(v))
        return 0;
    header(w, QUAD_RECT, 0, h, v[0].diffuse, bits(v[0].rhw));
    row(w + 4, v[0].x, v[1].x, v[0].y, v[2].y);
    row(w + 8, v[0].z, v[2].z, 0.0f, 0.0f);
    row(w + 12, v[0].u, v[0].v, v[1].u, v[2].v);
    return 16;
}
