/* The shadow fit's compute passes (src/render/shaders/shadow_range.hlsl, shadow_extents.hlsl,
   shadow_cascades.hlsl) on this machine's GPU, against render/shadow_fit.c on the same depth
   buffers (tests/shadow_fit_scenes.h): each pass is checked against its step of the C given the
   GPU's own input to it. The reductions must match to the bit; where a value comes through a
   division or a square root, which Vulkan lets a GPU round within 2.5 and 3 units in the last
   place, it may differ by that much, and the test counts how often and by how many units, and
   checks that the GPU's own cascades still hold every receiving pixel. Exits 77 (skipped) without
   a GPU. */
#include "shadow_fit_scenes.h"
#include "render/shaders/shader_code.h"
#include "render/shaders/shadow_cascades.h"
#include "render/shaders/shadow_extents.h"
#include "render/shaders/shadow_range.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* the fit's state and result as the passes lay them out (shadow_fit.hlsli, shadow_cascades.hlsl) */
#define STATE_EXTENTS 2
#define STATE_SIZE (STATE_EXTENTS + 4 * SHADOW_FIT_CASCADES)

struct FrameUniforms {
    float pixelX[4], pixelY[4], screen[4];
    uint32_t size[4];
    float axisX[4], axisY[4], alongLight[4], lightRange[4];
    float inverse[12];
};

struct GpuCascades {
    float world[SHADOW_FIT_CASCADES][16];
    float fromView[SHADOW_FIT_CASCADES][16];
    float farDepth[SHADOW_FIT_CASCADES], offset[SHADOW_FIT_CASCADES], blur[SHADOW_FIT_CASCADES];
    float slant[SHADOW_FIT_CASCADES];
    float square[SHADOW_FIT_CASCADES][4];
    uint32_t count[4];
};

static SDL_GPUDevice *dev;
static SDL_GPUComputePipeline *passes[3];
static SDL_GPUBuffer *depthBuffer, *stateBuffer, *resultBuffer;
static SDL_GPUTransferBuffer *upload, *download;

/* how often a value differed, and by at most how many units in the last place */
struct Ulps {
    double values, differ;
    unsigned most;
};

static struct Ulps splitUlps, matrixUlps, viewUlps, lookupUlps;

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
    depthBuffer = buffer(SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ, SCENE_PIXELS * sizeof(float));
    stateBuffer = buffer(SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE,
                         STATE_SIZE * sizeof(uint32_t));
    resultBuffer = buffer(SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE, sizeof(struct GpuCascades));
    upload = transfer(SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, SCENE_PIXELS * sizeof(float) + STATE_SIZE * sizeof(uint32_t));
    download = transfer(SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, sizeof(struct GpuCascades) + STATE_SIZE * sizeof(uint32_t));
    return 1;
}

static void close_device(void)
{
    int i;

    SDL_ReleaseGPUTransferBuffer(dev, upload);
    SDL_ReleaseGPUTransferBuffer(dev, download);
    SDL_ReleaseGPUBuffer(dev, depthBuffer);
    SDL_ReleaseGPUBuffer(dev, stateBuffer);
    SDL_ReleaseGPUBuffer(dev, resultBuffer);
    for (i = 0; i < 3; i++)
        SDL_ReleaseGPUComputePipeline(dev, passes[i]);
    SDL_DestroyGPUDevice(dev);
    SDL_Quit();
}

/* the frame's constants as the passes take them; the light's axis over its range is the CPU's,
   as is the light's near depth over it (shadow_fit_cascades divides the same way) */
static void uniforms(const struct ShadowFitFrame *f, int width, int height, struct FrameUniforms *u)
{
    int i;

    memset(u, 0, sizeof *u);
    memcpy(u->pixelX, f->pixel[0], sizeof u->pixelX);
    memcpy(u->pixelY, f->pixel[1], sizeof u->pixelY);
    u->screen[0] = f->centre[0];
    u->screen[1] = f->centre[1];
    u->screen[2] = f->nearZ;
    u->screen[3] = f->reach;
    u->size[0] = (uint32_t)width;
    u->size[1] = (uint32_t)height;
    for (i = 0; i < 3; i++) {
        u->axisX[i] = f->axis[0][i];
        u->axisY[i] = f->axis[1][i];
        u->alongLight[i] = f->axis[2][i] / f->lightRange;
    }
    u->alongLight[3] = -f->lightNear / f->lightRange;
    u->lightRange[0] = f->lightRange;
    memcpy(u->inverse, f->inverse, sizeof u->inverse);
}

static uint32_t order_float(float x)
{
    uint32_t u;

    memcpy(&u, &x, 4);
    return u & 0x80000000u ? ~u : u | 0x80000000u;
}

static float unorder_float(uint32_t u)
{
    float x;

    u = u & 0x80000000u ? u & 0x7fffffffu : ~u;
    memcpy(&x, &u, 4);
    return x;
}

/* the three passes over a depth buffer: the fit's state after them and their result */
static void gpu_fit(const struct ShadowFitFrame *frame, const float *buffer, int width, int height, uint32_t *state,
                    struct GpuCascades *result)
{
    struct FrameUniforms u;
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    SDL_GPUComputePass *pass;
    SDL_GPUTransferBufferLocation from;
    SDL_GPUBufferRegion region;
    SDL_GPUStorageBufferReadWriteBinding rw;
    SDL_GPUFence *fence;
    Uint32 depthBytes = (Uint32)(width * height) * sizeof(float);
    uint32_t init[STATE_SIZE];
    unsigned char *map;
    int i, p;

    init[0] = 0x7f800000u;
    init[1] = 0;
    for (i = 0; i < 4 * SHADOW_FIT_CASCADES; i++)
        init[STATE_EXTENTS + i] = i & 2 ? order_float(-HUGE_VALF) : order_float(HUGE_VALF);
    map = SDL_MapGPUTransferBuffer(dev, upload, false);
    assert(map);
    memcpy(map, buffer, depthBytes);
    memcpy(map + SCENE_PIXELS * sizeof(float), init, sizeof init);
    SDL_UnmapGPUTransferBuffer(dev, upload);
    uniforms(frame, width, height, &u);

    cmd = SDL_AcquireGPUCommandBuffer(dev);
    assert(cmd);
    copy = SDL_BeginGPUCopyPass(cmd);
    from.transfer_buffer = upload;
    from.offset = 0;
    region.buffer = depthBuffer;
    region.offset = 0;
    region.size = depthBytes > 0 ? depthBytes : 4;
    SDL_UploadToGPUBuffer(copy, &from, &region, false);
    from.offset = SCENE_PIXELS * sizeof(float);
    region.buffer = stateBuffer;
    region.size = sizeof init;
    SDL_UploadToGPUBuffer(copy, &from, &region, false);
    SDL_EndGPUCopyPass(copy);

    SDL_zero(rw);
    for (p = 0; p < 3; p++) {
        rw.buffer = p < 2 ? stateBuffer : resultBuffer;
        pass = SDL_BeginGPUComputePass(cmd, NULL, 0, &rw, 1);
        assert(pass);
        SDL_BindGPUComputePipeline(pass, passes[p]);
        SDL_BindGPUComputeStorageBuffers(pass, 0, p < 2 ? &depthBuffer : &stateBuffer, 1);
        SDL_PushGPUComputeUniformData(cmd, 0, &u, sizeof u);
        if (p < 2)
            SDL_DispatchGPUCompute(pass, (Uint32)(width + 15) / 16, (Uint32)(height + 15) / 16, 1);
        else
            SDL_DispatchGPUCompute(pass, 1, 1, 1);
        SDL_EndGPUComputePass(pass);
    }

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
    SDL_EndGPUCopyPass(copy);
    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    assert(fence);
    assert(SDL_WaitForGPUFences(dev, true, &fence, 1));
    SDL_ReleaseGPUFence(dev, fence);
    map = SDL_MapGPUTransferBuffer(dev, download, false);
    assert(map);
    memcpy(state, map, STATE_SIZE * sizeof(uint32_t));
    memcpy(result, map + STATE_SIZE * sizeof(uint32_t), sizeof *result);
    SDL_UnmapGPUTransferBuffer(dev, download);
}

/* Every receiving pixel of the last frame scene_render drew lies in the GPU's cascade for its
   depth, inside the map by the soft edge and the lookup's offset, through either of its matrices. */
static void check_cover(const struct ShadowFitFrame *frame, const struct ShadowFitCamera *cam, const struct GpuCascades *g)
{
    int p, c, i, j;

    for (p = 0; p < SCENE_PIXELS; p++) {
        double d = depth[p], v[3], l[4], lv[4], radius, margin;

        if (!(d >= frame->nearZ && d <= frame->reach))
            continue;
        for (c = 0; c < SHADOW_FIT_CASCADES && !(d < g->farDepth[c]); c++)
            ;
        assert(g->count[0] == SHADOW_FIT_CASCADES && c < SHADOW_FIT_CASCADES);
        radius = (g->square[c][2] - g->square[c][0]) * 0.5;
        margin = (0.5 + 1.5 * 2.0 * radius / SHADOW_FIT_SIZE) / radius;
        for (i = 0; i < 3; i++)
            v[i] = cam->view[i] * hit[p][0] + cam->view[3 + i] * hit[p][1] + cam->view[6 + i] * hit[p][2] + cam->view[9 + i];
        for (j = 0; j < 4; j++) {
            l[j] = g->world[c][12 + j];
            lv[j] = g->fromView[c][12 + j];
            for (i = 0; i < 3; i++) {
                l[j] += hit[p][i] * g->world[c][i * 4 + j];
                lv[j] += v[i] * g->fromView[c][i * 4 + j];
            }
        }
        assert(fabs(l[0]) <= 1.0 - margin && fabs(l[1]) <= 1.0 - margin && l[2] > 0.0 && l[2] < 1.0);
        assert(fabs(lv[0]) <= 1.0 - margin + 1e-5 && fabs(lv[1]) <= 1.0 - margin + 1e-5 && lv[2] > 0.0 && lv[2] < 1.0);
    }
}

/* Each pass against its step of the C, given the GPU's input to it; with cam, the frame
   scene_render drew last, whose pixels the GPU's cascades then hold. */
static void compare(const struct ShadowFitFrame *frame, const float *buffer, int width, int height,
                    const struct ShadowFitCamera *cam)
{
    struct ShadowFitRange range, gpuRange;
    struct ShadowFitExtent ext[SHADOW_FIT_CASCADES], gpuExt[SHADOW_FIT_CASCADES];
    struct ShadowFitCascades c;
    struct GpuCascades g;
    float split[SHADOW_FIT_CASCADES + 1], gpuSplit[SHADOW_FIT_CASCADES + 1];
    uint32_t state[STATE_SIZE];
    int k, a;

    gpu_fit(frame, buffer, width, height, state, &g);
    /* the range, to the bit (the empty state holds +0 where the C holds minus infinity) */
    shadow_fit_range(&range, frame, buffer, width, height);
    memcpy(&gpuRange.nearest, &state[0], 4);
    memcpy(&gpuRange.farthest, &state[1], 4);
    if (range.nearest <= range.farthest)
        assert(!memcmp(&gpuRange, &range, sizeof range));
    else
        assert(gpuRange.nearest == HUGE_VALF && gpuRange.farthest == 0.0f);
    /* the split, within the rounding of a division and two square roots */
    shadow_fit_split(split, &gpuRange);
    gpuSplit[0] = split[0];
    memcpy(gpuSplit + 1, g.farDepth, sizeof g.farDepth);
    count_ulps(&splitUlps, gpuSplit + 1, split + 1, SHADOW_FIT_CASCADES, 8);
    /* the extents of the GPU's slices, to the bit */
    shadow_fit_extents(ext, frame, gpuSplit, buffer, width, height);
    for (k = 0; k < SHADOW_FIT_CASCADES; k++)
        for (a = 0; a < 2; a++) {
            gpuExt[k].lo[a] = unorder_float(state[STATE_EXTENTS + 4 * k + a]);
            gpuExt[k].hi[a] = unorder_float(state[STATE_EXTENTS + 4 * k + 2 + a]);
        }
    assert(!memcmp(gpuExt, ext, sizeof ext));
    /* the cascades from the GPU's extents and split: squares and the snapped translation to the
       bit, the rest within the rounding of a division */
    shadow_fit_cascades(&c, frame, gpuSplit, gpuExt);
    assert((int)g.count[0] == c.cascades);
    for (k = 0; k < SHADOW_FIT_CASCADES; k++) {
        assert(g.square[k][0] == c.lo[k][0] && g.square[k][1] == c.lo[k][1]);
        assert(g.square[k][2] == c.hi[k][0] && g.square[k][3] == c.hi[k][1]);
        assert(g.farDepth[k] == c.farDepth[k] && g.offset[k] == c.offset[k]);
        for (a = 2; a < 16; a += 4)
            assert(g.world[k][a] == c.world[k][a]);
        assert(!memcmp(&g.world[k][12], &c.world[k][12], 4 * sizeof(float)));
        count_ulps(&matrixUlps, g.world[k], c.world[k], 16, 3);
        /* fromView sums products whose terms cancel, so its last places say little: the GPU's
           own matrices are checked by what they hold instead */
        count_ulps(&viewUlps, g.fromView[k], c.fromView[k], 16, ~0u);
        count_ulps(&lookupUlps, &g.blur[k], &c.blur[k], 1, 3);
        count_ulps(&lookupUlps, &g.slant[k], &c.slant[k], 1, 3);
    }
    if (cam)
        check_cover(frame, cam, &g);
}

int main(int argc, char **argv)
{
    static float cropped[SCENE_PIXELS];
    struct ShadowFitBox boxes[SCENE_BOXES];
    int kind, n, frames = 0;

    (void)argc;
    (void)argv;
    if (!open_device())
        return 77;
    for (kind = 0; kind < SCENE_KINDS; kind++) {
        struct Scene scene;
        size_t count;

        scene_make(&scene, kind);
        count = scene_boxes(&scene, boxes);
        for (n = 0; n < 12; n++) {
            struct Pose pose;
            struct ShadowFitCamera cam;
            struct ShadowFitFrame frame;
            float light[3];
            int j;

            random_pose(&scene, &pose);
            random_light(light);
            pose_camera(&pose, SCENE_W, SCENE_H, &cam);
            scene_render(&scene, &pose, &cam);
            assert(shadow_fit_frame(&frame, &cam, light, boxes, count));
            compare(&frame, depth, SCENE_W, SCENE_H, &cam);
            /* a size that is not a multiple of the groups': the top left 250 by 139 */
            for (j = 0; j < 139; j++)
                memcpy(&cropped[j * 250], &depth[j * SCENE_W], 250 * sizeof(float));
            compare(&frame, cropped, 250, 139, NULL);
            /* a single pixel, and nothing */
            memset(cropped, 0, sizeof cropped);
            cropped[77 * SCENE_W + 130] = depth[77 * SCENE_W + 130];
            compare(&frame, cropped, SCENE_W, SCENE_H, NULL);
            memset(cropped, 0, sizeof cropped);
            compare(&frame, cropped, SCENE_W, SCENE_H, NULL);
            frames += 4;
        }
    }
    close_device();
    printf("shadow fit on the GPU: %d frames; range, extents, squares and snapped translations to the bit; "
           "split depths differ in %.0f of %.0f values (at most %u units in the last place), world matrices in %.0f of "
           "%.0f (%u), view matrices in %.0f of %.0f (%u), blur and slant in %.0f of %.0f (%u); every receiving pixel "
           "in the GPU's cascades\n",
           frames, splitUlps.differ, splitUlps.values, splitUlps.most, matrixUlps.differ, matrixUlps.values, matrixUlps.most,
           viewUlps.differ, viewUlps.values, viewUlps.most, lookupUlps.differ, lookupUlps.values, lookupUlps.most);
    return 0;
}
