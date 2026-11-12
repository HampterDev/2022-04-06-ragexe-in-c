/* render/gfx.h on SDL 3's GPU API: Vulkan with SPIR-V shaders on Windows and Linux, Metal with
   MSL on macOS, one shader format handed to SDL_CreateGPUDevice so that SDL has one driver to
   choose. The shaders are HLSL compiled offline (src/render/shaders, tools/shaders).

   Draws are made in modes (render/draw_modes.h): the pipelines of every mode, for every kind of
   target it draws into, are made once, when the screen's samples and depth are known
   (gfx_create_zbuffer), and so are the samplers. A frame is recorded, not drawn: each draw becomes
   a record of what its mode and the draw itself give the shaders (struct GpuDraw, read from a
   storage buffer: a mesh's through an instance-rate attribute that holds its index, a quad's
   through its own record in the frame's quads) in a list of passes, and what it draws and the
   texels of the textures it uses since they changed go into one staging area. The client's quads
   and triangles are instances (render/quads.h): a record each in the frame's quads, which the quad
   shader makes the vertices of, batched into draws while what they are drawn with stays the same;
   the sprite atlas's pages are layers of texture arrays, which draws batch across. When the frame
   is presented, or the screen's picture is read (gfx_read_frame), one command buffer copies all of
   that through one transfer buffer in one copy pass and replays the passes. Textures keep their
   pixels on the CPU in the client's own format, so that the model loader's thread creates, locks
   and fills them without calling SDL; the main thread widens them to B8G8R8A8 when a draw first
   uses them after a change. Render targets live on the GPU, and nothing of them comes back to the
   CPU but the screen's picture when it is read. */
#include "render/gfx.h"
#include "render/device.h"
#include "render/present_fit.h"
#include "render/screen_wobble.h"
#include "render/texel_convert.h"
#include "render/vertex_fog.h"
#include "render/shaders/shader_code.h"
#include "render/shaders/quad.h"
#include "render/shaders/mesh.h"
#include "render/shaders/caster.h"
#include "render/shaders/rows.h"
#include "render/shaders/rounding.h"
#include "diag/perf.h"
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { K_SCREEN, K_SYSMEM, K_TEXTURE, K_RTTEX };

#define CLIENT_SURFACE_KEY 0x00ff00ffu
#define TARGET_FORMAT SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM
/* A target's textures are a pixel wider and taller than the target: every pass's viewport is the
   target moved by half a pixel, (0.5, 0.5, width, height), where Direct3D 9's pixel centres fall on
   the target's, and a viewport must lie within its texture (SDL's GPU API, Metal). */
#define MARGIN 1

/* The records the shaders read (src/render/shaders/records.hlsli). */
struct GpuDraw {
    uint32_t mode[4];
    float target[4];
    uint32_t stages[4];
    float fog[4];
    float fogColour[4];
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

enum { PROG_QUAD, PROG_MESH, PROG_CASTER, PROG_ROWS, PROG_COUNT };
enum { MASK_RGBA, MASK_RGB, MASK_NONE };
/* What a pass draws into, as far as its pipelines care: the screen (multisampled, with depth) and a
   render target's texture (one sample, without depth) (render/draw_modes.h, DRAW_INTO_*). */
enum { CLASS_SCREEN, CLASS_TEXTURE, CLASS_COUNT };
/* A quad's corners, the per-vertex attribute of every quad draw (0 to 3), and its triangles
   (0, 1, 2) and (2, 1, 3), uploaded once (render/shaders/quad.hlsl) */
static const uint32_t quad_corners[4] = { 0, 1, 2, 3 };
static const WORD quad_triangles[6] = { 0, 1, 2, 2, 1, 3 };

struct gfx_surface {
    int kind, refs;
    int width, height;
    int fmt;                /* the client's format; GFX_FMT_8888 for targets and system surfaces */
    int levels;
    unsigned char *pixels;  /* level 0 in fmt's layout; a target has none */
    SDL_GPUTexture *tex;    /* B8G8R8A8 */
    DWORD key;
    int dirty;              /* the CPU's pixels are newer than tex, in dirtyRect */
    RECT dirtyRect, lockRect;
    int mips;               /* the lower levels are to be made again */
    unsigned pinned;        /* the frame whose recorded commands use it */
    int dead;               /* released while pinned: freed after that frame */
    struct gfx_surface *prev, *next;
    unsigned version;
    /* gfx_create_layer_texture's: placed, when a draw first reads it, as a layer of an array */
    int layered, layer;
    struct gpu_array *array;
    int failed;             /* its GPU texture or layer could not be made: draws read none, and it is
                               not tried again */
};

/* An array of layered textures of one size and number of levels, of a fixed number of layers
   (array_capacity); another is made when it is full, so that nothing is ever copied, and an array
   goes when its last layer does. */
struct gpu_array {
    int width, height, levels, capacity, used;
    unsigned char *taken;
    SDL_GPUTexture *tex;
    struct gpu_array *next;
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
    struct gfx_mesh *mesh;  /* a mesh's draw */
    uint32_t firstIndex, count;  /* a mesh's indices, the wobble's vertices; a quad draw's instances */
    int indexed;
    uint32_t record;  /* the first instance: the draw's record, or a quad draw's first instance */
};

enum { STEP_PASS, STEP_UPLOADS, STEP_COPY };

/* A step of the frame: a render pass, the draws into one target or into one shadow cascade; the
   uploads of textures changed after the frame's first draws; or a copy between GPU textures. */
struct gpu_step {
    int kind;
    /* STEP_PASS */
    struct gfx_surface *target;  /* NULL: the shadow maps' layer */
    int layer;
    SDL_GPULoadOp loadColour, loadDepth;
    SDL_FColor clearColour;
    float clearDepth;
    int unresolve;               /* draws the resolved picture back into the multisampled one */
    int whole;                   /* the wobble's: its viewport is the whole target (gfx_wobble) */
    /* STEP_PASS: draws; STEP_UPLOADS: uploads */
    uint32_t first, count;
    /* STEP_COPY: from a target's texture into another target's (gfx_blit), or the screen's into the
       wobble's copy of it (gfx_wobble) */
    SDL_GPUTexture *src, *dst;
    RECT srcRect, dstRect;
};

/* An upload into a texture, or into a layer of an array. A layer with mipmaps goes up whole into a
   scratch texture (tex) of its size and levels, whose levels are made as a texture's are and then
   copied into the layer, so that a layer's levels are the texture's own on every backend. */
struct gpu_upload {
    struct gfx_surface *surface;  /* a texture or system surface whose pixels go up */
    SDL_GPUTexture *tex;
    struct gpu_array *array;  /* the layered texture's array, of which it is layer `layer` */
    uint32_t layer;
    int scratch;       /* tex is the layer's scratch texture */
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

#define FRAMES_IN_FLIGHT 2

/* A pipeline: its program and what it holds of a mode and of the target. */
struct pipe_desc {
    int program;
    int blend, src, dst;  /* enum DrawFactor */
    int depth;            /* enum DrawDepth; DRAW_DEPTH_OFF where the target has no depth */
    int cull;             /* GFX_CULL_* */
    int mask;             /* MASK_* */
    int r32f, hasDepth, samples, a2c;
};
#define PIPELINE_MAX 256

/* What a draw is made with, of its mode and gfx_set_effect and gfx_set_fog: draws are batched while
   it stays the same, as the Direct3D 9 client's were while it changed no state, and a change
   counts as a mode sent (diag/perf.h). The blend factors are the D3DBLEND numbers given, whole. */
struct draw_state {
    int src, dst;
    unsigned char combine, depth, alphaTest, alphaRef, fog, colour, address[2], filter, lightmap, into;
};

static struct {
    SDL_Window *window;
    SDL_GPUDevice *dev;
    int claimed, claimLogged;  /* the window takes the device's pictures (claim_window) */
    SDL_Mutex *lock;
    SDL_AtomicInt contentTick;
    struct gfx_surface *handles;
    SDL_GPUTexture **deadTextures;
    size_t deadCount, deadCapacity;
    struct gfx_mesh *meshes;

    /* the screen holds the picture (resolved when multisampled); target is where draws go */
    struct gfx_surface *screen, *target;
    SDL_GPUTexture *depth, *msaa;
    SDL_GPUTextureFormat depthFormat;
    SDL_GPUSampleCount samples;
    int mipmaps;

    /* what draws are made with: the mode, the effect modes' blending and combination, the fog,
       all of it resolved (struct draw_state), and the stages' textures */
    int mode;
    int effectSrc, effectDst, effectCombine;
    struct GfxFog fog;
    struct draw_state state;
    struct gfx_surface *stageTex[2];
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
    /* every pipeline, made once (make_pipelines), and which serves what: a mode's quads into each
       kind of target, flat or not; its meshes by cull mode (GFX_CULL_* - 1); the effect modes'
       quads by depth test or none and blend factors; the shadow casters; the wobble */
    struct { struct pipe_desc desc; SDL_GPUGraphicsPipeline *pipeline; } pipelines[PIPELINE_MAX];
    int npipelines;
    SDL_GPUGraphicsPipeline *quadPipeline[DRAW_MODE_COUNT][CLASS_COUNT][2];
    SDL_GPUGraphicsPipeline *meshPipeline[DRAW_MODE_COUNT][3];
    SDL_GPUGraphicsPipeline *effectPipeline[2][DRAW_FACTOR_COUNT][DRAW_FACTOR_COUNT];
    SDL_GPUGraphicsPipeline *casterPipeline, *rowsPipeline;
    SDL_GPUSampler *samplers[2][3];  /* wrapped or not, by enum DrawFilter */
    SDL_GPUSampler *pointClamp;
    SDL_GPUTexture *white;
    SDL_GPUTexture *frameCopy;  /* the screen's picture as the wobble found it (gfx_wobble) */
    int frameCopyW, frameCopyH;
    SDL_GPUBuffer *records, *views, *fogs, *shadows, *rows, *quads, *offsets, *instances;
    uint32_t recordsSize, viewsSize, fogsSize, shadowsSize, rowsSize, quadsSize, offsetsSize, instanceCount;
    SDL_GPUBuffer *corners, *triangles;  /* quad_corners and quad_triangles, uploaded once */
    struct gpu_array *arrays;  /* every array of layered textures */
    SDL_GPUTexture *noAtlas;   /* the array a quad draw binds where stage 0 reads no layer */

    /* the quads given since their draw was last made (batch_flush): the draw's record and its
       instances, whether they are flat and some go through the camera's view, the textures they
       read, stage 0's array where it reads layers, and the layered textures they read of it */
    struct {
        int open, flat, camera;
        uint32_t record, first, count, prims, bytes;
        struct gfx_surface *tex[2];
        struct gpu_array *array;
        struct gfx_surface **layers;
        size_t nlayers, layerCapacity;
    } batch;
    float matrix[16];  /* the last matrix put among the frame's quads, at row matrixIndex */
    int matrixIndex;
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
    struct bytes drawRecords, viewRecords, fogTables, shadowRecords, rowTables, texels;
    /* the instances' records with the matrices and vertices they name (render/quads.h), and the row
       each record starts at */
    struct bytes quadData, quadOffsets;
    struct gfx_surface **pins;
    size_t npins, pinCapacity;
    struct gfx_mesh **meshPins;
    size_t nmeshPins, meshPinCapacity;
    int cameraView, meshView, casterView, casterCascade;
    int fogTable;     /* the fog's table (render/vertex_fog.h) in this frame's, or -1 */
    float fogBase, fogDirection, fogCount;
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
    return s && (s->kind == K_SCREEN || s->kind == K_RTTEX);
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

static void batch_flush(void);

/* An array without layers goes; the frames recorded so far may still read it (retire_texture). A
   batch begun on it has no quads, since each holds its layer (batch_header): it ends. */
static void free_array(struct gpu_array *a)
{
    struct gpu_array **link;
    if (G.batch.array == a) {
        G.batch.open = 0;
        G.batch.array = NULL;
    }
    for (link = &G.arrays; *link; link = &(*link)->next)
        if (*link == a) {
            *link = a->next;
            break;
        }
    retire_texture(a->tex);
    free(a->taken);
    free(a);
}

/* Layered textures are given up on the thread that draws, as they are placed (place_layer). */
static void destroy_handle(struct gfx_surface *s)
{
    struct gpu_array *empty = NULL;
    lock_handles();
    if (s->prev)
        s->prev->next = s->next;
    else
        G.handles = s->next;
    if (s->next)
        s->next->prev = s->prev;
    if (s->array) {
        s->array->taken[s->layer] = 0;
        if (!--s->array->used)
            empty = s->array;
    }
    unlock_handles();
    if (empty)
        free_array(empty);
    retire_texture(s->tex);
    free(s->pixels);
    free(s);
}

/* whether the quads given since their draw was last made read s */
static int batch_reads(const struct gfx_surface *s)
{
    size_t i;
    if (!G.batch.open)
        return 0;
    if (G.batch.tex[0] == s || G.batch.tex[1] == s)
        return 1;
    for (i = 0; i < G.batch.nlayers; i++)
        if (G.batch.layers[i] == s)
            return 1;
    return 0;
}

static void free_handle(struct gfx_surface *s)
{
    int i, pinned;
    if (batch_reads(s))
        batch_flush();
    for (i = 0; i < 2; i++)
        if (G.stageTex[i] == s)
            G.stageTex[i] = NULL;
    if (G.screen == s)
        G.screen = NULL;
    if (G.target == s)
        G.target = G.screen;
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
    if (s && s->kind == STEP_PASS && s->target && !s->whole && s->target == G.target
        && s->unresolve == G.unresolving)
        return s;
    s = add_step(STEP_PASS);
    if (!s)
        return NULL;
    s->target = G.target;
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

/* How many layers an array of layered textures of a size holds: as many as make 16 MB, from 64
   (the sprite atlas's pages of 256 by 256) down to one for a texture of 16 MB or more, which is
   then as large as it would be alone. */
static int array_capacity(int w, int h)
{
    long long per = (long long)w * h * 4, n = per > 0 ? (16ll << 20) / per : 64;
    return n < 1 ? 1 : n > 64 ? 64 : (int)n;
}

/* The array a layered texture is a layer of, placed in the first array of its size and levels
   with a free layer, or in a new one; NULL where none can be made, which the texture then
   remembers. */
static struct gpu_array *place_layer(struct gfx_surface *s)
{
    struct gpu_array *a;
    int i;
    if (s->array || s->failed || !G.dev)
        return s->array;
    for (a = G.arrays; a; a = a->next)
        if (a->width == s->width && a->height == s->height && a->levels == s->levels && a->used < a->capacity)
            break;
    if (!a) {
        SDL_GPUTextureCreateInfo info;
        a = calloc(1, sizeof *a);
        if (!a) {
            s->failed = 1;
            return NULL;
        }
        a->width = s->width;
        a->height = s->height;
        a->levels = s->levels;
        a->capacity = array_capacity(s->width, s->height);
        a->taken = calloc((size_t)a->capacity, 1);
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
        info.format = TARGET_FORMAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)s->width;
        info.height = (Uint32)s->height;
        info.layer_count_or_depth = (Uint32)a->capacity;
        info.num_levels = (Uint32)s->levels;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        a->tex = a->taken ? SDL_CreateGPUTexture(G.dev, &info) : NULL;
        if (!a->tex) {
            SDL_Log("gfx: an array of %d layers of %dx%d: %s", a->capacity, s->width, s->height, SDL_GetError());
            free(a->taken);
            free(a);
            s->failed = 1;
            return NULL;
        }
        a->next = G.arrays;
        G.arrays = a;
    }
    for (i = 0; a->taken[i]; i++)
        ;
    lock_handles();
    a->taken[i] = 1;
    a->used++;
    s->array = a;
    s->layer = i;
    s->dirty = 1;
    whole_rect(&s->dirtyRect, s);
    unlock_handles();
    return a;
}

/* A surface's GPU texture, made when first needed: a colour target as well where its mipmaps are
   made on the GPU (SDL's mipmaps draw); a layered texture's array. */
static SDL_GPUTexture *ensure_texture(struct gfx_surface *s)
{
    if (s->layered)
        return place_layer(s) ? s->array->tex : NULL;
    if (s->tex || s->failed || !G.dev)
        return s->tex;
    s->tex = new_texture(s->width, s->height, s->levels,
                         SDL_GPU_TEXTUREUSAGE_SAMPLER | (s->levels > 1 ? SDL_GPU_TEXTUREUSAGE_COLOR_TARGET : 0));
    if (!s->tex) {
        SDL_Log("gfx: SDL_CreateGPUTexture %dx%d: %s", s->width, s->height, SDL_GetError());
        s->failed = 1;
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
    u->array = s->layered ? s->array : NULL;
    u->layer = s->layered ? (uint32_t)s->layer : 0;
    u->scratch = s->layered && tex != s->array->tex;
    u->x = x;
    u->y = y;
    u->w = w;
    u->h = h;
    u->offset = offset;
    u->mips = mips;
    return 1;
}

/* A layered texture's levels made anew: the whole of its top level staged for a scratch texture
   (struct gpu_upload), which goes once the frame is drawn. Answers 0 without memory for it. */
static int layer_mipmaps(struct gfx_surface *s)
{
    SDL_GPUTexture *scratch;
    uint32_t offset;
    RECT r;
    int ok;
    whole_rect(&r, s);
    scratch = new_texture(s->width, s->height, s->levels, SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET);
    if (!scratch)
        return 0;
    ok = stage_texels(s, &r, &offset)
         && add_upload(s, scratch, 0, 0, (uint32_t)s->width, (uint32_t)s->height, offset, 1);
    retire_texture(scratch);
    return ok;
}

/* The GPU texture a draw samples for a surface (a layered texture's array), with the CPU's changes
   since its last upload staged; the surface stays alive until the frame is drawn. */
static SDL_GPUTexture *texture_for_draw(struct gfx_surface *s)
{
    int dirty, mips;
    RECT r;
    uint32_t offset;
    SDL_GPUTexture *tex;
    if (!s)
        return NULL;
    if (is_target(s)) {
        pin(s);
        return s->tex;
    }
    if (!(tex = ensure_texture(s)))
        return NULL;
    lock_handles();
    dirty = s->dirty;
    mips = s->mips || (dirty && s->levels > 1);
    r = s->dirtyRect;
    s->dirty = 0;
    s->mips = 0;
    unlock_handles();
    if (s->layered && mips && s->levels > 1) {
        if (!layer_mipmaps(s)) {
            mark_dirty(s, NULL);
            lock_handles();
            s->mips = 1;
            unlock_handles();
        }
        pin(s);
        return tex;
    }
    if (dirty && clip_rect(&r, s)) {
        if (stage_texels(s, &r, &offset)
            && add_upload(s, tex, (uint32_t)r.left, (uint32_t)r.top, (uint32_t)(r.right - r.left),
                          (uint32_t)(r.bottom - r.top), offset, mips && s->levels > 1))
            mips = 0;
        else
            mark_dirty(s, &r);  /* staged by a later draw */
    }
    if (mips && s->levels > 1 && !add_upload(s, tex, 0, 0, 0, 0, 0, 1)) {
        lock_handles();
        s->mips = 1;
        unlock_handles();
    }
    pin(s);
    return tex;
}

/* ---- samplers and pipelines ---- */

/* The samplers, made once: linear between texels and mipmap levels, linear on the top level, or
   the nearest texel of it (render/draw_modes.h, enum DrawFilter), each repeating or clamped to the
   edge (a border is the shaders', records.hlsli). Linear minification is anisotropic, as the
   Direct3D 9 client had it (build/ref/COMMIT.txt). */
static int make_samplers(void)
{
    int wrap, filter;
    for (wrap = 0; wrap < 2; wrap++)
        for (filter = 0; filter < 3; filter++) {
            SDL_GPUSamplerCreateInfo info;
            int linear = filter != DRAW_FILTER_POINT;
            SDL_zero(info);
            info.mag_filter = info.min_filter = linear ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
            info.mipmap_mode = filter == DRAW_FILTER_MIPMAPS ? SDL_GPU_SAMPLERMIPMAPMODE_LINEAR
                                                             : SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
            info.address_mode_u = info.address_mode_v = info.address_mode_w =
                wrap ? SDL_GPU_SAMPLERADDRESSMODE_REPEAT : SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            info.min_lod = 0.0f;
            info.max_lod = filter == DRAW_FILTER_MIPMAPS ? 1000.0f : 0.0f;
            info.enable_anisotropy = linear;
            info.max_anisotropy = 16.0f;
            G.samplers[wrap][filter] = SDL_CreateGPUSampler(G.dev, &info);
            if (!G.samplers[wrap][filter])
                return 0;
        }
    return 1;
}

static SDL_GPUSampler *stage_sampler(const struct draw_state *s, int stage)
{
    return G.samplers[s->address[stage] == DRAW_ADDRESS_WRAP][stage ? DRAW_FILTER_LINEAR : s->filter];
}

static SDL_GPUBlendFactor sdl_factor(int f)
{
    switch (f) {
    case DRAW_FACTOR_ZERO: return SDL_GPU_BLENDFACTOR_ZERO;
    case DRAW_FACTOR_SRC_COLOUR: return SDL_GPU_BLENDFACTOR_SRC_COLOR;
    case DRAW_FACTOR_INV_SRC_COLOUR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
    case DRAW_FACTOR_SRC_ALPHA: return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    case DRAW_FACTOR_INV_SRC_ALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    case DRAW_FACTOR_DST_COLOUR: return SDL_GPU_BLENDFACTOR_DST_COLOR;
    case DRAW_FACTOR_INV_DST_COLOUR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR;
    default: return SDL_GPU_BLENDFACTOR_ONE;
    }
}

static SDL_GPUGraphicsPipeline *create_pipeline(const struct pipe_desc *p)
{
    SDL_GPUGraphicsPipelineCreateInfo info;
    SDL_GPUColorTargetDescription colour;
    SDL_GPUVertexBufferDescription buffers[2];
    SDL_GPUVertexAttribute attrs[8];
    Uint32 nattrs = 0;
    Uint32 nbuffers = 2;
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
    if (p->program == PROG_QUAD) {
        /* the corner; per instance, where its record starts in the frame's quads */
        buffers[0].pitch = 4;
        ATTR(0, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
    } else if (p->program == PROG_MESH) {
        buffers[0].pitch = sizeof(struct GfxMeshVertex);
        ATTR(0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
        ATTR(1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 12);
        ATTR(2, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 24);
        ATTR(3, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 28);
        ATTR(4, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 32);
        ATTR(5, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 40);
    } else if (p->program == PROG_CASTER) {
        buffers[0].pitch = sizeof(struct GfxMeshVertex);
        ATTR(0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
        ATTR(1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 32);
    } else {
        /* the wobble's triangle comes from the vertex index: the draw's index alone */
        buffers[0] = buffers[1];
        nbuffers = 1;
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
    info.vertex_input_state.num_vertex_buffers = nbuffers;
    info.vertex_input_state.vertex_attributes = attrs;
    info.vertex_input_state.num_vertex_attributes = nattrs;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    /* Direct3D 9 culls by the winding on the screen, y down, as the target's pixels are laid out
       here too: GFX_CULL_CW drops the clockwise triangles, GFX_CULL_CCW the others */
    info.rasterizer_state.cull_mode = p->cull == GFX_CULL_NONE ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK;
    info.rasterizer_state.front_face = p->cull == GFX_CULL_CW ? SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
                                                              : SDL_GPU_FRONTFACE_CLOCKWISE;
    info.rasterizer_state.enable_depth_clip = true;
    info.multisample_state.sample_count = (SDL_GPUSampleCount)p->samples;
    info.multisample_state.enable_alpha_to_coverage = p->a2c != 0;
    info.depth_stencil_state.enable_depth_test = p->depth != DRAW_DEPTH_OFF;
    info.depth_stencil_state.enable_depth_write = p->depth == DRAW_DEPTH_WRITE || p->depth == DRAW_DEPTH_ALWAYS;
    info.depth_stencil_state.compare_op = p->depth == DRAW_DEPTH_ALWAYS ? SDL_GPU_COMPAREOP_ALWAYS
                                                                        : SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    colour.format = p->r32f ? SDL_GPU_TEXTUREFORMAT_R32_FLOAT : TARGET_FORMAT;
    colour.blend_state.enable_blend = p->blend != 0;
    colour.blend_state.src_color_blendfactor = sdl_factor(p->src);
    colour.blend_state.dst_color_blendfactor = sdl_factor(p->dst);
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
    info.target_info.has_depth_stencil_target = p->hasDepth != 0;
    info.target_info.depth_stencil_format = G.depthFormat;
    return SDL_CreateGPUGraphicsPipeline(G.dev, &info);
}

/* The pipeline of a description, made once: modes that differ in nothing a pipeline holds share
   theirs. */
static SDL_GPUGraphicsPipeline *pipeline(struct pipe_desc *p)
{
    int i;
    if (!p->hasDepth)
        p->depth = DRAW_DEPTH_OFF;
    if (!p->blend)
        p->src = p->dst = DRAW_FACTOR_ONE;
    for (i = 0; i < G.npipelines; i++)
        if (!memcmp(&G.pipelines[i].desc, p, sizeof *p))
            return G.pipelines[i].pipeline;
    if (G.npipelines == PIPELINE_MAX) {
        SDL_Log("gfx: more than %d pipelines", PIPELINE_MAX);
        return NULL;
    }
    G.pipelines[i].pipeline = create_pipeline(p);
    if (!G.pipelines[i].pipeline) {
        SDL_Log("gfx: SDL_CreateGPUGraphicsPipeline: %s", SDL_GetError());
        return NULL;
    }
    G.pipelines[i].desc = *p;
    G.npipelines++;
    return G.pipelines[i].pipeline;
}

/* custom: multisampling smooths the edges alpha-tested texels cut, where Direct3D 9 had the
   drivers' transparency supersampling, for what it multisampled: draws without blending that are
   not flat on the screen. The one rule for a pipeline and the draw records made with it. */
static int alpha_to_coverage(const struct DrawModeDesc *m, int samples, int flat)
{
    return samples != SDL_GPU_SAMPLECOUNT_1 && m->src == DRAW_BLEND_OFF && !flat && m->alphaTest == DRAW_ALPHA_AT_LEAST;
}

/* What a mode makes of a kind of target's pipeline: its blending (by `src` and `dst` when the mode
   takes the draw's), depth, colour mask, samples, and alpha to coverage. */
static void mode_pipe(struct pipe_desc *p, int program, const struct DrawModeDesc *m, int cls, int src, int dst, int flat)
{
    memset(p, 0, sizeof *p);
    p->program = program;
    p->blend = m->src != DRAW_BLEND_OFF;
    if (p->blend)
        draw_blend_factors(m->src == DRAW_BLEND_FROM_DRAW ? src : m->src, m->dst == DRAW_BLEND_FROM_DRAW ? dst : m->dst,
                           &p->src, &p->dst);
    p->depth = m->depth;
    p->cull = GFX_CULL_NONE;
    /* the screen is X8R8G8B8 on Direct3D 9: its alpha stays 1 */
    p->mask = !m->colour ? MASK_NONE : cls == CLASS_TEXTURE ? MASK_RGBA : MASK_RGB;
    p->hasDepth = cls == CLASS_SCREEN && G.depth != NULL;
    p->samples = cls == CLASS_SCREEN ? (int)G.samples : (int)SDL_GPU_SAMPLECOUNT_1;
    p->a2c = alpha_to_coverage(m, p->samples, flat);
}

/* Every pipeline the modes draw with, once the screen's samples and depth are known: a mode's
   quads into each kind of target it draws into, flat and not, its meshes in each cull mode, the
   effect modes' quads by every pair of the blend factors the screen has, the shadow casters' and
   the wobble's (gfx_wobble). */
static int make_pipelines(void)
{
    struct pipe_desc p;
    int mode, cls, flat, cull, s, d;
    for (mode = 0; mode < DRAW_MODE_COUNT; mode++) {
        const struct DrawModeDesc *m = draw_mode(mode);
        if (m->src == DRAW_BLEND_FROM_DRAW) {
            for (s = 0; s < DRAW_FACTOR_COUNT; s++)
                for (d = 0; d < DRAW_FACTOR_COUNT; d++) {
                    SDL_GPUGraphicsPipeline **at = &G.effectPipeline[m->depth == DRAW_DEPTH_OFF][s][d];
                    mode_pipe(&p, PROG_QUAD, m, CLASS_SCREEN, DRAW_BLEND_ONE, DRAW_BLEND_ONE, 1);
                    p.src = s;
                    p.dst = d;
                    if (!(*at = pipeline(&p)))
                        return 0;
                }
            continue;
        }
        for (cls = 0; cls < CLASS_COUNT; cls++) {
            if (!(m->into & (DRAW_INTO_SCREEN << cls)))
                continue;
            for (flat = 0; flat < 2; flat++) {
                mode_pipe(&p, PROG_QUAD, m, cls, 0, 0, flat);
                if (!(G.quadPipeline[mode][cls][flat] = pipeline(&p)))
                    return 0;
            }
        }
        if (m->into & DRAW_MESHES)
            for (cull = GFX_CULL_NONE; cull <= GFX_CULL_CCW; cull++) {
                mode_pipe(&p, PROG_MESH, m, CLASS_SCREEN, 0, 0, 0);
                p.cull = cull;
                if (!(G.meshPipeline[mode][cull - GFX_CULL_NONE] = pipeline(&p)))
                    return 0;
            }
    }
    memset(&p, 0, sizeof p);
    p.program = PROG_CASTER;
    p.depth = DRAW_DEPTH_WRITE;
    p.cull = GFX_CULL_NONE;
    p.r32f = 1;
    p.hasDepth = 1;
    p.samples = SDL_GPU_SAMPLECOUNT_1;
    G.casterPipeline = pipeline(&p);
    /* the wobble's, into the screen with its depth kept, its alpha 1 */
    memset(&p, 0, sizeof p);
    p.program = PROG_ROWS;
    p.depth = DRAW_DEPTH_OFF;
    p.cull = GFX_CULL_NONE;
    p.mask = MASK_RGB;
    p.hasDepth = G.depth != NULL;
    p.samples = (int)G.samples;
    G.rowsPipeline = pipeline(&p);
    return G.casterPipeline && G.rowsPipeline;
}

/* The pipeline of a quad draw in the current mode into a pass's kind of target, NULL where the mode
   does not draw into it. */
static SDL_GPUGraphicsPipeline *quad_pipeline(int mode, const struct draw_state *s, int cls, int flat)
{
    const struct DrawModeDesc *m = draw_mode(mode);
    int src, dst;
    if (m->src != DRAW_BLEND_FROM_DRAW)
        return G.quadPipeline[mode][cls][flat];
    if (cls != CLASS_SCREEN)
        return NULL;
    draw_blend_factors(s->src, s->dst, &src, &dst);
    return G.effectPipeline[m->depth == DRAW_DEPTH_OFF][src][dst];
}

/* ---- modes ---- */

static void resolve_state(struct draw_state *s, int mode, int src, int dst, int combine, int fog)
{
    const struct DrawModeDesc *m = draw_mode(mode);
    memset(s, 0, sizeof *s);
    s->combine = (unsigned char)(m->combine == DRAW_COMBINE_FROM_DRAW ? combine : m->combine);
    s->src = m->src == DRAW_BLEND_FROM_DRAW ? src : m->src;
    s->dst = m->dst == DRAW_BLEND_FROM_DRAW ? dst : m->dst;
    s->depth = m->depth;
    s->alphaTest = m->alphaTest;
    s->alphaRef = m->alphaRef;
    s->fog = (unsigned char)(m->fog && fog);
    s->colour = m->colour;
    s->address[0] = m->address[0];
    s->address[1] = m->address[1];
    s->filter = m->filter;
    s->lightmap = m->lightmap;
    s->into = m->into;
}

/* The draws batched so far are made with what they were batched with when that changes. */
static void change_state(int mode, int src, int dst, int combine, const struct GfxFog *fog)
{
    struct draw_state s;
    int changed;
    resolve_state(&s, mode, src, dst, combine, fog->on);
    changed = memcmp(&s, &G.state, sizeof s) || memcmp(fog, &G.fog, sizeof *fog);
    perf_count_mode(changed);
    if (changed)
        batch_flush();
    if (memcmp(fog, &G.fog, sizeof *fog)) {
        G.cameraView = G.meshView = G.casterView = -1;
        G.fogTable = -1;
    }
    G.mode = mode;
    G.effectSrc = src;
    G.effectDst = dst;
    G.effectCombine = combine;
    G.fog = *fog;
    G.state = s;
}

void gfx_set_mode(int mode)
{
    if (draw_mode(mode))
        change_state(mode, G.effectSrc, G.effectDst, G.effectCombine, &G.fog);
}

void gfx_set_effect(int srcBlend, int destBlend, int combine)
{
    if (combine < 0 || combine >= DRAW_COMBINE_COUNT || combine == 2)
        combine = G.effectCombine;
    change_state(G.mode, srcBlend, destBlend, combine, &G.fog);
}

void gfx_set_fog(const struct GfxFog *fog)
{
    struct GfxFog f;
    memset(&f, 0, sizeof f);
    if (fog && fog->on)
        f = *fog;
    change_state(G.mode, G.effectSrc, G.effectDst, G.effectCombine, &f);
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
   part of it, and a clamped read goes no further than the centre of the target's last texel, as
   clamping at the target's edge would. */
static void stage_scale(float *scale, float *limit, const struct gfx_surface *s, int clamped)
{
    const struct gfx_surface *g = is_target(s) ? s : NULL;
    scale[0] = g ? (float)g->width / (float)(g->width + MARGIN) : 1.0f;
    scale[1] = g ? (float)g->height / (float)(g->height + MARGIN) : 1.0f;
    limit[0] = g && clamped ? ((float)g->width - 0.5f) / (float)(g->width + MARGIN) : 3.0e38f;
    limit[1] = g && clamped ? ((float)g->height - 0.5f) / (float)(g->height + MARGIN) : 3.0e38f;
}

/* The fog's table of the rhw at which its steps begin (render/vertex_fog.h) in this frame's, made
   once a frame for each fog the quads are drawn in: its number, -1 without memory. */
static int fog_table(void)
{
    struct VertexFogTable table;
    long at;
    if (G.fogTable >= 0)
        return G.fogTable;
    at = bytes_add(&G.fogTables, sizeof table.rhw, 16);
    if (at < 0)
        return -1;
    vertex_fog_table(G.fog.start, G.fog.end, G.fog.origin, G.fog.span, &table);
    memcpy(G.fogTables.data + at, table.rhw, sizeof table.rhw);
    G.fogBase = table.base;
    G.fogDirection = table.direction;
    G.fogCount = table.count;
    G.fogTable = (int)(at / (long)sizeof table.rhw);
    return G.fogTable;
}

/* What a draw record holds of the state it is made with, its stages' textures and its pass; the
   fog's table where the draw is of quads, whose fog is the vertices'. */
static void fill_state(struct GpuDraw *r, const struct gpu_step *pass, const struct draw_state *s,
                       struct gfx_surface *const *tex, int quads, int a2c)
{
    int i;
    float w = 1.0f, h = 1.0f;
    struct gfx_surface *t = pass->target;
    if (t) {
        w = (float)t->width;
        h = (float)t->height;
    }
    r->mode[1] = s->alphaTest;
    r->mode[2] = s->fog;
    r->target[0] = (float)s->alphaRef;
    r->target[1] = w * 0.5f;
    r->target[2] = h * 0.5f;
    r->target[3] = a2c ? 1.0f : 0.0f;
    r->stages[0] = s->combine;
    for (i = 0; i < 2; i++) {
        if (s->address[i] == DRAW_ADDRESS_BORDER && tex[i])
            r->stages[1] |= 1u << i;
        stage_scale(r->uvScale + i * 2, r->uvLimit + i * 2, tex[i], s->address[i] != DRAW_ADDRESS_WRAP);
    }
    r->stages[2] = s->lightmap;
    if (s->fog && quads) {
        int table = fog_table();
        r->mode[2] = table >= 0;
        r->stages[3] = table >= 0 ? (uint32_t)table : 0;
        r->fog[0] = G.fogBase;
        r->fog[1] = G.fogDirection;
        r->fog[2] = G.fogCount;
    }
    colour_floats(r->fogColour, G.fog.colour);
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

/* The meshes' fog in a view record: its start and end as view depths, and 1 / (end - start)
   (render/gfx.h, GfxFog), computed as the Direct3D 9 client computed them. */
static void view_fog(float *fog)
{
    const struct GfxFog *f = &G.fog;
    float start = f->origin, end = f->origin + f->span;
    if (f->end != f->start) {
        start = f->start * f->span + f->origin;
        end = f->end * f->span + f->origin;
    }
    fog[0] = start;
    fog[1] = end;
    fog[2] = end != start ? 1.0f / (end - start) : 0.0f;
}

static int add_view(const float *toClip, const float *toView)
{
    struct GpuView *view;
    long at = bytes_add(&G.viewRecords, sizeof *view, 16);
    if (at < 0)
        return 0;
    view = (struct GpuView *)(G.viewRecords.data + at);
    memcpy(view->toClip, toClip, sizeof view->toClip);
    if (toView)
        memcpy(view->toView, toView, sizeof view->toView);
    view_fog(view->fog);
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
    *cache = add_view(clip, view);
    return *cache;
}

/* ---- client draws ---- */

/* A stage's texture and sampler; a layered texture reads as white here, since only a quad's stage
   0 reads layers (quad_draw). */
static void bind_stage(SDL_GPUTextureSamplerBinding *bind, const struct draw_state *s, struct gfx_surface *surface,
                       int stage)
{
    SDL_GPUTexture *tex = surface && surface->layered ? NULL : texture_for_draw(surface);
    bind->texture = tex ? tex : G.white;
    bind->sampler = stage_sampler(s, stage);
}

/* A draw of the instances first to first + count of the frame's quads, whose headers name the
   draw record `record`, in `mode` with the state s, the stages' textures tex (stage 0 the layers
   of `array` where it is one, of which the instances read the `nlayers` textures `layers`) into
   the current target: flat or not, and through the camera's view where some are QUAD_CAMERA. A
   mode that does not draw into the target draws nothing (render/gfx.h, gfx_set_mode). Answers
   whether it recorded the draw. */
static int quad_draw(int mode, const struct draw_state *s, struct gfx_surface *const *tex, struct gpu_array *array,
                     struct gfx_surface *const *layers, size_t nlayers, uint32_t record, uint32_t first,
                     uint32_t count, int flat, int camera)
{
    struct gpu_step *pass;
    struct gpu_draw *d;
    struct GpuDraw *r;
    struct gfx_surface *stages[2];
    SDL_GPUTextureSamplerBinding bind[3];
    SDL_GPUGraphicsPipeline *pipe;
    size_t i;
    int cls;
    if (!G.dev || !G.target || G.shadowPass || !count || (camera && !G.worldCameraSet))
        return 0;
    /* a texture's upload may need a step of its own before the pass */
    stages[0] = tex[0];
    stages[1] = tex[1];
    if (array) {
        for (i = 0; i < nlayers; i++)
            texture_for_draw(layers[i]);
        stages[0] = nlayers ? layers[0] : NULL;
        bind[0].texture = G.white;
        bind[2].texture = array->tex;
    } else {
        bind_stage(bind, s, tex[0], 0);
        bind[2].texture = G.noAtlas;
    }
    bind[0].sampler = bind[2].sampler = stage_sampler(s, 0);
    bind_stage(bind + 1, s, tex[1], 1);
    pass = current_pass();
    if (!pass)
        return 0;
    cls = pass->target == G.screen ? CLASS_SCREEN : CLASS_TEXTURE;
    pipe = quad_pipeline(mode, s, cls, flat);
    if (!pipe)
        return 0;
    d = add_draw(pass);
    if (!d)
        return 0;
    d->program = PROG_QUAD;
    d->pipeline = pipe;
    memcpy(d->bind, bind, sizeof bind);
    d->count = count;
    d->record = first;
    r = (struct GpuDraw *)(G.drawRecords.data + (size_t)record * sizeof *r);
    fill_state(r, pass, s, stages, 1, alpha_to_coverage(draw_mode(mode), (int)pass_samples(pass), flat));
    if (array)
        r->stages[1] |= 4;
    if (camera)
        r->mode[3] = (uint32_t)view_for(&G.worldCamera, &G.cameraView, pass);
    G.target->version = next_version();
    return 1;
}

/* The draw of the quads given since it was last made, in the current mode with the stages'
   textures, as the Direct3D 9 client's batch was drawn when a state changed; counted as
   diag/perf.h says. */
static void batch_flush(void)
{
    if (!G.batch.open)
        return;
    G.batch.open = 0;
    if (!quad_draw(G.mode, &G.state, G.batch.tex, G.batch.array, G.batch.layers, G.batch.nlayers, G.batch.record,
                   G.batch.first, G.batch.count, G.batch.flat, G.batch.camera))
        return;
    g_perf.draws++;
    g_perf.prims += G.batch.prims;
    g_perf.instanceBytes += G.batch.bytes;
}

/* The quads given now join the batch while they are drawn with what it is drawn with: the same
   textures, or stage 0 a layer of the same array, and all flat or none; otherwise the batch's draw
   is made and a new batch begins, with a draw record of its own. Answers 0 where nothing can be
   drawn. */
static int batch_join(int flat)
{
    struct gfx_surface *t0 = G.stageTex[0], *t1 = G.stageTex[1];
    struct gpu_array *array = t0 && t0->layered ? place_layer(t0) : NULL;
    struct GpuDraw *r;
    if (!G.dev || !G.target || G.shadowPass)
        return 0;
    if (array)
        t0 = NULL;
    if (G.batch.open && G.batch.flat == flat && G.batch.tex[0] == t0 && G.batch.tex[1] == t1 && G.batch.array == array)
        return 1;
    batch_flush();
    G.batch.record = add_record(&r);
    if (!r)
        return 0;
    G.batch.open = 1;
    G.batch.flat = flat;
    G.batch.camera = 0;
    G.batch.first = (uint32_t)(G.quadOffsets.size / 4);
    G.batch.count = G.batch.prims = G.batch.bytes = 0;
    G.batch.tex[0] = t0;
    G.batch.tex[1] = t1;
    G.batch.array = array;
    G.batch.nlayers = 0;
    return 1;
}

/* The device's part of the header of an instance of the batch: the layer stage 0 reads, the draw
   record, and whether its corners move onto pixel boundaries: a flat draw in a multisampled
   pass (render/shaders/instances.hlsli, snapped). The batch holds the layered texture it reads,
   which goes up and stays alive with the draw; 0 without memory for that, and the instance is not
   drawn. */
static int batch_header(struct QuadHeader *h, int matrix)
{
    struct gfx_surface *t0 = G.stageTex[0];
    h->layer = G.batch.array ? (uint32_t)t0->layer : 0;
    h->draw = G.batch.record;
    h->snap = G.batch.flat && G.target == G.screen && G.msaa;
    h->matrix = (uint32_t)matrix;
    if (G.batch.array && (!G.batch.nlayers || G.batch.layers[G.batch.nlayers - 1] != t0)) {
        if (!grow((void **)&G.batch.layers, &G.batch.layerCapacity, G.batch.nlayers + 1, sizeof *G.batch.layers))
            return 0;
        G.batch.layers[G.batch.nlayers++] = t0;
    }
    return 1;
}

/* An instance's record into the frame's quads and its place into their offsets: n words, a whole
   number of 16-byte rows, drawing `prims` triangles. */
static void batch_add(const uint32_t *words, int n, uint32_t prims)
{
    long at = bytes_add(&G.quadData, (size_t)n * 4, 16), offset;
    if (at < 0)
        return;
    offset = bytes_add(&G.quadOffsets, 4, 4);
    if (offset < 0) {
        G.quadData.size = (size_t)at;
        return;
    }
    memcpy(G.quadData.data + at, words, (size_t)n * 4);
    *(uint32_t *)(G.quadOffsets.data + offset) = (uint32_t)(at / 16);
    G.batch.count++;
    G.batch.prims += prims;
    G.batch.bytes += (uint32_t)n * 4 + 4;
}

/* A GFX_QUAD_WORLD instance's matrix and projection in the frame's quads (the projection alone for
   points in view space): the row it starts at, the last one's again where they are the same, -1
   without memory. */
static int quad_matrix(const struct GfxQuad *q)
{
    float m[16];
    long at;
    if (q->world.viewSpace)
        memset(m, 0, 12 * sizeof *m);
    else
        memcpy(m, q->world.view, 12 * sizeof *m);
    m[12] = q->world.proj[0];
    m[13] = q->world.proj[1];
    m[14] = q->world.centre[0];
    m[15] = q->world.centre[1];
    if (G.matrixIndex >= 0 && !memcmp(m, G.matrix, sizeof m))
        return G.matrixIndex;
    at = bytes_add(&G.quadData, sizeof m, 16);
    if (at < 0)
        return -1;
    memcpy(G.quadData.data + at, m, sizeof m);
    memcpy(G.matrix, m, sizeof m);
    G.matrixIndex = (int)(at / 16);
    g_perf.instanceBytes += sizeof m;
    return G.matrixIndex;
}

void gfx_draw_quad(const struct GfxQuad *q)
{
    uint32_t words[QUAD_WORDS_MAX];
    struct QuadHeader h;
    int matrix = 0, n;
    g_perf.drawCalls++;
    if (!q || (q->kind != GFX_QUAD_SPRITE && q->kind != GFX_QUAD_WORLD))
        return;
    if (!batch_join(q->kind == GFX_QUAD_SPRITE || q->world.flat))
        return;
    if ((q->kind == GFX_QUAD_WORLD && (matrix = quad_matrix(q)) < 0) || !batch_header(&h, matrix))
        return;
    n = quad_words(q, &h, words);
    batch_add(words, n, 2);
}

/* The records of a primitive's instances, with the device's header h: a rectangle's own, or the
   primitive's vertices put into the frame's quads, which its records name; answers how many
   instances it added. */
static uint32_t vertex_records(const struct TexDrawVertex *v, int n, int strip, int world, const struct QuadHeader *h)
{
    uint32_t words[QUAD_WORDS_MAX], before = G.batch.count;
    int count = quad_instance_count(n, strip), i, m;
    long at;
    if (strip && !world && (m = quad_rect_words(v, h, words)) > 0) {
        batch_add(words, m, 2);
        return G.batch.count - before;
    }
    at = bytes_add(&G.quadData, (size_t)n * sizeof *v, 16);
    if (at < 0)
        return 0;
    memcpy(G.quadData.data + at, v, (size_t)n * sizeof *v);
    G.batch.bytes += (uint32_t)n * sizeof *v;
    for (i = 0; i < count; i++) {
        m = quad_vertex_words(n, strip, world, (uint32_t)(at / 4), i, h, words);
        batch_add(words, m, strip ? 2 : 1);
    }
    return G.batch.count - before;
}

void gfx_draw_vertices(const struct TexDrawVertex *v, int n, int strip, int world)
{
    struct QuadHeader h;
    g_perf.drawCalls++;
    if (!v || !quad_instance_count(n, strip) || (world && !G.worldCameraSet)
        || !batch_join(!world && quad_vertices_flat(v, n)))
        return;
    if (!batch_header(&h, 0))
        return;
    G.batch.camera |= world != 0;
    vertex_records(v, n, strip, world, &h);
}

/* A quad of the device's own (a blit, a clear inside a pass) drawn at once into the current
   target, in `mode` with the state s and stage 0's texture tex0: the strip of four flat vertices
   v. */
static int own_quad(int mode, const struct draw_state *s, struct gfx_surface *tex0, const struct TexDrawVertex *v)
{
    struct QuadHeader h;
    struct gfx_surface *tex[2];
    struct GpuDraw *r;
    uint32_t first;
    batch_flush();
    if (!G.dev || !G.target || G.shadowPass)
        return 0;
    h.layer = 0;
    h.draw = add_record(&r);
    h.snap = G.target == G.screen && G.msaa;
    h.matrix = 0;
    if (!r)
        return 0;
    /* the batch's count and bytes take the quad, and are begun again for the next batch */
    first = (uint32_t)(G.quadOffsets.size / 4);
    G.batch.count = G.batch.prims = G.batch.bytes = 0;
    if (!vertex_records(v, 4, 1, 0, &h))
        return 0;
    tex[0] = tex0 && !tex0->layered ? tex0 : NULL;
    tex[1] = NULL;
    return quad_draw(mode, s, tex, NULL, NULL, 0, h.draw, first, 1, 1, 0);
}

/* A quad from a rectangle of a surface to one of a target (DRAW_BLIT): one texel to a pixel when
   they are the same size, stretched with the nearest texel otherwise; the client's mode, textures
   and target are as before after. */
static void draw_textured_quad(struct gfx_surface *dst, const RECT *d, struct gfx_surface *src, const RECT *s)
{
    struct TexDrawVertex v[4];
    struct gfx_surface *prevTarget = G.target;
    struct draw_state state;
    float tw = (float)src->width, th = (float)src->height;
    int i;
    for (i = 0; i < 4; i++) {
        v[i].z = 0.0f;
        v[i].rhw = 1.0f;
        v[i].diffuse = 0xffffffff;
    }
    v[0].x = (float)d->left - 0.5f;  v[0].y = (float)d->top - 0.5f;    v[0].u = s->left / tw;  v[0].v = s->top / th;
    v[1].x = (float)d->right - 0.5f; v[1].y = (float)d->top - 0.5f;    v[1].u = s->right / tw; v[1].v = s->top / th;
    v[2].x = (float)d->left - 0.5f;  v[2].y = (float)d->bottom - 0.5f; v[2].u = s->left / tw;  v[2].v = s->bottom / th;
    v[3].x = (float)d->right - 0.5f; v[3].y = (float)d->bottom - 0.5f; v[3].u = s->right / tw; v[3].v = s->bottom / th;
    batch_flush();
    g_perf.blitQuads++;
    resolve_state(&state, DRAW_BLIT, 0, 0, 0, 0);
    G.target = dst;
    own_quad(DRAW_BLIT, &state, src, v);
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
    G.casterView = add_view(clip, view);
    G.casterCascade = G.shadowCascade;
    return G.casterView;
}

void gfx_draw_mesh(gfx_mesh *m, const struct GfxMeshDraw *draw)
{
    struct gpu_step *pass;
    struct gpu_draw *d;
    struct GpuDraw *r;
    SDL_GPUGraphicsPipeline *pipe;
    SDL_GPUTextureSamplerBinding bind[2];
    int cull;
    g_perf.drawCalls++;
    batch_flush();
    if (!G.dev || !G.target || !m || !draw || !G.worldViewSet || draw->first < 0 || draw->count <= 0
        || draw->count % 3 || draw->first > m->nindices - draw->count)
        return;
    if (G.shadowPass && G.shadowCascade < 0)
        return;
    cull = draw->cull >= GFX_CULL_NONE && draw->cull <= GFX_CULL_CCW ? draw->cull : GFX_CULL_NONE;
    pipe = G.shadowPass ? G.casterPipeline : G.meshPipeline[G.mode][cull - GFX_CULL_NONE];
    if (!pipe || (!G.shadowPass && G.target != G.screen) || !mesh_ready(m))
        return;
    /* the casters read their texture as the opaque lists do */
    if (G.shadowPass) {
        struct draw_state caster;
        resolve_state(&caster, DRAW_OPAQUE_CLAMP, 0, 0, 0, 0);
        bind_stage(bind, &caster, G.stageTex[0], 0);
    } else {
        bind_stage(bind, &G.state, G.stageTex[0], 0);
        bind_stage(bind + 1, &G.state, G.stageTex[1], 1);
    }
    pass = current_pass();
    if (!pass)
        return;
    if (!pin_mesh(m))
        return;
    d = add_draw(pass);
    if (!d)
        return;
    d->program = G.shadowPass ? PROG_CASTER : PROG_MESH;
    d->pipeline = pipe;
    d->mesh = m;
    d->indexed = 1;
    d->firstIndex = (uint32_t)draw->first;
    d->count = (uint32_t)draw->count;
    d->bind[0] = bind[0];
    d->record = add_record(&r);
    if (!r) {
        pass->count--;
        G.ndraws--;
        return;
    }
    g_perf.draws++;
    g_perf.prims += (unsigned)draw->count / 3;
    r->uvU[0] = draw->uv[0];
    r->uvU[1] = draw->uv[2];
    r->uvU[2] = draw->uv[4];
    r->uvV[0] = draw->uv[1];
    r->uvV[1] = draw->uv[3];
    r->uvV[2] = draw->uv[5];
    r->mesh[0] = draw->kind == GFX_MESH_MODEL;
    if (G.shadowPass) {
        stage_scale(r->uvScale, r->uvLimit, G.stageTex[0], 1);
        r->mode[3] = (uint32_t)caster_view();
        return;
    }
    d->bind[1] = bind[1];
    d->bind[2].texture = G.shadowReceive ? G.shadowMaps : G.noShadow;
    d->bind[2].sampler = G.pointClamp;
    /* meshes are never flat */
    fill_state(r, pass, &G.state, G.stageTex, 0, alpha_to_coverage(draw_mode(G.mode), (int)G.samples, 0));
    memcpy(r->colour, draw->colour, sizeof r->colour);
    r->mesh[1] = draw->kind == GFX_MESH_MODEL && draw->lightGround;
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
        dst.layer = u->scratch ? 0 : u->layer;
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

/* The lower levels of each texture uploaded that wants them, made from the top one by SDL; then a
   layer's scratch texture, every level of it, into the layer. */
static void make_mipmaps(SDL_GPUCommandBuffer *cmd, size_t first, size_t count)
{
    SDL_GPUCopyPass *copy = NULL;
    size_t i;
    for (i = first; i < first + count; i++)
        if (G.uploads[i].mips)
            SDL_GenerateMipmapsForGPUTexture(cmd, G.uploads[i].tex);
    for (i = first; i < first + count; i++) {
        const struct gpu_upload *u = &G.uploads[i];
        int level;
        if (!u->scratch)
            continue;
        if (!copy && !(copy = SDL_BeginGPUCopyPass(cmd)))
            return;
        for (level = 0; level < u->array->levels; level++) {
            SDL_GPUTextureLocation src, dst;
            Uint32 w = (Uint32)u->array->width >> level, h = (Uint32)u->array->height >> level;
            SDL_zero(src);
            SDL_zero(dst);
            src.texture = u->tex;
            src.mip_level = (Uint32)level;
            dst.texture = u->array->tex;
            dst.mip_level = (Uint32)level;
            dst.layer = u->layer;
            SDL_CopyGPUTextureToTexture(copy, &src, &dst, w ? w : 1, h ? h : 1, 1, false);
        }
    }
    if (copy)
        SDL_EndGPUCopyPass(copy);
}

/* The storage buffers the programs read (src/render/shaders), at most STORAGE_BUFFERS_MAX a stage
   (make_shader):
   the quads' vertex shader the draw records, views, fog tables and the frame's quads; the meshes'
   shaders the shadows besides the records and views; the wobble's the records and the rows alone. */
static void bind_program(SDL_GPURenderPass *pass, int program)
{
    SDL_GPUBuffer *vertex[4], *fragment[2];
    Uint32 nv = 2, nf = 1;
    vertex[0] = G.records;
    vertex[1] = G.views;
    vertex[2] = program == PROG_QUAD ? G.fogs : G.shadows;
    vertex[3] = G.quads;
    fragment[0] = G.records;
    fragment[1] = G.shadows;
    if (program == PROG_QUAD)
        nv = 4;
    if (program == PROG_MESH) {
        nv = 3;
        nf = 2;
    }
    if (program == PROG_ROWS) {
        nv = 1;
        fragment[0] = G.rows;
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
    /* the target moved by half a pixel, to where Direct3D 9's pixel centres were (MARGIN); the
       wobble's covers the whole target, every sample of its first column and row too */
    viewport.x = viewport.y = s->whole ? 0.0f : 0.5f;
    viewport.w = (float)w;
    viewport.h = (float)h;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(pass, &viewport);
    scissor.x = scissor.y = 0;
    scissor.w = w;
    scissor.h = h;
    SDL_SetGPUScissor(pass, &scissor);
    for (i = s->first; i < s->first + s->count; i++) {
        const struct gpu_draw *d = &G.draws[i];
        int nbind = d->program == PROG_MESH || d->program == PROG_QUAD ? 3 : 1;
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
            /* a quad draw's vertices are the corners, its instances the offsets of their records */
            if (program == PROG_QUAD) {
                SDL_GPUBufferBinding buffers[2];
                buffers[0].buffer = G.corners;
                buffers[0].offset = 0;
                buffers[1].buffer = G.offsets;
                buffers[1].offset = 0;
                SDL_BindGPUVertexBuffers(pass, 0, buffers, 2);
                binding.buffer = G.triangles;
                binding.offset = 0;
                SDL_BindGPUIndexBuffer(pass, &binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
            } else {
                binding.buffer = G.instances;
                binding.offset = 0;
                SDL_BindGPUVertexBuffers(pass, 1, &binding, 1);
            }
        }
        if (memcmp(textures, d->bind, (size_t)nbind * sizeof *textures)) {
            memcpy(textures, d->bind, (size_t)nbind * sizeof *textures);
            SDL_BindGPUFragmentSamplers(pass, 0, textures, (Uint32)nbind);
        }
        if (d->program == PROG_QUAD) {
            SDL_DrawGPUIndexedPrimitives(pass, 6, d->count, 0, 0, d->record);
            continue;
        }
        /* the wobble's draw has no vertices: its triangle comes from the vertex index */
        if (d->mesh && d->mesh != mesh) {
            binding.buffer = d->mesh->buffer;
            binding.offset = 0;
            SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
            binding.offset = d->mesh->indexOffset;
            SDL_BindGPUIndexBuffer(pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            mesh = d->mesh;
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
    G.drawRecords.size = G.viewRecords.size = G.fogTables.size = G.shadowRecords.size = G.rowTables.size = 0;
    G.texels.size = G.quadData.size = G.quadOffsets.size = 0;
    G.batch.open = 0;
    G.cameraView = G.meshView = G.casterView = G.casterCascade = G.shadowRecord = G.fogTable = G.matrixIndex = -1;
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
   download a rectangle of the screen (gfx_read_frame). */
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
    uint32_t oRecords, oViews, oFogs, oShadows, oRows, oQuads, oOffsets, oTexels, total;
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
    if (!G.fogTables.size)
        bytes_add(&G.fogTables, 16, 16);
    if (!G.shadowRecords.size)
        bytes_add(&G.shadowRecords, sizeof(struct GpuShadow), 16);
    if (!G.quadData.size)
        bytes_add(&G.quadData, 16, 16);
    if (!G.quadOffsets.size)
        bytes_add(&G.quadOffsets, 4, 4);
    instances = instances_needed((uint32_t)(G.drawRecords.size / sizeof(struct GpuDraw)));
    oRecords = 0;
    oViews = align16(oRecords + (uint32_t)G.drawRecords.size);
    oFogs = align16(oViews + (uint32_t)G.viewRecords.size);
    oShadows = align16(oFogs + (uint32_t)G.fogTables.size);
    oRows = align16(oShadows + (uint32_t)G.shadowRecords.size);
    oQuads = align16(oRows + (uint32_t)G.rowTables.size);
    oOffsets = align16(oQuads + (uint32_t)G.quadData.size);
    oTexels = align16(oOffsets + (uint32_t)G.quadOffsets.size);
    total = oTexels + (uint32_t)G.texels.size;
    if (instances == -2
        || !ensure_buffer(&G.records, &G.recordsSize, (uint32_t)G.drawRecords.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.views, &G.viewsSize, (uint32_t)G.viewRecords.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.fogs, &G.fogsSize, (uint32_t)G.fogTables.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.shadows, &G.shadowsSize, (uint32_t)G.shadowRecords.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || (G.rowTables.size
            && !ensure_buffer(&G.rows, &G.rowsSize, (uint32_t)G.rowTables.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ))
        || !ensure_buffer(&G.quads, &G.quadsSize, (uint32_t)G.quadData.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.offsets, &G.offsetsSize, (uint32_t)G.quadOffsets.size, SDL_GPU_BUFFERUSAGE_VERTEX)) {
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
    memcpy(mapped + oFogs, G.fogTables.data, G.fogTables.size);
    memcpy(mapped + oShadows, G.shadowRecords.data, G.shadowRecords.size);
    if (G.rowTables.size)
        memcpy(mapped + oRows, G.rowTables.data, G.rowTables.size);
    memcpy(mapped + oQuads, G.quadData.data, G.quadData.size);
    memcpy(mapped + oOffsets, G.quadOffsets.data, G.quadOffsets.size);
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
    copy_into(copy, oFogs, G.fogs, (uint32_t)G.fogTables.size);
    copy_into(copy, oShadows, G.shadows, (uint32_t)G.shadowRecords.size);
    copy_into(copy, oRows, G.rows, (uint32_t)G.rowTables.size);
    copy_into(copy, oQuads, G.quads, (uint32_t)G.quadData.size);
    copy_into(copy, oOffsets, G.offsets, (uint32_t)G.quadOffsets.size);
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

/* The screen's picture, a rectangle of it or the whole without one, downloaded after everything
   recorded before it (render/gfx.h): the frame's work is submitted with the download, and the CPU
   waits for both. */
int gfx_read_frame(const RECT *r, void *out, size_t pitch)
{
    struct flush_extra extra;
    RECT rect;
    if (!G.dev || !G.screen || !G.screen->tex || !out)
        return -1;
    whole_rect(&rect, G.screen);
    if (r) {
        if (r->left < 0 || r->top < 0 || r->right > G.screen->width || r->bottom > G.screen->height
            || r->left >= r->right || r->top >= r->bottom)
            return -1;
        rect = *r;
    }
    if (pitch < (size_t)(rect.right - rect.left) * 4)
        return -1;
    SDL_zero(extra);
    extra.download = G.screen->tex;
    extra.x = rect.left;
    extra.y = rect.top;
    extra.w = rect.right - rect.left;
    extra.h = rect.bottom - rect.top;
    extra.out = out;
    extra.pitch = pitch;
    return flush(&extra) ? 0 : -1;
}

/* ---- the device ---- */

static int choose_vsync(void)
{
    const char *env = SDL_getenv("RAGEXE_VSYNC");
    return !(env && env[0] == '0' && !env[1]);
}

/* The storage buffers a shader stage may read: as many as every Vulkan device takes
   (maxPerStageDescriptorStorageBuffers is 4 at least; Metal takes 31). SDL tells nothing of the
   device's own limits, so a shader that reads more is not made on any device, rather than failing
   on the smallest alone. */
#define STORAGE_BUFFERS_MAX 4

static SDL_GPUShader *make_shader(const struct GpuShaderCode *code)
{
    SDL_GPUShaderCreateInfo info;
    if (code->storageBuffers > STORAGE_BUFFERS_MAX) {
        SDL_SetError("a shader reads %u storage buffers, more than every device takes (%d)", code->storageBuffers,
                     STORAGE_BUFFERS_MAX);
        return NULL;
    }
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

/* custom: whether the GPU rounds floats as render/shaders/exact.hlsli assumes, to nearest, ties to
   even, which Vulkan leaves to the device: sums, differences, products and div_rn's quotients of
   pairs that tell the rounding modes apart (ties, and three quarters of a unit, of either sign),
   drawn as points into a row of float pixels (render/shaders/rounding.hlsl) and compared with the
   CPU's. A GPU that rounds otherwise draws the client's quads all the same, now and then a pixel
   apart from where Direct3D 9 put them: it is said once, when the device is made. */
#define CHECK_WIDTH 64

static void check_rounding(void)
{
    static const uint32_t pairs[][2] = {
        { 0x3f800000u, 0x33800000u }, { 0x3f800001u, 0x33800000u }, { 0x3f800000u, 0x33c00000u },
        { 0xbf800000u, 0xb3c00000u }, { 0x3f800800u, 0x3f800800u }, { 0x3f800001u, 0x3f800001u },
        { 0xbf800001u, 0x3f800001u }, { 0x3f800000u, 0x40400000u }, { 0x40000000u, 0x40400000u },
        { 0x3f800000u, 0x41200000u }, { 0x3f800000u, 0x3f8ccccdu }, { 0x40a00000u, 0x3fb504f3u },
        { 0x3a83126fu, 0x40e00000u }, { 0x47f1206cu, 0x3a83126fu }, { 0xc2f6e979u, 0x3f9d70a4u },
        { 0x3f7fffffu, 0x3f800001u },
    };
    enum { N = sizeof pairs / sizeof pairs[0] };
    float in[N][4], want[N][4];
    SDL_GPUShader *vs = make_shader(&rounding_vs), *fs = make_shader(&rounding_fs);
    SDL_GPUGraphicsPipeline *pipe = NULL;
    SDL_GPUBuffer *cases = NULL;
    SDL_GPUTexture *row = NULL;
    SDL_GPUTransferBuffer *up = NULL, *down = NULL;
    SDL_GPUFence *fence = NULL;
    int i, k, ok = 0, differ = 0;
    for (i = 0; i < N; i++) {
        float a, b;
        memcpy(&a, &pairs[i][0], 4);
        memcpy(&b, &pairs[i][1], 4);
        in[i][0] = a;
        in[i][1] = b;
        in[i][2] = in[i][3] = 0.0f;
        want[i][0] = a + b;
        want[i][1] = a - b;
        want[i][2] = a * b;
        want[i][3] = a / b;
    }
    if (vs && fs) {
        SDL_GPUGraphicsPipelineCreateInfo info;
        SDL_GPUColorTargetDescription colour;
        SDL_zero(info);
        SDL_zero(colour);
        colour.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
        info.vertex_shader = vs;
        info.fragment_shader = fs;
        info.primitive_type = SDL_GPU_PRIMITIVETYPE_POINTLIST;
        info.rasterizer_state.enable_depth_clip = true;
        info.target_info.color_target_descriptions = &colour;
        info.target_info.num_color_targets = 1;
        pipe = SDL_CreateGPUGraphicsPipeline(G.dev, &info);
    }
    {
        SDL_GPUBufferCreateInfo binfo;
        SDL_GPUTextureCreateInfo tinfo;
        SDL_GPUTransferBufferCreateInfo xinfo;
        SDL_zero(binfo);
        binfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
        binfo.size = sizeof in;
        cases = SDL_CreateGPUBuffer(G.dev, &binfo);
        SDL_zero(tinfo);
        tinfo.type = SDL_GPU_TEXTURETYPE_2D;
        tinfo.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
        tinfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        tinfo.width = CHECK_WIDTH;
        tinfo.height = 1;
        tinfo.layer_count_or_depth = 1;
        tinfo.num_levels = 1;
        row = SDL_CreateGPUTexture(G.dev, &tinfo);
        SDL_zero(xinfo);
        xinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        xinfo.size = sizeof in;
        up = SDL_CreateGPUTransferBuffer(G.dev, &xinfo);
        xinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
        xinfo.size = CHECK_WIDTH * 16;
        down = SDL_CreateGPUTransferBuffer(G.dev, &xinfo);
    }
    if (pipe && cases && row && up && down) {
        SDL_GPUCommandBuffer *cmd;
        void *mapped = SDL_MapGPUTransferBuffer(G.dev, up, false);
        if (mapped) {
            memcpy(mapped, in, sizeof in);
            SDL_UnmapGPUTransferBuffer(G.dev, up);
        }
        cmd = mapped ? SDL_AcquireGPUCommandBuffer(G.dev) : NULL;
        if (cmd) {
            SDL_GPUTransferBufferLocation src;
            SDL_GPUBufferRegion dst;
            SDL_GPUColorTargetInfo target;
            SDL_GPUTextureRegion from;
            SDL_GPUTextureTransferInfo to;
            SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
            SDL_GPURenderPass *pass;
            src.transfer_buffer = up;
            src.offset = 0;
            dst.buffer = cases;
            dst.offset = 0;
            dst.size = sizeof in;
            SDL_UploadToGPUBuffer(copy, &src, &dst, false);
            SDL_EndGPUCopyPass(copy);
            SDL_zero(target);
            target.texture = row;
            target.load_op = SDL_GPU_LOADOP_CLEAR;
            target.store_op = SDL_GPU_STOREOP_STORE;
            pass = SDL_BeginGPURenderPass(cmd, &target, 1, NULL);
            SDL_BindGPUGraphicsPipeline(pass, pipe);
            SDL_BindGPUVertexStorageBuffers(pass, 0, &cases, 1);
            SDL_DrawGPUPrimitives(pass, N, 1, 0, 0);
            SDL_EndGPURenderPass(pass);
            copy = SDL_BeginGPUCopyPass(cmd);
            SDL_zero(from);
            SDL_zero(to);
            from.texture = row;
            from.w = CHECK_WIDTH;
            from.h = 1;
            from.d = 1;
            to.transfer_buffer = down;
            SDL_DownloadFromGPUTexture(copy, &from, &to);
            SDL_EndGPUCopyPass(copy);
            fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
        }
    }
    if (fence && SDL_WaitForGPUFences(G.dev, true, &fence, 1)) {
        const float *got = SDL_MapGPUTransferBuffer(G.dev, down, false);
        if (got) {
            ok = 1;
            for (i = 0; i < N; i++)
                for (k = 0; k < 4; k++)
                    differ += memcmp(&got[i * 4 + k], &want[i][k], 4) != 0;
            SDL_UnmapGPUTransferBuffer(G.dev, down);
        }
    }
    if (!ok)
        SDL_Log("gfx: the GPU's rounding could not be checked: %s", SDL_GetError());
    else if (differ)
        SDL_Log("gfx: this GPU does not round floats as the CPU does (%d of %d results differ): sprites, effects "
                "and the interface may land a pixel apart from where Direct3D 9 put them", differ, N * 4);
    if (fence)
        SDL_ReleaseGPUFence(G.dev, fence);
    if (down) SDL_ReleaseGPUTransferBuffer(G.dev, down);
    if (up) SDL_ReleaseGPUTransferBuffer(G.dev, up);
    if (row) SDL_ReleaseGPUTexture(G.dev, row);
    if (cases) SDL_ReleaseGPUBuffer(G.dev, cases);
    if (pipe) SDL_ReleaseGPUGraphicsPipeline(G.dev, pipe);
    if (vs) SDL_ReleaseGPUShader(G.dev, vs);
    if (fs) SDL_ReleaseGPUShader(G.dev, fs);
}

/* What the device starts drawing with (render/gfx.h, gfx_set_mode). */
static void reset_state(void)
{
    G.mode = DRAW_OVERLAY;
    G.effectSrc = DRAW_BLEND_SRCALPHA;
    G.effectDst = DRAW_BLEND_INVSRCALPHA;
    G.effectCombine = DRAW_COMBINE_MODULATE;
    memset(&G.fog, 0, sizeof G.fog);
    resolve_state(&G.state, G.mode, G.effectSrc, G.effectDst, G.effectCombine, 0);
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

/* What never changes, uploaded once: the white texel a stage without a texture samples (and the
   atlas a quad draw binds that reads no layer), a quad's corners and its triangles. */
static int upload_constants(void)
{
    static const unsigned char white[4] = { 0xff, 0xff, 0xff, 0xff };
    SDL_GPUBufferCreateInfo binfo;
    SDL_GPUTransferBufferCreateInfo info;
    SDL_GPUTransferBuffer *buffer;
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    unsigned char *p;
    int ok, i;
    SDL_zero(binfo);
    binfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    binfo.size = sizeof quad_corners;
    G.corners = SDL_CreateGPUBuffer(G.dev, &binfo);
    binfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    binfo.size = sizeof quad_triangles;
    G.triangles = SDL_CreateGPUBuffer(G.dev, &binfo);
    SDL_zero(info);
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    info.size = 64;
    buffer = G.corners && G.triangles ? SDL_CreateGPUTransferBuffer(G.dev, &info) : NULL;
    if (!buffer)
        return 0;
    p = SDL_MapGPUTransferBuffer(G.dev, buffer, false);
    cmd = p ? SDL_AcquireGPUCommandBuffer(G.dev) : NULL;
    if (p) {
        memcpy(p, white, sizeof white);
        memcpy(p + 16, quad_corners, sizeof quad_corners);
        memcpy(p + 32, quad_triangles, sizeof quad_triangles);
        SDL_UnmapGPUTransferBuffer(G.dev, buffer);
    }
    if (cmd) {
        copy = SDL_BeginGPUCopyPass(cmd);
        for (i = 0; i < 2; i++) {
            SDL_GPUTextureTransferInfo src;
            SDL_GPUTextureRegion dst;
            SDL_zero(src);
            SDL_zero(dst);
            src.transfer_buffer = buffer;
            dst.texture = i ? G.noAtlas : G.white;
            dst.w = dst.h = dst.d = 1;
            SDL_UploadToGPUTexture(copy, &src, &dst, false);
        }
        for (i = 0; i < 2; i++) {
            SDL_GPUTransferBufferLocation src;
            SDL_GPUBufferRegion dst;
            src.transfer_buffer = buffer;
            src.offset = i ? 32 : 16;
            dst.buffer = i ? G.triangles : G.corners;
            dst.offset = 0;
            dst.size = i ? (Uint32)sizeof quad_triangles : (Uint32)sizeof quad_corners;
            SDL_UploadToGPUBuffer(copy, &src, &dst, false);
        }
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
    G.vsCode[PROG_QUAD] = &quad_vs;
    G.fsCode[PROG_QUAD] = &quad_fs;
    G.vsCode[PROG_MESH] = &mesh_vs;
    G.fsCode[PROG_MESH] = &mesh_fs;
    G.vsCode[PROG_CASTER] = &caster_vs;
    G.fsCode[PROG_CASTER] = &caster_fs;
    G.vsCode[PROG_ROWS] = &rows_vs;
    G.fsCode[PROG_ROWS] = &rows_fs;
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
    info.format = TARGET_FORMAT;
    G.noAtlas = SDL_CreateGPUTexture(G.dev, &info);
    if (!G.pointClamp || !G.white || !G.noShadow || !G.noAtlas || !make_samplers() || !upload_constants()) {
        SDL_Log("gfx: device resources: %s", SDL_GetError());
        return 0;
    }
    G.frame = 1;
    G.cameraView = G.meshView = G.casterView = G.casterCascade = G.shadowRecord = G.fogTable = G.matrixIndex = -1;
    check_rounding();
    return 1;
}

/* Whether the window takes the device's pictures, claiming it for the device where it can: it is
   claimed, and gets its swapchain, at the first present while it is shown, not minimized and of a
   size in pixels. A window never shown (the scene harness's) is never claimed, so there is no
   swapchain at all, and its frames are drawn and can be read (gfx_read_frame), not shown. A claim
   counts once SDL keeps the window, which it says by knowing its swapchain's format: SDL 3.4's
   Vulkan driver answers true for a window whose surface has no size (a minimized one on Windows)
   but keeps nothing of it. A claim that fails or is not kept is tried again at the next present:
   on Android, the window has no surface while the app is in the background. */
static int claim_window(void)
{
    SDL_GPUPresentMode mode = SDL_GPU_PRESENTMODE_VSYNC;
    int w = 0, h = 0;
    if (G.claimed)
        return 1;
    if (!G.window || (SDL_GetWindowFlags(G.window) & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED))
        || !SDL_GetWindowSizeInPixels(G.window, &w, &h) || w <= 0 || h <= 0)
        return 0;
    if (!SDL_ClaimWindowForGPUDevice(G.dev, G.window)) {
        /* said once, not at every present it is tried again */
        if (!G.claimLogged)
            SDL_Log("gfx: SDL_ClaimWindowForGPUDevice: %s", SDL_GetError());
        G.claimLogged = 1;
        return 0;
    }
    if (SDL_GetGPUSwapchainTextureFormat(G.dev, G.window) == SDL_GPU_TEXTUREFORMAT_INVALID)
        return 0;
    G.claimed = 1;
    G.claimLogged = 0;
    if (!choose_vsync()) {
        if (SDL_WindowSupportsGPUPresentMode(G.dev, G.window, SDL_GPU_PRESENTMODE_IMMEDIATE))
            mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
        else if (SDL_WindowSupportsGPUPresentMode(G.dev, G.window, SDL_GPU_PRESENTMODE_MAILBOX))
            mode = SDL_GPU_PRESENTMODE_MAILBOX;
    }
    SDL_SetGPUSwapchainParameters(G.dev, G.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode);
    return 1;
}

int gfx_open(struct C3dDevice *d)
{
    G.window = d->hwnd;
    if (!G.window)
        return GFX_E_DDRAW_CREATE;
    if (!G.lock)
        G.lock = SDL_CreateMutex();
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

/* The screen: d->target, the one surface the client draws the frame into and the window shows, a
   render target that holds the resolved picture where the screen is multisampled. */
int gfx_create_screen(struct C3dDevice *d)
{
    struct gfx_surface *screen;
    if (!G.window || !create_device())
        return GFX_E_PRIMARY;
    reset_state();
    screen = new_handle(K_SCREEN, d->width, d->height, GFX_FMT_8888);
    if (!screen)
        return GFX_E_PRIMARY;
    G.screen = screen;
    G.samples = SDL_GPU_SAMPLECOUNT_1;
    screen->tex = new_target_texture(d->width, d->height);
    if (!screen->tex)
        return GFX_E_PRIMARY;
    d->target = screen;
    G.target = screen;
    d->bpp = 32;
    d->rMask = 0xff0000;
    d->gMask = 0xff00;
    d->bMask = 0xff;
    d->dev = G.dev;
    return 0;
}

int gfx_create_zbuffer(struct C3dDevice *d)
{
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
    /* the screen's samples and depth are known: every pipeline can be made */
    return make_pipelines() ? 0 : GFX_E_ZBUFFER;
}

int gfx_get_caps(struct gfx_caps *out)
{
    if (!G.dev)
        return -1;
    memset(out, 0, sizeof *out);
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
    return 0;
}

int gfx_close(struct C3dDevice *d)
{
    struct gfx_surface *s, *next;
    int i;
    if (G.dev)
        flush_only();
    if (d->target) { gfx_release(d->target); d->target = NULL; }
    if (d->save)   { gfx_release(d->save);   d->save = NULL; }
    G.target = G.screen = NULL;
    if (G.dev) {
        wait_frames(1);
        SDL_WaitForGPUIdle(G.dev);
        lock_handles();
        for (s = G.handles; s; s = next) {
            next = s->next;
            if (s->tex)
                SDL_ReleaseGPUTexture(G.dev, s->tex);
            s->tex = NULL;
            s->array = NULL;
            s->failed = 0;
            s->dirty = 1;
            whole_rect(&s->dirtyRect, s);
        }
        while (G.arrays) {
            struct gpu_array *a = G.arrays;
            G.arrays = a->next;
            SDL_ReleaseGPUTexture(G.dev, a->tex);
            free(a->taken);
            free(a);
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
        for (i = 0; i < G.npipelines; i++)
            SDL_ReleaseGPUGraphicsPipeline(G.dev, G.pipelines[i].pipeline);
        for (i = 0; i < 6; i++)
            if (G.samplers[i / 3][i % 3])
                SDL_ReleaseGPUSampler(G.dev, G.samplers[i / 3][i % 3]);
        for (i = 0; i < PROG_COUNT; i++) {
            if (G.vs[i])
                SDL_ReleaseGPUShader(G.dev, G.vs[i]);
            if (G.fs[i])
                SDL_ReleaseGPUShader(G.dev, G.fs[i]);
        }
        if (G.pointClamp) SDL_ReleaseGPUSampler(G.dev, G.pointClamp);
        if (G.white) SDL_ReleaseGPUTexture(G.dev, G.white);
        if (G.noAtlas) SDL_ReleaseGPUTexture(G.dev, G.noAtlas);
        if (G.noShadow) SDL_ReleaseGPUTexture(G.dev, G.noShadow);
        if (G.shadowMaps) SDL_ReleaseGPUTexture(G.dev, G.shadowMaps);
        if (G.shadowDepth) SDL_ReleaseGPUTexture(G.dev, G.shadowDepth);
        if (G.depth) SDL_ReleaseGPUTexture(G.dev, G.depth);
        if (G.msaa) SDL_ReleaseGPUTexture(G.dev, G.msaa);
        if (G.records) SDL_ReleaseGPUBuffer(G.dev, G.records);
        if (G.views) SDL_ReleaseGPUBuffer(G.dev, G.views);
        if (G.fogs) SDL_ReleaseGPUBuffer(G.dev, G.fogs);
        if (G.shadows) SDL_ReleaseGPUBuffer(G.dev, G.shadows);
        if (G.rows) SDL_ReleaseGPUBuffer(G.dev, G.rows);
        if (G.frameCopy) SDL_ReleaseGPUTexture(G.dev, G.frameCopy);
        if (G.quads) SDL_ReleaseGPUBuffer(G.dev, G.quads);
        if (G.offsets) SDL_ReleaseGPUBuffer(G.dev, G.offsets);
        if (G.corners) SDL_ReleaseGPUBuffer(G.dev, G.corners);
        if (G.triangles) SDL_ReleaseGPUBuffer(G.dev, G.triangles);
        if (G.instances) SDL_ReleaseGPUBuffer(G.dev, G.instances);
        if (G.upload) SDL_ReleaseGPUTransferBuffer(G.dev, G.upload);
        if (G.download) SDL_ReleaseGPUTransferBuffer(G.dev, G.download);
        if (G.claimed)
            SDL_ReleaseWindowFromGPUDevice(G.dev, G.window);
        SDL_DestroyGPUDevice(G.dev);
    }
    G.dev = NULL;
    G.claimed = G.claimLogged = 0;
    memset(G.pipelines, 0, sizeof G.pipelines);
    G.npipelines = 0;
    memset(G.quadPipeline, 0, sizeof G.quadPipeline);
    memset(G.meshPipeline, 0, sizeof G.meshPipeline);
    memset(G.effectPipeline, 0, sizeof G.effectPipeline);
    G.casterPipeline = G.rowsPipeline = NULL;
    memset(G.samplers, 0, sizeof G.samplers);
    memset(G.vs, 0, sizeof G.vs);
    memset(G.fs, 0, sizeof G.fs);
    G.pointClamp = NULL;
    G.white = G.noAtlas = G.noShadow = G.shadowMaps = G.shadowDepth = G.depth = G.msaa = G.frameCopy = NULL;
    G.frameCopyW = G.frameCopyH = 0;
    G.records = G.views = G.fogs = G.shadows = G.rows = G.quads = G.offsets = NULL;
    G.instances = NULL;
    G.corners = G.triangles = NULL;
    G.recordsSize = G.viewsSize = G.fogsSize = G.shadowsSize = G.rowsSize = G.quadsSize = G.offsetsSize = 0;
    G.instanceCount = 0;
    G.upload = G.download = NULL;
    G.uploadSize = G.downloadSize = 0;
    G.shadowState = 0;
    d->dev = NULL;
    return 0;
}

/* The present mode follows RAGEXE_VSYNC (claim_window). Without vsync the frame never waits for
   the window: it is drawn but not shown unless a refresh has passed since the last picture
   (present_into). A window not claimed shows nothing; the frame is drawn all the same. */
int gfx_present(int vsync)
{
    struct flush_extra extra;

    if (!G.dev || !G.screen)
        return -1;
    SDL_zero(extra);
    extra.present = claim_window();
    extra.wait = vsync;
    flush(&extra);
    return 0;
}

/* custom: the screen, its multisampled picture and depth at w by h, for a window resized to that;
   the swapchain follows the window by itself. Without memory for them it answers -1 and keeps the
   old ones. */
int gfx_resize(int w, int h)
{
    SDL_GPUTexture *screen, *msaa = NULL, *depth = NULL;
    if (!G.dev || !G.screen || w <= 0 || h <= 0)
        return -1;
    flush_only();
    screen = new_target_texture(w, h);
    if (G.msaa)
        msaa = new_samples_texture(w, h, 0);
    if (G.depth)
        depth = new_samples_texture(w, h, 1);
    if (!screen || (G.msaa && !msaa) || (G.depth && !depth)) {
        SDL_GPUTexture *made[3];
        int i;
        made[0] = screen;
        made[1] = msaa;
        made[2] = depth;
        for (i = 0; i < 3; i++)
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
    G.screen->version = next_version();
    G.screen->width = w;
    G.screen->height = h;
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

/* Touches no GPU object, as gfx_create_texture; the draws that place it (place_layer) and its
   release run on the thread that draws. */
gfx_surface *gfx_create_layer_texture(int w, int h, int fmt)
{
    struct gfx_surface *s = gfx_create_texture(w, h, fmt);
    if (s)
        s->layered = 1;
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

/* A rectangle of a texture's or a system surface's CPU pixels, the whole of them without one; a
   target has none. A rectangle that is empty or reaches outside the surface locks nothing. */
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
    if (!s->pixels)
        return -1;
    bytes = s->kind == K_SYSMEM ? 4 : texel_bytes(s->fmt);
    s->lockRect = rect;
    out->bits = s->pixels + (size_t)rect.top * s->width * bytes + (size_t)rect.left * bytes;
    out->pitch = s->width * bytes;
    out->width = s->width;
    out->height = s->height;
    out->bpp = bytes * 8;
    return 0;
}

/* The multisampled screen takes the resolved picture after a copy changed it. */
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

int gfx_unlock(gfx_surface *s)
{
    s->version = next_version();
    if (s->pixels)
        mark_dirty(s, &s->lockRect);
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
int gfx_blit(gfx_surface *dst, const RECT *dr, gfx_surface *src, const RECT *sr)
{
    RECT d, s;
    batch_flush();
    g_perf.blits++;
    dst->version = next_version();
    if (dst == src)
        return 0;
    if (!blit_rects(dst, dr, src, sr, &d, &s))
        return 0;
    if (is_target(src)) {
        /* from a target into a target: a copy on the GPU; a target's pixels never go to the CPU */
        struct gpu_step *step;
        if (!G.dev || !is_target(dst) || !src->tex || !dst->tex)
            return -1;
        pin(src);
        pin(dst);
        step = add_step(STEP_COPY);
        if (!step)
            return -1;
        step->src = src->tex;
        step->dst = dst->tex;
        step->srcRect = s;
        step->dstRect = d;
        if (dst == G.screen)
            unresolve();
        return 0;
    }
    if (is_target(dst)) {
        if (!src->pixels)
            return -1;
        draw_textured_quad(dst, &d, src, &s);
        return 0;
    }
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
        if (!clip_rect(&rc, t))
            return 0;
    } else {
        whole_rect(&rc, t);
    }
    depth = (flags & GFX_CLEAR_ZBUFFER) && pass_has_depth(pass);
    whole = rc.left == 0 && rc.top == 0 && rc.right == t->width && rc.bottom == t->height;
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
        /* a clear inside a pass: a quad over the rectangle that writes the colour, the depth or
           both */
        struct TexDrawVertex v[4];
        struct draw_state state;
        int mode = !(flags & GFX_CLEAR_TARGET) ? DRAW_CLEAR_DEPTH : depth ? DRAW_CLEAR : DRAW_CLEAR_COLOUR;
        int i;
        for (i = 0; i < 4; i++) {
            v[i].x = (float)((i & 1) ? rc.right : rc.left) - 0.5f;
            v[i].y = (float)((i & 2) ? rc.bottom : rc.top) - 0.5f;
            v[i].z = z;
            v[i].rhw = 1.0f;
            v[i].diffuse = colour;
            v[i].u = v[i].v = 0.0f;
        }
        resolve_state(&state, mode, 0, 0, 0, 0);
        own_quad(mode, &state, NULL, v);
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
    if (!G.dev || !s || !is_target(s) || !s->tex)
        return -1;
    G.target = s;
    return 0;
}

/* The quads given since then read the old texture still; the next ones join their batch while they
   read the same textures (batch_join). */
void gfx_set_texture(int stage, gfx_surface *s)
{
    if ((unsigned)stage >= 2 || G.stageTex[stage] == s)
        return;
    g_perf.texSets++;
    G.stageTex[stage] = s;
}

/* The texture the wobble copies the screen's picture into, at the screen's size; one of another
   size goes after the frames that may use it. */
static int frame_copy_ready(void)
{
    if (G.frameCopy && G.frameCopyW == G.screen->width && G.frameCopyH == G.screen->height)
        return 1;
    retire_texture(G.frameCopy);
    G.frameCopy = new_texture(G.screen->width, G.screen->height, 1, SDL_GPU_TEXTUREUSAGE_SAMPLER);
    G.frameCopyW = G.frameCopy ? G.screen->width : 0;
    G.frameCopyH = G.frameCopy ? G.screen->height : 0;
    return G.frameCopy != NULL;
}

/* A copy of the screen's picture (resolved, when multisampled, at the end of the pass before), then
   a pass of its own over the whole screen that draws it back with the rows moved, into the
   multisampled picture as well. Everything that can fail is in place before `fill` is called, and
   is taken back where something failed. */
int gfx_wobble(const struct WobbleRow *rows, int count, void (*fill)(void *context), void *context)
{
    struct gfx_surface *screen = G.screen;
    size_t nsteps, ndraws, nrecords, nrows;
    struct gpu_step *copy, *pass;
    struct gpu_draw *d;
    struct GpuDraw *r = NULL;
    uint32_t *table;
    long at;
    int y;
    batch_flush();
    if (!G.dev || !screen || !screen->tex || G.shadowPass || !G.rowsPipeline || !rows || count <= 0 || !fill
        || !frame_copy_ready())
        return 0;
    /* what the frame holds with the draws batched so far, to take back to */
    nsteps = G.nsteps;
    ndraws = G.ndraws;
    nrecords = G.drawRecords.size;
    nrows = G.rowTables.size;
    at = bytes_add(&G.rowTables, (size_t)screen->height * sizeof *table, 16);
    copy = at >= 0 ? add_step(STEP_COPY) : NULL;
    pass = copy ? add_step(STEP_PASS) : NULL;
    d = pass ? add_draw(pass) : NULL;
    if (d)
        d->record = add_record(&r);
    if (!r) {
        G.nsteps = nsteps;
        G.ndraws = ndraws;
        G.drawRecords.size = nrecords;
        G.rowTables.size = nrows;
        return 0;
    }
    /* add_step may have moved the steps */
    copy = &G.steps[nsteps];
    pin(screen);
    copy->src = screen->tex;
    copy->dst = G.frameCopy;
    whole_rect(&copy->srcRect, screen);
    copy->dstRect = copy->srcRect;
    pass->target = screen;
    pass->whole = 1;
    pass->loadColour = SDL_GPU_LOADOP_LOAD;
    pass->loadDepth = SDL_GPU_LOADOP_LOAD;
    pass->first = (uint32_t)ndraws;
    d->program = PROG_ROWS;
    d->pipeline = G.rowsPipeline;
    d->bind[0].texture = G.frameCopy;
    d->bind[0].sampler = G.pointClamp;
    d->count = 3;
    r->mode[3] = (uint32_t)(at / (long)sizeof *table);
    fill(context);
    table = (uint32_t *)(G.rowTables.data + at);
    for (y = 0; y < screen->height; y++) {
        int source = y, shift = 0;
        if (y < count && rows[y].source >= 0 && rows[y].source < screen->height && rows[y].shift >= 0) {
            source = rows[y].source;
            shift = rows[y].shift < screen->width ? rows[y].shift : screen->width;
        }
        table[y] = (uint32_t)source | (uint32_t)shift << 16;
    }
    screen->version = next_version();
    return 1;
}

unsigned gfx_content_version(gfx_surface *s)
{
    return s ? s->version : 0;
}

struct gfx_caps g_gfxCaps;
