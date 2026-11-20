#include "render/static_world.h"

#include <math.h>

#define SW_MAX_ELEMENTS 0xffffffffu
/* the half size of a box around corners that are not all finite: every view and cascade keeps it */
#define SW_UNBOUNDED 1e30f
#define SW_FLAGS (SW_CULL_CCW | SW_CULL_NONE | SW_ANIMATED)

int sw_layout_place(struct SwLayout *layout, uint32_t vertices, uint32_t indices, struct SwPlacement *out)
{
    if (!layout || !out || vertices == 0 || vertices > SW_MESH_VERTICES || indices == 0 || indices % 3 != 0
        || vertices > SW_MAX_ELEMENTS - layout->vertexCount || indices > SW_MAX_ELEMENTS - layout->indexCount)
        return 0;
    out->baseVertex = layout->vertexCount;
    out->vertexCount = vertices;
    out->firstIndex = layout->indexCount;
    out->indexCount = indices;
    layout->vertexCount += vertices;
    layout->indexCount += indices;
    return 1;
}

uint32_t sw_layout_grow(uint32_t capacity, uint32_t needed, uint32_t minimum)
{
    uint32_t grown;

    if (needed <= capacity)
        return capacity;
    grown = capacity > minimum ? capacity : minimum;
    if (grown == 0)
        grown = 1;
    while (grown < needed)
        grown = grown > SW_MAX_ELEMENTS / 2 ? SW_MAX_ELEMENTS : grown * 2;
    return grown;
}

static void place_point(const float *m, const float *p, float *out)
{
    if (!m) {
        out[0] = p[0];
        out[1] = p[1];
        out[2] = p[2];
        return;
    }
    out[0] = p[0] * m[0] + p[1] * m[3] + p[2] * m[6] + m[9];
    out[1] = p[0] * m[1] + p[1] * m[4] + p[2] * m[7] + m[10];
    out[2] = p[0] * m[2] + p[1] * m[5] + p[2] * m[8] + m[11];
}

static int finite3(const float *p)
{
    return isfinite(p[0]) && isfinite(p[1]) && isfinite(p[2]);
}

static int run_valid(const struct SwMesh *mesh, const struct SwRun *run)
{
    uint32_t i;

    if (run->count == 0 || run->count % 3 != 0 || run->first > mesh->indexCount
        || run->count > mesh->indexCount - run->first || run->pass >= SW_PASS_COUNT || (run->flags & ~SW_FLAGS))
        return 0;
    for (i = run->first; i < run->first + run->count; i++)
        if (mesh->indices[i] >= mesh->vertexCount)
            return 0;
    return 1;
}

static void run_record(const struct SwMesh *mesh, const struct SwPlacement *place, const struct SwRun *run,
                       uint32_t id, struct SwRecord *r)
{
    const float *m = mesh->animated ? NULL : mesh->placed;
    float lo[3] = {0, 0, 0}, hi[3] = {0, 0, 0}, p[3];
    int bounded = 1, k;
    uint32_t i;

    for (i = run->first; i < run->first + run->count; i++) {
        place_point(m, mesh->positions + (size_t)mesh->indices[i] * mesh->stride, p);
        if (!finite3(p)) {
            bounded = 0;
            continue;
        }
        for (k = 0; k < 3; k++) {
            if (i == run->first || p[k] < lo[k])
                lo[k] = p[k];
            if (i == run->first || p[k] > hi[k])
                hi[k] = p[k];
        }
    }
    r->firstIndex = place->firstIndex + run->first;
    r->indexCount = run->count;
    r->baseVertex = place->baseVertex;
    r->sort[0] = run->sort[0];
    r->sort[1] = run->sort[1];
    r->id = id;
    r->textureLayer = run->textureLayer;
    r->lightmapLayer = run->lightmapLayer;
    r->textureArray = run->textureArray;
    r->lightmapArray = run->lightmapArray;
    r->transform = mesh->transform;
    r->uv = run->uv;
    r->pass = run->pass;
    r->flags = (uint8_t)(run->flags | (mesh->animated ? SW_ANIMATED : 0));
    for (k = 0; k < 3; k++) {
        r->centre[k] = bounded ? (lo[k] + hi[k]) * 0.5f : 0.0f;
        r->half[k] = bounded ? (hi[k] - lo[k]) * 0.5f : SW_UNBOUNDED;
    }
    place_point(m, mesh->positions + (size_t)mesh->indices[run->first] * mesh->stride, r->key);
}

int sw_records_build(const struct SwMesh *mesh, const struct SwPlacement *place, const struct SwRun *runs,
                     size_t count, uint32_t firstId, struct SwRecord *out)
{
    size_t i;

    if (!mesh || !place || (count && (!runs || !out)) || !mesh->positions || mesh->stride < 3 || !mesh->indices
        || mesh->vertexCount != place->vertexCount || mesh->indexCount != place->indexCount)
        return 0;
    for (i = 0; i < count; i++)
        if (!run_valid(mesh, &runs[i]))
            return 0;
    for (i = 0; i < count; i++)
        run_record(mesh, place, &runs[i], firstId + (uint32_t)i, &out[i]);
    return 1;
}

int sw_vertex_ids(const struct SwMesh *mesh, const struct SwRun *runs, size_t count, uint32_t firstId, uint32_t *out)
{
    size_t i;
    uint32_t k;

    if (!mesh || !out || (count && !runs) || !mesh->indices)
        return 0;
    for (k = 0; k < mesh->vertexCount; k++)
        out[k] = SW_NO_RECORD;
    for (i = 0; i < count; i++) {
        const struct SwRun *run = &runs[i];
        uint32_t id = firstId + (uint32_t)i;

        if (run->first > mesh->indexCount || run->count > mesh->indexCount - run->first)
            return 0;
        for (k = run->first; k < run->first + run->count; k++) {
            uint16_t v = mesh->indices[k];

            if (v >= mesh->vertexCount || (out[v] != SW_NO_RECORD && out[v] != id))
                return 0;
            out[v] = id;
        }
    }
    return 1;
}

/* the order the passes draw records in; a total order over what a record draws, so that any
   sort gives the same sequence */
int sw_record_before(const struct SwRecord *a, const struct SwRecord *b)
{
    const uint32_t x[13] = {a->pass, a->sort[0], a->sort[1], (uint32_t)(a->flags & SW_CULL_MASK), a->firstIndex,
                            a->indexCount, a->baseVertex, (uint32_t)a->textureArray << 16 | a->textureLayer,
                            (uint32_t)a->lightmapArray << 16 | a->lightmapLayer, a->transform, a->uv, a->flags, a->id};
    const uint32_t y[13] = {b->pass, b->sort[0], b->sort[1], (uint32_t)(b->flags & SW_CULL_MASK), b->firstIndex,
                            b->indexCount, b->baseVertex, (uint32_t)b->textureArray << 16 | b->textureLayer,
                            (uint32_t)b->lightmapArray << 16 | b->lightmapLayer, b->transform, b->uv, b->flags, b->id};
    int i;

    for (i = 0; i < 13; i++)
        if (x[i] != y[i])
            return x[i] < y[i];
    return 0;
}

static void record_sift(struct SwRecord *r, size_t root, size_t count)
{
    for (;;) {
        size_t child = root * 2 + 1;
        struct SwRecord swap;

        if (child >= count)
            return;
        if (child + 1 < count && sw_record_before(&r[child], &r[child + 1]))
            child++;
        if (!sw_record_before(&r[root], &r[child]))
            return;
        swap = r[root];
        r[root] = r[child];
        r[child] = swap;
        root = child;
    }
}

void sw_records_sort(struct SwRecord *records, size_t count)
{
    size_t i;

    if (!records || count < 2)
        return;
    for (i = count / 2; i-- > 0;)
        record_sift(records, i, count);
    for (i = count - 1; i > 0; i--) {
        struct SwRecord swap = records[0];

        records[0] = records[i];
        records[i] = swap;
        record_sift(records, 0, i);
    }
}

static uint8_t record_cull(const struct SwRecord *r)
{
    return (uint8_t)(r->flags & SW_CULL_MASK);
}

size_t sw_draw_runs(const struct SwRecord *records, size_t count, struct SwDrawRun *out)
{
    size_t n = 0, i;

    for (i = 0; i < count; i++) {
        const struct SwRecord *r = &records[i];
        struct SwDrawRun *last = n ? &out[n - 1] : NULL;

        if (last && last->pass == r->pass && last->textureArray == r->textureArray
            && last->lightmapArray == r->lightmapArray && last->cull == record_cull(r)) {
            last->count++;
            last->indices += r->indexCount;
            continue;
        }
        out[n].first = (uint32_t)i;
        out[n].count = 1;
        out[n].indices = r->indexCount;
        out[n].pass = r->pass;
        out[n].textureArray = r->textureArray;
        out[n].lightmapArray = r->lightmapArray;
        out[n].cull = record_cull(r);
        n++;
    }
    return n;
}

/* the plane a * (view x or y, by axis) + b * (view depth) + c >= 0 in world coordinates; the view's
   x, y and depth are the columns 0, 1 and 2 of the view matrix */
static void view_plane(float *plane, const float *m, int axis, float a, float b, float c)
{
    int i;

    for (i = 0; i < 4; i++)
        plane[i] = a * m[i * 3 + axis] + b * m[i * 3 + 2];
    plane[3] += c;
}

void sw_frustum(struct SwFrustum *out, const struct SwProjection *p, const float *view)
{
    /* on screen where 0 <= x * proj[0] + centre[0] * z <= size[0] * z, and so for y */
    view_plane(out->plane[0], view, 0, p->proj[0], p->centre[0], 0.0f);
    view_plane(out->plane[1], view, 0, -p->proj[0], p->size[0] - p->centre[0], 0.0f);
    view_plane(out->plane[2], view, 1, p->proj[1], p->centre[1], 0.0f);
    view_plane(out->plane[3], view, 1, -p->proj[1], p->size[1] - p->centre[1], 0.0f);
    view_plane(out->plane[4], view, 0, 0.0f, 1.0f, -p->nearZ);
    view_plane(out->plane[5], view, 0, 0.0f, -1.0f, p->farZ);
}

int sw_record_bounds(const struct SwRecord *r, const float (*transforms)[12], float *centre, float *half)
{
    const float *m;
    int k;

    if (!(r->flags & SW_ANIMATED)) {
        for (k = 0; k < 3; k++) {
            centre[k] = r->centre[k];
            half[k] = r->half[k];
        }
        return 1;
    }
    if (!transforms)
        return 0;
    m = transforms[r->transform];
    place_point(m, r->centre, centre);
    for (k = 0; k < 3; k++)
        half[k] = fabsf(m[k]) * r->half[0] + fabsf(m[3 + k]) * r->half[1] + fabsf(m[6 + k]) * r->half[2];
    return 1;
}

/* whether a box lies wholly outside one of the planes, with world/frustum.c's test */
static int outside_frustum(const struct SwFrustum *f, const float *c, const float *h)
{
    int i;

    for (i = 0; i < 6; i++) {
        const float *n = f->plane[i];
        float reach = fabsf(h[0] * n[0]) + fabsf(n[1] * h[1]) + fabsf(h[2] * n[2]);
        float dist = c[0] * n[0] + n[1] * c[1] + c[2] * n[2] + n[3];

        if (dist > reach)
            continue;
        if (!(dist > -reach))
            return 1;
    }
    return 0;
}

int sw_record_in_view(const struct SwRecord *record, const float (*transforms)[12], const struct SwFrustum *frustum)
{
    float centre[3], half[3];

    return sw_record_bounds(record, transforms, centre, half) && !outside_frustum(frustum, centre, half);
}

void sw_cascade_planes(const struct SwCascade *cascade, struct SwFrustum *out)
{
    int a, k;

    for (a = 0; a < 3; a++) {
        float lo = a < 2 ? cascade->lo[a] : cascade->depth[0], hi = a < 2 ? cascade->hi[a] : cascade->depth[1];

        for (k = 0; k < 3; k++) {
            out->plane[a * 2][k] = cascade->axis[a][k];
            out->plane[a * 2 + 1][k] = -cascade->axis[a][k];
        }
        out->plane[a * 2][3] = -lo;
        out->plane[a * 2 + 1][3] = hi;
    }
}

size_t sw_cull_cascade(const struct SwRecord *records, size_t count, const float (*transforms)[12],
                       const struct SwCascade *cascade, uint32_t *out)
{
    struct SwFrustum box;
    size_t n = 0, i;

    sw_cascade_planes(cascade, &box);
    for (i = 0; i < count; i++)
        if (sw_record_in_view(&records[i], transforms, &box))
            out[n++] = (uint32_t)i;
    return n;
}

int sw_texture_place(struct SwTextureArrays *arrays, uint16_t w, uint16_t h, uint8_t levels, uint8_t *array,
                     uint16_t *layer)
{
    size_t i;

    if (!arrays || !array || !layer || w == 0 || h == 0 || levels == 0 || arrays->maxLayers == 0
        || arrays->maxLayers > 65536 || arrays->count > arrays->capacity)
        return 0;
    for (i = arrays->count; i-- > 0;) {
        struct SwTextureArray *a = &arrays->arrays[i];

        if (a->width != w || a->height != h || a->levels != levels)
            continue;
        if (a->layers >= arrays->maxLayers)
            break;
        *array = (uint8_t)i;
        *layer = (uint16_t)a->layers++;
        return 1;
    }
    if (arrays->count >= arrays->capacity || arrays->count >= SW_NO_ARRAY || !arrays->arrays)
        return 0;
    arrays->arrays[arrays->count].width = w;
    arrays->arrays[arrays->count].height = h;
    arrays->arrays[arrays->count].levels = levels;
    arrays->arrays[arrays->count].layers = 1;
    *array = (uint8_t)arrays->count++;
    *layer = 0;
    return 2;
}

void sw_texture_unplace(struct SwTextureArrays *arrays, uint8_t array, uint16_t layer)
{
    struct SwTextureArray *a;

    if (!arrays || array >= arrays->count)
        return;
    a = &arrays->arrays[array];
    if (a->layers == (uint32_t)layer + 1)
        a->layers--;
    if (!a->layers && array + 1u == arrays->count)
        arrays->count--;
}
