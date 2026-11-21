/* The graphics device: render/gfx.h on SDL 3's GPU API, Vulkan with SPIR-V shaders on Windows and
   Linux, Metal with MSL on macOS, one shader format handed to SDL_CreateGPUDevice so that SDL has
   one driver to choose. The shaders are HLSL compiled offline (src/render/shaders, tools/shaders).
   It draws what it is handed: the client's quads and triangles, Granny's skinned meshes, the
   wobble, blits and clears, and the draws of the layers that record into its frames through
   render/gpu_layer.h (the static world's, render/gpu_world.c), which it knows only by their hooks.

   Draws are made in modes (render/draw_modes.h): the pipelines of every mode, for every kind of
   target it draws into, are made once, when the screen's samples and depth are known
   (gfx_create_zbuffer), and so are the samplers. A frame is recorded, not drawn: each draw becomes
   a record of what its mode and the draw itself give the shaders (struct GpuDraw, read from a
   storage buffer: a quad's through its own record in the frame's quads, a Granny actor's through
   its pose's header there, the wobble's through an instance-rate attribute that holds its index;
   a layer's draws as the layer has them) in a list of passes, and what it draws and the texels of
   the textures it uses since they changed are written once, into transfer buffers the frame's copy
   pass reads. The client's quads and triangles are instances (render/quads.h): a record each in
   the frame's quads, which the quad shader makes the vertices of, batched into draws while what
   they are drawn with stays the same; the sprite atlas's pages are layers of texture arrays, which
   draws batch across. Granny's models stay on the GPU once a draw has taken them up, and their
   actors are skinned by the skin shader in the poses the frame's quads hold (gfx_draw_skin).
   Textures keep their pixels on the CPU in the client's own format, so that the model loader's
   thread creates, locks and fills them without calling SDL; the main thread widens them to
   B8G8R8A8 when a draw first uses them after a change. Render targets live on the GPU, and nothing
   of them comes back to the CPU but the screen's picture when it is read.

   custom: a frame is recorded into a packet of its own, one of a ring (render/frame_ring.h), and
   handed whole to the device's render thread (render/render_thread.h, frame_run) when it is
   presented or the screen's picture is read (flush): the render thread records it into one command
   buffer, one copy pass, the layers' passes before the frame's (gpu_layer.h, the compute hook) and
   the frame's passes, and submits it, while the main thread records the next. A packet holds every
   GPU object its passes use as they were when it was handed over, so that the render thread reads
   nothing the main thread changes; what the main thread gives up goes once the next frame handed
   over has been submitted (retire). The main thread presents: it puts the screen, as the frame the
   render thread last finished left it, into the window's picture. Command buffers are recorded one
   at a time, the render thread's and the main thread's own alike (commands_take), as SDL moves
   textures and buffers in memory when a command buffer is submitted. */
#include "render/gfx.h"
#include "render/gpu_layer.h"
#include "render/dirty_rects.h"
#include "render/frame_ring.h"
#include "render/present_fit.h"
#include "render/render_thread.h"
#include "render/screen_wobble.h"
#include "render/texel_convert.h"
#include "render/skin_layout.h"
#include "render/shaders/shader_code.h"
#include "render/shaders/quad.h"
#include "render/shaders/skin.h"
#include "render/shaders/rows.h"
#include "core/compiler.h"
#include "diag/perf_counters.h"
#include "platform/clock.h"
#include <SDL3/SDL.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { K_SCREEN, K_SYSMEM, K_TEXTURE, K_RTTEX };

#define CLIENT_SURFACE_KEY 0x00ff00ffu

/* The device's own programs: the client's quads, the wobble's rows, Granny's skinned meshes. */
enum { PROG_QUAD, PROG_ROWS, PROG_SKIN, PROG_COUNT };
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
    struct DirtyRects dirty;  /* where the CPU's pixels are newer than tex, each to go up alone */
    int fresh;              /* a texture not yet sent: its pixels are zero but where dirty says, and
                               its GPU texture is cleared to that before they go up (clear_texels) */
    RECT lockRect;
    int mips;               /* the lower levels are to be made again */
    unsigned pinned;        /* the frame whose recorded commands use it */
    int dead;               /* released while pinned: freed after that frame */
    int held;               /* holds a reference of the holds (gfx_hold) */
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

/* custom: a Granny model (render/gfx.h, gfx_create_skin): its vertices and indices, kept on the CPU,
   and its buffers on the GPU, made and filled from them in the frame's upload when a draw first
   takes it (skin_ready), and again with a new device or after an upload that failed. */
struct gfx_skin {
    int refs;
    int held;         /* holds a reference of the holds (gfx_hold_skin) */
    unsigned pinned;  /* the frame whose recorded draws take it */
    int dead;         /* released while pinned: freed after that frame */
    int failed;       /* its buffers could not be made: draws take nothing of it, and it is not tried
                         again with this device */
    struct SkinVertex *vertices;
    uint32_t *indices;
    uint32_t vertexCount, indexCount;
    SDL_GPUBuffer *vertexBuffer, *indexBuffer;
    struct gfx_skin *prev, *next;
};

/* a model whose buffers the frame's upload fills, its vertices and then its indices at `at` of the
   frame's texels, staged when a draw first takes it (skin_ready) */
struct skin_upload {
    struct gfx_skin *model;
    SDL_GPUBuffer *vertices, *indices;
    uint32_t at, vertexBytes, indexBytes;
};

/* One recorded draw: of the frame's quads (PROG_QUAD, `count` instances from `record`, the first's
   place among the offsets of their records), the wobble's (`count` vertices, its draw record
   `record`), of a Granny model's (PROG_SKIN, `count` of its indices from firstIndex in `instances`
   poses, their headers' places among the quads' offsets from firstInstance, the model's buffers
   skinVertices and skinIndices), or a layer's (`layer`, its pipeline, textures, program and words
   in `d`, drawn by its draw hook). A draw of the device's own keeps its pipeline, textures and
   program in `d` too. */
struct gpu_draw {
    struct GpuLayerDraw d;
    int layer;  /* 0: the device's own; n: layer n - 1's */
    uint32_t firstIndex, count;
    uint32_t instances, firstInstance;
    uint32_t record;
    SDL_GPUBuffer *skinVertices, *skinIndices;
};

enum { STEP_PASS, STEP_UPLOADS, STEP_COPY };

/* A step of the frame: a render pass, the draws into one target; the uploads of textures changed
   after the frame's first draws; or a copy between GPU textures. */
struct gpu_step {
    int kind;
    /* STEP_PASS: the target, and what the render thread draws into, taken from it when the pass
       begins: its texture or the screen's multisampled one with the texture to resolve into, its
       depth, its size */
    struct gfx_surface *target;
    SDL_GPUTexture *colour, *resolve, *depth;
    int width, height;
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
    /* the layered texture's array, of which it is layer `layer`, and the array's size and levels */
    SDL_GPUTexture *array;
    uint32_t arrayW, arrayH, arrayLevels;
    uint32_t layer;
    int scratch;       /* tex is the layer's scratch texture */
    int fromLayer;     /* copies the layer's top level into its scratch texture, not texels */
    uint32_t x, y, w, h;
    uint32_t offset;   /* into the staging texels */
    int mips;
};

/* GPU objects given up while a frame was recorded, which earlier frames may still record into
   their command buffers: released once the frame is submitted (release_retired) */
struct retired {
    SDL_GPUTexture *texture;
    SDL_GPUBuffer *buffer;
};

/* the frame's per-frame buffers, which its lists go into */
struct frame_buffers {
    SDL_GPUBuffer *records, *rows, *quads, *offsets, *instances;
};

#define PIPELINE_MAX 256

/* What a draw is made with, of its mode and gfx_set_effect and gfx_set_fog: draws are batched while
   it stays the same, as the Direct3D 9 client's were while it changed no state, and a change
   counts as a mode sent (diag/perf.h). The blend factors are the D3DBLEND numbers given, whole. */
struct draw_state {
    int src, dst;
    unsigned char combine, depth, alphaTest, alphaRef, fog, colour, address[2], filter, lightmap, into;
};


/* How a frame went: drawn, not handed over whole (its uploads put back at once), or not submitted
   by the render thread (its uploads put back when it is retired). */
enum { FRAME_DRAWN, FRAME_DROPPED, FRAME_UNSUBMITTED };

/* A frame's packet, a slot of the ring (render/frame_ring.h): what the main thread records, which
   it hands to the render thread (frame_run) whole, and takes back once the render thread has
   submitted it (frame_retire). While the render thread has it, the main thread writes none of it,
   and the render thread reads nothing of the main thread's but the device's fixed objects. */
struct gpu_frame {
    unsigned number;  /* G.frame while it was recorded */
    struct gpu_step *steps;
    size_t nsteps, stepCapacity;
    struct gpu_draw *draws;
    size_t ndraws, drawCapacity;
    struct gpu_upload *uploads;
    size_t nuploads, uploadCapacity, frameUploads;  /* frameUploads: those of the first copy pass */
    /* what goes up: the draw records, the wobble's tables of rows, the instances' records with the
       matrices and vertices they name (render/quads.h), the row each record starts at, and the
       texels with everything else */
    struct GpuBytes records, rows, quads, offsets, texels;
    /* the layers' parts of it (struct GpuLayer, partSize) */
    void *parts[GPU_LAYERS_MAX];
    /* the surfaces it draws with, which a release while it is recorded leaves until it is handed
       over (frame_unpin) */
    struct gfx_surface **pins;
    size_t npins, pinCapacity;
    struct skin_upload *skinUploads;
    size_t nskinUploads, skinUploadCapacity;
    /* blocks of CLEAR_TEXELS like texels among the staged ones (clear_texels): the texel and the
       block's offset plus one, 0 for none */
    struct { uint32_t texel; long at; } clearBlocks[2];
    /* the GPU objects given up while it was recorded, which the render thread releases once it has
       submitted it */
    struct retired *retired;
    size_t nretired, retiredCapacity;
    /* handed over with it: its buffers, the identity instances where they go up with it (at that
       offset of its texels, -1 for none), a download of the screen (gfx_read_frame) */
    struct frame_buffers buffers;
    long instancesAt;
    uint32_t instanceCount;
    struct {
        SDL_GPUTexture *texture;
        SDL_GPUTransferBuffer *buffer;
        int x, y, w, h;
        unsigned char *out;
        size_t pitch;
    } download;
    int drawsScreen;  /* a pass of it draws into the screen */
    /* the render thread's: the frame's fence, how it went, and whether the download reached `out` */
    SDL_GPUFence *fence;
    int state, downloaded;
};

static struct {
    SDL_Window *window;
    SDL_GPUDevice *dev;
    int claimed, claimLogged;  /* the window takes the device's pictures (claim_window) */
    SDL_Mutex *lock;
    SDL_AtomicInt contentTick;
    struct gfx_surface *handles;
    /* GPU objects given up since the last frame was handed over, which goes with the next */
    struct retired *retiring;
    size_t nretiring, retiringCapacity;

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

    /* fixed resources */
    SDL_GPUShader *vs[PROG_COUNT], *fs[PROG_COUNT];
    /* every pipeline, the layers' too, made once (gpu_pipeline), and which of them serve what of
       the device's: a mode's quads into each kind of target, flat or not; its Granny models; the
       effect modes' quads by depth test or none and blend factors; the wobble */
    struct { struct GpuPipe desc; SDL_GPUGraphicsPipeline *pipeline; } pipelines[PIPELINE_MAX];
    int npipelines;
    SDL_GPUGraphicsPipeline *quadPipeline[DRAW_MODE_COUNT][CLASS_COUNT][2];
    SDL_GPUGraphicsPipeline *skinPipeline[DRAW_MODE_COUNT];
    SDL_GPUGraphicsPipeline *effectPipeline[2][DRAW_FACTOR_COUNT][DRAW_FACTOR_COUNT];
    SDL_GPUGraphicsPipeline *rowsPipeline;
    SDL_GPUSampler *samplers[2][3];  /* wrapped or not, by enum DrawFilter */
    SDL_GPUSampler *pointClamp;
    SDL_GPUTexture *white;
    SDL_GPUTexture *frameCopy;  /* the screen's picture as the wobble found it (gfx_wobble) */
    int frameCopyW, frameCopyH;
    /* the frames' buffers as large as the frames have needed, which each frame takes as it goes */
    struct frame_buffers buffers;
    uint32_t recordsSize, rowsSize, quadsSize, offsetsSize, instanceCount;
    SDL_GPUBuffer *corners, *triangles;  /* quad_corners and quad_triangles, uploaded once */
    struct gpu_array *arrays;  /* every array of layered textures */
    SDL_GPUTexture *noAtlas;   /* the array a quad draw binds where stage 0 reads no layer */

    /* the quads given since their draw was last made (batch_flush): the draw's record and its
       instances, whether they are flat, the textures they read, stage 0's array where it reads
       layers, and the layered textures they read of it */
    struct {
        int open, flat;
        uint32_t record, first, count, prims, bytes;
        struct gfx_surface *tex[2];
        struct gpu_array *array;
        struct gfx_surface **layers;
        size_t nlayers, layerCapacity;
    } batch;
    /* the last matrix of each kind put among the frame's quads, at row matrixIndex: [0] a
       projection alone (points in view space), [1] a view and its projection */
    float matrix[2][16];
    int matrixIndex[2];
    /* the Granny models given since their draw was last made (batch_flush, skin_batch_join): the
       draw's record, the model and the textures, the range drawn and the poses that draw it, whose
       headers' places among the quads' offsets begin at firstInstance; every model */
    struct {
        int open;
        uint32_t record, firstInstance, prims, bytes;
        struct gfx_skin *model;
        struct gfx_surface *tex[2];
        struct SkinBatch range;
    } skinBatch;
    struct gfx_skin *skins;
    /* where a frame's download of the screen goes (gfx_read_frame) */
    SDL_GPUTransferBuffer *download;
    uint32_t downloadSize;
    Uint64 presentedNs;  /* when the window last took a picture */
    int screenShown;     /* a frame drawn into the screen as it is has been submitted */

    /* the frames: the ring's packets and the render thread that submits them (frame_run); the
       number of the frame being recorded, which pins what it draws with, and the packet it is
       recorded into */
    struct gpu_frame frames[FRAME_RING_SLOTS];
    struct RenderThread *thread;
    SDL_Mutex *recording;  /* held while a command buffer is recorded (commands_take) */
    unsigned frame;
    struct gpu_frame *f;
    /* the surfaces and Granny models held, one reference each, until gfx_release_holds (gfx_hold,
       gfx_hold_skin) */
    struct gfx_surface **holds;
    size_t nholds, holdCapacity;
    struct gfx_skin **skinHolds;
    size_t nskinHolds, skinHoldCapacity;
    int unresolving;  /* the draw being recorded puts the resolved screen back into the multisampled one */
    /* the layers that record into the frames (gpu_add_layer) */
    const struct GpuLayer *layers[GPU_LAYERS_MAX];
    int nlayers;
} G;

static struct PresentCover present_cover;

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

int gpu_grow(void **items, size_t *capacity, size_t need, size_t size)
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

/* the smallest transfer buffer a frame's stream is given */
#define STAGE_MIN 65536u
/* a stream's transfer buffer of more than 4 MB whose frames have needed a quarter of it at most
   this many times in a row is made again as small as they need (frame_begin); each packet keeps
   its own, so that is this many of its frames, one in FRAME_RING_SLOTS of the client's */
#define STAGING_QUIET 120

/* A staged stream's transfer buffer, unmapped and released; it is the frame's own, which neither
   the GPU nor the render thread has while the main thread records into it. */
static void stage_release(struct GpuBytes *b)
{
    if (b->buffer && G.dev) {
        if (b->data)
            SDL_UnmapGPUTransferBuffer(G.dev, b->buffer);
        SDL_ReleaseGPUTransferBuffer(G.dev, b->buffer);
    }
    b->buffer = NULL;
    b->data = NULL;
    b->capacity = 0;
}

/* b made to hold `need` bytes, with what the frame wrote into it so far: a staged stream's transfer
   buffer made again larger and mapped, other bytes grown on the CPU. 0 where it cannot be. Kept out
   of bytes_at, which every record of a frame goes through: inlined there, its locals and calls
   cost each of those calls a frame the size of this function's. */
static RAGE_NOINLINE int bytes_grow(struct GpuBytes *b, size_t need)
{
    SDL_GPUTransferBufferCreateInfo info;
    SDL_GPUTransferBuffer *made;
    unsigned char *data = NULL;
    size_t c = b->capacity ? b->capacity : STAGE_MIN;
    if (!b->staged)
        return gpu_grow((void **)&b->data, &b->capacity, need, 1);
    while (c < need) {
        if (c > UINT32_MAX / 2)
            return 0;
        c *= 2;
    }
    SDL_zero(info);
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    info.size = (Uint32)c;
    made = G.dev ? SDL_CreateGPUTransferBuffer(G.dev, &info) : NULL;
    if (made && !(data = SDL_MapGPUTransferBuffer(G.dev, made, false)))
        SDL_ReleaseGPUTransferBuffer(G.dev, made);
    if (!data) {
        SDL_Log("gfx: a transfer buffer of %u bytes: %s", (unsigned)c, SDL_GetError());
        return 0;
    }
    if (b->size)
        memcpy(data, b->data, b->size);
    stage_release(b);
    b->buffer = made;
    b->data = data;
    b->capacity = c;
    return 1;
}

/* `size` bytes at an `align`-aligned offset of b, zeroed, or with `zero` 0 left for the caller to
   write whole (the padding before them zeroed); answers the offset, or -1 */
static long bytes_at(struct GpuBytes *b, size_t size, size_t align, int zero)
{
    size_t at = (b->size + align - 1) & ~(align - 1);
    if (at + size > b->capacity && !bytes_grow(b, at + size))
        return -1;
    if (zero)
        memset(b->data + b->size, 0, at + size - b->size);
    else if (at > b->size)
        memset(b->data + b->size, 0, at - b->size);
    b->size = at + size;
    return (long)at;
}

long gpu_bytes_add(struct GpuBytes *b, size_t size, size_t align)
{
    return bytes_at(b, size, align, 1);
}

long gpu_bytes_take(struct GpuBytes *b, size_t size, size_t align)
{
    return bytes_at(b, size, align, 0);
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

/* A GPU object given up while frames recorded before may still use it goes with the next frame
   handed over, and the render thread releases it once that frame is submitted (frame_run): by then
   every frame that could record it into a command buffer has, and SDL keeps what a submitted
   command buffer uses until the GPU is done with it. The model loader's thread may give a texture
   up too. */
void gpu_retire(SDL_GPUTexture *texture, SDL_GPUBuffer *buffer)
{
    if (!texture && !buffer)
        return;
    lock_handles();
    if (gpu_grow((void **)&G.retiring, &G.retiringCapacity, G.nretiring + 1, sizeof *G.retiring)) {
        G.retiring[G.nretiring].texture = texture;
        G.retiring[G.nretiring].buffer = buffer;
        G.nretiring++;
    }
    unlock_handles();
}

static void retire_texture(SDL_GPUTexture *texture)
{
    gpu_retire(texture, NULL);
}

static void retire_buffer(SDL_GPUBuffer *buffer)
{
    gpu_retire(NULL, buffer);
}

static void release_retired(struct retired *list, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        if (list[i].texture)
            SDL_ReleaseGPUTexture(G.dev, list[i].texture);
        if (list[i].buffer)
            SDL_ReleaseGPUBuffer(G.dev, list[i].buffer);
    }
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

/* whether the quads, or Granny's draws, given since their draw was last made read s */
static int batch_reads(const struct gfx_surface *s)
{
    size_t i;
    if (G.skinBatch.open && (G.skinBatch.tex[0] == s || G.skinBatch.tex[1] == s))
        return 1;
    if (!G.batch.open)
        return 0;
    if (G.batch.tex[0] == s || G.batch.tex[1] == s)
        return 1;
    for (i = 0; i < G.batch.nlayers; i++)
        if (G.batch.layers[i] == s)
            return 1;
    return 0;
}

/* A surface the frame being recorded draws with goes when the frame is handed over (frame_unpin);
   the render thread reads no surface, only the GPU objects the frame's packet holds, and those go
   later (retire). */
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

void gfx_release(gfx_surface *s)
{
    if (s && --s->refs <= 0)
        free_handle(s);
}

/* A hold is a reference, taken and let go by the thread that draws (render/rp_pool.c). */
void gfx_hold(gfx_surface *s)
{
    if (!s || s->held)
        return;
    if (!gpu_grow((void **)&G.holds, &G.holdCapacity, G.nholds + 1, sizeof *G.holds))
        abort(); /* not held, it could go before it is drawn */
    s->held = 1;
    s->refs++;
    G.holds[G.nholds++] = s;
}

void gfx_release_holds(void)
{
    size_t i, n = G.nholds;

    G.nholds = 0;
    for (i = 0; i < n; i++) {
        struct gfx_surface *s = G.holds[i];

        s->held = 0;
        gfx_release(s);
    }
    n = G.nskinHolds;
    G.nskinHolds = 0;
    for (i = 0; i < n; i++) {
        struct gfx_skin *k = G.skinHolds[i];

        k->held = 0;
        gfx_skin_release(k);
    }
}

/* s stays until the frame being recorded is handed over (frame_unpin). */
static void pin(struct gfx_surface *s)
{
    struct gpu_frame *f = G.f;
    if (!s || s->pinned == G.frame)
        return;
    if (!gpu_grow((void **)&f->pins, &f->pinCapacity, f->npins + 1, sizeof *f->pins))
        return;
    lock_handles();
    s->pinned = G.frame;
    unlock_handles();
    f->pins[f->npins++] = s;
}

static void whole_rect(RECT *r, const struct gfx_surface *s)
{
    r->left = 0;
    r->top = 0;
    r->right = s->width;
    r->bottom = s->height;
}

/* The rectangle r of s (the whole of it for NULL) newer on the CPU than on the GPU. Called with
   the handles locked. */
static void dirty_locked(struct gfx_surface *s, const RECT *r)
{
    RECT all;
    if (!r) {
        whole_rect(&all, s);
        s->dirty.count = 0;
        r = &all;
    }
    dirty_rects_add(&s->dirty, r);
}

static void mark_dirty(struct gfx_surface *s, const RECT *r)
{
    lock_handles();
    dirty_locked(s, r);
    unlock_handles();
}

/* Command buffers are recorded one at a time. SDL 3.4's Vulkan driver moves textures and buffers
   to other memory when a command buffer is submitted (on every submit while no window is claimed,
   on those that present while one is), copying their contents in the command buffer submitted and
   pointing their handles at the new memory, and leaves only those with an upload or download
   pending where they are. A render pass or a bind takes a texture's or a buffer's memory as it is
   recorded, so a submit while another command buffer is being recorded could leave that one's
   earlier passes drawing into the old memory and its later ones reading the new. So the render
   thread holds the device's recording from acquiring a frame's command buffer to submitting it,
   and the main thread while it records and submits one of its own and while SDL makes a texture,
   for which SDL submits a command buffer of its own. Acquiring a command buffer or a swapchain
   texture names no texture or buffer of the device's and needs no hold. SDL's mutexes may be taken
   again by the thread holding them. */
static void commands_take(void)
{
    SDL_LockMutex(G.recording);
}

static void commands_give(void)
{
    SDL_UnlockMutex(G.recording);
}

/* A command buffer of the main thread's, recorded while it is held; NULL where it could not be had. */
SDL_GPUCommandBuffer *gpu_commands(void)
{
    SDL_GPUCommandBuffer *cmd;
    commands_take();
    cmd = SDL_AcquireGPUCommandBuffer(G.dev);
    if (!cmd)
        commands_give();
    return cmd;
}

/* Submits a command buffer of gpu_commands, with a fence where `fence` is given; 0 where it could
   not be submitted. */
int gpu_submit(SDL_GPUCommandBuffer *cmd, SDL_GPUFence **fence)
{
    int ok;
    if (fence)
        ok = (*fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd)) != NULL;
    else
        ok = SDL_SubmitGPUCommandBuffer(cmd);
    commands_give();
    return ok;
}

SDL_GPUTexture *gpu_make_texture(const SDL_GPUTextureCreateInfo *info)
{
    SDL_GPUTexture *t;
    commands_take();
    t = SDL_CreateGPUTexture(G.dev, info);
    commands_give();
    return t;
}

SDL_GPUTexture *gpu_new_texture(int w, int h, int levels, SDL_GPUTextureUsageFlags usage)
{
    SDL_GPUTextureCreateInfo info;
    SDL_zero(info);
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = GPU_TARGET_FORMAT;
    info.usage = usage;
    info.width = (Uint32)w;
    info.height = (Uint32)h;
    info.layer_count_or_depth = 1;
    info.num_levels = (Uint32)levels;
    info.sample_count = SDL_GPU_SAMPLECOUNT_1;
    return gpu_make_texture(&info);
}

/* A target's texture (GPU_MARGIN). */
static SDL_GPUTexture *new_target_texture(int w, int h)
{
    return gpu_new_texture(w + GPU_MARGIN, h + GPU_MARGIN, 1,
                           SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER);
}

/* ---- the frame's lists ---- */

static struct gpu_step *add_step(int kind)
{
    struct gpu_frame *f = G.f;
    struct gpu_step *s;
    if (!gpu_grow((void **)&f->steps, &f->stepCapacity, f->nsteps + 1, sizeof *f->steps))
        return NULL;
    s = &f->steps[f->nsteps++];
    memset(s, 0, sizeof *s);
    s->kind = kind;
    return s;
}

static struct gpu_step *last_step(void)
{
    return G.f->nsteps ? &G.f->steps[G.f->nsteps - 1] : NULL;
}

static int pass_has_depth(const struct gpu_step *p)
{
    return !p->target ? 1 : p->target == G.screen && G.depth != NULL;
}

static int pass_samples(const struct gpu_step *p)
{
    return p->target && p->target == G.screen && G.msaa ? (int)G.samples : (int)SDL_GPU_SAMPLECOUNT_1;
}

/* What the render thread draws a pass into (run_pass), from its target as it is when the pass
   begins: a target's textures change only where everything recorded before has gone (gfx_resize). */
static void pass_targets(struct gpu_step *s)
{
    int ms = s->target == G.screen && G.msaa;
    s->colour = ms ? G.msaa : s->target->tex;
    s->resolve = ms && !s->unresolve ? s->target->tex : NULL;
    s->depth = pass_has_depth(s) ? G.depth : NULL;
    s->width = s->target->width;
    s->height = s->target->height;
}

/* The pass the next draw goes into: the current target's. */
static struct gpu_step *current_pass(void)
{
    struct gpu_step *s = last_step();
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
    s->first = (uint32_t)G.f->ndraws;
    pass_targets(s);
    pin(s->target);
    return s;
}

static struct gpu_draw *add_draw(struct gpu_step *pass)
{
    struct gpu_frame *f = G.f;
    struct gpu_draw *d;
    if (!gpu_grow((void **)&f->draws, &f->drawCapacity, f->ndraws + 1, sizeof *f->draws))
        return NULL;
    d = &f->draws[f->ndraws++];
    memset(d, 0, sizeof *d);
    pass->count++;
    return d;
}

/* A draw record among the frame's (`records`, which the shaders read), or among a layer's own
   (those its draws push as their uniforms). */
uint32_t gpu_add_record_to(struct GpuBytes *b, struct GpuDraw **out)
{
    long at = gpu_bytes_add(b, sizeof(struct GpuDraw), 16);
    if (at < 0) {
        *out = NULL;
        return 0;
    }
    *out = (struct GpuDraw *)(b->data + at);
    return (uint32_t)(at / (long)sizeof(struct GpuDraw));
}

static uint32_t add_record(struct GpuDraw **out)
{
    return gpu_add_record_to(&G.f->records, out);
}

struct GpuBytes *gpu_records(void)
{
    return &G.f->records;
}

struct GpuBytes *gpu_texels(void)
{
    return &G.f->texels;
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
        info.format = GPU_TARGET_FORMAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)s->width;
        info.height = (Uint32)s->height;
        info.layer_count_or_depth = (Uint32)a->capacity;
        info.num_levels = (Uint32)s->levels;
        info.sample_count = SDL_GPU_SAMPLECOUNT_1;
        a->tex = a->taken ? gpu_make_texture(&info) : NULL;
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
    if (!s->fresh)
        dirty_locked(s, NULL);
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
    s->tex = gpu_new_texture(s->width, s->height, s->levels,
                         SDL_GPU_TEXTUREUSAGE_SAMPLER | (s->levels > 1 ? SDL_GPU_TEXTUREUSAGE_COLOR_TARGET : 0));
    if (!s->tex) {
        SDL_Log("gfx: SDL_CreateGPUTexture %dx%d: %s", s->width, s->height, SDL_GetError());
        s->failed = 1;
        return NULL;
    }
    if (!s->fresh)
        mark_dirty(s, NULL);
    return s->tex;
}

/* Copies the CPU's pixels of a rectangle into the staging texels, widened to B8G8R8A8. */
static int stage_texels(struct gfx_surface *s, const RECT *r, uint32_t *offset)
{
    int w = r->right - r->left, h = r->bottom - r->top;
    int bytes = s->kind == K_SYSMEM ? 4 : texel_bytes(s->fmt);
    long at;
    const unsigned char *src;
    int64_t start;
    if (w <= 0 || h <= 0 || !s->pixels)
        return 0;
    at = gpu_bytes_take(&G.f->texels, (size_t)w * h * 4, 16);
    if (at < 0)
        return 0;
    src = s->pixels + (size_t)r->top * s->width * bytes + (size_t)r->left * bytes;
    start = platform_stopwatch_ns();
    if (s->kind == K_SYSMEM)
        texel_rows_keyed_to_bgra(src, (long)s->width * 4, w, h, s->key, G.f->texels.data + at, (long)w * 4);
    else
        texel_rows_to_bgra(s->fmt, src, (long)s->width * bytes, w, h, G.f->texels.data + at, (long)w * 4);
    g_perf.texelsNs += platform_stopwatch_ns() - start;
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
   the frame has got to, in a copy pass of its own between the render passes. Answers the record,
   or NULL without memory for it. */
static struct gpu_upload *add_upload(struct gfx_surface *s, SDL_GPUTexture *tex, uint32_t x, uint32_t y, uint32_t w,
                                     uint32_t h, uint32_t offset, int mips)
{
    struct gpu_frame *f = G.f;
    struct gpu_upload *u;
    int late = s->pinned == G.frame;
    if (!gpu_grow((void **)&f->uploads, &f->uploadCapacity, f->nuploads + 1, sizeof *f->uploads))
        return NULL;
    if (!late) {
        /* the first copy pass's uploads stay at the front of the list */
        memmove(f->uploads + f->frameUploads + 1, f->uploads + f->frameUploads,
                (f->nuploads - f->frameUploads) * sizeof *f->uploads);
        u = &f->uploads[f->frameUploads++];
        f->nuploads++;
        {
            size_t i;
            for (i = 0; i < f->nsteps; i++)
                if (f->steps[i].kind == STEP_UPLOADS)
                    f->steps[i].first++;
        }
    } else {
        struct gpu_step *step = last_step();
        if (!step || step->kind != STEP_UPLOADS) {
            step = add_step(STEP_UPLOADS);
            if (!step)
                return NULL;
            step->first = (uint32_t)f->nuploads;
        }
        step->count++;
        u = &f->uploads[f->nuploads++];
    }
    u->surface = s;
    u->tex = tex;
    u->array = s->layered ? s->array->tex : NULL;
    u->arrayW = (uint32_t)s->width;
    u->arrayH = (uint32_t)s->height;
    u->arrayLevels = (uint32_t)s->levels;
    u->layer = s->layered ? (uint32_t)s->layer : 0;
    u->scratch = s->layered && tex != s->array->tex;
    u->fromLayer = 0;
    u->x = x;
    u->y = y;
    u->w = w;
    u->h = h;
    u->offset = offset;
    u->mips = mips;
    return u;
}

/* The texels of the rectangle r of s, clipped to it, staged and recorded for tex at r's place;
   answers 0 where they could not be (no memory) and 1 where they were or r lies outside s. */
static int upload_rect(struct gfx_surface *s, SDL_GPUTexture *tex, RECT r)
{
    uint32_t offset;
    if (!clip_rect(&r, s))
        return 1;
    return stage_texels(s, &r, &offset)
           && add_upload(s, tex, (uint32_t)r.left, (uint32_t)r.top, (uint32_t)(r.right - r.left),
                         (uint32_t)(r.bottom - r.top), offset, 0);
}

#define CLEAR_TEXELS 65536

/* Uploads that set every texel of tex's top level (s's size) to the one s's pixels widen to where
   they are zero, from one block of CLEAR_TEXELS such texels among the frame's staged ones, a band
   of whole rows at a time: what a texture not yet sent holds but where it was written. Answers 0
   without memory for them. */
static int clear_texels(struct gfx_surface *s, SDL_GPUTexture *tex)
{
    struct gpu_frame *f = G.f;
    uint32_t texel = texel_unpack(s->fmt, 0), rows, y;
    long at;
    int i;
    if (s->width > CLEAR_TEXELS)
        return 0;
    for (i = 0; i < 2 && !(f->clearBlocks[i].at && f->clearBlocks[i].texel == texel); i++)
        ;
    if (i == 2) {
        uint32_t *block;
        long k;
        i = f->clearBlocks[0].at ? 1 : 0;
        at = gpu_bytes_take(&f->texels, CLEAR_TEXELS * 4, 16);
        if (at < 0)
            return 0;
        block = (uint32_t *)(f->texels.data + at);
        for (k = 0; k < CLEAR_TEXELS; k++)
            block[k] = texel;
        f->clearBlocks[i].texel = texel;
        f->clearBlocks[i].at = at + 1;
    }
    at = f->clearBlocks[i].at - 1;
    rows = CLEAR_TEXELS / (uint32_t)s->width;
    for (y = 0; y < (uint32_t)s->height; y += rows) {
        uint32_t h = (uint32_t)s->height - y < rows ? (uint32_t)s->height - y : rows;
        if (!add_upload(s, tex, 0, y, (uint32_t)s->width, h, (uint32_t)at, 0))
            return 0;
    }
    return 1;
}

/* Whether one of the n rectangles covers all of s */
static int covers(const struct gfx_surface *s, const RECT *rects, int n)
{
    int i;
    for (i = 0; i < n; i++)
        if (rects[i].left <= 0 && rects[i].top <= 0 && rects[i].right >= s->width && rects[i].bottom >= s->height)
            return 1;
    return 0;
}

/* A layered texture's levels made anew from its top level, in a scratch texture of its size and
   levels that goes once the frame is drawn (struct gpu_upload): unless one of the n rectangles
   covers all of it, the layer's top level copied into it on the GPU, or for a texture not yet sent
   the texels it holds unwritten (clear_texels); the rectangles' texels over that; then its levels
   made and each copied into the layer (make_mipmaps). Answers 0 without memory for it. */
static int layer_mipmaps(struct gfx_surface *s, const RECT *rects, int n, int fresh)
{
    SDL_GPUTexture *scratch;
    struct gpu_upload *copy;
    int i, ok = 1;
    scratch = gpu_new_texture(s->width, s->height, s->levels,
                              SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET);
    if (!scratch)
        return 0;
    if (!covers(s, rects, n)) {
        if (fresh) {
            ok = clear_texels(s, scratch);
        } else {
            copy = add_upload(s, scratch, 0, 0, (uint32_t)s->width, (uint32_t)s->height, 0, 0);
            if (copy)
                copy->fromLayer = 1;
            ok = copy != NULL;
        }
    }
    for (i = 0; i < n && ok; i++)
        ok = upload_rect(s, scratch, rects[i]);
    ok = ok && add_upload(s, scratch, 0, 0, 0, 0, 0, 1);
    retire_texture(scratch);
    return ok;
}

/* The GPU texture a draw samples for a surface (a layered texture's array), with the CPU's changes
   since its last upload staged, each of its dirty rectangles alone (a texture not yet sent cleared
   first, clear_texels), and its levels made again where it has them; the surface stays alive until
   the frame is drawn. */
static SDL_GPUTexture *texture_for_draw(struct gfx_surface *s)
{
    struct DirtyRects dirty;
    int mips, fresh, i;
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
    fresh = s->fresh;
    mips = s->levels > 1 && (s->mips || dirty.count > 0 || fresh);
    s->dirty.count = 0;
    s->fresh = 0;
    s->mips = 0;
    unlock_handles();
    if (s->layered && mips) {
        if (!layer_mipmaps(s, dirty.rect, dirty.count, fresh)) {
            /* staged by a later draw, all of it where the texture was not sent yet */
            if (fresh)
                mark_dirty(s, NULL);
            for (i = 0; i < dirty.count; i++)
                mark_dirty(s, &dirty.rect[i]);
            lock_handles();
            s->mips = 1;
            unlock_handles();
        }
        pin(s);
        return tex;
    }
    if (fresh && !covers(s, dirty.rect, dirty.count) && !clear_texels(s, tex)) {
        /* no room for the block: the texture's texels go up, all of them */
        dirty.count = 1;
        whole_rect(&dirty.rect[0], s);
    }
    for (i = 0; i < dirty.count; i++)
        if (!upload_rect(s, tex, dirty.rect[i]))
            mark_dirty(s, &dirty.rect[i]);  /* staged by a later draw */
    if (mips && !add_upload(s, tex, 0, 0, 0, 0, 0, 1)) {
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

SDL_GPUSampler *gpu_stage_sampler(int address, int filter, int stage)
{
    return G.samplers[address == DRAW_ADDRESS_WRAP][stage ? DRAW_FILTER_LINEAR : filter];
}

static SDL_GPUSampler *stage_sampler(const struct draw_state *s, int stage)
{
    return gpu_stage_sampler(s->address[stage], s->filter, stage);
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

static SDL_GPUGraphicsPipeline *create_pipeline(const struct GpuPipe *p)
{
    SDL_GPUGraphicsPipelineCreateInfo info;
    SDL_GPUColorTargetDescription colour[2];
    SDL_zero(info);
    SDL_zero(colour);
    info.vertex_shader = p->vs;
    info.fragment_shader = p->fs;
    info.vertex_input_state.vertex_buffer_descriptions = p->buffers;
    info.vertex_input_state.num_vertex_buffers = p->nbuffers;
    info.vertex_input_state.vertex_attributes = p->attrs;
    info.vertex_input_state.num_vertex_attributes = p->nattrs;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    /* Direct3D 9 culls by the winding on the screen, y down, as the target's pixels are laid out
       here too: GPU_CULL_CW drops the clockwise triangles, GPU_CULL_CCW the others */
    info.rasterizer_state.cull_mode = p->cull == GPU_CULL_NONE ? SDL_GPU_CULLMODE_NONE : SDL_GPU_CULLMODE_BACK;
    info.rasterizer_state.front_face = p->cull == GPU_CULL_CW ? SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
                                                              : SDL_GPU_FRONTFACE_CLOCKWISE;
    info.rasterizer_state.enable_depth_clip = true;
    info.multisample_state.sample_count = (SDL_GPUSampleCount)p->samples;
    info.multisample_state.enable_alpha_to_coverage = p->a2c != 0;
    info.depth_stencil_state.enable_depth_test = p->depth != DRAW_DEPTH_OFF;
    info.depth_stencil_state.enable_depth_write = p->depth == DRAW_DEPTH_WRITE || p->depth == DRAW_DEPTH_ALWAYS;
    info.depth_stencil_state.compare_op = p->depth == DRAW_DEPTH_ALWAYS ? SDL_GPU_COMPAREOP_ALWAYS
                                                                        : SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    colour[0].format = p->r32f ? SDL_GPU_TEXTUREFORMAT_R32_FLOAT : GPU_TARGET_FORMAT;
    colour[0].blend_state.enable_blend = p->blend != 0;
    colour[0].blend_state.src_color_blendfactor = sdl_factor(p->src);
    colour[0].blend_state.dst_color_blendfactor = sdl_factor(p->dst);
    colour[0].blend_state.src_alpha_blendfactor = colour[0].blend_state.src_color_blendfactor;
    colour[0].blend_state.dst_alpha_blendfactor = colour[0].blend_state.dst_color_blendfactor;
    colour[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colour[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    colour[0].blend_state.enable_color_write_mask = true;
    colour[0].blend_state.color_write_mask = p->mask == GPU_MASK_NONE ? 0
        : p->mask == GPU_MASK_RGB ? (SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B)
        : (SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A);
    /* a second target, written in red alone where `back` */
    colour[1] = colour[0];
    colour[1].blend_state.color_write_mask = p->back ? SDL_GPU_COLORCOMPONENT_R : 0;
    info.target_info.color_target_descriptions = colour;
    info.target_info.num_color_targets = p->targets == 2 ? 2 : 1;
    info.target_info.has_depth_stencil_target = p->hasDepth != 0;
    info.target_info.depth_stencil_format = G.depthFormat;
    return SDL_CreateGPUGraphicsPipeline(G.dev, &info);
}

/* The pipeline of a description, made once: modes that differ in nothing a pipeline holds share
   theirs. */
SDL_GPUGraphicsPipeline *gpu_pipeline(struct GpuPipe *p)
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
   takes the draw's), depth, cull (none), colour mask, samples, and alpha to coverage. */
static void mode_state(struct GpuPipe *p, const struct DrawModeDesc *m, int cls, int src, int dst, int flat)
{
    p->blend = m->src != DRAW_BLEND_OFF;
    if (p->blend)
        draw_blend_factors(m->src == DRAW_BLEND_FROM_DRAW ? src : m->src, m->dst == DRAW_BLEND_FROM_DRAW ? dst : m->dst,
                           &p->src, &p->dst);
    p->depth = m->depth;
    p->cull = GPU_CULL_NONE;
    /* the screen is X8R8G8B8 on Direct3D 9: its alpha stays 1 */
    p->mask = !m->colour ? GPU_MASK_NONE : cls == CLASS_TEXTURE ? GPU_MASK_RGBA : GPU_MASK_RGB;
    p->hasDepth = cls == CLASS_SCREEN && G.depth != NULL;
    p->samples = cls == CLASS_SCREEN ? (int)G.samples : (int)SDL_GPU_SAMPLECOUNT_1;
    p->a2c = alpha_to_coverage(m, p->samples, flat);
}

void gpu_pipe_mode(struct GpuPipe *p, const struct DrawModeDesc *m)
{
    mode_state(p, m, CLASS_SCREEN, 0, 0, 0);
}

#define ATTR(p, loc, slot, fmt, off) ((p)->attrs[(p)->nattrs].location = (loc), \
                                      (p)->attrs[(p)->nattrs].buffer_slot = (slot), \
                                      (p)->attrs[(p)->nattrs].format = (fmt), (p)->attrs[(p)->nattrs].offset = (off), \
                                      (p)->nattrs++)

/* A pipeline of one of the device's programs, its description zeroed, then its shaders and vertex
   input: the quads' the corner, then where the instance's record starts in the frame's quads;
   Granny's models' their vertices, then where each pose's header lies in the frame's quads; the
   wobble's the draw's record per instance alone (instances_needed). */
static void own_pipe(struct GpuPipe *p, int program)
{
    memset(p, 0, sizeof *p);
    p->vs = G.vs[program];
    p->fs = G.fs[program];
    p->targets = 1;
    p->buffers[0].slot = 0;
    p->buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    p->buffers[1].slot = 1;
    p->buffers[1].pitch = 4;
    p->buffers[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
    p->nbuffers = 2;
    if (program == PROG_QUAD) {
        p->buffers[0].pitch = 4;
        ATTR(p, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
        ATTR(p, 1, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
    } else if (program == PROG_SKIN) {
        p->buffers[0].pitch = sizeof(struct SkinVertex);
        ATTR(p, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(struct SkinVertex, position));
        ATTR(p, 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(struct SkinVertex, normal));
        ATTR(p, 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(struct SkinVertex, uv));
        ATTR(p, 3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(struct SkinVertex, weights));
        ATTR(p, 4, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4, offsetof(struct SkinVertex, bones));
        ATTR(p, 5, 0, SDL_GPU_VERTEXELEMENTFORMAT_UINT, offsetof(struct SkinVertex, flags));
        ATTR(p, 6, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
    } else {
        /* the wobble's triangle comes from the vertex index: the draw's record alone, its one
           buffer at slot 0, which a draw binds as the first */
        memset(&p->buffers[1], 0, sizeof p->buffers[1]);
        p->buffers[0].pitch = 4;
        p->buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
        ATTR(p, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
        p->nbuffers = 1;
    }
}

#undef ATTR

/* What a mode makes of a pipeline of one of the device's programs into a kind of target. */
static void mode_pipe(struct GpuPipe *p, int program, const struct DrawModeDesc *m, int cls, int src, int dst, int flat)
{
    own_pipe(p, program);
    mode_state(p, m, cls, src, dst, flat);
}

/* Every pipeline the modes draw with, once the screen's samples and depth are known: a mode's
   quads into each kind of target it draws into, flat and not, and its Granny models, the effect
   modes' quads by every pair of the blend factors the screen has, and the wobble's (gfx_wobble);
   then the layers' (the pipelines hook). */
static int make_pipelines(void)
{
    struct GpuPipe p;
    int mode, cls, flat, s, d, i;
    for (mode = 0; mode < DRAW_MODE_COUNT; mode++) {
        const struct DrawModeDesc *m = draw_mode(mode);
        if (m->src == DRAW_BLEND_FROM_DRAW) {
            for (s = 0; s < DRAW_FACTOR_COUNT; s++)
                for (d = 0; d < DRAW_FACTOR_COUNT; d++) {
                    SDL_GPUGraphicsPipeline **at = &G.effectPipeline[m->depth == DRAW_DEPTH_OFF][s][d];
                    mode_pipe(&p, PROG_QUAD, m, CLASS_SCREEN, DRAW_BLEND_ONE, DRAW_BLEND_ONE, 1);
                    p.src = s;
                    p.dst = d;
                    if (!(*at = gpu_pipeline(&p)))
                        return 0;
                }
            continue;
        }
        for (cls = 0; cls < CLASS_COUNT; cls++) {
            if (!(m->into & (DRAW_INTO_SCREEN << cls)))
                continue;
            for (flat = 0; flat < 2; flat++) {
                mode_pipe(&p, PROG_QUAD, m, cls, 0, 0, flat);
                if (!(G.quadPipeline[mode][cls][flat] = gpu_pipeline(&p)))
                    return 0;
            }
        }
        /* Granny's models are drawn among the opaque lists' quads, which draw meshes, both faces of
           each triangle as the quads */
        if ((m->into & DRAW_INTO_SCREEN) && (m->into & DRAW_MESHES)) {
            mode_pipe(&p, PROG_SKIN, m, CLASS_SCREEN, 0, 0, 0);
            if (!(G.skinPipeline[mode] = gpu_pipeline(&p)))
                return 0;
        }
    }
    /* the wobble's, into the screen with its depth kept, its alpha 1 */
    own_pipe(&p, PROG_ROWS);
    p.depth = DRAW_DEPTH_OFF;
    p.cull = GPU_CULL_NONE;
    p.mask = GPU_MASK_RGB;
    p.hasDepth = G.depth != NULL;
    p.samples = (int)G.samples;
    if (!(G.rowsPipeline = gpu_pipeline(&p)))
        return 0;
    for (i = 0; i < G.nlayers; i++)
        if (G.layers[i]->pipelines && !G.layers[i]->pipelines())
            return 0;
    return 1;
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

/* The draws batched so far are made with what they were batched with when that changes. A call
   that gives what is set already changes nothing (most of the effect lists' calls). */
static void change_state(int mode, int src, int dst, int combine, const struct GfxFog *fog)
{
    struct draw_state s;
    int changed;
    if (mode == G.mode && src == G.effectSrc && dst == G.effectDst && combine == G.effectCombine && fog == &G.fog) {
        perf_count_mode(0);
        return;
    }
    resolve_state(&s, mode, src, dst, combine, fog->on);
    changed = memcmp(&s, &G.state, sizeof s) || memcmp(fog, &G.fog, sizeof *fog);
    perf_count_mode(changed);
    if (changed)
        batch_flush();
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
   texture is wider and taller than the target (GPU_MARGIN), so the coordinates scale to the target's
   part of it, and a clamped read goes no further than the centre of the target's last texel, as
   clamping at the target's edge would. */
static void stage_scale(float *scale, float *limit, const struct gfx_surface *s, int clamped)
{
    const struct gfx_surface *g = is_target(s) ? s : NULL;
    scale[0] = g ? (float)g->width / (float)(g->width + GPU_MARGIN) : 1.0f;
    scale[1] = g ? (float)g->height / (float)(g->height + GPU_MARGIN) : 1.0f;
    limit[0] = g && clamped ? ((float)g->width - 0.5f) / (float)(g->width + GPU_MARGIN) : 3.0e38f;
    limit[1] = g && clamped ? ((float)g->height - 0.5f) / (float)(g->height + GPU_MARGIN) : 3.0e38f;
}

/* What a draw record holds of the state it is made with, its stages' textures and its pass; the
   fog itself where the draw is of quads, whose fog is the vertices' (render/shaders/quad.hlsli,
   vertex_fog). */
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
        r->fog[0] = G.fog.start;
        r->fog[1] = G.fog.end;
        r->fog[2] = G.fog.origin;
        r->fog[3] = G.fog.span;
    }
    colour_floats(r->fogColour, G.fog.colour);
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
   the current target, flat or not. A mode that does not draw into the target draws nothing
   (render/gfx.h, gfx_set_mode). Answers whether it recorded the draw. */
static int quad_draw(int mode, const struct draw_state *s, struct gfx_surface *const *tex, struct gpu_array *array,
                     struct gfx_surface *const *layers, size_t nlayers, uint32_t record, uint32_t first,
                     uint32_t count, int flat)
{
    struct gpu_step *pass;
    struct gpu_draw *d;
    struct GpuDraw *r;
    struct gfx_surface *stages[2];
    SDL_GPUTextureSamplerBinding bind[3];
    SDL_GPUGraphicsPipeline *pipe;
    size_t i;
    int cls;
    if (!G.dev || !G.target || !count)
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
    d->d.program = PROG_QUAD;
    d->d.pipeline = pipe;
    memcpy(d->d.bind, bind, sizeof bind);
    d->count = count;
    d->record = first;
    r = (struct GpuDraw *)(G.f->records.data + (size_t)record * sizeof *r);
    fill_state(r, pass, s, stages, 1, alpha_to_coverage(draw_mode(mode), (int)pass_samples(pass), flat));
    if (array)
        r->stages[1] |= 4;
    G.target->version = next_version();
    return 1;
}

static void skin_flush(void);

/* The draw of the quads given since it was last made, in the current mode with the stages'
   textures, as the Direct3D 9 client's batch was drawn when a state changed; counted as
   diag/perf.h says. Granny's draws given since theirs was made likewise (skin_flush): one batch is
   open at a time. */
static void batch_flush(void)
{
    skin_flush();
    if (!G.batch.open)
        return;
    G.batch.open = 0;
    if (!quad_draw(G.mode, &G.state, G.batch.tex, G.batch.array, G.batch.layers, G.batch.nlayers, G.batch.record,
                   G.batch.first, G.batch.count, G.batch.flat))
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
    if (!G.dev || !G.target)
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
    G.batch.first = (uint32_t)(G.f->offsets.size / 4);
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
        if (!gpu_grow((void **)&G.batch.layers, &G.batch.layerCapacity, G.batch.nlayers + 1, sizeof *G.batch.layers))
            return 0;
        G.batch.layers[G.batch.nlayers++] = t0;
    }
    return 1;
}

/* An instance's record into the frame's quads and its place into their offsets: n words, a whole
   number of 16-byte rows, drawing `prims` triangles. */
static void batch_add(const uint32_t *words, int n, uint32_t prims)
{
    long at = gpu_bytes_take(&G.f->quads, (size_t)n * 4, 16), offset;
    if (at < 0)
        return;
    offset = gpu_bytes_take(&G.f->offsets, 4, 4);
    if (offset < 0) {
        G.f->quads.size = (size_t)at;
        return;
    }
    memcpy(G.f->quads.data + at, words, (size_t)n * 4);
    *(uint32_t *)(G.f->offsets.data + offset) = (uint32_t)(at / 16);
    G.batch.count++;
    G.batch.prims += prims;
    G.batch.bytes += (uint32_t)n * 4 + 4;
}

/* The matrices are the ones the Direct3D 9 client drew with (build/ref/COMMIT.txt), computed in
   floats as it computed them: the GPU divides their points and moves them by the viewport as
   Direct3D 9 did, half a pixel on (GPU_MARGIN).

   A matrix and projection in the frame's quads, as GFX_QUAD_WORLD's and a corner set's go through
   (view NULL for points in view space: the projection alone): the row it starts at, the last
   one's of its kind again where they are the same, -1 without memory. */
static int quad_matrix(const float *view, const float *proj, const float *centre)
{
    float m[16];
    int kind = view != NULL;
    long at;
    if (view)
        memcpy(m, view, 12 * sizeof *m);
    else
        memset(m, 0, 12 * sizeof *m);
    m[12] = proj[0];
    m[13] = proj[1];
    m[14] = centre[0];
    m[15] = centre[1];
    if (G.matrixIndex[kind] >= 0 && !memcmp(m, G.matrix[kind], sizeof m))
        return G.matrixIndex[kind];
    at = gpu_bytes_take(&G.f->quads, sizeof m, 16);
    if (at < 0)
        return -1;
    memcpy(G.f->quads.data + at, m, sizeof m);
    memcpy(G.matrix[kind], m, sizeof m);
    G.matrixIndex[kind] = (int)(at / 16);
    g_perf.instanceBytes += sizeof m;
    return G.matrixIndex[kind];
}

/* A corner set's first row in the frame's quads: where the device put it for an earlier triangle
   of this frame, or put there now with its matrix; -1 without memory, or for a set of none. */
static int corner_set(struct GfxCornerSet *set)
{
    uint32_t words[QUAD_CORNER_WORDS_MAX];
    const struct GfxCorners *c = &set->corners;
    int matrix, n;
    long at;
    if (set->frame == G.frame && set->row >= 0)
        return set->row;
    matrix = quad_matrix(c->view, c->proj, c->centre);
    n = matrix < 0 ? 0 : quad_corner_words(c, (uint32_t)matrix, words);
    at = n ? gpu_bytes_take(&G.f->quads, (size_t)n * 4, 16) : -1;
    if (at < 0)
        return -1;
    memcpy(G.f->quads.data + at, words, (size_t)n * 4);
    g_perf.instanceBytes += (unsigned)n * 4;
    set->frame = G.frame;
    set->row = (int)(at / 16);
    return set->row;
}

void gfx_draw_quad(const struct GfxQuad *q)
{
    uint32_t words[QUAD_WORDS_MAX];
    struct QuadHeader h;
    int row = 0, flat, n;
    g_perf.drawCalls++;
    if (!q || (q->kind != GFX_QUAD_SPRITE && q->kind != GFX_QUAD_WORLD && q->kind != GFX_QUAD_CORNERS))
        return;
    if (q->kind == GFX_QUAD_CORNERS
        && (!q->corners.set || q->corners.corner[0] >= q->corners.set->corners.count
            || q->corners.corner[1] >= q->corners.set->corners.count
            || q->corners.corner[2] >= q->corners.set->corners.count))
        return;
    flat = q->kind == GFX_QUAD_SPRITE || (q->kind == GFX_QUAD_WORLD ? q->world.flat : q->corners.flat);
    if (!batch_join(flat))
        return;
    if (q->kind == GFX_QUAD_WORLD)
        row = quad_matrix(q->world.viewSpace ? NULL : q->world.view, q->world.proj, q->world.centre);
    else if (q->kind == GFX_QUAD_CORNERS)
        row = corner_set(q->corners.set);
    if (row < 0 || !batch_header(&h, row))
        return;
    n = quad_words(q, &h, words);
    batch_add(words, n, q->kind == GFX_QUAD_CORNERS ? 1 : 2);
}

/* The records of a primitive's instances, with the device's header h: a rectangle's own, or the
   primitive's vertices put into the frame's quads, which its records name; answers how many
   instances it added. */
static uint32_t vertex_records(const struct TexDrawVertex *v, int n, int strip, const struct QuadHeader *h)
{
    uint32_t words[QUAD_WORDS_MAX], before = G.batch.count;
    int count = quad_instance_count(n, strip), i, m;
    long at;
    if (strip && (m = quad_rect_words(v, h, words)) > 0) {
        batch_add(words, m, 2);
        return G.batch.count - before;
    }
    at = gpu_bytes_take(&G.f->quads, (size_t)n * sizeof *v, 16);
    if (at < 0)
        return 0;
    memcpy(G.f->quads.data + at, v, (size_t)n * sizeof *v);
    G.batch.bytes += (uint32_t)n * sizeof *v;
    for (i = 0; i < count; i++) {
        m = quad_vertex_words(n, strip, (uint32_t)(at / 4), i, h, words);
        batch_add(words, m, strip ? 2 : 1);
    }
    return G.batch.count - before;
}

void gfx_draw_vertices(const struct TexDrawVertex *v, int n, int strip)
{
    struct QuadHeader h;
    g_perf.drawCalls++;
    if (!v || !quad_instance_count(n, strip) || !batch_join(quad_vertices_flat(v, n)))
        return;
    if (!batch_header(&h, 0))
        return;
    vertex_records(v, n, strip, &h);
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
    if (!G.dev || !G.target)
        return 0;
    h.layer = 0;
    h.draw = add_record(&r);
    h.snap = G.target == G.screen && G.msaa;
    h.matrix = 0;
    if (!r)
        return 0;
    /* the batch's count and bytes take the quad, and are begun again for the next batch */
    first = (uint32_t)(G.f->offsets.size / 4);
    G.batch.count = G.batch.prims = G.batch.bytes = 0;
    if (!vertex_records(v, 4, 1, &h))
        return 0;
    tex[0] = tex0 && !tex0->layered ? tex0 : NULL;
    tex[1] = NULL;
    return quad_draw(mode, s, tex, NULL, NULL, 0, h.draw, first, 1, 1);
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

/* ---- the layers' draws ---- */

void gpu_end_batch(void)
{
    batch_flush();
}

int gpu_mode(void)
{
    return G.mode;
}

const struct GfxFog *gpu_fog(void)
{
    return &G.fog;
}

int gpu_into_screen(void)
{
    return G.target && G.target == G.screen;
}

struct GpuStep *gpu_pass(void)
{
    return (struct GpuStep *)current_pass();
}

void gpu_pass_size(const struct GpuStep *pass, int *w, int *h)
{
    const struct gpu_step *s = (const struct gpu_step *)pass;
    *w = s->target ? s->target->width : 1;
    *h = s->target ? s->target->height : 1;
}

void gpu_bind_stage(SDL_GPUTextureSamplerBinding *bind, int stage)
{
    bind_stage(bind, &G.state, G.stageTex[stage], stage);
}

void gpu_fill_record(struct GpuDraw *r, const struct GpuStep *pass, int textures, int vertexFog)
{
    struct gfx_surface *none[2] = { NULL, NULL };
    const struct gpu_step *s = (const struct gpu_step *)pass;
    fill_state(r, s, &G.state, textures ? G.stageTex : none, vertexFog,
               alpha_to_coverage(draw_mode(G.mode), pass_samples(s), 0));
}

struct GpuLayerDraw *gpu_add_draw(struct GpuStep *pass, int layer, int program, SDL_GPUGraphicsPipeline *pipeline)
{
    struct gpu_draw *d;
    if (!pipeline || layer < 0 || layer >= G.nlayers)
        return NULL;
    d = add_draw((struct gpu_step *)pass);
    if (!d)
        return NULL;
    d->layer = layer + 1;
    d->d.program = program;
    d->d.pipeline = pipeline;
    return &d->d;
}

void gpu_drawn(void)
{
    G.target->version = next_version();
}

void *gpu_part(int layer)
{
    return G.f->parts[layer];
}

unsigned gpu_frame_number(void)
{
    return G.frame;
}

/* ---- Granny's models ---- */

gfx_skin *gfx_create_skin(const struct SkinVertex *vertices, uint32_t vertexCount, const uint32_t *indices,
                          uint32_t indexCount)
{
    struct gfx_skin *k;
    if (!vertices || !indices || !vertexCount || !indexCount || vertexCount > UINT32_MAX / sizeof *vertices
        || indexCount > UINT32_MAX / sizeof *indices)
        return NULL;
    k = calloc(1, sizeof *k);
    if (!k)
        return NULL;
    k->vertices = malloc((size_t)vertexCount * sizeof *vertices);
    k->indices = malloc((size_t)indexCount * sizeof *indices);
    if (!k->vertices || !k->indices) {
        free(k->vertices);
        free(k->indices);
        free(k);
        return NULL;
    }
    memcpy(k->vertices, vertices, (size_t)vertexCount * sizeof *vertices);
    memcpy(k->indices, indices, (size_t)indexCount * sizeof *indices);
    k->vertexCount = vertexCount;
    k->indexCount = indexCount;
    k->refs = 1;
    lock_handles();
    k->next = G.skins;
    if (G.skins)
        G.skins->prev = k;
    G.skins = k;
    unlock_handles();
    return k;
}

/* its buffers given up; the frames handed over may still draw with them (retire) */
static void skin_buffers_release(struct gfx_skin *k)
{
    retire_buffer(k->vertexBuffer);
    retire_buffer(k->indexBuffer);
    k->vertexBuffer = k->indexBuffer = NULL;
}

static void skin_destroy(struct gfx_skin *k)
{
    lock_handles();
    if (k->prev)
        k->prev->next = k->next;
    else
        G.skins = k->next;
    if (k->next)
        k->next->prev = k->prev;
    unlock_handles();
    skin_buffers_release(k);
    free(k->vertices);
    free(k->indices);
    free(k);
}

/* A hold is a reference, as a surface's (gfx_hold). */
void gfx_hold_skin(gfx_skin *k)
{
    if (!k || k->held)
        return;
    if (!gpu_grow((void **)&G.skinHolds, &G.skinHoldCapacity, G.nskinHolds + 1, sizeof *G.skinHolds))
        abort(); /* not held, it could go before it is drawn */
    k->held = 1;
    k->refs++;
    G.skinHolds[G.nskinHolds++] = k;
}

void gfx_skin_release(gfx_skin *k)
{
    if (!k || --k->refs > 0)
        return;
    if (G.skinBatch.open && G.skinBatch.model == k)
        batch_flush();
    if (k->pinned == G.frame && G.frame)
        k->dead = 1;
    else
        skin_destroy(k);
}

/* The model's buffers on the GPU, made the first time a draw takes it and filled from its arrays,
   staged among the frame's texels now, by the frame's upload (the model stays pinned until the
   frame is handed over); 0 where they cannot be made, which is not tried again with this device. */
static int skin_ready(struct gfx_skin *k)
{
    struct gpu_frame *f = G.f;
    struct skin_upload *u;
    SDL_GPUBufferCreateInfo info;
    size_t vbytes = (size_t)k->vertexCount * sizeof *k->vertices, ibytes = (size_t)k->indexCount * sizeof *k->indices;
    long at;
    if (k->vertexBuffer)
        return 1;
    if (k->failed || vbytes + ibytes > UINT32_MAX
        || !gpu_grow((void **)&f->skinUploads, &f->skinUploadCapacity, f->nskinUploads + 1,
                     sizeof *f->skinUploads))
        return 0;
    at = gpu_bytes_take(&f->texels, vbytes + ibytes, 16);
    if (at < 0 || (unsigned long)at > UINT32_MAX)
        return 0;
    SDL_zero(info);
    info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    info.size = (Uint32)vbytes;
    k->vertexBuffer = SDL_CreateGPUBuffer(G.dev, &info);
    info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    info.size = (Uint32)ibytes;
    k->indexBuffer = SDL_CreateGPUBuffer(G.dev, &info);
    if (!k->vertexBuffer || !k->indexBuffer) {
        SDL_Log("gfx: a Granny model's buffers (%u vertices, %u indices): %s", k->vertexCount, k->indexCount,
                SDL_GetError());
        skin_buffers_release(k);
        k->failed = 1;
        f->texels.size = (size_t)at;
        return 0;
    }
    memcpy(f->texels.data + at, k->vertices, vbytes);
    memcpy(f->texels.data + at + vbytes, k->indices, ibytes);
    u = &f->skinUploads[f->nskinUploads++];
    u->model = k;
    u->vertices = k->vertexBuffer;
    u->indices = k->indexBuffer;
    u->at = (uint32_t)at;
    u->vertexBytes = (uint32_t)vbytes;
    u->indexBytes = (uint32_t)ibytes;
    k->pinned = G.frame;
    g_perf.vertexBytes += (unsigned)(vbytes + ibytes);
    return 1;
}

int gfx_skin_pose(const float (*rows)[4], size_t count)
{
    long at;
    if (!G.dev || !rows || !count || count > UINT32_MAX / 16)
        return -1;
    at = gpu_bytes_take(&G.f->quads, count * 16, 16);
    if (at < 0)
        return -1;
    memcpy(G.f->quads.data + at, rows, count * 16);
    g_perf.instanceBytes += (unsigned)(count * 16);
    return (int)(at / 16);
}

/* A pose's header for the batch's draw: the pose's row and the draw's record, and its place among
   the quads' offsets, which the draw's instances read one after the other. */
static int skin_instance(int pose)
{
    long at = gpu_bytes_add(&G.f->quads, 16, 16), offset;
    uint32_t *h;
    if (at < 0)
        return 0;
    offset = gpu_bytes_take(&G.f->offsets, 4, 4);
    if (offset < 0) {
        G.f->quads.size = (size_t)at;
        return 0;
    }
    h = (uint32_t *)(G.f->quads.data + at);
    h[0] = (uint32_t)pose;
    h[1] = G.skinBatch.record;
    *(uint32_t *)(G.f->offsets.data + offset) = (uint32_t)(at / 16);
    G.skinBatch.bytes += 20;
    return 1;
}

/* The draw of the Granny draws given since it was last made, in the current mode and state, with
   the stages' textures: only into the screen, in a mode that draws meshes. */
static void skin_flush(void)
{
    struct gpu_step *pass;
    struct gpu_draw *d;
    struct GpuDraw *r;
    SDL_GPUTextureSamplerBinding bind[3];
    SDL_GPUGraphicsPipeline *pipe = G.skinPipeline[G.mode];
    if (!G.skinBatch.open)
        return;
    G.skinBatch.open = 0;
    if (!pipe || !G.dev || G.target != G.screen || !skin_ready(G.skinBatch.model))
        return;
    bind_stage(bind, &G.state, G.skinBatch.tex[0], 0);
    bind_stage(bind + 1, &G.state, G.skinBatch.tex[1], 1);
    bind[2].texture = G.noAtlas;
    bind[2].sampler = bind[0].sampler;
    pass = current_pass();
    if (!pass || !(d = add_draw(pass)))
        return;
    d->d.program = PROG_SKIN;
    d->d.pipeline = pipe;
    memcpy(d->d.bind, bind, sizeof bind);
    d->skinVertices = G.skinBatch.model->vertexBuffer;
    d->skinIndices = G.skinBatch.model->indexBuffer;
    d->firstIndex = G.skinBatch.range.first;
    d->count = G.skinBatch.range.count;
    d->instances = G.skinBatch.range.instances;
    d->firstInstance = G.skinBatch.firstInstance;
    r = (struct GpuDraw *)(G.f->records.data + (size_t)G.skinBatch.record * sizeof *r);
    fill_state(r, pass, &G.state, G.skinBatch.tex, 1,
               alpha_to_coverage(draw_mode(G.mode), (int)pass_samples(pass), 0));
    G.skinBatch.model->pinned = G.frame;
    G.target->version = next_version();
    g_perf.draws++;
    g_perf.prims += G.skinBatch.prims;
    g_perf.instanceBytes += G.skinBatch.bytes;
}

void gfx_draw_skin(gfx_skin *k, uint32_t first, uint32_t count, int pose)
{
    struct gfx_surface *t0 = G.stageTex[0], *t1 = G.stageTex[1];
    struct GpuDraw *r;
    int join = SKIN_JOIN_NEW;
    g_perf.drawCalls++;
    if (!k || k->failed || pose < 0 || count < 3 || first > k->indexCount || count > k->indexCount - first
        || !G.dev || !G.target || G.target != G.screen)
        return;
    count -= count % 3;
    if (t0 && t0->layered)
        t0 = NULL;
    if (G.skinBatch.open && G.skinBatch.model == k && G.skinBatch.tex[0] == t0 && G.skinBatch.tex[1] == t1)
        join = skin_batch_join(&G.skinBatch.range, first, count, pose);
    if (join == SKIN_JOIN_EXTEND) {
        G.skinBatch.range.count += count;
    } else if (join == SKIN_JOIN_INSTANCE) {
        if (!skin_instance(pose))
            return;
        G.skinBatch.range.instances++;
        G.skinBatch.range.pose = pose;
    } else {
        batch_flush();
        G.skinBatch.record = add_record(&r);
        if (!r)
            return;
        G.skinBatch.model = k;
        G.skinBatch.tex[0] = t0;
        G.skinBatch.tex[1] = t1;
        G.skinBatch.firstInstance = (uint32_t)(G.f->offsets.size / 4);
        G.skinBatch.prims = G.skinBatch.bytes = 0;
        if (!skin_instance(pose))
            return;
        G.skinBatch.range.first = first;
        G.skinBatch.range.count = count;
        G.skinBatch.range.instances = 1;
        G.skinBatch.range.pose = pose;
        G.skinBatch.open = 1;
    }
    G.skinBatch.prims += count / 3;
}




/* ---- the frame's buffers, on the main thread ---- */

/* A buffer of the frames'at least `need` bytes; one made larger goes once the frames handed over
   are done with it (retire). */
static int ensure_buffer(SDL_GPUBuffer **buffer, uint32_t *size, uint32_t need, SDL_GPUBufferUsageFlags usage)
{
    SDL_GPUBufferCreateInfo info;
    uint32_t s = *size ? *size : 4096;
    if (*buffer && need <= *size)
        return 1;
    while (s < need)
        s *= 2;
    retire_buffer(*buffer);
    SDL_zero(info);
    info.usage = usage;
    info.size = s;
    *buffer = SDL_CreateGPUBuffer(G.dev, &info);
    *size = *buffer ? s : 0;
    return *buffer != NULL;
}

/* The identity instance buffer (0, 1, 2, ...): a draw's first instance is its record. Where it is
   made larger, its ids are staged among the frame's texels: answers their offset, -1 where it is
   large enough already, -2 where it could not be made. */
static long instances_needed(uint32_t records)
{
    uint32_t count = G.instanceCount ? G.instanceCount : 4096, i;
    long at;
    uint32_t *ids;
    uint32_t size = 0;
    if (G.buffers.instances && records <= G.instanceCount)
        return -1;
    while (count < records)
        count *= 2;
    retire_buffer(G.buffers.instances);
    G.buffers.instances = NULL;
    G.instanceCount = 0;
    if (!ensure_buffer(&G.buffers.instances, &size, count * 4, SDL_GPU_BUFFERUSAGE_VERTEX))
        return -2;
    at = gpu_bytes_take(&G.f->texels, (size_t)count * 4, 16);
    if (at < 0)
        return -2;
    ids = (uint32_t *)(G.f->texels.data + at);
    for (i = 0; i < count; i++)
        ids[i] = i;
    G.instanceCount = count;
    return at;
}

/* ---- the frame's replay, on the render thread ---- */

/* A stream of the frame's into a buffer of its own, whole. */
static void copy_into(SDL_GPUCopyPass *copy, const struct GpuBytes *from, SDL_GPUBuffer *buffer)
{
    SDL_GPUTransferBufferLocation src;
    SDL_GPUBufferRegion dst;
    if (!from->size || !from->buffer || !buffer)
        return;
    src.transfer_buffer = from->buffer;
    src.offset = 0;
    dst.buffer = buffer;
    dst.offset = 0;
    dst.size = (Uint32)from->size;
    SDL_UploadToGPUBuffer(copy, &src, &dst, true);
}

/* The Granny models whose buffers the frame fills, from their vertices and indices staged among its
   texels (skin_ready). */
static void skin_copy(SDL_GPUCopyPass *copy, const struct gpu_frame *f)
{
    size_t i;
    for (i = 0; i < f->nskinUploads; i++) {
        const struct skin_upload *u = &f->skinUploads[i];
        SDL_GPUTransferBufferLocation src;
        SDL_GPUBufferRegion dst;
        src.transfer_buffer = f->texels.buffer;
        src.offset = u->at;
        dst.buffer = u->vertices;
        dst.offset = 0;
        dst.size = u->vertexBytes;
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
        src.offset += u->vertexBytes;
        dst.buffer = u->indices;
        dst.size = u->indexBytes;
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    }
}

/* answers whether a texture uploaded wants its mipmaps made */
static int run_uploads(SDL_GPUCopyPass *copy, const struct gpu_frame *f, size_t first, size_t count)
{
    size_t i;
    int mips = 0;
    for (i = first; i < first + count; i++) {
        const struct gpu_upload *u = &f->uploads[i];
        SDL_GPUTextureTransferInfo src;
        SDL_GPUTextureRegion dst;
        if (u->fromLayer) {
            SDL_GPUTextureLocation from, to;
            SDL_zero(from);
            SDL_zero(to);
            from.texture = u->array;
            from.layer = u->layer;
            to.texture = u->tex;
            SDL_CopyGPUTextureToTexture(copy, &from, &to, u->w, u->h, 1, false);
            continue;
        }
        if (!u->w || !u->h) {
            mips |= u->mips;
            continue;
        }
        SDL_zero(src);
        SDL_zero(dst);
        src.transfer_buffer = f->texels.buffer;
        src.offset = u->offset;
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

/* A layer of an array with mipmaps takes its levels as a texture of its own makes them, on every
   backend: its top level goes into a scratch texture of its size and levels (the layer's own, with
   the texels written over it: layer_mipmaps), SDL makes that one's levels
   (SDL_GenerateMipmapsForGPUTexture), and every level is copied into the layer. The sprite
   atlas's pages and the static world's textures alike; blits into the layer level by level made
   other mipmaps than a texture's own on Mesa's lavapipe. */
void gpu_copy_levels(SDL_GPUCopyPass *copy, SDL_GPUTexture *scratch, SDL_GPUTexture *array, uint32_t layer,
                     uint32_t w, uint32_t h, uint32_t levels)
{
    uint32_t level;
    for (level = 0; level < levels; level++) {
        SDL_GPUTextureLocation src, dst;
        SDL_zero(src);
        SDL_zero(dst);
        src.texture = scratch;
        src.mip_level = level;
        dst.texture = array;
        dst.mip_level = level;
        dst.layer = layer;
        SDL_CopyGPUTextureToTexture(copy, &src, &dst, w >> level ? w >> level : 1, h >> level ? h >> level : 1, 1,
                                    false);
    }
}

/* The lower levels of each texture uploaded that wants them, made from the top one by SDL; then a
   layer's scratch texture, every level of it, into the layer (copy_levels), once, for the record
   that asked for its levels. */
static void make_mipmaps(SDL_GPUCommandBuffer *cmd, const struct gpu_frame *f, size_t first, size_t count)
{
    SDL_GPUCopyPass *copy = NULL;
    size_t i;
    for (i = first; i < first + count; i++)
        if (f->uploads[i].mips)
            SDL_GenerateMipmapsForGPUTexture(cmd, f->uploads[i].tex);
    for (i = first; i < first + count; i++) {
        const struct gpu_upload *u = &f->uploads[i];
        if (!u->scratch || !u->mips)
            continue;
        if (!copy && !(copy = SDL_BeginGPUCopyPass(cmd)))
            return;
        gpu_copy_levels(copy, u->tex, u->array, u->layer, u->arrayW, u->arrayH, u->arrayLevels);
    }
    if (copy)
        SDL_EndGPUCopyPass(copy);
}

/* The storage buffers the device's programs read (src/render/shaders), in the order they declare
   them, at most STORAGE_BUFFERS_MAX a stage (gpu_shader), and their vertex and index buffers: the
   quads' the draw records and the frame's quads, a quad's corners and, per instance, where its
   record starts among the quads, and the corners' two triangles; Granny's models the same two
   buffers, and per instance where its pose's header lies among the quads, the model's vertices and
   indices bound by run_pass by the draw; the wobble the records and its rows, and the records'
   instances. (A layer binds its own: gpu_pass_use.) */
static void bind_program(SDL_GPURenderPass *pass, const struct gpu_frame *f, int program)
{
    const struct frame_buffers *b = &f->buffers;
    SDL_GPUBuffer *vertex[4], *fragment[1];
    SDL_GPUBufferBinding binding[2];
    Uint32 nv = 0;
    fragment[0] = b->records;
    SDL_zero(binding);
    switch (program) {
    case PROG_QUAD:
        vertex[nv++] = b->records;
        vertex[nv++] = b->quads;
        binding[0].buffer = G.corners;
        binding[1].buffer = b->offsets;
        SDL_BindGPUVertexBuffers(pass, 0, binding, 2);
        binding[0].buffer = G.triangles;
        SDL_BindGPUIndexBuffer(pass, &binding[0], SDL_GPU_INDEXELEMENTSIZE_16BIT);
        break;
    case PROG_SKIN:
        vertex[nv++] = b->records;
        vertex[nv++] = b->quads;
        break;
    default:
        vertex[nv++] = b->records;
        fragment[0] = b->rows;
        binding[0].buffer = b->instances;
        SDL_BindGPUVertexBuffers(pass, 0, binding, 1);
        break;
    }
    SDL_BindGPUVertexStorageBuffers(pass, 0, vertex, nv);
    SDL_BindGPUFragmentStorageBuffers(pass, 0, fragment, 1);
}

/* A pass as run_pass records it: what it has bound, the pipeline, the program and whose it is (0
   the device's, n layer n - 1's), the fragment shader's textures; and whose the draw being drawn
   is. A layer's draw hook is handed `pub`. */
struct pass_run {
    struct GpuPass pub;
    SDL_GPUGraphicsPipeline *bound;
    int layer, program;
    int drawLayer;
    SDL_GPUTextureSamplerBinding textures[3];
};

/* The draw's pipeline bound where another is; 1 where its program is not the one bound, whose
   buffers are then to be bound, and the textures bound are forgotten. */
static int use_program(struct pass_run *run, const struct GpuLayerDraw *d, int layer)
{
    if (d->pipeline != run->bound) {
        SDL_BindGPUGraphicsPipeline(run->pub.pass, d->pipeline);
        run->bound = d->pipeline;
    }
    if (d->program == run->program && layer == run->layer)
        return 0;
    run->program = d->program;
    run->layer = layer;
    memset(run->textures, 0, sizeof run->textures);
    return 1;
}

static void bind_samplers(struct pass_run *run, const SDL_GPUTextureSamplerBinding *bind, int n)
{
    if (memcmp(run->textures, bind, (size_t)n * sizeof *run->textures)) {
        memcpy(run->textures, bind, (size_t)n * sizeof *run->textures);
        SDL_BindGPUFragmentSamplers(run->pub.pass, 0, run->textures, (Uint32)n);
    }
}

int gpu_pass_use(struct GpuPass *pass, const struct GpuLayerDraw *draw)
{
    struct pass_run *run = (struct pass_run *)pass;
    return use_program(run, draw, run->drawLayer);
}

void gpu_pass_samplers(struct GpuPass *pass, const SDL_GPUTextureSamplerBinding *bind, int n)
{
    bind_samplers((struct pass_run *)pass, bind, n);
}

/* A pass of the frame's steps: its draws in their order, the layers' by their draw hooks. */
static void run_pass(SDL_GPUCommandBuffer *cmd, const struct gpu_frame *f, const struct gpu_step *s)
{
    SDL_GPUColorTargetInfo colour;
    SDL_GPUDepthStencilTargetInfo depth;
    SDL_GPUBufferBinding binding;
    SDL_GPUBuffer *skin = NULL;
    struct pass_run run;
    SDL_Rect scissor;
    SDL_GPUViewport viewport;
    uint32_t i;
    SDL_zero(colour);
    SDL_zero(depth);
    SDL_zero(run);
    run.program = -1;
    colour.texture = s->colour;
    if (s->resolve) {
        colour.resolve_texture = s->resolve;
        colour.store_op = SDL_GPU_STOREOP_RESOLVE_AND_STORE;
    } else {
        colour.store_op = SDL_GPU_STOREOP_STORE;
    }
    if (!colour.texture)
        return;
    colour.load_op = s->loadColour;
    colour.clear_color = s->clearColour;
    if (s->depth) {
        depth.texture = s->depth;
        depth.load_op = s->loadDepth;
        depth.clear_depth = s->clearDepth;
        depth.store_op = SDL_GPU_STOREOP_STORE;
        depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    }
    run.pub.cmd = cmd;
    run.pub.records = f->buffers.records;
    run.pub.pass = SDL_BeginGPURenderPass(cmd, &colour, 1, s->depth ? &depth : NULL);
    if (!run.pub.pass) {
        SDL_Log("gfx: SDL_BeginGPURenderPass: %s", SDL_GetError());
        return;
    }
    /* the target moved by half a pixel, to where Direct3D 9's pixel centres were (GPU_MARGIN); the
       wobble's covers the whole target, every sample of its first column and row too */
    viewport.x = viewport.y = s->whole ? 0.0f : 0.5f;
    viewport.w = (float)s->width;
    viewport.h = (float)s->height;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(run.pub.pass, &viewport);
    scissor.x = scissor.y = 0;
    scissor.w = s->width;
    scissor.h = s->height;
    SDL_SetGPUScissor(run.pub.pass, &scissor);
    for (i = s->first; i < s->first + s->count; i++) {
        const struct gpu_draw *d = &f->draws[i];
        if (!d->d.pipeline)
            continue;
        if (d->layer) {
            const struct GpuLayer *layer = G.layers[d->layer - 1];
            run.drawLayer = d->layer;
            if (layer->draw)
                layer->draw(f->parts[d->layer - 1], &run.pub, &d->d);
            continue;
        }
        if (use_program(&run, &d->d, 0)) {
            bind_program(run.pub.pass, f, d->d.program);
            skin = NULL;
        }
        if (d->d.program == PROG_SKIN && d->skinVertices != skin) {
            SDL_GPUBufferBinding vertices[2];
            SDL_zero(vertices);
            vertices[0].buffer = d->skinVertices;
            vertices[1].buffer = f->buffers.offsets;
            SDL_BindGPUVertexBuffers(run.pub.pass, 0, vertices, 2);
            binding.buffer = d->skinIndices;
            binding.offset = 0;
            SDL_BindGPUIndexBuffer(run.pub.pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            skin = d->skinVertices;
        }
        bind_samplers(&run, d->d.bind, d->d.program == PROG_ROWS ? 1 : 3);
        if (d->d.program == PROG_QUAD) {
            SDL_DrawGPUIndexedPrimitives(run.pub.pass, 6, d->count, 0, 0, d->record);
        } else if (d->d.program == PROG_ROWS) {
            /* no vertices: the triangle comes from the vertex index */
            SDL_DrawGPUPrimitives(run.pub.pass, d->count, 1, 0, d->record);
        } else {
            SDL_DrawGPUIndexedPrimitives(run.pub.pass, d->count, d->instances, d->firstIndex, 0, d->firstInstance);
        }
    }
    SDL_EndGPURenderPass(run.pub.pass);
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

/* The download of a rectangle of the screen the frame asks for (gfx_read_frame), after its steps. */
static void download_screen(SDL_GPUCommandBuffer *cmd, const struct gpu_frame *f)
{
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureRegion src;
    SDL_GPUTextureTransferInfo dst;
    if (!copy)
        return;
    SDL_zero(src);
    SDL_zero(dst);
    src.texture = f->download.texture;
    src.x = (Uint32)f->download.x;
    src.y = (Uint32)f->download.y;
    src.w = (Uint32)f->download.w;
    src.h = (Uint32)f->download.h;
    src.d = 1;
    dst.transfer_buffer = f->download.buffer;
    dst.pixels_per_row = (Uint32)f->download.w;
    dst.rows_per_layer = (Uint32)f->download.h;
    SDL_DownloadFromGPUTexture(copy, &src, &dst);
    SDL_EndGPUCopyPass(copy);
}

/* A frame into one command buffer, submitted with a fence: one copy pass of what it staged (its
   streams, the layers' parts (the copy hook), the Granny models it fills, the textures it changed
   before its first draw), the textures' mipmaps, the layers' passes (the compute hook), then its
   steps, and the download it asks for, holding the device's recording (commands_take). 0 where it
   could not be submitted. */
static int frame_record(struct gpu_frame *f)
{
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    size_t i;
    int mips, k;
    commands_take();
    cmd = SDL_AcquireGPUCommandBuffer(G.dev);
    if (!cmd) {
        SDL_Log("gfx: SDL_AcquireGPUCommandBuffer: %s", SDL_GetError());
        commands_give();
        return 0;
    }
    copy = SDL_BeginGPUCopyPass(cmd);
    copy_into(copy, &f->records, f->buffers.records);
    copy_into(copy, &f->rows, f->buffers.rows);
    copy_into(copy, &f->quads, f->buffers.quads);
    copy_into(copy, &f->offsets, f->buffers.offsets);
    if (f->instancesAt >= 0) {
        SDL_GPUTransferBufferLocation src;
        SDL_GPUBufferRegion dst;
        src.transfer_buffer = f->texels.buffer;
        src.offset = (Uint32)f->instancesAt;
        dst.buffer = f->buffers.instances;
        dst.offset = 0;
        dst.size = f->instanceCount * 4;
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    }
    for (k = 0; k < G.nlayers; k++)
        if (G.layers[k]->copy)
            G.layers[k]->copy(f->parts[k], copy, f->texels.buffer);
    skin_copy(copy, f);
    mips = run_uploads(copy, f, 0, f->frameUploads);
    SDL_EndGPUCopyPass(copy);
    if (mips)
        make_mipmaps(cmd, f, 0, f->frameUploads);
    for (k = 0; k < G.nlayers; k++)
        if (G.layers[k]->compute)
            G.layers[k]->compute(f->parts[k], cmd);
    for (i = 0; i < f->nsteps; i++) {
        const struct gpu_step *s = &f->steps[i];
        if (s->kind == STEP_PASS) {
            run_pass(cmd, f, s);
        } else if (s->kind == STEP_UPLOADS) {
            SDL_GPUCopyPass *late = SDL_BeginGPUCopyPass(cmd);
            mips = run_uploads(late, f, s->first, s->count);
            SDL_EndGPUCopyPass(late);
            if (mips)
                make_mipmaps(cmd, f, s->first, s->count);
        } else if (s->kind == STEP_COPY) {
            run_copy(cmd, s);
        }
    }
    if (f->download.out)
        download_screen(cmd, f);
    f->fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    commands_give();
    if (!f->fence) {
        SDL_Log("gfx: SDL_SubmitGPUCommandBufferAndAcquireFence: %s", SDL_GetError());
        return 0;
    }
    return 1;
}

/* Waits for the GPU to finish a frame submitted. */
static void frame_wait(struct gpu_frame *f)
{
    if (!f->fence)
        return;
    SDL_WaitForGPUFences(G.dev, true, &f->fence, 1);
    SDL_ReleaseGPUFence(G.dev, f->fence);
    f->fence = NULL;
}

/* The render thread's part of a frame (render/render_thread.h). First it waits for the GPU to
   finish the frame FRAME_RING_IN_FLIGHT before: so the main thread runs ahead of the GPU by that
   and the frame it records at most, also where no swapchain paces the frames (a hidden window),
   and SDL's Vulkan driver, which gives each command buffer a cache of descriptor sets and takes it
   back only once it finds that command buffer finished, gives the frame the cache of the one it
   waited for (one that finds none given back makes a descriptor pool for every kind of binding the
   frame binds, which the driver can take 15 ms and more over). Then the frame is recorded and
   submitted, unless the main thread could not hand it over whole; what was given up while it was
   recorded is released; and a download it asks for is waited for and copied out to the main
   thread, which waits for it. */
static void frame_run(void *context, int slot, int wait)
{
    struct gpu_frame *f = &G.frames[slot];
    (void)context;
    if (wait >= 0)
        frame_wait(&G.frames[wait]);
    if (f->state == FRAME_DRAWN && !frame_record(f))
        f->state = FRAME_UNSUBMITTED;
    release_retired(f->retired, f->nretired);
    f->nretired = 0;
    if (f->download.out && f->fence) {
        const unsigned char *p = NULL;
        int y;
        frame_wait(f);
        p = SDL_MapGPUTransferBuffer(G.dev, f->download.buffer, false);
        if (p) {
            for (y = 0; y < f->download.h; y++)
                memcpy(f->download.out + (size_t)y * f->download.pitch, p + (size_t)y * f->download.w * 4,
                       (size_t)f->download.w * 4);
            SDL_UnmapGPUTransferBuffer(G.dev, f->download.buffer);
            f->downloaded = 1;
        }
    }
}

/* ---- handing frames over, on the main thread ---- */

/* Whether a frame shown without waiting may ask for the window's next picture: once a refresh of
   the window's display (60 Hz where its mode does not say) has passed since the last one, when SDL's
   acquire, which waits for the presentation engine, no longer has to. */
static int present_due(void)
{
    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(G.window));
    float hz = mode && mode->refresh_rate > 0.0f ? mode->refresh_rate : 60.0f;
    return SDL_GetTicksNS() - G.presentedNs >= (Uint64)(1e9 / hz);
}

/* A present in two halves, in a command buffer of the main thread's own, which SDL wants for a
   swapchain: present_begin takes the window's next picture, which with `wait` waits for one, while
   the render thread still records and submits the frame handed over before; present_end, once
   that frame is back, blits the screen into it, fitted (present_fit). The screen then holds the
   last frame the render thread finished, and the frame being handed over is shown at the next
   present, as Direct3D 9's driver queued a frame. Without `wait` the screen is not shown unless
   present_due, as Direct3D 9's flip without waiting (D3DPRESENT_DONOTWAIT) left a frame the screen
   had no room for. Nothing is shown until a frame drawn into the screen as it is now has been
   submitted. */
struct present {
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUTexture *swap;
    Uint32 w, h;
};

/* 0 where nothing is to be shown */
static int present_begin(int wait, struct present *p)
{
    SDL_zerop(p);
    if (!G.claimed || !G.screen || !G.screen->tex || !G.screenShown || (!wait && !present_due()))
        return 0;
    p->cmd = SDL_AcquireGPUCommandBuffer(G.dev);
    if (!p->cmd) {
        SDL_Log("gfx: SDL_AcquireGPUCommandBuffer: %s", SDL_GetError());
        return 0;
    }
    if (!(wait ? SDL_WaitAndAcquireGPUSwapchainTexture(p->cmd, G.window, &p->swap, &p->w, &p->h)
               : SDL_AcquireGPUSwapchainTexture(p->cmd, G.window, &p->swap, &p->w, &p->h))) {
        SDL_CancelGPUCommandBuffer(p->cmd);
        p->cmd = NULL;
        return 0;
    }
    if (p->swap)
        G.presentedNs = SDL_GetTicksNS();
    return 1;
}

static void present_end(const struct present *p)
{
    SDL_GPUTexture *swap = p->swap;
    Uint32 w = p->w, h = p->h;
    int fx, fy, fw, fh, pw = 0, ph = 0;
    struct PresentCover cover = present_cover;
    SDL_GPUBlitInfo blit;
    commands_take();
    if (swap) {
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
        /* filtered only where it shrinks: a filtered enlargement would read the texture's margin
           past the picture's last row and column (GPU_MARGIN) */
        blit.filter = fw < G.screen->width || fh < G.screen->height ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;
        SDL_BlitGPUTexture(p->cmd, &blit);
    }
    if (!gpu_submit(p->cmd, NULL))
        SDL_Log("gfx: SDL_SubmitGPUCommandBuffer: %s", SDL_GetError());
}

/* The surface or Granny model an upload of a frame named, if it is alive still: a frame taken back
   after the render thread could not submit it may name some that went when it was handed over, so
   they are looked for among the live ones by their address, which one made since may have taken
   (it then goes up again, which does no harm). */
static struct gfx_surface *live_surface(const struct gfx_surface *s)
{
    struct gfx_surface *h;
    lock_handles();
    for (h = G.handles; h && h != s; h = h->next)
        ;
    unlock_handles();
    return h;
}

static struct gfx_skin *live_skin(const struct gfx_skin *k)
{
    struct gfx_skin *h;
    lock_handles();
    for (h = G.skins; h && h != k; h = h->next)
        ;
    unlock_handles();
    return h;
}

/* The uploads of a frame that did not reach the GPU go up with a later one: the textures it was to
   fill, the Granny models it was to fill, whose buffers are made again, and the identity instances
   it was to make. `live` where the frame has not been handed over yet, so that what it names is all
   alive. */
static void requeue_uploads(struct gpu_frame *f, int live)
{
    size_t i;
    for (i = 0; i < f->nskinUploads; i++) {
        struct gfx_skin *k = live ? f->skinUploads[i].model : live_skin(f->skinUploads[i].model);
        if (k && k->vertexBuffer == f->skinUploads[i].vertices)
            skin_buffers_release(k);
    }
    for (i = 0; i < f->nuploads; i++) {
        const struct gpu_upload *u = &f->uploads[i];
        struct gfx_surface *s = live ? u->surface : live_surface(u->surface);
        if (!s)
            continue;
        if (u->w && u->h && !u->fromLayer) {
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
    if (f->instancesAt >= 0 && G.buffers.instances == f->buffers.instances) {
        retire_buffer(G.buffers.instances);
        G.buffers.instances = NULL;
        G.instanceCount = 0;
    }
}

/* What the frame being handed over pinned may go now: the render thread reads no surface or
   model, only the GPU objects the packet holds, which go once the frame is submitted (retire). */
static void frame_unpin(struct gpu_frame *f)
{
    struct gfx_skin *k, *next;
    size_t i;
    for (k = G.skins; k; k = next) {
        next = k->next;
        if (k->dead)
            skin_destroy(k);
    }
    for (i = 0; i < f->npins; i++) {
        struct gfx_surface *s = f->pins[i];
        int dead;
        lock_handles();
        s->pinned = 0;
        dead = s->dead;
        unlock_handles();
        if (dead)
            destroy_handle(s);
    }
    f->npins = 0;
}

/* the layers told how a frame went (the retired hook) */
static void layers_retired(struct gpu_frame *f, int how)
{
    int i;
    for (i = 0; i < G.nlayers; i++)
        if (G.layers[i]->retired)
            G.layers[i]->retired(f->parts[i], how, f->number);
}

/* A frame back from the render thread (frame_ring_retire): what it could not submit goes up with a
   later frame, a frame drawn into the screen may be shown, and the layers are told how it went.
   Answers whether it could not be submitted. */
static int frame_retire(struct gpu_frame *f)
{
    if (f->state == FRAME_UNSUBMITTED) {
        requeue_uploads(f, 0);
        layers_retired(f, GPU_FRAME_UNSUBMITTED);
        return 1;
    }
    if (f->state == FRAME_DRAWN && f->drawsScreen)
        G.screenShown = 1;
    layers_retired(f, f->state == FRAME_DRAWN ? GPU_FRAME_DRAWN : GPU_FRAME_DROPPED);
    return 0;
}

/* Takes back every frame handed over, waiting for those the render thread has not finished yet;
   answers whether one of them could not be submitted. */
static int retire_frames(void)
{
    int slot, lost = 0;
    while ((slot = render_thread_retire(G.thread)) >= 0)
        lost |= frame_retire(&G.frames[slot]);
    return lost;
}

/* the frame's streams that go up */
#define FRAME_STREAMS 5
static void frame_streams(struct gpu_frame *f, struct GpuBytes **out)
{
    out[0] = &f->records;
    out[1] = &f->rows;
    out[2] = &f->quads;
    out[3] = &f->offsets;
    out[4] = &f->texels;
}

/* The next frame's packet, in the slot the ring gives: every frame before the one just handed over
   has been taken back (flush), so the ring has one free. Its lists are emptied and its streams'
   transfer buffers mapped, each made again small where its frames have needed a quarter of it at
   most for a while. */
static void frame_begin(void)
{
    struct gpu_frame *f;
    struct GpuBytes *streams[FRAME_STREAMS];
    int i;
    f = G.f = &G.frames[render_thread_begin(G.thread)];
    if (!++G.frame)
        G.frame = 1;
    f->number = G.frame;
    f->nsteps = f->ndraws = f->nuploads = f->frameUploads = 0;
    f->nskinUploads = 0;
    memset(f->clearBlocks, 0, sizeof f->clearBlocks);
    memset(&f->download, 0, sizeof f->download);
    f->instancesAt = -1;
    f->state = FRAME_DRAWN;
    f->drawsScreen = f->downloaded = 0;
    frame_streams(f, streams);
    for (i = 0; i < FRAME_STREAMS; i++) {
        struct GpuBytes *b = streams[i];
        b->staged = 1;
        b->size = 0;
        if (b->quiet >= STAGING_QUIET) {
            stage_release(b);
            b->quiet = 0;
        }
        if (b->buffer && !(b->data = SDL_MapGPUTransferBuffer(G.dev, b->buffer, false)))
            stage_release(b);
    }
    G.batch.open = G.skinBatch.open = 0;
    G.matrixIndex[0] = G.matrixIndex[1] = -1;
    for (i = 0; i < G.nlayers; i++)
        if (G.layers[i]->begin)
            G.layers[i]->begin(f->parts[i]);
}

/* The frame as the main thread hands it over: its streams unmapped, what was given up while it was
   recorded going with it, how it went. */
static void hand_over(struct gpu_frame *f, int state)
{
    struct GpuBytes *streams[FRAME_STREAMS];
    struct retired *list;
    size_t capacity;
    int i;
    frame_streams(f, streams);
    for (i = 0; i < FRAME_STREAMS; i++) {
        struct GpuBytes *b = streams[i];
        if (!b->buffer)
            continue;
        SDL_UnmapGPUTransferBuffer(G.dev, b->buffer);
        b->data = NULL;
        if (b->capacity > (4u << 20) && b->size <= b->capacity / 4)
            b->quiet++;
        else
            b->quiet = 0;
    }
    lock_handles();
    list = f->retired;
    capacity = f->retiredCapacity;
    f->retired = G.retiring;
    f->nretired = G.nretiring;
    f->retiredCapacity = G.retiringCapacity;
    G.retiring = list;
    G.retiringCapacity = capacity;
    G.nretiring = 0;
    unlock_handles();
    f->state = state;
    render_thread_post(G.thread);
}

/* What a flush also does: end the frame (gfx_present), showing the screen where the window is
   claimed (claim_window), waiting for the window's next picture or not; and download a rectangle of
   the screen (gfx_read_frame). */
struct flush_extra {
    int present, wait;
    SDL_GPUTexture *download;
    int x, y, w, h;
    unsigned char *out;  /* B8G8R8A8 rows */
    size_t pitch;        /* bytes from one row of out to the next */
};

/* Hands everything recorded since the last flush over to the render thread, which submits it in one
   command buffer (frame_record), and begins the next frame. First the frame handed over before is
   taken back, once the render thread has finished it, so that nothing it reads changes while the
   layers prepare theirs (the prepare hook) and the frame's buffers are made large enough; the frame
   takes the buffers, the layers' parts and its download as they are then. The window's next picture
   is waited for while the render thread finishes that frame, and shows it (present_begin). A frame
   the render thread could not submit puts back what it was to take up, which the frame being
   handed over was recorded without: that one is dropped as well, and the next is recorded with
   them. A download is waited for. */
static int flush(const struct flush_extra *extra)
{
    struct gpu_frame *f = G.f;
    struct present present;
    long instances = -1;
    int ok = 1, download, presenting, lost, pending = 0, k;
    size_t i;
    batch_flush();
    if (!G.dev)
        return 0;
    for (k = 0; k < G.nlayers; k++)
        if (G.layers[k]->pending && G.layers[k]->pending())
            pending = 1;
    if (!f->nsteps && !f->nuploads && !f->nskinUploads && !pending && !extra->present && !extra->download)
        return 1;
    presenting = extra->present && present_begin(extra->wait, &present);
    lost = retire_frames();
    if (presenting)
        present_end(&present);
    if (lost)
        goto failed;
    for (k = 0; k < G.nlayers; k++)
        if (G.layers[k]->prepare)
            G.layers[k]->prepare();
    /* every storage buffer holds one record at least */
    if (!f->records.size)
        gpu_bytes_add(&f->records, sizeof(struct GpuDraw), 16);
    if (!f->quads.size)
        gpu_bytes_add(&f->quads, 16, 16);
    if (!f->offsets.size)
        gpu_bytes_add(&f->offsets, 4, 4);
    instances = instances_needed((uint32_t)(f->records.size / sizeof(struct GpuDraw)));
    f->instancesAt = instances;
    f->instanceCount = G.instanceCount;
    f->buffers.instances = G.buffers.instances;
    if (instances == -2 || !f->records.size || !f->quads.size || !f->offsets.size
        || !ensure_buffer(&G.buffers.records, &G.recordsSize, (uint32_t)f->records.size,
                          SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || (f->rows.size
            && !ensure_buffer(&G.buffers.rows, &G.rowsSize, (uint32_t)f->rows.size,
                              SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ))
        || !ensure_buffer(&G.buffers.quads, &G.quadsSize, (uint32_t)f->quads.size,
                          SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.buffers.offsets, &G.offsetsSize, (uint32_t)f->offsets.size, SDL_GPU_BUFFERUSAGE_VERTEX)) {
        SDL_Log("gfx: frame buffers: %s", SDL_GetError());
        goto failed;
    }
    f->buffers = G.buffers;
    for (k = 0; k < G.nlayers; k++)
        if (G.layers[k]->hand_over && !G.layers[k]->hand_over(f->parts[k]))
            goto failed;
    for (i = 0; i < f->nsteps; i++)
        if ((f->steps[i].kind == STEP_PASS && f->steps[i].target == G.screen)
            || (f->steps[i].kind == STEP_COPY && G.screen && f->steps[i].dst == G.screen->tex))
            f->drawsScreen = 1;
    if (extra->download) {
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
            f->download.texture = extra->download;
            f->download.buffer = G.download;
            f->download.x = extra->x;
            f->download.y = extra->y;
            f->download.w = extra->w;
            f->download.h = extra->h;
            f->download.out = extra->out;
            f->download.pitch = extra->pitch;
        } else {
            ok = 0;
        }
    }
    for (k = 0; k < G.nlayers; k++)
        if (G.layers[k]->flushed)
            G.layers[k]->flushed(extra->present);
    download = f->download.out != NULL;
    frame_unpin(f);
    hand_over(f, FRAME_DRAWN);
    /* the frame comes back with its download (frame_run) */
    if (download) {
        retire_frames();
        ok = f->downloaded;
    }
    frame_begin();
    return ok;

failed:
    requeue_uploads(f, 1);
    frame_unpin(f);
    hand_over(f, FRAME_DROPPED);
    frame_begin();
    return 0;
}

/* Hands over what was recorded and waits for the render thread to submit it, and every frame
   before: what the main thread records into a command buffer of its own after this reaches the
   GPU after them. 0 where it did not. */
int gpu_flush(void)
{
    struct flush_extra extra;
    int ok;
    SDL_zero(extra);
    ok = flush(&extra);
    if (retire_frames())
        ok = 0;
    return ok;
}

/* The screen's picture, a rectangle of it or the whole without one, downloaded after everything
   recorded before it (render/gfx.h): the frame's work is handed over with the download, and the
   main thread waits for the render thread to submit it and for the GPU to finish it. */
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

/* A mipmap level of a layer of a texture, w by h texels of B8G8R8A8, downloaded (render/gfx.h,
   gfx_read_texture_level); the callers have submitted everything recorded before it, which may
   make the texture they read. */
int gpu_read_level(SDL_GPUTexture *tex, uint32_t layer, uint32_t levels, int level, uint32_t w, uint32_t h,
                   void *out)
{
    SDL_GPUTransferBufferCreateInfo info;
    SDL_GPUTransferBuffer *down;
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    SDL_GPUTextureRegion src;
    SDL_GPUTextureTransferInfo dst;
    SDL_GPUFence *fence;
    const unsigned char *p;
    int ok = 0;
    if (!tex || level < 0 || (uint32_t)level >= levels || !out)
        return -1;
    w = w >> level ? w >> level : 1;
    h = h >> level ? h >> level : 1;
    SDL_zero(info);
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    info.size = w * h * 4;
    down = SDL_CreateGPUTransferBuffer(G.dev, &info);
    cmd = down ? gpu_commands() : NULL;
    if (!cmd) {
        if (down)
            SDL_ReleaseGPUTransferBuffer(G.dev, down);
        return -1;
    }
    SDL_zero(src);
    SDL_zero(dst);
    src.texture = tex;
    src.mip_level = (Uint32)level;
    src.layer = layer;
    src.w = w;
    src.h = h;
    src.d = 1;
    dst.transfer_buffer = down;
    dst.pixels_per_row = w;
    dst.rows_per_layer = h;
    copy = SDL_BeginGPUCopyPass(cmd);
    SDL_DownloadFromGPUTexture(copy, &src, &dst);
    SDL_EndGPUCopyPass(copy);
    if (gpu_submit(cmd, &fence) && SDL_WaitForGPUFences(G.dev, true, &fence, 1)
        && (p = SDL_MapGPUTransferBuffer(G.dev, down, false))) {
        memcpy(out, p, (size_t)w * h * 4);
        SDL_UnmapGPUTransferBuffer(G.dev, down);
        ok = 1;
    }
    if (fence)
        SDL_ReleaseGPUFence(G.dev, fence);
    SDL_ReleaseGPUTransferBuffer(G.dev, down);
    return ok ? 0 : -1;
}

int gfx_read_texture_level(gfx_surface *s, int level, void *out)
{
    if (!G.dev || !s || s->kind != K_TEXTURE || !gpu_flush())
        return -1;
    if (s->layered)
        return s->array ? gpu_read_level(s->array->tex, (uint32_t)s->layer, (uint32_t)s->levels, level,
                                     (uint32_t)s->width, (uint32_t)s->height, out)
                        : -1;
    return gpu_read_level(s->tex, 0, (uint32_t)s->levels, level, (uint32_t)s->width, (uint32_t)s->height, out);
}

/* ---- the device ---- */

static int choose_vsync(void)
{
    const char *env = SDL_getenv("RAGEXE_VSYNC");
    return !(env && env[0] == '0' && !env[1]);
}

/* The storage buffers a shader stage may read, read-only and read-write together: what every
   Vulkan device takes (maxPerStageDescriptorStorageBuffers; Metal takes 31). Every shader keeps to
   it, packing what it reads into fewer buffers where it would read more (the static world's water
   its cells and sines, its cull the records, runs and chunks): the quads' and Granny's actors'
   vertex shaders read three. SDL tells nothing of the device's own limits, so a
   shader that reads more is not made on any device, rather than failing on the smallest alone. */
#define STORAGE_BUFFERS_MAX 4

SDL_GPUShader *gpu_shader(const struct GpuShaderCode *code)
{
    SDL_GPUShaderCreateInfo info;
    if (code->storageBuffers > STORAGE_BUFFERS_MAX) {
        SDL_SetError("a shader reads %u storage buffers, more than the renderer allows (%d)", code->storageBuffers,
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

SDL_GPUComputePipeline *gpu_compute(const struct GpuComputeCode *c)
{
    SDL_GPUComputePipelineCreateInfo info;
    if (c->code.storageBuffers + c->readwriteStorageBuffers > STORAGE_BUFFERS_MAX) {
        SDL_SetError("a compute shader reads %u storage buffers, more than the renderer allows (%d)",
                     c->code.storageBuffers + c->readwriteStorageBuffers, STORAGE_BUFFERS_MAX);
        return NULL;
    }
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
        if (numbers[i] <= want && SDL_GPUTextureSupportsSampleCount(G.dev, GPU_TARGET_FORMAT, counts[i])
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
    cmd = p ? gpu_commands() : NULL;
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
    ok = cmd && gpu_submit(cmd, NULL);
    SDL_ReleaseGPUTransferBuffer(G.dev, buffer);
    return ok;
}

static void destroy_device(void);

/* The device with everything it keeps, whole or not at all: a setup that fails lets go what it
   made. */
static int create_device(void)
{
    static const struct GpuShaderCode *const vs[PROG_COUNT] = { &quad_vs, &rows_vs, &skin_vs };
    static const struct GpuShaderCode *const fs[PROG_COUNT] = { &quad_fs, &rows_fs, &skin_fs };
    const char *debug = SDL_getenv("RAGEXE_GPU_DEBUG");
    SDL_GPUSamplerCreateInfo sampler;
    SDL_GPUTextureCreateInfo info;
    SDL_PropertiesID props;
    int i;
    if (G.dev)
        return 1;
    if (!G.recording && !(G.recording = SDL_CreateMutex())) {
        SDL_Log("gfx: SDL_CreateMutex: %s", SDL_GetError());
        return 0;
    }
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
    for (i = 0; i < PROG_COUNT; i++) {
        G.vs[i] = gpu_shader(vs[i]);
        G.fs[i] = gpu_shader(fs[i]);
        if (!G.vs[i] || !G.fs[i]) {
            SDL_Log("gfx: SDL_CreateGPUShader: %s", SDL_GetError());
            destroy_device();
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
    G.white = gpu_new_texture(1, 1, 1, SDL_GPU_TEXTUREUSAGE_SAMPLER);
    SDL_zero(info);
    info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
    info.format = GPU_TARGET_FORMAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = info.height = 1;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    G.noAtlas = gpu_make_texture(&info);
    if (!G.pointClamp || !G.white || !G.noAtlas || !make_samplers() || !upload_constants()) {
        SDL_Log("gfx: device resources: %s", SDL_GetError());
        destroy_device();
        return 0;
    }
    for (i = 0; i < G.nlayers; i++)
        if (G.layers[i]->made && !G.layers[i]->made()) {
            destroy_device();
            return 0;
        }
    G.thread = render_thread_start(frame_run, NULL);
    if (!G.thread) {
        SDL_Log("gfx: the render thread: %s", SDL_GetError());
        destroy_device();
        return 0;
    }
    frame_begin();
    return 1;
}

/* The window claimed for the device, to take its pictures, where it can be: it is claimed, and
   gets its swapchain, at the first present while it is shown, not minimized and of a size in
   pixels. A window never shown (the scene harness's) is never claimed, so there is no swapchain at
   all, and its frames are drawn and can be read (gfx_read_frame), not shown. A claim counts once
   SDL keeps the window, which it says by knowing its swapchain's format: SDL 3.4's Vulkan driver
   answers true for a window whose surface has no size (a minimized one on Windows) but keeps
   nothing of it. A claim that fails or is not kept is tried again at the next present: on Android,
   the window has no surface while the app is in the background. */
static void claim_window(void)
{
    SDL_GPUPresentMode mode = SDL_GPU_PRESENTMODE_VSYNC;
    int w = 0, h = 0;
    if (G.claimed)
        return;
    if (!G.window || (SDL_GetWindowFlags(G.window) & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED))
        || !SDL_GetWindowSizeInPixels(G.window, &w, &h) || w <= 0 || h <= 0)
        return;
    if (!SDL_ClaimWindowForGPUDevice(G.dev, G.window)) {
        /* said once, not at every present it is tried again */
        if (!G.claimLogged)
            SDL_Log("gfx: SDL_ClaimWindowForGPUDevice: %s", SDL_GetError());
        G.claimLogged = 1;
        return;
    }
    if (SDL_GetGPUSwapchainTextureFormat(G.dev, G.window) == SDL_GPU_TEXTUREFORMAT_INVALID)
        return;
    G.claimed = 1;
    G.claimLogged = 0;
    if (!choose_vsync()) {
        if (SDL_WindowSupportsGPUPresentMode(G.dev, G.window, SDL_GPU_PRESENTMODE_IMMEDIATE))
            mode = SDL_GPU_PRESENTMODE_IMMEDIATE;
        else if (SDL_WindowSupportsGPUPresentMode(G.dev, G.window, SDL_GPU_PRESENTMODE_MAILBOX))
            mode = SDL_GPU_PRESENTMODE_MAILBOX;
    }
    SDL_SetGPUSwapchainParameters(G.dev, G.window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode);
}

int gfx_open(PlatformWindow window)
{
    G.window = window;
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
    info.format = depth ? G.depthFormat : GPU_TARGET_FORMAT;
    info.usage = depth ? SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET : SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    info.width = (Uint32)(w + GPU_MARGIN);
    info.height = (Uint32)(h + GPU_MARGIN);
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.sample_count = G.samples;
    return gpu_make_texture(&info);
}

/* The screen, the one surface the client draws the frame into and the window shows, a render
   target that holds the resolved picture where the screen is multisampled. */
gfx_surface *gfx_create_screen(int w, int h)
{
    struct gfx_surface *screen;
    if (!G.window || !create_device())
        return NULL;
    reset_state();
    screen = new_handle(K_SCREEN, w, h, GFX_FMT_8888);
    if (!screen)
        return NULL;
    G.screen = screen;
    G.screenShown = 0;
    G.samples = SDL_GPU_SAMPLECOUNT_1;
    screen->tex = new_target_texture(w, h);
    if (!screen->tex)
        return NULL;
    G.target = screen;
    return screen;
}

int gfx_create_zbuffer(void)
{
    if (!G.screen)
        return GFX_E_ZBUFFER;
    G.samples = pick_samples();
    if (G.samples != SDL_GPU_SAMPLECOUNT_1) {
        G.msaa = new_samples_texture(G.screen->width, G.screen->height, 0);
        if (!G.msaa)
            G.samples = SDL_GPU_SAMPLECOUNT_1;
    }
    G.depth = new_samples_texture(G.screen->width, G.screen->height, 1);
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

void gfx_close(void)
{
    if (G.dev)
        gpu_flush();
    G.target = G.screen = NULL;
    destroy_device();
}

/* The device and everything made with it let go: a device whole, every frame handed over taken
   back (gpu_flush), or what a setup that failed had made (create_device). */
static void destroy_device(void)
{
    struct gfx_surface *s, *next;
    struct gfx_skin *k;
    int i;
    if (G.dev) {
        /* the render thread ends, and the GPU finishes the frames it submitted */
        render_thread_stop(G.thread);
        G.thread = NULL;
        for (i = 0; i < FRAME_RING_SLOTS; i++)
            frame_wait(&G.frames[i]);
        SDL_WaitForGPUIdle(G.dev);
        lock_handles();
        for (s = G.handles; s; s = next) {
            next = s->next;
            if (s->tex)
                SDL_ReleaseGPUTexture(G.dev, s->tex);
            s->tex = NULL;
            s->array = NULL;
            s->failed = 0;
            dirty_locked(s, NULL);
        }
        while (G.arrays) {
            struct gpu_array *a = G.arrays;
            G.arrays = a->next;
            SDL_ReleaseGPUTexture(G.dev, a->tex);
            free(a->taken);
            free(a);
        }
        for (k = G.skins; k; k = k->next) {
            skin_buffers_release(k);
            k->failed = 0;
        }
        unlock_handles();
        for (i = 0; i < G.nlayers; i++)
            if (G.layers[i]->lost)
                G.layers[i]->lost();
        lock_handles();
        release_retired(G.retiring, G.nretiring);
        G.nretiring = 0;
        unlock_handles();
        for (i = 0; i < FRAME_RING_SLOTS; i++) {
            struct GpuBytes *streams[FRAME_STREAMS];
            int j;
            frame_streams(&G.frames[i], streams);
            for (j = 0; j < FRAME_STREAMS; j++) {
                stage_release(streams[j]);
                streams[j]->size = 0;
                streams[j]->quiet = 0;
            }
            release_retired(G.frames[i].retired, G.frames[i].nretired);
            G.frames[i].nretired = 0;
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
        if (G.depth) SDL_ReleaseGPUTexture(G.dev, G.depth);
        if (G.msaa) SDL_ReleaseGPUTexture(G.dev, G.msaa);
        if (G.buffers.records) SDL_ReleaseGPUBuffer(G.dev, G.buffers.records);
        if (G.buffers.rows) SDL_ReleaseGPUBuffer(G.dev, G.buffers.rows);
        if (G.frameCopy) SDL_ReleaseGPUTexture(G.dev, G.frameCopy);
        if (G.buffers.quads) SDL_ReleaseGPUBuffer(G.dev, G.buffers.quads);
        if (G.buffers.offsets) SDL_ReleaseGPUBuffer(G.dev, G.buffers.offsets);
        if (G.corners) SDL_ReleaseGPUBuffer(G.dev, G.corners);
        if (G.triangles) SDL_ReleaseGPUBuffer(G.dev, G.triangles);
        if (G.buffers.instances) SDL_ReleaseGPUBuffer(G.dev, G.buffers.instances);
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
    memset(G.skinPipeline, 0, sizeof G.skinPipeline);
    memset(G.effectPipeline, 0, sizeof G.effectPipeline);
    G.rowsPipeline = NULL;
    memset(G.samplers, 0, sizeof G.samplers);
    memset(G.vs, 0, sizeof G.vs);
    memset(G.fs, 0, sizeof G.fs);
    G.pointClamp = NULL;
    G.white = G.noAtlas = G.depth = G.msaa = G.frameCopy = NULL;
    G.frameCopyW = G.frameCopyH = 0;
    memset(&G.buffers, 0, sizeof G.buffers);
    G.corners = G.triangles = NULL;
    G.recordsSize = G.rowsSize = G.quadsSize = G.offsetsSize = 0;
    G.instanceCount = 0;
    G.download = NULL;
    G.downloadSize = 0;
    G.screenShown = 0;
    G.f = NULL;
}

/* The present mode follows RAGEXE_VSYNC (claim_window). Without vsync the frame never waits for
   the window: it is drawn but not shown unless a refresh has passed since the last picture
   (present_begin). A window not claimed shows nothing; the frame is drawn and ends all the same. */
int gfx_present(int vsync)
{
    struct flush_extra extra;

    if (!G.dev || !G.screen)
        return -1;
    claim_window();
    SDL_zero(extra);
    extra.present = 1;
    extra.wait = vsync;
    flush(&extra);
    return 0;
}

/* custom: the screen, its multisampled picture and depth at w by h, for a window resized to that,
   once everything recorded before has been handed over and submitted; the swapchain follows the
   window by itself. Without memory for them it answers -1 and keeps the old ones. */
int gfx_resize(int w, int h)
{
    SDL_GPUTexture *screen, *msaa = NULL, *depth = NULL;
    if (!G.dev || !G.screen || w <= 0 || h <= 0)
        return -1;
    gpu_flush();
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
    retire_texture(G.screen->tex);
    G.screen->tex = screen;
    if (G.msaa) {
        retire_texture(G.msaa);
        G.msaa = msaa;
    }
    if (G.depth) {
        retire_texture(G.depth);
        G.depth = depth;
    }
    G.screen->version = next_version();
    G.screen->width = w;
    G.screen->height = h;
    G.screenShown = 0;
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
    lock_handles();
    s->fresh = 1;
    unlock_handles();
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
int gfx_lock(gfx_surface *s, const RECT *r, struct gfx_lock *out)
{
    RECT rect;
    int bytes;
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
    if (!G.dev || !t)
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
    G.frameCopy = gpu_new_texture(G.screen->width, G.screen->height, 1, SDL_GPU_TEXTUREUSAGE_SAMPLER);
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
    if (!G.dev || !screen || !screen->tex || !G.rowsPipeline || !rows || count <= 0 || !fill
        || !frame_copy_ready())
        return 0;
    /* what the frame holds with the draws batched so far, to take back to */
    nsteps = G.f->nsteps;
    ndraws = G.f->ndraws;
    nrecords = G.f->records.size;
    nrows = G.f->rows.size;
    at = gpu_bytes_add(&G.f->rows, (size_t)screen->height * sizeof *table, 16);
    copy = at >= 0 ? add_step(STEP_COPY) : NULL;
    pass = copy ? add_step(STEP_PASS) : NULL;
    d = pass ? add_draw(pass) : NULL;
    if (d)
        d->record = add_record(&r);
    if (!r) {
        G.f->nsteps = nsteps;
        G.f->ndraws = ndraws;
        G.f->records.size = nrecords;
        G.f->rows.size = nrows;
        return 0;
    }
    /* add_step may have moved the steps */
    copy = &G.f->steps[nsteps];
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
    pass_targets(pass);
    d->d.program = PROG_ROWS;
    d->d.pipeline = G.rowsPipeline;
    d->d.bind[0].texture = G.frameCopy;
    d->d.bind[0].sampler = G.pointClamp;
    d->count = 3;
    r->mode[3] = (uint32_t)(at / (long)sizeof *table);
    fill(context);
    table = (uint32_t *)(G.f->rows.data + at);
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

int gfx_get_levels(gfx_surface *s)
{
    return s && s->kind == K_TEXTURE ? s->levels : 1;
}

int gpu_texture_texels(gfx_surface *s, struct GpuTexels *out)
{
    if (!s || s->kind != K_TEXTURE)
        return 0;
    out->width = s->width;
    out->height = s->height;
    out->levels = s->levels;
    out->fmt = s->fmt;
    out->pixels = s->pixels;
    return 1;
}

void gpu_surface_ref(gfx_surface *s)
{
    s->refs++;
}

/* ---- the layers ---- */

SDL_GPUDevice *gpu_dev(void)
{
    return G.dev;
}

SDL_GPUTextureFormat gpu_depth_format(void)
{
    return G.depthFormat;
}

SDL_GPUTexture *gpu_white(void)
{
    return G.white;
}

SDL_GPUTexture *gpu_no_atlas(void)
{
    return G.noAtlas;
}

SDL_GPUSampler *gpu_point_clamp(void)
{
    return G.pointClamp;
}

int gpu_screen_size(int *w, int *h)
{
    if (!G.screen)
        return 0;
    *w = G.screen->width;
    *h = G.screen->height;
    return 1;
}

/* Each packet takes the layer's part, zeroed, for good. */
int gpu_add_layer(const struct GpuLayer *layer)
{
    void *parts[FRAME_RING_SLOTS];
    int i, j;
    for (i = 0; i < G.nlayers; i++)
        if (G.layers[i] == layer)
            return i;
    if (!layer || G.dev || G.nlayers == GPU_LAYERS_MAX)
        return -1;
    for (j = 0; j < FRAME_RING_SLOTS; j++) {
        parts[j] = layer->partSize ? calloc(1, layer->partSize) : NULL;
        if (layer->partSize && !parts[j]) {
            while (j--)
                free(parts[j]);
            return -1;
        }
    }
    for (j = 0; j < FRAME_RING_SLOTS; j++)
        G.frames[j].parts[G.nlayers] = parts[j];
    G.layers[G.nlayers] = layer;
    return G.nlayers++;
}

struct gfx_caps g_gfxCaps;
