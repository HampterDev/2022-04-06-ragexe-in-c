#include "render/world_layout.h"

#include <string.h>

static uint32_t float_bits(float f)
{
    uint32_t u;

    memcpy(&u, &f, 4);
    return u;
}

void wl_records(const struct SwRecord *records, size_t count, struct WlRecord *byId, struct WlCullRecord *cull)
{
    size_t i;
    int k;

    for (i = 0; i < count; i++) {
        const struct SwRecord *r = &records[i];
        struct WlRecord *w = &byId[r->id];
        struct WlCullRecord *c = &cull[i];

        w->data[0] = r->textureLayer | (uint32_t)r->lightmapLayer << 16;
        w->data[1] = r->transform | (uint32_t)r->uv << 16;
        w->data[2] = r->pass | (uint32_t)r->flags << 8 | (uint32_t)r->textureArray << 16
                     | (uint32_t)r->lightmapArray << 24;
        w->data[3] = 0;
        for (k = 0; k < 3; k++) {
            c->centre[k] = float_bits(r->centre[k]);
            c->half[k] = float_bits(r->half[k]);
        }
        c->centre[3] = r->transform | (uint32_t)r->flags << 16;
        c->half[3] = r->firstIndex;
        c->draw[0] = r->indexCount;
        c->draw[1] = r->baseVertex;
        c->draw[2] = c->draw[3] = 0;
    }
}

size_t wl_chunks(const struct SwRecord *records, size_t count, struct WlChunk *out)
{
    size_t n = 0, i;
    uint32_t at;

    for (i = 0; i < count; i++)
        for (at = 0; at < records[i].indexCount; at += WL_CHUNK) {
            if (out) {
                out[n].record = (uint32_t)i;
                out[n].offset = at;
            }
            n++;
            if (records[i].indexCount - at <= WL_CHUNK)
                break;
        }
    return n;
}

size_t wl_runs(const struct SwDrawRun *runs, size_t count, struct WlRun *out, uint32_t *indices)
{
    size_t n = 0, i;
    uint32_t at = 0;

    for (i = 0; i < count && runs[i].pass <= SW_PASS_GROUND; i++) {
        if (runs[i].indices > 0xffffffffu - at)
            break;
        out[n].first = runs[i].first;
        out[n].count = runs[i].count;
        out[n].start = at;
        out[n].indices = runs[i].indices;
        at += runs[i].indices;
        n++;
    }
    *indices = at;
    return n;
}

void wl_slot(const float *local, int parent, int animated, int world, struct WlSlot *out)
{
    memcpy(out->local, local, sizeof out->local);
    out->link[0] = parent;
    out->link[1] = world ? 1 : 0;
    out->link[2] = animated;
    out->link[3] = 0;
}

void wl_projection(float out[16], const float proj[2], const float centre[2], float nearZ, float depthScale, int w,
                   int h)
{
    memset(out, 0, 16 * sizeof *out);
    out[0] = 2.0f * proj[0] / (float)w;
    out[5] = -2.0f * proj[1] / (float)h;
    out[8] = 2.0f * centre[0] / (float)w - 1.0f;
    out[9] = 1.0f - 2.0f * centre[1] / (float)h;
    out[10] = depthScale;
    out[11] = 1.0f;
    out[14] = -nearZ * depthScale;
}

void wl_view_clip(float out[16], const float view[12], const float projection[16])
{
    float a[16], r[16];
    int i, j;

    for (i = 0; i < 4; i++) {
        a[i * 4] = view[i * 3];
        a[i * 4 + 1] = view[i * 3 + 1];
        a[i * 4 + 2] = view[i * 3 + 2];
        a[i * 4 + 3] = i == 3 ? 1.0f : 0.0f;
    }
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            r[i * 4 + j] = a[i * 4] * projection[j] + a[i * 4 + 1] * projection[4 + j] + a[i * 4 + 2] * projection[8 + j]
                           + a[i * 4 + 3] * projection[12 + j];
    memcpy(out, r, sizeof r);
}

void wl_groups(uint32_t groups, uint32_t out[2])
{
    out[0] = groups < WL_GROUPS_ACROSS ? groups : WL_GROUPS_ACROSS;
    out[1] = groups ? (groups - 1) / WL_GROUPS_ACROSS + 1 : 0;
}

void wl_cull_frame(const struct SwFrustum *frustum, uint32_t runs, struct WlCullFrame *out, uint32_t groups[2])
{
    memset(out, 0, sizeof *out);
    memcpy(out->planes, frustum->plane, sizeof out->planes);
    wl_groups(runs, groups);
    out->counts[0] = runs;
    out->counts[1] = groups[0];
}

void wl_compact_frame(uint32_t chunks, struct WlCompactFrame *out, uint32_t groups[2])
{
    memset(out, 0, sizeof *out);
    wl_groups(chunks, groups);
    out->counts[0] = chunks;
    out->counts[1] = groups[0];
}

void wl_transform_frame(const float camera[12], const float projection[16], const float (*fromView)[16],
                        uint32_t cascades, uint32_t slots, struct WlTransformFrame *out)
{
    uint32_t c;

    memset(out, 0, sizeof *out);
    memcpy(out->camera, camera, sizeof out->camera);
    memcpy(out->projection, projection, sizeof out->projection);
    for (c = 0; c < cascades && c < WL_CASCADES; c++)
        memcpy(out->fromView[c], fromView[c], sizeof out->fromView[c]);
    out->counts[0] = slots;
    out->counts[1] = cascades < WL_CASCADES ? cascades : WL_CASCADES;
}
