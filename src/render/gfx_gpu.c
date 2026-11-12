/* render/gfx.h on SDL 3's GPU API: Vulkan with SPIR-V shaders on Windows and Linux, Metal with
   MSL on macOS, one shader format handed to SDL_CreateGPUDevice so that SDL has one driver to
   choose. The shaders are HLSL compiled offline (src/render/shaders, tools/shaders).

   A frame is recorded, not drawn: each draw becomes a record of its state (struct GpuDraw, read
   by the shaders from a storage buffer through an instance-rate attribute that holds its index)
   in a list of passes, and its vertices, indices and the texels of the textures it uses since they
   changed go into one staging area. When the frame is presented, or the CPU needs a target's
   pixels, one command buffer copies all of that through one transfer buffer in one copy pass and
   replays the passes. Textures keep their pixels on the CPU in the client's own format, so that the
   model loader's thread creates, locks and fills them without calling SDL; the main thread widens
   them to B8G8R8A8 when a draw first uses them after a change. Render targets live on the GPU;
   locking one downloads it. */
#include "render/gfx.h"
#include "render/gfx_batch.h"
#include "render/device.h"
#include "render/present_fit.h"
#include "render/texel_convert.h"
#include "render/shaders/shader_code.h"
#include "render/shaders/fixed.h"
#include "render/shaders/mesh.h"
#include "render/shaders/caster.h"
#include "diag/perf.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { K_SCREEN, K_TARGET, K_SAVE, K_SYSMEM, K_TEXTURE, K_RTTEX, K_ZBUFFER };

#define CLIENT_SURFACE_KEY 0x00ff00ffu
#define N_RS 256
#define N_TSS 33
#define FLAT_KIND 0x100
#define TARGET_FORMAT SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM
/* A target's textures are a pixel wider and taller than the target: every pass's viewport is the
   target moved by half a pixel, (0.5, 0.5, width, height), where Direct3D 9's pixel centres fall on
   the target's, and a viewport must lie within its texture (SDL's GPU API, Metal). */
#define MARGIN 1

/* The records the shaders read (src/render/shaders/records.hlsli). */
struct GpuDraw {
    uint32_t stage0[4], stage0b[4], stage1[4], stage1b[4];
    uint32_t mode[4];
    float target[4];
    float fogColour[4];
    float border0[4], border1[4];
    float colour[4];
    float uvU[4], uvV[4];
    float uvScale[4], uvLimit[4];
    uint32_t mesh[4];
};

struct GpuView {
    float toClip[16];
    float toView[16];
    float fog[4];
};

struct GpuShadow {
    float fromView[GFX_SHADOW_CASCADES][16];
    float farDepth[4], offset[4], blur[4], slant[4];
    float lightView[4];  /* w: the bias */
    float ambient[4];    /* w: the darkness */
    float mapSize[4];    /* x: the maps' size */
};

enum { KIND_SCREEN, KIND_WORLD, KIND_MESH, KIND_CASTER };
enum { DRAW_TEXTURE = 1, DRAW_BORDER = 2 };
enum { PROG_FIXED, PROG_MESH, PROG_CASTER, PROG_COUNT };
enum { LAYOUT_TEX1, LAYOUT_TEX2, LAYOUT_MESH };
enum { MASK_RGBA, MASK_RGB, MASK_NONE };

struct gfx_surface {
    int kind, refs;
    int width, height;
    int fmt;                /* the client's format; GFX_FMT_8888 for targets and system surfaces */
    int levels;
    unsigned char *pixels;  /* level 0 in fmt's layout; a target's lock buffer, B8G8R8A8 */
    unsigned char *lockCopy;/* a target's pixels as downloaded, to tell whether a lock wrote */
    SDL_GPUTexture *tex;    /* B8G8R8A8 */
    DWORD key;
    int dirty;              /* the CPU's pixels are newer than tex, in dirtyRect */
    RECT dirtyRect, lockRect;
    int mips;               /* the lower levels are to be made again */
    int gpuNewer;           /* tex is newer than the CPU's pixels (a copy from a target) */
    int blitDest;           /* tex takes stretched copies from targets: it is a colour target too */
    unsigned pinned;        /* the frame whose recorded commands use it */
    int dead;               /* released while pinned: freed after that frame */
    struct gfx_surface *prev, *next;
    unsigned version;
};

/* custom: a static world mesh (render/gfx.h). The arrays stay with it; its buffer is filled
   through the frame's upload when it is first drawn. */
struct gfx_mesh {
    struct GfxMeshVertex *verts;
    WORD *indices;
    int nverts, nindices;
    SDL_GPUBuffer *buffer;  /* the vertices, then the indices from indexOffset */
    uint32_t indexOffset;
    unsigned pinned;
    int dead;
    struct gfx_mesh *prev, *next;
};

/* One recorded draw. */
struct gpu_draw {
    SDL_GPUGraphicsPipeline *pipeline;
    int program;
    SDL_GPUTextureSamplerBinding bind[3];
    struct gfx_mesh *mesh;  /* NULL: the frame's vertices */
    uint32_t vertexOffset;  /* bytes into the frame's vertices */
    uint32_t firstIndex, count;
    int indexed;
    uint32_t record;
};

enum { STEP_PASS, STEP_UPLOADS, STEP_COPY };

/* A render pass: the draws into one target, or into one shadow cascade. */
struct gpu_step {
    int kind;
    /* STEP_PASS */
    struct gfx_surface *target;  /* NULL: the shadow maps' layer */
    int layer;
    SDL_GPULoadOp loadColour, loadDepth;
    SDL_FColor clearColour;
    float clearDepth;
    int unresolve;               /* draws the resolved picture back into the multisampled one */
    /* STEP_PASS: draws; STEP_UPLOADS: uploads */
    uint32_t first, count;
    /* STEP_COPY */
    SDL_GPUTexture *src, *dst;
    struct gfx_surface *dstSurface;  /* a surface's texture it copies into, or NULL for a target */
    RECT srcRect, dstRect;
};

struct gpu_upload {
    struct gfx_surface *surface;  /* whose pixels go up, or NULL */
    SDL_GPUTexture *tex;
    uint32_t x, y, w, h;
    uint32_t offset;   /* into the staging texels */
    int mips;
};

struct gpu_mesh_upload {
    struct gfx_mesh *mesh;
    SDL_GPUBuffer *buffer;
    uint32_t offset, size;
};

struct bytes {
    unsigned char *data;
    size_t size, capacity;
};

#define PIPELINE_SLOTS 512
#define FRAMES_IN_FLIGHT 2
#define SAMPLER_SLOTS 64

static struct {
    SDL_Window *window;
    SDL_GPUDevice *dev;
    int claimed;
    SDL_ThreadID mainThread;
    SDL_Mutex *lock;
    SDL_AtomicInt contentTick;
    struct gfx_surface *handles;
    SDL_GPUTexture **deadTextures;
    size_t deadCount, deadCapacity;
    struct gfx_mesh *meshes;

    /* the main target: screen holds the picture (resolved when multisampled) */
    struct gfx_surface *screen, *mainTarget, *target;
    SDL_GPUTexture *depth, *msaa;
    SDL_GPUTextureFormat depthFormat;
    SDL_GPUSampleCount samples;
    int mipmaps;

    /* the device's state, Direct3D 9's numbers */
    DWORD rs[N_RS];
    DWORD tss[8][N_TSS];
    DWORD samp[2][5];
    struct gfx_surface *stageTex[8];
    struct GfxWorldView worldView, worldCamera;
    int worldViewSet, worldCameraSet;

    /* shadows */
    int shadowState, shadowPass, shadowReady, shadowReceive, shadowCascade;
    struct GfxShadow shadow;
    int shadowRecord;  /* in this frame's records, or -1 */
    SDL_GPUTexture *shadowMaps, *shadowDepth, *noShadow;

    /* fixed resources */
    SDL_GPUShader *vs[PROG_COUNT], *fs[PROG_COUNT];
    const struct GpuShaderCode *vsCode[PROG_COUNT], *fsCode[PROG_COUNT];
    struct { uint64_t key; SDL_GPUGraphicsPipeline *pipeline; } pipelines[PIPELINE_SLOTS];
    struct { uint32_t key; SDL_GPUSampler *sampler; } samplers[SAMPLER_SLOTS];
    SDL_GPUSampler *pointClamp;
    SDL_GPUTexture *white;
    SDL_GPUBuffer *records, *views, *shadows, *vertices, *indices, *instances;
    uint32_t recordsSize, viewsSize, shadowsSize, verticesSize, indicesSize, instanceCount;
    SDL_GPUTransferBuffer *upload, *download;
    /* the frames the GPU may still be drawing: at most FRAMES_IN_FLIGHT, also where no swapchain
       paces them (a hidden window), so that the CPU does not run ahead and the cycled buffers do
       not pile up */
    SDL_GPUFence *inFlight[FRAMES_IN_FLIGHT];
    int inFlightNext;
    Uint64 presentedNs;  /* when the window last took a picture */
    uint32_t uploadSize, downloadSize;

    /* the frame being recorded */
    unsigned frame;
    struct gpu_step *steps;
    size_t nsteps, stepCapacity;
    struct gpu_draw *draws;
    size_t ndraws, drawCapacity;
    struct gpu_upload *uploads;
    size_t nuploads, uploadCapacity, frameUploads;  /* frameUploads: those of the first copy pass */
    struct gpu_mesh_upload *meshUploads;
    size_t nmeshUploads, meshUploadCapacity;
    struct bytes drawRecords, viewRecords, shadowRecords, vertexData, indexData, texels;
    struct gfx_surface **pins;
    size_t npins, pinCapacity;
    struct gfx_mesh **meshPins;
    size_t nmeshPins, meshPinCapacity;
    int cameraView, meshView, casterView, casterCascade;
    int unresolving;  /* the draw being recorded puts the resolved screen back into the multisampled one */
    int viewsTargetW, viewsTargetH;
} G;

static struct PresentCover present_cover;

static int shadow_record(void);

void gfx_set_cover(const struct PresentCover *cover)
{
    present_cover = *cover;
}

static unsigned next_version(void)
{
    return (unsigned)SDL_AddAtomicInt(&G.contentTick, 1) + 1;
}

static int on_main_thread(void)
{
    return SDL_GetCurrentThreadID() == G.mainThread;
}

static void lock_handles(void)
{
    if (G.lock)
        SDL_LockMutex(G.lock);
}

static void unlock_handles(void)
{
    if (G.lock)
        SDL_UnlockMutex(G.lock);
}

/* ---- growing arrays ---- */

static int grow(void **items, size_t *capacity, size_t need, size_t size)
{
    size_t c = *capacity ? *capacity : 64;
    void *p;
    if (need <= *capacity)
        return 1;
    while (c < need)
        c *= 2;
    p = realloc(*items, c * size);
    if (!p)
        return 0;
    *items = p;
    *capacity = c;
    return 1;
}

/* `size` bytes at an `align`-aligned offset of b, zeroed; answers the offset, or -1 */
static long bytes_add(struct bytes *b, size_t size, size_t align)
{
    size_t at = (b->size + align - 1) & ~(align - 1);
    if (!grow((void **)&b->data, &b->capacity, at + size, 1))
        return -1;
    memset(b->data + b->size, 0, at + size - b->size);
    b->size = at + size;
    return (long)at;
}

/* ---- surfaces ---- */

static int is_target(const struct gfx_surface *s)
{
    return s && (s->kind == K_SCREEN || s->kind == K_TARGET || s->kind == K_SAVE || s->kind == K_RTTEX);
}

static int is_main_target(const struct gfx_surface *s)
{
    return s && (s->kind == K_SCREEN || s->kind == K_TARGET);
}

/* The surface whose texture a target uses: the back buffer shares the screen's. */
static struct gfx_surface *gpu_of(struct gfx_surface *s)
{
    return s && s->kind == K_TARGET && G.screen ? G.screen : s;
}

static struct gfx_surface *new_handle(int kind, int w, int h, int fmt)
{
    struct gfx_surface *s = calloc(1, sizeof *s);
    if (!s)
        return NULL;
    s->kind = kind;
    s->refs = 1;
    s->width = w;
    s->height = h;
    s->fmt = fmt;
    s->levels = 1;
    s->version = next_version();
    lock_handles();
    s->next = G.handles;
    if (G.handles)
        G.handles->prev = s;
    G.handles = s;
    unlock_handles();
    return s;
}

/* A GPU texture goes after the next flush, since the frame recorded so far may use it; the model
   loader's thread may give one up too. */
static void retire_texture(SDL_GPUTexture *tex)
{
    if (!tex)
        return;
    lock_handles();
    if (grow((void **)&G.deadTextures, &G.deadCapacity, G.deadCount + 1, sizeof *G.deadTextures))
        G.deadTextures[G.deadCount++] = tex;
    unlock_handles();
}

static void collect_dead_textures(void)
{
    size_t i;
    lock_handles();
    for (i = 0; i < G.deadCount; i++)
        if (G.dev)
            SDL_ReleaseGPUTexture(G.dev, G.deadTextures[i]);
    G.deadCount = 0;
    unlock_handles();
}

static void destroy_handle(struct gfx_surface *s)
{
    lock_handles();
    if (s->prev)
        s->prev->next = s->next;
    else
        G.handles = s->next;
    if (s->next)
        s->next->prev = s->prev;
    unlock_handles();
    if (s->kind != K_TARGET)
        retire_texture(s->tex);
    free(s->pixels);
    free(s->lockCopy);
    free(s);
}

static void free_handle(struct gfx_surface *s)
{
    int i, pinned;
    for (i = 0; i < 8; i++)
        if (G.stageTex[i] == s) {
            batch_flush();
            G.stageTex[i] = NULL;
        }
    if (G.mainTarget == s)
        G.mainTarget = NULL;
    if (G.screen == s)
        G.screen = NULL;
    if (G.target == s)
        G.target = G.mainTarget;
    lock_handles();
    pinned = s->pinned == G.frame && G.frame;
    if (pinned)
        s->dead = 1;
    unlock_handles();
    if (!pinned)
        destroy_handle(s);
}

void gfx_addref(gfx_surface *s)
{
    if (s)
        s->refs++;
}

void gfx_release(gfx_surface *s)
{
    if (s && --s->refs <= 0)
        free_handle(s);
}

static void pin(struct gfx_surface *s)
{
    if (!s || s->pinned == G.frame)
        return;
    if (!grow((void **)&G.pins, &G.pinCapacity, G.npins + 1, sizeof *G.pins))
        return;
    lock_handles();
    s->pinned = G.frame;
    unlock_handles();
    G.pins[G.npins++] = s;
}

/* The mesh stays alive until the frame is drawn; 0 without memory to say so. */
static int pin_mesh(struct gfx_mesh *m)
{
    if (m->pinned == G.frame)
        return 1;
    if (!grow((void **)&G.meshPins, &G.meshPinCapacity, G.nmeshPins + 1, sizeof *G.meshPins))
        return 0;
    m->pinned = G.frame;
    G.meshPins[G.nmeshPins++] = m;
    return 1;
}

static void whole_rect(RECT *r, const struct gfx_surface *s)
{
    r->left = 0;
    r->top = 0;
    r->right = s->width;
    r->bottom = s->height;
}

static void union_rect(RECT *into, const RECT *r)
{
    if (r->left < into->left) into->left = r->left;
    if (r->top < into->top) into->top = r->top;
    if (r->right > into->right) into->right = r->right;
    if (r->bottom > into->bottom) into->bottom = r->bottom;
}

static void mark_dirty(struct gfx_surface *s, const RECT *r)
{
    RECT all;
    whole_rect(&all, s);
    if (!r)
        r = &all;
    lock_handles();
    if (s->dirty)
        union_rect(&s->dirtyRect, r);
    else
        s->dirtyRect = *r;
    s->dirty = 1;
    unlock_handles();
}

static SDL_GPUTexture *new_texture(int w, int h, int levels, SDL_GPUTextureUsageFlags usage)
{
    SDL_GPUTextureCreateInfo info;
    SDL_zero(info);
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = TARGET_FORMAT;
    info.usage = usage;
    info.width = (Uint32)w;
    info.height = (Uint32)h;
    info.layer_count_or_depth = 1;
    info.num_levels = (Uint32)levels;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    return SDL_CreateGPUTexture(G.dev, &info);
}

/* A target's texture (MARGIN). */
static SDL_GPUTexture *new_target_texture(int w, int h)
{
    return new_texture(w + MARGIN, h + MARGIN, 1, SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
}

/* ---- the frame's lists ---- */

static struct gpu_step *add_step(int kind)
{
    struct gpu_step *s;
    if (!grow((void **)&G.steps, &G.stepCapacity, G.nsteps + 1, sizeof *G.steps))
        return NULL;
    s = &G.steps[G.nsteps++];
    memset(s, 0, sizeof *s);
    s->kind = kind;
    return s;
}

static struct gpu_step *last_step(void)
{
    return G.nsteps ? &G.steps[G.nsteps - 1] : NULL;
}

/* The pass the next draw goes into: the current target's, or the shadow cascade's. */
static struct gpu_step *current_pass(void)
{
    struct gpu_step *s = last_step();
    if (G.shadowPass) {
        if (s && s->kind == STEP_PASS && !s->target && s->layer == G.shadowCascade)
            return s;
        s = add_step(STEP_PASS);
        if (s) {
            s->layer = G.shadowCascade;
            s->loadColour = SDL_GPU_LOADOP_LOAD;
            s->loadDepth = SDL_GPU_LOADOP_LOAD;
            s->first = (uint32_t)G.ndraws;
        }
        return s;
    }
    if (s && s->kind == STEP_PASS && s->target && gpu_of(s->target) == gpu_of(G.target)
        && s->unresolve == G.unresolving)
        return s;
    s = add_step(STEP_PASS);
    if (!s)
        return NULL;
    s->target = gpu_of(G.target);
    s->unresolve = G.unresolving;
    s->loadColour = SDL_GPU_LOADOP_LOAD;
    s->loadDepth = SDL_GPU_LOADOP_LOAD;
    s->first = (uint32_t)G.ndraws;
    pin(s->target);
    return s;
}

static int pass_has_depth(const struct gpu_step *p)
{
    return !p->target ? 1 : p->target == G.screen && G.depth != NULL;
}

static int pass_samples(const struct gpu_step *p)
{
    return p->target && p->target == G.screen && G.msaa ? (int)G.samples : (int)SDL_GPU_SAMPLECOUNT_1;
}

static struct gpu_draw *add_draw(struct gpu_step *pass)
{
    struct gpu_draw *d;
    if (!grow((void **)&G.draws, &G.drawCapacity, G.ndraws + 1, sizeof *G.draws))
        return NULL;
    d = &G.draws[G.ndraws++];
    memset(d, 0, sizeof *d);
    pass->count++;
    return d;
}

static uint32_t add_record(struct GpuDraw **out)
{
    long at = bytes_add(&G.drawRecords, sizeof(struct GpuDraw), 16);
    if (at < 0) {
        *out = NULL;
        return 0;
    }
    *out = (struct GpuDraw *)(G.drawRecords.data + at);
    return (uint32_t)(at / (long)sizeof(struct GpuDraw));
}

/* ---- textures for draws ---- */

/* A surface's GPU texture, made when first needed: a colour target as well where its mipmaps are
   made on the GPU or a stretched copy from a target goes into it (SDL's blits and mipmaps draw). */
static SDL_GPUTexture *ensure_texture(struct gfx_surface *s)
{
    if (s->tex || !G.dev)
        return s->tex;
    s->tex = new_texture(s->width, s->height, s->levels,
                         SDL_GPU_TEXTUREUSAGE_SAMPLER
                         | (s->levels > 1 || s->blitDest ? SDL_GPU_TEXTUREUSAGE_COLOR_TARGET : 0));
    if (!s->tex) {
        SDL_Log("gfx: SDL_CreateGPUTexture %dx%d: %s", s->width, s->height, SDL_GetError());
        return NULL;
    }
    s->dirty = 1;
    whole_rect(&s->dirtyRect, s);
    return s->tex;
}

/* Copies the CPU's pixels of a rectangle into the staging texels, widened to B8G8R8A8. */
static int stage_texels(struct gfx_surface *s, const RECT *r, uint32_t *offset)
{
    int w = r->right - r->left, h = r->bottom - r->top;
    int bytes = s->kind == K_SYSMEM ? 4 : texel_bytes(s->fmt);
    long at;
    const unsigned char *src;
    if (w <= 0 || h <= 0 || !s->pixels)
        return 0;
    at = bytes_add(&G.texels, (size_t)w * h * 4, 16);
    if (at < 0)
        return 0;
    src = s->pixels + (size_t)r->top * s->width * bytes + (size_t)r->left * bytes;
    if (s->kind == K_SYSMEM)
        texel_rows_keyed_to_bgra(src, (long)s->width * 4, w, h, s->key, G.texels.data + at, (long)w * 4);
    else
        texel_rows_to_bgra(s->fmt, src, (long)s->width * bytes, w, h, G.texels.data + at, (long)w * 4);
    *offset = (uint32_t)at;
    return 1;
}

static int clip_rect(RECT *r, const struct gfx_surface *s)
{
    if (r->left < 0) r->left = 0;
    if (r->top < 0) r->top = 0;
    if (r->right > s->width) r->right = s->width;
    if (r->bottom > s->height) r->bottom = s->height;
    return r->right > r->left && r->bottom > r->top;
}

/* Records an upload of the given texels; one used earlier in the frame takes its new pixels where
   the frame has got to, in a copy pass of its own between the render passes. Answers 0 without
   memory for it. */
static int add_upload(struct gfx_surface *s, SDL_GPUTexture *tex, uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                      uint32_t offset, int mips)
{
    struct gpu_upload *u;
    int late = s->pinned == G.frame;
    if (!grow((void **)&G.uploads, &G.uploadCapacity, G.nuploads + 1, sizeof *G.uploads))
        return 0;
    if (!late) {
        /* the first copy pass's uploads stay at the front of the list */
        memmove(G.uploads + G.frameUploads + 1, G.uploads + G.frameUploads,
                (G.nuploads - G.frameUploads) * sizeof *G.uploads);
        u = &G.uploads[G.frameUploads++];
        G.nuploads++;
        {
            size_t i;
            for (i = 0; i < G.nsteps; i++)
                if (G.steps[i].kind == STEP_UPLOADS)
                    G.steps[i].first++;
        }
    } else {
        struct gpu_step *step = last_step();
        if (!step || step->kind != STEP_UPLOADS) {
            step = add_step(STEP_UPLOADS);
            if (!step)
                return 0;
            step->first = (uint32_t)G.nuploads;
        }
        step->count++;
        u = &G.uploads[G.nuploads++];
    }
    u->surface = s;
    u->tex = tex;
    u->x = x;
    u->y = y;
    u->w = w;
    u->h = h;
    u->offset = offset;
    u->mips = mips;
    return 1;
}

/* The GPU texture a draw samples for a surface, with the CPU's changes since its last upload
   staged; the surface stays alive until the frame is drawn. */
static SDL_GPUTexture *texture_for_draw(struct gfx_surface *s)
{
    int dirty, mips;
    RECT r;
    uint32_t offset;
    if (!s)
        return NULL;
    if (is_target(s)) {
        pin(gpu_of(s));
        return gpu_of(s)->tex;
    }
    if (!ensure_texture(s))
        return NULL;
    lock_handles();
    dirty = s->dirty;
    mips = s->mips || (dirty && s->levels > 1);
    r = s->dirtyRect;
    s->dirty = 0;
    s->mips = 0;
    unlock_handles();
    if (dirty && clip_rect(&r, s)) {
        if (stage_texels(s, &r, &offset)
            && add_upload(s, s->tex, (uint32_t)r.left, (uint32_t)r.top, (uint32_t)(r.right - r.left),
                          (uint32_t)(r.bottom - r.top), offset, mips && s->levels > 1))
            mips = 0;
        else
            mark_dirty(s, &r);  /* staged by a later draw */
    }
    if (mips && s->levels > 1 && !add_upload(s, s->tex, 0, 0, 0, 0, 0, 1)) {
        lock_handles();
        s->mips = 1;
        unlock_handles();
    }
    pin(s);
    return s->tex;
}

/* ---- samplers and pipelines ---- */

enum { SAMP_ADDRESS, SAMP_BORDER, SAMP_MAG, SAMP_MIN, SAMP_MIP };

static SDL_GPUSampler *sampler_for(uint32_t key)
{
    SDL_GPUSamplerCreateInfo info;
    unsigned i, h = (key * 2654435761u) % SAMPLER_SLOTS;
    for (i = 0; i < SAMPLER_SLOTS; i++) {
        unsigned at = (h + i) % SAMPLER_SLOTS;
        if (G.samplers[at].sampler && G.samplers[at].key == key)
            return G.samplers[at].sampler;
        if (!G.samplers[at].sampler) {
            DWORD address = key & 7, mag = key >> 3 & 1, min = key >> 4 & 1, mip = key >> 5 & 3;
            SDL_zero(info);
            info.mag_filter = mag ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
            info.min_filter = min ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
            info.mipmap_mode = mip == 2 ? SDL_GPU_SAMPLERMIPMAPMODE_LINEAR : SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
            info.address_mode_u = info.address_mode_v = info.address_mode_w =
                address == GFX_ADDRESS_WRAP ? SDL_GPU_SAMPLERADDRESSMODE_REPEAT
                : address == 2 ? SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT
                : SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            info.min_lod = 0.0f;
            info.max_lod = mip ? 1000.0f : 0.0f;
            /* custom: anisotropic minification whenever the stage minifies linearly, as the
               Direct3D 9 client had it (build/ref/COMMIT.txt) */
            info.enable_anisotropy = min != 0;
            info.max_anisotropy = 16.0f;
            G.samplers[at].sampler = SDL_CreateGPUSampler(G.dev, &info);
            G.samplers[at].key = key;
            return G.samplers[at].sampler;
        }
    }
    return G.pointClamp;
}

/* The sampler of one of the first two stages' states. */
static SDL_GPUSampler *stage_sampler(int stage)
{
    DWORD address = G.samp[stage][SAMP_ADDRESS], mip = G.samp[stage][SAMP_MIP];
    uint32_t key = (address == GFX_ADDRESS_WRAP ? 1u : address == 2 ? 2u : 3u)
                 | (G.samp[stage][SAMP_MAG] == GFX_FILTER_LINEAR ? 8u : 0u)
                 | (G.samp[stage][SAMP_MIN] == GFX_FILTER_LINEAR ? 16u : 0u)
                 | (mip == GFX_MIPFILTER_LINEAR ? 2u : mip == 2 ? 1u : 0u) << 5;
    return sampler_for(key);
}

static SDL_GPUBlendFactor blend_factor(DWORD b, int source)
{
    switch (b) {
    case 1: return SDL_GPU_BLENDFACTOR_ZERO;
    case 3: return SDL_GPU_BLENDFACTOR_SRC_COLOR;
    case 4: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
    case 5: return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    case 6: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    case 7: return SDL_GPU_BLENDFACTOR_DST_ALPHA;
    case 8: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
    case 9: return SDL_GPU_BLENDFACTOR_DST_COLOR;
    case 10: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR;
    case 11: return SDL_GPU_BLENDFACTOR_SRC_ALPHA_SATURATE;
    /* D3DBLEND_BOTHSRCALPHA and BOTHINVSRCALPHA set both factors from the source's */
    case 12: return source ? SDL_GPU_BLENDFACTOR_SRC_ALPHA : SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    case 13: return source ? SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA : SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    default: return SDL_GPU_BLENDFACTOR_ONE;
    }
}

static SDL_GPUCompareOp compare_op(DWORD f)
{
    switch (f) {
    case 1: return SDL_GPU_COMPAREOP_NEVER;
    case 2: return SDL_GPU_COMPAREOP_LESS;
    case 3: return SDL_GPU_COMPAREOP_EQUAL;
    case 4: return SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    case 5: return SDL_GPU_COMPAREOP_GREATER;
    case 6: return SDL_GPU_COMPAREOP_NOT_EQUAL;
    case 7: return SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
    default: return SDL_GPU_COMPAREOP_ALWAYS;
    }
}

/* The pipeline's key: what it holds of the render states and the pass. */
struct pipe_desc {
    int program, layout, topology;
    int blend, src, dst;
    int ztest, zfunc, zwrite;
    int cull;      /* GFX_CULL_* */
    int mask;      /* MASK_* */
    int r32f, depth, samples, a2c;
};

static uint64_t pipe_key(const struct pipe_desc *p)
{
    int log2s = p->samples == SDL_GPU_SAMPLECOUNT_8 ? 3 : p->samples == SDL_GPU_SAMPLECOUNT_4 ? 2
              : p->samples == SDL_GPU_SAMPLECOUNT_2 ? 1 : 0;
    return (uint64_t)p->program | (uint64_t)p->layout << 2 | (uint64_t)p->topology << 4 | (uint64_t)p->blend << 7
         | (uint64_t)(p->src & 15) << 8 | (uint64_t)(p->dst & 15) << 12 | (uint64_t)p->ztest << 16
         | (uint64_t)(p->zfunc & 15) << 17 | (uint64_t)p->zwrite << 21 | (uint64_t)(p->cull & 3) << 22
         | (uint64_t)p->mask << 24 | (uint64_t)p->r32f << 26 | (uint64_t)p->depth << 27 | (uint64_t)log2s << 28
         | (uint64_t)p->a2c << 31;
}

static SDL_GPUGraphicsPipeline *create_pipeline(const struct pipe_desc *p)
{
    SDL_GPUGraphicsPipelineCreateInfo info;
    SDL_GPUColorTargetDescription colour;
    SDL_GPUVertexBufferDescription buffers[2];
    SDL_GPUVertexAttribute attrs[8];
    Uint32 nattrs = 0;
    SDL_zero(info);
    SDL_zero(colour);
    SDL_zero(buffers);
    SDL_zero(attrs);
    buffers[0].slot = 0;
    buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    buffers[1].slot = 1;
    buffers[1].pitch = 4;
    buffers[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
#define ATTR(loc, fmt, off) (attrs[nattrs].location = (loc), attrs[nattrs].buffer_slot = 0, \
                             attrs[nattrs].format = (fmt), attrs[nattrs].offset = (off), nattrs++)
    if (p->program == PROG_FIXED) {
        buffers[0].pitch = p->layout == LAYOUT_TEX2 ? 40 : 32;
        ATTR(0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 0);
        ATTR(1, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 16);
        ATTR(2, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 20);
        ATTR(3, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 24);
        ATTR(4, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, p->layout == LAYOUT_TEX2 ? 32 : 24);
    } else if (p->program == PROG_MESH) {
        buffers[0].pitch = sizeof(struct GfxMeshVertex);
        ATTR(0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
        ATTR(1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 12);
        ATTR(2, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 24);
        ATTR(3, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 28);
        ATTR(4, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 32);
        ATTR(5, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 40);
    } else {
        buffers[0].pitch = sizeof(struct GfxMeshVertex);
        ATTR(0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
        ATTR(1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 32);
    }
#undef ATTR
    attrs[nattrs].location = nattrs;
    attrs[nattrs].buffer_slot = 1;
    attrs[nattrs].format = SDL_GPU_VERTEXELEMENTFORMAT_UINT;
    attrs[nattrs].offset = 0;
    nattrs++;
    info.vertex_shader = G.vs[p->program];
    info.fragment_shader = G.fs[p->program];
    info.vertex_input_state.vertex_buffer_descriptions = buffers;
    info.vertex_input_state.num_vertex_buffers = 2;
    info.vertex_input_state.vertex_attributes = attrs;
    info.vertex_input_state.num_vertex_attributes = nattrs;
    info.primitive_type = (SDL_GPUPrimitiveType)p->topology;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    /* Direct3D 9 culls by the winding on the screen, y down, as the target's pixels are laid out
       here too: GFX_CULL_CW drops the clockwise triangles, GFX_CULL_CCW the others */
    info.rasterizer_state.cull_mode = p->cull == GFX_CULL_NONE ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK;
    info.rasterizer_state.front_face = p->cull == GFX_CULL_CW ? SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
                                                              : SDL_GPU_FRONTFACE_CLOCKWISE;
    info.rasterizer_state.enable_depth_clip = true;
    info.multisample_state.sample_count = (SDL_GPUSampleCount)p->samples;
    info.multisample_state.enable_alpha_to_coverage = p->a2c != 0;
    info.depth_stencil_state.enable_depth_test = p->ztest != 0;
    info.depth_stencil_state.enable_depth_write = p->zwrite != 0;
    info.depth_stencil_state.compare_op = compare_op((DWORD)p->zfunc);
    colour.format = p->r32f ? SDL_GPU_TEXTUREFORMAT_R32_FLOAT : TARGET_FORMAT;
    colour.blend_state.enable_blend = p->blend != 0;
    colour.blend_state.src_color_blendfactor = blend_factor((DWORD)p->src, 1);
    colour.blend_state.dst_color_blendfactor = blend_factor((DWORD)p->dst, 0);
    colour.blend_state.src_alpha_blendfactor = colour.blend_state.src_color_blendfactor;
    colour.blend_state.dst_alpha_blendfactor = colour.blend_state.dst_color_blendfactor;
    colour.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colour.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    colour.blend_state.enable_color_write_mask = true;
    colour.blend_state.color_write_mask = p->mask == MASK_NONE ? 0
        : p->mask == MASK_RGB ? (SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B)
        : (SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A);
    info.target_info.color_target_descriptions = &colour;
    info.target_info.num_color_targets = 1;
    info.target_info.has_depth_stencil_target = p->depth != 0;
    info.target_info.depth_stencil_format = G.depthFormat;
    return SDL_CreateGPUGraphicsPipeline(G.dev, &info);
}

static SDL_GPUGraphicsPipeline *pipeline_for(struct pipe_desc *p)
{
    uint64_t key;
    unsigned i, h;
    if (!p->depth) {
        p->ztest = 0;
        p->zwrite = 0;
        p->zfunc = 0;
    } else if (!p->ztest) {
        p->zwrite = 0;
        p->zfunc = 0;
    }
    if (!p->blend)
        p->src = p->dst = 0;
    key = pipe_key(p);
    h = (unsigned)((key ^ key >> 29) * 0x9e3779b97f4a7c15ull >> 40) % PIPELINE_SLOTS;
    for (i = 0; i < PIPELINE_SLOTS; i++) {
        unsigned at = (h + i) % PIPELINE_SLOTS;
        if (G.pipelines[at].pipeline && G.pipelines[at].key == key)
            return G.pipelines[at].pipeline;
        if (!G.pipelines[at].pipeline) {
            G.pipelines[at].pipeline = create_pipeline(p);
            if (!G.pipelines[at].pipeline) {
                SDL_Log("gfx: SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
                return NULL;
            }
            G.pipelines[at].key = key;
            return G.pipelines[at].pipeline;
        }
    }
    SDL_Log("gfx: more than %d pipelines", PIPELINE_SLOTS);
    return NULL;
}

/* The pipeline of the render states for a draw into a pass. */
static void state_pipe(struct pipe_desc *p, const struct gpu_step *pass, int program, int layout, int topology,
                       int flat)
{
    memset(p, 0, sizeof *p);
    p->program = program;
    p->layout = layout;
    p->topology = topology;
    p->blend = G.rs[GFX_RS_ALPHABLENDENABLE] != 0;
    p->src = (int)G.rs[GFX_RS_SRCBLEND];
    p->dst = (int)G.rs[GFX_RS_DESTBLEND];
    p->ztest = G.rs[GFX_RS_ZENABLE] != 0;
    p->zfunc = (int)G.rs[GFX_RS_ZFUNC];
    p->zwrite = G.rs[GFX_RS_ZWRITEENABLE] != 0;
    p->cull = G.rs[GFX_RS_CULLMODE] == GFX_CULL_CW ? GFX_CULL_CW
            : G.rs[GFX_RS_CULLMODE] == GFX_CULL_CCW ? GFX_CULL_CCW : GFX_CULL_NONE;
    /* the screen and its copy are X8R8G8B8 on Direct3D 9: their alpha stays 1 */
    p->mask = pass->target && pass->target->kind != K_RTTEX ? MASK_RGB : MASK_RGBA;
    p->depth = pass_has_depth(pass);
    p->samples = pass_samples(pass);
    /* custom: multisampling smooths the edges alpha-tested texels cut, where Direct3D 9 had the
       drivers' transparency supersampling, for what it multisampled: draws without blending
       that are not flat on the screen */
    p->a2c = p->samples != SDL_GPU_SAMPLECOUNT_1 && !p->blend && !flat && G.rs[GFX_RS_ALPHATESTENABLE]
             && G.rs[GFX_RS_ALPHAFUNC] == GFX_CMP_GREATEREQUAL;
}

/* ---- records ---- */

static void colour_floats(float *out, DWORD c)
{
    out[0] = (float)(c >> 16 & 0xff) / 255.0f;
    out[1] = (float)(c >> 8 & 0xff) / 255.0f;
    out[2] = (float)(c & 0xff) / 255.0f;
    out[3] = (float)(c >> 24 & 0xff) / 255.0f;
}

/* How a stage's texture coordinates are scaled and bounded (records.hlsli, stage_uv): a target's
   texture is wider and taller than the target (MARGIN), so the coordinates scale to the target's
   part of it, and a clamped read (clamp or border addressing) goes no further than the centre of the
   target's last texel, as clamping at the target's edge would. */
static void stage_scale(float *scale, float *limit, const struct gfx_surface *s, int stage)
{
    const struct gfx_surface *g = is_target(s) ? gpu_of((struct gfx_surface *)s) : NULL;
    DWORD address = G.samp[stage][SAMP_ADDRESS];
    int clamped = address != GFX_ADDRESS_WRAP && address != 2;
    scale[0] = g ? (float)g->width / (float)(g->width + MARGIN) : 1.0f;
    scale[1] = g ? (float)g->height / (float)(g->height + MARGIN) : 1.0f;
    limit[0] = g && clamped ? ((float)g->width - 0.5f) / (float)(g->width + MARGIN) : 3.0e38f;
    limit[1] = g && clamped ? ((float)g->height - 0.5f) / (float)(g->height + MARGIN) : 3.0e38f;
}

/* The fixed-function state of a draw record. */
static void fill_state(struct GpuDraw *r, const struct gpu_step *pass, int kind, int a2c)
{
    int i;
    float w = 1.0f, h = 1.0f;
    struct gfx_surface *t = pass->target;
    if (t) {
        w = (float)t->width;
        h = (float)t->height;
    }
    for (i = 0; i < 2; i++) {
        uint32_t *s = i ? r->stage1 : r->stage0, *sb = i ? r->stage1b : r->stage0b;
        s[0] = G.tss[i][GFX_TSS_COLOROP];
        s[1] = G.tss[i][GFX_TSS_COLORARG1];
        s[2] = G.tss[i][GFX_TSS_COLORARG2];
        s[3] = G.tss[i][GFX_TSS_ALPHAOP];
        sb[0] = G.tss[i][GFX_TSS_ALPHAARG1];
        sb[1] = G.tss[i][GFX_TSS_ALPHAARG2];
        sb[2] = G.tss[i][GFX_TSS_TEXCOORDINDEX];
        sb[3] = (G.stageTex[i] ? DRAW_TEXTURE : 0) | (G.samp[i][SAMP_ADDRESS] == GFX_ADDRESS_BORDER ? DRAW_BORDER : 0);
        colour_floats(i ? r->border1 : r->border0, G.samp[i][SAMP_BORDER]);
        stage_scale(r->uvScale + i * 2, r->uvLimit + i * 2, G.stageTex[i], i);
    }
    r->mode[0] = (uint32_t)kind;
    r->mode[1] = G.rs[GFX_RS_ALPHATESTENABLE] ? G.rs[GFX_RS_ALPHAFUNC] : 0;
    r->mode[2] = G.rs[GFX_RS_FOGENABLE] != 0;
    r->target[0] = (float)(G.rs[GFX_RS_ALPHAREF] & 0xff);
    r->target[1] = w * 0.5f;
    r->target[2] = h * 0.5f;
    r->target[3] = a2c ? 1.0f : 0.0f;
    colour_floats(r->fogColour, G.rs[GFX_RS_FOGCOLOR]);
}

/* The matrices are the ones the Direct3D 9 client drew with (build/ref/COMMIT.txt), computed in
   floats as it computed them: the GPU divides their points and moves them by the viewport as
   Direct3D 9 did, half a pixel on (MARGIN). */

/* the client's 4x3 view (render/gfx.h) as a row-vector 4x4 matrix */
static void matrix_4x3(float *out, const float *m)
{
    int r;
    for (r = 0; r < 4; r++) {
        out[r * 4] = m[r * 3];
        out[r * 4 + 1] = m[r * 3 + 1];
        out[r * 4 + 2] = m[r * 3 + 2];
        out[r * 4 + 3] = r == 3 ? 1.0f : 0.0f;
    }
}

static void multiply(float *out, const float *a, const float *b)
{
    float r[16];
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + a[i * 4 + 3] * b[12 + j];
    memcpy(out, r, sizeof r);
}

/* View space to clip space for a target w x h pixels: a point lands where renderer_project_vertex
   (render/rp_project.c) puts it, as a pre-transformed vertex, and with the same depth
   (render/gfx.h, GfxWorldView). */
static void world_projection(float *out, const struct GfxWorldView *v, int w, int h)
{
    memset(out, 0, 16 * sizeof *out);
    out[0] = 2.0f * v->proj[0] / (float)w;
    out[5] = -2.0f * v->proj[1] / (float)h;
    out[8] = 2.0f * v->centre[0] / (float)w - 1.0f;
    out[9] = 1.0f - 2.0f * v->centre[1] / (float)h;
    out[10] = v->depthScale;
    out[11] = 1.0f;
    out[14] = -v->nearZ * v->depthScale;
}

static int add_view(const float *toClip, const float *toView, const struct GfxWorldView *v)
{
    struct GpuView *view;
    long at = bytes_add(&G.viewRecords, sizeof *view, 16);
    if (at < 0)
        return 0;
    view = (struct GpuView *)(G.viewRecords.data + at);
    memcpy(view->toClip, toClip, sizeof view->toClip);
    if (toView)
        memcpy(view->toView, toView, sizeof view->toView);
    if (v) {
        view->fog[0] = v->fogStart;
        view->fog[1] = v->fogEnd;
        view->fog[2] = v->fogEnd != v->fogStart ? 1.0f / (v->fogEnd - v->fogStart) : 0.0f;
    }
    return (int)(at / (long)sizeof *view);
}

/* The view record of a world view in a target's pixels, made once per frame and target size. */
static int view_for(const struct GfxWorldView *v, int *cache, const struct gpu_step *pass)
{
    float view[16], proj[16], clip[16];
    int w = pass->target ? pass->target->width : 1, h = pass->target ? pass->target->height : 1;
    if (w != G.viewsTargetW || h != G.viewsTargetH) {
        G.cameraView = G.meshView = -1;
        G.viewsTargetW = w;
        G.viewsTargetH = h;
    }
    if (*cache >= 0)
        return *cache;
    matrix_4x3(view, v->view);
    world_projection(proj, v, w, h);
    multiply(clip, view, proj);
    *cache = add_view(clip, view, v);
    return *cache;
}

/* ---- client draws ---- */

static int topology_of(int prim)
{
    switch (prim) {
    case GFX_PT_POINTLIST: return SDL_GPU_PRIMITIVETYPE_POINTLIST;
    case GFX_PT_LINELIST: return SDL_GPU_PRIMITIVETYPE_LINELIST;
    case GFX_PT_LINESTRIP: return SDL_GPU_PRIMITIVETYPE_LINESTRIP;
    case GFX_PT_TRIANGLESTRIP: return SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
    default: return SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    }
}

static void bind_stage(SDL_GPUTextureSamplerBinding *bind, int stage)
{
    SDL_GPUTexture *tex = texture_for_draw(G.stageTex[stage]);
    bind->texture = tex ? tex : G.white;
    bind->sampler = stage_sampler(stage);
}

/* Where the rasterizer puts an edge of a flat draw's vertex to cover the pixels whose centres it
   covered: at the boundary before the first pixel centre at or after the coordinate, snapped to
   1/256 of a pixel as the rasterizer snaps it. The same for an edge on either side under the
   top-left rule, which takes a centre on a left or top edge and leaves one on a right or bottom
   edge. In Direct3D 9's pixel coordinates (centres on whole numbers). */
static float pixel_boundary(float v)
{
    double snapped = floor((double)v * 256.0 + 0.5) / 256.0;
    return (float)(ceil(snapped) - 0.5);
}

static float read_float(const unsigned char *p)
{
    float f;
    memcpy(&f, p, 4);
    return f;
}

/* Direct3D 9 drew flat draws without multisampling: every sample of a pixel was covered when its
   centre was. In a multisampled pass a flat draw's vertices move onto the pixel boundaries that
   keep that coverage (pixel_boundary), and take the values their triangle's plane has there (a
   flat triangle's values are affine on the screen), so each pixel centre reads what it read before:
   exactly so for edges along the pixel rows and columns, which the UI's and the sprites' are. */
static void snap_flat(unsigned char *verts, int nverts, int stride, int topology, const WORD *indices,
                      uint32_t count)
{
    static unsigned char *orig, *done;
    static size_t capacity;
    int nvalues = stride == 40 ? 13 : 11;
    uint32_t t, ntris;
    size_t bytes = (size_t)nverts * stride;
    if (topology != SDL_GPU_PRIMITIVETYPE_TRIANGLELIST && topology != SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP)
        return;
    if (bytes + (size_t)nverts > capacity) {
        unsigned char *grown = realloc(orig, bytes + (size_t)nverts);
        if (!grown)
            return;
        orig = grown;
        capacity = bytes + (size_t)nverts;
    }
    done = orig + bytes;
    memcpy(orig, verts, bytes);
    memset(done, 0, (size_t)nverts);
    ntris = topology == SDL_GPU_PRIMITIVETYPE_TRIANGLELIST ? count / 3 : (count > 2 ? count - 2 : 0);
    for (t = 0; t < ntris; t++) {
        uint32_t first = topology == SDL_GPU_PRIMITIVETYPE_TRIANGLELIST ? t * 3 : t;
        int k[3], i, j;
        float px[3], py[3], value[3][13], dx1, dy1, dx2, dy2, det;
        for (i = 0; i < 3; i++) {
            k[i] = indices ? indices[first + i] : (int)(first + i);
            if (k[i] >= nverts)
                break;
        }
        if (i < 3 || (done[k[0]] && done[k[1]] && done[k[2]]))
            continue;
        for (i = 0; i < 3; i++) {
            const unsigned char *v = orig + (size_t)k[i] * stride;
            px[i] = read_float(v);
            py[i] = read_float(v + 4);
            value[i][0] = read_float(v + 8);
            for (j = 0; j < 8; j++)
                value[i][1 + j] = v[16 + j];
            for (j = 0; j < (stride == 40 ? 4 : 2); j++)
                value[i][9 + j] = read_float(v + 24 + j * 4);
        }
        dx1 = px[1] - px[0];
        dy1 = py[1] - py[0];
        dx2 = px[2] - px[0];
        dy2 = py[2] - py[0];
        det = dx1 * dy2 - dx2 * dy1;
        if (det > -1e-6f && det < 1e-6f)
            continue;
        for (i = 0; i < 3; i++) {
            unsigned char *out = verts + (size_t)k[i] * stride;
            float sx = pixel_boundary(px[i]), sy = pixel_boundary(py[i]), ox, oy;
            if (done[k[i]])
                continue;
            done[k[i]] = 1;
            if (sx == px[i] && sy == py[i])
                continue;
            ox = sx - px[0];
            oy = sy - py[0];
            memcpy(out, &sx, 4);
            memcpy(out + 4, &sy, 4);
            for (j = 0; j < nvalues; j++) {
                float d1 = value[1][j] - value[0][j], d2 = value[2][j] - value[0][j];
                float gx = (d1 * dy2 - d2 * dy1) / det, gy = (d2 * dx1 - d1 * dx2) / det;
                float a = value[0][j] + gx * ox + gy * oy;
                if (j == 0) {
                    memcpy(out + 8, &a, 4);
                } else if (j < 9) {
                    a = floorf(a + 0.5f);
                    out[16 + j - 1] = (unsigned char)(a < 0.0f ? 0.0f : a > 255.0f ? 255.0f : a);
                } else {
                    memcpy(out + 24 + (j - 9) * 4, &a, 4);
                }
            }
        }
    }
}

/* answers whether it recorded the draw */
static int raw_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices)
{
    int flat = (vertexKind & FLAT_KIND) != 0, kind = vertexKind & 0xff, stride = kind == GFX_VTX_TEX2 ? 40 : 32;
    int fan = prim == GFX_PT_TRIANGLEFAN, n = nindices ? nindices : nverts, i;
    struct gpu_step *pass;
    struct gpu_draw *d;
    struct GpuDraw *r;
    struct pipe_desc p;
    long voff, ioff = -1;
    uint32_t count = (uint32_t)n;
    SDL_GPUTextureSamplerBinding bind[2];
    if (!G.dev || !G.target || nverts <= 0 || (kind == GFX_VTX_WORLD && !G.worldCameraSet) || G.shadowPass)
        return 0;
    /* a texture's upload may need a step of its own before the pass */
    bind_stage(bind, 0);
    bind_stage(bind + 1, 1);
    pass = current_pass();
    if (!pass)
        return 0;
    state_pipe(&p, pass, PROG_FIXED, kind == GFX_VTX_TEX2 ? LAYOUT_TEX2 : LAYOUT_TEX1, topology_of(prim), flat);
    d = add_draw(pass);
    if (!d)
        return 0;
    d->program = PROG_FIXED;
    d->pipeline = pipeline_for(&p);
    d->bind[0] = bind[0];
    d->bind[1] = bind[1];
    d->record = add_record(&r);
    if (!r || !d->pipeline) {
        pass->count--;
        G.ndraws--;
        return 0;
    }
    fill_state(r, pass, kind == GFX_VTX_WORLD ? KIND_WORLD : KIND_SCREEN, p.a2c);
    if (kind == GFX_VTX_WORLD)
        r->mode[3] = (uint32_t)view_for(&G.worldCamera, &G.cameraView, pass);
    voff = bytes_add(&G.vertexData, (size_t)nverts * stride, 8);
    if (voff < 0) {
        pass->count--;
        G.ndraws--;
        return 0;
    }
    memcpy(G.vertexData.data + voff, verts, (size_t)nverts * stride);
    d->vertexOffset = (uint32_t)voff;
    if (fan) {
        /* SDL has no fans: their triangles as a list */
        int tris = n > 2 ? n - 2 : 0;
        WORD *out;
        ioff = bytes_add(&G.indexData, (size_t)tris * 3 * 2, 4);
        if (ioff < 0)
            return 0;
        out = (WORD *)(G.indexData.data + ioff);
        for (i = 0; i < tris; i++) {
            out[i * 3] = indices ? indices[0] : 0;
            out[i * 3 + 1] = indices ? indices[i + 1] : (WORD)(i + 1);
            out[i * 3 + 2] = indices ? indices[i + 2] : (WORD)(i + 2);
        }
        count = (uint32_t)tris * 3;
    } else if (nindices) {
        ioff = bytes_add(&G.indexData, (size_t)nindices * 2, 4);
        if (ioff < 0)
            return 0;
        memcpy(G.indexData.data + ioff, indices, (size_t)nindices * 2);
    }
    d->indexed = ioff >= 0;
    d->firstIndex = ioff >= 0 ? (uint32_t)(ioff / 2) : 0;
    d->count = count;
    if (flat && kind != GFX_VTX_WORLD && p.samples != SDL_GPU_SAMPLECOUNT_1)
        snap_flat(G.vertexData.data + voff, nverts, stride, fan ? SDL_GPU_PRIMITIVETYPE_TRIANGLELIST : p.topology,
                  d->indexed ? (const WORD *)(G.indexData.data + ioff) : NULL, count);
    G.target->version = next_version();
    return 1;
}

/* The client's vertices, batched (gfx_batch.c) or not, counted as diag/perf.h says; the blit
   quads draw through raw_draw alone and count as blits. */
static void client_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices)
{
    if (!raw_draw(prim, vertexKind, verts, nverts, indices, nindices))
        return;
    g_perf.draws++;
    g_perf.prims += perf_prim_count(prim, nindices ? nindices : nverts);
    g_perf.vertexBytes += (unsigned)nverts * ((vertexKind & 0xff) == GFX_VTX_TEX2 ? 40 : 32) + (unsigned)nindices * 2;
}

/* Pre-transformed vertices that all share one rhw lie flat on the screen (the UI, sprites); the
   Direct3D 9 client drew those without multisampling (build/ref/COMMIT.txt). */
static int flat_draw(int vertexKind, const void *verts, int nverts)
{
    int stride = vertexKind == GFX_VTX_TEX2 ? 40 : 32, i;
    const unsigned char *v = verts;
    DWORD rhw, other;
    if (nverts < 1)
        return 1;
    memcpy(&rhw, v + 12, 4);
    for (i = 1; i < nverts; i++) {
        memcpy(&other, v + (size_t)i * stride + 12, 4);
        if (other != rhw)
            return 0;
    }
    return 1;
}

void gfx_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices)
{
    g_perf.drawCalls++;
    if (vertexKind != GFX_VTX_WORLD && flat_draw(vertexKind, verts, nverts))
        vertexKind |= FLAT_KIND;
    if (batch_add(prim, vertexKind, verts, nverts, indices, nindices))
        return;
    batch_flush();
    client_draw(prim, vertexKind, verts, nverts, indices, nindices);
}

/* A quad from a rectangle of a surface to one of a target: one texel to a pixel when they are the
   same size, stretched with bilinear filtering otherwise, and nothing blended, tested or fogged;
   the client's states are as before after. */
static void draw_textured_quad(struct gfx_surface *dst, const RECT *d, struct gfx_surface *src, const RECT *s)
{
    struct { float x, y, z, rhw; DWORD diffuse, specular; float u, v; } v[4];
    DWORD rs[N_RS], tss0[N_TSS], tss1[N_TSS], samp0[5];
    struct gfx_surface *stage0 = G.stageTex[0], *stage1 = G.stageTex[1], *prevTarget = G.target;
    float tw = (float)src->width, th = (float)src->height;
    int i;
    for (i = 0; i < 4; i++) {
        v[i].z = 0.0f;
        v[i].rhw = 1.0f;
        v[i].diffuse = 0xffffffff;
        v[i].specular = 0;
    }
    v[0].x = (float)d->left - 0.5f;  v[0].y = (float)d->top - 0.5f;    v[0].u = s->left / tw;  v[0].v = s->top / th;
    v[1].x = (float)d->right - 0.5f; v[1].y = (float)d->top - 0.5f;    v[1].u = s->right / tw; v[1].v = s->top / th;
    v[2].x = (float)d->left - 0.5f;  v[2].y = (float)d->bottom - 0.5f; v[2].u = s->left / tw;  v[2].v = s->bottom / th;
    v[3].x = (float)d->right - 0.5f; v[3].y = (float)d->bottom - 0.5f; v[3].u = s->right / tw; v[3].v = s->bottom / th;
    batch_flush();
    g_perf.blitQuads++;
    memcpy(rs, G.rs, sizeof rs);
    memcpy(tss0, G.tss[0], sizeof tss0);
    memcpy(tss1, G.tss[1], sizeof tss1);
    memcpy(samp0, G.samp[0], sizeof samp0);
    G.rs[GFX_RS_ALPHATESTENABLE] = 0;
    G.rs[GFX_RS_ALPHABLENDENABLE] = 0;
    G.rs[GFX_RS_ZENABLE] = 0;
    G.rs[GFX_RS_ZWRITEENABLE] = 0;
    G.rs[GFX_RS_FOGENABLE] = 0;
    G.rs[GFX_RS_CULLMODE] = GFX_CULL_NONE;
    G.tss[0][GFX_TSS_COLOROP] = GFX_TOP_SELECTARG1;
    G.tss[0][GFX_TSS_COLORARG1] = GFX_TA_TEXTURE;
    G.tss[0][GFX_TSS_ALPHAOP] = GFX_TOP_SELECTARG1;
    G.tss[0][GFX_TSS_ALPHAARG1] = GFX_TA_TEXTURE;
    G.tss[0][GFX_TSS_TEXCOORDINDEX] = 0;
    G.tss[1][GFX_TSS_COLOROP] = GFX_TOP_DISABLE;
    G.tss[1][GFX_TSS_ALPHAOP] = GFX_TOP_DISABLE;
    G.samp[0][SAMP_ADDRESS] = GFX_ADDRESS_CLAMP;
    G.samp[0][SAMP_MAG] = 1;
    G.samp[0][SAMP_MIN] = 1;
    G.samp[0][SAMP_MIP] = GFX_MIPFILTER_NONE;
    G.stageTex[0] = src;
    G.stageTex[1] = NULL;
    G.target = dst;
    raw_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1 | FLAT_KIND, v, 4, NULL, 0);
    memcpy(G.rs, rs, sizeof rs);
    memcpy(G.tss[0], tss0, sizeof tss0);
    memcpy(G.tss[1], tss1, sizeof tss1);
    memcpy(G.samp[0], samp0, sizeof samp0);
    G.stageTex[0] = stage0;
    G.stageTex[1] = stage1;
    G.target = prevTarget;
}

/* ---- meshes ---- */

gfx_mesh *gfx_mesh_create(const struct GfxMeshVertex *verts, int nverts, const WORD *indices, int nindices)
{
    struct gfx_mesh *m;
    if (!verts || !indices || nverts <= 0 || nverts > 0x10000 || nindices <= 0 || nindices % 3)
        return NULL;
    m = calloc(1, sizeof *m);
    if (!m)
        return NULL;
    m->verts = malloc((size_t)nverts * sizeof *verts);
    m->indices = malloc((size_t)nindices * sizeof *indices);
    if (!m->verts || !m->indices) {
        free(m->verts);
        free(m->indices);
        free(m);
        return NULL;
    }
    memcpy(m->verts, verts, (size_t)nverts * sizeof *verts);
    memcpy(m->indices, indices, (size_t)nindices * sizeof *indices);
    m->nverts = nverts;
    m->nindices = nindices;
    m->next = G.meshes;
    if (G.meshes)
        G.meshes->prev = m;
    G.meshes = m;
    return m;
}

static void mesh_free(struct gfx_mesh *m)
{
    if (m->buffer && G.dev)
        SDL_ReleaseGPUBuffer(G.dev, m->buffer);
    if (m->prev)
        m->prev->next = m->next;
    else
        G.meshes = m->next;
    if (m->next)
        m->next->prev = m->prev;
    free(m->verts);
    free(m->indices);
    free(m);
}

void gfx_mesh_destroy(gfx_mesh *m)
{
    if (!m)
        return;
    if (m->pinned == G.frame && G.frame)
        m->dead = 1;
    else
        mesh_free(m);
}

/* The mesh's buffer, filled through the frame's upload on its first draw: the vertices, then the
   indices 32 bits wide, since a draw may start at any index and Metal takes an index buffer's offset
   in multiples of 4 bytes. */
static int mesh_ready(struct gfx_mesh *m)
{
    SDL_GPUBufferCreateInfo info;
    uint32_t vbytes = (uint32_t)m->nverts * sizeof *m->verts, ibytes = (uint32_t)m->nindices * 4;
    long at;
    struct gpu_mesh_upload *u;
    uint32_t *indices;
    int i;
    if (m->buffer)
        return 1;
    SDL_zero(info);
    info.usage = SDL_GPU_BUFFERUSAGE_VERTEX | SDL_GPU_BUFFERUSAGE_INDEX;
    info.size = vbytes + ibytes;
    m->buffer = SDL_CreateGPUBuffer(G.dev, &info);
    if (!m->buffer)
        return 0;
    m->indexOffset = vbytes;
    at = bytes_add(&G.texels, vbytes + ibytes, 16);
    /* pinned with its upload: destroyed in this frame, it stays until the upload is made */
    if (at < 0 || !grow((void **)&G.meshUploads, &G.meshUploadCapacity, G.nmeshUploads + 1, sizeof *G.meshUploads)
        || !pin_mesh(m)) {
        SDL_ReleaseGPUBuffer(G.dev, m->buffer);
        m->buffer = NULL;
        return 0;
    }
    memcpy(G.texels.data + at, m->verts, vbytes);
    indices = (uint32_t *)(G.texels.data + at + vbytes);
    for (i = 0; i < m->nindices; i++)
        indices[i] = m->indices[i];
    u = &G.meshUploads[G.nmeshUploads++];
    u->mesh = m;
    u->buffer = m->buffer;
    u->offset = (uint32_t)at;
    u->size = vbytes + ibytes;
    g_perf.vertexBytes += vbytes + (uint32_t)m->nindices * 2;
    return 1;
}

void gfx_set_world_view(const struct GfxWorldView *view)
{
    if (!G.worldViewSet || memcmp(&G.worldView, view, sizeof *view))
        G.meshView = G.casterView = -1;
    G.worldView = *view;
    G.worldViewSet = 1;
}

void gfx_set_world_camera(const struct GfxWorldView *view)
{
    batch_flush();
    if (!G.worldCameraSet || memcmp(&G.worldCamera, view, sizeof *view))
        G.cameraView = -1;
    G.worldCamera = *view;
    G.worldCameraSet = 1;
}

/* A caster's view: view space to the cascade's light clip space; the viewport puts its points on
   the texels Direct3D 9 put them on, as the lookups' texture matrix expects. */
static int caster_view(void)
{
    float view[16], clip[16];
    if (G.casterCascade == G.shadowCascade && G.casterView >= 0)
        return G.casterView;
    matrix_4x3(view, G.worldView.view);
    multiply(clip, view, G.shadow.fromView[G.shadowCascade]);
    G.casterView = add_view(clip, view, &G.worldView);
    G.casterCascade = G.shadowCascade;
    return G.casterView;
}

void gfx_draw_mesh(gfx_mesh *m, const struct GfxMeshDraw *draw)
{
    struct gpu_step *pass;
    struct gpu_draw *d;
    struct GpuDraw *r;
    struct pipe_desc p;
    SDL_GPUTextureSamplerBinding bind[2];
    g_perf.drawCalls++;
    batch_flush();
    if (!G.dev || !G.target || !m || !draw || !G.worldViewSet || draw->first < 0 || draw->count <= 0
        || draw->count % 3 || draw->first > m->nindices - draw->count)
        return;
    if (G.shadowPass && G.shadowCascade < 0)
        return;
    if (!mesh_ready(m))
        return;
    bind_stage(bind, 0);
    if (!G.shadowPass)
        bind_stage(bind + 1, 1);
    pass = current_pass();
    if (!pass)
        return;
    if (!pin_mesh(m))
        return;
    g_perf.draws++;
    g_perf.prims += (unsigned)draw->count / 3;
    if (G.shadowPass) {
        memset(&p, 0, sizeof p);
        p.program = PROG_CASTER;
        p.layout = LAYOUT_MESH;
        p.topology = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        p.ztest = 1;
        p.zfunc = GFX_CMP_LESSEQUAL;
        p.zwrite = 1;
        p.cull = GFX_CULL_NONE;
        p.r32f = 1;
        p.depth = 1;
        p.samples = SDL_GPU_SAMPLECOUNT_1;
    } else {
        state_pipe(&p, pass, PROG_MESH, LAYOUT_MESH, SDL_GPU_PRIMITIVETYPE_TRIANGLELIST, 0);
    }
    d = add_draw(pass);
    if (!d)
        return;
    d->program = p.program;
    d->pipeline = pipeline_for(&p);
    d->mesh = m;
    d->indexed = 1;
    d->firstIndex = (uint32_t)draw->first;
    d->count = (uint32_t)draw->count;
    d->bind[0] = bind[0];
    d->record = add_record(&r);
    if (!r || !d->pipeline) {
        pass->count--;
        G.ndraws--;
        return;
    }
    r->uvU[0] = draw->uv[0];
    r->uvU[1] = draw->uv[2];
    r->uvU[2] = draw->uv[4];
    r->uvV[0] = draw->uv[1];
    r->uvV[1] = draw->uv[3];
    r->uvV[2] = draw->uv[5];
    r->mesh[0] = draw->kind == GFX_MESH_MODEL;
    if (G.shadowPass) {
        r->mode[0] = KIND_CASTER;
        r->stage0b[3] = G.stageTex[0] ? DRAW_TEXTURE : 0;
        stage_scale(r->uvScale, r->uvLimit, G.stageTex[0], 0);
        r->mode[3] = (uint32_t)caster_view();
        return;
    }
    d->bind[1] = bind[1];
    d->bind[2].texture = G.shadowReceive ? G.shadowMaps : G.noShadow;
    d->bind[2].sampler = G.pointClamp;
    fill_state(r, pass, KIND_MESH, p.a2c);
    memcpy(r->colour, draw->colour, sizeof r->colour);
    r->mesh[1] = draw->kind == GFX_MESH_MODEL && draw->lightGround;
    r->mesh[2] = G.tss[1][GFX_TSS_COLOROP] != GFX_TOP_DISABLE;
    r->mesh[3] = G.shadowReceive ? (uint32_t)shadow_record() + 1 : 0;
    r->mode[3] = (uint32_t)view_for(&G.worldView, &G.meshView, pass);
    G.target->version = next_version();
}

/* ---- shadows ---- */

static int shadow_supported(void)
{
    const char *env = SDL_getenv("RAGEXE_SHADOWS");
    if (env && env[0] == '0' && !env[1])
        return 0;
    return G.dev && SDL_GPUTextureSupportsFormat(G.dev, SDL_GPU_TEXTUREFORMAT_R32_FLOAT, SDL_GPU_TEXTURETYPE_2D_ARRAY,
                                                 SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
}

int gfx_shadows_supported(void)
{
    if (!G.dev)
        return 0;
    if (G.shadowState == 0)
        G.shadowState = shadow_supported() ? 1 : -1;
    return G.shadowState > 0;
}

static int shadow_prepare(void)
{
    SDL_GPUTextureCreateInfo info;
    if (!gfx_shadows_supported())
        return 0;
    if (G.shadowMaps && G.shadowDepth)
        return 1;
    SDL_zero(info);
    info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
    info.format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = info.height = GFX_SHADOW_SIZE + MARGIN;
    info.layer_count_or_depth = GFX_SHADOW_CASCADES;
    info.num_levels = 1;
    G.shadowMaps = SDL_CreateGPUTexture(G.dev, &info);
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = G.depthFormat;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    info.layer_count_or_depth = 1;
    G.shadowDepth = SDL_CreateGPUTexture(G.dev, &info);
    if (!G.shadowMaps || !G.shadowDepth) {
        SDL_Log("gfx: shadow maps: %s", SDL_GetError());
        G.shadowState = -1;
        return 0;
    }
    return 1;
}

/* A cascade's view-to-light matrix in its shadow map's texture space, with the texel centres where
   the caster pass drew them (caster_view). */
static void shadow_texture_matrix(float *out, int cascade)
{
    float tex[16];
    memset(tex, 0, sizeof tex);
    tex[0] = 0.5f;
    tex[5] = -0.5f;
    tex[10] = 1.0f;
    tex[12] = 0.5f + 0.5f / GFX_SHADOW_SIZE;
    tex[13] = 0.5f + 0.5f / GFX_SHADOW_SIZE;
    tex[15] = 1.0f;
    multiply(out, G.shadow.fromView[cascade], tex);
}

/* The shadow record of the cascades drawn last, in the frame's records: made again after a flush
   started a new list of them. */
static int shadow_record(void)
{
    const struct GfxShadow *shadow = &G.shadow;
    struct GpuShadow *s;
    long at;
    int c;
    if (G.shadowRecord >= 0)
        return G.shadowRecord;
    at = bytes_add(&G.shadowRecords, sizeof *s, 16);
    if (at < 0)
        return 0;
    G.shadowRecord = (int)(at / (long)sizeof *s);
    s = (struct GpuShadow *)(G.shadowRecords.data + at);
    for (c = 0; c < GFX_SHADOW_CASCADES; c++) {
        int on = c < shadow->cascades;
        shadow_texture_matrix(s->fromView[c], on ? c : 0);
        s->farDepth[c] = on ? shadow->farDepth[c] : 0.0f;
        s->offset[c] = on ? shadow->offset[c] : 0.0f;
        s->blur[c] = on ? shadow->blur[c] : 0.0f;
        s->slant[c] = on ? shadow->slant[c] : 0.0f;
    }
    for (c = 0; c < 3; c++) {
        s->lightView[c] = shadow->lightView[c];
        s->ambient[c] = shadow->ambient[c];
    }
    s->lightView[3] = shadow->bias;
    s->ambient[3] = shadow->darkness > 1.0f / 255.0f ? shadow->darkness : 1.0f / 255.0f;
    s->mapSize[0] = (float)GFX_SHADOW_SIZE;
    return G.shadowRecord;
}

int gfx_shadow_begin(const struct GfxShadow *shadow)
{
    batch_flush();
    G.shadowReady = 0;
    G.shadowReceive = 0;
    if (!G.dev || G.shadowPass || shadow->cascades < 1 || shadow->cascades > GFX_SHADOW_CASCADES || !shadow_prepare())
        return 0;
    G.shadow = *shadow;
    G.shadowRecord = -1;
    G.shadowCascade = -1;
    G.shadowPass = 1;
    return 1;
}

int gfx_shadow_cascade(int cascade)
{
    struct gpu_step *s;
    batch_flush();
    G.shadowCascade = -1;
    if (!G.shadowPass || cascade < 0 || cascade >= G.shadow.cascades)
        return 0;
    G.shadowCascade = cascade;
    s = add_step(STEP_PASS);
    if (!s) {
        G.shadowCascade = -1;
        return 0;
    }
    s->layer = cascade;
    s->loadColour = SDL_GPU_LOADOP_CLEAR;
    s->clearColour.r = s->clearColour.g = s->clearColour.b = s->clearColour.a = 1.0f;
    s->loadDepth = SDL_GPU_LOADOP_CLEAR;
    s->clearDepth = 1.0f;
    s->first = (uint32_t)G.ndraws;
    return 1;
}

void gfx_shadow_end(void)
{
    if (!G.shadowPass)
        return;
    G.shadowPass = 0;
    G.shadowCascade = -1;
    G.shadowReady = 1;
}

void gfx_shadow_receive(int receive)
{
    batch_flush();
    G.shadowReceive = receive && G.shadowReady;
}

/* ---- the frame's replay ---- */

static int ensure_buffer(SDL_GPUBuffer **buffer, uint32_t *size, uint32_t need, SDL_GPUBufferUsageFlags usage)
{
    SDL_GPUBufferCreateInfo info;
    uint32_t s = *size ? *size : 4096;
    if (*buffer && need <= *size)
        return 1;
    while (s < need)
        s *= 2;
    if (*buffer)
        SDL_ReleaseGPUBuffer(G.dev, *buffer);
    SDL_zero(info);
    info.usage = usage;
    info.size = s;
    *buffer = SDL_CreateGPUBuffer(G.dev, &info);
    *size = *buffer ? s : 0;
    return *buffer != NULL;
}

/* The identity instance buffer (0, 1, 2, ...): a draw's first instance is its record. */
static long instances_needed(uint32_t records)
{
    uint32_t count = G.instanceCount ? G.instanceCount : 4096, i;
    long at;
    uint32_t *ids;
    uint32_t size = 0;
    if (G.instances && records <= G.instanceCount)
        return -1;
    while (count < records)
        count *= 2;
    if (G.instances)
        SDL_ReleaseGPUBuffer(G.dev, G.instances);
    G.instances = NULL;
    G.instanceCount = 0;
    if (!ensure_buffer(&G.instances, &size, count * 4, SDL_GPU_BUFFERUSAGE_VERTEX))
        return -2;
    at = bytes_add(&G.texels, (size_t)count * 4, 16);
    if (at < 0)
        return -2;
    ids = (uint32_t *)(G.texels.data + at);
    for (i = 0; i < count; i++)
        ids[i] = i;
    G.instanceCount = count;
    return at;
}

static void copy_into(SDL_GPUCopyPass *copy, uint32_t from, SDL_GPUBuffer *buffer, uint32_t size)
{
    SDL_GPUTransferBufferLocation src;
    SDL_GPUBufferRegion dst;
    if (!size)
        return;
    src.transfer_buffer = G.upload;
    src.offset = from;
    dst.buffer = buffer;
    dst.offset = 0;
    dst.size = size;
    SDL_UploadToGPUBuffer(copy, &src, &dst, true);
}

/* answers whether a texture uploaded wants its mipmaps made */
static int run_uploads(SDL_GPUCopyPass *copy, size_t first, size_t count, uint32_t base)
{
    size_t i;
    int mips = 0;
    for (i = first; i < first + count; i++) {
        struct gpu_upload *u = &G.uploads[i];
        SDL_GPUTextureTransferInfo src;
        SDL_GPUTextureRegion dst;
        if (!u->w || !u->h) {
            mips |= u->mips;
            continue;
        }
        SDL_zero(src);
        SDL_zero(dst);
        src.transfer_buffer = G.upload;
        src.offset = base + u->offset;
        src.pixels_per_row = u->w;
        src.rows_per_layer = u->h;
        dst.texture = u->tex;
        dst.x = u->x;
        dst.y = u->y;
        dst.w = u->w;
        dst.h = u->h;
        dst.d = 1;
        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        mips |= u->mips;
    }
    return mips;
}

static void make_mipmaps(SDL_GPUCommandBuffer *cmd, size_t first, size_t count)
{
    size_t i;
    for (i = first; i < first + count; i++)
        if (G.uploads[i].mips)
            SDL_GenerateMipmapsForGPUTexture(cmd, G.uploads[i].tex);
}

static void bind_program(SDL_GPURenderPass *pass, int program)
{
    SDL_GPUBuffer *vertex[3], *fragment[2];
    Uint32 nv = 2, nf = 1;
    vertex[0] = G.records;
    vertex[1] = G.views;
    vertex[2] = G.shadows;
    fragment[0] = G.records;
    fragment[1] = G.shadows;
    if (program == PROG_MESH) {
        nv = 3;
        nf = 2;
    }
    SDL_BindGPUVertexStorageBuffers(pass, 0, vertex, nv);
    SDL_BindGPUFragmentStorageBuffers(pass, 0, fragment, nf);
}

static void run_pass(SDL_GPUCommandBuffer *cmd, const struct gpu_step *s)
{
    SDL_GPUColorTargetInfo colour;
    SDL_GPUDepthStencilTargetInfo depth;
    SDL_GPURenderPass *pass;
    SDL_GPUBufferBinding binding;
    SDL_GPUGraphicsPipeline *bound = NULL;
    int program = -1, ms = s->target && s->target == G.screen && G.msaa, hasDepth = pass_has_depth(s);
    const struct gfx_mesh *mesh = NULL;
    uint32_t vertexOffset = ~0u;
    SDL_GPUTextureSamplerBinding textures[3];
    SDL_Rect scissor;
    int w = s->target ? s->target->width : GFX_SHADOW_SIZE, h = s->target ? s->target->height : GFX_SHADOW_SIZE;
    SDL_GPUViewport viewport;
    uint32_t i;
    SDL_zero(colour);
    SDL_zero(depth);
    SDL_zero(textures);
    if (s->target) {
        colour.texture = ms ? G.msaa : s->target->tex;
        if (ms && !s->unresolve) {
            colour.resolve_texture = s->target->tex;
            colour.store_op = SDL_GPU_STOREOP_RESOLVE_AND_STORE;
        } else {
            colour.store_op = SDL_GPU_STOREOP_STORE;
        }
    } else {
        colour.texture = G.shadowMaps;
        colour.layer_or_depth_plane = (Uint32)s->layer;
        colour.store_op = SDL_GPU_STOREOP_STORE;
    }
    if (!colour.texture)
        return;
    colour.load_op = s->loadColour;
    colour.clear_color = s->clearColour;
    if (hasDepth) {
        depth.texture = s->target ? G.depth : G.shadowDepth;
        depth.load_op = s->loadDepth;
        depth.clear_depth = s->clearDepth;
        depth.store_op = s->target ? SDL_GPU_STOREOP_STORE : SDL_GPU_STOREOP_DONT_CARE;
        depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    }
    pass = SDL_BeginGPURenderPass(cmd, &colour, 1, hasDepth ? &depth : NULL);
    if (!pass) {
        SDL_Log("gfx: SDL_BeginGPURenderPass: %s", SDL_GetError());
        return;
    }
    /* the target moved by half a pixel, to where Direct3D 9's pixel centres were (MARGIN) */
    viewport.x = viewport.y = 0.5f;
    viewport.w = (float)w;
    viewport.h = (float)h;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(pass, &viewport);
    scissor.x = scissor.y = 0;
    scissor.w = w;
    scissor.h = h;
    SDL_SetGPUScissor(pass, &scissor);
    binding.buffer = G.instances;
    binding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 1, &binding, 1);
    for (i = s->first; i < s->first + s->count; i++) {
        const struct gpu_draw *d = &G.draws[i];
        int nbind = d->program == PROG_MESH ? 3 : d->program == PROG_FIXED ? 2 : 1;
        if (!d->pipeline)
            continue;
        if (d->pipeline != bound) {
            SDL_BindGPUGraphicsPipeline(pass, d->pipeline);
            bound = d->pipeline;
        }
        if (d->program != program) {
            bind_program(pass, d->program);
            program = d->program;
            memset(textures, 0, sizeof textures);
            mesh = NULL;
            vertexOffset = ~0u;
        }
        if (memcmp(textures, d->bind, (size_t)nbind * sizeof *textures)) {
            memcpy(textures, d->bind, (size_t)nbind * sizeof *textures);
            SDL_BindGPUFragmentSamplers(pass, 0, textures, (Uint32)nbind);
        }
        if (d->mesh) {
            if (d->mesh != mesh) {
                binding.buffer = d->mesh->buffer;
                binding.offset = 0;
                SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
                binding.offset = d->mesh->indexOffset;
                SDL_BindGPUIndexBuffer(pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
                mesh = d->mesh;
            }
        } else {
            if (mesh || vertexOffset == ~0u) {
                binding.buffer = G.indices;
                binding.offset = 0;
                SDL_BindGPUIndexBuffer(pass, &binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
                mesh = NULL;
            }
            if (d->vertexOffset != vertexOffset) {
                binding.buffer = G.vertices;
                binding.offset = d->vertexOffset;
                SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
                vertexOffset = d->vertexOffset;
            }
        }
        if (d->indexed)
            SDL_DrawGPUIndexedPrimitives(pass, d->count, 1, d->firstIndex, 0, d->record);
        else
            SDL_DrawGPUPrimitives(pass, d->count, 1, 0, d->record);
    }
    SDL_EndGPURenderPass(pass);
}

static void run_copy(SDL_GPUCommandBuffer *cmd, const struct gpu_step *s)
{
    int sw = s->srcRect.right - s->srcRect.left, sh = s->srcRect.bottom - s->srcRect.top;
    int dw = s->dstRect.right - s->dstRect.left, dh = s->dstRect.bottom - s->dstRect.top;
    if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
        return;
    if (sw == dw && sh == dh) {
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
        SDL_GPUTextureLocation from, to;
        SDL_zero(from);
        SDL_zero(to);
        from.texture = s->src;
        from.x = (Uint32)s->srcRect.left;
        from.y = (Uint32)s->srcRect.top;
        to.texture = s->dst;
        to.x = (Uint32)s->dstRect.left;
        to.y = (Uint32)s->dstRect.top;
        SDL_CopyGPUTextureToTexture(copy, &from, &to, (Uint32)sw, (Uint32)sh, 1, false);
        SDL_EndGPUCopyPass(copy);
    } else {
        SDL_GPUBlitInfo blit;
        SDL_zero(blit);
        blit.source.texture = s->src;
        blit.source.x = (Uint32)s->srcRect.left;
        blit.source.y = (Uint32)s->srcRect.top;
        blit.source.w = (Uint32)sw;
        blit.source.h = (Uint32)sh;
        blit.destination.texture = s->dst;
        blit.destination.x = (Uint32)s->dstRect.left;
        blit.destination.y = (Uint32)s->dstRect.top;
        blit.destination.w = (Uint32)dw;
        blit.destination.h = (Uint32)dh;
        blit.load_op = SDL_GPU_LOADOP_LOAD;
        blit.filter = SDL_GPU_FILTER_NEAREST;
        SDL_BlitGPUTexture(cmd, &blit);
    }
}

/* Whether a frame shown without waiting may ask for the window's next picture: once a refresh of
   the window's display (60 Hz where its mode does not say) has passed since the last one, when SDL's
   acquire, which waits for the presentation engine, no longer has to. */
static int present_due(void)
{
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(G.window));
    float hz = mode && mode->refresh_rate > 0.0f ? mode->refresh_rate : 60.0f;
    return SDL_GetTicksNS() - G.presentedNs >= (Uint64)(1e9 / hz);
}

/* The screen into the window's next picture, fitted (present_fit). With `wait` the frame waits for
   one; without, it is drawn but not shown unless present_due, as Direct3D 9's flip without waiting
   (D3DPRESENT_DONOTWAIT) left a frame the screen had no room for. */
static void present_into(SDL_GPUCommandBuffer *cmd, int wait)
{
    SDL_GPUTexture *swap = NULL;
    Uint32 w = 0, h = 0;
    int fx, fy, fw, fh, pw = 0, ph = 0;
    struct PresentCover cover = present_cover;
    SDL_GPUBlitInfo blit;
    if (!G.claimed || !G.screen || !G.screen->tex || (!wait && !present_due()))
        return;
    if (!(wait ? SDL_WaitAndAcquireGPUSwapchainTexture(cmd, G.window, &swap, &w, &h)
               : SDL_AcquireGPUSwapchainTexture(cmd, G.window, &swap, &w, &h))
        || !swap)
        return;
    G.presentedNs = SDL_GetTicksNS();
    /* the cover is in window points, the swapchain in pixels */
    SDL_GetWindowSize(G.window, &pw, &ph);
    if (pw > 0 && pw != (int)w) {
        cover.left = (int)((long long)cover.left * w / pw);
        cover.right = (int)((long long)cover.right * w / pw);
    }
    if (ph > 0 && ph != (int)h) {
        cover.top = (int)((long long)cover.top * h / ph);
        cover.bottom = (int)((long long)cover.bottom * h / ph);
        cover.lift = (int)((long long)cover.lift * h / ph);
    }
    present_fit((int)w, (int)h, G.screen->width, G.screen->height, &cover, &fx, &fy, &fw, &fh);
    SDL_zero(blit);
    blit.source.texture = G.screen->tex;
    blit.source.w = (Uint32)G.screen->width;
    blit.source.h = (Uint32)G.screen->height;
    blit.destination.texture = swap;
    blit.destination.x = (Uint32)(fx > 0 ? fx : 0);
    blit.destination.y = (Uint32)(fy > 0 ? fy : 0);
    blit.destination.w = (Uint32)fw;
    blit.destination.h = (Uint32)fh;
    blit.load_op = fx || fy || fw != (int)w || fh != (int)h ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_DONT_CARE;
    blit.clear_color.a = 1.0f;
    /* filtered only where it shrinks: a filtered enlargement would read the texture's margin past
       the picture's last row and column (MARGIN) */
    blit.filter = fw < G.screen->width || fh < G.screen->height ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
    SDL_BlitGPUTexture(cmd, &blit);
}

static void frame_reset(void)
{
    size_t i;
    for (i = 0; i < G.npins; i++) {
        struct gfx_surface *s = G.pins[i];
        int dead;
        lock_handles();
        s->pinned = 0;
        dead = s->dead;
        unlock_handles();
        if (dead)
            destroy_handle(s);
    }
    for (i = 0; i < G.nmeshPins; i++) {
        struct gfx_mesh *m = G.meshPins[i];
        m->pinned = 0;
        if (m->dead)
            mesh_free(m);
    }
    G.npins = G.nmeshPins = 0;
    G.nsteps = G.ndraws = G.nuploads = G.frameUploads = G.nmeshUploads = 0;
    G.drawRecords.size = G.viewRecords.size = G.shadowRecords.size = 0;
    G.vertexData.size = G.indexData.size = G.texels.size = 0;
    G.cameraView = G.meshView = G.casterView = G.casterCascade = G.shadowRecord = -1;
    G.frame++;
    if (!G.frame)
        G.frame = 1;
}

/* Waits for the GPU to finish the frame submitted FRAMES_IN_FLIGHT submissions ago, or for all of
   them. */
static void wait_frames(int all)
{
    int i;
    for (i = 0; i < FRAMES_IN_FLIGHT; i++) {
        SDL_GPUFence **f = &G.inFlight[(G.inFlightNext + i) % FRAMES_IN_FLIGHT];
        if (*f && (all || i == 0)) {
            SDL_WaitForGPUFences(G.dev, true, f, 1);
            SDL_ReleaseGPUFence(G.dev, *f);
            *f = NULL;
        }
    }
}

/* What a flush also does: present the screen, waiting for the window's next picture or not, and
   download a rectangle of a texture. */
struct flush_extra {
    int present, wait;
    SDL_GPUTexture *download;
    int x, y, w, h;
    unsigned char *out;  /* B8G8R8A8 rows */
    size_t pitch;        /* bytes from one row of out to the next */
};

/* A flush whose command buffer did not reach the GPU: the textures and meshes it was to fill go up
   with the next one. */
static void requeue_uploads(long instances)
{
    size_t i;
    for (i = 0; i < G.nuploads; i++) {
        const struct gpu_upload *u = &G.uploads[i];
        struct gfx_surface *s = u->surface;
        if (!s || is_target(s))
            continue;
        if (u->w && u->h) {
            RECT r;
            r.left = (LONG)u->x;
            r.top = (LONG)u->y;
            r.right = (LONG)(u->x + u->w);
            r.bottom = (LONG)(u->y + u->h);
            mark_dirty(s, &r);
        }
        if (u->mips) {
            lock_handles();
            s->mips = 1;
            unlock_handles();
        }
    }
    for (i = 0; i < G.nmeshUploads; i++) {
        struct gfx_mesh *m = G.meshUploads[i].mesh;
        SDL_ReleaseGPUBuffer(G.dev, m->buffer);
        m->buffer = NULL;
    }
    if (instances >= 0) {
        SDL_ReleaseGPUBuffer(G.dev, G.instances);
        G.instances = NULL;
        G.instanceCount = 0;
    }
    /* the copies from targets into textures were not made: the CPU's pixels are the textures' */
    for (i = 0; i < G.nsteps; i++)
        if (G.steps[i].kind == STEP_COPY && G.steps[i].dstSurface)
            G.steps[i].dstSurface->gpuNewer = 0;
}

static uint32_t align16(uint32_t v)
{
    return (v + 15u) & ~15u;
}

/* Submits everything recorded since the last flush in one command buffer: one transfer buffer,
   one copy pass, then the steps. */
static int flush(const struct flush_extra *extra)
{
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    SDL_GPUFence *fence;
    uint32_t oRecords, oViews, oShadows, oVertices, oIndices, oTexels, total;
    long instances;
    unsigned char *mapped;
    size_t i;
    int ok = 1, downloaded = 0;
    batch_flush();
    if (!G.dev)
        return 0;
    if (!G.nsteps && !G.nuploads && !G.nmeshUploads && !extra->present && !extra->download) {
        frame_reset();
        return 1;
    }
    /* every storage buffer holds one record at least */
    if (!G.drawRecords.size)
        bytes_add(&G.drawRecords, sizeof(struct GpuDraw), 16);
    if (!G.viewRecords.size)
        bytes_add(&G.viewRecords, sizeof(struct GpuView), 16);
    if (!G.shadowRecords.size)
        bytes_add(&G.shadowRecords, sizeof(struct GpuShadow), 16);
    if (!G.indexData.size)
        bytes_add(&G.indexData, 4, 4);
    if (!G.vertexData.size)
        bytes_add(&G.vertexData, 64, 16);
    instances = instances_needed((uint32_t)(G.drawRecords.size / sizeof(struct GpuDraw)));
    oRecords = 0;
    oViews = align16(oRecords + (uint32_t)G.drawRecords.size);
    oShadows = align16(oViews + (uint32_t)G.viewRecords.size);
    oVertices = align16(oShadows + (uint32_t)G.shadowRecords.size);
    oIndices = align16(oVertices + (uint32_t)G.vertexData.size);
    oTexels = align16(oIndices + (uint32_t)G.indexData.size);
    total = oTexels + (uint32_t)G.texels.size;
    if (instances == -2
        || !ensure_buffer(&G.records, &G.recordsSize, (uint32_t)G.drawRecords.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.views, &G.viewsSize, (uint32_t)G.viewRecords.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.shadows, &G.shadowsSize, (uint32_t)G.shadowRecords.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.vertices, &G.verticesSize, (uint32_t)G.vertexData.size, SDL_GPU_BUFFERUSAGE_VERTEX)
        || !ensure_buffer(&G.indices, &G.indicesSize, (uint32_t)G.indexData.size, SDL_GPU_BUFFERUSAGE_INDEX)) {
        SDL_Log("gfx: frame buffers: %s", SDL_GetError());
        goto failed;
    }
    if (!G.upload || total > G.uploadSize) {
        SDL_GPUTransferBufferCreateInfo info;
        uint32_t size = G.uploadSize ? G.uploadSize : 1u << 20;
        while (size < total)
            size *= 2;
        if (G.upload)
            SDL_ReleaseGPUTransferBuffer(G.dev, G.upload);
        SDL_zero(info);
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = size;
        G.upload = SDL_CreateGPUTransferBuffer(G.dev, &info);
        G.uploadSize = G.upload ? size : 0;
        if (!G.upload) {
            SDL_Log("gfx: transfer buffer of %u bytes: %s", size, SDL_GetError());
            goto failed;
        }
    }
    mapped = SDL_MapGPUTransferBuffer(G.dev, G.upload, true);
    if (!mapped)
        goto failed;
    memcpy(mapped + oRecords, G.drawRecords.data, G.drawRecords.size);
    memcpy(mapped + oViews, G.viewRecords.data, G.viewRecords.size);
    memcpy(mapped + oShadows, G.shadowRecords.data, G.shadowRecords.size);
    memcpy(mapped + oVertices, G.vertexData.data, G.vertexData.size);
    memcpy(mapped + oIndices, G.indexData.data, G.indexData.size);
    if (G.texels.size)
        memcpy(mapped + oTexels, G.texels.data, G.texels.size);
    SDL_UnmapGPUTransferBuffer(G.dev, G.upload);

    cmd = SDL_AcquireGPUCommandBuffer(G.dev);
    if (!cmd) {
        SDL_Log("gfx: SDL_AcquireGPUCommandBuffer: %s", SDL_GetError());
        goto failed;
    }
    copy = SDL_BeginGPUCopyPass(cmd);
    copy_into(copy, oRecords, G.records, (uint32_t)G.drawRecords.size);
    copy_into(copy, oViews, G.views, (uint32_t)G.viewRecords.size);
    copy_into(copy, oShadows, G.shadows, (uint32_t)G.shadowRecords.size);
    copy_into(copy, oVertices, G.vertices, (uint32_t)G.vertexData.size);
    copy_into(copy, oIndices, G.indices, (uint32_t)G.indexData.size);
    if (instances >= 0) {
        SDL_GPUTransferBufferLocation src;
        SDL_GPUBufferRegion dst;
        src.transfer_buffer = G.upload;
        src.offset = oTexels + (uint32_t)instances;
        dst.buffer = G.instances;
        dst.offset = 0;
        dst.size = G.instanceCount * 4;
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    }
    for (i = 0; i < G.nmeshUploads; i++) {
        SDL_GPUTransferBufferLocation src;
        SDL_GPUBufferRegion dst;
        src.transfer_buffer = G.upload;
        src.offset = oTexels + G.meshUploads[i].offset;
        dst.buffer = G.meshUploads[i].buffer;
        dst.offset = 0;
        dst.size = G.meshUploads[i].size;
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    }
    if (run_uploads(copy, 0, G.frameUploads, oTexels)) {
        SDL_EndGPUCopyPass(copy);
        make_mipmaps(cmd, 0, G.frameUploads);
    } else {
        SDL_EndGPUCopyPass(copy);
    }
    for (i = 0; i < G.nsteps; i++) {
        const struct gpu_step *s = &G.steps[i];
        if (s->kind == STEP_PASS) {
            run_pass(cmd, s);
        } else if (s->kind == STEP_UPLOADS) {
            SDL_GPUCopyPass *late = SDL_BeginGPUCopyPass(cmd);
            int mips = run_uploads(late, s->first, s->count, oTexels);
            SDL_EndGPUCopyPass(late);
            if (mips)
                make_mipmaps(cmd, s->first, s->count);
        } else if (s->kind == STEP_COPY) {
            run_copy(cmd, s);
        }
    }
    if (extra->download) {
        SDL_GPUTextureRegion src;
        SDL_GPUTextureTransferInfo dst;
        uint32_t need = (uint32_t)extra->w * (uint32_t)extra->h * 4;
        if (!G.download || need > G.downloadSize) {
            SDL_GPUTransferBufferCreateInfo info;
            if (G.download)
                SDL_ReleaseGPUTransferBuffer(G.dev, G.download);
            SDL_zero(info);
            info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
            info.size = need;
            G.download = SDL_CreateGPUTransferBuffer(G.dev, &info);
            G.downloadSize = G.download ? need : 0;
        }
        if (G.download) {
            copy = SDL_BeginGPUCopyPass(cmd);
            SDL_zero(src);
            SDL_zero(dst);
            src.texture = extra->download;
            src.x = (Uint32)extra->x;
            src.y = (Uint32)extra->y;
            src.w = (Uint32)extra->w;
            src.h = (Uint32)extra->h;
            src.d = 1;
            dst.transfer_buffer = G.download;
            dst.pixels_per_row = (Uint32)extra->w;
            dst.rows_per_layer = (Uint32)extra->h;
            SDL_DownloadFromGPUTexture(copy, &src, &dst);
            SDL_EndGPUCopyPass(copy);
            downloaded = 1;
        }
    }
    if (extra->present)
        present_into(cmd, extra->wait);
    if (!extra->download)
        wait_frames(0);
    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    if (!fence) {
        SDL_Log("gfx: SDL_SubmitGPUCommandBufferAndAcquireFence: %s", SDL_GetError());
        goto failed;
    }
    if (extra->download) {
        const unsigned char *p = NULL;
        int y;
        if (downloaded && SDL_WaitForGPUFences(G.dev, true, &fence, 1))
            p = SDL_MapGPUTransferBuffer(G.dev, G.download, false);
        if (p) {
            for (y = 0; y < extra->h; y++)
                memcpy(extra->out + (size_t)y * extra->pitch, p + (size_t)y * extra->w * 4, (size_t)extra->w * 4);
            SDL_UnmapGPUTransferBuffer(G.dev, G.download);
        } else {
            ok = 0;
        }
        SDL_ReleaseGPUFence(G.dev, fence);
        /* the queue finished everything submitted before it */
        wait_frames(1);
    } else {
        G.inFlight[G.inFlightNext] = fence;
        G.inFlightNext = (G.inFlightNext + 1) % FRAMES_IN_FLIGHT;
    }
    frame_reset();
    collect_dead_textures();
    return ok;

failed:
    requeue_uploads(instances);
    frame_reset();
    return 0;
}

static int flush_only(void)
{
    struct flush_extra extra;
    SDL_zero(extra);
    return flush(&extra);
}

/* Downloads a rectangle of a target's or a texture's GPU picture, the whole of it without one,
   into B8G8R8A8 rows `pitch` bytes apart. */
static int download(struct gfx_surface *s, const RECT *r, unsigned char *out, size_t pitch)
{
    struct flush_extra extra;
    struct gfx_surface *g = gpu_of(s);
    SDL_zero(extra);
    if (!g->tex)
        return 0;
    extra.download = g->tex;
    if (r) {
        extra.x = r->left;
        extra.y = r->top;
        extra.w = r->right - r->left;
        extra.h = r->bottom - r->top;
    } else {
        extra.w = g->width;
        extra.h = g->height;
    }
    extra.out = out;
    extra.pitch = pitch;
    return flush(&extra);
}

/* ---- the device ---- */

static int choose_vsync(void)
{
    const char *env = SDL_getenv("RAGEXE_VSYNC");
    return !(env && env[0] == '0' && !env[1]);
}

static SDL_GPUShader *make_shader(const struct GpuShaderCode *code)
{
    SDL_GPUShaderCreateInfo info;
    SDL_zero(info);
#ifdef __APPLE__
    info.code = code->msl;
    info.code_size = code->mslSize;
    info.entrypoint = "main0";
    info.format = SDL_GPU_SHADERFORMAT_MSL;
#else
    info.code = code->spirv;
    info.code_size = code->spirvSize;
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
#endif
    info.stage = code->stage == GPU_SHADER_VERTEX ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT;
    info.num_samplers = code->samplers;
    info.num_storage_textures = code->storageTextures;
    info.num_storage_buffers = code->storageBuffers;
    info.num_uniform_buffers = code->uniformBuffers;
    return SDL_CreateGPUShader(G.dev, &info);
}

static void reset_state(void)
{
    int i;
    memset(G.rs, 0, sizeof G.rs);
    memset(G.tss, 0, sizeof G.tss);
    G.rs[GFX_RS_ZENABLE] = 1;
    G.rs[GFX_RS_ZWRITEENABLE] = 1;
    G.rs[GFX_RS_SRCBLEND] = GFX_BLEND_ONE;
    G.rs[GFX_RS_DESTBLEND] = 1;
    G.rs[GFX_RS_CULLMODE] = GFX_CULL_CCW;
    G.rs[GFX_RS_ZFUNC] = GFX_CMP_LESSEQUAL;
    G.rs[GFX_RS_ALPHAFUNC] = 8;
    for (i = 0; i < 8; i++) {
        G.tss[i][GFX_TSS_COLOROP] = i ? GFX_TOP_DISABLE : GFX_TOP_MODULATE;
        G.tss[i][GFX_TSS_COLORARG1] = GFX_TA_TEXTURE;
        G.tss[i][GFX_TSS_COLORARG2] = GFX_TA_CURRENT;
        G.tss[i][GFX_TSS_ALPHAOP] = i ? GFX_TOP_DISABLE : GFX_TOP_SELECTARG1;
        G.tss[i][GFX_TSS_ALPHAARG1] = GFX_TA_TEXTURE;
        G.tss[i][GFX_TSS_ALPHAARG2] = GFX_TA_CURRENT;
        G.tss[i][GFX_TSS_TEXCOORDINDEX] = (DWORD)i;
    }
    for (i = 0; i < 2; i++) {
        G.samp[i][SAMP_ADDRESS] = GFX_ADDRESS_WRAP;
        G.samp[i][SAMP_BORDER] = 0;
        G.samp[i][SAMP_MAG] = 1;
        G.samp[i][SAMP_MIN] = 1;
        G.samp[i][SAMP_MIP] = GFX_MIPFILTER_NONE;
    }
    memset(G.stageTex, 0, sizeof G.stageTex);
}

/* The sample count RAGEXE_MSAA asks for (4 when unset, as Direct3D 9 took): of 2, 4 and 8, the
   most SDL's GPU API offers, the highest at or below it that the GPU takes for the target and depth
   formats, or none. */
static SDL_GPUSampleCount pick_samples(void)
{
    const char *env = SDL_getenv("RAGEXE_MSAA");
    int want = env && env[0] ? SDL_atoi(env) : 4;
    static const SDL_GPUSampleCount counts[] = { SDL_GPU_SAMPLECOUNT_8, SDL_GPU_SAMPLECOUNT_4, SDL_GPU_SAMPLECOUNT_2 };
    static const int numbers[] = { 8, 4, 2 };
    int i;
    for (i = 0; i < 3; i++)
        if (numbers[i] <= want && SDL_GPUTextureSupportsSampleCount(G.dev, TARGET_FORMAT, counts[i])
            && SDL_GPUTextureSupportsSampleCount(G.dev, G.depthFormat, counts[i]))
            return counts[i];
    return SDL_GPU_SAMPLECOUNT_1;
}

/* The white texel a stage without a texture samples, uploaded once. */
static int upload_white(void)
{
    static const unsigned char white[4] = { 0xff, 0xff, 0xff, 0xff };
    SDL_GPUTransferBufferCreateInfo info;
    SDL_GPUTransferBuffer *buffer;
    SDL_GPUTextureTransferInfo src;
    SDL_GPUTextureRegion dst;
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    void *p;
    int ok;
    SDL_zero(info);
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    info.size = sizeof white;
    buffer = SDL_CreateGPUTransferBuffer(G.dev, &info);
    if (!buffer)
        return 0;
    p = SDL_MapGPUTransferBuffer(G.dev, buffer, false);
    cmd = p ? SDL_AcquireGPUCommandBuffer(G.dev) : NULL;
    if (p)
        memcpy(p, white, sizeof white);
    if (p)
        SDL_UnmapGPUTransferBuffer(G.dev, buffer);
    if (cmd) {
        SDL_zero(src);
        SDL_zero(dst);
        src.transfer_buffer = buffer;
        dst.texture = G.white;
        dst.w = dst.h = dst.d = 1;
        copy = SDL_BeginGPUCopyPass(cmd);
        SDL_UploadToGPUTexture(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
    }
    ok = cmd && SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(G.dev, buffer);
    return ok;
}

static int create_device(void)
{
    const char *debug = SDL_getenv("RAGEXE_GPU_DEBUG");
    SDL_GPUSamplerCreateInfo sampler;
    SDL_GPUTextureCreateInfo info;
    SDL_PropertiesID props;
    int i;
    if (G.dev)
        return 1;
    G.vsCode[PROG_FIXED] = &fixed_vs;
    G.fsCode[PROG_FIXED] = &fixed_fs;
    G.vsCode[PROG_MESH] = &mesh_vs;
    G.fsCode[PROG_MESH] = &mesh_fs;
    G.vsCode[PROG_CASTER] = &caster_vs;
    G.fsCode[PROG_CASTER] = &caster_fs;
    props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, debug && debug[0] == '1');
#ifdef __APPLE__
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);
#else
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
#endif
    /* the renderer clips depth, writes no clip distances and draws nothing indirectly: without
       those features Vulkan devices that lack them (older Android ones) run as well */
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN, false);
    G.dev = props ? SDL_CreateGPUDeviceWithProperties(props) : NULL;
    SDL_DestroyProperties(props);
    if (!G.dev) {
        SDL_Log("gfx: SDL_CreateGPUDevice: %s", SDL_GetError());
        return 0;
    }
    G.mainThread = SDL_GetCurrentThreadID();
    for (i = 0; i < PROG_COUNT; i++) {
        G.vs[i] = make_shader(G.vsCode[i]);
        G.fs[i] = make_shader(G.fsCode[i]);
        if (!G.vs[i] || !G.fs[i]) {
            SDL_Log("gfx: SDL_CreateGPUShader: %s", SDL_GetError());
            return 0;
        }
    }
    G.depthFormat = SDL_GPUTextureSupportsFormat(G.dev, SDL_GPU_TEXTUREFORMAT_D24_UNORM, SDL_GPU_TEXTURETYPE_2D,
                                                 SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)
                        ? SDL_GPU_TEXTUREFORMAT_D24_UNORM : SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    SDL_zero(sampler);
    sampler.min_filter = sampler.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler.address_mode_u = sampler.address_mode_v = sampler.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    G.pointClamp = SDL_CreateGPUSampler(G.dev, &sampler);
    G.white = new_texture(1, 1, 1, SDL_GPU_TEXTUREUSAGE_SAMPLER);
    SDL_zero(info);
    info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
    info.format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = info.height = 1;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    G.noShadow = SDL_CreateGPUTexture(G.dev, &info);
    if (!G.pointClamp || !G.white || !G.noShadow || !upload_white()) {
        SDL_Log("gfx: device resources: %s", SDL_GetError());
        return 0;
    }
    G.frame = 1;
    G.cameraView = G.meshView = G.casterView = G.casterCascade = G.shadowRecord = -1;
    if (G.window && SDL_ClaimWindowForGPUDevice(G.dev, G.window)) {
        SDL_GPUPresentMode mode = SDL_GPU_PRESENTMODE_VSYNC;
        G.claimed = 1;
        if (!choose_vsync()) {
            if (SDL_WindowSupportsGPUPresentMode(G.dev, G.window, SDL_GPU_PRESENTMODE_IMMEDIATE))
                mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
            else if (SDL_WindowSupportsGPUPresentMode(G.dev, G.window, SDL_GPU_PRESENTMODE_MAILBOX))
                mode = SDL_GPU_PRESENTMODE_MAILBOX;
        }
        SDL_SetGPUSwapchainParameters(G.dev, G.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode);
    } else if (G.window) {
        /* without a swapchain the frames are drawn and can be captured, not shown */
        SDL_Log("gfx: SDL_ClaimWindowForGPUDevice: %s", SDL_GetError());
    }
    return 1;
}

int gfx_open(struct C3dDevice *d, int flags)
{
    (void)flags;
    G.window = d->hwnd;
    if (!G.window)
        return GFX_E_DDRAW_CREATE;
    if (!G.lock)
        G.lock = SDL_CreateMutex();
    d->dd = G.window;
    return 0;
}

/* The screen's multisampled picture, which it is resolved from, and its depth buffer, at G.samples
   samples a pixel. */
static SDL_GPUTexture *new_samples_texture(int w, int h, int depth)
{
    SDL_GPUTextureCreateInfo info;
    SDL_zero(info);
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = depth ? G.depthFormat : TARGET_FORMAT;
    info.usage = depth ? SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET : SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    info.width = (Uint32)(w + MARGIN);
    info.height = (Uint32)(h + MARGIN);
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.sample_count = G.samples;
    return SDL_CreateGPUTexture(G.dev, &info);
}

int gfx_create_screen(struct C3dDevice *d, int flags)
{
    struct gfx_surface *screen, *save;
    if (!G.window || !create_device())
        return GFX_E_PRIMARY;
    reset_state();
    batch_init(client_draw);
    screen = new_handle(K_SCREEN, d->width, d->height, GFX_FMT_8888);
    if (!screen)
        return GFX_E_PRIMARY;
    G.screen = screen;
    G.samples = SDL_GPU_SAMPLECOUNT_1;
    screen->tex = new_target_texture(d->width, d->height);
    if (!screen->tex)
        return GFX_E_PRIMARY;
    d->primary = screen;
    if (flags & 2) {
        d->back = new_handle(K_TARGET, d->width, d->height, GFX_FMT_8888);
        if (!d->back)
            return GFX_E_PRIMARY;
    }
    d->target = (flags & 2) ? d->back : d->primary;
    d->target->refs++;
    G.mainTarget = d->target;
    G.target = d->target;
    d->bpp = 32;
    d->rMask = 0xff0000;
    d->gMask = 0xff00;
    d->bMask = 0xff;
    d->aMask = 0;
    save = new_handle(K_SAVE, d->width, d->height, GFX_FMT_8888);
    if (!save)
        return GFX_E_PRIMARY;
    save->tex = new_target_texture(d->width, d->height);
    if (!save->tex)
        return GFX_E_PRIMARY;
    d->save = save;
    d->dev = G.dev;
    return 0;
}

int gfx_create_3d(struct C3dDevice *d, int flags)
{
    (void)flags;
    if (!G.dev)
        return GFX_E_DEVICE_CREATE;
    d->d3d = G.dev;
    d->zbufferCaps = 0;
    memset(d->zformat, 0, sizeof(d->zformat));
    return 0;
}

int gfx_create_zbuffer(struct C3dDevice *d)
{
    struct gfx_surface *z;
    if (!G.screen)
        return GFX_E_ZBUFFER;
    G.samples = pick_samples();
    if (G.samples != SDL_GPU_SAMPLECOUNT_1) {
        G.msaa = new_samples_texture(d->width, d->height, 0);
        if (!G.msaa)
            G.samples = SDL_GPU_SAMPLECOUNT_1;
    }
    G.depth = new_samples_texture(d->width, d->height, 1);
    if (!G.depth)
        return GFX_E_ZBUFFER;
    z = new_handle(K_ZBUFFER, d->width, d->height, GFX_FMT_8888);
    if (!z)
        return GFX_E_ZBUFFER;
    d->zbuffer = z;
    return 0;
}

int gfx_get_caps(struct C3dDevice *d, struct gfx_caps *out)
{
    (void)d;
    if (!G.dev)
        return -1;
    memset(out, 0, sizeof *out);
    out->zbufferless = 0;
    out->vertexFog = 1;
    out->trilinear = 1;
    out->minTexW = 1;
    out->minTexH = 1;
    /* every GPU of the floor (plans/gpu-renderer.md) takes 8192 at least */
    out->maxTexW = 8192;
    out->maxTexH = 8192;
    out->texAspect = 0;
    out->guardLeft = -32768.0f;
    out->guardTop = -32768.0f;
    out->guardRight = 32767.0f;
    out->guardBottom = 32767.0f;
    out->bltStretch = 1;
    return 0;
}

void gfx_default_states(struct C3dDevice *d)
{
    static const struct { int state; DWORD value; } render_states[] = {
        { GFX_RS_CULLMODE, GFX_CULL_NONE },
        { GFX_RS_ZFUNC, GFX_CMP_LESSEQUAL },
        { GFX_RS_ALPHAFUNC, GFX_CMP_GREATEREQUAL },
        { GFX_RS_ALPHAREF, 0xf0 },
        { GFX_RS_ZENABLE, TRUE },
        { GFX_RS_ZWRITEENABLE, TRUE },
        { GFX_RS_SRCBLEND, GFX_BLEND_SRCALPHA },
        { GFX_RS_DESTBLEND, GFX_BLEND_INVSRCALPHA },
    };
    static const struct { int stage, state; DWORD value; } stage_states[] = {
        { 0, GFX_TSS_TEXCOORDINDEX, 0 },
        { 0, GFX_TSS_COLOROP, GFX_TOP_MODULATE },
        { 0, GFX_TSS_COLORARG1, GFX_TA_TEXTURE },
        { 0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE },
        { 0, GFX_TSS_ALPHAOP, GFX_TOP_MODULATE },
        { 0, GFX_TSS_ALPHAARG1, GFX_TA_TEXTURE },
        { 0, GFX_TSS_ALPHAARG2, GFX_TA_DIFFUSE },
        { 1, GFX_TSS_TEXCOORDINDEX, 1 },
        { 1, GFX_TSS_COLORARG1, GFX_TA_TEXTURE },
        { 1, GFX_TSS_COLOROP, GFX_TOP_MODULATEALPHA_ADDCOLOR },
        { 1, GFX_TSS_COLORARG2, GFX_TA_CURRENT },
        { 1, GFX_TSS_ALPHAOP, GFX_TOP_DISABLE },
        { 2, GFX_TSS_COLOROP, GFX_TOP_DISABLE },
        { 2, GFX_TSS_ALPHAOP, GFX_TOP_DISABLE },
        { 0, GFX_TSS_MINFILTER, GFX_FILTER_LINEAR },
        { 0, GFX_TSS_MAGFILTER, GFX_FILTER_LINEAR },
        { 1, GFX_TSS_MINFILTER, GFX_FILTER_LINEAR },
        { 1, GFX_TSS_MAGFILTER, GFX_FILTER_LINEAR },
        { 0, GFX_TSS_ADDRESS, GFX_ADDRESS_CLAMP },
        { 1, GFX_TSS_ADDRESS, GFX_ADDRESS_BORDER },
        { 1, GFX_TSS_BORDERCOLOR, 0xff000000 },
    };
    size_t i;
    (void)d;
    for (i = 0; i < sizeof render_states / sizeof render_states[0]; i++)
        gfx_set_render_state(render_states[i].state, render_states[i].value);
    for (i = 0; i < sizeof stage_states / sizeof stage_states[0]; i++)
        gfx_set_stage_state(stage_states[i].stage, stage_states[i].state, stage_states[i].value);
}

int gfx_close(struct C3dDevice *d)
{
    struct gfx_surface *s, *next;
    int i;
    if (G.dev)
        flush_only();
    batch_drop();
    if (d->target)  { gfx_release(d->target);  d->target = NULL; }
    if (d->back)    { gfx_release(d->back);    d->back = NULL; }
    if (d->primary) { gfx_release(d->primary); d->primary = NULL; }
    if (d->zbuffer) { gfx_release(d->zbuffer); d->zbuffer = NULL; }
    if (d->save)    { gfx_release(d->save);    d->save = NULL; }
    G.mainTarget = G.target = G.screen = NULL;
    if (G.dev) {
        wait_frames(1);
        SDL_WaitForGPUIdle(G.dev);
        lock_handles();
        for (s = G.handles; s; s = next) {
            next = s->next;
            if (s->kind != K_TARGET && s->tex)
                SDL_ReleaseGPUTexture(G.dev, s->tex);
            s->tex = NULL;
            s->dirty = 1;
            whole_rect(&s->dirtyRect, s);
        }
        unlock_handles();
        collect_dead_textures();
        {
            /* the client keeps its meshes: they fill new buffers on their next draw */
            struct gfx_mesh *m;
            for (m = G.meshes; m; m = m->next) {
                if (m->buffer)
                    SDL_ReleaseGPUBuffer(G.dev, m->buffer);
                m->buffer = NULL;
            }
        }
        for (i = 0; i < PIPELINE_SLOTS; i++)
            if (G.pipelines[i].pipeline)
                SDL_ReleaseGPUGraphicsPipeline(G.dev, G.pipelines[i].pipeline);
        for (i = 0; i < SAMPLER_SLOTS; i++)
            if (G.samplers[i].sampler)
                SDL_ReleaseGPUSampler(G.dev, G.samplers[i].sampler);
        for (i = 0; i < PROG_COUNT; i++) {
            if (G.vs[i])
                SDL_ReleaseGPUShader(G.dev, G.vs[i]);
            if (G.fs[i])
                SDL_ReleaseGPUShader(G.dev, G.fs[i]);
        }
        if (G.pointClamp) SDL_ReleaseGPUSampler(G.dev, G.pointClamp);
        if (G.white) SDL_ReleaseGPUTexture(G.dev, G.white);
        if (G.noShadow) SDL_ReleaseGPUTexture(G.dev, G.noShadow);
        if (G.shadowMaps) SDL_ReleaseGPUTexture(G.dev, G.shadowMaps);
        if (G.shadowDepth) SDL_ReleaseGPUTexture(G.dev, G.shadowDepth);
        if (G.depth) SDL_ReleaseGPUTexture(G.dev, G.depth);
        if (G.msaa) SDL_ReleaseGPUTexture(G.dev, G.msaa);
        if (G.records) SDL_ReleaseGPUBuffer(G.dev, G.records);
        if (G.views) SDL_ReleaseGPUBuffer(G.dev, G.views);
        if (G.shadows) SDL_ReleaseGPUBuffer(G.dev, G.shadows);
        if (G.vertices) SDL_ReleaseGPUBuffer(G.dev, G.vertices);
        if (G.indices) SDL_ReleaseGPUBuffer(G.dev, G.indices);
        if (G.instances) SDL_ReleaseGPUBuffer(G.dev, G.instances);
        if (G.upload) SDL_ReleaseGPUTransferBuffer(G.dev, G.upload);
        if (G.download) SDL_ReleaseGPUTransferBuffer(G.dev, G.download);
        if (G.claimed)
            SDL_ReleaseWindowFromGPUDevice(G.dev, G.window);
        SDL_DestroyGPUDevice(G.dev);
    }
    G.dev = NULL;
    G.claimed = 0;
    memset(G.pipelines, 0, sizeof G.pipelines);
    memset(G.samplers, 0, sizeof G.samplers);
    memset(G.vs, 0, sizeof G.vs);
    memset(G.fs, 0, sizeof G.fs);
    G.pointClamp = NULL;
    G.white = G.noShadow = G.shadowMaps = G.shadowDepth = G.depth = G.msaa = NULL;
    G.records = G.views = G.shadows = G.vertices = G.indices = G.instances = NULL;
    G.recordsSize = G.viewsSize = G.shadowsSize = G.verticesSize = G.indicesSize = G.instanceCount = 0;
    G.upload = G.download = NULL;
    G.uploadSize = G.downloadSize = 0;
    G.shadowState = 0;
    d->dev = NULL;
    d->d3d = NULL;
    d->dd = NULL;
    return 0;
}

/* The present mode follows RAGEXE_VSYNC (create_device). Without vsync the frame never waits for the
   window: it is drawn but not shown unless a refresh has passed since the last picture
   (present_into). */
int gfx_present(struct C3dDevice *d, int vsync)
{
    struct flush_extra extra;
    (void)d;
    if (!G.dev || !G.screen)
        return -1;
    SDL_zero(extra);
    extra.present = 1;
    extra.wait = vsync;
    flush(&extra);
    return 0;
}

/* custom: the screen, its multisampled picture and depth, and its copy at w by h, for a window
   resized to that; the swapchain follows the window by itself. Without memory for them it answers
   -1 and keeps the old ones. */
int gfx_resize(struct C3dDevice *d, int w, int h)
{
    struct gfx_surface *s;
    SDL_GPUTexture *screen, *msaa = NULL, *depth = NULL, *save = NULL;
    if (!G.dev || !G.screen || w <= 0 || h <= 0)
        return -1;
    flush_only();
    screen = new_target_texture(w, h);
    if (G.msaa)
        msaa = new_samples_texture(w, h, 0);
    if (G.depth)
        depth = new_samples_texture(w, h, 1);
    if (d->save)
        save = new_target_texture(w, h);
    if (!screen || (G.msaa && !msaa) || (G.depth && !depth) || (d->save && !save)) {
        SDL_GPUTexture *made[4];
        int i;
        made[0] = screen;
        made[1] = msaa;
        made[2] = depth;
        made[3] = save;
        for (i = 0; i < 4; i++)
            if (made[i])
                SDL_ReleaseGPUTexture(G.dev, made[i]);
        return -1;
    }
    SDL_ReleaseGPUTexture(G.dev, G.screen->tex);
    G.screen->tex = screen;
    if (G.msaa) {
        SDL_ReleaseGPUTexture(G.dev, G.msaa);
        G.msaa = msaa;
    }
    if (G.depth) {
        SDL_ReleaseGPUTexture(G.dev, G.depth);
        G.depth = depth;
    }
    if (d->save) {
        SDL_ReleaseGPUTexture(G.dev, d->save->tex);
        d->save->tex = save;
    }
    G.screen->version = next_version();
    lock_handles();
    for (s = G.handles; s; s = s->next)
        if (s->kind == K_SCREEN || s->kind == K_TARGET || s->kind == K_SAVE || s->kind == K_ZBUFFER) {
            s->width = w;
            s->height = h;
            /* the lock buffers are made again at the new size */
            free(s->pixels);
            free(s->lockCopy);
            s->pixels = s->lockCopy = NULL;
        }
    unlock_handles();
    G.viewsTargetW = G.viewsTargetH = 0;
    return 0;
}

void gfx_set_mipmaps(int on)
{
    G.mipmaps = on;
}

/* ---- surfaces of the API ---- */

gfx_surface *gfx_create_surface(int w, int h)
{
    struct gfx_surface *s;
    g_perf.surfCreated++;
    if (w <= 0 || h <= 0)
        return NULL;
    s = new_handle(K_SYSMEM, w, h, GFX_FMT_8888);
    if (!s)
        return NULL;
    s->pixels = calloc((size_t)w * h, 4);
    if (!s->pixels) {
        gfx_release(s);
        return NULL;
    }
    s->key = CLIENT_SURFACE_KEY;
    mark_dirty(s, NULL);
    return s;
}

int gfx_texture_format_ok(int fmt)
{
    return fmt == GFX_FMT_1555 || fmt == GFX_FMT_4444 || fmt == GFX_FMT_565 || fmt == GFX_FMT_8888;
}

static int mip_levels(int w, int h)
{
    int levels = 1, most = 1, n;
    if (w > 2) {
        for (;;) {
            if ((1 << levels) >= h)
                break;
            levels++;
            n = 1 << levels;
            if (n >= w)
                break;
        }
    }
    for (n = w > h ? w : h; n > 1; n >>= 1)
        most++;
    return levels < most ? levels : most;
}

/* May run on the model loader's thread: it touches no GPU object. */
gfx_surface *gfx_create_texture(int w, int h, int fmt)
{
    struct gfx_surface *s;
    perf_count_shared(&g_perf.texCreated, 1);
    if (!gfx_texture_format_ok(fmt) || w <= 0 || h <= 0)
        return NULL;
    s = new_handle(K_TEXTURE, w, h, fmt);
    if (!s)
        return NULL;
    s->levels = G.mipmaps ? mip_levels(w, h) : 1;
    s->pixels = calloc((size_t)w * h, (size_t)texel_bytes(fmt));
    if (!s->pixels) {
        gfx_release(s);
        return NULL;
    }
    mark_dirty(s, NULL);
    return s;
}

gfx_surface *gfx_create_render_target(int w, int h)
{
    struct gfx_surface *s;
    if (!G.dev || w <= 0 || h <= 0)
        return NULL;
    s = new_handle(K_RTTEX, w, h, GFX_FMT_8888);
    if (!s)
        return NULL;
    s->tex = new_target_texture(w, h);
    if (!s->tex) {
        gfx_release(s);
        return NULL;
    }
    return s;
}

int gfx_get_size(gfx_surface *s, int *w, int *h)
{
    *w = s->width;
    *h = s->height;
    return 0;
}

/* The CPU's pixels of a surface whose GPU texture took a copy from a target since. */
static int cpu_current(struct gfx_surface *s)
{
    unsigned char *bgra;
    int ok;
    if (!s->gpuNewer)
        return 1;
    bgra = malloc((size_t)s->width * s->height * 4);
    if (!bgra)
        return 0;
    ok = download(s, NULL, bgra, (size_t)s->width * 4);
    if (ok) {
        int bytes = texel_bytes(s->fmt);
        texel_rows_from_bgra(s->fmt, bgra, (long)s->width * 4, s->width, s->height, s->pixels,
                             (long)s->width * bytes);
        s->gpuNewer = 0;
    }
    free(bgra);
    return ok;
}

/* A rectangle of a surface, the whole of it without one; a target's is downloaded, and only that.
   A rectangle that is empty or reaches outside the surface locks nothing. */
int gfx_lock(gfx_surface *s, const RECT *r, int flags, struct gfx_lock *out)
{
    RECT rect;
    int bytes;
    if (flags & GFX_LOCK_UNSIZED)
        return -1;
    perf_count_shared(&g_perf.locks, 1);
    whole_rect(&rect, s);
    if (r) {
        if (r->left < 0 || r->top < 0 || r->right > s->width || r->bottom > s->height || r->left >= r->right
            || r->top >= r->bottom)
            return -1;
        rect = *r;
    }
    bytes = is_target(s) ? 4 : texel_bytes(s->fmt);
    if (is_target(s)) {
        size_t size = (size_t)s->width * s->height * 4, pitch = (size_t)s->width * 4;
        size_t at = (size_t)rect.top * pitch + (size_t)rect.left * 4, row = (size_t)(rect.right - rect.left) * 4;
        int y;
        if (!on_main_thread())
            return -1;
        g_perf.targetLocks++;
        if (!s->pixels)
            s->pixels = malloc(size);
        if (!s->lockCopy)
            s->lockCopy = malloc(size);
        if (!s->pixels || !s->lockCopy || !download(s, &rect, s->pixels + at, pitch))
            return -1;
        for (y = 0; y < rect.bottom - rect.top; y++)
            memcpy(s->lockCopy + at + (size_t)y * pitch, s->pixels + at + (size_t)y * pitch, row);
    } else {
        if (!s->pixels || (s->gpuNewer && (!on_main_thread() || !cpu_current(s))))
            return -1;
    }
    s->lockRect = rect;
    out->bits = s->pixels + (size_t)rect.top * s->width * bytes + (size_t)rect.left * bytes;
    out->pitch = s->width * bytes;
    out->width = s->width;
    out->height = s->height;
    out->bpp = bytes * 8;
    return 0;
}

/* The multisampled screen takes the resolved picture after the CPU or a copy changed it. */
static void unresolve(void)
{
    struct gfx_surface *prev = G.target;
    RECT all;
    if (!G.screen || !G.msaa)
        return;
    whole_rect(&all, G.screen);
    G.unresolving = 1;
    G.target = G.screen;
    draw_textured_quad(G.screen, &all, G.screen, &all);
    G.target = prev;
    G.unresolving = 0;
}

/* The pixels a lock of a target changed go back to it where the frame has got to, drawn back into
   the multisampled screen as well: the locked rectangle's rows from the first that changed to the
   last. */
static void target_write_back(struct gfx_surface *s)
{
    struct gfx_surface *g = gpu_of(s);
    const RECT *r = &s->lockRect;
    size_t pitch = (size_t)s->width * 4, row = (size_t)(r->right - r->left) * 4, left = (size_t)r->left * 4, i;
    int y0 = -1, y1 = -1, y;
    long at;
    if (!s->pixels || !s->lockCopy || !g->tex)
        return;
    for (y = r->top; y < r->bottom; y++)
        if (memcmp(s->pixels + (size_t)y * pitch + left, s->lockCopy + (size_t)y * pitch + left, row)) {
            if (y0 < 0)
                y0 = y;
            y1 = y + 1;
        }
    if (y0 < 0)
        return;
    at = bytes_add(&G.texels, (size_t)(y1 - y0) * row, 16);
    if (at < 0)
        return;
    for (y = y0; y < y1; y++)
        memcpy(G.texels.data + at + (size_t)(y - y0) * row, s->pixels + (size_t)y * pitch + left, row);
    if (is_main_target(s) || s->kind == K_SAVE)
        for (i = 0; i < (size_t)(y1 - y0) * row / 4; i++)
            G.texels.data[at + i * 4 + 3] = 0xff;
    pin(g);
    add_upload(g, g->tex, (uint32_t)r->left, (uint32_t)y0, (uint32_t)(r->right - r->left), (uint32_t)(y1 - y0),
               (uint32_t)at, 0);
    if (g == G.screen)
        unresolve();
}

int gfx_unlock(gfx_surface *s)
{
    s->version = next_version();
    if (is_target(s)) {
        if (on_main_thread())
            target_write_back(s);
    } else {
        mark_dirty(s, &s->lockRect);
    }
    return 0;
}

static DWORD read_pixel(const unsigned char *p, int bytes)
{
    DWORD v = 0;
    memcpy(&v, p, (size_t)bytes);
    return v;
}

static void write_pixel(unsigned char *p, int bytes, DWORD v)
{
    memcpy(p, &v, (size_t)bytes);
}

/* A blit's rectangles, the whole surface where none is given; 0 when one is empty or reaches outside
   its surface. */
static int blit_rects(const struct gfx_surface *dst, const RECT *dr, const struct gfx_surface *src, const RECT *sr,
                      RECT *d, RECT *s)
{
    if (sr) *s = *sr; else whole_rect(s, src);
    if (dr) *d = *dr; else whole_rect(d, dst);
    return d->left >= 0 && d->top >= 0 && d->right <= dst->width && d->bottom <= dst->height && d->left < d->right
        && d->top < d->bottom && s->left >= 0 && s->top >= 0 && s->right <= src->width && s->bottom <= src->height
        && s->left < s->right && s->top < s->bottom;
}

/* Between CPU pixels, stretched by picking the nearest texel, as Direct3D 9 copied between surfaces
   in memory. */
static int cpu_blit(struct gfx_surface *dst, const RECT *dp, struct gfx_surface *src, const RECT *sp)
{
    RECT d = *dp, s = *sp;
    int sbytes = src->kind == K_SYSMEM ? 4 : texel_bytes(src->fmt), dbytes = dst->kind == K_SYSMEM ? 4 : texel_bytes(dst->fmt);
    int same = src->fmt == dst->fmt;
    int x, y, dw = d.right - d.left, dh = d.bottom - d.top, sw = s.right - s.left, sh = s.bottom - s.top;
    if (!dst->pixels || !src->pixels)
        return 0;
    g_perf.cpuBlits++;
    for (y = 0; y < dh; y++) {
        const unsigned char *srow = src->pixels + (size_t)(s.top + (dh == sh ? y : y * sh / dh)) * src->width * sbytes;
        unsigned char *drow = dst->pixels + (size_t)(d.top + y) * dst->width * dbytes;
        for (x = 0; x < dw; x++) {
            int sx = s.left + (dw == sw ? x : x * sw / dw);
            DWORD p = read_pixel(srow + (size_t)sx * sbytes, sbytes);
            if (!same)
                p = texel_pack(dst->fmt, texel_unpack(src->fmt, p));
            write_pixel(drow + (size_t)(d.left + x) * dbytes, dbytes, p);
        }
    }
    mark_dirty(dst, &d);
    return 0;
}

/* Rectangles that are empty or reach outside their surfaces copy nothing, as Direct3D 9 copied
   nothing between surfaces in memory for them. */
int gfx_blit(gfx_surface *dst, const RECT *dr, gfx_surface *src, const RECT *sr, int flags)
{
    RECT d, s;
    (void)flags;
    batch_flush();
    g_perf.blits++;
    dst->version = next_version();
    if (dst == src || (is_target(dst) && is_target(src) && gpu_of(dst) == gpu_of(src)))
        return 0;
    if (!blit_rects(dst, dr, src, sr, &d, &s))
        return 0;
    if (is_target(src)) {
        /* from a target: a copy on the GPU, into a target or into a surface's texture, whose
           CPU pixels follow when they are locked */
        struct gfx_surface *g = gpu_of(src);
        struct gpu_step *step;
        SDL_GPUTexture *into;
        int stretch = d.right - d.left != s.right - s.left || d.bottom - d.top != s.bottom - s.top;
        if (!G.dev || !g->tex)
            return -1;
        if (is_target(dst)) {
            into = gpu_of(dst)->tex;
            pin(gpu_of(dst));
        } else {
            if (!cpu_current(dst))
                return -1;
            if (stretch && !dst->blitDest) {
                /* a stretched copy draws: the texture is made again as a colour target, and its
                   pixels go up again from the CPU's */
                dst->blitDest = 1;
                retire_texture(dst->tex);
                dst->tex = NULL;
            }
            if (!ensure_texture(dst))
                return -1;
            into = texture_for_draw(dst);
        }
        if (!into)
            return -1;
        pin(g);
        step = add_step(STEP_COPY);
        if (!step)
            return -1;
        step->src = g->tex;
        step->dst = into;
        step->srcRect = s;
        step->dstRect = d;
        if (!is_target(dst)) {
            step->dstSurface = dst;
            dst->gpuNewer = 1;
        }
        else if (gpu_of(dst) == G.screen)
            unresolve();
        return 0;
    }
    if (is_target(dst)) {
        if (!src->pixels || (src->gpuNewer && !cpu_current(src)))
            return -1;
        draw_textured_quad(dst, &d, src, &s);
        return 0;
    }
    if ((src->gpuNewer && !cpu_current(src)) || (dst->gpuNewer && !cpu_current(dst)))
        return -1;
    return cpu_blit(dst, &d, src, &s);
}

/* The lower levels are made from the top one on the GPU when the texture next goes up. */
int gfx_refresh_mips(gfx_surface *tex, const RECT *r)
{
    (void)r;
    if (!tex || tex->levels <= 1)
        return 0;
    lock_handles();
    tex->mips = 1;
    unlock_handles();
    return 0;
}

int gfx_begin_scene(void)
{
    return G.dev ? 0 : -1;
}

void gfx_end_scene(void)
{
    batch_flush();
}

int gfx_clear(int flags, DWORD colour, float z, const RECT *r)
{
    struct gpu_step *pass;
    struct gfx_surface *t = G.target;
    RECT rc;
    int whole, depth;
    batch_flush();
    if (!G.dev || !t || G.shadowPass)
        return -1;
    pass = current_pass();
    if (!pass)
        return -1;
    if (r) {
        rc = *r;
        if (!clip_rect(&rc, gpu_of(t)))
            return 0;
    } else {
        whole_rect(&rc, gpu_of(t));
    }
    depth = (flags & GFX_CLEAR_ZBUFFER) && pass_has_depth(pass);
    whole = rc.left == 0 && rc.top == 0 && rc.right == gpu_of(t)->width && rc.bottom == gpu_of(t)->height;
    if (!pass->count && whole) {
        if (flags & GFX_CLEAR_TARGET) {
            colour_floats(&pass->clearColour.r, colour);
            /* X8R8G8B8 targets keep their alpha at 1 */
            if (t->kind != K_RTTEX)
                pass->clearColour.a = 1.0f;
            pass->loadColour = SDL_GPU_LOADOP_CLEAR;
        }
        if (depth) {
            pass->clearDepth = z;
            pass->loadDepth = SDL_GPU_LOADOP_CLEAR;
        }
    } else if ((flags & GFX_CLEAR_TARGET) || depth) {
        /* a clear inside a pass: a quad over the rectangle that writes the colour and the depth */
        struct { float x, y, z, rhw; DWORD diffuse, specular; float u, v; } v[4];
        DWORD rs[N_RS], tss0[N_TSS];
        struct gfx_surface *stage0 = G.stageTex[0];
        int i;
        for (i = 0; i < 4; i++) {
            v[i].x = (float)((i & 1) ? rc.right : rc.left) - 0.5f;
            v[i].y = (float)((i & 2) ? rc.bottom : rc.top) - 0.5f;
            v[i].z = z;
            v[i].rhw = 1.0f;
            v[i].diffuse = colour;
            v[i].specular = 0;
            v[i].u = v[i].v = 0.0f;
        }
        memcpy(rs, G.rs, sizeof rs);
        memcpy(tss0, G.tss[0], sizeof tss0);
        G.rs[GFX_RS_ALPHATESTENABLE] = 0;
        G.rs[GFX_RS_ALPHABLENDENABLE] = 0;
        G.rs[GFX_RS_FOGENABLE] = 0;
        G.rs[GFX_RS_CULLMODE] = GFX_CULL_NONE;
        G.rs[GFX_RS_ZENABLE] = depth;
        G.rs[GFX_RS_ZWRITEENABLE] = depth;
        G.rs[GFX_RS_ZFUNC] = 8;
        G.tss[0][GFX_TSS_COLOROP] = GFX_TOP_DISABLE;
        G.stageTex[0] = NULL;
        {
            /* a clear of the depth alone leaves the colour */
            struct pipe_desc p;
            struct gpu_draw *d;
            struct GpuDraw *rec;
            long voff;
            state_pipe(&p, pass, PROG_FIXED, LAYOUT_TEX1, SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP, 1);
            if (!(flags & GFX_CLEAR_TARGET))
                p.mask = MASK_NONE;
            d = add_draw(pass);
            if (d) {
                d->program = PROG_FIXED;
                d->pipeline = pipeline_for(&p);
                d->bind[0].texture = d->bind[1].texture = G.white;
                d->bind[0].sampler = d->bind[1].sampler = G.pointClamp;
                d->record = add_record(&rec);
                voff = bytes_add(&G.vertexData, sizeof v, 8);
                if (rec && voff >= 0 && d->pipeline) {
                    fill_state(rec, pass, KIND_SCREEN, 0);
                    memcpy(G.vertexData.data + voff, v, sizeof v);
                    d->vertexOffset = (uint32_t)voff;
                    d->count = 4;
                } else {
                    pass->count--;
                    G.ndraws--;
                }
            }
        }
        memcpy(G.rs, rs, sizeof rs);
        memcpy(G.tss[0], tss0, sizeof tss0);
        G.stageTex[0] = stage0;
    }
    t->version = next_version();
    return 0;
}

gfx_surface *gfx_get_render_target(void)
{
    if (!G.target)
        return NULL;
    G.target->refs++;
    return G.target;
}

int gfx_set_render_target(gfx_surface *s)
{
    batch_flush();
    if (!G.dev || !s || !is_target(s) || !gpu_of(s)->tex)
        return -1;
    G.target = s;
    return 0;
}

/* The states are counted as diag/perf.h says. */
void gfx_set_render_state(int state, DWORD value)
{
    if ((unsigned)state >= N_RS)
        return;
    perf_count_state(G.rs[state] != value);
    if (G.rs[state] == value)
        return;
    batch_flush();
    G.rs[state] = value;
}

int gfx_get_render_state(int state, DWORD *out)
{
    if ((unsigned)state >= N_RS)
        return -1;
    *out = G.rs[state];
    return 0;
}

static void set_sampler(int stage, int which, DWORD value)
{
    if (stage >= 2)
        return;
    perf_count_state(G.samp[stage][which] != value);
    if (G.samp[stage][which] == value)
        return;
    batch_flush();
    G.samp[stage][which] = value;
}

void gfx_set_stage_state(int stage, int state, DWORD value)
{
    if ((unsigned)stage >= 8)
        return;
    switch (state) {
    case GFX_TSS_ADDRESS:     set_sampler(stage, SAMP_ADDRESS, value); break;
    case GFX_TSS_BORDERCOLOR: set_sampler(stage, SAMP_BORDER, value); break;
    case GFX_TSS_MAGFILTER:   set_sampler(stage, SAMP_MAG, value); break;
    case GFX_TSS_MINFILTER:   set_sampler(stage, SAMP_MIN, value); break;
    case GFX_TSS_MIPFILTER:   set_sampler(stage, SAMP_MIP, value); break;
    default:
        if ((unsigned)state >= N_TSS)
            break;
        perf_count_state(G.tss[stage][state] != value);
        if (G.tss[stage][state] != value) {
            batch_flush();
            G.tss[stage][state] = value;
        }
        break;
    }
}

void gfx_set_texture(int stage, gfx_surface *s)
{
    if ((unsigned)stage >= 8 || G.stageTex[stage] == s)
        return;
    batch_flush();
    g_perf.texSets++;
    G.stageTex[stage] = s;
}

unsigned gfx_content_version(gfx_surface *s)
{
    return s ? s->version : 0;
}

struct gfx_caps g_gfxCaps;
