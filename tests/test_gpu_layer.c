/* The graphics device's seam for layers (render/gpu_layer.h) on a real GPU, in a hidden window: a
   layer whose hooks write down when the device calls them, over the device made, a frame drawn with
   two draws of the layer's, a frame the layer drops at its hand-over and the frame after it, the
   device lost and made again. The layer's part of a packet is zeroed when the layer is added and
   kept with the packet; the render thread's hooks find what the main thread put into it; the
   device binds a draw's pipeline and tells the layer to bind its buffers only when its program
   changes; a layer is added once, and none while a device is there. Skipped (77) where SDL finds
   no GPU device. */
#include "render/gpu_layer.h"
#include "render/shaders/rows.h"
#include "diag/perf_counters.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct perf_counters g_perf;

#define W 64
#define H 48

/* the layer's part of a packet: the frame recorded into it, and the words of its draws as the
   render thread's hooks found them */
struct part {
    unsigned frame;
    unsigned copied, computed;
    uint32_t drawn[2];
    int changed[2], ndrawn;
};

static char order[256];
static size_t norder;
static SDL_Mutex *lock;
static int layer, dropNext;
static SDL_GPUShader *vs, *fs;
static SDL_GPUGraphicsPipeline *pipe;

/* the hooks called, one letter each, from either thread */
static void note(char c)
{
    SDL_LockMutex(lock);
    assert(norder + 1 < sizeof order);
    order[norder++] = c;
    order[norder] = 0;
    SDL_UnlockMutex(lock);
}

/* what was noted since the last call, which forgets it */
static void expect(const char *want)
{
    SDL_LockMutex(lock);
    if (strcmp(order, want)) {
        fprintf(stderr, "hooks %s, expected %s\n", order, want);
        assert(0);
    }
    norder = 0;
    order[0] = 0;
    SDL_UnlockMutex(lock);
}

static int made(void)
{
    note('M');
    vs = gpu_shader(&rows_vs);
    fs = gpu_shader(&rows_fs);
    return vs && fs;
}

/* the wobble's program, which this layer binds but never draws with */
static int pipelines(void)
{
    struct GpuPipe p;
    note('P');
    memset(&p, 0, sizeof p);
    p.vs = vs;
    p.fs = fs;
    p.buffers[0].pitch = 4;
    p.buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
    p.attrs[0].format = SDL_GPU_VERTEXELEMENTFORMAT_UINT;
    p.nbuffers = p.nattrs = 1;
    p.depth = DRAW_DEPTH_OFF;
    p.cull = GPU_CULL_NONE;
    p.mask = GPU_MASK_RGB;
    p.hasDepth = 1;
    p.samples = SDL_GPU_SAMPLECOUNT_1;
    p.targets = 1;
    pipe = gpu_pipeline(&p);
    return pipe != NULL;
}

static void lost(void)
{
    note('L');
    SDL_ReleaseGPUShader(gpu_dev(), vs);
    SDL_ReleaseGPUShader(gpu_dev(), fs);
    vs = fs = NULL;
    pipe = NULL;
}

static void begin(void *p)
{
    struct part *part = p;
    note('B');
    part->ndrawn = 0;
    part->copied = part->computed = 0;
}

static int pending(void)
{
    note('N');
    return 0;
}

static void prepare(void)
{
    note('R');
}

static int hand_over(void *p)
{
    struct part *part = p;
    note('H');
    part->frame = gpu_frame_number();
    if (dropNext) {
        dropNext = 0;
        return 0;
    }
    return 1;
}

static void flushed(int presented)
{
    assert(!presented);
    note('F');
}

static void retired(void *p, int how, unsigned number)
{
    const struct part *part = p;
    note(how == GPU_FRAME_DRAWN ? 'D' : how == GPU_FRAME_DROPPED ? 'X' : 'U');
    assert(part->frame == number);
}

static void copy(void *p, SDL_GPUCopyPass *pass, SDL_GPUTransferBuffer *texels)
{
    struct part *part = p;
    (void)pass;
    (void)texels;
    note('C');
    part->copied = part->frame;
}

static void compute(void *p, SDL_GPUCommandBuffer *cmd)
{
    struct part *part = p;
    (void)cmd;
    note('K');
    part->computed = part->frame;
}

static void draw(void *p, struct GpuPass *pass, const struct GpuLayerDraw *d)
{
    struct part *part = p;
    note('W');
    assert(part->ndrawn < 2 && pass->cmd && pass->pass && pass->records && d->pipeline == pipe);
    part->changed[part->ndrawn] = gpu_pass_use(pass, d);
    part->drawn[part->ndrawn++] = d->words[0];
}

static const struct GpuLayer test_layer = {
    sizeof(struct part), made, pipelines, lost, begin, pending, prepare, hand_over, flushed, retired, copy, compute,
    draw,
};

static const struct GpuLayer other_layer = { 0 };

static int read_frame(void)
{
    static uint32_t pixels[W * H];
    return gfx_read_frame(NULL, pixels, W * 4);
}

/* the device and its screen, their hooks called as they are made */
static gfx_surface *open_device(void)
{
    gfx_surface *screen = gfx_create_screen(W, H);
    if (!screen)
        return NULL;
    expect("MB");
    assert(gfx_create_zbuffer() == 0);
    expect("P");
    return screen;
}

int main(void)
{
    SDL_Window *window;
    gfx_surface *screen;
    struct part *part;
    struct GpuStep *step;
    struct GpuLayerDraw *d;
    unsigned frame;
    int i;
    SDL_setenv_unsafe("RAGEXE_MSAA", "1", 1);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    lock = SDL_CreateMutex();
    window = SDL_CreateWindow("gpu layer", W, H, SDL_WINDOW_HIDDEN);
    if (!lock || !window) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    layer = gpu_add_layer(&test_layer);
    assert(layer == 0 && gpu_add_layer(&test_layer) == layer);
    assert(gfx_open(window) == 0);
    screen = open_device();
    if (!screen) {
        fprintf(stderr, "skipped: no GPU device\n");
        return 77;
    }
    /* no layer joins a device made */
    assert(gpu_add_layer(&other_layer) < 0 && gpu_add_layer(&test_layer) == layer);

    /* a frame with two draws of the layer's, read back: the main thread's hooks, the render
       thread's, the frame taken back and the next begun */
    part = gpu_part(layer);
    assert(part && part->frame == 0 && part->ndrawn == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gpu_end_batch();
    step = gpu_pass();
    assert(step);
    for (i = 0; i < 2; i++) {
        d = gpu_add_draw(step, layer, 0, pipe);
        assert(d && d->pipeline == pipe && d->program == 0);
        d->words[0] = 7u + (uint32_t)i;
    }
    assert(!gpu_add_draw(step, layer, 0, NULL) && !gpu_add_draw(step, layer + 1, 0, pipe));
    frame = gpu_frame_number();
    assert(read_frame() == 0);
    expect("NRHFCKWWDB");
    assert(part->frame == frame && part->copied == frame && part->computed == frame);
    assert(part->ndrawn == 2 && part->drawn[0] == 7 && part->drawn[1] == 8);
    /* the same program twice: its buffers bound for the first draw alone */
    assert(part->changed[0] == 1 && part->changed[1] == 0);

    /* a frame the layer drops at its hand-over: nothing of it reaches the render thread, and it is
       told so when it is taken back, at the next flush, which goes whole */
    dropNext = 1;
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    assert(read_frame() < 0);
    expect("NRHB");
    assert(read_frame() == 0);
    expect("NXRHFCKDB");

    /* the device lost: told after what was recorded went, and made again with the layer */
    gfx_close();
    expect("NL");
    gfx_release(screen);
    screen = open_device();
    assert(screen);
    assert(read_frame() == 0);
    expect("NRHFCKDB");
    gfx_close();
    expect("NL");
    gfx_release(screen);
    SDL_DestroyWindow(window);
    SDL_DestroyMutex(lock);
    SDL_Quit();
    puts("gpu layer: the hooks in order over the device made, a frame drawn, a frame dropped and the one "
         "after it, the device lost and made again; the layer's part kept with its packet; a program's "
         "buffers bound once for its draws; one layer added once, none while a device is there passed");
    return 0;
}
