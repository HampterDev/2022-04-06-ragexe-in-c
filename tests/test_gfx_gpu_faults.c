/* render/gfx_gpu.c on a real GPU, in a hidden window, with the SDL calls it makes passed through
   this file, which can make them fail and watches when they are made:
   - the device made with each of the objects it makes failing in turn, and the render thread
     failing to start: the device is whole or absent afterwards, a device that failed lets go of
     every object it made, and the device made after that draws;
   - command buffers recorded one at a time (gfx_gpu.c, commands_take): the render thread's passes
     slowed down while the main thread makes a texture every frame, and no command buffer submitted
     nor texture made (which SDL submits a command buffer for) on the main thread while the render
     thread records one, the frames' pictures checked as they go;
   - a frame the render thread could not submit, and a frame dropped on the main thread (a buffer
     it needs that could not be made): every frame read afterwards shows the textures they were to
     take up.
   Skipped (77) where SDL finds no GPU device. */
#include <SDL3/SDL.h>
#include "render/render_thread.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static SDL_ThreadID mainThread;
/* the nth object made from now fails (0: none), and whether one did */
static int faultIn, faulted;
static int failThread;          /* render_thread_start fails once */
static int failRenderAcquire;   /* the render thread's next command buffer cannot be had */
static int failBuffer;          /* the next buffer cannot be made */
static SDL_AtomicInt live;      /* objects made and not released */
static SDL_AtomicInt renderOpen;  /* the render thread is recording a command buffer */
static SDL_AtomicInt overlaps;    /* main-thread submits and textures made while it is */
static SDL_AtomicInt renderSubmits;

static int on_main(void)
{
    return SDL_GetCurrentThreadID() == mainThread;
}

static int fault(void)
{
    if (faultIn && !--faultIn) {
        faulted = 1;
        SDL_SetError("fault injected");
        return 1;
    }
    return 0;
}

static void *made(void *object)
{
    if (object)
        SDL_AtomicIncRef(&live);
    return object;
}

static void released(const void *object)
{
    if (object)
        SDL_AtomicDecRef(&live);
}

static void check_main_submit(void)
{
    if (on_main() && SDL_GetAtomicInt(&renderOpen))
        SDL_AtomicIncRef(&overlaps);
}

static SDL_GPUShader *f_CreateGPUShader(SDL_GPUDevice *d, const SDL_GPUShaderCreateInfo *i)
{
    return fault() ? NULL : made(SDL_CreateGPUShader(d, i));
}

static SDL_GPUSampler *f_CreateGPUSampler(SDL_GPUDevice *d, const SDL_GPUSamplerCreateInfo *i)
{
    return fault() ? NULL : made(SDL_CreateGPUSampler(d, i));
}

static SDL_GPUTexture *f_CreateGPUTexture(SDL_GPUDevice *d, const SDL_GPUTextureCreateInfo *i)
{
    check_main_submit();
    return fault() ? NULL : made(SDL_CreateGPUTexture(d, i));
}

static SDL_GPUBuffer *f_CreateGPUBuffer(SDL_GPUDevice *d, const SDL_GPUBufferCreateInfo *i)
{
    if (failBuffer) {
        failBuffer = 0;
        SDL_SetError("fault injected");
        return NULL;
    }
    return fault() ? NULL : made(SDL_CreateGPUBuffer(d, i));
}

static SDL_GPUTransferBuffer *f_CreateGPUTransferBuffer(SDL_GPUDevice *d, const SDL_GPUTransferBufferCreateInfo *i)
{
    return fault() ? NULL : made(SDL_CreateGPUTransferBuffer(d, i));
}

static SDL_GPUComputePipeline *f_CreateGPUComputePipeline(SDL_GPUDevice *d, const SDL_GPUComputePipelineCreateInfo *i)
{
    return fault() ? NULL : made(SDL_CreateGPUComputePipeline(d, i));
}

static SDL_GPUGraphicsPipeline *f_CreateGPUGraphicsPipeline(SDL_GPUDevice *d,
                                                            const SDL_GPUGraphicsPipelineCreateInfo *i)
{
    return fault() ? NULL : made(SDL_CreateGPUGraphicsPipeline(d, i));
}

static void f_ReleaseGPUShader(SDL_GPUDevice *d, SDL_GPUShader *o)
{
    released(o);
    SDL_ReleaseGPUShader(d, o);
}

static void f_ReleaseGPUSampler(SDL_GPUDevice *d, SDL_GPUSampler *o)
{
    released(o);
    SDL_ReleaseGPUSampler(d, o);
}

static void f_ReleaseGPUTexture(SDL_GPUDevice *d, SDL_GPUTexture *o)
{
    released(o);
    SDL_ReleaseGPUTexture(d, o);
}

static void f_ReleaseGPUBuffer(SDL_GPUDevice *d, SDL_GPUBuffer *o)
{
    released(o);
    SDL_ReleaseGPUBuffer(d, o);
}

static void f_ReleaseGPUTransferBuffer(SDL_GPUDevice *d, SDL_GPUTransferBuffer *o)
{
    released(o);
    SDL_ReleaseGPUTransferBuffer(d, o);
}

static void f_ReleaseGPUComputePipeline(SDL_GPUDevice *d, SDL_GPUComputePipeline *o)
{
    released(o);
    SDL_ReleaseGPUComputePipeline(d, o);
}

static void f_ReleaseGPUGraphicsPipeline(SDL_GPUDevice *d, SDL_GPUGraphicsPipeline *o)
{
    released(o);
    SDL_ReleaseGPUGraphicsPipeline(d, o);
}

static struct RenderThread *f_render_thread_start(RenderThreadRun run, void *context)
{
    if (failThread) {
        failThread = 0;
        faulted = 1;
        return NULL;
    }
    return render_thread_start(run, context);
}

static SDL_GPUCommandBuffer *f_AcquireGPUCommandBuffer(SDL_GPUDevice *d)
{
    SDL_GPUCommandBuffer *cmd;
    if (on_main()) {
        check_main_submit();
        return SDL_AcquireGPUCommandBuffer(d);
    }
    if (failRenderAcquire) {
        failRenderAcquire = 0;
        SDL_SetError("fault injected");
        return NULL;
    }
    cmd = SDL_AcquireGPUCommandBuffer(d);
    if (cmd)
        SDL_SetAtomicInt(&renderOpen, 1);
    return cmd;
}

static bool f_SubmitGPUCommandBuffer(SDL_GPUCommandBuffer *cmd)
{
    check_main_submit();
    return SDL_SubmitGPUCommandBuffer(cmd);
}

static SDL_GPUFence *f_SubmitGPUCommandBufferAndAcquireFence(SDL_GPUCommandBuffer *cmd)
{
    SDL_GPUFence *fence;
    check_main_submit();
    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    if (!on_main()) {
        SDL_AtomicIncRef(&renderSubmits);
        SDL_SetAtomicInt(&renderOpen, 0);
    }
    return fence;
}

/* the render thread's passes take a while, so that the main thread goes on meanwhile */
static SDL_GPURenderPass *f_BeginGPURenderPass(SDL_GPUCommandBuffer *cmd, const SDL_GPUColorTargetInfo *colours,
                                              Uint32 n, const SDL_GPUDepthStencilTargetInfo *depth)
{
    if (!on_main())
        SDL_DelayNS(1000000);
    return SDL_BeginGPURenderPass(cmd, colours, n, depth);
}

#define SDL_CreateGPUShader f_CreateGPUShader
#define SDL_CreateGPUSampler f_CreateGPUSampler
#define SDL_CreateGPUTexture f_CreateGPUTexture
#define SDL_CreateGPUBuffer f_CreateGPUBuffer
#define SDL_CreateGPUTransferBuffer f_CreateGPUTransferBuffer
#define SDL_CreateGPUComputePipeline f_CreateGPUComputePipeline
#define SDL_CreateGPUGraphicsPipeline f_CreateGPUGraphicsPipeline
#define SDL_ReleaseGPUShader f_ReleaseGPUShader
#define SDL_ReleaseGPUSampler f_ReleaseGPUSampler
#define SDL_ReleaseGPUTexture f_ReleaseGPUTexture
#define SDL_ReleaseGPUBuffer f_ReleaseGPUBuffer
#define SDL_ReleaseGPUTransferBuffer f_ReleaseGPUTransferBuffer
#define SDL_ReleaseGPUComputePipeline f_ReleaseGPUComputePipeline
#define SDL_ReleaseGPUGraphicsPipeline f_ReleaseGPUGraphicsPipeline
#define render_thread_start f_render_thread_start
#define SDL_AcquireGPUCommandBuffer f_AcquireGPUCommandBuffer
#define SDL_SubmitGPUCommandBuffer f_SubmitGPUCommandBuffer
#define SDL_SubmitGPUCommandBufferAndAcquireFence f_SubmitGPUCommandBufferAndAcquireFence
#define SDL_BeginGPURenderPass f_BeginGPURenderPass
#include "../src/render/gfx_gpu.c"

struct perf_counters g_perf;
struct C3dDevice g_3dDevice;

#define W 64
#define H 48

static DWORD centre(int *ok)
{
    RECT r;
    DWORD p = 0;
    r.left = W / 2;
    r.top = H / 2;
    r.right = r.left + 1;
    r.bottom = r.top + 1;
    *ok = gfx_read_frame(&r, &p, 4) == 0;
    return p & 0xffffff;
}

static gfx_surface *texture_of(DWORD argb)
{
    gfx_surface *t = gfx_create_texture(4, 4, GFX_FMT_8888);
    struct gfx_lock lk;
    int x, y;
    assert(t && gfx_lock(t, NULL, &lk) == 0);
    for (y = 0; y < 4; y++)
        for (x = 0; x < 4; x++)
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &argb, 4);
    gfx_unlock(t);
    return t;
}

/* t over the whole screen, by the nearest texel */
static void draw(gfx_surface *t)
{
    struct TexDrawVertex v[4];
    int i;
    for (i = 0; i < 4; i++) {
        v[i].x = (i & 1 ? (float)W : 0.0f) - 0.5f;
        v[i].y = (i & 2 ? (float)H : 0.0f) - 0.5f;
        v[i].z = 0.5f;
        v[i].rhw = 1.0f;
        v[i].diffuse = 0xffffffff;
        v[i].u = i & 1 ? 1.0f : 0.0f;
        v[i].v = i & 2 ? 1.0f : 0.0f;
    }
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gfx_set_mode(DRAW_BLIT);
    gfx_set_texture(0, t);
    gfx_draw_vertices(v, 4, 1);
    gfx_end_scene();
    gfx_set_texture(0, NULL);
}

static void open_device(struct C3dDevice *d)
{
    assert(gfx_create_screen(d) == 0);
    assert(gfx_get_caps(&g_gfxCaps) == 0);
    assert(gfx_create_zbuffer(d) == 0);
}

/* Each object create_device makes failing in turn, then the render thread: a setup that fails
   leaves no device and no object, and gfx_close (which c3d_device_init calls) does no harm. */
static int check_setup_faults(struct C3dDevice *d)
{
    int n, steps = 0;
    for (n = 1;; n++) {
        int hr;
        faulted = 0;
        faultIn = n;
        hr = gfx_create_screen(d);
        faultIn = 0;
        if (!faulted) {
            assert(hr == 0);
            gfx_close(d);
            break;
        }
        steps++;
        /* whole or absent */
        assert(!G.dev == !G.thread && !G.dev == !G.f);
        assert(hr == 0 || !G.dev || !d->target);
        gfx_close(d);
        assert(!G.dev && SDL_GetAtomicInt(&live) == 0);
    }
    failThread = 1;
    faulted = 0;
    assert(gfx_create_screen(d) != 0 && faulted && !G.dev && !G.thread && !G.f);
    gfx_close(d);
    assert(SDL_GetAtomicInt(&live) == 0);
    return steps;
}

/* A texture made on the main thread every frame while the render thread records the frame before
   with its passes slowed down. */
static void check_recording_alone(void)
{
    int i, ok;
    for (i = 0; i < 48; i++) {
        DWORD colour = 0xff000000 | (DWORD)(0x102030 + i * 0x030507);
        gfx_surface *t = texture_of(colour);
        draw(t);
        gfx_release(t);
        if (i % 8 == 7) {
            assert(centre(&ok) == (colour & 0xffffff) && ok);
        } else {
            gfx_present(0);
        }
    }
    assert(SDL_GetAtomicInt(&overlaps) == 0 && SDL_GetAtomicInt(&renderSubmits) >= 48);
}

/* Every frame read after one the render thread could not submit, and after one dropped on the main
   thread, shows the texture that frame was to take up. */
static void check_lost_frames(void)
{
    int i, ok;
    DWORD got;
    gfx_surface *t = texture_of(0xff3060c0), *u;
    draw(t);
    failRenderAcquire = 1;
    gfx_present(0);
    for (i = 0; i < 3; i++) {
        draw(t);
        got = centre(&ok);
        assert(!failRenderAcquire && (!ok || got == 0x3060c0));
    }
    assert(ok);
    /* a frame whose quads need larger buffers than any before, which cannot be made */
    u = texture_of(0xffc06030);
    draw(u);
    gfx_set_mode(DRAW_BLIT);
    gfx_set_texture(0, u);
    for (i = 0; i < 4000; i++) {
        struct TexDrawVertex v[4];
        int k;
        for (k = 0; k < 4; k++) {
            v[k].x = (float)(i % W) + (k & 1) - 0.5f;
            v[k].y = (float)(i % H) + (k >> 1) - 0.5f;
            v[k].z = 0.5f;
            v[k].rhw = 1.0f;
            v[k].diffuse = 0xffffffff;
            v[k].u = (float)(k & 1);
            v[k].v = (float)(k >> 1);
        }
        gfx_draw_vertices(v, 4, 1);
        gfx_set_texture(0, i & 1 ? u : t);
    }
    gfx_end_scene();
    gfx_set_texture(0, NULL);
    failBuffer = 1;
    centre(&ok);
    assert(!failBuffer && !ok);
    for (i = 0; i < 2; i++) {
        draw(u);
        got = centre(&ok);
        assert(ok && got == 0xc06030);
    }
    gfx_release(t);
    gfx_release(u);
}

int main(void)
{
    struct C3dDevice *d = &g_3dDevice;
    int steps;
    SDL_setenv_unsafe("RAGEXE_MSAA", "1", 1);
    mainThread = SDL_GetCurrentThreadID();
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->hwnd = SDL_CreateWindow("gfx faults", W, H, SDL_WINDOW_HIDDEN);
    if (!d->hwnd) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->width = W;
    d->height = H;
    assert(gfx_open(d) == 0);
    if (gfx_create_screen(d) != 0) {
        fprintf(stderr, "skipped: no GPU device\n");
        return 77;
    }
    gfx_close(d);
    assert(SDL_GetAtomicInt(&live) == 0);
    steps = check_setup_faults(d);
    open_device(d);
    check_recording_alone();
    check_lost_frames();
    gfx_close(d);
    assert(SDL_GetAtomicInt(&live) == 0);
    SDL_DestroyWindow(d->hwnd);
    SDL_Quit();
    printf("gfx gpu faults: the device whole or absent with each of its %d objects failing and the render "
           "thread failing, nothing kept of a device that failed; no command buffer submitted or texture "
           "made on the main thread while the render thread recorded one (%d frames submitted); frames "
           "after one not submitted and one dropped show what those were to take up\n",
           steps, SDL_GetAtomicInt(&renderSubmits));
    return 0;
}
