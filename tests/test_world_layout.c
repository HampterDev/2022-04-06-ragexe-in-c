/* custom: the static world as the device's shaders read it (render/world_layout.h): every field of
   a record packed where world.hlsli, world_cull.hlsl and world_compact.hlsl read it and back, ids no
   record has left alone; the runs the cull compacts, those of the passes up to the ground, each
   behind the one before and none past 2^32 - 1 indices; the chunks the compaction copies, every
   index of every record of those runs in one chunk, in order, and none of the others', up to a
   record of 2^32 - 1 indices; the slots' links; the projection a view-space point lands on the
   screen through, at the pixel renderer_project_vertex puts it on; the uniforms; and the
   dispatches' groups split across and down, to the last of 2^32 - 1.
   Fuzzed with a fixed seed. tests/test_static_world_maps.c runs the shaders on what this packs. */
#include "render/world_layout.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static uint32_t seed = 0x9e3779b9u;

static uint32_t next(void)
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

static float bits_float(uint32_t u)
{
    float f;

    memcpy(&f, &u, 4);
    return f;
}

static void random_record(struct SwRecord *r, uint32_t id)
{
    int k;

    memset(r, 0, sizeof *r);
    r->firstIndex = next();
    r->indexCount = next() % 9000 * 3;
    r->baseVertex = next();
    r->id = id;
    r->textureLayer = (uint16_t)next();
    r->lightmapLayer = (uint16_t)next();
    r->transform = (uint16_t)next();
    r->uv = (uint16_t)next();
    r->textureArray = (uint8_t)next();
    r->lightmapArray = (uint8_t)next();
    r->pass = (uint8_t)(next() % SW_PASS_COUNT);
    r->flags = (uint8_t)(next() & 7);
    for (k = 0; k < 3; k++) {
        r->centre[k] = bits_float(next());
        r->half[k] = (float)(next() % 1000) * 0.25f;
    }
}

static void test_records(void)
{
    static struct SwRecord records[200];
    static struct WlRecord byId[400];
    static struct WlCullRecord cull[200];
    int round, i, k;

    for (round = 0; round < 100; round++) {
        int n = (int)(next() % 200);

        memset(byId, 0xab, sizeof byId);
        for (i = 0; i < n; i++)
            random_record(&records[i], (uint32_t)(i * 2 + 1));
        wl_records(records, (size_t)n, byId, cull);
        for (i = 0; i < n; i++) {
            const struct SwRecord *r = &records[i];
            const struct WlRecord *w = &byId[r->id];
            const struct WlCullRecord *c = &cull[i];

            /* world.hlsli: record_slot, record_uv, record_pass, the layers and the arrays */
            assert((w->data[0] & 0xffff) == r->textureLayer && w->data[0] >> 16 == r->lightmapLayer);
            assert((w->data[1] & 0xffff) == r->transform && w->data[1] >> 16 == r->uv);
            assert((w->data[2] & 0xff) == r->pass && (w->data[2] >> 8 & 0xff) == r->flags);
            assert((w->data[2] >> 16 & 0xff) == r->textureArray && w->data[2] >> 24 == r->lightmapArray);
            assert(w->data[3] == 0);
            /* world_cull.hlsl: the bounds' bits, slot | flags << 16, the first index, the count, the
               base vertex */
            for (k = 0; k < 3; k++) {
                assert(!memcmp(&c->centre[k], &r->centre[k], 4) && !memcmp(&c->half[k], &r->half[k], 4));
            }
            assert(c->centre[3] == (r->transform | (uint32_t)r->flags << 16) && c->half[3] == r->firstIndex);
            assert(c->draw[0] == r->indexCount && c->draw[1] == r->baseVertex && !c->draw[2] && !c->draw[3]);
            /* the id no record has keeps what it held */
            assert(byId[r->id - 1].data[0] == 0xabababab);
        }
    }
}

static void test_runs(void)
{
    static struct SwRecord records[300];
    static struct SwDrawRun runs[300];
    static struct WlRun out[300];
    static struct WlChunk chunks[300 * 3];
    int round, i;

    for (round = 0; round < 200; round++) {
        int n = (int)(next() % 300);
        size_t nruns, m, indirect = 0, nchunks, c = 0, held;
        uint32_t total, at = 0, groups[2], rec;
        struct WlCompactFrame u;

        for (i = 0; i < n; i++) {
            random_record(&records[i], (uint32_t)i);
            records[i].indexCount = next() % 64 * 3;
            records[i].textureArray = (uint8_t)(next() % 3);
            records[i].lightmapArray = (uint8_t)(next() % 2);
        }
        sw_records_sort(records, (size_t)n);
        nruns = sw_draw_runs(records, (size_t)n, runs);
        m = wl_runs(runs, nruns, out, &total);
        while (indirect < nruns && runs[indirect].pass <= SW_PASS_GROUND)
            indirect++;
        assert(m == indirect);
        for (i = 0; i < (int)m; i++) {
            assert(out[i].first == runs[i].first && out[i].count == runs[i].count);
            assert(out[i].start == at && out[i].indices == runs[i].indices);
            at += runs[i].indices;
        }
        assert(total == at);
        /* the chunks of the runs' records: each record's indices 64 at a time, in order */
        held = m ? out[m - 1].first + out[m - 1].count : 0;
        nchunks = wl_chunks(records, held, NULL);
        assert(wl_chunks(records, held, chunks) == nchunks);
        for (rec = 0; rec < held; rec++) {
            uint32_t from;

            for (from = 0; from < records[rec].indexCount; from += WL_CHUNK) {
                assert(c < nchunks && chunks[c].record == rec && chunks[c].offset == from);
                c++;
            }
        }
        assert(c == nchunks);
        /* the compaction: a group for each chunk */
        wl_compact_frame((uint32_t)nchunks, &u, groups);
        assert(u.counts[0] == nchunks && groups[0] == nchunks && groups[1] == (nchunks ? 1u : 0u));
        assert(u.counts[1] == groups[0] && !u.counts[2] && !u.counts[3]);
    }
    /* a record of 2^32 - 1 indices: 2^26 chunks, the last of 63 indices, the count not wrapping */
    {
        struct SwRecord huge;

        memset(&huge, 0, sizeof huge);
        huge.indexCount = 0xffffffffu;
        assert(wl_chunks(&huge, 1, NULL) == 0x4000000u);
        huge.indexCount = 0;
        assert(wl_chunks(&huge, 1, NULL) == 0);
    }
    /* runs whose indices would pass 2^32 - 1 are left out */
    {
        struct SwDrawRun big[3];

        memset(big, 0, sizeof big);
        big[0].indices = 0x80000000u;
        big[1].indices = 0x7fffffffu;
        big[2].indices = 1;
        assert(wl_runs(big, 3, out, &(uint32_t){0}) == 2);
        assert(out[1].start == 0x80000000u);
    }
}

static void test_slots(void)
{
    float local[12];
    struct WlSlot s;
    int k;

    for (k = 0; k < 12; k++)
        local[k] = (float)k * 1.5f;
    wl_slot(local, 7, -1, 0, &s);
    assert(!memcmp(s.local, local, sizeof local) && s.link[0] == 7 && s.link[1] == 0 && s.link[2] == -1 && !s.link[3]);
    wl_slot(local, -1, 3, 1, &s);
    assert(s.link[0] == -1 && s.link[1] == 1 && s.link[2] == 3);
}

/* a view-space point through wl_view_clip and wl_projection, divided and moved by the viewport as
   the GPU does, lands on the pixel the CPU's projection puts it on (render/rp_project.c: x * proj
   / z + centre), within the float rounding of the two ways */
static void test_projection(void)
{
    int round, k;

    for (round = 0; round < 10000; round++) {
        float proj[2] = {(float)(200 + next() % 900), (float)(200 + next() % 900)};
        int w = 320 + (int)(next() % 2000), h = 240 + (int)(next() % 1400);
        float centre[2] = {(float)(w / 2), (float)(h / 2)}, nearZ = 10.0f, depthScale = 40000.0f / 39990.0f;
        float view[12] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0}, p[16], clip[16], v[3], out[4];
        double x, y, want[2];

        wl_projection(p, proj, centre, nearZ, depthScale, w, h);
        wl_view_clip(clip, view, p);
        v[0] = (float)((int)(next() % 4001) - 2000);
        v[1] = (float)((int)(next() % 4001) - 2000);
        v[2] = (float)(20 + next() % 30000);
        for (k = 0; k < 4; k++)
            out[k] = v[0] * clip[k] + v[1] * clip[4 + k] + v[2] * clip[8 + k] + clip[12 + k];
        assert(out[3] == v[2]);
        x = ((double)out[0] / out[3] + 1.0) * 0.5 * w;
        y = (1.0 - (double)out[1] / out[3]) * 0.5 * h;
        want[0] = (double)v[0] * proj[0] / v[2] + centre[0];
        want[1] = (double)v[1] * proj[1] / v[2] + centre[1];
        assert(fabs(x - want[0]) < 1e-3 * (1 + fabs(want[0])) && fabs(y - want[1]) < 1e-3 * (1 + fabs(want[1])));
        /* the depth: 0 at the near plane, 1 at the far one */
        assert(fabs((double)out[2] / out[3] - (v[2] - nearZ) * depthScale / v[2]) < 1e-5);
    }
}

static void test_uniforms(void)
{
    struct SwFrustum f;
    struct WlCullFrame c;
    struct WlTransformFrame t;
    float camera[12], projection[16], from[WL_CASCADES][16];
    uint32_t groups[2];
    int k;

    for (k = 0; k < 24; k++)
        f.plane[k / 4][k % 4] = (float)k;
    wl_cull_frame(&f, 17, &c, groups);
    assert(!memcmp(c.planes, f.plane, sizeof c.planes) && c.counts[0] == 17 && c.counts[1] == 17);
    assert(groups[0] == 17 && groups[1] == 1 && !c.counts[2] && !c.counts[3]);
    wl_cull_frame(&f, 70000, &c, groups);
    assert(c.counts[0] == 70000 && c.counts[1] == WL_GROUPS_ACROSS && groups[1] == 2);
    for (k = 0; k < 12; k++)
        camera[k] = (float)k;
    for (k = 0; k < 16; k++)
        projection[k] = (float)-k;
    for (k = 0; k < WL_CASCADES * 16; k++)
        from[k / 16][k % 16] = (float)k + 0.5f;
    wl_transform_frame(camera, projection, (const float (*)[16])from, 3, 99, &t);
    assert(!memcmp(t.camera, camera, sizeof camera) && !memcmp(t.projection, projection, sizeof projection));
    assert(!memcmp(t.fromView, from, 3 * sizeof from[0]) && t.fromView[3][0] == 0.0f);
    assert(t.counts[0] == 99 && t.counts[1] == 3);
    wl_transform_frame(camera, projection, (const float (*)[16])from, 9, 1, &t);
    assert(t.counts[1] == WL_CASCADES && t.fromView[3][0] == from[3][0]);
}

/* a dispatch's groups across and down: as few rows as hold them, none past the limit, numbered
   without a gap; and the compaction's of 2^26 + 1 chunks */
static void test_groups(void)
{
    static const uint32_t edges[] = { 0, 1, 2, 63, 64, 65534, 65535, 65536, 131070, 131071,
                                      0xfffffffeu, 0xffffffffu };
    struct WlCompactFrame u;
    uint32_t g[2], i;

    for (i = 0; i < sizeof edges / sizeof edges[0] + 100000; i++) {
        uint32_t n = i < sizeof edges / sizeof edges[0] ? edges[i] : next() >> (next() % 32);
        uint64_t whole;

        wl_groups(n, g);
        whole = (uint64_t)g[0] * g[1];
        assert(g[0] <= WL_GROUPS_ACROSS && g[1] <= WL_GROUPS_ACROSS + 2);
        if (!n) {
            assert(!g[0] && !g[1]);
            continue;
        }
        assert(whole >= n && whole - g[0] < n);
        assert(g[1] == 1 ? g[0] == n : g[0] == WL_GROUPS_ACROSS);
    }
    wl_compact_frame(0, &u, g);
    assert(!u.counts[0] && !u.counts[1] && !g[0] && !g[1]);
    wl_compact_frame(0x4000001u, &u, g);
    assert(u.counts[0] == 0x4000001u && u.counts[1] == WL_GROUPS_ACROSS && g[0] == WL_GROUPS_ACROSS);
    assert((uint64_t)g[0] * g[1] >= 0x4000001u && (uint64_t)g[0] * (g[1] - 1) < 0x4000001u);
}

int main(void)
{
    test_records();
    test_runs();
    test_slots();
    test_projection();
    test_uniforms();
    test_groups();
    puts("world_layout: records, runs, chunks, slots, projection, uniforms and groups passed");
    return 0;
}
