/* render/gfx.h on SDL 3's GPU API: Vulkan with SPIR-V shaders on Windows and Linux, Metal with
   MSL on macOS, one shader format handed to SDL_CreateGPUDevice so that SDL has one driver to
   choose. The shaders are HLSL compiled offline (src/render/shaders, tools/shaders).

   Draws are made in modes (render/draw_modes.h): the pipelines of every mode, for every kind of
   target it draws into, are made once, when the screen's samples and depth are known
   (gfx_create_zbuffer), and so are the samplers. A frame is
   recorded, not drawn: each draw becomes a record of what its mode and the draw itself give the
   shaders (struct GpuDraw, read from a storage buffer through an instance-rate attribute that holds
   its index) in a list of passes, and its vertices, indices and the texels of the textures it uses
   since they changed go into one staging area. When the frame is presented, or the CPU needs a
   target's pixels, one command buffer copies all of that through one transfer buffer in one copy
   pass and replays the passes. Textures keep their pixels on the CPU in the client's own format, so
   that the model loader's thread creates, locks and fills them without calling SDL; the main thread
   widens them to B8G8R8A8 when a draw first uses them after a change. Render targets live on the
   GPU; locking one downloads it.

   custom: the static world (gfx_world_*) stays on the GPU from frame to frame: its meshes in shared
   vertex and index buffers, its textures in texture arrays, its draw records, transform slots and
   cells of water. Its meshes and textures go up through a stream of their own as they arrive, a
   bounded transfer buffer at a time (world_stream); its records, slots and cells with the frame's
   upload. After the copy pass three compute passes compute the slots' views (world_transform.hlsl),
   cull the records for the view, giving each run's kept records their places in the frame's index
   buffer of them and writing the run's one indirect draw (world_cull.hlsl), and copy the kept
   records' indices to their places, 64 at a time (world_compact.hlsl); the main pass draws the
   ground and the opaque models with those draws. */
#include "render/gfx.h"
#include "render/gfx_batch.h"
#include "render/device.h"
#include "render/present_fit.h"
#include "render/texel_convert.h"
#include "render/vertex_fog.h"
#include "render/world_layout.h"
#include "render/shaders/shader_code.h"
#include "render/shaders/quad.h"
#include "render/shaders/world.h"
#include "render/shaders/world_caster.h"
#include "render/shaders/world_transform.h"
#include "render/shaders/world_cull.h"
#include "render/shaders/world_compact.h"
#include "render/shaders/water.h"
#include "diag/perf.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { K_SCREEN, K_TARGET, K_SAVE, K_SYSMEM, K_TEXTURE, K_RTTEX, K_ZBUFFER };

#define CLIENT_SURFACE_KEY 0x00ff00ffu
#define FLAT_KIND 0x100
/* the frame's water instances: a cell and its colour (water.hlsl, WaterIn) */
#define WATER_INSTANCE 8
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
    float uvScale[4], uvLimit[4];
    uint32_t mesh[4];
};

struct GpuView {
    float toClip[16];
};

struct GpuShadow {
    float fromView[GFX_SHADOW_CASCADES][16];
    float farDepth[4], offset[4], blur[4], slant[4];
    float lightView[4];  /* w: the bias */
    float ambient[4];    /* w: the darkness */
    float mapSize[4];    /* x: the maps' size */
};

enum { KIND_SCREEN, KIND_WORLD, KIND_MESH };
enum { PROG_QUAD, PROG_WORLD, PROG_CASTER, PROG_WATER, PROG_COUNT };
/* The cull modes of the pipelines: Direct3D 9's, which culls by the winding on the screen, y down:
   CULL_CW drops the clockwise triangles, CULL_CCW the others. */
enum { CULL_NONE = 1, CULL_CW, CULL_CCW };
enum { MASK_RGBA, MASK_RGB, MASK_NONE };
/* What a pass draws into, as far as its pipelines care: the screen (multisampled, with depth), its
   copy, and a render target's texture (one sample each, without depth) (render/draw_modes.h,
   DRAW_INTO_*). */
enum { CLASS_SCREEN, CLASS_COPY, CLASS_TEXTURE, CLASS_COUNT };
/* The vertices of every quad in the frame's vertex data: GFX_VTX_TEX2's, one texture's
   coordinates given twice where a draw has only those. */
#define QUAD_STRIDE GFX_VTX_TEX2_SIZE

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

/* One recorded draw: of the frame's quads (PROG_QUAD), of the world's records, one (indexed) or a
   run's indirect command, firstIndex (indirect), of its casters, or of its water's cells
   (PROG_WATER, `instances` of them from firstInstance of the frame's); a world's draw is of the
   world of `generation` (gfx_world_generation). */
struct gpu_draw {
    SDL_GPUGraphicsPipeline *pipeline;
    int program;
    SDL_GPUTextureSamplerBinding bind[3];
    uint32_t vertexOffset;  /* bytes into the frame's vertices */
    uint32_t firstIndex, count;
    int indexed, indirect;
    int32_t baseVertex;
    uint32_t instances, firstInstance;
    uint32_t record;
    uint32_t uniform[4];    /* what the world's programs take as their uniform */
    int generation;
    /* the world's programs: the texture arrays bind[0] and bind[1] read (SW_NO_ARRAY: white), whose
       textures run_pass takes as they are once the flush has made and grown them (world_array) */
    uint8_t arrays[2];
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
    int cull;             /* CULL_* */
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
    int claimed;
    SDL_ThreadID mainThread;
    SDL_Mutex *lock;
    SDL_AtomicInt contentTick;
    struct gfx_surface *handles;
    SDL_GPUTexture **deadTextures;
    size_t deadCount, deadCapacity;

    /* the main target: screen holds the picture (resolved when multisampled) */
    struct gfx_surface *screen, *mainTarget, *target;
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
    struct GfxWorldView worldCamera;
    int worldCameraSet;

    /* shadows */
    int shadowState, shadowPass, shadowReady, shadowReceive, shadowCascade;
    struct GfxShadow shadow;
    int shadowRecord;  /* in this frame's records, or -1 */
    SDL_GPUTexture *shadowMaps, *shadowDepth, *noShadow;

    /* fixed resources */
    SDL_GPUShader *vs[PROG_COUNT], *fs[PROG_COUNT];
    const struct GpuShaderCode *vsCode[PROG_COUNT], *fsCode[PROG_COUNT];
    /* every pipeline, made once (make_pipelines), and which serves what: a mode's quads into each
       kind of target, flat or not; its world records by cull mode (CULL_* - 1) and its water; the
       effect modes' quads by depth test or none and blend factors; the shadow casters */
    struct { struct pipe_desc desc; SDL_GPUGraphicsPipeline *pipeline; } pipelines[PIPELINE_MAX];
    int npipelines;
    SDL_GPUGraphicsPipeline *quadPipeline[DRAW_MODE_COUNT][CLASS_COUNT][2];
    SDL_GPUGraphicsPipeline *worldPipeline[DRAW_MODE_COUNT][3];
    SDL_GPUGraphicsPipeline *waterPipeline[DRAW_MODE_COUNT];
    SDL_GPUGraphicsPipeline *effectPipeline[2][DRAW_FACTOR_COUNT][DRAW_FACTOR_COUNT];
    SDL_GPUGraphicsPipeline *casterPipeline;
    SDL_GPUSampler *samplers[2][3];  /* wrapped or not, by enum DrawFilter */
    SDL_GPUSampler *pointClamp;
    SDL_GPUTexture *white;
    SDL_GPUBuffer *records, *views, *fogs, *shadows, *vertices, *indices, *instances;
    uint32_t recordsSize, viewsSize, fogsSize, shadowsSize, verticesSize, indicesSize, instanceCount;
    SDL_GPUTransferBuffer *upload, *download;
    /* the frames the GPU may still be drawing: at most FRAMES_IN_FLIGHT, also where no swapchain
       paces them (a hidden window), so that the CPU does not run ahead and the cycled buffers do
       not pile up */
    SDL_GPUFence *inFlight[FRAMES_IN_FLIGHT];
    int inFlightNext;
    Uint64 presentedNs;  /* when the window last took a picture */
    uint32_t uploadSize, downloadSize;
    int stagingQuiet;  /* flushes in a row that needed a quarter of the transfer buffer at most */

    /* the frame being recorded */
    unsigned frame;
    struct gpu_step *steps;
    size_t nsteps, stepCapacity;
    struct gpu_draw *draws;
    size_t ndraws, drawCapacity;
    struct gpu_upload *uploads;
    size_t nuploads, uploadCapacity, frameUploads;  /* frameUploads: those of the first copy pass */
    struct bytes drawRecords, viewRecords, fogTables, shadowRecords, vertexData, indexData, texels;
    struct gfx_surface **pins;
    size_t npins, pinCapacity;
    int cameraView;
    int fogTable;     /* the fog's table (render/vertex_fog.h) in this frame's, or -1 */
    float fogBase, fogDirection, fogCount;
    int unresolving;  /* the draw being recorded puts the resolved screen back into the multisampled one */
    int viewsTargetW, viewsTargetH;
} G;

/* custom: the static world (render/gfx.h, gfx_world_*), which stays from frame to frame: its shared
   buffers, texture arrays, records and transform slots, and what the frame gives of it. */
struct world_array {
    SDL_GPUTexture *tex;
    uint32_t width, height, levels;
    uint32_t layers;    /* placed */
    uint32_t capacity;  /* made */
    uint32_t uploaded;  /* gone up, which a larger array made in its place takes over */
    int failed;         /* could not be made: its textures draw white */
};

/* what goes up with the next stream (world_stream): a mesh staged in W.meshes at `at` (a: its base
   vertex, b its vertices, c its first index, d its indices), or a texture's top level from its
   surface, held until then (a: its array, b its layer) */
struct world_upload {
    struct gfx_surface *surface;
    uint32_t at;
    uint32_t a, b, c, d;
};

/* a buffer or an array given up for a larger one, whose contents the next stream moves over */
struct world_copy {
    SDL_GPUBuffer *from, *to;
    uint32_t size;
};

struct world_array_copy {
    SDL_GPUTexture *from, *to;
    uint32_t layers, array;
};

struct GpuWaterCell;

static struct {
    int generation;
    /* uploads that failed one after the other (world_failed), and the world given up after them */
    int failures, off;
    /* the shared buffers, the vertices and indices placed and those gone up */
    SDL_GPUBuffer *vertices, *ids, *indices;
    uint32_t verticesSize, idsSize, indicesSize;
    uint32_t vertexCount, indexCount, uploadedVertices, uploadedIndices;
    /* what the next stream takes up, and its transfer buffer and the fence of its last batch */
    struct world_upload *meshUploads, *textureUploads;
    size_t nmeshUploads, meshUploadCapacity, ntextureUploads, textureUploadCapacity;
    struct bytes meshes;
    SDL_GPUTransferBuffer *stream;
    uint32_t streamSize;
    SDL_GPUFence *streamFence;
    struct world_copy *copies;
    size_t ncopies, copyCapacity;
    struct world_array_copy *arrayCopies;
    size_t narrayCopies, arrayCopyCapacity;
    struct world_array arrays[SW_NO_ARRAY];
    size_t narrays;
    SDL_GPUTexture *white;  /* the array of a record without a texture: one white texel */
    int whiteDue;
    uint32_t whiteAt;
    /* the records by id and in draw order, their runs, and the runs drawn indirectly (the first
       nindirect), whose kept records the cull gives their places in `drawn` (`places`), and whose
       kept indices the compaction copies there by chunk, drawnIndices of them at most */
    struct WlRecord *byId;
    size_t byIdCapacity;
    struct WlCullRecord *cull;
    size_t cullCapacity;
    struct SwDrawRun *runs;
    size_t nruns, runCapacity;
    struct WlRun *indirect;
    size_t nindirect, indirectCapacity;
    struct WlChunk *chunks;
    size_t nchunks, chunkCapacity;
    uint32_t recordCount, idCount, recordsAt, drawnIndices;
    int recordsChanged;
    SDL_GPUBuffer *recordBuffer, *cullBuffer, *runBuffer, *chunkBuffer, *places, *drawn, *commands;
    uint32_t recordBufferSize, cullBufferSize, runBufferSize, chunkBufferSize, placesSize, drawnSize, commandsSize;
    /* the transform slots, and their views, places and casters' views for the frame */
    struct WlSlot *slots;
    size_t slotCapacity;
    uint32_t slotCount, slotsAt;
    int slotsChanged;
    SDL_GPUBuffer *slotBuffer, *views, *placed, *casterViews;
    uint32_t slotBufferSize, viewsSize, placedSize, casterViewsSize;
    /* the water */
    struct GpuWaterCell *cells;
    size_t cellCapacity;
    uint32_t cellCount, cellsAt;
    int cellsChanged;
    float sines[360];
    SDL_GPUBuffer *cellBuffer, *sineBuffer, *waveBuffer, *instanceBuffer;
    uint32_t cellBufferSize, sineSize, waveSize, instanceSize;
    /* the frame: given (gfx_world_frame) until it is presented; its compute passes due, and run in
       this flush */
    struct GfxWorldView camera;
    struct SwFrustum frustum;
    int frameSet, frameDue, computeDue, computed, casterRecord;
    uint32_t frameAt, waveCount;
    struct bytes animated, uvs, waves, instances;
    SDL_GPUBuffer *animatedBuffer, *uvBuffer;
    uint32_t animatedSize, uvSize;
    SDL_GPUComputePipeline *transform, *cullPipeline, *compactPipeline;
} W;

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
    for (i = 0; i < 2; i++)
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
    SDL_zero(info);
    SDL_zero(colour);
    SDL_zero(buffers);
    SDL_zero(attrs);
    /* the quads: their vertices, then the draw's record per instance (instances_needed); the
       world's records and casters: their vertices, then each vertex's record id; the water: a cell
       and its colour per instance */
    buffers[0].slot = 0;
    buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    buffers[1].slot = 1;
    buffers[1].pitch = 4;
    buffers[1].input_rate = p->program == PROG_QUAD ? SDL_GPU_VERTEXINPUTRATE_INSTANCE : SDL_GPU_VERTEXINPUTRATE_VERTEX;
#define ATTR(loc, slot, fmt, off) (attrs[nattrs].location = (loc), attrs[nattrs].buffer_slot = (slot), \
                                   attrs[nattrs].format = (fmt), attrs[nattrs].offset = (off), nattrs++)
    if (p->program == PROG_QUAD) {
        buffers[0].pitch = QUAD_STRIDE;
        ATTR(0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 0);
        ATTR(1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 16);
        ATTR(2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 20);
        ATTR(3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 28);
        ATTR(4, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
    } else if (p->program == PROG_WORLD) {
        buffers[0].pitch = sizeof(struct GfxMeshVertex);
        ATTR(0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
        ATTR(1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 12);
        ATTR(2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 24);
        ATTR(3, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 28);
        ATTR(4, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 32);
        ATTR(5, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 40);
        ATTR(6, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
    } else if (p->program == PROG_CASTER) {
        buffers[0].pitch = sizeof(struct GfxMeshVertex);
        ATTR(0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
        ATTR(1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 32);
        ATTR(2, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
    } else {
        buffers[0].pitch = WATER_INSTANCE;
        buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
        ATTR(0, 0, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
        ATTR(1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 4);
    }
#undef ATTR
    info.vertex_shader = G.vs[p->program];
    info.fragment_shader = G.fs[p->program];
    info.vertex_input_state.vertex_buffer_descriptions = buffers;
    info.vertex_input_state.num_vertex_buffers = p->program == PROG_WATER ? 1 : 2;
    info.vertex_input_state.vertex_attributes = attrs;
    info.vertex_input_state.num_vertex_attributes = nattrs;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    /* Direct3D 9 culls by the winding on the screen, y down, as the target's pixels are laid out
       here too: CULL_CW drops the clockwise triangles, CULL_CCW the others */
    info.rasterizer_state.cull_mode = p->cull == CULL_NONE ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK;
    info.rasterizer_state.front_face = p->cull == CULL_CW ? SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
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
    p->cull = CULL_NONE;
    /* the screen and its copy are X8R8G8B8 on Direct3D 9: their alpha stays 1 */
    p->mask = !m->colour ? MASK_NONE : cls == CLASS_TEXTURE ? MASK_RGBA : MASK_RGB;
    p->hasDepth = cls == CLASS_SCREEN && G.depth != NULL;
    p->samples = cls == CLASS_SCREEN ? (int)G.samples : (int)SDL_GPU_SAMPLECOUNT_1;
    p->a2c = alpha_to_coverage(m, p->samples, flat);
}

/* Every pipeline the modes draw with, once the screen's samples and depth are known: a mode's
   quads into each kind of target it draws into, flat and not, its world records in each cull mode
   and its water, the effect modes' quads by every pair of the blend factors the screen has, and the
   shadow casters'. */
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
        /* the water is drawn among the quads of the alpha lists, which draw meshes, in world
           coordinates */
        if ((m->into & DRAW_INTO_SCREEN) && (m->into & DRAW_MESHES)) {
            mode_pipe(&p, PROG_WATER, m, CLASS_SCREEN, 0, 0, 0);
            if (!(G.waterPipeline[mode] = pipeline(&p)))
                return 0;
        }
        if (m->into & DRAW_MESHES)
            for (cull = CULL_NONE; cull <= CULL_CCW; cull++) {
                mode_pipe(&p, PROG_WORLD, m, CLASS_SCREEN, 0, 0, 0);
                p.cull = cull;
                if (!(G.worldPipeline[mode][cull - CULL_NONE] = pipeline(&p)))
                    return 0;
            }
    }
    memset(&p, 0, sizeof p);
    p.program = PROG_CASTER;
    p.depth = DRAW_DEPTH_WRITE;
    p.cull = CULL_NONE;
    p.r32f = 1;
    p.hasDepth = 1;
    p.samples = SDL_GPU_SAMPLECOUNT_1;
    G.casterPipeline = pipeline(&p);
    return G.casterPipeline != NULL;
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
        G.cameraView = -1;
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
    const struct gfx_surface *g = is_target(s) ? gpu_of((struct gfx_surface *)s) : NULL;
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

/* What a draw record holds of the state it is made with, its stages' textures and its pass. */
static void fill_state(struct GpuDraw *r, const struct gpu_step *pass, const struct draw_state *s,
                       struct gfx_surface *const *tex, int kind, int a2c)
{
    int i;
    float w = 1.0f, h = 1.0f;
    struct gfx_surface *t = pass->target;
    if (t) {
        w = (float)t->width;
        h = (float)t->height;
    }
    r->mode[0] = (uint32_t)kind;
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
    if (s->fog && kind != KIND_MESH) {
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

static void multiply(float *out, const float *a, const float *b)
{
    float r[16];
    int i, j;
    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + a[i * 4 + 3] * b[12 + j];
    memcpy(out, r, sizeof r);
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

static int add_view(const float *toClip)
{
    struct GpuView *view;
    long at = bytes_add(&G.viewRecords, sizeof *view, 16);
    if (at < 0)
        return 0;
    view = (struct GpuView *)(G.viewRecords.data + at);
    memcpy(view->toClip, toClip, sizeof view->toClip);
    return (int)(at / (long)sizeof *view);
}

/* The view record of a world view in a target's pixels, made once per frame and target size. */
static int view_for(const struct GfxWorldView *v, int *cache, const struct gpu_step *pass)
{
    float proj[16], clip[16];
    int w = pass->target ? pass->target->width : 1, h = pass->target ? pass->target->height : 1;
    if (w != G.viewsTargetW || h != G.viewsTargetH) {
        G.cameraView = -1;
        G.viewsTargetW = w;
        G.viewsTargetH = h;
    }
    if (*cache >= 0)
        return *cache;
    wl_projection(proj, v->proj, v->centre, v->nearZ, v->depthScale, w, h);
    wl_view_clip(clip, v->view, proj);
    *cache = add_view(clip);
    return *cache;
}

/* ---- client draws ---- */

static void bind_stage(SDL_GPUTextureSamplerBinding *bind, const struct draw_state *s, struct gfx_surface *surface,
                       int stage)
{
    SDL_GPUTexture *tex = texture_for_draw(surface);
    bind->texture = tex ? tex : G.white;
    bind->sampler = stage_sampler(s, stage);
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

/* The values a quad vertex carries besides its position (QUAD_STRIDE): z, the colour's bytes and
   the two coordinate pairs. */
#define QUAD_VALUES 9

/* Direct3D 9 drew flat draws without multisampling: every sample of a pixel was covered when its
   centre was. In a multisampled pass a flat draw's vertices move onto the pixel boundaries that
   keep that coverage (pixel_boundary), and take the values their triangle's plane has there (a
   flat triangle's values are affine on the screen), so each pixel centre reads what it read before:
   exactly so for edges along the pixel rows and columns, which the UI's and the sprites' are. The
   vertices are QUAD_STRIDE apart, the triangles a list. */
static void snap_flat(unsigned char *verts, int nverts, const WORD *indices, uint32_t count)
{
    static unsigned char *orig, *done;
    static size_t capacity;
    uint32_t t;
    size_t bytes = (size_t)nverts * QUAD_STRIDE;
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
    for (t = 0; t + 3 <= count; t += 3) {
        int k[3], i, j;
        float px[3], py[3], value[3][QUAD_VALUES], dx1, dy1, dx2, dy2, det;
        for (i = 0; i < 3; i++) {
            k[i] = indices ? indices[t + i] : (int)(t + i);
            if (k[i] >= nverts)
                break;
        }
        if (i < 3 || (done[k[0]] && done[k[1]] && done[k[2]]))
            continue;
        for (i = 0; i < 3; i++) {
            const unsigned char *v = orig + (size_t)k[i] * QUAD_STRIDE;
            px[i] = read_float(v);
            py[i] = read_float(v + 4);
            value[i][0] = read_float(v + 8);
            for (j = 0; j < 4; j++)
                value[i][1 + j] = v[16 + j];
            for (j = 0; j < 4; j++)
                value[i][5 + j] = read_float(v + 20 + j * 4);
        }
        dx1 = px[1] - px[0];
        dy1 = py[1] - py[0];
        dx2 = px[2] - px[0];
        dy2 = py[2] - py[0];
        det = dx1 * dy2 - dx2 * dy1;
        if (det > -1e-6f && det < 1e-6f)
            continue;
        for (i = 0; i < 3; i++) {
            unsigned char *out = verts + (size_t)k[i] * QUAD_STRIDE;
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
            for (j = 0; j < QUAD_VALUES; j++) {
                float d1 = value[1][j] - value[0][j], d2 = value[2][j] - value[0][j];
                float gx = (d1 * dy2 - d2 * dy1) / det, gy = (d2 * dx1 - d1 * dx2) / det;
                float a = value[0][j] + gx * ox + gy * oy;
                if (j == 0) {
                    memcpy(out + 8, &a, 4);
                } else if (j < 5) {
                    a = floorf(a + 0.5f);
                    out[16 + j - 1] = (unsigned char)(a < 0.0f ? 0.0f : a > 255.0f ? 255.0f : a);
                } else {
                    memcpy(out + 20 + (j - 5) * 4, &a, 4);
                }
            }
        }
    }
}

/* Records a quad draw of `mode` with the state s and the stages' textures tex into the current
   target, its vertices widened to QUAD_STRIDE and a strip's triangles made a list. A mode that does
   not draw into the target draws nothing (render/gfx.h, gfx_set_mode). Answers whether it recorded
   the draw. */
static int raw_draw(int mode, const struct draw_state *s, struct gfx_surface *const *tex, int prim, int vertexKind,
                    const void *verts, int nverts, const WORD *indices, int nindices)
{
    int flat = (vertexKind & FLAT_KIND) != 0, kind = vertexKind & 0xff;
    int stride = kind == GFX_VTX_TEX2 ? QUAD_STRIDE : GFX_VTX_TEX1_SIZE;
    int n = nindices ? nindices : nverts, i, cls, a2c;
    struct gpu_step *pass;
    struct gpu_draw *d;
    struct GpuDraw *r;
    long voff, ioff;
    uint32_t count;
    unsigned char *out;
    const unsigned char *in = verts;
    SDL_GPUTextureSamplerBinding bind[2];
    SDL_GPUGraphicsPipeline *pipe;
    if (!G.dev || !G.target || nverts <= 0 || (kind == GFX_VTX_WORLD && !G.worldCameraSet) || G.shadowPass
        || (prim != GFX_PT_TRIANGLELIST && prim != GFX_PT_TRIANGLESTRIP))
        return 0;
    /* a texture's upload may need a step of its own before the pass */
    bind_stage(bind, s, tex[0], 0);
    bind_stage(bind + 1, s, tex[1], 1);
    pass = current_pass();
    if (!pass)
        return 0;
    cls = pass->target == G.screen ? CLASS_SCREEN : pass->target->kind == K_RTTEX ? CLASS_TEXTURE : CLASS_COPY;
    flat = flat && kind != GFX_VTX_WORLD;
    pipe = quad_pipeline(mode, s, cls, flat);
    if (!pipe)
        return 0;
    a2c = alpha_to_coverage(draw_mode(mode), (int)pass_samples(pass), flat);
    d = add_draw(pass);
    if (!d)
        return 0;
    d->program = PROG_QUAD;
    d->pipeline = pipe;
    d->bind[0] = bind[0];
    d->bind[1] = bind[1];
    d->record = add_record(&r);
    voff = bytes_add(&G.vertexData, (size_t)nverts * QUAD_STRIDE, 8);
    count = prim == GFX_PT_TRIANGLESTRIP ? (uint32_t)(n > 2 ? n - 2 : 0) * 3 : (uint32_t)n;
    ioff = prim == GFX_PT_TRIANGLESTRIP || nindices ? bytes_add(&G.indexData, (size_t)count * 2, 4) : 0;
    if (!r || voff < 0 || ioff < 0) {
        pass->count--;
        G.ndraws--;
        return 0;
    }
    fill_state(r, pass, s, tex, kind == GFX_VTX_WORLD ? KIND_WORLD : KIND_SCREEN, a2c);
    if (kind == GFX_VTX_WORLD)
        r->mode[3] = (uint32_t)view_for(&G.worldCamera, &G.cameraView, pass);
    out = G.vertexData.data + voff;
    for (i = 0; i < nverts; i++) {
        memcpy(out + (size_t)i * QUAD_STRIDE, in + (size_t)i * stride, (size_t)stride);
        if (stride < QUAD_STRIDE)
            memcpy(out + (size_t)i * QUAD_STRIDE + GFX_VTX_TEX1_SIZE, in + (size_t)i * stride + 20, 8);
    }
    d->vertexOffset = (uint32_t)voff;
    if (prim == GFX_PT_TRIANGLESTRIP)
        batch_strip_list((WORD *)(G.indexData.data + ioff), n, indices, 0);
    else if (nindices) {
        memcpy(G.indexData.data + ioff, indices, (size_t)nindices * 2);
    }
    d->indexed = prim == GFX_PT_TRIANGLESTRIP || nindices;
    d->firstIndex = d->indexed ? (uint32_t)(ioff / 2) : 0;
    d->count = count;
    if (flat && pass_samples(pass) != SDL_GPU_SAMPLECOUNT_1)
        snap_flat(out, nverts, d->indexed ? (const WORD *)(G.indexData.data + ioff) : NULL, count);
    G.target->version = next_version();
    return 1;
}

/* The client's vertices, batched (gfx_batch.c) or not, in the current mode and with the stages'
   textures, counted as diag/perf.h says; the blit quads draw through raw_draw alone and count as
   blits. */
static void client_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices)
{
    if (!raw_draw(G.mode, &G.state, G.stageTex, prim, vertexKind, verts, nverts, indices, nindices))
        return;
    g_perf.draws++;
    g_perf.prims += perf_prim_count(prim, nindices ? nindices : nverts);
    g_perf.vertexBytes += (unsigned)nverts * ((vertexKind & 0xff) == GFX_VTX_TEX2 ? QUAD_STRIDE : GFX_VTX_TEX1_SIZE)
                          + (unsigned)nindices * 2;
}

/* Pre-transformed vertices that all share one rhw lie flat on the screen (the UI, sprites); the
   Direct3D 9 client drew those without multisampling (build/ref/COMMIT.txt). */
static int flat_draw(int vertexKind, const void *verts, int nverts)
{
    int stride = vertexKind == GFX_VTX_TEX2 ? QUAD_STRIDE : GFX_VTX_TEX1_SIZE, i;
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

/* A quad from a rectangle of a surface to one of a target (DRAW_BLIT): one texel to a pixel when
   they are the same size, stretched with the nearest texel otherwise; the client's mode, textures
   and target are as before after. */
static void draw_textured_quad(struct gfx_surface *dst, const RECT *d, struct gfx_surface *src, const RECT *s)
{
    struct { float x, y, z, rhw; DWORD diffuse; float u, v; } v[4];
    struct gfx_surface *prevTarget = G.target, *tex[2];
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
    tex[0] = src;
    tex[1] = NULL;
    G.target = dst;
    raw_draw(DRAW_BLIT, &state, tex, GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1 | FLAT_KIND, v, 4, NULL, 0);
    G.target = prevTarget;
}

/* ---- the static world ---- */

/* The water's records as its shaders read them (src/render/shaders/water.hlsli). */
struct GpuWaterCell {
    uint32_t cell[4];
    float corners[8];
};

struct GpuWaterFrame {
    uint32_t frame[4];
    float view[12];
    float screen[4];
};

#define INDIRECT_SIZE ((uint32_t)sizeof(SDL_GPUIndexedIndirectDrawCommand))
/* what the transform pass writes of a slot (world_transform.hlsl, world.hlsli): its views to clip
   and to view space, its place in the world, and its view in each cascade's light space; and a
   texture-coordinate matrix as the shaders read it, two float4 */
#define SLOT_VIEWS ((uint32_t)sizeof(float[2][16]))
#define SLOT_PLACE ((uint32_t)sizeof(float[12]))
#define SLOT_CASTERS ((uint32_t)sizeof(float[WL_CASCADES][16]))
#define UV_MATRIX ((uint32_t)sizeof(float[2][4]))
/* a vertex of the world's: struct GfxMeshVertex, then its record's id beside it */
#define WORLD_VERTEX ((uint32_t)sizeof(struct GfxMeshVertex))
/* the stream that takes the world's meshes and textures up (world_stream): its transfer buffer
   holds this much, or one texture where that is larger */
#define STREAM_BYTES (32u << 20)
/* a staging transfer buffer this much larger than the frames' uploads have been for a while is
   made again as small as they need */
#define STAGING_QUIET 120

_Static_assert(GFX_SHADOW_CASCADES == WL_CASCADES, "the transform pass takes every cascade");

static int world_stream(int textures);

/* A buffer of the world's at least `need` bytes: made, or made larger (sw_layout_grow) with the
   `keep` bytes it held copied over by the next stream (world_stream). */
static int world_buffer(SDL_GPUBuffer **buffer, uint32_t *size, uint32_t need, uint32_t keep,
                        SDL_GPUBufferUsageFlags usage)
{
    SDL_GPUBufferCreateInfo info;
    SDL_GPUBuffer *grown;
    uint32_t s;
    if (*buffer && need <= *size)
        return 1;
    s = sw_layout_grow(*buffer ? *size : 0, need, 4096);
    SDL_zero(info);
    info.usage = usage;
    info.size = s;
    grown = SDL_CreateGPUBuffer(G.dev, &info);
    if (!grown)
        return 0;
    if (*buffer && keep) {
        struct world_copy *c;
        if (!grow((void **)&W.copies, &W.copyCapacity, W.ncopies + 1, sizeof *W.copies)) {
            SDL_ReleaseGPUBuffer(G.dev, grown);
            return 0;
        }
        c = &W.copies[W.ncopies++];
        c->from = *buffer;
        c->to = grown;
        c->size = keep;
    } else if (*buffer) {
        SDL_ReleaseGPUBuffer(G.dev, *buffer);
    }
    *buffer = grown;
    *size = s;
    return 1;
}

int gfx_world_generation(void)
{
    return W.generation;
}

/* whether the device takes the world's parts: it is there, and has not given the world up after
   uploads that failed again and again (world_failed) */
static int world_on(void)
{
    return G.dev && !W.off;
}

static void world_release(void)
{
    size_t i;
    SDL_GPUBuffer **buffers[] = { &W.vertices, &W.ids, &W.indices, &W.recordBuffer, &W.cullBuffer, &W.runBuffer,
                                  &W.chunkBuffer, &W.places, &W.drawn, &W.commands, &W.slotBuffer, &W.views,
                                  &W.placed, &W.casterViews,
                                  &W.animatedBuffer, &W.uvBuffer, &W.cellBuffer, &W.waveBuffer, &W.sineBuffer,
                                  &W.instanceBuffer };
    if (G.dev) {
        for (i = 0; i < sizeof buffers / sizeof buffers[0]; i++)
            if (*buffers[i])
                SDL_ReleaseGPUBuffer(G.dev, *buffers[i]);
        for (i = 0; i < W.narrays; i++)
            if (W.arrays[i].tex)
                SDL_ReleaseGPUTexture(G.dev, W.arrays[i].tex);
        for (i = 0; i < W.ncopies; i++)
            SDL_ReleaseGPUBuffer(G.dev, W.copies[i].from);
        for (i = 0; i < W.narrayCopies; i++)
            SDL_ReleaseGPUTexture(G.dev, W.arrayCopies[i].from);
    }
    for (i = 0; i < W.ntextureUploads; i++)
        gfx_release(W.textureUploads[i].surface);
    for (i = 0; i < sizeof buffers / sizeof buffers[0]; i++)
        *buffers[i] = NULL;
    W.verticesSize = W.idsSize = W.indicesSize = W.recordBufferSize = W.cullBufferSize = W.runBufferSize = 0;
    W.chunkBufferSize = W.placesSize = W.drawnSize = W.commandsSize = 0;
    W.slotBufferSize = W.viewsSize = W.placedSize = W.casterViewsSize = W.animatedSize = W.uvSize = 0;
    W.cellBufferSize = W.waveSize = W.sineSize = W.instanceSize = 0;
    memset(W.arrays, 0, sizeof W.arrays);
    W.narrays = W.ncopies = W.narrayCopies = W.nmeshUploads = W.ntextureUploads = 0;
    W.meshes.size = 0;
    W.vertexCount = W.indexCount = W.uploadedVertices = W.uploadedIndices = 0;
    W.recordCount = W.idCount = W.slotCount = W.cellCount = 0;
    W.nruns = W.nindirect = W.nchunks = 0;
    W.drawnIndices = 0;
    W.recordsChanged = W.slotsChanged = W.cellsChanged = 0;
    W.computeDue = W.computed = 0;
}

/* the device's world made again from nothing: the client builds it anew (gfx_world_generation) */
static void world_reset(void)
{
    world_release();
    W.frameSet = 0;
    W.generation++;
}

void gfx_world_clear(void)
{
    world_reset();
    W.failures = 0;
    W.off = 0;
}

/* What went up was lost: the world is made again, unless the last attempt failed as well, after
   which the device takes no more of it until the client clears it (a new map), rather than stage
   all of it again every frame. */
static void world_failed(const char *what)
{
    SDL_Log("gfx: the world's %s: %s", what, SDL_GetError());
    world_reset();
    if (++W.failures >= 2) {
        W.off = 1;
        SDL_Log("gfx: the world failed twice and is not drawn until the next map");
    }
}

int gfx_world_mesh(const struct SwPlacement *place, const struct GfxMeshVertex *verts, const uint32_t *ids,
                   const WORD *indices)
{
    struct world_upload *u;
    size_t vbytes, ibytes, i;
    uint32_t *wide;
    long at;
    if (!world_on() || !place || !verts || !ids || !indices || !place->vertexCount || !place->indexCount
        || place->baseVertex > 0xffffffffu / 52 - place->vertexCount
        || place->firstIndex > 0xffffffffu / 4 - place->indexCount)
        return 0;
    vbytes = (size_t)place->vertexCount * WORLD_VERTEX;
    ibytes = (size_t)place->indexCount * 4;
    at = bytes_add(&W.meshes, vbytes + (size_t)place->vertexCount * 4 + ibytes, 16);
    if (at < 0 || !grow((void **)&W.meshUploads, &W.meshUploadCapacity, W.nmeshUploads + 1, sizeof *W.meshUploads))
        return 0;
    memcpy(W.meshes.data + at, verts, vbytes);
    memcpy(W.meshes.data + at + vbytes, ids, (size_t)place->vertexCount * 4);
    /* the device's indices are 32-bit, so that a draw may start at any index (Metal takes an index
       buffer's offset in whole 4-byte words) */
    wide = (uint32_t *)(W.meshes.data + at + vbytes + (size_t)place->vertexCount * 4);
    for (i = 0; i < place->indexCount; i++)
        wide[i] = indices[i];
    u = &W.meshUploads[W.nmeshUploads++];
    memset(u, 0, sizeof *u);
    u->at = (uint32_t)at;
    u->a = place->baseVertex;
    u->b = place->vertexCount;
    u->c = place->firstIndex;
    u->d = place->indexCount;
    if (place->baseVertex + place->vertexCount > W.vertexCount)
        W.vertexCount = place->baseVertex + place->vertexCount;
    if (place->firstIndex + place->indexCount > W.indexCount)
        W.indexCount = place->firstIndex + place->indexCount;
    g_perf.vertexBytes += (unsigned)(vbytes + (size_t)place->vertexCount * 4 + ibytes);
    /* a map's first frame places all of its meshes: they go up as they come, a stream's worth at a
       time, rather than all held until the frame is drawn */
    if (W.meshes.size >= STREAM_BYTES && !world_stream(0)) {
        world_failed("meshes");
        return 0;
    }
    return 1;
}

int gfx_get_levels(gfx_surface *s)
{
    return s && s->kind == K_TEXTURE ? s->levels : 1;
}

int gfx_world_array(uint8_t array, gfx_surface *like)
{
    struct world_array *a;
    if (!world_on() || !like || like->kind != K_TEXTURE || array != W.narrays || array >= SW_NO_ARRAY)
        return 0;
    a = &W.arrays[W.narrays++];
    memset(a, 0, sizeof *a);
    a->width = (uint32_t)like->width;
    a->height = (uint32_t)like->height;
    a->levels = (uint32_t)like->levels;
    return 1;
}

int gfx_world_texture(gfx_surface *texture, uint8_t array, uint16_t layer)
{
    struct world_array *a;
    struct world_upload *u;
    if (!world_on() || !texture || texture->kind != K_TEXTURE || !texture->pixels || array >= W.narrays
        || layer >= GFX_WORLD_LAYERS)
        return 0;
    a = &W.arrays[array];
    if (a->width != (uint32_t)texture->width || a->height != (uint32_t)texture->height
        || a->levels != (uint32_t)texture->levels
        || !grow((void **)&W.textureUploads, &W.textureUploadCapacity, W.ntextureUploads + 1,
                 sizeof *W.textureUploads))
        return 0;
    /* the texture's pixels go up with the next stream, straight from the surface, which stays
       until then */
    gfx_addref(texture);
    u = &W.textureUploads[W.ntextureUploads++];
    memset(u, 0, sizeof *u);
    u->surface = texture;
    u->a = array;
    u->b = layer;
    if (layer + 1u > a->layers)
        a->layers = layer + 1u;
    return 1;
}

int gfx_world_records(const struct SwRecord *records, size_t count, uint32_t ids)
{
    size_t i, indirectRecords, chunks;
    if (!world_on() || (count && !records) || count > 0x7fffffff || ids > 0x7fffffff)
        return 0;
    for (i = 0; i < count; i++)
        if (records[i].id >= ids)
            return 0;
    if (!grow((void **)&W.byId, &W.byIdCapacity, ids, sizeof *W.byId)
        || !grow((void **)&W.cull, &W.cullCapacity, count, sizeof *W.cull)
        || !grow((void **)&W.runs, &W.runCapacity, count, sizeof *W.runs)
        || !grow((void **)&W.indirect, &W.indirectCapacity, count, sizeof *W.indirect))
        return 0;
    /* the chunks of every record, at least as many as those of the runs drawn indirectly */
    chunks = wl_chunks(records, count, NULL);
    if (chunks > 0x7fffffff || !grow((void **)&W.chunks, &W.chunkCapacity, chunks, sizeof *W.chunks))
        return 0;
    memset(W.byId, 0, ids * sizeof *W.byId);
    wl_records(records, count, W.byId, W.cull);
    W.nruns = sw_draw_runs(records, count, W.runs);
    W.nindirect = wl_runs(W.runs, W.nruns, W.indirect, &W.drawnIndices);
    indirectRecords = W.nindirect ? W.indirect[W.nindirect - 1].first + W.indirect[W.nindirect - 1].count : 0;
    W.nchunks = wl_chunks(records, indirectRecords, W.chunks);
    W.recordCount = (uint32_t)count;
    W.idCount = ids;
    W.recordsChanged = 1;
    return 1;
}

int gfx_world_slots(const struct GfxWorldSlot *slots, size_t first, size_t count)
{
    size_t i;
    if (!world_on() || (count && !slots) || first > 0xffff || count > 0x10000 - first
        || !grow((void **)&W.slots, &W.slotCapacity, first + count, sizeof *W.slots))
        return 0;
    for (i = 0; i < count; i++)
        wl_slot(slots[i].local, slots[i].parent, slots[i].animated, first + i == 0, &W.slots[first + i]);
    if (first + count > W.slotCount)
        W.slotCount = (uint32_t)(first + count);
    W.slotsChanged = 1;
    return 1;
}

int gfx_world_water(const struct GfxWaterCell *cells, size_t count, const float *sines)
{
    size_t i;
    if (!world_on() || (count && !cells) || !sines || count > 0x7fffffff
        || !grow((void **)&W.cells, &W.cellCapacity, count, sizeof *W.cells))
        return 0;
    for (i = 0; i < count; i++) {
        struct GpuWaterCell *c = &W.cells[i];
        int k;
        c->cell[0] = (uint32_t)cells[i].x;
        c->cell[1] = (uint32_t)cells[i].y;
        c->cell[2] = (uint32_t)cells[i].block;
        memcpy(&c->cell[3], &cells[i].top, 4);
        for (k = 0; k < 4; k++) {
            c->corners[k * 2] = cells[i].corners[k][0];
            c->corners[k * 2 + 1] = cells[i].corners[k][1];
        }
    }
    memcpy(W.sines, sines, sizeof W.sines);
    W.cellCount = (uint32_t)count;
    W.cellsChanged = 1;
    return 1;
}

void gfx_world_frame(const struct GfxWorldFrame *frame)
{
    long at;
    size_t i;
    W.frameSet = 0;
    if (!world_on() || !frame)
        return;
    W.camera = frame->camera;
    W.frustum = frame->frustum;
    W.animated.size = W.uvs.size = W.waves.size = 0;
    at = bytes_add(&W.animated, (frame->animatedCount ? frame->animatedCount : 1) * sizeof frame->animated[0], 16);
    if (at < 0)
        return;
    if (frame->animatedCount)
        memcpy(W.animated.data, frame->animated, frame->animatedCount * sizeof frame->animated[0]);
    at = bytes_add(&W.uvs, (frame->uvCount ? frame->uvCount : 1) * UV_MATRIX, 16);
    if (at < 0)
        return;
    for (i = 0; i < frame->uvCount; i++) {
        float *u = (float *)(W.uvs.data + i * UV_MATRIX);
        /* (u, v, 1) dotted with each: uv[0], uv[2], uv[4] make u', uv[1], uv[3], uv[5] v' */
        u[0] = frame->uv[i][0];
        u[1] = frame->uv[i][2];
        u[2] = frame->uv[i][4];
        u[4] = frame->uv[i][1];
        u[5] = frame->uv[i][3];
        u[6] = frame->uv[i][5];
    }
    at = bytes_add(&W.waves, (frame->waveCount ? frame->waveCount : 1) * sizeof *frame->waves, 16);
    if (at < 0)
        return;
    if (frame->waveCount)
        memcpy(W.waves.data, frame->waves, frame->waveCount * sizeof *frame->waves);
    W.waveCount = (uint32_t)frame->waveCount;
    W.frameSet = 1;
}

/* the array a record's texture or lightmap lies in, white where it has none or its array could not
   be made */
static SDL_GPUTexture *world_array(uint8_t array)
{
    return array < W.narrays && W.arrays[array].tex ? W.arrays[array].tex : W.white;
}

/* the pipeline cull of a record's flags */
static int world_cull(int flags)
{
    return flags & SW_CULL_NONE ? CULL_NONE : flags & SW_CULL_CCW ? CULL_CCW : CULL_CW;
}

/* Whether the world draws now: a frame of it given, into the screen, in a mode that draws meshes. */
static int world_ready(void)
{
    const struct DrawModeDesc *m = draw_mode(G.mode);
    return world_on() && G.target && W.frameSet && gpu_of(G.target) == G.screen && !G.shadowPass
           && (m->into & DRAW_MESHES) && W.recordCount && W.slotCount;
}

/* The draw record of a draw of the world's records in the current mode. */
static uint32_t world_record(struct gpu_step *pass, const struct GfxWorldDraw *draw, struct GpuDraw **out)
{
    struct gfx_surface *none[2] = { NULL, NULL };
    struct GpuDraw *r;
    uint32_t at = add_record(&r);
    *out = r;
    if (!r)
        return 0;
    fill_state(r, pass, &G.state, none, KIND_MESH, alpha_to_coverage(draw_mode(G.mode), (int)G.samples, 0));
    /* a stage reads with a border where the record has its texture (world.hlsl) */
    r->stages[1] = (G.state.address[0] == DRAW_ADDRESS_BORDER ? 1u : 0u)
                   | (G.state.address[1] == DRAW_ADDRESS_BORDER ? 2u : 0u);
    memcpy(r->colour, draw->colour, sizeof r->colour);
    r->mesh[0] = draw->kind == GFX_MESH_MODEL;
    r->mesh[1] = draw->kind == GFX_MESH_MODEL && draw->lightGround;
    r->mesh[3] = G.shadowReceive ? (uint32_t)shadow_record() + 1 : 0;
    view_fog(r->fog);
    return at;
}

/* A draw of the world's: of the world as it is now (gfx_world_generation), which a flush that
   makes the world again leaves undrawn (run_pass). */
static struct gpu_draw *world_add_draw(struct gpu_step *pass, int program, SDL_GPUGraphicsPipeline *pipe)
{
    struct gpu_draw *d;
    if (!pipe)
        return NULL;
    d = add_draw(pass);
    if (!d)
        return NULL;
    d->program = program;
    d->pipeline = pipe;
    d->generation = W.generation;
    return d;
}

static void world_bind(struct gpu_draw *d, uint8_t textureArray, uint8_t lightmapArray)
{
    d->arrays[0] = textureArray;
    d->bind[0].sampler = stage_sampler(&G.state, 0);
    d->arrays[1] = lightmapArray;
    d->bind[1].sampler = stage_sampler(&G.state, 1);
    d->bind[2].texture = G.shadowReceive ? G.shadowMaps : G.noShadow;
    d->bind[2].sampler = G.pointClamp;
}

void gfx_world_draw(int pass, const struct GfxWorldDraw *draw)
{
    struct gpu_step *step;
    struct GpuDraw *r;
    uint32_t record;
    size_t i;
    g_perf.drawCalls++;
    batch_flush();
    if (!draw || !world_ready() || pass < SW_PASS_OPAQUE_CLAMP || pass > SW_PASS_GROUND)
        return;
    step = current_pass();
    if (!step)
        return;
    record = world_record(step, draw, &r);
    if (!r)
        return;
    for (i = 0; i < W.nindirect; i++) {
        const struct SwDrawRun *run = &W.runs[i];
        struct gpu_draw *d;
        if (run->pass != pass)
            continue;
        d = world_add_draw(step, PROG_WORLD, G.worldPipeline[G.mode][world_cull(run->cull) - CULL_NONE]);
        if (!d)
            return;
        world_bind(d, run->textureArray, run->lightmapArray);
        d->indirect = 1;
        d->firstIndex = (uint32_t)i;
        d->uniform[0] = record;
        g_perf.draws++;
        g_perf.prims += run->indices / 3;
    }
    W.computeDue = 1;
    G.target->version = next_version();
}

void gfx_world_draw_record(const struct SwRecord *rec, const struct GfxWorldDraw *draw)
{
    struct gpu_step *step;
    struct gpu_draw *d;
    struct GpuDraw *r;
    uint32_t record;
    g_perf.drawCalls++;
    batch_flush();
    if (!rec || !draw || !world_ready() || !rec->indexCount || rec->id >= W.idCount
        || rec->firstIndex > W.indexCount || rec->indexCount > W.indexCount - rec->firstIndex)
        return;
    step = current_pass();
    if (!step)
        return;
    record = world_record(step, draw, &r);
    if (!r)
        return;
    d = world_add_draw(step, PROG_WORLD, G.worldPipeline[G.mode][world_cull(rec->flags) - CULL_NONE]);
    if (!d)
        return;
    world_bind(d, rec->textureArray, rec->lightmapArray);
    d->indexed = 1;
    d->firstIndex = rec->firstIndex;
    d->count = rec->indexCount;
    d->baseVertex = (int32_t)rec->baseVertex;
    d->uniform[0] = record;
    g_perf.draws++;
    g_perf.prims += rec->indexCount / 3;
    W.computeDue = 1;
    G.target->version = next_version();
}

void gfx_world_cast(uint32_t firstIndex, uint32_t count, uint32_t baseVertex, uint8_t textureArray, uint32_t ids)
{
    struct gpu_step *step;
    struct gpu_draw *d;
    struct draw_state caster;
    g_perf.drawCalls++;
    batch_flush();
    if (!world_on() || !G.shadowPass || G.shadowCascade < 0 || !W.frameSet || !W.slotCount || !count || count % 3
        || ids > W.idCount || firstIndex > W.indexCount || count > W.indexCount - firstIndex)
        return;
    step = current_pass();
    if (!step)
        return;
    if (W.casterRecord < 0) {
        struct GpuDraw *r;
        W.casterRecord = (int)add_record(&r);
        if (!r) {
            W.casterRecord = -1;
            return;
        }
        stage_scale(r->uvScale, r->uvLimit, NULL, 1);
    }
    d = world_add_draw(step, PROG_CASTER, G.casterPipeline);
    if (!d)
        return;
    /* the casters read their texture as the opaque lists do */
    resolve_state(&caster, DRAW_OPAQUE_CLAMP, 0, 0, 0, 0);
    d->arrays[0] = textureArray;
    d->bind[0].sampler = stage_sampler(&caster, 0);
    d->indexed = 1;
    d->firstIndex = firstIndex;
    d->count = count;
    d->baseVertex = (int32_t)baseVertex;
    d->uniform[0] = (uint32_t)W.casterRecord;
    d->uniform[1] = (uint32_t)G.shadowCascade;
    g_perf.draws++;
    g_perf.prims += count / 3;
    W.computeDue = 1;
}

void gfx_world_draw_water(const uint32_t *cells, size_t count, DWORD colour)
{
    struct gpu_step *step;
    struct gpu_draw *d;
    struct GpuDraw *r;
    SDL_GPUTextureSamplerBinding bind[2];
    long at;
    size_t i;
    g_perf.drawCalls++;
    batch_flush();
    if (!world_on() || !G.target || !cells || !count || !W.frameSet || !W.cellCount || !W.waveCount
        || !G.worldCameraSet || G.shadowPass || gpu_of(G.target) != G.screen || !G.waterPipeline[G.mode]
        || count > 0x7fffffff)
        return;
    bind_stage(bind, &G.state, G.stageTex[0], 0);
    bind[1].texture = G.white;
    bind[1].sampler = stage_sampler(&G.state, 1);
    step = current_pass();
    if (!step)
        return;
    at = bytes_add(&W.instances, count * WATER_INSTANCE, WATER_INSTANCE);
    if (at < 0)
        return;
    for (i = 0; i < count; i++) {
        uint32_t *p = (uint32_t *)(W.instances.data + at + i * WATER_INSTANCE);
        p[0] = cells[i] < W.cellCount ? cells[i] : 0;
        p[1] = colour;
    }
    d = world_add_draw(step, PROG_WATER, G.waterPipeline[G.mode]);
    if (!d)
        return;
    d->record = add_record(&r);
    if (!r) {
        step->count--;
        G.ndraws--;
        return;
    }
    fill_state(r, step, &G.state, G.stageTex, KIND_WORLD, alpha_to_coverage(draw_mode(G.mode), (int)pass_samples(step), 0));
    r->mode[3] = (uint32_t)view_for(&G.worldCamera, &G.cameraView, step);
    d->bind[0] = bind[0];
    d->bind[1] = bind[1];
    d->instances = (uint32_t)count;
    d->firstInstance = (uint32_t)(at / WATER_INSTANCE);
    d->uniform[0] = d->record;
    d->uniform[1] = W.waveCount;
    g_perf.draws++;
    g_perf.prims += (unsigned)count * 2;
    g_perf.vertexBytes += (unsigned)count * WATER_INSTANCE;
    G.target->version = next_version();
}

void gfx_set_world_camera(const struct GfxWorldView *view)
{
    batch_flush();
    if (!G.worldCameraSet || memcmp(&G.worldCamera, view, sizeof *view))
        G.cameraView = -1;
    G.worldCamera = *view;
    G.worldCameraSet = 1;
}

/* The shared buffers as large as what is placed, what they held copied over by the next stream. */
static int world_shared_buffers(void)
{
    SDL_GPUBufferUsageFlags vertex = SDL_GPU_BUFFERUSAGE_VERTEX;
    if (!W.vertexCount)
        return 1;
    return world_buffer(&W.vertices, &W.verticesSize, W.vertexCount * WORLD_VERTEX, W.uploadedVertices * WORLD_VERTEX,
                        vertex)
           && world_buffer(&W.ids, &W.idsSize, W.vertexCount * 4, W.uploadedVertices * 4, vertex)
           && world_buffer(&W.indices, &W.indicesSize, W.indexCount * 4, W.uploadedIndices * 4,
                           SDL_GPU_BUFFERUSAGE_INDEX | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ);
}

/* The texture arrays as large as what is placed: an array is made with exactly the layers placed
   in it when it is first made (a map's first frame places all its textures but the streamed
   models'), and made again with the power of two at or above the layers placed, its layers copied
   over by the next stream, where more come: never more than an array that began at four layers
   and doubled. An array that cannot be made is left out: its textures draw white. */
static void world_arrays(void)
{
    size_t i;
    for (i = 0; i < W.narrays; i++) {
        struct world_array *a = &W.arrays[i];
        SDL_GPUTextureCreateInfo info;
        SDL_GPUTexture *made;
        uint32_t capacity;
        if (a->failed || !a->layers || (a->tex && a->layers <= a->capacity))
            continue;
        capacity = a->layers;
        if (a->tex)
            for (capacity = 1; capacity < a->layers; capacity <<= 1)
                ;
        if (capacity > GFX_WORLD_LAYERS)
            capacity = GFX_WORLD_LAYERS;
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
        info.format = TARGET_FORMAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
        info.width = a->width;
        info.height = a->height;
        info.layer_count_or_depth = capacity;
        info.num_levels = a->levels;
        made = SDL_CreateGPUTexture(G.dev, &info);
        if (made && a->tex && !grow((void **)&W.arrayCopies, &W.arrayCopyCapacity, W.narrayCopies + 1,
                                    sizeof *W.arrayCopies)) {
            SDL_ReleaseGPUTexture(G.dev, made);
            made = NULL;
        }
        if (!made) {
            SDL_Log("gfx: the world's texture array %u (%ux%u, %u layers): %s", (unsigned)i, a->width, a->height,
                    capacity, SDL_GetError());
            if (a->tex)
                SDL_ReleaseGPUTexture(G.dev, a->tex);
            a->tex = NULL;
            a->failed = 1;
            continue;
        }
        if (a->tex) {
            struct world_array_copy *c = &W.arrayCopies[W.narrayCopies++];
            c->from = a->tex;
            c->to = made;
            c->layers = a->uploaded;
            c->array = (uint32_t)i;
        }
        a->tex = made;
        a->capacity = capacity;
    }
}

/* the bytes an upload takes in the stream */
static uint32_t stream_bytes(const struct world_upload *u, int texture)
{
    if (texture)
        return W.arrays[u->a].width * W.arrays[u->a].height * 4;
    return u->b * (WORLD_VERTEX + 4) + u->d * 4;
}

static void stream_wait(void)
{
    if (W.streamFence) {
        SDL_WaitForGPUFences(G.dev, true, &W.streamFence, 1);
        SDL_ReleaseGPUFence(G.dev, W.streamFence);
        W.streamFence = NULL;
    }
}

/* The buffers' and arrays' growth: what they held into their larger selves. */
static void stream_copies(SDL_GPUCopyPass *copy)
{
    size_t i;
    uint32_t level, layer;
    for (i = 0; i < W.ncopies; i++) {
        SDL_GPUBufferLocation from, to;
        from.buffer = W.copies[i].from;
        from.offset = 0;
        to.buffer = W.copies[i].to;
        to.offset = 0;
        SDL_CopyGPUBufferToBuffer(copy, &from, &to, W.copies[i].size, false);
    }
    for (i = 0; i < W.narrayCopies; i++) {
        const struct world_array *a = &W.arrays[W.arrayCopies[i].array];
        for (layer = 0; layer < W.arrayCopies[i].layers; layer++)
            for (level = 0; level < a->levels; level++) {
                SDL_GPUTextureLocation from, to;
                SDL_zero(from);
                SDL_zero(to);
                from.texture = W.arrayCopies[i].from;
                from.mip_level = level;
                from.layer = layer;
                to.texture = W.arrayCopies[i].to;
                to.mip_level = level;
                to.layer = layer;
                SDL_CopyGPUTextureToTexture(copy, &from, &to, a->width >> level ? a->width >> level : 1,
                                            a->height >> level ? a->height >> level : 1, 1, false);
            }
    }
}

static void stream_release_copies(void)
{
    size_t i;
    for (i = 0; i < W.ncopies; i++)
        SDL_ReleaseGPUBuffer(G.dev, W.copies[i].from);
    for (i = 0; i < W.narrayCopies; i++)
        SDL_ReleaseGPUTexture(G.dev, W.arrayCopies[i].from);
    W.ncopies = W.narrayCopies = 0;
}

/* A mesh's vertices, records and indices from the stream at `at`. */
static void stream_mesh(SDL_GPUCopyPass *copy, const struct world_upload *u, uint32_t at)
{
    SDL_GPUTransferBufferLocation src;
    SDL_GPUBufferRegion dst;
    const uint32_t offsets[3] = { at, at + u->b * WORLD_VERTEX, at + u->b * (WORLD_VERTEX + 4) };
    SDL_GPUBuffer *buffers[3] = { W.vertices, W.ids, W.indices };
    const uint32_t places[3] = { u->a * WORLD_VERTEX, u->a * 4, u->c * 4 };
    const uint32_t sizes[3] = { u->b * WORLD_VERTEX, u->b * 4, u->d * 4 };
    int k;
    for (k = 0; k < 3; k++) {
        src.transfer_buffer = W.stream;
        src.offset = offsets[k];
        dst.buffer = buffers[k];
        dst.offset = places[k];
        dst.size = sizes[k];
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    }
}

/* A texture's top level from the stream at `at` into its layer. */
static void stream_texture(SDL_GPUCopyPass *copy, const struct world_upload *u, uint32_t at)
{
    const struct world_array *a = &W.arrays[u->a];
    SDL_GPUTextureTransferInfo src;
    SDL_GPUTextureRegion dst;
    SDL_zero(src);
    SDL_zero(dst);
    src.transfer_buffer = W.stream;
    src.offset = at;
    src.pixels_per_row = a->width;
    src.rows_per_layer = a->height;
    dst.texture = a->tex;
    dst.layer = u->b;
    dst.w = a->width;
    dst.h = a->height;
    dst.d = 1;
    SDL_UploadToGPUTexture(copy, &src, &dst, false);
}

/* A layer's mipmaps, each made from the level above with a linear blit, as
   SDL_GenerateMipmapsForGPUTexture makes a texture's on Vulkan. */
static void stream_mipmaps(SDL_GPUCommandBuffer *cmd, const struct world_upload *u)
{
    const struct world_array *a = &W.arrays[u->a];
    uint32_t level;
    for (level = 1; level < a->levels; level++) {
        SDL_GPUBlitInfo blit;
        SDL_zero(blit);
        blit.source.texture = a->tex;
        blit.source.mip_level = level - 1;
        blit.source.layer_or_depth_plane = u->b;
        blit.source.w = a->width >> (level - 1) ? a->width >> (level - 1) : 1;
        blit.source.h = a->height >> (level - 1) ? a->height >> (level - 1) : 1;
        blit.destination.texture = a->tex;
        blit.destination.mip_level = level;
        blit.destination.layer_or_depth_plane = u->b;
        blit.destination.w = a->width >> level ? a->width >> level : 1;
        blit.destination.h = a->height >> level ? a->height >> level : 1;
        blit.load_op = SDL_GPU_LOADOP_DONT_CARE;
        blit.filter = SDL_GPU_FILTER_LINEAR;
        SDL_BlitGPUTexture(cmd, &blit);
    }
}

/* Takes what is pending of the world up through the stream (a transfer buffer of STREAM_BYTES,
   or of the largest texture where that is larger): the meshes, and with `textures` the textures
   too, as many as fit at a time, each batch in a command buffer of its own that is waited on
   before the next fills the buffer again; the buffers' and arrays' growth goes first. A texture
   goes up from its surface's own pixels, widened straight into the stream. 0 where a batch could
   not go. */
static int world_stream(int textures)
{
    size_t mesh = 0, texture = 0, released = 0;
    uint32_t largest = STREAM_BYTES;
    int copies = 1;
    if (!world_shared_buffers())
        return 0;
    if (textures)
        for (texture = 0; texture < W.ntextureUploads; texture++)
            if (stream_bytes(&W.textureUploads[texture], 1) > largest)
                largest = stream_bytes(&W.textureUploads[texture], 1);
    texture = 0;
    if (!W.stream || W.streamSize < largest) {
        SDL_GPUTransferBufferCreateInfo info;
        stream_wait();
        if (W.stream)
            SDL_ReleaseGPUTransferBuffer(G.dev, W.stream);
        SDL_zero(info);
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = largest;
        W.stream = SDL_CreateGPUTransferBuffer(G.dev, &info);
        W.streamSize = W.stream ? largest : 0;
        if (!W.stream)
            return 0;
    }
    while (mesh < W.nmeshUploads || (textures && texture < W.ntextureUploads) || (copies && (W.ncopies || W.narrayCopies))) {
        size_t meshFrom = mesh, textureFrom = texture, i;
        int copied = copies;
        uint32_t used = 0;
        unsigned char *mapped;
        SDL_GPUCommandBuffer *cmd;
        SDL_GPUCopyPass *copy;
        stream_wait();
        mapped = SDL_MapGPUTransferBuffer(G.dev, W.stream, false);
        if (!mapped)
            goto failed;
        for (; mesh < W.nmeshUploads; mesh++) {
            uint32_t n = stream_bytes(&W.meshUploads[mesh], 0);
            if (n > W.streamSize - used)
                break;
            memcpy(mapped + used, W.meshes.data + W.meshUploads[mesh].at, n);
            used += (n + 15u) & ~15u;
        }
        for (; textures && mesh == W.nmeshUploads && texture < W.ntextureUploads; texture++) {
            struct gfx_surface *s = W.textureUploads[texture].surface;
            uint32_t n = stream_bytes(&W.textureUploads[texture], 1);
            if (n > W.streamSize - used)
                break;
            texel_rows_to_bgra(s->fmt, s->pixels, (long)s->width * texel_bytes(s->fmt), s->width, s->height,
                               mapped + used, (long)s->width * 4);
            used += (n + 15u) & ~15u;
        }
        SDL_UnmapGPUTransferBuffer(G.dev, W.stream);
        cmd = SDL_AcquireGPUCommandBuffer(G.dev);
        if (!cmd)
            goto failed;
        copy = SDL_BeginGPUCopyPass(cmd);
        if (copies)
            stream_copies(copy);
        used = 0;
        for (i = meshFrom; i < mesh; i++) {
            stream_mesh(copy, &W.meshUploads[i], used);
            used += (stream_bytes(&W.meshUploads[i], 0) + 15u) & ~15u;
        }
        for (i = textureFrom; i < texture; i++) {
            if (W.arrays[W.textureUploads[i].a].tex)
                stream_texture(copy, &W.textureUploads[i], used);
            used += (stream_bytes(&W.textureUploads[i], 1) + 15u) & ~15u;
        }
        SDL_EndGPUCopyPass(copy);
        for (i = textureFrom; i < texture; i++)
            if (W.arrays[W.textureUploads[i].a].tex)
                stream_mipmaps(cmd, &W.textureUploads[i]);
        W.streamFence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
        if (!W.streamFence)
            goto failed;
        if (copies)
            stream_release_copies();
        copies = 0;
        for (i = textureFrom; i < texture; i++) {
            struct world_array *a = &W.arrays[W.textureUploads[i].a];
            if (W.textureUploads[i].b + 1u > a->uploaded)
                a->uploaded = W.textureUploads[i].b + 1u;
            gfx_release(W.textureUploads[i].surface);
        }
        released = texture;
        /* nothing fitted (the stream is as large as the largest texture and a mesh holds a few
           MB at most) */
        if (meshFrom == mesh && textureFrom == texture && !copied)
            goto failed;
    }
    W.nmeshUploads = 0;
    W.meshes.size = 0;
    W.uploadedVertices = W.vertexCount;
    W.uploadedIndices = W.indexCount;
    if (textures)
        W.ntextureUploads = 0;
    return 1;

failed:
    /* the textures that went up let their surfaces go: the list keeps those it still holds */
    memmove(W.textureUploads, W.textureUploads + released, (W.ntextureUploads - released) * sizeof *W.textureUploads);
    W.ntextureUploads -= released;
    return 0;
}

/* What goes up with this flush besides the stream, staged in G.texels: the white texel, the records,
   slots and cells that changed, the frame's matrices, waves and water instances; and the buffers
   they go into. 0 without memory. */
static int world_prepare(void)
{
    long at;
    if (!world_on())
        return 1;
    if (!W.white) {
        SDL_GPUTextureCreateInfo info;
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
        info.format = TARGET_FORMAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = info.height = 1;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        W.white = SDL_CreateGPUTexture(G.dev, &info);
        if (!W.white)
            return 0;
        W.whiteDue = 1;
    }
    if (W.whiteDue) {
        at = bytes_add(&G.texels, 4, 16);
        if (at < 0)
            return 0;
        memset(G.texels.data + at, 0xff, 4);
        W.whiteAt = (uint32_t)at;
    }
    if (W.recordsChanged) {
        uint32_t ids = W.idCount ? W.idCount : 1, records = W.recordCount ? W.recordCount : 1;
        uint32_t runs = W.nindirect ? (uint32_t)W.nindirect : 1, chunks = W.nchunks ? (uint32_t)W.nchunks : 1;
        if (!world_buffer(&W.recordBuffer, &W.recordBufferSize, ids * (uint32_t)sizeof *W.byId, 0,
                          SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
            || !world_buffer(&W.cullBuffer, &W.cullBufferSize, records * (uint32_t)sizeof *W.cull, 0,
                             SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ)
            || !world_buffer(&W.runBuffer, &W.runBufferSize, runs * (uint32_t)sizeof *W.indirect, 0,
                             SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ)
            || !world_buffer(&W.chunkBuffer, &W.chunkBufferSize, chunks * (uint32_t)sizeof *W.chunks, 0,
                             SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ)
            || !world_buffer(&W.places, &W.placesSize, records * 4, 0,
                             SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE)
            || !world_buffer(&W.commands, &W.commandsSize, runs * INDIRECT_SIZE, 0,
                             SDL_GPU_BUFFERUSAGE_INDIRECT | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE)
            || !world_buffer(&W.drawn, &W.drawnSize, (W.drawnIndices ? W.drawnIndices : 1) * 4, 0,
                             SDL_GPU_BUFFERUSAGE_INDEX | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE))
            return 0;
        size_t idBytes = (size_t)W.idCount * sizeof *W.byId, cullBytes = (size_t)W.recordCount * sizeof *W.cull;
        size_t runBytes = W.nindirect * sizeof *W.indirect;
        at = bytes_add(&G.texels, idBytes + cullBytes + runBytes + W.nchunks * sizeof *W.chunks, 16);
        if (at < 0)
            return 0;
        memcpy(G.texels.data + at, W.byId, idBytes);
        memcpy(G.texels.data + at + idBytes, W.cull, cullBytes);
        memcpy(G.texels.data + at + idBytes + cullBytes, W.indirect, runBytes);
        memcpy(G.texels.data + at + idBytes + cullBytes + runBytes, W.chunks, W.nchunks * sizeof *W.chunks);
        W.recordsAt = (uint32_t)at;
    }
    if (W.slotsChanged) {
        uint32_t n = W.slotCount ? W.slotCount : 1;
        if (!world_buffer(&W.slotBuffer, &W.slotBufferSize, n * (uint32_t)sizeof *W.slots, 0,
                          SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ)
            || !world_buffer(&W.views, &W.viewsSize, n * SLOT_VIEWS, 0,
                             SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
            || !world_buffer(&W.placed, &W.placedSize, n * SLOT_PLACE, 0,
                             SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ)
            || !world_buffer(&W.casterViews, &W.casterViewsSize, n * SLOT_CASTERS, 0,
                             SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ))
            return 0;
        at = bytes_add(&G.texels, (size_t)W.slotCount * sizeof *W.slots, 16);
        if (at < 0)
            return 0;
        memcpy(G.texels.data + at, W.slots, (size_t)W.slotCount * sizeof *W.slots);
        W.slotsAt = (uint32_t)at;
    }
    if (W.cellsChanged) {
        if (!world_buffer(&W.cellBuffer, &W.cellBufferSize, (W.cellCount ? W.cellCount : 1) * (uint32_t)sizeof *W.cells,
                          0, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
            || !world_buffer(&W.sineBuffer, &W.sineSize, sizeof W.sines, 0, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ))
            return 0;
        at = bytes_add(&G.texels, (size_t)W.cellCount * sizeof *W.cells + sizeof W.sines, 16);
        if (at < 0)
            return 0;
        memcpy(G.texels.data + at, W.cells, (size_t)W.cellCount * sizeof *W.cells);
        memcpy(G.texels.data + at + (size_t)W.cellCount * sizeof *W.cells, W.sines, sizeof W.sines);
        W.cellsAt = (uint32_t)at;
    }
    if (W.frameSet && (W.computeDue || W.instances.size)) {
        if (!world_buffer(&W.animatedBuffer, &W.animatedSize, (uint32_t)W.animated.size, 0,
                          SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ)
            || !world_buffer(&W.uvBuffer, &W.uvSize, (uint32_t)W.uvs.size, 0, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
            || !world_buffer(&W.waveBuffer, &W.waveSize, (uint32_t)W.waves.size, 0,
                             SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
            || (W.instances.size && !world_buffer(&W.instanceBuffer, &W.instanceSize, (uint32_t)W.instances.size, 0,
                                                  SDL_GPU_BUFFERUSAGE_VERTEX)))
            return 0;
        at = bytes_add(&G.texels, W.animated.size + W.uvs.size + W.waves.size + W.instances.size, 16);
        if (at < 0)
            return 0;
        memcpy(G.texels.data + at, W.animated.data, W.animated.size);
        memcpy(G.texels.data + at + W.animated.size, W.uvs.data, W.uvs.size);
        memcpy(G.texels.data + at + W.animated.size + W.uvs.size, W.waves.data, W.waves.size);
        if (W.instances.size)
            memcpy(G.texels.data + at + W.animated.size + W.uvs.size + W.waves.size, W.instances.data,
                   W.instances.size);
        W.frameAt = (uint32_t)at;
        W.frameDue = 1;
    }
    return 1;
}

static void upload_buffer(SDL_GPUCopyPass *copy, uint32_t from, SDL_GPUBuffer *buffer, uint32_t offset,
                          uint32_t size, int cycle)
{
    SDL_GPUTransferBufferLocation src;
    SDL_GPUBufferRegion dst;
    if (!size || !buffer)
        return;
    src.transfer_buffer = G.upload;
    src.offset = from;
    dst.buffer = buffer;
    dst.offset = offset;
    dst.size = size;
    SDL_UploadToGPUBuffer(copy, &src, &dst, cycle);
}

/* The copy pass's part of the world: what world_prepare staged. */
static void world_copy(SDL_GPUCopyPass *copy, uint32_t base)
{
    if (!world_on())
        return;
    if (W.whiteDue) {
        SDL_GPUTextureTransferInfo src;
        SDL_GPUTextureRegion dst;
        SDL_zero(src);
        SDL_zero(dst);
        src.transfer_buffer = G.upload;
        src.offset = base + W.whiteAt;
        dst.texture = W.white;
        dst.w = dst.h = dst.d = 1;
        SDL_UploadToGPUTexture(copy, &src, &dst, false);
    }
    if (W.recordsChanged) {
        uint32_t at = base + W.recordsAt, ids = W.idCount * (uint32_t)sizeof *W.byId;
        uint32_t cull = W.recordCount * (uint32_t)sizeof *W.cull;
        upload_buffer(copy, at, W.recordBuffer, 0, ids, 1);
        upload_buffer(copy, at + ids, W.cullBuffer, 0, cull, 1);
        uint32_t runs = (uint32_t)(W.nindirect * sizeof *W.indirect);
        upload_buffer(copy, at + ids + cull, W.runBuffer, 0, runs, 1);
        upload_buffer(copy, at + ids + cull + runs, W.chunkBuffer, 0, (uint32_t)(W.nchunks * sizeof *W.chunks), 1);
    }
    if (W.slotsChanged)
        upload_buffer(copy, base + W.slotsAt, W.slotBuffer, 0, W.slotCount * (uint32_t)sizeof *W.slots, 1);
    if (W.cellsChanged) {
        upload_buffer(copy, base + W.cellsAt, W.cellBuffer, 0, W.cellCount * (uint32_t)sizeof *W.cells, 1);
        upload_buffer(copy, base + W.cellsAt + W.cellCount * (uint32_t)sizeof *W.cells, W.sineBuffer, 0,
                      sizeof W.sines, 1);
    }
    if (W.frameDue) {
        uint32_t at = base + W.frameAt;
        upload_buffer(copy, at, W.animatedBuffer, 0, (uint32_t)W.animated.size, 1);
        at += (uint32_t)W.animated.size;
        upload_buffer(copy, at, W.uvBuffer, 0, (uint32_t)W.uvs.size, 1);
        at += (uint32_t)W.uvs.size;
        upload_buffer(copy, at, W.waveBuffer, 0, (uint32_t)W.waves.size, 1);
        at += (uint32_t)W.waves.size;
        upload_buffer(copy, at, W.instanceBuffer, 0, (uint32_t)W.instances.size, 1);
    }
}

/* The frame's compute passes: the slots' views; the view's cull of the runs, which gives each run's
   kept records their places in the frame's buffer of indices and writes the run's one indirect
   draw; and the compaction, which copies the kept records' indices to their places. Sets
   W.computed where they ran, without which nothing of the world is drawn. */
static void world_compute(SDL_GPUCommandBuffer *cmd)
{
    struct WlTransformFrame transform;
    struct WlCullFrame cull;
    struct WlCompactFrame compact;
    uint32_t groups[2];
    float projection[16];
    SDL_GPUStorageBufferReadWriteBinding rw[3];
    SDL_GPUBuffer *read[4];
    SDL_GPUComputePass *pass;
    if (!world_on() || !W.computeDue || !W.frameSet || !W.slotCount || !W.recordCount || !W.transform
        || !W.cullPipeline || !W.compactPipeline || !G.screen || !W.slotBuffer || !W.animatedBuffer || !W.views
        || !W.placed || !W.casterViews || !W.cullBuffer || !W.runBuffer || !W.chunkBuffer || !W.places
        || !W.indices || !W.drawn || !W.commands)
        return;
    wl_projection(projection, W.camera.proj, W.camera.centre, W.camera.nearZ, W.camera.depthScale,
                  G.screen->width, G.screen->height);
    wl_transform_frame(W.camera.view, projection, (const float (*)[16])G.shadow.fromView,
                       G.shadowReady ? (uint32_t)G.shadow.cascades : 0, W.slotCount, &transform);
    SDL_zero(rw);
    rw[0].buffer = W.views;
    rw[0].cycle = true;
    rw[1].buffer = W.placed;
    rw[1].cycle = true;
    rw[2].buffer = W.casterViews;
    rw[2].cycle = true;
    pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 3);
    if (!pass)
        return;
    SDL_BindGPUComputePipeline(pass, W.transform);
    read[0] = W.slotBuffer;
    read[1] = W.animatedBuffer;
    SDL_BindGPUComputeStorageBuffers(pass, 0, read, 2);
    SDL_PushGPUComputeUniformData(cmd, 0, &transform, sizeof transform);
    SDL_DispatchGPUCompute(pass, (W.slotCount + 63) / 64, 1, 1);
    SDL_EndGPUComputePass(pass);
    if (W.nindirect) {
        wl_cull_frame(&W.frustum, (uint32_t)W.nindirect, &cull, groups);
        SDL_zero(rw);
        rw[0].buffer = W.places;
        rw[0].cycle = true;
        rw[1].buffer = W.commands;
        rw[1].cycle = true;
        pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 2);
        if (!pass)
            return;
        SDL_BindGPUComputePipeline(pass, W.cullPipeline);
        read[0] = W.cullBuffer;
        read[1] = W.placed;
        read[2] = W.runBuffer;
        SDL_BindGPUComputeStorageBuffers(pass, 0, read, 3);
        SDL_PushGPUComputeUniformData(cmd, 0, &cull, sizeof cull);
        SDL_DispatchGPUCompute(pass, groups[0], groups[1], 1);
        SDL_EndGPUComputePass(pass);
        wl_compact_frame((uint32_t)W.nchunks, &compact, groups);
        if (groups[0]) {
            SDL_zero(rw);
            rw[0].buffer = W.drawn;
            rw[0].cycle = true;
            pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 1);
            if (!pass)
                return;
            SDL_BindGPUComputePipeline(pass, W.compactPipeline);
            read[0] = W.cullBuffer;
            read[1] = W.places;
            read[2] = W.indices;
            read[3] = W.chunkBuffer;
            SDL_BindGPUComputeStorageBuffers(pass, 0, read, 4);
            SDL_PushGPUComputeUniformData(cmd, 0, &compact, sizeof compact);
            SDL_DispatchGPUCompute(pass, groups[0], groups[1], 1);
            SDL_EndGPUComputePass(pass);
        }
    }
    W.computed = 1;
}

/* Whether a recorded draw of the world's may be replayed: it is of the world as it is now, and what
   it reads is there. */
static int world_drawable(const struct gpu_draw *d)
{
    if (d->generation != W.generation || !world_on())
        return 0;
    if (d->program == PROG_WATER)
        return W.cellBuffer && W.waveBuffer && W.sineBuffer && W.instanceBuffer;
    if (!W.computed || !W.recordBuffer || !W.vertices || !W.ids || !W.uvBuffer)
        return 0;
    if (d->program == PROG_CASTER)
        return W.casterViews && W.indices;
    return W.views && (d->indirect ? W.drawn && W.commands : W.indices != NULL);
}

/* After a flush: what went up has gone up. */
static void world_flushed(void)
{
    W.recordsChanged = W.slotsChanged = W.cellsChanged = 0;
    W.whiteDue = 0;
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

/* The storage buffers the programs read (src/render/shaders), in the order they declare them, and
   their vertex and index buffers: the quads' the frame's vertices, which each draw binds at its
   offset, and the records' instances; the world's records and casters the shared buffers, whose
   index buffer run_pass binds by the draw (the shared indices, or the frame's compacted ones); the
   water its cells' instances. */
static void bind_program(SDL_GPURenderPass *pass, int program)
{
    SDL_GPUBuffer *vertex[6], *fragment[2];
    SDL_GPUBufferBinding binding[2];
    Uint32 nv = 0, nf = 1;
    fragment[0] = G.records;
    fragment[1] = G.shadows;
    SDL_zero(binding);
    switch (program) {
    case PROG_QUAD:
        vertex[nv++] = G.records;
        vertex[nv++] = G.views;
        vertex[nv++] = G.fogs;
        binding[1].buffer = G.instances;
        SDL_BindGPUVertexBuffers(pass, 1, &binding[1], 1);
        binding[0].buffer = G.indices;
        SDL_BindGPUIndexBuffer(pass, &binding[0], SDL_GPU_INDEXELEMENTSIZE_16BIT);
        break;
    case PROG_WORLD:
    case PROG_CASTER:
        if (program == PROG_WORLD)
            vertex[nv++] = G.records;
        vertex[nv++] = W.recordBuffer;
        vertex[nv++] = program == PROG_WORLD ? W.views : W.casterViews;
        if (program == PROG_WORLD)
            vertex[nv++] = G.shadows;
        vertex[nv++] = W.uvBuffer;
        nf = program == PROG_WORLD ? 2 : 1;
        binding[0].buffer = W.vertices;
        binding[1].buffer = W.ids;
        SDL_BindGPUVertexBuffers(pass, 0, binding, 2);
        break;
    default:
        vertex[nv++] = G.records;
        vertex[nv++] = G.views;
        vertex[nv++] = G.fogs;
        vertex[nv++] = W.cellBuffer;
        vertex[nv++] = W.waveBuffer;
        vertex[nv++] = W.sineBuffer;
        binding[0].buffer = W.instanceBuffer;
        SDL_BindGPUVertexBuffers(pass, 0, binding, 1);
        break;
    }
    SDL_BindGPUVertexStorageBuffers(pass, 0, vertex, nv);
    SDL_BindGPUFragmentStorageBuffers(pass, 0, fragment, nf);
}

/* The uniform a world program's draw takes: the draw's record and the cascade; the water's with
   the frame's view, which its quads are tested in. */
static void push_uniforms(SDL_GPUCommandBuffer *cmd, const struct gpu_draw *d)
{
    if (d->program == PROG_WATER) {
        struct GpuWaterFrame u;
        memset(&u, 0, sizeof u);
        u.frame[0] = d->uniform[0];
        u.frame[1] = d->uniform[1];
        memcpy(u.view, W.camera.view, sizeof u.view);
        u.screen[0] = W.camera.proj[0];
        u.screen[1] = W.camera.proj[1];
        u.screen[2] = W.camera.centre[0];
        u.screen[3] = W.camera.centre[1];
        SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof u);
        return;
    }
    SDL_PushGPUVertexUniformData(cmd, 0, d->uniform, sizeof d->uniform);
    SDL_PushGPUFragmentUniformData(cmd, 0, d->uniform, sizeof d->uniform);
}

static void run_pass(SDL_GPUCommandBuffer *cmd, const struct gpu_step *s)
{
    SDL_GPUColorTargetInfo colour;
    SDL_GPUDepthStencilTargetInfo depth;
    SDL_GPURenderPass *pass;
    SDL_GPUBufferBinding binding;
    SDL_GPUGraphicsPipeline *bound = NULL;
    int program = -1, ms = s->target && s->target == G.screen && G.msaa, hasDepth = pass_has_depth(s);
    SDL_GPUBuffer *indices = NULL;
    uint32_t vertexOffset = ~0u, pushed[4];
    int pushedAny = 0;
    SDL_GPUTextureSamplerBinding textures[3];
    SDL_Rect scissor;
    int w = s->target ? s->target->width : GFX_SHADOW_SIZE, h = s->target ? s->target->height : GFX_SHADOW_SIZE;
    SDL_GPUViewport viewport;
    uint32_t i;
    int j;
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
    for (i = s->first; i < s->first + s->count; i++) {
        const struct gpu_draw *d = &G.draws[i];
        int nbind = d->program == PROG_WORLD ? 3 : d->program == PROG_CASTER ? 1 : 2;
        SDL_GPUTextureSamplerBinding bind[3];
        if (!d->pipeline || (d->program != PROG_QUAD && !world_drawable(d)))
            continue;
        memcpy(bind, d->bind, sizeof bind);
        if (d->program == PROG_WORLD || d->program == PROG_CASTER)
            for (j = 0; j < (d->program == PROG_WORLD ? 2 : 1); j++)
                bind[j].texture = world_array(d->arrays[j]);
        if (d->pipeline != bound) {
            SDL_BindGPUGraphicsPipeline(pass, d->pipeline);
            bound = d->pipeline;
        }
        if (d->program != program) {
            bind_program(pass, d->program);
            program = d->program;
            memset(textures, 0, sizeof textures);
            vertexOffset = ~0u;
            pushedAny = 0;
            indices = NULL;
        }
        if (d->program == PROG_WORLD || d->program == PROG_CASTER) {
            SDL_GPUBuffer *want = d->indirect ? W.drawn : W.indices;
            if (want != indices) {
                binding.buffer = want;
                binding.offset = 0;
                SDL_BindGPUIndexBuffer(pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
                indices = want;
            }
        }
        if (memcmp(textures, bind, (size_t)nbind * sizeof *textures)) {
            memcpy(textures, bind, (size_t)nbind * sizeof *textures);
            SDL_BindGPUFragmentSamplers(pass, 0, textures, (Uint32)nbind);
        }
        if (d->program != PROG_QUAD && (!pushedAny || memcmp(pushed, d->uniform, sizeof pushed))) {
            push_uniforms(cmd, d);
            memcpy(pushed, d->uniform, sizeof pushed);
            pushedAny = 1;
        }
        if (d->program == PROG_QUAD) {
            if (d->vertexOffset != vertexOffset) {
                binding.buffer = G.vertices;
                binding.offset = d->vertexOffset;
                SDL_BindGPUVertexBuffers(pass, 0, &binding, 1);
                vertexOffset = d->vertexOffset;
            }
            if (d->indexed)
                SDL_DrawGPUIndexedPrimitives(pass, d->count, 1, d->firstIndex, 0, d->record);
            else
                SDL_DrawGPUPrimitives(pass, d->count, 1, 0, d->record);
        } else if (d->program == PROG_WATER) {
            SDL_DrawGPUPrimitives(pass, 6, d->instances, 0, d->firstInstance);
        } else if (d->indirect) {
            SDL_DrawGPUIndexedPrimitivesIndirect(pass, W.commands, d->firstIndex * INDIRECT_SIZE, 1);
        } else {
            SDL_DrawGPUIndexedPrimitives(pass, d->count, 1, d->firstIndex, d->baseVertex, 0);
        }
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
    G.npins = 0;
    G.nsteps = G.ndraws = G.nuploads = G.frameUploads = 0;
    G.drawRecords.size = G.viewRecords.size = G.fogTables.size = G.shadowRecords.size = 0;
    G.vertexData.size = G.indexData.size = G.texels.size = 0;
    G.cameraView = G.shadowRecord = G.fogTable = -1;
    W.instances.size = 0;
    W.computeDue = W.frameDue = W.computed = 0;
    W.casterRecord = -1;
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

/* A flush whose command buffer did not reach the GPU: the textures it was to fill go up with the
   next one, and so do the world's records, slots and cells, which stay changed (world_flushed). */
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
    uint32_t oRecords, oViews, oFogs, oShadows, oVertices, oIndices, oTexels, total;
    long instances;
    unsigned char *mapped;
    size_t i;
    int ok = 1, downloaded = 0, shrink = 0;
    batch_flush();
    if (!G.dev)
        return 0;
    if (!G.nsteps && !G.nuploads && !W.nmeshUploads && !W.ntextureUploads && !extra->present && !extra->download) {
        frame_reset();
        return 1;
    }
    /* the world's arrays as large as their layers, its meshes and textures up through the stream,
       then what goes with the frame */
    if (world_on()) {
        world_arrays();
        if (!world_stream(1))
            world_failed("uploads");
        else if (!world_prepare())
            world_failed("buffers");
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
    if (!G.indexData.size)
        bytes_add(&G.indexData, 4, 4);
    if (!G.vertexData.size)
        bytes_add(&G.vertexData, 64, 16);
    instances = instances_needed((uint32_t)(G.drawRecords.size / sizeof(struct GpuDraw)));
    oRecords = 0;
    oViews = align16(oRecords + (uint32_t)G.drawRecords.size);
    oFogs = align16(oViews + (uint32_t)G.viewRecords.size);
    oShadows = align16(oFogs + (uint32_t)G.fogTables.size);
    oVertices = align16(oShadows + (uint32_t)G.shadowRecords.size);
    oIndices = align16(oVertices + (uint32_t)G.vertexData.size);
    oTexels = align16(oIndices + (uint32_t)G.indexData.size);
    total = oTexels + (uint32_t)G.texels.size;
    /* a transfer buffer far larger than the frames have needed for a while is made again as large
       as they need, and the staging texels' memory given back */
    if (G.upload && G.uploadSize > (4u << 20) && total <= G.uploadSize / 4) {
        if (++G.stagingQuiet >= STAGING_QUIET) {
            SDL_ReleaseGPUTransferBuffer(G.dev, G.upload);
            G.upload = NULL;
            G.uploadSize = 0;
            G.stagingQuiet = 0;
            shrink = 1;
        }
    } else {
        G.stagingQuiet = 0;
    }
    if (instances == -2
        || !ensure_buffer(&G.records, &G.recordsSize, (uint32_t)G.drawRecords.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.views, &G.viewsSize, (uint32_t)G.viewRecords.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.fogs, &G.fogsSize, (uint32_t)G.fogTables.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.shadows, &G.shadowsSize, (uint32_t)G.shadowRecords.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.vertices, &G.verticesSize, (uint32_t)G.vertexData.size, SDL_GPU_BUFFERUSAGE_VERTEX)
        || !ensure_buffer(&G.indices, &G.indicesSize, (uint32_t)G.indexData.size, SDL_GPU_BUFFERUSAGE_INDEX)) {
        SDL_Log("gfx: frame buffers: %s", SDL_GetError());
        goto failed;
    }
    if (!G.upload || total > G.uploadSize) {
        SDL_GPUTransferBufferCreateInfo info;
        uint32_t size = G.upload ? G.uploadSize : 1u << 20;
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
    copy_into(copy, oFogs, G.fogs, (uint32_t)G.fogTables.size);
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
    world_copy(copy, oTexels);
    if (run_uploads(copy, 0, G.frameUploads, oTexels)) {
        SDL_EndGPUCopyPass(copy);
        make_mipmaps(cmd, 0, G.frameUploads);
    } else {
        SDL_EndGPUCopyPass(copy);
    }
    world_compute(cmd);
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
    world_flushed();
    W.failures = 0;
    /* the world's frame lasts until the frame is shown */
    if (extra->present)
        W.frameSet = 0;
    frame_reset();
    if (shrink) {
        free(G.texels.data);
        G.texels.data = NULL;
        G.texels.capacity = 0;
    }
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

static SDL_GPUComputePipeline *make_compute(const struct GpuComputeCode *c)
{
    SDL_GPUComputePipelineCreateInfo info;
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
    return SDL_CreateGPUComputePipeline(G.dev, &info);
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
    G.vsCode[PROG_QUAD] = &quad_vs;
    G.fsCode[PROG_QUAD] = &quad_fs;
    G.vsCode[PROG_WORLD] = &world_vs;
    G.fsCode[PROG_WORLD] = &world_fs;
    G.vsCode[PROG_CASTER] = &world_caster_vs;
    G.fsCode[PROG_CASTER] = &world_caster_fs;
    G.vsCode[PROG_WATER] = &water_vs;
    G.fsCode[PROG_WATER] = &water_fs;
    props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_DEBUGMODE_BOOLEAN, debug && debug[0] == '1');
#ifdef __APPLE__
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_MSL_BOOLEAN, true);
#else
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
#endif
    /* the renderer clips depth, writes no clip distances and its indirect draws start at instance
       0: without those features Vulkan devices that lack them (older Android ones) run as well */
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
    W.transform = make_compute(&world_transform_cs);
    W.cullPipeline = make_compute(&world_cull_cs);
    W.compactPipeline = make_compute(&world_compact_cs);
    if (!G.pointClamp || !G.white || !G.noShadow || !make_samplers() || !upload_white() || !W.transform
        || !W.cullPipeline || !W.compactPipeline) {
        SDL_Log("gfx: device resources: %s", SDL_GetError());
        return 0;
    }
    G.frame = 1;
    G.cameraView = G.shadowRecord = G.fogTable = W.casterRecord = -1;
    W.generation++;
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
    /* the screen's samples and depth are known: every pipeline can be made */
    return make_pipelines() ? 0 : GFX_E_ZBUFFER;
}

int gfx_get_caps(struct C3dDevice *d, struct gfx_caps *out)
{
    (void)d;
    if (!G.dev)
        return -1;
    memset(out, 0, sizeof *out);
    out->zbufferless = 0;
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
        /* the world is made again for the next device (gfx_world_generation) */
        world_release();
        if (W.streamFence) {
            SDL_WaitForGPUFences(G.dev, true, &W.streamFence, 1);
            SDL_ReleaseGPUFence(G.dev, W.streamFence);
        }
        if (W.stream)
            SDL_ReleaseGPUTransferBuffer(G.dev, W.stream);
        W.streamFence = NULL;
        W.stream = NULL;
        W.streamSize = 0;
        if (W.white)
            SDL_ReleaseGPUTexture(G.dev, W.white);
        if (W.transform)
            SDL_ReleaseGPUComputePipeline(G.dev, W.transform);
        if (W.cullPipeline)
            SDL_ReleaseGPUComputePipeline(G.dev, W.cullPipeline);
        if (W.compactPipeline)
            SDL_ReleaseGPUComputePipeline(G.dev, W.compactPipeline);
        W.white = NULL;
        W.transform = W.cullPipeline = W.compactPipeline = NULL;
        W.frameSet = 0;
        W.generation++;
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
        if (G.noShadow) SDL_ReleaseGPUTexture(G.dev, G.noShadow);
        if (G.shadowMaps) SDL_ReleaseGPUTexture(G.dev, G.shadowMaps);
        if (G.shadowDepth) SDL_ReleaseGPUTexture(G.dev, G.shadowDepth);
        if (G.depth) SDL_ReleaseGPUTexture(G.dev, G.depth);
        if (G.msaa) SDL_ReleaseGPUTexture(G.dev, G.msaa);
        if (G.records) SDL_ReleaseGPUBuffer(G.dev, G.records);
        if (G.views) SDL_ReleaseGPUBuffer(G.dev, G.views);
        if (G.fogs) SDL_ReleaseGPUBuffer(G.dev, G.fogs);
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
    G.npipelines = 0;
    memset(G.quadPipeline, 0, sizeof G.quadPipeline);
    memset(G.worldPipeline, 0, sizeof G.worldPipeline);
    memset(G.waterPipeline, 0, sizeof G.waterPipeline);
    memset(G.effectPipeline, 0, sizeof G.effectPipeline);
    G.casterPipeline = NULL;
    memset(G.samplers, 0, sizeof G.samplers);
    memset(G.vs, 0, sizeof G.vs);
    memset(G.fs, 0, sizeof G.fs);
    G.pointClamp = NULL;
    G.white = G.noShadow = G.shadowMaps = G.shadowDepth = G.depth = G.msaa = NULL;
    G.records = G.views = G.fogs = G.shadows = G.vertices = G.indices = G.instances = NULL;
    G.recordsSize = G.viewsSize = G.fogsSize = G.shadowsSize = G.verticesSize = G.indicesSize = G.instanceCount = 0;
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
        /* a clear inside a pass: a quad over the rectangle that writes the colour, the depth or
           both */
        struct { float x, y, z, rhw; DWORD diffuse; float u, v; } v[4];
        struct gfx_surface *none[2] = { NULL, NULL };
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
        raw_draw(mode, &state, none, GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1 | FLAT_KIND, v, 4, NULL, 0);
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

void gfx_set_texture(int stage, gfx_surface *s)
{
    if ((unsigned)stage >= 2 || G.stageTex[stage] == s)
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
