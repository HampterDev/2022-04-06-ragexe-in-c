#include "render/shadow_fit_gpu.h"

#include <math.h>
#include <string.h>

void shadow_fit_gpu_frame(const struct ShadowFitFrame *frame, int width, int height, const float *ambient,
                          float darkness, struct ShadowFitGpuFrame *out)
{
    int i;

    memset(out, 0, sizeof *out);
    memcpy(out->pixelX, frame->pixel[0], sizeof out->pixelX);
    memcpy(out->pixelY, frame->pixel[1], sizeof out->pixelY);
    out->screen[0] = frame->centre[0];
    out->screen[1] = frame->centre[1];
    out->screen[2] = frame->size[0];
    out->screen[3] = frame->size[1];
    out->depths[0] = frame->nearZ;
    out->depths[1] = frame->reach;
    out->depths[2] = frame->lightNear;
    out->depths[3] = frame->lightRange;
    out->size[0] = width > 0 ? (uint32_t)width : 0;
    out->size[1] = height > 0 ? (uint32_t)height : 0;
    for (i = 0; i < 3; i++) {
        out->axisX[i] = frame->axis[0][i];
        out->axisY[i] = frame->axis[1][i];
        out->axisZ[i] = frame->axis[2][i];
        out->lightView[i] = frame->lightView[i];
        out->ambient[i] = ambient[i] < 0.0f ? 0.0f : ambient[i] > 1.0f ? 1.0f : ambient[i];
    }
    memcpy(out->inverse, frame->inverse, sizeof out->inverse);
    out->lightView[3] = frame->bias;
    out->ambient[3] = darkness > 1.0f / 255.0f ? darkness : 1.0f / 255.0f;
}

uint32_t shadow_fit_gpu_order(float x)
{
    uint32_t u;

    memcpy(&u, &x, 4);
    return u & 0x80000000u ? ~u : u | 0x80000000u;
}

float shadow_fit_gpu_unorder(uint32_t u)
{
    float x;

    u = u & 0x80000000u ? u & 0x7fffffffu : ~u;
    memcpy(&x, &u, 4);
    return x;
}

void shadow_fit_gpu_state(uint32_t *state)
{
    const float infinity = HUGE_VALF;
    int i;

    /* the depths' own bits, which order as they do while they are positive */
    memcpy(&state[0], &infinity, 4);
    state[1] = 0;
    for (i = 0; i < 4 * SHADOW_FIT_CASCADES; i++)
        state[SHADOW_FIT_GPU_EXTENTS + i] = shadow_fit_gpu_order(i & 2 ? -HUGE_VALF : HUGE_VALF);
}

void shadow_fit_gpu_groups(uint32_t width, uint32_t height, uint32_t groups[2])
{
    groups[0] = width / SHADOW_FIT_GPU_TILE + (width % SHADOW_FIT_GPU_TILE != 0);
    groups[1] = height / SHADOW_FIT_GPU_TILE + (height % SHADOW_FIT_GPU_TILE != 0);
}

void shadow_fit_gpu_read(const uint32_t *state, const struct ShadowFitGpuCascades *result, struct ShadowFitGpuRead *out)
{
    int c, a;

    memset(out, 0, sizeof *out);
    memcpy(&out->range.nearest, &state[0], 4);
    memcpy(&out->range.farthest, &state[1], 4);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
        for (a = 0; a < 2; a++) {
            out->extents[c].lo[a] = shadow_fit_gpu_unorder(state[SHADOW_FIT_GPU_EXTENTS + 4 * c + a]);
            out->extents[c].hi[a] = shadow_fit_gpu_unorder(state[SHADOW_FIT_GPU_EXTENTS + 4 * c + 2 + a]);
            out->cascades.lo[c][a] = result->square[c][a];
            out->cascades.hi[c][a] = result->square[c][2 + a];
        }
        memcpy(out->cascades.world[c], result->world[c], sizeof result->world[c]);
        memcpy(out->cascades.fromView[c], result->fromView[c], sizeof result->fromView[c]);
        out->cascades.farDepth[c] = result->farDepth[c];
        out->cascades.offset[c] = result->offset[c];
        out->cascades.blur[c] = result->blur[c];
        out->cascades.slant[c] = result->slant[c];
        out->cascades.kept[c] = result->kept[c];
        out->previous[c] = result->previous[c];
    }
    out->kept.nearest = result->range[0];
    out->kept.farthest = result->range[1];
    out->previousRange.nearest = result->range[2];
    out->previousRange.farthest = result->range[3];
    out->cascades.cascades = (int)result->count[0];
}
