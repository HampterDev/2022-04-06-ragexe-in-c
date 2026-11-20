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

    for (i = 0; i < count; i++) {
        if (runs[i].indices > WL_SECTION_INDICES - at)
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

size_t wl_caster_runs(const struct SwRecord *records, size_t count, uint32_t *order, size_t *held, struct WlRun *out,
                      uint32_t *indices)
{
    size_t from[257], n = 0, placed, i;
    uint32_t at = 0;
    unsigned a;

    /* the records by array, in draw order within each (a counting sort) */
    memset(from, 0, sizeof from);
    for (i = 0; i < count; i++)
        from[records[i].textureArray + 1]++;
    for (a = 1; a <= 256; a++)
        from[a] += from[a - 1];
    for (i = 0; i < count; i++)
        order[from[records[i].textureArray]++] = (uint32_t)i;
    /* as many as fit, a run for each stretch of them with one caster's array */
    for (placed = 0; placed < count && records[order[placed]].indexCount <= WL_SECTION_INDICES - at; placed++) {
        const struct SwRecord *r = &records[order[placed]];

        if (!placed || wl_caster_array(r) != wl_caster_array(&records[order[placed - 1]])) {
            if (n)
                out[n - 1].indices = at - out[n - 1].start;
            out[n].first = (uint32_t)placed;
            out[n].start = at;
            n++;
        }
        out[n - 1].count = (uint32_t)(placed + 1 - out[n - 1].first);
        at += r->indexCount;
    }
    if (n)
        out[n - 1].indices = at - out[n - 1].start;
    *held = placed;
    *indices = at;
    return n;
}

uint8_t wl_caster_array(const struct SwRecord *record)
{
    return record->pass == SW_PASS_GROUND ? SW_NO_ARRAY : record->textureArray;
}

void wl_slot(const float *local, int parent, int animated, int world, struct WlSlot *out)
{
    memcpy(out->local, local, sizeof out->local);
    out->link[0] = parent;
    out->link[1] = world ? 1 : 0;
    out->link[2] = animated;
    out->link[3] = 0;
}

/* a count of words as the shaders index them, or 0xffffffff where it passes what a uint holds */
static uint32_t words(uint64_t n)
{
    return n < 0xffffffffu ? (uint32_t)n : 0xffffffffu;
}

void wl_cull_layout(size_t records, size_t runs, size_t casterRuns, size_t orderCount, size_t chunks,
                    struct WlCullLayout *out)
{
    uint64_t at = (uint64_t)records * (sizeof(struct WlCullRecord) / 4);

    out->runs = words(at);
    at += (uint64_t)runs * (sizeof(struct WlRun) / 4);
    out->casterRuns = words(at);
    at += (uint64_t)casterRuns * (sizeof(struct WlRun) / 4);
    out->order = words(at);
    at += orderCount;
    out->chunks = words(at);
    at += (uint64_t)chunks * (sizeof(struct WlChunk) / 4);
    out->words = words(at);
}

void wl_draw_layout(size_t records, size_t runs, size_t casterRuns, struct WlDrawLayout *out)
{
    uint64_t at = ((uint64_t)runs + (uint64_t)casterRuns * WL_CASCADES) * 5;

    out->commands = 0;
    out->places = words(at);
    out->words = words(at + (uint64_t)records * WL_SECTIONS);
}

uint32_t wl_view_command(uint32_t run)
{
    return run;
}

uint32_t wl_caster_command(size_t runs, size_t casterRuns, int cascade, uint32_t run)
{
    return words((uint64_t)runs + (uint64_t)cascade * casterRuns + run);
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

void wl_cull_frame(const struct SwFrustum *frustum, uint32_t runs, const struct WlCullLayout *cull,
                   const struct WlDrawLayout *draws, size_t viewRuns, uint32_t records, uint32_t sectionIndices,
                   struct WlCullFrame *out, uint32_t groups[2])
{
    uint32_t views = frustum ? 1u : WL_CASCADES;

    memset(out, 0, sizeof *out);
    if (frustum)
        memcpy(out->planes, frustum->plane, sizeof out->planes);
    wl_groups(words((uint64_t)runs * views), groups);
    out->counts[0] = runs;
    out->counts[1] = groups[0];
    out->counts[2] = views;
    out->counts[3] = frustum ? 0u : 1u;
    out->layout[0] = frustum ? cull->runs : cull->casterRuns;
    out->layout[1] = frustum ? WL_NO_ORDER : cull->order;
    out->layout[2] = frustum ? wl_view_command(0) : wl_caster_command(viewRuns, runs, 0, 0);
    out->layout[3] = draws->places;
    out->sections[0] = frustum ? 0u : 1u;
    out->sections[1] = sectionIndices;
    out->sections[2] = records;
}

void wl_compact_frame(uint32_t chunks, uint32_t first, uint32_t sections, const struct WlCullLayout *cull,
                      const struct WlDrawLayout *draws, uint32_t records, struct WlCompactFrame *out,
                      uint32_t groups[2])
{
    memset(out, 0, sizeof *out);
    wl_groups(words((uint64_t)chunks * sections), groups);
    out->counts[0] = chunks;
    out->counts[1] = groups[0];
    out->counts[2] = sections;
    out->counts[3] = first;
    out->layout[0] = cull->chunks;
    out->layout[1] = draws->places;
    out->layout[2] = records;
}

void wl_transform_frame(const float camera[12], const float projection[16], uint32_t slots,
                        struct WlTransformFrame *out)
{
    memset(out, 0, sizeof *out);
    memcpy(out->camera, camera, sizeof out->camera);
    memcpy(out->projection, projection, sizeof out->projection);
    out->counts[0] = slots;
}
