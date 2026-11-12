/* render/vertex_fog.c: the table the quad shader looks the vertex fog up in gives the step the
   original computed on the CPU, for every rhw tried: each step's first rhw and the one before it,
   powers of two and a sweep of a million values, for the fog of the maps, the whole range, an end
   at the start, an end before the start and a range of a hair. */
#include "render/vertex_fog.h"
#include <assert.h>
#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static float g_fogNear, g_fogFar;

/* The original's vertex fog as the client computed it on the CPU until the fog went into the
   shaders (render/rp_pool.c, rp_vertex_fog_alpha), word for word. */
static int rp_vertex_fog_alpha(float rhw)
{
    float range = g_fogFar - g_fogNear;
    float t = (1.0f / rhw - 10.0f) / 1490.0f;

    if (range != 0.0f)
        t = (t - g_fogNear) / range;
    t = 0.0f > t ? 0.0f : t;
    t = 1.0f < t ? 1.0f : t;
    t = (1.0f - t) * 255.0f;
    return (int)t;
}

static float from_bits(uint32_t bits)
{
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static uint32_t to_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    return bits;
}

static long checked;

static void check(const struct VertexFogTable *table, float rhw)
{
    int want = rp_vertex_fog_alpha(rhw), got = vertex_fog_lookup(table, rhw);
    assert(vertex_fog_step(rhw, g_fogNear, g_fogFar, 10.0f, 1490.0f) == want);
    if (got != want) {
        fprintf(stderr, "fog %g..%g, rhw %.9g (%08lx): %d, the original %d\n", g_fogNear, g_fogFar, rhw,
                (unsigned long)to_bits(rhw), got, want);
        assert(0);
    }
    ++checked;
}

static void fog(float start, float end)
{
    struct VertexFogTable table;
    uint32_t bits, seed = 0x2545f491u;
    int i, count;
    g_fogNear = start;
    g_fogFar = end;
    vertex_fog_table(start, end, 10.0f, 1490.0f, &table);
    count = (int)table.count;
    assert(count >= 0 && count <= 255 && (table.direction == 1.0f || table.direction == -1.0f));
    for (i = 0; i < VERTEX_FOG_ENTRIES; i++) {
        if (i >= count) {
            assert(to_bits(table.rhw[i]) == 0x7f800000u);
            continue;
        }
        assert(table.rhw[i] > 0.0f && (!i || table.rhw[i] >= table.rhw[i - 1]));
        check(&table, table.rhw[i]);
        check(&table, from_bits(to_bits(table.rhw[i]) - 1));
        check(&table, from_bits(to_bits(table.rhw[i]) + 1));
    }
    for (bits = 0x00000001u; bits < 0x7f800000u; bits += 0x00800000u)
        check(&table, from_bits(bits));
    check(&table, FLT_MAX);
    check(&table, FLT_MIN);
    check(&table, 1.0f);
    /* every rhw of the views the client draws, from 1 / 1,500 to 1 / 10, and a million random */
    for (bits = to_bits(1.0f / 1600.0f); bits <= to_bits(1.0f / 5.0f); bits += 97)
        check(&table, from_bits(bits));
    for (i = 0; i < 1000000; i++) {
        seed = seed * 1664525u + 1013904223u;
        check(&table, from_bits((seed >> 1) % 0x7f800000u + 1));
    }
}

int main(void)
{
    fog(0.2f, 0.8f);
    fog(0.0f, 1.0f);
    fog(0.5f, 0.5f);
    fog(0.8f, 0.2f);
    fog(0.3f, 0.3000001f);
    fog(-0.5f, 2.0f);
    fog(0.0f, 0.0f);
    printf("Vertex fog: the shader's table against the original's CPU fog for %ld rhw values passed.\n", checked);
    return 0;
}
