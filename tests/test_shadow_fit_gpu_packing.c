/* render/shadow_fit_gpu.c: the shadow fit as the device's compute passes read and write it. The
   layouts are the shaders' (src/render/shaders/shadow_fit.hlsli, Frame; cascades.hlsli, Cascades:
   16-byte vectors and row-major matrices, each member where they put it); a frame's constants land
   in their places, the ambient light clamped and the darkness at least 1 / 255; a float's order as
   a uint is its order as a float, -0 below +0 and the infinities at the ends, and comes back as it
   went; the state a frame starts from is nothing seen; and what the passes leave reads back into
   the C's terms. */
#include "render/shadow_fit_gpu.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static void test_layouts(void)
{
    assert(sizeof(struct ShadowFitGpuFrame) == 13 * 16);
    assert(offsetof(struct ShadowFitGpuFrame, pixelY) == 16 && offsetof(struct ShadowFitGpuFrame, screen) == 32);
    assert(offsetof(struct ShadowFitGpuFrame, depths) == 48 && offsetof(struct ShadowFitGpuFrame, size) == 64);
    assert(offsetof(struct ShadowFitGpuFrame, axisX) == 80 && offsetof(struct ShadowFitGpuFrame, axisZ) == 112);
    assert(offsetof(struct ShadowFitGpuFrame, inverse) == 128 && offsetof(struct ShadowFitGpuFrame, lightView) == 176);
    assert(offsetof(struct ShadowFitGpuFrame, ambient) == 192);
    assert(sizeof(struct ShadowFitGpuCascades) == 1120);
    assert(offsetof(struct ShadowFitGpuCascades, fromView) == 256 && offsetof(struct ShadowFitGpuCascades, farDepth) == 512);
    assert(offsetof(struct ShadowFitGpuCascades, square) == 576 && offsetof(struct ShadowFitGpuCascades, kept) == 640);
    assert(offsetof(struct ShadowFitGpuCascades, previous) == 656 && offsetof(struct ShadowFitGpuCascades, range) == 672);
    assert(offsetof(struct ShadowFitGpuCascades, planes) == 688 && offsetof(struct ShadowFitGpuCascades, lightView) == 1072);
    assert(offsetof(struct ShadowFitGpuCascades, count) == 1104);
    assert(SHADOW_FIT_GPU_STATE == 18 && SHADOW_FIT_GPU_HISTORY == 8);
}

static void test_frame(void)
{
    struct ShadowFitFrame f;
    struct ShadowFitGpuFrame u;
    static const float ambient[3] = {-0.5f, 0.25f, 3.0f};
    int i;

    memset(&f, 0, sizeof f);
    for (i = 0; i < 4; i++) {
        f.pixel[0][i] = 1.0f + (float)i;
        f.pixel[1][i] = 10.0f + (float)i;
    }
    for (i = 0; i < 3; i++) {
        f.axis[0][i] = 20.0f + (float)i;
        f.axis[1][i] = 30.0f + (float)i;
        f.axis[2][i] = 40.0f + (float)i;
        f.lightView[i] = 50.0f + (float)i;
    }
    for (i = 0; i < 12; i++)
        f.inverse[i] = 60.0f + (float)i;
    f.centre[0] = 800.0f;
    f.centre[1] = 450.0f;
    f.size[0] = 1600.0f;
    f.size[1] = 900.0f;
    f.nearZ = 10.0f;
    f.reach = 2355.0f;
    f.lightNear = -120.5f;
    f.lightRange = 640.0f;
    f.bias = 0.00625f;
    shadow_fit_gpu_frame(&f, 1600, 900, ambient, 0.0f, &u);
    assert(!memcmp(u.pixelX, f.pixel[0], sizeof u.pixelX) && !memcmp(u.pixelY, f.pixel[1], sizeof u.pixelY));
    assert(u.screen[0] == 800.0f && u.screen[1] == 450.0f && u.screen[2] == 1600.0f && u.screen[3] == 900.0f);
    assert(u.depths[0] == 10.0f && u.depths[1] == 2355.0f && u.depths[2] == -120.5f && u.depths[3] == 640.0f);
    assert(u.size[0] == 1600 && u.size[1] == 900 && u.size[2] == 0 && u.size[3] == 0);
    for (i = 0; i < 3; i++) {
        assert(u.axisX[i] == f.axis[0][i] && u.axisY[i] == f.axis[1][i] && u.axisZ[i] == f.axis[2][i]);
        assert(u.lightView[i] == f.lightView[i]);
    }
    assert(u.axisX[3] == 0.0f && u.axisY[3] == 0.0f && u.axisZ[3] == 0.0f);
    assert(!memcmp(u.inverse, f.inverse, sizeof u.inverse));
    assert(u.lightView[3] == f.bias);
    assert(u.ambient[0] == 0.0f && u.ambient[1] == 0.25f && u.ambient[2] == 1.0f && u.ambient[3] == 1.0f / 255.0f);
    shadow_fit_gpu_frame(&f, -3, 0, ambient, 0.5f, &u);
    assert(u.size[0] == 0 && u.size[1] == 0 && u.ambient[3] == 0.5f);
}

static void test_order(void)
{
    static const float sorted[] = {-INFINITY, -3.4e38f, -1.0f, -1e-38f, -1e-45f, -0.0f, 0.0f, 1e-45f, 1e-38f, 1.0f,
                                   3.4e38f, INFINITY};
    unsigned i, j;
    float x = -1000.0f;

    for (i = 0; i < sizeof sorted / sizeof sorted[0]; i++) {
        float back = shadow_fit_gpu_unorder(shadow_fit_gpu_order(sorted[i]));

        assert(!memcmp(&back, &sorted[i], 4));
        for (j = 0; j < i; j++)
            assert(shadow_fit_gpu_order(sorted[j]) < shadow_fit_gpu_order(sorted[i]));
    }
    /* floats from -1000 to 1000, each and the next one up, a ten-thousandth apart */
    while (x < 1000.0f) {
        float next = x + fabsf(x) * 1e-4f + 1e-3f;

        assert(shadow_fit_gpu_order(x) < shadow_fit_gpu_order(nextafterf(x, INFINITY)));
        assert(shadow_fit_gpu_order(x) < shadow_fit_gpu_order(next));
        assert(shadow_fit_gpu_unorder(shadow_fit_gpu_order(x)) == x);
        x = next;
    }
}

static void test_state_and_read(void)
{
    uint32_t state[SHADOW_FIT_GPU_STATE];
    struct ShadowFitGpuCascades g;
    struct ShadowFitGpuRead r;
    float nearest = 12.5f, farthest = 980.0f;
    int c, a, i;

    shadow_fit_gpu_state(state);
    memset(&g, 0, sizeof g);
    shadow_fit_gpu_read(state, &g, &r);
    assert(r.range.nearest == INFINITY && r.range.farthest == 0.0f);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        for (a = 0; a < 2; a++)
            assert(r.extents[c].lo[a] == INFINITY && r.extents[c].hi[a] == -INFINITY);
    /* the passes' results */
    memcpy(&state[0], &nearest, 4);
    memcpy(&state[1], &farthest, 4);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        for (a = 0; a < 4; a++)
            state[SHADOW_FIT_GPU_EXTENTS + 4 * c + a] = shadow_fit_gpu_order((float)(c * 100 + a) - 150.0f);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
        for (i = 0; i < 16; i++) {
            g.world[c][i] = (float)(c * 16 + i);
            g.fromView[c][i] = (float)(c * 16 + i) + 0.5f;
        }
        for (a = 0; a < 4; a++)
            g.square[c][a] = (float)(c * 10 + a);
        g.farDepth[c] = 100.0f * (float)(c + 1);
        g.offset[c] = 0.25f * (float)c;
        g.blur[c] = 0.125f * (float)c;
        g.slant[c] = 0.0625f * (float)c;
        g.kept[c] = 8.0f * (float)(c + 1);
        g.previous[c] = 16.0f * (float)(c + 1);
    }
    g.range[0] = 12.0f;
    g.range[1] = 990.0f;
    g.range[2] = 11.0f;
    g.range[3] = 995.0f;
    g.count[0] = SHADOW_FIT_CASCADES;
    shadow_fit_gpu_read(state, &g, &r);
    assert(r.range.nearest == 12.5f && r.range.farthest == 980.0f);
    assert(r.kept.nearest == 12.0f && r.kept.farthest == 990.0f);
    assert(r.previousRange.nearest == 11.0f && r.previousRange.farthest == 995.0f);
    assert(r.cascades.cascades == SHADOW_FIT_CASCADES);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
        for (a = 0; a < 2; a++) {
            assert(r.extents[c].lo[a] == (float)(c * 100 + a) - 150.0f);
            assert(r.extents[c].hi[a] == (float)(c * 100 + 2 + a) - 150.0f);
            assert(r.cascades.lo[c][a] == g.square[c][a] && r.cascades.hi[c][a] == g.square[c][2 + a]);
        }
        assert(!memcmp(r.cascades.world[c], g.world[c], sizeof g.world[c]));
        assert(!memcmp(r.cascades.fromView[c], g.fromView[c], sizeof g.fromView[c]));
        assert(r.cascades.farDepth[c] == g.farDepth[c] && r.cascades.offset[c] == g.offset[c]);
        assert(r.cascades.blur[c] == g.blur[c] && r.cascades.slant[c] == g.slant[c]);
        assert(r.cascades.kept[c] == g.kept[c] && r.previous[c] == g.previous[c]);
    }
}

/* the range's and the extents' groups: a tile each, the last one partly outside the targets, to the
   largest size without wrapping */
static void test_groups(void)
{
    uint32_t g[2];

    shadow_fit_gpu_groups(0, 0, g);
    assert(g[0] == 0 && g[1] == 0);
    shadow_fit_gpu_groups(1, SHADOW_FIT_GPU_TILE, g);
    assert(g[0] == 1 && g[1] == 1);
    shadow_fit_gpu_groups(SHADOW_FIT_GPU_TILE + 1, 2 * SHADOW_FIT_GPU_TILE, g);
    assert(g[0] == 2 && g[1] == 2);
    shadow_fit_gpu_groups(1601, 901, g);
    assert(g[0] == 51 && g[1] == 29);
    shadow_fit_gpu_groups(0xffffffffu, 0xffffffe0u, g);
    assert(g[0] == 0x8000000u && g[1] == 0x7ffffffu);
}

int main(void)
{
    test_layouts();
    test_frame();
    test_order();
    test_state_and_read();
    test_groups();
    puts("shadow fit packing: the shaders' layouts, a frame's constants, floats ordered as uints and back, the "
         "state of nothing seen, the passes' results read back and their groups passed");
    return 0;
}
