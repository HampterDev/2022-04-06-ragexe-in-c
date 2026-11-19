/* render/gfx.h on SDL 3's GPU API: Vulkan with SPIR-V shaders on Windows and Linux, Metal with
   MSL on macOS, one shader format handed to SDL_CreateGPUDevice so that SDL has one driver to
   choose. The shaders are HLSL compiled offline (src/render/shaders, tools/shaders).

   Draws are made in modes (render/draw_modes.h): the pipelines of every mode, for every kind of
   target it draws into, are made once, when the screen's samples and depth are known
   (gfx_create_zbuffer), and so are the samplers. A frame is recorded, not drawn: each draw becomes
   a record of what its mode and the draw itself give the shaders (struct GpuDraw, read from a
   storage buffer: a quad's through its own record in the frame's quads, a Granny actor's through
   its pose's header there, a draw of the static world's as a uniform, the wobble's through an
   instance-rate attribute that holds its index) in a list of passes, and what it draws and the
   texels of the textures it uses since they changed go into one staging area. The client's quads
   and triangles are instances (render/quads.h): a record each in the frame's quads, which the quad
   shader makes the vertices of, batched into draws while what they are drawn with stays the same;
   the sprite atlas's pages are layers of texture arrays, which draws batch across. Granny's models
   stay on the GPU once a draw has taken them up, and their actors are skinned by the skin shader in
   the poses the frame's quads hold (gfx_draw_skin). When the frame is presented, or the screen's
   picture is read (gfx_read_frame), one command buffer copies all of that through one transfer
   buffer in one copy pass and replays the passes. Textures keep their pixels on the CPU in the
   client's own format, so that the model loader's thread creates, locks and fills them without
   calling SDL; the main thread widens them to B8G8R8A8 when a draw first uses them after a change.
   Render targets live on the GPU, and nothing of them comes back to the CPU but the screen's
   picture when it is read.

   custom: the static world (gfx_world_*) stays on the GPU from frame to frame: its meshes in shared
   vertex and index buffers, its textures in texture arrays, its draw records, transform slots and
   cells of water. Its meshes and textures go up through a stream of their own as they arrive, a
   bounded transfer buffer at a time (world_stream); its records, slots and cells with the frame's
   upload. After the copy pass three compute passes compute the slots' views (world_transform.hlsl),
   cull the records for the view, giving each run's kept records their places in the frame's index
   buffer of them and writing the run's one indirect draw (world_cull.hlsl), and copy the kept
   records' indices to their places, 64 at a time (world_compact.hlsl); the main pass draws the
   ground and the opaque models with those draws. In a frame with shadows, a depth pass then draws
   the view's records into two targets of view depth (world_depth.hlsl), three compute passes fit
   the shadow cascades to them (shadow_range, shadow_extents and shadow_cascades.hlsl), the cull and
   the compaction run again for each cascade's casters, and the shadow pass draws those into the
   cascades' maps, all before the frame's own passes, which receive from the fit's buffer: the CPU
   never sees the cascades. */
#include "render/gfx.h"
#include "render/device.h"
#include "render/dirty_rects.h"
#include "render/present_fit.h"
#include "render/screen_wobble.h"
#include "render/texel_convert.h"
#include "render/world_layout.h"
#include "render/skin_layout.h"
#include "render/shaders/shader_code.h"
#include "render/shaders/quad.h"
#include "render/shaders/world.h"
#include "render/shaders/world_caster.h"
#include "render/shaders/world_depth.h"
#include "render/shaders/world_transform.h"
#include "render/shaders/world_cull.h"
#include "render/shaders/world_compact.h"
#include "render/shaders/shadow_range.h"
#include "render/shaders/shadow_extents.h"
#include "render/shaders/shadow_cascades.h"
#include "render/shaders/water.h"
#include "render/shaders/skin.h"
#include "render/shaders/rows.h"
#include "diag/perf.h"
#include "platform/clock.h"
#include <SDL3/SDL.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { K_SCREEN, K_SYSMEM, K_TEXTURE, K_RTTEX };

#define CLIENT_SURFACE_KEY 0x00ff00ffu
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

/* A view to clip space among the frame's views, which a water draw's record names (mode[3]) and
   push_uniforms hands its vertex shader as its uniform; no shader reads the views from a buffer. */
struct GpuView {
    float toClip[16];
};

enum { PROG_QUAD, PROG_WORLD, PROG_CASTER, PROG_DEPTH, PROG_WATER, PROG_ROWS, PROG_SKIN, PROG_COUNT };
/* The cull modes of the pipelines: Direct3D 9's, which culls by the winding on the screen, y down:
   CULL_CW drops the clockwise triangles, CULL_CCW the others. */
enum { CULL_NONE = 1, CULL_CW, CULL_CCW };
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
   models' part of the transfer buffer */
struct skin_upload {
    struct gfx_skin *model;
    uint32_t at;
};

/* One recorded draw: of the frame's quads (PROG_QUAD, `count` instances from `record`, the first's
   place among the offsets of their records), of the world's records, one (`count` indices from
   firstIndex after baseVertex) or a run's indirect command, the firstIndex-th (indirect), of its
   water's cells (PROG_WATER, `instances` of them from firstInstance of the frame's), the wobble's
   (`count` vertices, its draw record `record`), or of a Granny model's (PROG_SKIN, `count` of its
   indices from firstIndex in `instances` poses, their headers' places among the quads' offsets
   from firstInstance); a world's draw is of the world of `generation` (gfx_world_generation). */
struct gpu_draw {
    SDL_GPUGraphicsPipeline *pipeline;
    int program;
    SDL_GPUTextureSamplerBinding bind[3];
    uint32_t firstIndex, count;
    int indirect;
    int32_t baseVertex;
    uint32_t instances, firstInstance;
    uint32_t record;
    uint32_t uniform[4];    /* what the world's programs take as their uniform: the draw's record
                               (the water: its record, the waves' count and the cells', where its
                               sines start) */
    int generation;
    struct gfx_skin *skin;
    /* the world's programs: the texture arrays bind[0] and bind[1] read (SW_NO_ARRAY: white), whose
       textures run_pass takes as they are once the flush has made and grown them (world_array) */
    uint8_t arrays[2];
};

enum { STEP_PASS, STEP_UPLOADS, STEP_COPY };

/* A step of the frame: a render pass, the draws into one target; the uploads of textures changed
   after the frame's first draws; or a copy between GPU textures. */
struct gpu_step {
    int kind;
    /* STEP_PASS */
    struct gfx_surface *target;
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
    int fromLayer;     /* copies the layer's top level into its scratch texture, not texels */
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
    int back;             /* PROG_DEPTH: whether it writes the second target, the opaque depth */
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
    struct GfxWorldView worldCamera;
    int worldCameraSet;

    /* shadows: whether the device draws them (gfx_shadows_supported; -1 for good once something
       they need could not be made), the cascades' maps and their depth, what a draw reads without
       them, and the depth pass's targets at the screen's size, or the size they could not be made
       at, which is not tried again */
    int shadowState;
    SDL_GPUTexture *shadowMaps, *shadowDepth, *noShadow;
    SDL_GPUTexture *fitFront, *fitBack, *fitDepth;
    int fitW, fitH, fitFailedW, fitFailedH;

    /* fixed resources */
    SDL_GPUShader *vs[PROG_COUNT], *fs[PROG_COUNT];
    const struct GpuShaderCode *vsCode[PROG_COUNT], *fsCode[PROG_COUNT];
    /* every pipeline, made once (make_pipelines), and which serves what: a mode's quads into each
       kind of target, flat or not; its world records by cull mode (CULL_* - 1) and its water; the
       effect modes' quads by depth test or none and blend factors; the depth pass's by whether it
       writes the opaque depth and cull mode; the shadow casters; the wobble */
    struct { struct pipe_desc desc; SDL_GPUGraphicsPipeline *pipeline; } pipelines[PIPELINE_MAX];
    int npipelines;
    SDL_GPUGraphicsPipeline *quadPipeline[DRAW_MODE_COUNT][CLASS_COUNT][2];
    SDL_GPUGraphicsPipeline *worldPipeline[DRAW_MODE_COUNT][3];
    SDL_GPUGraphicsPipeline *waterPipeline[DRAW_MODE_COUNT];
    SDL_GPUGraphicsPipeline *skinPipeline[DRAW_MODE_COUNT];
    SDL_GPUGraphicsPipeline *effectPipeline[2][DRAW_FACTOR_COUNT][DRAW_FACTOR_COUNT];
    SDL_GPUGraphicsPipeline *depthPipeline[2][3];
    SDL_GPUGraphicsPipeline *casterPipeline, *rowsPipeline;
    SDL_GPUSampler *samplers[2][3];  /* wrapped or not, by enum DrawFilter */
    SDL_GPUSampler *pointClamp;
    SDL_GPUTexture *white;
    SDL_GPUTexture *frameCopy;  /* the screen's picture as the wobble found it (gfx_wobble) */
    int frameCopyW, frameCopyH;
    SDL_GPUBuffer *records, *rows, *quads, *offsets, *instances;
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
       headers' places among the quads' offsets begin at firstInstance; every model, and those whose
       buffers the frame's upload fills, from their own arrays (skinBytes of them) */
    struct {
        int open;
        uint32_t record, firstInstance, prims, bytes;
        struct gfx_skin *model;
        struct gfx_surface *tex[2];
        struct SkinBatch range;
    } skinBatch;
    struct gfx_skin *skins;
    struct skin_upload *skinUploads;
    size_t nskinUploads, skinUploadCapacity;
    uint32_t skinBytes;
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
    struct bytes drawRecords, viewRecords, rowTables, texels;
    /* the instances' records with the matrices and vertices they name (render/quads.h), and the row
       each record starts at */
    struct bytes quadData, quadOffsets;
    struct gfx_surface **pins;
    size_t npins, pinCapacity;
    /* blocks of CLEAR_TEXELS like texels among this frame's staged ones (clear_texels): the texel
       and the block's offset plus one, 0 for none */
    struct { uint32_t texel; long at; } clearBlocks[2];
    /* the surfaces and Granny models held, one reference each, until gfx_release_holds (gfx_hold,
       gfx_hold_skin) */
    struct gfx_surface **holds;
    size_t nholds, holdCapacity;
    struct gfx_skin **skinHolds;
    size_t nskinHolds, skinHoldCapacity;
    int cameraView;
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
    /* the records by id and in draw order, the view's runs (nindirect of the nruns compacted), the
       casters' runs and their order, which the cull and compaction read (cullData, as cullLayout
       lays it out), the chunks the compaction copies, and the frame's draws (commands and places,
       as drawLayout) and its compacted indices in WL_SECTIONS sections of sectionIndices each */
    struct WlRecord *byId;
    size_t byIdCapacity;
    struct WlCullRecord *cull;
    size_t cullCapacity;
    struct SwDrawRun *runs;
    size_t nruns, runCapacity;
    struct WlRun *indirect;
    size_t nindirect, indirectCapacity;
    struct WlRun *casterRuns;
    size_t ncasterRuns, casterRunCapacity;
    uint8_t *casterArrays;  /* the texture array of each of the casters' runs */
    size_t casterArrayCapacity;
    uint32_t *order;
    size_t orderHeld, orderCapacity;
    struct WlChunk *chunks;
    size_t nchunks, chunkCapacity;
    struct WlCullLayout cullLayout;
    struct WlDrawLayout drawLayout;
    uint32_t recordCount, idCount, recordsAt, sectionIndices;
    int recordsChanged;
    SDL_GPUBuffer *recordBuffer, *cullData, *draws, *drawn;
    uint32_t recordBufferSize, cullDataSize, drawsSize, drawnSize;
    /* the transform slots, and their views and places for the frame */
    struct WlSlot *slots;
    size_t slotCapacity;
    uint32_t slotCount, slotsAt;
    int slotsChanged;
    SDL_GPUBuffer *slotBuffer, *views, *placed;
    uint32_t slotBufferSize, viewsSize, placedSize;
    /* the water: its cells, then the table of sines, in one buffer */
    struct GpuWaterCell *cells;
    size_t cellCapacity;
    uint32_t cellCount, cellsAt;
    int cellsChanged;
    float sines[360];
    SDL_GPUBuffer *cellBuffer, *waveBuffer, *instanceBuffer;
    uint32_t cellBufferSize, waveSize, instanceSize;
    /* the frame: given (gfx_world_frame) until it is presented, with its shadows where shadowSet;
       its compute passes due, and run in this flush; and whether this flush's shadow passes could
       not all be recorded, after which its draws of the world receive none (run_pass) */
    struct GfxWorldView camera;
    struct SwFrustum frustum;
    struct GfxShadow shadow;
    int frameSet, shadowSet, frameDue, computeDue, computed, shadowFailed;
    uint32_t frameAt, waveCount;
    struct bytes animated, uvs, waves, instances;
    SDL_GPUBuffer *animatedBuffer, *uvBuffer;
    uint32_t animatedSize, uvSize;
    SDL_GPUComputePipeline *transform, *cullPipeline, *compactPipeline;
    /* the shadow fit: its passes, its state, the squares kept from frame to frame (cleared, with
       historyDue, when the world is made again) and its result, which every draw of the world
       binds */
    SDL_GPUComputePipeline *fitRange, *fitExtents, *fitCascades;
    SDL_GPUBuffer *fitState, *fitHistory, *fitResult;
    int historyDue, fitStateDue;
    uint32_t fitStateAt;
    /* the shadows the GPU fitted last (gfx_read_shadow_fit), where fitted */
    struct GfxShadow fitted;
    int fittedSet;
} W;

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
    if (!grow((void **)&G.holds, &G.holdCapacity, G.nholds + 1, sizeof *G.holds))
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
    s->tex = new_texture(s->width, s->height, s->levels,
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
    at = bytes_add(&G.texels, (size_t)w * h * 4, 16);
    if (at < 0)
        return 0;
    src = s->pixels + (size_t)r->top * s->width * bytes + (size_t)r->left * bytes;
    start = platform_stopwatch_ns();
    if (s->kind == K_SYSMEM)
        texel_rows_keyed_to_bgra(src, (long)s->width * 4, w, h, s->key, G.texels.data + at, (long)w * 4);
    else
        texel_rows_to_bgra(s->fmt, src, (long)s->width * bytes, w, h, G.texels.data + at, (long)w * 4);
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
    struct gpu_upload *u;
    int late = s->pinned == G.frame;
    if (!grow((void **)&G.uploads, &G.uploadCapacity, G.nuploads + 1, sizeof *G.uploads))
        return NULL;
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
                return NULL;
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
    uint32_t texel = texel_unpack(s->fmt, 0), rows, y;
    long at;
    int i;
    if (s->width > CLEAR_TEXELS)
        return 0;
    for (i = 0; i < 2 && !(G.clearBlocks[i].at && G.clearBlocks[i].texel == texel); i++)
        ;
    if (i == 2) {
        uint32_t *block;
        long k;
        i = G.clearBlocks[0].at ? 1 : 0;
        at = bytes_add(&G.texels, CLEAR_TEXELS * 4, 16);
        if (at < 0)
            return 0;
        block = (uint32_t *)(G.texels.data + at);
        for (k = 0; k < CLEAR_TEXELS; k++)
            block[k] = texel;
        G.clearBlocks[i].texel = texel;
        G.clearBlocks[i].at = at + 1;
    }
    at = G.clearBlocks[i].at - 1;
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
    scratch = new_texture(s->width, s->height, s->levels, SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET);
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
    SDL_GPUColorTargetDescription colour[2];
    SDL_GPUVertexBufferDescription buffers[2];
    SDL_GPUVertexAttribute attrs[8];
    Uint32 nattrs = 0;
    Uint32 nbuffers = 2;
    SDL_zero(info);
    SDL_zero(colour);
    SDL_zero(buffers);
    SDL_zero(attrs);
    /* the quads: the corner, then where the instance's record starts in the frame's quads; the
       world's records, its depth pass and its casters: their vertices, then each vertex's record id;
       Granny's models: their vertices, then where each pose's header lies in the frame's quads; the
       water: a cell and its colour per instance; the wobble: the draw's record per instance alone
       (instances_needed) */
    buffers[0].slot = 0;
    buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    buffers[1].slot = 1;
    buffers[1].pitch = 4;
    buffers[1].input_rate = p->program == PROG_QUAD || p->program == PROG_SKIN ? SDL_GPU_VERTEXINPUTRATE_INSTANCE
                                                                               : SDL_GPU_VERTEXINPUTRATE_VERTEX;
#define ATTR(loc, slot, fmt, off) (attrs[nattrs].location = (loc), attrs[nattrs].buffer_slot = (slot), \
                                   attrs[nattrs].format = (fmt), attrs[nattrs].offset = (off), nattrs++)
    if (p->program == PROG_QUAD) {
        buffers[0].pitch = 4;
        ATTR(0, 0, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
        ATTR(1, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
    } else if (p->program == PROG_WORLD) {
        buffers[0].pitch = sizeof(struct GfxMeshVertex);
        ATTR(0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
        ATTR(1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 12);
        ATTR(2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 24);
        ATTR(3, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 28);
        ATTR(4, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 32);
        ATTR(5, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 40);
        ATTR(6, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
    } else if (p->program == PROG_CASTER || p->program == PROG_DEPTH) {
        buffers[0].pitch = sizeof(struct GfxMeshVertex);
        ATTR(0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
        ATTR(1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 32);
        ATTR(2, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
    } else if (p->program == PROG_SKIN) {
        buffers[0].pitch = sizeof(struct SkinVertex);
        ATTR(0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(struct SkinVertex, position));
        ATTR(1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(struct SkinVertex, normal));
        ATTR(2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(struct SkinVertex, uv));
        ATTR(3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, offsetof(struct SkinVertex, weights));
        ATTR(4, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4, offsetof(struct SkinVertex, bones));
        ATTR(5, 0, SDL_GPU_VERTEXELEMENTFORMAT_UINT, offsetof(struct SkinVertex, flags));
        ATTR(6, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
    } else if (p->program == PROG_WATER) {
        buffers[0].pitch = WATER_INSTANCE;
        buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
        ATTR(0, 0, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
        ATTR(1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 4);
        nbuffers = 1;
    } else {
        /* the wobble's triangle comes from the vertex index: the draw's record alone, its one
           buffer at slot 0, which a draw binds as the first */
        buffers[0].pitch = 4;
        buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
        ATTR(0, 0, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
        nbuffers = 1;
    }
#undef ATTR
    info.vertex_shader = G.vs[p->program];
    info.fragment_shader = G.fs[p->program];
    info.vertex_input_state.vertex_buffer_descriptions = buffers;
    info.vertex_input_state.num_vertex_buffers = nbuffers;
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
    colour[0].format = p->r32f ? SDL_GPU_TEXTUREFORMAT_R32_FLOAT : TARGET_FORMAT;
    colour[0].blend_state.enable_blend = p->blend != 0;
    colour[0].blend_state.src_color_blendfactor = sdl_factor(p->src);
    colour[0].blend_state.dst_color_blendfactor = sdl_factor(p->dst);
    colour[0].blend_state.src_alpha_blendfactor = colour[0].blend_state.src_color_blendfactor;
    colour[0].blend_state.dst_alpha_blendfactor = colour[0].blend_state.dst_color_blendfactor;
    colour[0].blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colour[0].blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    colour[0].blend_state.enable_color_write_mask = true;
    colour[0].blend_state.color_write_mask = p->mask == MASK_NONE ? 0
        : p->mask == MASK_RGB ? (SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B)
        : (SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A);
    /* the depth pass's second target, the opaque depth, which the blended passes leave */
    colour[1] = colour[0];
    colour[1].blend_state.color_write_mask = p->back ? SDL_GPU_COLORCOMPONENT_R : 0;
    info.target_info.color_target_descriptions = colour;
    info.target_info.num_color_targets = p->program == PROG_DEPTH ? 2 : 1;
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
    /* the screen is X8R8G8B8 on Direct3D 9: its alpha stays 1 */
    p->mask = !m->colour ? MASK_NONE : cls == CLASS_TEXTURE ? MASK_RGBA : MASK_RGB;
    p->hasDepth = cls == CLASS_SCREEN && G.depth != NULL;
    p->samples = cls == CLASS_SCREEN ? (int)G.samples : (int)SDL_GPU_SAMPLECOUNT_1;
    p->a2c = alpha_to_coverage(m, p->samples, flat);
}

static void shadows_off(const char *what);

/* Every pipeline the modes draw with, once the screen's samples and depth are known: a mode's
   quads into each kind of target it draws into, flat and not, its world records in each cull mode,
   its water and Granny's models, the effect modes' quads by every pair of the blend factors the
   screen has, the wobble's (gfx_wobble), and the depth pass's and the shadow casters', without which
   the device draws no shadows. */
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
           coordinates, and Granny's models among the opaque lists' quads, both faces of each
           triangle as the quads */
        if ((m->into & DRAW_INTO_SCREEN) && (m->into & DRAW_MESHES)) {
            mode_pipe(&p, PROG_WATER, m, CLASS_SCREEN, 0, 0, 0);
            if (!(G.waterPipeline[mode] = pipeline(&p)))
                return 0;
            mode_pipe(&p, PROG_SKIN, m, CLASS_SCREEN, 0, 0, 0);
            if (!(G.skinPipeline[mode] = pipeline(&p)))
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
    /* the wobble's, into the screen with its depth kept, its alpha 1 */
    memset(&p, 0, sizeof p);
    p.program = PROG_ROWS;
    p.depth = DRAW_DEPTH_OFF;
    p.cull = CULL_NONE;
    p.mask = MASK_RGB;
    p.hasDepth = G.depth != NULL;
    p.samples = (int)G.samples;
    if (!(G.rowsPipeline = pipeline(&p)))
        return 0;
    /* the shadows' own, last, which only turn the shadows off where they cannot be made: the depth
       pass's, single-sampled into two targets of view depth, by whether it writes the second (the
       passes up to the ground) and by cull mode, and the casters' */
    if (G.shadowState < 0)
        return 1;
    for (d = 0; d < 2; d++)
        for (cull = CULL_NONE; cull <= CULL_CCW; cull++) {
            memset(&p, 0, sizeof p);
            p.program = PROG_DEPTH;
            p.depth = DRAW_DEPTH_WRITE;
            p.cull = cull;
            p.mask = MASK_RGBA;
            p.r32f = 1;
            p.hasDepth = 1;
            p.samples = SDL_GPU_SAMPLECOUNT_1;
            p.back = d;
            if (!(G.depthPipeline[d][cull - CULL_NONE] = pipeline(&p))) {
                shadows_off("the depth pass's pipelines");
                return 1;
            }
        }
    memset(&p, 0, sizeof p);
    p.program = PROG_CASTER;
    p.depth = DRAW_DEPTH_WRITE;
    p.cull = CULL_NONE;
    p.mask = MASK_RGBA;
    p.r32f = 1;
    p.hasDepth = 1;
    p.samples = SDL_GPU_SAMPLECOUNT_1;
    if (!(G.casterPipeline = pipeline(&p)))
        shadows_off("the casters' pipeline");
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
    if (memcmp(fog, &G.fog, sizeof *fog))
        G.cameraView = -1;
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

/* The matrices are the ones the Direct3D 9 client drew with (build/ref/COMMIT.txt), computed in
   floats as it computed them: the GPU divides their points and moves them by the viewport as
   Direct3D 9 did, half a pixel on (MARGIN). */

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
        return -1;
    view = (struct GpuView *)(G.viewRecords.data + at);
    memcpy(view->toClip, toClip, sizeof view->toClip);
    return (int)(at / (long)sizeof *view);
}

/* A world view in a target's pixels among the frame's views (struct GpuView), made once per frame
   and target size; -1 without memory. */
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
    d->program = PROG_QUAD;
    d->pipeline = pipe;
    memcpy(d->bind, bind, sizeof bind);
    d->count = count;
    d->record = first;
    r = (struct GpuDraw *)(G.drawRecords.data + (size_t)record * sizeof *r);
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

/* A matrix and projection in the frame's quads, as GFX_QUAD_WORLD's and a corner set's go through
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
    at = bytes_add(&G.quadData, sizeof m, 16);
    if (at < 0)
        return -1;
    memcpy(G.quadData.data + at, m, sizeof m);
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
    at = n ? bytes_add(&G.quadData, (size_t)n * 4, 16) : -1;
    if (at < 0)
        return -1;
    memcpy(G.quadData.data + at, words, (size_t)n * 4);
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
    at = bytes_add(&G.quadData, (size_t)n * sizeof *v, 16);
    if (at < 0)
        return 0;
    memcpy(G.quadData.data + at, v, (size_t)n * sizeof *v);
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
    first = (uint32_t)(G.quadOffsets.size / 4);
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

/* its buffers given up; the frames submitted may still draw with them, which SDL waits for */
static void skin_buffers_release(struct gfx_skin *k)
{
    if (G.dev) {
        if (k->vertexBuffer)
            SDL_ReleaseGPUBuffer(G.dev, k->vertexBuffer);
        if (k->indexBuffer)
            SDL_ReleaseGPUBuffer(G.dev, k->indexBuffer);
    }
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
    if (!grow((void **)&G.skinHolds, &G.skinHoldCapacity, G.nskinHolds + 1, sizeof *G.skinHolds))
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

/* The model's buffers on the GPU, made the first time a draw takes it and filled from its arrays by
   the frame's upload (the model stays pinned until then); 0 where they cannot be made, which is not
   tried again with this device. */
static int skin_ready(struct gfx_skin *k)
{
    SDL_GPUBufferCreateInfo info;
    size_t vbytes = (size_t)k->vertexCount * sizeof *k->vertices, ibytes = (size_t)k->indexCount * sizeof *k->indices;
    size_t at = ((size_t)G.skinBytes + 15) & ~(size_t)15;
    if (k->vertexBuffer)
        return 1;
    if (k->failed || at > UINT32_MAX || vbytes + ibytes > UINT32_MAX - at
        || !grow((void **)&G.skinUploads, &G.skinUploadCapacity, G.nskinUploads + 1, sizeof *G.skinUploads))
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
        return 0;
    }
    G.skinBytes = (uint32_t)(at + vbytes + ibytes);
    G.skinUploads[G.nskinUploads].model = k;
    G.skinUploads[G.nskinUploads++].at = (uint32_t)at;
    k->pinned = G.frame;
    g_perf.vertexBytes += (unsigned)(vbytes + ibytes);
    return 1;
}

int gfx_skin_pose(const float (*rows)[4], size_t count)
{
    long at;
    if (!rows || !count || count > UINT32_MAX / 16)
        return -1;
    at = bytes_add(&G.quadData, count * 16, 16);
    if (at < 0)
        return -1;
    memcpy(G.quadData.data + at, rows, count * 16);
    g_perf.instanceBytes += (unsigned)(count * 16);
    return (int)(at / 16);
}

/* A pose's header for the batch's draw: the pose's row and the draw's record, and its place among
   the quads' offsets, which the draw's instances read one after the other. */
static int skin_instance(int pose)
{
    long at = bytes_add(&G.quadData, 16, 16), offset;
    uint32_t *h;
    if (at < 0)
        return 0;
    offset = bytes_add(&G.quadOffsets, 4, 4);
    if (offset < 0) {
        G.quadData.size = (size_t)at;
        return 0;
    }
    h = (uint32_t *)(G.quadData.data + at);
    h[0] = (uint32_t)pose;
    h[1] = G.skinBatch.record;
    *(uint32_t *)(G.quadOffsets.data + offset) = (uint32_t)(at / 16);
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
    d->program = PROG_SKIN;
    d->pipeline = pipe;
    memcpy(d->bind, bind, sizeof bind);
    d->skin = G.skinBatch.model;
    d->firstIndex = G.skinBatch.range.first;
    d->count = G.skinBatch.range.count;
    d->instances = G.skinBatch.range.instances;
    d->firstInstance = G.skinBatch.firstInstance;
    r = (struct GpuDraw *)(G.drawRecords.data + (size_t)G.skinBatch.record * sizeof *r);
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
        G.skinBatch.firstInstance = (uint32_t)(G.quadOffsets.size / 4);
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

/* ---- the static world ---- */

/* The water's records as its shaders read them (src/render/shaders/water.hlsli). */
struct GpuWaterCell {
    uint32_t cell[4];
    float corners[8];
};

/* the water's uniform (water.hlsl, WaterFrame) */
struct GpuWaterFrame {
    uint32_t frame[4];  /* the draw's record, the waves, the cells */
    float view[12];
    float screen[4];
    float toClip[16];
};

#define INDIRECT_SIZE ((uint32_t)sizeof(SDL_GPUIndexedIndirectDrawCommand))
/* what the transform pass writes of a slot (world_transform.hlsl, world.hlsli): its views to clip
   and to view space, and its place in the world; and a texture-coordinate matrix as the shaders
   read it, two float4 */
#define SLOT_VIEWS ((uint32_t)sizeof(float[2][16]))
#define SLOT_PLACE ((uint32_t)sizeof(float[12]))
#define UV_MATRIX ((uint32_t)sizeof(float[2][4]))
/* a vertex of the world's: struct GfxMeshVertex, then its record's id beside it */
#define WORLD_VERTEX ((uint32_t)sizeof(struct GfxMeshVertex))
/* the stream that takes the world's meshes and textures up (world_stream): its transfer buffer
   holds this much, or one texture where that is larger */
#define STREAM_BYTES (32u << 20)
/* a staging transfer buffer this much larger than the frames' uploads have been for a while is
   made again as small as they need */
#define STAGING_QUIET 120

_Static_assert(GFX_SHADOW_CASCADES == WL_CASCADES, "the cull compacts every cascade's casters");
_Static_assert(GFX_SHADOW_CASCADES == SHADOW_FIT_CASCADES && GFX_SHADOW_SIZE == SHADOW_FIT_SIZE,
               "the maps are the fit's");

static int world_stream(int textures);
static void copy_levels(SDL_GPUCopyPass *copy, SDL_GPUTexture *scratch, SDL_GPUTexture *array, uint32_t layer,
                        uint32_t w, uint32_t h, uint32_t levels);

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
    SDL_GPUBuffer **buffers[] = { &W.vertices, &W.ids, &W.indices, &W.recordBuffer, &W.cullData, &W.draws, &W.drawn,
                                  &W.slotBuffer, &W.views, &W.placed, &W.animatedBuffer, &W.uvBuffer, &W.cellBuffer,
                                  &W.waveBuffer, &W.instanceBuffer };
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
    W.verticesSize = W.idsSize = W.indicesSize = W.recordBufferSize = W.cullDataSize = W.drawsSize = W.drawnSize = 0;
    W.slotBufferSize = W.viewsSize = W.placedSize = W.animatedSize = W.uvSize = 0;
    W.cellBufferSize = W.waveSize = W.instanceSize = 0;
    memset(W.arrays, 0, sizeof W.arrays);
    W.narrays = W.ncopies = W.narrayCopies = W.nmeshUploads = W.ntextureUploads = 0;
    W.meshes.size = 0;
    W.vertexCount = W.indexCount = W.uploadedVertices = W.uploadedIndices = 0;
    W.recordCount = W.idCount = W.slotCount = W.cellCount = 0;
    W.nruns = W.nindirect = W.ncasterRuns = W.orderHeld = W.nchunks = 0;
    W.sectionIndices = 0;
    W.recordsChanged = W.slotsChanged = W.cellsChanged = 0;
    W.computeDue = W.computed = 0;
    /* the shadows' squares go with the world they were fitted to; nothing staged for them is
       uploaded from a flush that never went up */
    W.historyDue = 1;
    W.fitStateDue = 0;
}

/* the device's world made again from nothing: the client builds it anew (gfx_world_generation) */
static void world_reset(void)
{
    world_release();
    W.frameSet = W.shadowSet = W.fittedSet = 0;
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
    texture->refs++;
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
    size_t i, chunks;
    uint32_t viewIndices, casterIndices;
    if (!world_on() || (count && !records) || count > 0x7fffffff || ids > 0x7fffffff)
        return 0;
    for (i = 0; i < count; i++)
        if (records[i].id >= ids)
            return 0;
    if (!grow((void **)&W.byId, &W.byIdCapacity, ids, sizeof *W.byId)
        || !grow((void **)&W.cull, &W.cullCapacity, count, sizeof *W.cull)
        || !grow((void **)&W.runs, &W.runCapacity, count, sizeof *W.runs)
        || !grow((void **)&W.indirect, &W.indirectCapacity, count, sizeof *W.indirect)
        || !grow((void **)&W.casterRuns, &W.casterRunCapacity, count, sizeof *W.casterRuns)
        || !grow((void **)&W.casterArrays, &W.casterArrayCapacity, count, 1)
        || !grow((void **)&W.order, &W.orderCapacity, count, sizeof *W.order))
        return 0;
    /* the chunks of every record: the view's cull and every cascade's take any of them */
    chunks = wl_chunks(records, count, NULL);
    if (chunks > 0x7fffffff || !grow((void **)&W.chunks, &W.chunkCapacity, chunks, sizeof *W.chunks))
        return 0;
    memset(W.byId, 0, ids * sizeof *W.byId);
    wl_records(records, count, W.byId, W.cull);
    W.nruns = sw_draw_runs(records, count, W.runs);
    W.nindirect = wl_runs(W.runs, W.nruns, W.indirect, &viewIndices);
    W.ncasterRuns = wl_caster_runs(records, count, W.order, &W.orderHeld, W.casterRuns, &casterIndices);
    for (i = 0; i < W.ncasterRuns; i++)
        W.casterArrays[i] = records[W.order[W.casterRuns[i].first]].textureArray;
    W.nchunks = wl_chunks(records, count, W.chunks);
    W.sectionIndices = viewIndices > casterIndices ? viewIndices : casterIndices;
    wl_cull_layout(count, W.nindirect, W.ncasterRuns, W.orderHeld, W.nchunks, &W.cullLayout);
    wl_draw_layout(count, W.nindirect, W.ncasterRuns, &W.drawLayout);
    /* every record in a run of the view's and of the casters', or none drawn: the compaction copies
       every record's chunks to the places the culls give, which they give the records of their runs
       alone */
    if (W.nindirect < W.nruns || W.orderHeld < count || W.cullLayout.words > 0x3fffffff
        || W.drawLayout.words > 0x3fffffff) {
        W.recordCount = 0;
        return 0;
    }
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

static int shadow_prepare(void);

void gfx_world_frame(const struct GfxWorldFrame *frame)
{
    long at;
    size_t i;
    W.frameSet = W.shadowSet = W.fittedSet = 0;
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
    if (frame->shadow && shadow_prepare()) {
        W.shadow = *frame->shadow;
        W.shadowSet = 1;
    }
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
    return world_on() && G.target && W.frameSet && G.target == G.screen && (m->into & DRAW_MESHES) && W.recordCount
           && W.slotCount;
}

/* The draw record of a draw of the world's records in the current mode, which its draws push as
   their uniform (push_uniforms). */
static uint32_t world_record(struct gpu_step *pass, const struct GfxWorldDraw *draw, struct GpuDraw **out)
{
    struct gfx_surface *none[2] = { NULL, NULL };
    struct GpuDraw *r;
    uint32_t at = add_record(&r);
    *out = r;
    if (!r)
        return 0;
    fill_state(r, pass, &G.state, none, 0, alpha_to_coverage(draw_mode(G.mode), (int)G.samples, 0));
    /* a stage reads with a border where the record has its texture (world.hlsl) */
    r->stages[1] = (G.state.address[0] == DRAW_ADDRESS_BORDER ? 1u : 0u)
                   | (G.state.address[1] == DRAW_ADDRESS_BORDER ? 2u : 0u);
    memcpy(r->colour, draw->colour, sizeof r->colour);
    r->mesh[0] = draw->kind == GFX_MESH_MODEL;
    r->mesh[1] = draw->kind == GFX_MESH_MODEL && draw->lightGround;
    r->mesh[3] = W.shadowSet ? 1u : 0u;
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
    d->bind[2].texture = W.shadowSet ? G.shadowMaps : G.noShadow;
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
        d->firstIndex = wl_view_command((uint32_t)i);
        d->uniform[0] = record;
        g_perf.draws++;
        g_perf.indirectDraws++;
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
    d->firstIndex = rec->firstIndex;
    d->count = rec->indexCount;
    d->baseVertex = (int32_t)rec->baseVertex;
    d->uniform[0] = record;
    g_perf.draws++;
    g_perf.prims += rec->indexCount / 3;
    W.computeDue = 1;
    G.target->version = next_version();
}

void gfx_world_draw_water(const uint32_t *cells, size_t count, DWORD colour)
{
    struct gpu_step *step;
    struct gpu_draw *d;
    struct GpuDraw *r;
    SDL_GPUTextureSamplerBinding bind[2];
    long at;
    size_t i;
    int view;
    g_perf.drawCalls++;
    batch_flush();
    if (!world_on() || !G.target || !cells || !count || !W.frameSet || !W.cellCount || !W.waveCount
        || !G.worldCameraSet || G.target != G.screen || !G.waterPipeline[G.mode] || count > 0x7fffffff)
        return;
    bind_stage(bind, &G.state, G.stageTex[0], 0);
    bind[1].texture = G.white;
    bind[1].sampler = stage_sampler(&G.state, 1);
    step = current_pass();
    if (!step)
        return;
    /* the camera's view in the screen's pixels, which push_uniforms hands the water as its uniform */
    view = view_for(&G.worldCamera, &G.cameraView, step);
    if (view < 0)
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
    fill_state(r, step, &G.state, G.stageTex, 1, alpha_to_coverage(draw_mode(G.mode), (int)pass_samples(step), 0));
    r->mode[3] = (uint32_t)view;
    d->bind[0] = bind[0];
    d->bind[1] = bind[1];
    /* the quads' pixels (quad.hlsli), whose stage 0 reads no layer of an atlas here */
    d->bind[2].texture = G.noAtlas;
    d->bind[2].sampler = bind[0].sampler;
    d->instances = (uint32_t)count;
    d->firstInstance = (uint32_t)(at / WATER_INSTANCE);
    d->uniform[0] = d->record;
    d->uniform[1] = W.waveCount;
    d->uniform[2] = W.cellCount;
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
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
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

/* A texture's top level from the stream at `at`: into its layer, or, where its array has mipmaps,
   into `scratch` (a texture of its size and levels), whose levels copy_levels then takes into the
   layer. */
static void stream_texture(SDL_GPUCopyPass *copy, const struct world_upload *u, uint32_t at, SDL_GPUTexture *scratch)
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
    dst.texture = scratch ? scratch : a->tex;
    dst.layer = scratch ? 0 : u->b;
    dst.w = a->width;
    dst.h = a->height;
    dst.d = 1;
    SDL_UploadToGPUTexture(copy, &src, &dst, false);
}

/* Takes what is pending of the world up through the stream (a transfer buffer of STREAM_BYTES,
   or of the largest texture where that is larger): the meshes, and with `textures` the textures
   too, as many as fit at a time, each batch in a command buffer of its own that is waited on
   before the next fills the buffer again; the buffers' and arrays' growth goes first. A texture
   goes up from its surface's own pixels, widened straight into the stream; one with mipmaps goes
   into a scratch texture of its own first, whose levels SDL makes as a texture's (copy_levels). 0
   where a batch could not go. */
static int world_stream(int textures)
{
    size_t mesh = 0, texture = 0, released = 0;
    uint32_t largest = STREAM_BYTES;
    int copies = 1;
    SDL_GPUTexture **scratch = NULL;
    size_t scratchCapacity = 0;
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
        int copied = copies, made = 1;
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
            int64_t start;
            if (n > W.streamSize - used)
                break;
            start = platform_stopwatch_ns();
            texel_rows_to_bgra(s->fmt, s->pixels, (long)s->width * texel_bytes(s->fmt), s->width, s->height,
                               mapped + used, (long)s->width * 4);
            g_perf.texelsNs += platform_stopwatch_ns() - start;
            used += (n + 15u) & ~15u;
        }
        SDL_UnmapGPUTransferBuffer(G.dev, W.stream);
        /* a scratch texture for each texture of the batch with mipmaps */
        if (!grow((void **)&scratch, &scratchCapacity, texture - textureFrom + 1, sizeof *scratch))
            goto failed;
        for (i = textureFrom; i < texture; i++) {
            const struct world_array *a = &W.arrays[W.textureUploads[i].a];
            scratch[i - textureFrom] = a->tex && a->levels > 1
                                           ? new_texture((int)a->width, (int)a->height, (int)a->levels,
                                                         SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET)
                                           : NULL;
            made = made && (scratch[i - textureFrom] || !a->tex || a->levels == 1);
        }
        cmd = made ? SDL_AcquireGPUCommandBuffer(G.dev) : NULL;
        if (!cmd) {
            for (i = textureFrom; i < texture; i++)
                if (scratch[i - textureFrom])
                    SDL_ReleaseGPUTexture(G.dev, scratch[i - textureFrom]);
            goto failed;
        }
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
                stream_texture(copy, &W.textureUploads[i], used, scratch[i - textureFrom]);
            used += (stream_bytes(&W.textureUploads[i], 1) + 15u) & ~15u;
        }
        SDL_EndGPUCopyPass(copy);
        copy = NULL;
        for (i = textureFrom; i < texture; i++)
            if (scratch[i - textureFrom])
                SDL_GenerateMipmapsForGPUTexture(cmd, scratch[i - textureFrom]);
        for (i = textureFrom; i < texture; i++) {
            const struct world_array *a = &W.arrays[W.textureUploads[i].a];
            if (!scratch[i - textureFrom])
                continue;
            if (!copy)
                copy = SDL_BeginGPUCopyPass(cmd);
            copy_levels(copy, scratch[i - textureFrom], a->tex, W.textureUploads[i].b, a->width, a->height, a->levels);
        }
        if (copy)
            SDL_EndGPUCopyPass(copy);
        W.streamFence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
        /* released as soon as the batch no longer needs them */
        for (i = textureFrom; i < texture; i++)
            if (scratch[i - textureFrom])
                SDL_ReleaseGPUTexture(G.dev, scratch[i - textureFrom]);
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
    free(scratch);
    W.nmeshUploads = 0;
    W.meshes.size = 0;
    W.uploadedVertices = W.vertexCount;
    W.uploadedIndices = W.indexCount;
    if (textures)
        W.ntextureUploads = 0;
    return 1;

failed:
    free(scratch);
    /* the textures that went up let their surfaces go: the list keeps those it still holds */
    memmove(W.textureUploads, W.textureUploads + released, (W.ntextureUploads - released) * sizeof *W.textureUploads);
    W.ntextureUploads -= released;
    return 0;
}

/* What goes up with this flush besides the stream, staged in G.texels: the white texel, the records,
   slots and cells that changed, the frame's matrices, waves and water instances, the shadow fit's
   state and its squares cleared with the world; and the buffers they go into. 0 without memory. */
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
        uint32_t ids = W.idCount ? W.idCount : 1;
        size_t idBytes = (size_t)W.idCount * sizeof *W.byId, cullBytes = (size_t)W.recordCount * sizeof *W.cull;
        size_t runBytes = W.nindirect * sizeof *W.indirect, casterBytes = W.ncasterRuns * sizeof *W.casterRuns;
        size_t orderBytes = W.orderHeld * sizeof *W.order, chunkBytes = W.nchunks * sizeof *W.chunks;
        unsigned char *p;
        if (!world_buffer(&W.recordBuffer, &W.recordBufferSize, ids * (uint32_t)sizeof *W.byId, 0,
                          SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
            || !world_buffer(&W.cullData, &W.cullDataSize, (W.cullLayout.words ? W.cullLayout.words : 1) * 4, 0,
                             SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ)
            || !world_buffer(&W.draws, &W.drawsSize, (W.drawLayout.words ? W.drawLayout.words : 1) * 4, 0,
                             SDL_GPU_BUFFERUSAGE_INDIRECT | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ
                                 | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE)
            || !world_buffer(&W.drawn, &W.drawnSize, (W.sectionIndices ? W.sectionIndices : 1) * 4 * WL_SECTIONS, 0,
                             SDL_GPU_BUFFERUSAGE_INDEX | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE))
            return 0;
        at = bytes_add(&G.texels, idBytes + (size_t)W.cullLayout.words * 4, 16);
        if (at < 0)
            return 0;
        p = G.texels.data + at;
        memcpy(p, W.byId, idBytes);
        p += idBytes;
        memcpy(p, W.cull, cullBytes);
        memcpy(p + (size_t)W.cullLayout.runs * 4, W.indirect, runBytes);
        memcpy(p + (size_t)W.cullLayout.casterRuns * 4, W.casterRuns, casterBytes);
        memcpy(p + (size_t)W.cullLayout.order * 4, W.order, orderBytes);
        memcpy(p + (size_t)W.cullLayout.chunks * 4, W.chunks, chunkBytes);
        W.recordsAt = (uint32_t)at;
    }
    if (W.slotsChanged) {
        uint32_t n = W.slotCount ? W.slotCount : 1;
        if (!world_buffer(&W.slotBuffer, &W.slotBufferSize, n * (uint32_t)sizeof *W.slots, 0,
                          SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ)
            || !world_buffer(&W.views, &W.viewsSize, n * SLOT_VIEWS, 0,
                             SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
            || !world_buffer(&W.placed, &W.placedSize, n * SLOT_PLACE, 0,
                             SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ
                                 | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ))
            return 0;
        at = bytes_add(&G.texels, (size_t)W.slotCount * sizeof *W.slots, 16);
        if (at < 0)
            return 0;
        memcpy(G.texels.data + at, W.slots, (size_t)W.slotCount * sizeof *W.slots);
        W.slotsAt = (uint32_t)at;
    }
    if (W.cellsChanged) {
        if (!world_buffer(&W.cellBuffer, &W.cellBufferSize, W.cellCount * (uint32_t)sizeof *W.cells + sizeof W.sines,
                          0, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ))
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
    /* the fit's state as a frame with shadows begins it (render/shadow_fit_gpu.h), then the
       history of no frame before, where it is cleared; the state stays as the fit left it until
       the next such frame (gfx_read_shadow_fit) */
    W.fitStateDue = W.fitState && ((W.shadowSet && W.computeDue) || W.historyDue);
    if (W.fitStateDue) {
        uint32_t *s;
        at = bytes_add(&G.texels, (SHADOW_FIT_GPU_STATE + SHADOW_FIT_GPU_HISTORY) * 4, 16);
        if (at < 0)
            return 0;
        s = (uint32_t *)(G.texels.data + at);
        shadow_fit_gpu_state(s);
        memset(s + SHADOW_FIT_GPU_STATE, 0, SHADOW_FIT_GPU_HISTORY * 4);
        W.fitStateAt = (uint32_t)at;
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
        upload_buffer(copy, at, W.recordBuffer, 0, ids, 1);
        upload_buffer(copy, at + ids, W.cullData, 0, W.cullLayout.words * 4, 1);
    }
    if (W.slotsChanged)
        upload_buffer(copy, base + W.slotsAt, W.slotBuffer, 0, W.slotCount * (uint32_t)sizeof *W.slots, 1);
    if (W.cellsChanged)
        upload_buffer(copy, base + W.cellsAt, W.cellBuffer, 0,
                      W.cellCount * (uint32_t)sizeof *W.cells + (uint32_t)sizeof W.sines, 1);
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
    if (W.fitStateDue) {
        if (W.shadowSet && W.computeDue)
            upload_buffer(copy, base + W.fitStateAt, W.fitState, 0, SHADOW_FIT_GPU_STATE * 4, 1);
        /* the history is read and written by the fit itself from frame to frame: cleared in place,
           never cycled away */
        if (W.historyDue)
            upload_buffer(copy, base + W.fitStateAt + SHADOW_FIT_GPU_STATE * 4, W.fitHistory, 0,
                          SHADOW_FIT_GPU_HISTORY * 4, 0);
    }
}

static void world_shadows(SDL_GPUCommandBuffer *cmd);
static int flush_only(void);

/* The frame's compute passes: the slots' views; the view's cull of the runs, which gives each run's
   kept records their places in the view's section of the frame's buffer of indices and writes the
   run's one indirect draw; and the compaction, which copies the kept records' indices to their
   places. Sets W.computed where they ran, without which nothing of the world is drawn; then, in a
   frame with shadows, those (world_shadows). */
static void world_compute(SDL_GPUCommandBuffer *cmd)
{
    struct WlTransformFrame transform;
    struct WlCullFrame cull;
    struct WlCompactFrame compact;
    uint32_t groups[2];
    float projection[16];
    SDL_GPUStorageBufferReadWriteBinding rw[2];
    SDL_GPUBuffer *read[3];
    SDL_GPUComputePass *pass;
    W.shadowFailed = 0;
    if (!world_on() || !W.computeDue || !W.frameSet || !W.slotCount || !W.recordCount || !W.transform
        || !W.cullPipeline || !W.compactPipeline || !G.screen || !W.slotBuffer || !W.animatedBuffer || !W.views
        || !W.placed || !W.cullData || !W.draws || !W.indices || !W.drawn || !W.fitResult)
        return;
    wl_projection(projection, W.camera.proj, W.camera.centre, W.camera.nearZ, W.camera.depthScale,
                  G.screen->width, G.screen->height);
    wl_transform_frame(W.camera.view, projection, W.slotCount, &transform);
    SDL_zero(rw);
    rw[0].buffer = W.views;
    rw[0].cycle = true;
    rw[1].buffer = W.placed;
    rw[1].cycle = true;
    pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 2);
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
        wl_cull_frame(&W.frustum, (uint32_t)W.nindirect, &W.cullLayout, &W.drawLayout, W.nindirect, W.recordCount,
                      W.sectionIndices, &cull, groups);
        SDL_zero(rw);
        rw[0].buffer = W.draws;
        rw[0].cycle = true;
        pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 1);
        if (!pass)
            return;
        SDL_BindGPUComputePipeline(pass, W.cullPipeline);
        read[0] = W.cullData;
        read[1] = W.placed;
        read[2] = W.fitResult;
        SDL_BindGPUComputeStorageBuffers(pass, 0, read, 3);
        SDL_PushGPUComputeUniformData(cmd, 0, &cull, sizeof cull);
        SDL_DispatchGPUCompute(pass, groups[0], groups[1], 1);
        SDL_EndGPUComputePass(pass);
        wl_compact_frame((uint32_t)W.nchunks, 0, 1, &W.cullLayout, &W.drawLayout, W.recordCount, &compact, groups);
        if (groups[0]) {
            SDL_zero(rw);
            rw[0].buffer = W.drawn;
            rw[0].cycle = true;
            pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 1);
            if (!pass)
                return;
            SDL_BindGPUComputePipeline(pass, W.compactPipeline);
            read[0] = W.cullData;
            read[1] = W.draws;
            read[2] = W.indices;
            SDL_BindGPUComputeStorageBuffers(pass, 0, read, 3);
            SDL_PushGPUComputeUniformData(cmd, 0, &compact, sizeof compact);
            SDL_DispatchGPUCompute(pass, groups[0], groups[1], 1);
            SDL_EndGPUComputePass(pass);
        }
    }
    W.computed = 1;
    if (W.shadowSet)
        world_shadows(cmd);
}

/* Whether a recorded draw of the world's may be replayed: it is of the world as it is now, and what
   it reads is there. */
static int world_drawable(const struct gpu_draw *d)
{
    if (d->generation != W.generation || !world_on())
        return 0;
    if (d->program == PROG_WATER)
        return W.cellBuffer && W.waveBuffer && W.instanceBuffer;
    if (!W.computed || !W.recordBuffer || !W.vertices || !W.ids || !W.uvBuffer || !W.views)
        return 0;
    return d->indirect ? W.drawn && W.draws : W.indices != NULL;
}

/* After a flush: what went up has gone up. */
static void world_flushed(void)
{
    W.recordsChanged = W.slotsChanged = W.cellsChanged = 0;
    W.whiteDue = 0;
    if (W.fitStateDue)
        W.historyDue = 0;
    W.fitStateDue = 0;
}

/* ---- shadows ---- */

static int shadow_supported(void)
{
    const char *env = SDL_getenv("RAGEXE_SHADOWS");
    if (env && env[0] == '0' && !env[1])
        return 0;
    return G.dev && SDL_GPUTextureSupportsFormat(G.dev, SDL_GPU_TEXTUREFORMAT_R32_FLOAT, SDL_GPU_TEXTURETYPE_2D_ARRAY,
                                                 SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER)
           && SDL_GPUTextureSupportsFormat(G.dev, SDL_GPU_TEXTUREFORMAT_R32_FLOAT, SDL_GPU_TEXTURETYPE_2D,
                                           SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ);
}

int gfx_shadows_supported(void)
{
    if (!G.dev)
        return 0;
    if (G.shadowState == 0)
        G.shadowState = shadow_supported() ? 1 : -1;
    return G.shadowState > 0;
}

/* Shadows are optional, the renderer is not: a shader, pipeline or buffer only the shadows use that
   cannot be made turns them off for the device, said once, and the ground keeps its baked
   shadows. */
static void shadows_off(const char *what)
{
    if (G.shadowState >= 0)
        SDL_Log("gfx: %s: %s; no shadows", what, SDL_GetError());
    G.shadowState = -1;
}

static void release_texture(SDL_GPUTexture **t)
{
    if (*t)
        SDL_ReleaseGPUTexture(G.dev, *t);
    *t = NULL;
}

/* What a frame's shadows need: the cascades' maps and their depth, made once, and the depth pass's
   two targets of view depth and its depth at the screen's size (with the screen's MARGIN, so that
   their texels are the screen's pixels), made again when it changes. 0 where they cannot be: the
   frame then draws no shadows. Maps that cannot be made turn the shadows off; targets that cannot
   be made at the screen's size are tried again once it changes. */
static int shadow_prepare(void)
{
    SDL_GPUTextureCreateInfo info;
    if (!gfx_shadows_supported() || !G.screen)
        return 0;
    if (!G.shadowMaps || !G.shadowDepth) {
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
            shadows_off("shadow maps");
            release_texture(&G.shadowMaps);
            release_texture(&G.shadowDepth);
            return 0;
        }
    }
    if (!G.fitFront || G.fitW != G.screen->width || G.fitH != G.screen->height) {
        if (G.fitFailedW == G.screen->width && G.fitFailedH == G.screen->height)
            return 0;
        release_texture(&G.fitFront);
        release_texture(&G.fitBack);
        release_texture(&G.fitDepth);
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ;
        info.width = (Uint32)(G.screen->width + MARGIN);
        info.height = (Uint32)(G.screen->height + MARGIN);
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        G.fitFront = SDL_CreateGPUTexture(G.dev, &info);
        G.fitBack = SDL_CreateGPUTexture(G.dev, &info);
        info.format = G.depthFormat;
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        G.fitDepth = SDL_CreateGPUTexture(G.dev, &info);
        if (!G.fitFront || !G.fitBack || !G.fitDepth) {
            SDL_Log("gfx: the shadow fit's depth targets at %dx%d: %s; no shadows at this size", G.screen->width,
                    G.screen->height, SDL_GetError());
            release_texture(&G.fitFront);
            release_texture(&G.fitBack);
            release_texture(&G.fitDepth);
            G.fitW = G.fitH = 0;
            G.fitFailedW = G.screen->width;
            G.fitFailedH = G.screen->height;
            return 0;
        }
        G.fitW = G.screen->width;
        G.fitH = G.screen->height;
        G.fitFailedW = G.fitFailedH = 0;
    }
    return 1;
}

/* The depth pass (world_depth.hlsl): the view's runs the cull kept, every pass's, into the fit's two
   targets of view depth, the opaque passes' and the ground's first. Single-sampled, so that the
   compute passes can read what it draws, and at the main pass's viewport, so that a texel is a
   pixel of the screen. 0 where it could not be recorded. */
static int depth_pass(SDL_GPUCommandBuffer *cmd)
{
    SDL_GPUColorTargetInfo colour[2];
    SDL_GPUDepthStencilTargetInfo depth;
    SDL_GPURenderPass *pass;
    SDL_GPUBufferBinding binding[2];
    SDL_GPUBuffer *read[3];
    SDL_GPUGraphicsPipeline *bound = NULL;
    SDL_GPUViewport viewport;
    SDL_Rect scissor;
    size_t i;
    SDL_zero(colour);
    SDL_zero(depth);
    for (i = 0; i < 2; i++) {
        colour[i].texture = i ? G.fitBack : G.fitFront;
        colour[i].load_op = SDL_GPU_LOADOP_CLEAR;
        colour[i].store_op = SDL_GPU_STOREOP_STORE;
    }
    depth.texture = G.fitDepth;
    depth.clear_depth = 1.0f;
    depth.load_op = SDL_GPU_LOADOP_CLEAR;
    depth.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    pass = SDL_BeginGPURenderPass(cmd, colour, 2, &depth);
    if (!pass)
        return 0;
    viewport.x = viewport.y = 0.5f;
    viewport.w = (float)G.fitW;
    viewport.h = (float)G.fitH;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(pass, &viewport);
    scissor.x = scissor.y = 0;
    scissor.w = G.fitW;
    scissor.h = G.fitH;
    SDL_SetGPUScissor(pass, &scissor);
    SDL_zero(binding);
    binding[0].buffer = W.vertices;
    binding[1].buffer = W.ids;
    SDL_BindGPUVertexBuffers(pass, 0, binding, 2);
    binding[0].buffer = W.drawn;
    SDL_BindGPUIndexBuffer(pass, &binding[0], SDL_GPU_INDEXELEMENTSIZE_32BIT);
    read[0] = W.recordBuffer;
    read[1] = W.views;
    read[2] = W.uvBuffer;
    SDL_BindGPUVertexStorageBuffers(pass, 0, read, 3);
    for (i = 0; i < W.nindirect; i++) {
        const struct SwDrawRun *run = &W.runs[i];
        SDL_GPUGraphicsPipeline *pipe = G.depthPipeline[run->pass <= SW_PASS_GROUND][world_cull(run->cull) - CULL_NONE];
        SDL_GPUTextureSamplerBinding texture;
        int wrap = run->pass == SW_PASS_OPAQUE_WRAP || run->pass == SW_PASS_ALPHA_WRAP;
        if (pipe != bound) {
            SDL_BindGPUGraphicsPipeline(pass, pipe);
            bound = pipe;
        }
        texture.texture = world_array(run->textureArray);
        texture.sampler = G.samplers[wrap][DRAW_FILTER_MIPMAPS];
        SDL_BindGPUFragmentSamplers(pass, 0, &texture, 1);
        SDL_DrawGPUIndexedPrimitivesIndirect(pass, W.draws, wl_view_command((uint32_t)i) * INDIRECT_SIZE, 1);
        g_perf.draws++;
        g_perf.indirectDraws++;
    }
    SDL_EndGPURenderPass(pass);
    return 1;
}

/* The fit (shadow_range, shadow_extents and shadow_cascades.hlsl) of the depth pass's targets into
   W.fitResult, from and into the squares and the range of the frames before. 0 where it could not
   be recorded. */
static int fit_passes(SDL_GPUCommandBuffer *cmd, const struct ShadowFitGpuFrame *u)
{
    const struct GpuComputeCode *code[2] = { &shadow_range_cs, &shadow_extents_cs };
    SDL_GPUComputePipeline *pipes[2] = { W.fitRange, W.fitExtents };
    SDL_GPUStorageBufferReadWriteBinding rw[2];
    SDL_GPUTexture *depths[2] = { G.fitFront, G.fitBack };
    SDL_GPUComputePass *pass;
    int p;
    for (p = 0; p < 2; p++) {
        SDL_zero(rw);
        rw[0].buffer = W.fitState;
        pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 1);
        if (!pass)
            return 0;
        SDL_BindGPUComputePipeline(pass, pipes[p]);
        SDL_BindGPUComputeStorageTextures(pass, 0, depths, 2);
        /* the extents cut the range into slices as the range kept from the frames before says */
        if (p == 1)
            SDL_BindGPUComputeStorageBuffers(pass, 0, &W.fitHistory, 1);
        SDL_PushGPUComputeUniformData(cmd, 0, u, sizeof *u);
        SDL_DispatchGPUCompute(pass, (u->size[0] + code[p]->threads[0] - 1) / code[p]->threads[0],
                               (u->size[1] + code[p]->threads[1] - 1) / code[p]->threads[1], 1);
        SDL_EndGPUComputePass(pass);
    }
    SDL_zero(rw);
    rw[0].buffer = W.fitHistory;
    rw[1].buffer = W.fitResult;
    rw[1].cycle = true;
    pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 2);
    if (!pass)
        return 0;
    SDL_BindGPUComputePipeline(pass, W.fitCascades);
    SDL_BindGPUComputeStorageBuffers(pass, 0, &W.fitState, 1);
    SDL_PushGPUComputeUniformData(cmd, 0, u, sizeof *u);
    SDL_DispatchGPUCompute(pass, 1, 1, 1);
    SDL_EndGPUComputePass(pass);
    return 1;
}

/* Each cascade's casters: the cull of the casters' runs against the boxes the fit left, and their
   indices compacted into the cascades' sections, after the view's. 0 where it could not be
   recorded. */
static int caster_cull(SDL_GPUCommandBuffer *cmd)
{
    struct WlCullFrame cull;
    struct WlCompactFrame compact;
    SDL_GPUStorageBufferReadWriteBinding rw;
    SDL_GPUBuffer *read[3];
    SDL_GPUComputePass *pass;
    uint32_t groups[2];
    wl_cull_frame(NULL, (uint32_t)W.ncasterRuns, &W.cullLayout, &W.drawLayout, W.nindirect, W.recordCount,
                  W.sectionIndices, &cull, groups);
    SDL_zero(rw);
    rw.buffer = W.draws;
    pass = SDL_BeginGPUComputePass(cmd, NULL, 0, &rw, 1);
    if (!pass)
        return 0;
    SDL_BindGPUComputePipeline(pass, W.cullPipeline);
    read[0] = W.cullData;
    read[1] = W.placed;
    read[2] = W.fitResult;
    SDL_BindGPUComputeStorageBuffers(pass, 0, read, 3);
    SDL_PushGPUComputeUniformData(cmd, 0, &cull, sizeof cull);
    SDL_DispatchGPUCompute(pass, groups[0], groups[1], 1);
    SDL_EndGPUComputePass(pass);
    wl_compact_frame((uint32_t)W.nchunks, 1, WL_CASCADES, &W.cullLayout, &W.drawLayout, W.recordCount, &compact,
                     groups);
    if (!groups[0])
        return 1;
    SDL_zero(rw);
    rw.buffer = W.drawn;
    pass = SDL_BeginGPUComputePass(cmd, NULL, 0, &rw, 1);
    if (!pass)
        return 0;
    SDL_BindGPUComputePipeline(pass, W.compactPipeline);
    read[0] = W.cullData;
    read[1] = W.draws;
    read[2] = W.indices;
    SDL_BindGPUComputeStorageBuffers(pass, 0, read, 3);
    SDL_PushGPUComputeUniformData(cmd, 0, &compact, sizeof compact);
    SDL_DispatchGPUCompute(pass, groups[0], groups[1], 1);
    SDL_EndGPUComputePass(pass);
    return 1;
}

/* The shadow pass (world_caster.hlsl): each cascade's casters into its layer of the maps, a draw a
   texture array. The maps are a texel wider and taller than GFX_SHADOW_SIZE (MARGIN), drawn at the
   same half-pixel viewport as every pass. 0 where it could not be recorded. */
static int caster_pass(SDL_GPUCommandBuffer *cmd)
{
    SDL_GPUBufferBinding binding[2];
    SDL_GPUBuffer *read[4];
    SDL_GPUViewport viewport;
    SDL_Rect scissor;
    struct draw_state caster;
    int c;
    /* the casters read their texture as the opaque lists do */
    resolve_state(&caster, DRAW_OPAQUE_CLAMP, 0, 0, 0, 0);
    viewport.x = viewport.y = 0.5f;
    viewport.w = viewport.h = (float)GFX_SHADOW_SIZE;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    scissor.x = scissor.y = 0;
    scissor.w = scissor.h = GFX_SHADOW_SIZE;
    for (c = 0; c < GFX_SHADOW_CASCADES; c++) {
        SDL_GPUColorTargetInfo colour;
        SDL_GPUDepthStencilTargetInfo depth;
        SDL_GPURenderPass *pass;
        uint32_t uniform[4] = { 0, (uint32_t)c, 0, 0 };
        size_t k;
        SDL_zero(colour);
        SDL_zero(depth);
        colour.texture = G.shadowMaps;
        colour.layer_or_depth_plane = (Uint32)c;
        colour.clear_color.r = colour.clear_color.g = colour.clear_color.b = colour.clear_color.a = 1.0f;
        colour.load_op = SDL_GPU_LOADOP_CLEAR;
        colour.store_op = SDL_GPU_STOREOP_STORE;
        depth.texture = G.shadowDepth;
        depth.clear_depth = 1.0f;
        depth.load_op = SDL_GPU_LOADOP_CLEAR;
        depth.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
        pass = SDL_BeginGPURenderPass(cmd, &colour, 1, &depth);
        if (!pass)
            return 0;
        SDL_SetGPUViewport(pass, &viewport);
        SDL_SetGPUScissor(pass, &scissor);
        SDL_BindGPUGraphicsPipeline(pass, G.casterPipeline);
        SDL_zero(binding);
        binding[0].buffer = W.vertices;
        binding[1].buffer = W.ids;
        SDL_BindGPUVertexBuffers(pass, 0, binding, 2);
        binding[0].buffer = W.drawn;
        SDL_BindGPUIndexBuffer(pass, &binding[0], SDL_GPU_INDEXELEMENTSIZE_32BIT);
        read[0] = W.recordBuffer;
        read[1] = W.placed;
        read[2] = W.uvBuffer;
        read[3] = W.fitResult;
        SDL_BindGPUVertexStorageBuffers(pass, 0, read, 4);
        SDL_PushGPUVertexUniformData(cmd, 0, uniform, sizeof uniform);
        for (k = 0; k < W.ncasterRuns; k++) {
            SDL_GPUTextureSamplerBinding texture;
            texture.texture = world_array(W.casterArrays[k]);
            texture.sampler = stage_sampler(&caster, 0);
            SDL_BindGPUFragmentSamplers(pass, 0, &texture, 1);
            SDL_DrawGPUIndexedPrimitivesIndirect(
                pass, W.draws, wl_caster_command(W.nindirect, W.ncasterRuns, c, (uint32_t)k) * INDIRECT_SIZE, 1);
            g_perf.draws++;
            g_perf.indirectDraws++;
        }
        SDL_EndGPURenderPass(pass);
    }
    return 1;
}

/* The frame's shadows, before its own passes: the depth pass, the fit, the casters' cull and the
   shadow pass, each only once the one before it was recorded. The fit's result stays on the GPU,
   where every draw of the world reads it; where a step could not be recorded, the flush's draws of
   the world receive no shadows (W.shadowFailed, run_pass) and keep the ground's baked ones. */
static void world_shadows(SDL_GPUCommandBuffer *cmd)
{
    struct ShadowFitGpuFrame u;
    W.shadowFailed = 1;
    if (!shadow_prepare() || !W.uvBuffer || !W.recordBuffer || !W.vertices || !W.ids)
        return;
    shadow_fit_gpu_frame(&W.shadow.fit, G.fitW, G.fitH, W.shadow.ambient, W.shadow.darkness, &u);
    if (!depth_pass(cmd) || !fit_passes(cmd, &u) || (W.ncasterRuns && !caster_cull(cmd)) || !caster_pass(cmd))
        return;
    W.shadowFailed = 0;
    W.fitted = W.shadow;
    W.fittedSet = 1;
}

static void download_buffer(SDL_GPUCopyPass *copy, SDL_GPUBuffer *buffer, uint32_t size, SDL_GPUTransferBuffer *to,
                            uint32_t at)
{
    SDL_GPUBufferRegion src;
    SDL_GPUTransferBufferLocation dst;
    src.buffer = buffer;
    src.offset = 0;
    src.size = size;
    dst.transfer_buffer = to;
    dst.offset = at;
    SDL_DownloadFromGPUBuffer(copy, &src, &dst);
}

static void download_depths(SDL_GPUCopyPass *copy, SDL_GPUTexture *texture, int w, int h, SDL_GPUTransferBuffer *to,
                            uint32_t at)
{
    SDL_GPUTextureRegion src;
    SDL_GPUTextureTransferInfo dst;
    SDL_zero(src);
    SDL_zero(dst);
    src.texture = texture;
    src.w = (Uint32)w;
    src.h = (Uint32)h;
    src.d = 1;
    dst.transfer_buffer = to;
    dst.offset = at;
    dst.pixels_per_row = (Uint32)w;
    dst.rows_per_layer = (Uint32)h;
    SDL_DownloadFromGPUTexture(copy, &src, &dst);
}

int gfx_read_shadow_fit(struct GfxShadowFitRead *out)
{
    SDL_GPUTransferBufferCreateInfo info;
    SDL_GPUTransferBuffer *down;
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    SDL_GPUFence *fence;
    const unsigned char *p;
    struct ShadowFitGpuCascades g;
    uint32_t state[SHADOW_FIT_GPU_STATE], depthBytes;
    int depths, ok = 0;
    /* the frame's fit runs when what was recorded goes up */
    if (!G.dev || !out || !flush_only() || !W.fittedSet || !G.fitFront || !W.fitState || !W.fitResult)
        return -1;
    depths = out->front && out->back;
    if (depths && (out->width < G.fitW || out->height < G.fitH)) {
        out->width = G.fitW;
        out->height = G.fitH;
        return -1;
    }
    out->width = G.fitW;
    out->height = G.fitH;
    depthBytes = depths ? (uint32_t)G.fitW * (uint32_t)G.fitH * 4 : 0;
    SDL_zero(info);
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    info.size = depthBytes * 2 + (uint32_t)sizeof state + (uint32_t)sizeof g;
    down = SDL_CreateGPUTransferBuffer(G.dev, &info);
    cmd = down ? SDL_AcquireGPUCommandBuffer(G.dev) : NULL;
    if (!cmd) {
        if (down)
            SDL_ReleaseGPUTransferBuffer(G.dev, down);
        return -1;
    }
    copy = SDL_BeginGPUCopyPass(cmd);
    if (depths) {
        download_depths(copy, G.fitFront, G.fitW, G.fitH, down, 0);
        download_depths(copy, G.fitBack, G.fitW, G.fitH, down, depthBytes);
    }
    download_buffer(copy, W.fitState, (uint32_t)sizeof state, down, depthBytes * 2);
    download_buffer(copy, W.fitResult, (uint32_t)sizeof g, down, depthBytes * 2 + (uint32_t)sizeof state);
    SDL_EndGPUCopyPass(copy);
    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    if (fence && SDL_WaitForGPUFences(G.dev, true, &fence, 1) && (p = SDL_MapGPUTransferBuffer(G.dev, down, false))) {
        if (depths) {
            memcpy(out->front, p, depthBytes);
            memcpy(out->back, p + depthBytes, depthBytes);
        }
        memcpy(state, p + depthBytes * 2, sizeof state);
        memcpy(&g, p + depthBytes * 2 + sizeof state, sizeof g);
        SDL_UnmapGPUTransferBuffer(G.dev, down);
        ok = 1;
    }
    if (fence)
        SDL_ReleaseGPUFence(G.dev, fence);
    SDL_ReleaseGPUTransferBuffer(G.dev, down);
    if (!ok)
        return -1;
    out->frame = W.fitted.fit;
    shadow_fit_gpu_read(state, &g, &out->fit);
    return 0;
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

/* The vertices and indices of the Granny models whose buffers the frame fills, each where
   skin_ready placed it, into the models' part of the mapped transfer buffer */
static void skin_stage(unsigned char *at)
{
    size_t i;
    for (i = 0; i < G.nskinUploads; i++) {
        const struct gfx_skin *k = G.skinUploads[i].model;
        size_t vbytes = (size_t)k->vertexCount * sizeof *k->vertices;
        memcpy(at + G.skinUploads[i].at, k->vertices, vbytes);
        memcpy(at + G.skinUploads[i].at + vbytes, k->indices, (size_t)k->indexCount * sizeof *k->indices);
    }
}

/* The Granny models whose buffers the frame fills, from their vertices and indices put into the
   transfer buffer after `base` (skin_stage). */
static void skin_copy(SDL_GPUCopyPass *copy, uint32_t base)
{
    size_t i;
    for (i = 0; i < G.nskinUploads; i++) {
        const struct gfx_skin *k = G.skinUploads[i].model;
        uint32_t vbytes = k->vertexCount * (uint32_t)sizeof *k->vertices;
        SDL_GPUTransferBufferLocation src;
        SDL_GPUBufferRegion dst;
        src.transfer_buffer = G.upload;
        src.offset = base + G.skinUploads[i].at;
        dst.buffer = k->vertexBuffer;
        dst.offset = 0;
        dst.size = vbytes;
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
        src.offset += vbytes;
        dst.buffer = k->indexBuffer;
        dst.size = k->indexCount * (uint32_t)sizeof *k->indices;
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    }
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
        if (u->fromLayer) {
            SDL_GPUTextureLocation from, to;
            SDL_zero(from);
            SDL_zero(to);
            from.texture = u->array->tex;
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

/* A layer of an array with mipmaps takes its levels as a texture of its own makes them, on every
   backend: its top level goes into a scratch texture of its size and levels (the layer's own, with
   the texels written over it: layer_mipmaps), SDL makes that one's levels
   (SDL_GenerateMipmapsForGPUTexture), and every level is copied into the layer. The sprite
   atlas's pages and the static world's textures alike; blits into the layer level by level made
   other mipmaps than a texture's own on Mesa's lavapipe. */
static void copy_levels(SDL_GPUCopyPass *copy, SDL_GPUTexture *scratch, SDL_GPUTexture *array, uint32_t layer,
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
static void make_mipmaps(SDL_GPUCommandBuffer *cmd, size_t first, size_t count)
{
    SDL_GPUCopyPass *copy = NULL;
    size_t i;
    for (i = first; i < first + count; i++)
        if (G.uploads[i].mips)
            SDL_GenerateMipmapsForGPUTexture(cmd, G.uploads[i].tex);
    for (i = first; i < first + count; i++) {
        const struct gpu_upload *u = &G.uploads[i];
        if (!u->scratch || !u->mips)
            continue;
        if (!copy && !(copy = SDL_BeginGPUCopyPass(cmd)))
            return;
        copy_levels(copy, u->tex, u->array->tex, u->layer, (uint32_t)u->array->width, (uint32_t)u->array->height,
                    (uint32_t)u->array->levels);
    }
    if (copy)
        SDL_EndGPUCopyPass(copy);
}

/* The storage buffers the programs read (src/render/shaders), in the order they declare them, at
   most STORAGE_BUFFERS_MAX a stage (make_shader), and their vertex and index buffers: the quads'
   the draw records and the frame's quads, a quad's corners and, per instance, where its record
   starts among the quads, and the corners' two triangles; Granny's models the same two buffers,
   and per instance where its pose's header lies among the quads, the model's vertices and indices
   bound by run_pass by the draw; the world's records the records by id, the slots' views,
   the shadow fit's result and the texture-coordinate matrices, and the shared buffers, whose index
   buffer run_pass binds by the draw (the shared indices, or the frame's compacted ones); the water
   the draw records, its cells with the sines and its waves, and its cells' instances;
   the wobble the records and its rows, and the records' instances. (The depth pass and the shadow
   pass bind their own: depth_pass, caster_pass.) */
static void bind_program(SDL_GPURenderPass *pass, int program)
{
    SDL_GPUBuffer *vertex[4], *fragment[1];
    SDL_GPUBufferBinding binding[2];
    Uint32 nv = 0;
    fragment[0] = G.records;
    SDL_zero(binding);
    switch (program) {
    case PROG_QUAD:
        vertex[nv++] = G.records;
        vertex[nv++] = G.quads;
        binding[0].buffer = G.corners;
        binding[1].buffer = G.offsets;
        SDL_BindGPUVertexBuffers(pass, 0, binding, 2);
        binding[0].buffer = G.triangles;
        SDL_BindGPUIndexBuffer(pass, &binding[0], SDL_GPU_INDEXELEMENTSIZE_16BIT);
        break;
    case PROG_WORLD:
        vertex[nv++] = W.recordBuffer;
        vertex[nv++] = W.views;
        vertex[nv++] = W.fitResult;
        vertex[nv++] = W.uvBuffer;
        fragment[0] = W.fitResult;
        binding[0].buffer = W.vertices;
        binding[1].buffer = W.ids;
        SDL_BindGPUVertexBuffers(pass, 0, binding, 2);
        break;
    case PROG_WATER:
        vertex[nv++] = G.records;
        vertex[nv++] = W.cellBuffer;
        vertex[nv++] = W.waveBuffer;
        binding[0].buffer = W.instanceBuffer;
        SDL_BindGPUVertexBuffers(pass, 0, binding, 1);
        break;
    case PROG_SKIN:
        vertex[nv++] = G.records;
        vertex[nv++] = G.quads;
        break;
    default:
        vertex[nv++] = G.records;
        fragment[0] = G.rows;
        binding[0].buffer = G.instances;
        SDL_BindGPUVertexBuffers(pass, 0, binding, 1);
        break;
    }
    SDL_BindGPUVertexStorageBuffers(pass, 0, vertex, nv);
    SDL_BindGPUFragmentStorageBuffers(pass, 0, fragment, 1);
}

/* The uniform a world program's draw takes: its draw record itself; the water's its record, the
   frame's waves and cells, and the frame's view, which its quads are tested in, and that view to
   clip space (its draw record's view). */
static void push_uniforms(SDL_GPUCommandBuffer *cmd, const struct gpu_draw *d)
{
    const struct GpuDraw *r = (const struct GpuDraw *)G.drawRecords.data + d->uniform[0];
    if (d->program == PROG_WATER) {
        struct GpuWaterFrame u;
        const struct GpuView *v = (const struct GpuView *)G.viewRecords.data + r->mode[3];
        memset(&u, 0, sizeof u);
        u.frame[0] = d->uniform[0];
        u.frame[1] = d->uniform[1];
        u.frame[2] = d->uniform[2];
        memcpy(u.view, W.camera.view, sizeof u.view);
        u.screen[0] = W.camera.proj[0];
        u.screen[1] = W.camera.proj[1];
        u.screen[2] = W.camera.centre[0];
        u.screen[3] = W.camera.centre[1];
        memcpy(u.toClip, v->toClip, sizeof u.toClip);
        SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof u);
        return;
    }
    if (W.shadowFailed && r->mesh[3]) {
        /* the frame's shadows were not all drawn: the draw receives none (world.hlsl, receiving) */
        struct GpuDraw unshadowed = *r;
        unshadowed.mesh[3] = 0;
        SDL_PushGPUVertexUniformData(cmd, 0, &unshadowed, sizeof unshadowed);
        SDL_PushGPUFragmentUniformData(cmd, 0, &unshadowed, sizeof unshadowed);
        return;
    }
    SDL_PushGPUVertexUniformData(cmd, 0, r, sizeof *r);
    SDL_PushGPUFragmentUniformData(cmd, 0, r, sizeof *r);
}

static void run_pass(SDL_GPUCommandBuffer *cmd, const struct gpu_step *s)
{
    SDL_GPUColorTargetInfo colour;
    SDL_GPUDepthStencilTargetInfo depth;
    SDL_GPURenderPass *pass;
    SDL_GPUBufferBinding binding;
    SDL_GPUGraphicsPipeline *bound = NULL;
    int program = -1, ms = s->target == G.screen && G.msaa, hasDepth = pass_has_depth(s);
    SDL_GPUBuffer *indices = NULL;
    const struct gfx_skin *skin = NULL;
    uint32_t pushed[4];
    int pushedAny = 0;
    SDL_GPUTextureSamplerBinding textures[3];
    SDL_Rect scissor;
    int w = s->target->width, h = s->target->height;
    SDL_GPUViewport viewport;
    uint32_t i;
    int j;
    SDL_zero(colour);
    SDL_zero(depth);
    SDL_zero(textures);
    colour.texture = ms ? G.msaa : s->target->tex;
    if (ms && !s->unresolve) {
        colour.resolve_texture = s->target->tex;
        colour.store_op = SDL_GPU_STOREOP_RESOLVE_AND_STORE;
    } else {
        colour.store_op = SDL_GPU_STOREOP_STORE;
    }
    if (!colour.texture)
        return;
    colour.load_op = s->loadColour;
    colour.clear_color = s->clearColour;
    if (hasDepth) {
        depth.texture = G.depth;
        depth.load_op = s->loadDepth;
        depth.clear_depth = s->clearDepth;
        depth.store_op = SDL_GPU_STOREOP_STORE;
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
        int world = d->program == PROG_WORLD || d->program == PROG_WATER;  /* of gfx_world_* */
        int nbind = d->program == PROG_ROWS ? 1 : 3;
        SDL_GPUTextureSamplerBinding bind[3];
        if (!d->pipeline || (world && !world_drawable(d)))
            continue;
        memcpy(bind, d->bind, sizeof bind);
        if (d->program == PROG_WORLD) {
            for (j = 0; j < 2; j++)
                bind[j].texture = world_array(d->arrays[j]);
            /* the shadow passes not all recorded: no maps, and no shadows (push_uniforms) */
            if (W.shadowFailed)
                bind[2].texture = G.noShadow;
        }
        if (d->pipeline != bound) {
            SDL_BindGPUGraphicsPipeline(pass, d->pipeline);
            bound = d->pipeline;
        }
        if (d->program != program) {
            bind_program(pass, d->program);
            program = d->program;
            memset(textures, 0, sizeof textures);
            pushedAny = 0;
            indices = NULL;
            skin = NULL;
        }
        if (d->program == PROG_SKIN && d->skin != skin) {
            SDL_GPUBufferBinding vertices[2];
            SDL_zero(vertices);
            vertices[0].buffer = d->skin->vertexBuffer;
            vertices[1].buffer = G.offsets;
            SDL_BindGPUVertexBuffers(pass, 0, vertices, 2);
            binding.buffer = d->skin->indexBuffer;
            binding.offset = 0;
            SDL_BindGPUIndexBuffer(pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            skin = d->skin;
        }
        if (d->program == PROG_WORLD) {
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
        if (world && (!pushedAny || memcmp(pushed, d->uniform, sizeof pushed))) {
            push_uniforms(cmd, d);
            memcpy(pushed, d->uniform, sizeof pushed);
            pushedAny = 1;
        }
        if (d->program == PROG_QUAD) {
            SDL_DrawGPUIndexedPrimitives(pass, 6, d->count, 0, 0, d->record);
        } else if (d->program == PROG_ROWS) {
            /* no vertices: the triangle comes from the vertex index */
            SDL_DrawGPUPrimitives(pass, d->count, 1, 0, d->record);
        } else if (d->program == PROG_WATER) {
            SDL_DrawGPUPrimitives(pass, 6, d->instances, 0, d->firstInstance);
        } else if (d->program == PROG_SKIN) {
            SDL_DrawGPUIndexedPrimitives(pass, d->count, d->instances, d->firstIndex, 0, d->firstInstance);
        } else if (d->indirect) {
            SDL_DrawGPUIndexedPrimitivesIndirect(pass, W.draws, d->firstIndex * INDIRECT_SIZE, 1);
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
    struct gfx_skin *k, *next;
    size_t i;
    for (k = G.skins; k; k = next) {
        next = k->next;
        if (k->dead)
            skin_destroy(k);
    }
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
    G.drawRecords.size = G.viewRecords.size = G.rowTables.size = 0;
    G.texels.size = G.quadData.size = G.quadOffsets.size = 0;
    memset(G.clearBlocks, 0, sizeof G.clearBlocks);
    G.nskinUploads = G.skinBytes = 0;
    G.batch.open = G.skinBatch.open = 0;
    G.cameraView = G.matrixIndex[0] = G.matrixIndex[1] = -1;
    W.instances.size = 0;
    W.computeDue = W.frameDue = W.computed = 0;
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

/* A flush whose command buffer did not reach the GPU: the textures it was to fill go up with the
   next one, and so do the world's records, slots and cells, which stay changed (world_flushed), and
   the Granny models it was to fill, whose buffers are made again. */
static void requeue_uploads(long instances)
{
    size_t i;
    for (i = 0; i < G.nskinUploads; i++)
        skin_buffers_release(G.skinUploads[i].model);
    for (i = 0; i < G.nuploads; i++) {
        const struct gpu_upload *u = &G.uploads[i];
        struct gfx_surface *s = u->surface;
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
    uint32_t oRecords, oRows, oQuads, oOffsets, oTexels, oSkins, total;
    long instances;
    unsigned char *mapped;
    size_t i;
    int ok = 1, downloaded = 0, shrink = 0;
    batch_flush();
    if (!G.dev)
        return 0;
    if (!G.nsteps && !G.nuploads && !G.nskinUploads && !W.nmeshUploads && !W.ntextureUploads && !extra->present
        && !extra->download) {
        frame_reset();
        return 1;
    }
    /* The frame FRAMES_IN_FLIGHT submissions ago is waited for before anything here takes a command
       buffer. SDL's Vulkan backend gives each command buffer a cache of descriptor sets, and takes
       it back only once it finds that command buffer finished, at a wait or a submit that presents.
       A command buffer that finds no cache given back starts an empty one, which makes a
       descriptor pool for every kind of binding the frame binds (28 with shadows), and the driver
       can take 15 ms and more over those. Every texture made between frames holds a cache for a
       while, as SDL_CreateGPUTexture sets the texture's layout in a command buffer of its own;
       waiting first gives the frame the cache of the frame it waited for. */
    wait_frames(0);
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
    if (!G.quadData.size)
        bytes_add(&G.quadData, 16, 16);
    if (!G.quadOffsets.size)
        bytes_add(&G.quadOffsets, 4, 4);
    instances = instances_needed((uint32_t)(G.drawRecords.size / sizeof(struct GpuDraw)));
    oRecords = 0;
    oRows = align16(oRecords + (uint32_t)G.drawRecords.size);
    oQuads = align16(oRows + (uint32_t)G.rowTables.size);
    oOffsets = align16(oQuads + (uint32_t)G.quadData.size);
    oTexels = align16(oOffsets + (uint32_t)G.quadOffsets.size);
    oSkins = align16(oTexels + (uint32_t)G.texels.size);
    total = oSkins + G.skinBytes;
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
        || (G.rowTables.size
            && !ensure_buffer(&G.rows, &G.rowsSize, (uint32_t)G.rowTables.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ))
        || !ensure_buffer(&G.quads, &G.quadsSize, (uint32_t)G.quadData.size, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ)
        || !ensure_buffer(&G.offsets, &G.offsetsSize, (uint32_t)G.quadOffsets.size, SDL_GPU_BUFFERUSAGE_VERTEX)) {
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
    if (G.rowTables.size)
        memcpy(mapped + oRows, G.rowTables.data, G.rowTables.size);
    memcpy(mapped + oQuads, G.quadData.data, G.quadData.size);
    memcpy(mapped + oOffsets, G.quadOffsets.data, G.quadOffsets.size);
    if (G.texels.size)
        memcpy(mapped + oTexels, G.texels.data, G.texels.size);
    skin_stage(mapped + oSkins);
    SDL_UnmapGPUTransferBuffer(G.dev, G.upload);

    cmd = SDL_AcquireGPUCommandBuffer(G.dev);
    if (!cmd) {
        SDL_Log("gfx: SDL_AcquireGPUCommandBuffer: %s", SDL_GetError());
        goto failed;
    }
    copy = SDL_BeginGPUCopyPass(cmd);
    copy_into(copy, oRecords, G.records, (uint32_t)G.drawRecords.size);
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
    world_copy(copy, oTexels);
    skin_copy(copy, oSkins);
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
        W.frameSet = W.shadowSet = 0;
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

/* A mipmap level of a layer of a texture, w by h texels of B8G8R8A8, downloaded (render/gfx.h,
   gfx_read_texture_level); the callers have submitted everything recorded before it, which may
   make the texture they read. */
static int read_level(SDL_GPUTexture *tex, uint32_t layer, uint32_t levels, int level, uint32_t w, uint32_t h,
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
    cmd = down ? SDL_AcquireGPUCommandBuffer(G.dev) : NULL;
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
    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    if (fence && SDL_WaitForGPUFences(G.dev, true, &fence, 1) && (p = SDL_MapGPUTransferBuffer(G.dev, down, false))) {
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
    if (!G.dev || !s || s->kind != K_TEXTURE || !flush_only())
        return -1;
    if (s->layered)
        return s->array ? read_level(s->array->tex, (uint32_t)s->layer, (uint32_t)s->levels, level,
                                     (uint32_t)s->width, (uint32_t)s->height, out)
                        : -1;
    return read_level(s->tex, 0, (uint32_t)s->levels, level, (uint32_t)s->width, (uint32_t)s->height, out);
}

int gfx_read_world_level(uint8_t array, uint16_t layer, int level, void *out)
{
    const struct world_array *a;
    if (!G.dev || !flush_only())
        return -1;
    a = array < W.narrays ? &W.arrays[array] : NULL;
    if (!a || !a->tex || layer >= a->layers)
        return -1;
    return read_level(a->tex, layer, a->levels, level, a->width, a->height, out);
}

/* ---- the device ---- */

static int choose_vsync(void)
{
    const char *env = SDL_getenv("RAGEXE_VSYNC");
    return !(env && env[0] == '0' && !env[1]);
}

/* The storage buffers a shader stage may read, read-only and read-write together: what every
   Vulkan device takes (maxPerStageDescriptorStorageBuffers; Metal takes 31). Every shader keeps to
   it, packing what it reads into fewer buffers where it would read more (the water's cells and
   sines, the cull's records, runs and chunks): the world's, the water's and the casters' vertex
   shaders and the transform, cull and compaction passes read four, the quads', Granny's actors'
   and the depth pass's vertex shaders three. SDL tells nothing of the device's own limits, so a
   shader that reads more is not made on any device, rather than failing on the smallest alone. */
#define STORAGE_BUFFERS_MAX 4

static SDL_GPUShader *make_shader(const struct GpuShaderCode *code)
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

static SDL_GPUComputePipeline *make_compute(const struct GpuComputeCode *c)
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
    G.vsCode[PROG_WORLD] = &world_vs;
    G.fsCode[PROG_WORLD] = &world_fs;
    G.vsCode[PROG_CASTER] = &world_caster_vs;
    G.fsCode[PROG_CASTER] = &world_caster_fs;
    G.vsCode[PROG_DEPTH] = &world_depth_vs;
    G.fsCode[PROG_DEPTH] = &world_depth_fs;
    G.vsCode[PROG_WATER] = &water_vs;
    G.fsCode[PROG_WATER] = &water_fs;
    G.vsCode[PROG_ROWS] = &rows_vs;
    G.fsCode[PROG_ROWS] = &rows_fs;
    G.vsCode[PROG_SKIN] = &skin_vs;
    G.fsCode[PROG_SKIN] = &skin_fs;
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
        G.vs[i] = make_shader(G.vsCode[i]);
        G.fs[i] = make_shader(G.fsCode[i]);
        if (G.vs[i] && G.fs[i])
            continue;
        /* the casters' and the depth pass's serve the shadows alone */
        if (i == PROG_CASTER || i == PROG_DEPTH) {
            shadows_off("the shadows' shaders");
            continue;
        }
        SDL_Log("gfx: SDL_CreateGPUShader: %s", SDL_GetError());
        return 0;
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
    W.transform = make_compute(&world_transform_cs);
    W.cullPipeline = make_compute(&world_cull_cs);
    W.compactPipeline = make_compute(&world_compact_cs);
    {
        /* the shadow fit's result, which every draw of the world and the view's cull bind, shadows
           or none; then its passes, its state and its history, which only the shadows need */
        SDL_GPUBufferCreateInfo binfo;
        SDL_zero(binfo);
        binfo.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE
                      | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
        binfo.size = sizeof(struct ShadowFitGpuCascades);
        W.fitResult = SDL_CreateGPUBuffer(G.dev, &binfo);
        W.fitRange = make_compute(&shadow_range_cs);
        W.fitExtents = make_compute(&shadow_extents_cs);
        W.fitCascades = make_compute(&shadow_cascades_cs);
        if (!W.fitRange || !W.fitExtents || !W.fitCascades)
            shadows_off("the shadow fit's passes");
        binfo.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
        binfo.size = SHADOW_FIT_GPU_STATE * 4;
        W.fitState = SDL_CreateGPUBuffer(G.dev, &binfo);
        binfo.size = SHADOW_FIT_GPU_HISTORY * 4;
        W.fitHistory = SDL_CreateGPUBuffer(G.dev, &binfo);
        if (!W.fitState || !W.fitHistory)
            shadows_off("the shadow fit's buffers");
        W.historyDue = 1;
    }
    if (!G.pointClamp || !G.white || !G.noShadow || !G.noAtlas || !make_samplers() || !upload_constants()
        || !W.transform || !W.cullPipeline || !W.compactPipeline || !W.fitResult) {
        SDL_Log("gfx: device resources: %s", SDL_GetError());
        return 0;
    }
    G.frame = 1;
    G.cameraView = G.matrixIndex[0] = G.matrixIndex[1] = -1;
    W.generation++;
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
    struct gfx_skin *k;
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
        if (W.fitRange)
            SDL_ReleaseGPUComputePipeline(G.dev, W.fitRange);
        if (W.fitExtents)
            SDL_ReleaseGPUComputePipeline(G.dev, W.fitExtents);
        if (W.fitCascades)
            SDL_ReleaseGPUComputePipeline(G.dev, W.fitCascades);
        if (W.fitState)
            SDL_ReleaseGPUBuffer(G.dev, W.fitState);
        if (W.fitHistory)
            SDL_ReleaseGPUBuffer(G.dev, W.fitHistory);
        if (W.fitResult)
            SDL_ReleaseGPUBuffer(G.dev, W.fitResult);
        W.white = NULL;
        W.fittedSet = 0;
        W.transform = W.cullPipeline = W.compactPipeline = W.fitRange = W.fitExtents = W.fitCascades = NULL;
        W.fitState = W.fitHistory = W.fitResult = NULL;
        W.frameSet = W.shadowSet = 0;
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
        if (G.noAtlas) SDL_ReleaseGPUTexture(G.dev, G.noAtlas);
        if (G.noShadow) SDL_ReleaseGPUTexture(G.dev, G.noShadow);
        if (G.shadowMaps) SDL_ReleaseGPUTexture(G.dev, G.shadowMaps);
        if (G.shadowDepth) SDL_ReleaseGPUTexture(G.dev, G.shadowDepth);
        if (G.fitFront) SDL_ReleaseGPUTexture(G.dev, G.fitFront);
        if (G.fitBack) SDL_ReleaseGPUTexture(G.dev, G.fitBack);
        if (G.fitDepth) SDL_ReleaseGPUTexture(G.dev, G.fitDepth);
        if (G.depth) SDL_ReleaseGPUTexture(G.dev, G.depth);
        if (G.msaa) SDL_ReleaseGPUTexture(G.dev, G.msaa);
        if (G.records) SDL_ReleaseGPUBuffer(G.dev, G.records);
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
    memset(G.worldPipeline, 0, sizeof G.worldPipeline);
    memset(G.waterPipeline, 0, sizeof G.waterPipeline);
    memset(G.skinPipeline, 0, sizeof G.skinPipeline);
    memset(G.effectPipeline, 0, sizeof G.effectPipeline);
    memset(G.depthPipeline, 0, sizeof G.depthPipeline);
    G.casterPipeline = G.rowsPipeline = NULL;
    memset(G.samplers, 0, sizeof G.samplers);
    memset(G.vs, 0, sizeof G.vs);
    memset(G.fs, 0, sizeof G.fs);
    G.pointClamp = NULL;
    G.white = G.noAtlas = G.noShadow = G.shadowMaps = G.shadowDepth = G.depth = G.msaa = G.frameCopy = NULL;
    G.frameCopyW = G.frameCopyH = 0;
    G.fitFront = G.fitBack = G.fitDepth = NULL;
    G.fitW = G.fitH = G.fitFailedW = G.fitFailedH = 0;
    G.records = G.rows = G.quads = G.offsets = NULL;
    G.instances = NULL;
    G.corners = G.triangles = NULL;
    G.recordsSize = G.rowsSize = G.quadsSize = G.offsetsSize = 0;
    G.instanceCount = 0;
    G.upload = G.download = NULL;
    G.uploadSize = G.downloadSize = 0;
    G.shadowState = 0;
    d->dev = NULL;
    return 0;
}

/* The present mode follows RAGEXE_VSYNC (claim_window). Without vsync the frame never waits for
   the window: it is drawn but not shown unless a refresh has passed since the last picture
   (present_into). A window not claimed shows nothing; the frame is drawn and ends all the same. */
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
    if (!G.dev || !screen || !screen->tex || !G.rowsPipeline || !rows || count <= 0 || !fill
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
