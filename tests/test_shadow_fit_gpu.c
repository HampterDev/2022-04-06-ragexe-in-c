/* The shadow fit's compute passes (src/render/shaders/shadow_range.hlsl, shadow_extents.hlsl,
   shadow_cascades.hlsl) on this machine's GPU, against render/shadow_fit.c on the same depth
   buffers (tests/shadow_fit_scenes.h), blended receivers before the ground among them, and from
   what a frame before kept, packed and read back as the client does (render/shadow_fit_gpu.h): each
   pass is checked against its step of the C given the GPU's own input to it. The reductions, the
   range kept, the squares, the sizes kept and the snapped translations must match to the bit, on
   the half-texel boundary too, where the snap does not depend on how the GPU divides; the rest is
   ordinary shader arithmetic and may differ by a GPU's rounding of a division or a square root,
   which the test counts, and the matrices must agree within 1e-5 of their rows' largest element;
   the GPU's own cascades must still hold every receiving point, and each cascade's box for the
   casters' cull (Cascades.planes) must be its square and the light's depth. Exits 77 (skipped)
   without a GPU. */
#include "shadow_fit_scenes.h"
#include "render/shadow_fit_gpu.h"
#include "render/shaders/shader_code.h"
#include "render/shaders/shadow_cascades.h"
#include "render/shaders/shadow_extents.h"
#include "render/shaders/shadow_range.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STATE_SIZE SHADOW_FIT_GPU_STATE
#define HISTORY_BYTES (SHADOW_FIT_GPU_HISTORY * 4)

/* what a frame keeps for the next: the squares, then the range (render/shadow_fit_gpu.h, the
   history's floats) */
struct Kept {
    float squares[SHADOW_FIT_CASCADES];
    struct ShadowFitRange range;
    float unused[2];
};

static SDL_GPUDevice *dev;
static SDL_GPUComputePipeline *passes[3];
static SDL_GPUTexture *textures[2];
static SDL_GPUBuffer *stateBuffer, *historyBuffer, *resultBuffer;
static SDL_GPUTransferBuffer *upload, *download;

/* how often a value differed, and by at most how many units in the last place */
struct Ulps {
    double values, differ;
    unsigned most;
};

static struct Ulps splitUlps, matrixUlps, viewUlps, lookupUlps;
static double worstRelative;
/* the fits whose extents each slice's receivers reached */
static int sliceFits[SHADOW_FIT_CASCADES];

static unsigned ulps(float a, float b)
{
    int32_t x, y;

    memcpy(&x, &a, 4);
    memcpy(&y, &b, 4);
    x = x < 0 ? INT32_MIN - x : x;
    y = y < 0 ? INT32_MIN - y : y;
    return x > y ? (unsigned)(x - y) : (unsigned)(y - x);
}

static void count_ulps(struct Ulps *u, const float *gpu, const float *c, int n, unsigned allowed)
{
    int i;

    for (i = 0; i < n; i++) {
        unsigned d = ulps(gpu[i], c[i]);

        u->values++;
        u->differ += d != 0;
        u->most = d > u->most ? d : u->most;
        assert(d <= allowed);
    }
}

/* a 4x4's rows within 1e-5 of each row's largest element */
static void count_rows(const float *gpu, const float *c)
{
    int i, j;

    for (i = 0; i < 4; i++) {
        double scale = 0.0;

        for (j = 0; j < 4; j++)
            scale = fmax(scale, fabs(c[i * 4 + j]));
        for (j = 0; j < 4; j++) {
            double d = fabs((double)gpu[i * 4 + j] - c[i * 4 + j]) / (scale > 0.0 ? scale : 1.0);

            worstRelative = d > worstRelative ? d : worstRelative;
            assert(d <= 1e-5);
        }
    }
}

static SDL_GPUComputePipeline *pipeline(const struct GpuComputeCode *c)
{
    SDL_GPUComputePipelineCreateInfo info;
    SDL_GPUComputePipeline *p;

    SDL_zero(info);
#ifdef __APPLE__
    info.code = c->code.msl;
    info.code_size = c->code.mslSize;
    info.entrypoint = "main0";
    info.format = SDL_GPU_SHADERFORMAT_MSL;
#else
    info.code = c->code.spirv;
    info.code_size = c->code.spirvSize;
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
#endif
    info.num_samplers = c->code.samplers;
    info.num_readonly_storage_textures = c->code.storageTextures;
    info.num_readonly_storage_buffers = c->code.storageBuffers;
    info.num_readwrite_storage_textures = c->readwriteStorageTextures;
    info.num_readwrite_storage_buffers = c->readwriteStorageBuffers;
    info.num_uniform_buffers = c->code.uniformBuffers;
    info.threadcount_x = c->threads[0];
    info.threadcount_y = c->threads[1];
    info.threadcount_z = c->threads[2];
    p = SDL_CreateGPUComputePipeline(dev, &info);
    if (!p)
        fprintf(stderr, "SDL_CreateGPUComputePipeline: %s\n", SDL_GetError());
    assert(p);
    return p;
}

static SDL_GPUBuffer *buffer(SDL_GPUBufferUsageFlags usage, Uint32 size)
{
    SDL_GPUBufferCreateInfo info;
    SDL_GPUBuffer *b;

    SDL_zero(info);
    info.usage = usage;
    info.size = size;
    b = SDL_CreateGPUBuffer(dev, &info);
    assert(b);
    return b;
}

static SDL_GPUTransferBuffer *transfer(SDL_GPUTransferBufferUsage usage, Uint32 size)
{
    SDL_GPUTransferBufferCreateInfo info;
    SDL_GPUTransferBuffer *t;

    SDL_zero(info);
    info.usage = usage;
    info.size = size;
    t = SDL_CreateGPUTransferBuffer(dev, &info);
    assert(t);
    return t;
}

static int open_device(void)
{
    SDL_PropertiesID props;
    SDL_GPUTextureCreateInfo info;
    int i;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 0;
    }
    props = SDL_CreateProperties();
#ifdef __APPLE__
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);
#else
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
#endif
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN, false);
    dev = SDL_CreateGPUDeviceWithProperties(props);
    SDL_DestroyProperties(props);
    if (!dev) {
        fprintf(stderr, "skipped: no GPU device: %s\n", SDL_GetError());
        return 0;
    }
    passes[0] = pipeline(&shadow_range_cs);
    passes[1] = pipeline(&shadow_extents_cs);
    passes[2] = pipeline(&shadow_cascades_cs);
    SDL_zero(info);
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ;
    info.width = SCENE_W;
    info.height = SCENE_H;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    for (i = 0; i < 2; i++) {
        textures[i] = SDL_CreateGPUTexture(dev, &info);
        assert(textures[i]);
    }
    stateBuffer = buffer(SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE,
                         STATE_SIZE * sizeof(uint32_t));
    historyBuffer = buffer(SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE,
                           HISTORY_BYTES);
    resultBuffer = buffer(SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE, sizeof(struct ShadowFitGpuCascades));
    upload = transfer(SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, 2 * SCENE_PIXELS * sizeof(float) + STATE_SIZE * 4 + HISTORY_BYTES);
    download = transfer(SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
                        sizeof(struct ShadowFitGpuCascades) + STATE_SIZE * 4 + HISTORY_BYTES);
    return 1;
}

static void close_device(void)
{
    int i;

    SDL_ReleaseGPUTransferBuffer(dev, upload);
    SDL_ReleaseGPUTransferBuffer(dev, download);
    for (i = 0; i < 2; i++)
        SDL_ReleaseGPUTexture(dev, textures[i]);
    SDL_ReleaseGPUBuffer(dev, stateBuffer);
    SDL_ReleaseGPUBuffer(dev, historyBuffer);
    SDL_ReleaseGPUBuffer(dev, resultBuffer);
    for (i = 0; i < 3; i++)
        SDL_ReleaseGPUComputePipeline(dev, passes[i]);
    SDL_DestroyGPUDevice(dev);
    SDL_Quit();
}

static void upload_texture(SDL_GPUCopyPass *copy, SDL_GPUTexture *t, uint32_t at, int width, int height)
{
    SDL_GPUTextureTransferInfo src;
    SDL_GPUTextureRegion dst;

    SDL_zero(src);
    SDL_zero(dst);
    src.transfer_buffer = upload;
    src.offset = at;
    src.pixels_per_row = (Uint32)width;
    src.rows_per_layer = (Uint32)height;
    dst.texture = t;
    dst.w = (Uint32)width;
    dst.h = (Uint32)height;
    dst.d = 1;
    SDL_UploadToGPUTexture(copy, &src, &dst, false);
}

/* The passes over the depth buffers from what the frame before kept: the fit's state after them,
   their result and what they keep. With `given`, the cascades pass alone, from that state. */
static void gpu_fit(const struct ShadowFitFrame *frame, const struct ShadowFitDepths *d, const struct Kept *before,
                    const uint32_t *given, uint32_t *state, struct ShadowFitGpuCascades *result, struct Kept *kept)
{
    static const float ambient[3] = {0.25f, 0.25f, 0.25f};
    struct ShadowFitGpuFrame u;
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    SDL_GPUComputePass *pass;
    SDL_GPUTransferBufferLocation from;
    SDL_GPUBufferRegion region;
    SDL_GPUStorageBufferReadWriteBinding rw[2];
    SDL_GPUFence *fence;
    uint32_t depthBytes = (uint32_t)(d->width * d->height) * sizeof(float), init[STATE_SIZE], groups[2];
    unsigned char *map;
    int p;

    if (given)
        memcpy(init, given, sizeof init);
    else
        shadow_fit_gpu_state(init);
    map = SDL_MapGPUTransferBuffer(dev, upload, false);
    assert(map);
    memcpy(map, d->front, depthBytes);
    memcpy(map + SCENE_PIXELS * sizeof(float), d->back, depthBytes);
    memcpy(map + 2 * SCENE_PIXELS * sizeof(float), init, sizeof init);
    memcpy(map + 2 * SCENE_PIXELS * sizeof(float) + sizeof init, before, HISTORY_BYTES);
    SDL_UnmapGPUTransferBuffer(dev, upload);
    shadow_fit_gpu_frame(frame, d->width, d->height, ambient, 0.5f, &u);
    shadow_fit_gpu_groups(u.size[0], u.size[1], groups);

    cmd = SDL_AcquireGPUCommandBuffer(dev);
    assert(cmd);
    copy = SDL_BeginGPUCopyPass(cmd);
    if (depthBytes) {
        upload_texture(copy, textures[0], 0, d->width, d->height);
        upload_texture(copy, textures[1], SCENE_PIXELS * sizeof(float), d->width, d->height);
    }
    from.transfer_buffer = upload;
    from.offset = 2 * SCENE_PIXELS * sizeof(float);
    region.buffer = stateBuffer;
    region.offset = 0;
    region.size = sizeof init;
    SDL_UploadToGPUBuffer(copy, &from, &region, false);
    from.offset += sizeof init;
    region.buffer = historyBuffer;
    region.size = HISTORY_BYTES;
    SDL_UploadToGPUBuffer(copy, &from, &region, false);
    SDL_EndGPUCopyPass(copy);

    for (p = 0; p < 2 && !given; p++) {
        SDL_zero(rw);
        rw[0].buffer = stateBuffer;
        pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 1);
        assert(pass);
        SDL_BindGPUComputePipeline(pass, passes[p]);
        SDL_BindGPUComputeStorageTextures(pass, 0, textures, 2);
        if (p == 1)
            SDL_BindGPUComputeStorageBuffers(pass, 0, &historyBuffer, 1);
        SDL_PushGPUComputeUniformData(cmd, 0, &u, sizeof u);
        SDL_DispatchGPUCompute(pass, groups[0], groups[1], 1);
        SDL_EndGPUComputePass(pass);
    }
    SDL_zero(rw);
    rw[0].buffer = historyBuffer;
    rw[1].buffer = resultBuffer;
    pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 2);
    assert(pass);
    SDL_BindGPUComputePipeline(pass, passes[2]);
    SDL_BindGPUComputeStorageBuffers(pass, 0, &stateBuffer, 1);
    SDL_PushGPUComputeUniformData(cmd, 0, &u, sizeof u);
    SDL_DispatchGPUCompute(pass, 1, 1, 1);
    SDL_EndGPUComputePass(pass);

    copy = SDL_BeginGPUCopyPass(cmd);
    region.buffer = stateBuffer;
    region.offset = 0;
    region.size = STATE_SIZE * sizeof(uint32_t);
    from.transfer_buffer = download;
    from.offset = 0;
    SDL_DownloadFromGPUBuffer(copy, &region, &from);
    region.buffer = resultBuffer;
    region.size = sizeof *result;
    from.offset = STATE_SIZE * sizeof(uint32_t);
    SDL_DownloadFromGPUBuffer(copy, &region, &from);
    region.buffer = historyBuffer;
    region.size = HISTORY_BYTES;
    from.offset = STATE_SIZE * sizeof(uint32_t) + sizeof *result;
    SDL_DownloadFromGPUBuffer(copy, &region, &from);
    SDL_EndGPUCopyPass(copy);
    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    assert(fence);
    assert(SDL_WaitForGPUFences(dev, true, &fence, 1));
    SDL_ReleaseGPUFence(dev, fence);
    map = SDL_MapGPUTransferBuffer(dev, download, false);
    assert(map);
    memcpy(state, map, STATE_SIZE * sizeof(uint32_t));
    memcpy(result, map + STATE_SIZE * sizeof(uint32_t), sizeof *result);
    memcpy(kept, map + STATE_SIZE * sizeof(uint32_t) + sizeof *result, HISTORY_BYTES);
    SDL_UnmapGPUTransferBuffer(dev, download);
}

/* a point's place along a plane (inside where it is not negative) */
static double side(const float *plane, const double *p)
{
    return plane[0] * p[0] + plane[1] * p[1] + plane[2] * p[2] + plane[3];
}

/* One receiving point lies in the GPU's cascade for its depth, inside the map by the soft edge and
   the lookup's offset, through either of its matrices, and inside that cascade's box for the
   casters' cull. */
static void check_point(const struct ShadowFitCamera *cam, const struct ShadowFitGpuCascades *g, double d,
                        const double *at)
{
    double v[3], l[4], lv[4], radius, margin;
    int c, i, j;

    for (c = 0; c < SHADOW_FIT_CASCADES && !(d < g->farDepth[c]); c++)
        ;
    assert(g->count[0] == SHADOW_FIT_CASCADES && c < SHADOW_FIT_CASCADES);
    radius = (g->square[c][2] - g->square[c][0]) * 0.5;
    margin = (0.5 + 1.5 * 2.0 * radius / SHADOW_FIT_SIZE) / radius;
    for (i = 0; i < 3; i++)
        v[i] = cam->view[i] * at[0] + cam->view[3 + i] * at[1] + cam->view[6 + i] * at[2] + cam->view[9 + i];
    for (j = 0; j < 4; j++) {
        l[j] = g->world[c][12 + j];
        lv[j] = g->fromView[c][12 + j];
        for (i = 0; i < 3; i++) {
            l[j] += at[i] * g->world[c][i * 4 + j];
            lv[j] += v[i] * g->fromView[c][i * 4 + j];
        }
    }
    assert(fabs(l[0]) <= 1.0 - margin && fabs(l[1]) <= 1.0 - margin && l[2] > 0.0 && l[2] < 1.0);
    assert(fabs(lv[0]) <= 1.0 - margin + 1e-5 && fabs(lv[1]) <= 1.0 - margin + 1e-5 && lv[2] > 0.0 && lv[2] < 1.0);
    for (i = 0; i < 6; i++)
        assert(side(g->planes[c * 6 + i], at) >= -0.01);
}

/* Every receiving point of the last frame scene_render drew, front and back, through the GPU's
   cascades (check_point). */
static void check_cover(const struct ShadowFitFrame *frame, const struct ShadowFitCamera *cam,
                        const struct ShadowFitGpuCascades *g)
{
    int p, s;

    for (p = 0; p < SCENE_PIXELS; p++)
        for (s = 0; s < 2; s++) {
            double d = s ? depth[p] : front[p];

            if (d >= frame->nearZ && d <= frame->reach && (!s || depth[p] > front[p]))
                check_point(cam, g, d, s ? hit[p] : frontHit[p]);
        }
}

/* Each pass against its step of the C, given the GPU's input to it, as the client reads it back
   (render/shadow_fit_gpu.h); with `given`, the cascades pass alone from that state; with cam, the
   frame scene_render drew last, whose points the GPU's cascades then hold. Answers what the GPU
   kept. */
static void compare(const struct ShadowFitFrame *frame, const struct ShadowFitDepths *d, const struct Kept *before,
                    const uint32_t *given, const struct ShadowFitCamera *cam, struct Kept *kept)
{
    struct ShadowFitRange range, keep;
    struct ShadowFitExtent ext[SHADOW_FIT_CASCADES];
    struct ShadowFitCascades c;
    struct ShadowFitGpuCascades g;
    struct ShadowFitGpuRead r;
    float split[SHADOW_FIT_CASCADES + 1], gpuSplit[SHADOW_FIT_CASCADES + 1];
    uint32_t state[STATE_SIZE];
    int k, a, i;

    gpu_fit(frame, d, before, given, state, &g, kept);
    shadow_fit_gpu_read(state, &g, &r);
    /* the range, to the bit (the empty state holds +0 where the C holds minus infinity) */
    if (!given) {
        shadow_fit_range(&range, frame, d);
        if (range.nearest <= range.farthest)
            assert(!memcmp(&r.range, &range, sizeof range));
        else
            assert(r.range.nearest == HUGE_VALF && r.range.farthest == 0.0f);
    }
    /* the range kept, to the bit, and what the GPU keeps for the next frame */
    shadow_fit_keep(&keep, &r.range, &before->range, frame);
    assert(!memcmp(&r.kept, &keep, sizeof keep) && !memcmp(&r.previousRange, &before->range, sizeof keep));
    assert(!memcmp(&kept->range, &keep, sizeof keep));
    /* the split, within the rounding of a division and two square roots */
    shadow_fit_split(split, &r.kept);
    gpuSplit[0] = split[0];
    memcpy(gpuSplit + 1, r.cascades.farDepth, sizeof r.cascades.farDepth);
    count_ulps(&splitUlps, gpuSplit + 1, split + 1, SHADOW_FIT_CASCADES, 8);
    /* the extents of the GPU's slices, to the bit */
    if (!given) {
        shadow_fit_extents(ext, frame, gpuSplit, d);
        assert(!memcmp(r.extents, ext, sizeof ext));
        for (k = 0; k < SHADOW_FIT_CASCADES; k++)
            sliceFits[k] += ext[k].lo[0] <= ext[k].hi[0];
    }
    /* the cascades from the GPU's extents, split and squares before: the squares, the sizes kept
       and the snapped translations to the bit, the rest within the rounding of a division */
    shadow_fit_cascades(&c, frame, gpuSplit, r.extents, before->squares);
    assert(r.cascades.cascades == c.cascades);
    for (k = 0; k < SHADOW_FIT_CASCADES; k++) {
        assert(!memcmp(r.cascades.lo[k], c.lo[k], sizeof c.lo[k]) && !memcmp(r.cascades.hi[k], c.hi[k], sizeof c.hi[k]));
        assert(r.cascades.kept[k] == c.kept[k] && kept->squares[k] == c.kept[k] && r.previous[k] == before->squares[k]);
        assert(r.cascades.farDepth[k] == c.farDepth[k] && r.cascades.offset[k] == c.offset[k]);
        assert(!memcmp(&g.world[k][12], &c.world[k][12], 2 * sizeof(float)) && g.world[k][15] == 1.0f);
        count_ulps(&matrixUlps, g.world[k], c.world[k], 16, 3);
        count_rows(g.world[k], c.world[k]);
        /* fromView sums products whose terms cancel, so its last places say little: within 1e-5
           of its rows, and the GPU's own matrices are checked by what they hold */
        count_ulps(&viewUlps, g.fromView[k], c.fromView[k], 16, ~0u);
        count_rows(g.fromView[k], c.fromView[k]);
        count_ulps(&lookupUlps, &g.blur[k], &c.blur[k], 1, 3);
        count_ulps(&lookupUlps, &g.slant[k], &c.slant[k], 1, 3);
        /* the box: the square across the light and the light's depth along it */
        for (a = 0; a < 2; a++)
            for (i = 0; i < 3; i++) {
                assert(g.planes[k * 6 + a * 2][i] == frame->axis[a][i]);
                assert(g.planes[k * 6 + a * 2 + 1][i] == -frame->axis[a][i]);
            }
        assert(g.planes[k * 6][3] == -g.square[k][0] && g.planes[k * 6 + 1][3] == g.square[k][2]);
        assert(g.planes[k * 6 + 2][3] == -g.square[k][1] && g.planes[k * 6 + 3][3] == g.square[k][3]);
        assert(g.planes[k * 6 + 4][3] == -frame->lightNear && g.planes[k * 6 + 4][2] == frame->axis[2][2]);
    }
    assert(g.lightView[3] == frame->bias && g.ambient[3] == 0.5f && g.ambient[0] == 0.25f);
    if (cam)
        check_cover(frame, cam, &g);
}

/* Squares whose centres lie on the half-texel boundary and a few units in the last place either
   side of it, with radii whose texels are not powers of two: the GPU snaps them to the texel the C
   does, however its division rounds. The cascades pass alone, from a state of such extents. */
static int test_snap(const struct ShadowFitFrame *frame, const struct ShadowFitDepths *d)
{
    static const float radii[SHADOW_FIT_CASCADES] = {24.0f, 200.0f, 1048.0f, 120.0f};
    struct Kept before, kept;
    uint32_t state[STATE_SIZE];
    float nearest = 100.0f, farthest = 500.0f;
    int s, u, c, a, cases = 0;

    memset(&before, 0, sizeof before);
    for (c = 0; c < SHADOW_FIT_CASCADES; c++)
        before.squares[c] = radii[c];
    for (s = -3; s <= 3; s++)
        for (u = -3; u <= 3; u++) {
            shadow_fit_gpu_state(state);
            memcpy(&state[0], &nearest, 4);
            memcpy(&state[1], &farthest, 4);
            for (c = 0; c < SHADOW_FIT_CASCADES; c++) {
                float texel = 2.0f * radii[c] / SHADOW_FIT_SIZE, half = (radii[c] - 5.0f) / 1.05f;
                float centre = ((float)(s * 911 + 37 * c) + 0.5f) * texel;

                for (a = 0; a < abs(u); a++)
                    centre = nextafterf(centre, u < 0 ? -INFINITY : INFINITY);
                state[SHADOW_FIT_GPU_EXTENTS + 4 * c] = shadow_fit_gpu_order(centre - half);
                state[SHADOW_FIT_GPU_EXTENTS + 4 * c + 1] = shadow_fit_gpu_order(-half);
                state[SHADOW_FIT_GPU_EXTENTS + 4 * c + 2] = shadow_fit_gpu_order(centre + half);
                state[SHADOW_FIT_GPU_EXTENTS + 4 * c + 3] = shadow_fit_gpu_order(half);
            }
            compare(frame, d, &before, state, NULL, &kept);
            for (c = 0; c < SHADOW_FIT_CASCADES; c++)
                assert(kept.squares[c] == radii[c]);
            cases += SHADOW_FIT_CASCADES;
        }
    return cases;
}

int main(int argc, char **argv)
{
    static float cropFront[SCENE_PIXELS], cropBack[SCENE_PIXELS];
    static const struct Kept none = {{0}, {0.0f, 0.0f}, {0.0f, 0.0f}};
    struct ShadowFitBox boxes[SCENE_BOXES];
    int kind, n, frames = 0, snaps = 0;

    (void)argc;
    (void)argv;
    /* the passes' groups are the tiles the C dispatches them by */
    assert(shadow_range_cs.threads[0] == SHADOW_FIT_GPU_TILE_THREADS
           && shadow_range_cs.threads[1] == SHADOW_FIT_GPU_TILE_THREADS);
    assert(shadow_extents_cs.threads[0] == SHADOW_FIT_GPU_TILE_THREADS
           && shadow_extents_cs.threads[1] == SHADOW_FIT_GPU_TILE_THREADS);
    if (!open_device())
        return 77;
    for (kind = 0; kind < SCENE_KINDS; kind++) {
        struct Scene scene;
        size_t count;
        struct Kept kept = none;

        scene_make(&scene, kind);
        count = scene_boxes(&scene, boxes);
        for (n = 0; n < 12; n++) {
            struct Pose pose;
            struct ShadowFitCamera cam;
            struct ShadowFitFrame frame;
            struct ShadowFitDepths d = scene_depths(), cropped = {cropFront, cropBack, 250, 139};
            struct Kept before, unused;
            float light[3];
            int j;

            random_pose(&scene, &pose);
            random_light(light);
            pose_camera(&pose, SCENE_W, SCENE_H, &cam);
            scene_render(&scene, &pose, &cam);
            assert(shadow_fit_frame(&frame, &cam, light) && shadow_fit_bounds(&frame, boxes, count));
            /* from what the frame before kept, and from nothing */
            before = kept;
            compare(&frame, &d, &before, NULL, &cam, &kept);
            compare(&frame, &d, &none, NULL, &cam, &unused);
            if (kind == 0 && n < 2)
                snaps += test_snap(&frame, &d);
            /* a size that is not a multiple of the groups': the top left 250 by 139 */
            for (j = 0; j < 139; j++) {
                memcpy(&cropFront[j * 250], &front[j * SCENE_W], 250 * sizeof(float));
                memcpy(&cropBack[j * 250], &depth[j * SCENE_W], 250 * sizeof(float));
            }
            compare(&frame, &cropped, &before, NULL, NULL, &unused);
            /* a single pixel, and nothing */
            memset(cropFront, 0, sizeof cropFront);
            memset(cropBack, 0, sizeof cropBack);
            cropFront[77 * SCENE_W + 130] = front[77 * SCENE_W + 130];
            cropBack[77 * SCENE_W + 130] = depth[77 * SCENE_W + 130];
            cropped.width = SCENE_W;
            cropped.height = SCENE_H;
            compare(&frame, &cropped, &before, NULL, NULL, &unused);
            /* the last pixel alone, in the last row of a tile half outside the targets */
            cropFront[77 * SCENE_W + 130] = cropBack[77 * SCENE_W + 130] = 0.0f;
            cropFront[SCENE_PIXELS - 1] = front[77 * SCENE_W + 130];
            cropBack[SCENE_PIXELS - 1] = depth[77 * SCENE_W + 130];
            compare(&frame, &cropped, &before, NULL, NULL, &unused);
            memset(cropFront, 0, sizeof cropFront);
            memset(cropBack, 0, sizeof cropBack);
            compare(&frame, &cropped, &before, NULL, NULL, &unused);
            frames += 6;
        }
    }
    close_device();
    /* the extents were held to the C in every slice */
    for (n = 0; n < SHADOW_FIT_CASCADES; n++)
        assert(sliceFits[n] > 0);
    printf("shadow fit on the GPU: %d frames; range, range kept, extents (slices 0 to 3 reached in %d, %d, %d and "
           "%d), squares, sizes kept and snapped translations to the bit, %d squares on the half-texel boundary "
           "among them; split depths differ in %.0f of %.0f values (at most %u units in the last place), world "
           "matrices in %.0f of %.0f (%u), view matrices in %.0f of %.0f (%u), blur and slant in %.0f of %.0f (%u); "
           "every matrix within %.2g of its rows; every receiving point in the GPU's cascades and their boxes\n",
           frames, sliceFits[0], sliceFits[1], sliceFits[2], sliceFits[3], snaps, splitUlps.differ, splitUlps.values,
           splitUlps.most, matrixUlps.differ, matrixUlps.values, matrixUlps.most, viewUlps.differ, viewUlps.values,
           viewUlps.most, lookupUlps.differ, lookupUlps.values, lookupUlps.most, worstRelative);
    return 0;
}
