/* The static world on the GPU (render/gfx_world.h): a layer of the graphics device's
   (render/gpu_layer.h), which records into the device's frames through the device's hooks and
   knows the device only by that seam.

   The static world (gfx_world_*) stays on the GPU from frame to frame: its meshes in shared vertex
   and index buffers, its textures in texture arrays, its draw records, transform slots and cells of
   water. Its meshes and textures go up through a stream of their own as they arrive, a bounded
   transfer buffer at a time (world_stream); its records, slots and cells with the frame's upload,
   staged among the frame's texels (world_prepare). After the frame's copy pass three compute passes
   compute the slots' views (world_transform.hlsl), cull the records for the view, giving each run's
   kept records their places in the frame's index buffer of them and writing the run's one indirect
   draw (world_cull.hlsl), and copy the kept records' indices to their places, 64 at a time
   (world_compact.hlsl); the main pass draws the ground and the opaque models with those draws. In a
   frame with shadows, a depth pass then draws the view's records into two targets of view depth
   (world_depth.hlsl), three compute passes fit the shadow cascades to them (shadow_range,
   shadow_extents and shadow_cascades.hlsl), the cull and the compaction run again for each
   cascade's casters, and the shadow pass draws those into the cascades' maps, all before the
   frame's own passes, which receive from the fit's buffer: the CPU never sees the cascades. The
   water is drawn among the quads, in world coordinates (water.hlsl). */
#include "render/gfx_world.h"
#include "render/gpu_layer.h"
#include "render/texel_convert.h"
#include "render/world_layout.h"
#include "render/shaders/world.h"
#include "render/shaders/world_caster.h"
#include "render/shaders/world_caster_solid.h"
#include "render/shaders/world_depth.h"
#include "render/shaders/world_transform.h"
#include "render/shaders/world_cull.h"
#include "render/shaders/world_compact.h"
#include "render/shaders/shadow_range.h"
#include "render/shaders/shadow_extents.h"
#include "render/shaders/shadow_cascades.h"
#include "render/shaders/water.h"
#include "diag/perf_counters.h"
#include "platform/clock.h"
#include <SDL3/SDL.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* the frame's water instances: a cell and its colour (water.hlsl, WaterIn) */
#define WATER_INSTANCE 8

/* The world's programs: its records, the casters whose texels the alpha test drops
   (PROG_CASTER) and the others (PROG_CASTER_SOLID, world_caster.hlsli), sharing its vertex shader,
   the depth pass, the water. */
enum { PROG_WORLD, PROG_CASTER, PROG_CASTER_SOLID, PROG_DEPTH, PROG_WATER, WORLD_PROGRAMS };

/* A view to clip space among the frame's views, which a water draw's record names (mode[3]) and its
   draw hands its vertex shader as its uniform; no shader reads the views from a buffer. */
struct GpuView {
    float toClip[16];
};

/* a run of the view's records as the depth pass draws it */
struct world_run {
    uint8_t pass, cull, textureArray;
};

/* The static world as a frame hands it to the render thread (world_hand_over): its buffers and
   arrays as they are when the frame goes, what went into the frame's texels for them and where,
   and what its compute and shadow passes are given. */
struct world_frame {
    int on, generation;
    SDL_GPUBuffer *vertices, *ids, *indices, *recordBuffer, *cullData, *draws, *drawn, *slotBuffer, *views, *placed,
        *animatedBuffer, *uvBuffer, *cellBuffer, *waveBuffer, *instanceBuffer, *fitState, *fitHistory, *fitResult;
    SDL_GPUTexture *white, *arrays[SW_NO_ARRAY];
    uint32_t narrays;
    /* staged in the frame's texels; the fit's state goes up where stateUp, its history where
       historyDue */
    int whiteDue, recordsChanged, slotsChanged, cellsChanged, frameDue, fitStateDue, stateUp, historyDue;
    uint32_t whiteAt, recordsAt, slotsAt, cellsAt, frameAt, fitStateAt;
    uint32_t idBytes, cullBytes, slotBytes, cellBytes, animatedBytes, uvBytes, waveBytes, instanceBytes;
    /* the compute passes, where `compute`; the shadows, given (shadowSet) and drawn (`shadows`) */
    int compute, shadowSet, shadows;
    struct GfxWorldView camera;
    struct SwFrustum frustum;
    struct GfxShadow shadow;
    int screenW, screenH;
    uint32_t slotCount, recordCount, sectionIndices;
    size_t nindirect, ncasterRuns, nchunks;
    struct WlCullLayout cullLayout;
    struct WlDrawLayout drawLayout;
    struct world_run *runs;  /* the view's runs, nindirect */
    size_t runCapacity;
    uint8_t *casterArrays;   /* the caster's array of each of the casters' runs, ncasterRuns */
    size_t casterArrayCapacity;
    SDL_GPUTexture *fitFront, *fitBack, *fitDepth, *shadowMaps, *shadowDepth;
    int fitW, fitH;
};

/* The world's part of a frame's packet (struct GpuLayer, partSize): kept on the CPU, the views the
   water draws with and the records the world's draws take as their uniforms; the world as the frame
   hands it over; the render thread's: whether the frame's compute passes were recorded, without
   which nothing of the world is drawn, and whether its shadows were not all, and in a pass, the
   index buffer and the uniform its draws last bound. */
struct world_part {
    struct GpuBytes views, uniforms;
    struct world_frame world;
    int computed, shadowFailed;
    SDL_GPUBuffer *indices;
    uint32_t pushed[4];
    int pushedAny;
};

/* A draw of the world's, in a struct GpuLayerDraw's words: of the world's records, one (`count`
   indices from firstIndex after baseVertex) or a run's indirect command, the firstIndex-th
   (indirect), the texture arrays bind[0] and bind[1] read (SW_NO_ARRAY: white), whose textures its
   draw takes as they are once the flush has made and grown them (world_array); of its water's cells
   (PROG_WATER, `instances` of them from firstInstance of the frame's); of the world of `generation`
   (gfx_world_generation); and what its program takes as its uniform: the world's, its draw's
   record among the frame's uniforms; the water's, its record, the waves' count and the cells', and
   its view among the frame's. */
struct world_draw {
    uint32_t uniform[4];
    uint32_t firstIndex, count;
    int32_t baseVertex;
    uint32_t instances, firstInstance;
    int32_t generation;
    uint8_t arrays[2], indirect;
};
_Static_assert(sizeof(struct world_draw) <= sizeof(((struct GpuLayerDraw *)0)->words), "a draw's words hold it");

/* custom: the static world, which stays from frame to frame: its shared buffers, texture arrays,
   records and transform slots, and what the frame gives of it. */
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
    gfx_surface *surface;
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
    int layer;  /* the device's number for the layer (gpu_add_layer) */
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
    struct GpuBytes meshes;
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
    uint8_t *casterArrays;  /* the caster's array of each of the casters' runs (wl_caster_array) */
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
       its matrices, waves and water staged, and its compute passes due */
    struct GfxWorldView camera;
    struct SwFrustum frustum;
    struct GfxShadow shadow;
    int frameSet, shadowSet, frameDue, computeDue;
    uint32_t frameAt, waveCount;
    struct GpuBytes animated, uvs, waves, instances;
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
    /* the shadows the GPU fitted last (gfx_read_shadow_fit), where fitted: by a frame from
       fittedFrom on, the first recorded since the client last gave the world's frame */
    struct GfxShadow fitted;
    int fittedSet;
    unsigned fittedFrom;

    /* the camera's world view the water is drawn through (gfx_set_world_camera), and the frame's
       view of it in the screen's pixels (view_for), -1 for none yet, for a target of that size */
    struct GfxWorldView worldCamera;
    int worldCameraSet;
    int cameraView;
    int viewsTargetW, viewsTargetH;

    /* shadows: whether the device draws them (gfx_shadows_supported; -1 for good once something
       they need could not be made), the cascades' maps and their depth, what a draw reads without
       them, and the depth pass's targets at the screen's size, or the size they could not be made
       at, which is not tried again */
    int shadowState;
    SDL_GPUTexture *shadowMaps, *shadowDepth, *noShadow;
    SDL_GPUTexture *fitFront, *fitBack, *fitDepth;
    int fitW, fitH, fitFailedW, fitFailedH;

    /* the programs' shaders (the casters share their vertex shader), and the pipelines: the
       records' by mode and cull mode (GPU_CULL_* - 1), the water's by mode, the depth pass's by
       whether it writes the opaque depth and cull mode, the shadow casters' by whether they are
       alpha-tested */
    SDL_GPUShader *vs[WORLD_PROGRAMS], *fs[WORLD_PROGRAMS];
    SDL_GPUGraphicsPipeline *worldPipeline[DRAW_MODE_COUNT][3];
    SDL_GPUGraphicsPipeline *waterPipeline[DRAW_MODE_COUNT];
    SDL_GPUGraphicsPipeline *depthPipeline[2][3];
    SDL_GPUGraphicsPipeline *casterPipeline[2];
} W = { -1 };

/* The meshes' fog in a view record: its start and end as view depths, and 1 / (end - start)
   (render/gfx.h, GfxFog), computed as the Direct3D 9 client computed them. */
static void view_fog(float *fog)
{
    const struct GfxFog *f = gpu_fog();
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
    struct world_part *part = gpu_part(W.layer);
    struct GpuView *view;
    long at = gpu_bytes_add(&part->views, sizeof *view, 16);
    if (at < 0)
        return -1;
    view = (struct GpuView *)(part->views.data + at);
    memcpy(view->toClip, toClip, sizeof view->toClip);
    return (int)(at / (long)sizeof *view);
}

/* A world view in a target's pixels among the frame's views (struct GpuView), made once per frame
   and target size; -1 without memory. The matrices are the ones the Direct3D 9 client drew with
   (build/ref/COMMIT.txt), computed in floats as it computed them: the GPU divides their points and
   moves them by the viewport as Direct3D 9 did, half a pixel on (GPU_MARGIN). */
static int view_for(const struct GfxWorldView *v, int *cache, const struct GpuStep *pass)
{
    float proj[16], clip[16];
    int w, h;
    gpu_pass_size(pass, &w, &h);
    if (w != W.viewsTargetW || h != W.viewsTargetH) {
        W.cameraView = -1;
        W.viewsTargetW = w;
        W.viewsTargetH = h;
    }
    if (*cache >= 0)
        return *cache;
    wl_projection(proj, v->proj, v->centre, v->nearZ, v->depthScale, w, h);
    wl_view_clip(clip, v->view, proj);
    *cache = add_view(clip);
    return *cache;
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

_Static_assert(GFX_SHADOW_CASCADES == WL_CASCADES, "the cull compacts every cascade's casters");
_Static_assert(GFX_SHADOW_CASCADES == SHADOW_FIT_CASCADES && GFX_SHADOW_SIZE == SHADOW_FIT_SIZE,
               "the maps are the fit's");

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
    grown = SDL_CreateGPUBuffer(gpu_dev(), &info);
    if (!grown)
        return 0;
    if (*buffer && keep) {
        struct world_copy *c;
        if (!gpu_grow((void **)&W.copies, &W.copyCapacity, W.ncopies + 1, sizeof *W.copies)) {
            SDL_ReleaseGPUBuffer(gpu_dev(), grown);
            return 0;
        }
        c = &W.copies[W.ncopies++];
        c->from = *buffer;
        c->to = grown;
        c->size = keep;
    } else {
        gpu_retire(NULL, *buffer);
    }
    *buffer = grown;
    *size = s;
    return 1;
}

int gfx_world_generation(void)
{
    return W.generation;
}

/* whether the device takes the world's parts: it is there with the world attached to it, and has
   not given the world up after uploads that failed again and again (world_failed) */
static int world_on(void)
{
    return gpu_dev() && W.layer >= 0 && !W.off;
}

static void world_release(void)
{
    size_t i;
    SDL_GPUBuffer **buffers[] = { &W.vertices, &W.ids, &W.indices, &W.recordBuffer, &W.cullData, &W.draws, &W.drawn,
                                  &W.slotBuffer, &W.views, &W.placed, &W.animatedBuffer, &W.uvBuffer, &W.cellBuffer,
                                  &W.waveBuffer, &W.instanceBuffer };
    for (i = 0; i < sizeof buffers / sizeof buffers[0]; i++)
        gpu_retire(NULL, *buffers[i]);
    for (i = 0; i < W.narrays; i++)
        gpu_retire(W.arrays[i].tex, NULL);
    for (i = 0; i < W.ncopies; i++)
        gpu_retire(NULL, W.copies[i].from);
    for (i = 0; i < W.narrayCopies; i++)
        gpu_retire(W.arrayCopies[i].from, NULL);
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
    W.computeDue = 0;
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
    at = gpu_bytes_add(&W.meshes, vbytes + (size_t)place->vertexCount * 4 + ibytes, 16);
    if (at < 0 || !gpu_grow((void **)&W.meshUploads, &W.meshUploadCapacity, W.nmeshUploads + 1, sizeof *W.meshUploads))
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

int gfx_world_array(uint8_t array, gfx_surface *like)
{
    struct world_array *a;
    struct GpuTexels t;
    if (!world_on() || !gpu_texture_texels(like, &t) || array != W.narrays || array >= SW_NO_ARRAY)
        return 0;
    a = &W.arrays[W.narrays++];
    memset(a, 0, sizeof *a);
    a->width = (uint32_t)t.width;
    a->height = (uint32_t)t.height;
    a->levels = (uint32_t)t.levels;
    return 1;
}

int gfx_world_texture(gfx_surface *texture, uint8_t array, uint16_t layer)
{
    struct world_array *a;
    struct world_upload *u;
    struct GpuTexels t;
    if (!world_on() || !gpu_texture_texels(texture, &t) || !t.pixels || array >= W.narrays
        || layer >= GFX_WORLD_LAYERS)
        return 0;
    a = &W.arrays[array];
    if (a->width != (uint32_t)t.width || a->height != (uint32_t)t.height || a->levels != (uint32_t)t.levels
        || !gpu_grow((void **)&W.textureUploads, &W.textureUploadCapacity, W.ntextureUploads + 1,
                     sizeof *W.textureUploads))
        return 0;
    /* the texture's pixels go up with the next stream, straight from the surface, which stays
       until then */
    gpu_surface_ref(texture);
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
    if (!gpu_grow((void **)&W.byId, &W.byIdCapacity, ids, sizeof *W.byId)
        || !gpu_grow((void **)&W.cull, &W.cullCapacity, count, sizeof *W.cull)
        || !gpu_grow((void **)&W.runs, &W.runCapacity, count, sizeof *W.runs)
        || !gpu_grow((void **)&W.indirect, &W.indirectCapacity, count, sizeof *W.indirect)
        || !gpu_grow((void **)&W.casterRuns, &W.casterRunCapacity, count, sizeof *W.casterRuns)
        || !gpu_grow((void **)&W.casterArrays, &W.casterArrayCapacity, count, 1)
        || !gpu_grow((void **)&W.order, &W.orderCapacity, count, sizeof *W.order))
        return 0;
    /* the chunks of every record: the view's cull and every cascade's take any of them */
    chunks = wl_chunks(records, count, NULL);
    if (chunks > 0x7fffffff || !gpu_grow((void **)&W.chunks, &W.chunkCapacity, chunks, sizeof *W.chunks))
        return 0;
    memset(W.byId, 0, ids * sizeof *W.byId);
    wl_records(records, count, W.byId, W.cull);
    W.nruns = sw_draw_runs(records, count, W.runs);
    W.nindirect = wl_runs(W.runs, W.nruns, W.indirect, &viewIndices);
    W.ncasterRuns = wl_caster_runs(records, count, W.order, &W.orderHeld, W.casterRuns, &casterIndices);
    for (i = 0; i < W.ncasterRuns; i++)
        W.casterArrays[i] = wl_caster_array(&records[W.order[W.casterRuns[i].first]]);
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
        || !gpu_grow((void **)&W.slots, &W.slotCapacity, first + count, sizeof *W.slots))
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
        || !gpu_grow((void **)&W.cells, &W.cellCapacity, count, sizeof *W.cells))
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
    W.fittedFrom = gpu_frame_number();
    if (!world_on() || !frame)
        return;
    W.camera = frame->camera;
    W.frustum = frame->frustum;
    W.animated.size = W.uvs.size = W.waves.size = 0;
    at = gpu_bytes_add(&W.animated, (frame->animatedCount ? frame->animatedCount : 1) * sizeof frame->animated[0], 16);
    if (at < 0)
        return;
    if (frame->animatedCount)
        memcpy(W.animated.data, frame->animated, frame->animatedCount * sizeof frame->animated[0]);
    at = gpu_bytes_add(&W.uvs, (frame->uvCount ? frame->uvCount : 1) * UV_MATRIX, 16);
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
    at = gpu_bytes_add(&W.waves, (frame->waveCount ? frame->waveCount : 1) * sizeof *frame->waves, 16);
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

/* the array a record's texture or lightmap lies in as the frame was handed over, white where it has
   none or its array could not be made */
static SDL_GPUTexture *world_array(const struct world_frame *wf, uint8_t array)
{
    return array < wf->narrays && wf->arrays[array] ? wf->arrays[array] : wf->white;
}

/* the pipeline cull of a record's flags */
static int world_cull(int flags)
{
    return flags & SW_CULL_NONE ? GPU_CULL_NONE : flags & SW_CULL_CCW ? GPU_CULL_CCW : GPU_CULL_CW;
}

/* Whether the world draws now: a frame of it given, into the screen, in a mode that draws meshes. */
static int world_ready(void)
{
    const struct DrawModeDesc *m = draw_mode(gpu_mode());
    return world_on() && W.frameSet && gpu_into_screen() && (m->into & DRAW_MESHES) && W.recordCount && W.slotCount;
}

/* The draw record of a draw of the world's records in the current mode, among the frame's
   uniforms, which its draws push (world_draw_hook). */
static uint32_t world_record(struct GpuStep *pass, const struct GfxWorldDraw *draw, struct GpuDraw **out)
{
    const struct DrawModeDesc *m = draw_mode(gpu_mode());
    struct world_part *part = gpu_part(W.layer);
    struct GpuDraw *r;
    uint32_t at = gpu_add_record_to(&part->uniforms, &r);
    *out = r;
    if (!r)
        return 0;
    gpu_fill_record(r, pass, 0, 0);
    /* a stage reads with a border where the record has its texture (world.hlsl) */
    r->stages[1] = (m->address[0] == DRAW_ADDRESS_BORDER ? 1u : 0u) | (m->address[1] == DRAW_ADDRESS_BORDER ? 2u : 0u);
    memcpy(r->colour, draw->colour, sizeof r->colour);
    r->mesh[0] = draw->kind == GFX_MESH_MODEL;
    r->mesh[1] = draw->kind == GFX_MESH_MODEL && draw->lightGround;
    r->mesh[3] = W.shadowSet ? 1u : 0u;
    view_fog(r->fog);
    return at;
}

/* A draw of the world's: of the world as it is now (gfx_world_generation), which a flush that
   makes the world again leaves undrawn (world_drawable). */
static struct GpuLayerDraw *world_add_draw(struct GpuStep *pass, int program, SDL_GPUGraphicsPipeline *pipe,
                                           struct world_draw *w)
{
    memset(w, 0, sizeof *w);
    w->generation = W.generation;
    return gpu_add_draw(pass, W.layer, program, pipe);
}

/* the sampler of a stage of the current mode */
static SDL_GPUSampler *mode_sampler(int stage)
{
    const struct DrawModeDesc *m = draw_mode(gpu_mode());
    return gpu_stage_sampler(m->address[stage], m->filter, stage);
}

static void world_bind(struct GpuLayerDraw *d, struct world_draw *w, uint8_t textureArray, uint8_t lightmapArray)
{
    w->arrays[0] = textureArray;
    d->bind[0].sampler = mode_sampler(0);
    w->arrays[1] = lightmapArray;
    d->bind[1].sampler = mode_sampler(1);
    d->bind[2].texture = W.shadowSet ? W.shadowMaps : W.noShadow;
    d->bind[2].sampler = gpu_point_clamp();
}

void gfx_world_draw(int pass, const struct GfxWorldDraw *draw)
{
    struct GpuStep *step;
    struct GpuDraw *r;
    uint32_t record;
    size_t i;
    g_perf.drawCalls++;
    gpu_end_batch();
    if (!draw || !world_ready() || pass < SW_PASS_OPAQUE_CLAMP || pass > SW_PASS_GROUND)
        return;
    step = gpu_pass();
    if (!step)
        return;
    record = world_record(step, draw, &r);
    if (!r)
        return;
    for (i = 0; i < W.nindirect; i++) {
        const struct SwDrawRun *run = &W.runs[i];
        struct GpuLayerDraw *d;
        struct world_draw w;
        if (run->pass != pass)
            continue;
        d = world_add_draw(step, PROG_WORLD, W.worldPipeline[gpu_mode()][world_cull(run->cull) - GPU_CULL_NONE], &w);
        if (!d)
            return;
        world_bind(d, &w, run->textureArray, run->lightmapArray);
        w.indirect = 1;
        w.firstIndex = wl_view_command((uint32_t)i);
        w.uniform[0] = record;
        memcpy(d->words, &w, sizeof w);
        g_perf.draws++;
        g_perf.indirectDraws++;
    }
    W.computeDue = 1;
    gpu_drawn();
}

void gfx_world_draw_record(const struct SwRecord *rec, const struct GfxWorldDraw *draw)
{
    struct GpuStep *step;
    struct GpuLayerDraw *d;
    struct world_draw w;
    struct GpuDraw *r;
    uint32_t record;
    g_perf.drawCalls++;
    gpu_end_batch();
    if (!rec || !draw || !world_ready() || !rec->indexCount || rec->id >= W.idCount
        || rec->firstIndex > W.indexCount || rec->indexCount > W.indexCount - rec->firstIndex)
        return;
    step = gpu_pass();
    if (!step)
        return;
    record = world_record(step, draw, &r);
    if (!r)
        return;
    d = world_add_draw(step, PROG_WORLD, W.worldPipeline[gpu_mode()][world_cull(rec->flags) - GPU_CULL_NONE], &w);
    if (!d)
        return;
    world_bind(d, &w, rec->textureArray, rec->lightmapArray);
    w.firstIndex = rec->firstIndex;
    w.count = rec->indexCount;
    w.baseVertex = (int32_t)rec->baseVertex;
    w.uniform[0] = record;
    memcpy(d->words, &w, sizeof w);
    g_perf.draws++;
    g_perf.prims += rec->indexCount / 3;
    W.computeDue = 1;
    gpu_drawn();
}

void gfx_world_draw_water(const uint32_t *cells, size_t count, DWORD colour)
{
    struct GpuStep *step;
    struct GpuLayerDraw *d;
    struct world_draw w;
    struct GpuDraw *r;
    SDL_GPUTextureSamplerBinding bind[2];
    uint32_t record;
    long at;
    size_t i;
    int view;
    g_perf.drawCalls++;
    gpu_end_batch();
    if (!world_on() || !cells || !count || !W.frameSet || !W.cellCount || !W.waveCount || !W.worldCameraSet
        || !gpu_into_screen() || !W.waterPipeline[gpu_mode()] || count > 0x7fffffff)
        return;
    gpu_bind_stage(bind, 0);
    bind[1].texture = gpu_white();
    bind[1].sampler = mode_sampler(1);
    step = gpu_pass();
    if (!step)
        return;
    /* the camera's view in the screen's pixels, which the water's draw hands it as its uniform */
    view = view_for(&W.worldCamera, &W.cameraView, step);
    if (view < 0)
        return;
    at = gpu_bytes_add(&W.instances, count * WATER_INSTANCE, WATER_INSTANCE);
    if (at < 0)
        return;
    for (i = 0; i < count; i++) {
        uint32_t *p = (uint32_t *)(W.instances.data + at + i * WATER_INSTANCE);
        p[0] = cells[i] < W.cellCount ? cells[i] : 0;
        p[1] = colour;
    }
    record = gpu_add_record_to(gpu_records(), &r);
    if (!r)
        return;
    d = world_add_draw(step, PROG_WATER, W.waterPipeline[gpu_mode()], &w);
    if (!d)
        return;
    gpu_fill_record(r, step, 1, 1);
    r->mode[3] = (uint32_t)view;
    d->bind[0] = bind[0];
    d->bind[1] = bind[1];
    /* the quads' pixels (quad.hlsli), whose stage 0 reads no layer of an atlas here */
    d->bind[2].texture = gpu_no_atlas();
    d->bind[2].sampler = bind[0].sampler;
    w.instances = (uint32_t)count;
    w.firstInstance = (uint32_t)(at / WATER_INSTANCE);
    w.uniform[0] = record;
    w.uniform[1] = W.waveCount;
    w.uniform[2] = W.cellCount;
    w.uniform[3] = (uint32_t)view;
    memcpy(d->words, &w, sizeof w);
    g_perf.draws++;
    g_perf.prims += (unsigned)count * 2;
    g_perf.vertexBytes += (unsigned)count * WATER_INSTANCE;
    gpu_drawn();
}

void gfx_set_world_camera(const struct GfxWorldView *view)
{
    gpu_end_batch();
    if (!W.worldCameraSet || memcmp(&W.worldCamera, view, sizeof *view))
        W.cameraView = -1;
    W.worldCamera = *view;
    W.worldCameraSet = 1;
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
        info.format = GPU_TARGET_FORMAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = a->width;
        info.height = a->height;
        info.layer_count_or_depth = capacity;
        info.num_levels = a->levels;
        made = gpu_make_texture(&info);
        if (made && a->tex && !gpu_grow((void **)&W.arrayCopies, &W.arrayCopyCapacity, W.narrayCopies + 1,
                                    sizeof *W.arrayCopies)) {
            SDL_ReleaseGPUTexture(gpu_dev(), made);
            made = NULL;
        }
        if (!made) {
            SDL_Log("gfx: the world's texture array %u (%ux%u, %u layers): %s", (unsigned)i, a->width, a->height,
                    capacity, SDL_GetError());
            gpu_retire(a->tex, NULL);
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
        SDL_WaitForGPUFences(gpu_dev(), true, &W.streamFence, 1);
        SDL_ReleaseGPUFence(gpu_dev(), W.streamFence);
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

/* what the copies moved over goes; the frames handed over may still read it (gpu_retire) */
static void stream_release_copies(void)
{
    size_t i;
    for (i = 0; i < W.ncopies; i++)
        gpu_retire(NULL, W.copies[i].from);
    for (i = 0; i < W.narrayCopies; i++)
        gpu_retire(W.arrayCopies[i].from, NULL);
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
   into `scratch` (a texture of its size and levels), whose levels gpu_copy_levels then takes into
   the layer. */
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
   into a scratch texture of its own first, whose levels SDL makes as a texture's
   (gpu_copy_levels). 0 where a batch could not go. */
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
            SDL_ReleaseGPUTransferBuffer(gpu_dev(), W.stream);
        SDL_zero(info);
        info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        info.size = largest;
        W.stream = SDL_CreateGPUTransferBuffer(gpu_dev(), &info);
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
        mapped = SDL_MapGPUTransferBuffer(gpu_dev(), W.stream, false);
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
            struct GpuTexels t;
            uint32_t n = stream_bytes(&W.textureUploads[texture], 1);
            int64_t start;
            if (n > W.streamSize - used)
                break;
            gpu_texture_texels(W.textureUploads[texture].surface, &t);
            start = platform_stopwatch_ns();
            texel_rows_to_bgra(t.fmt, t.pixels, (long)t.width * texel_bytes(t.fmt), t.width, t.height, mapped + used,
                               (long)t.width * 4);
            g_perf.texelsNs += platform_stopwatch_ns() - start;
            used += (n + 15u) & ~15u;
        }
        SDL_UnmapGPUTransferBuffer(gpu_dev(), W.stream);
        /* a scratch texture for each texture of the batch with mipmaps */
        if (!gpu_grow((void **)&scratch, &scratchCapacity, texture - textureFrom + 1, sizeof *scratch))
            goto failed;
        for (i = textureFrom; i < texture; i++) {
            const struct world_array *a = &W.arrays[W.textureUploads[i].a];
            scratch[i - textureFrom] =
                a->tex && a->levels > 1
                    ? gpu_new_texture((int)a->width, (int)a->height, (int)a->levels,
                                      SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET)
                    : NULL;
            made = made && (scratch[i - textureFrom] || !a->tex || a->levels == 1);
        }
        cmd = made ? gpu_commands() : NULL;
        if (!cmd) {
            for (i = textureFrom; i < texture; i++)
                if (scratch[i - textureFrom])
                    SDL_ReleaseGPUTexture(gpu_dev(), scratch[i - textureFrom]);
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
            gpu_copy_levels(copy, scratch[i - textureFrom], a->tex, W.textureUploads[i].b, a->width, a->height,
                            a->levels);
        }
        if (copy)
            SDL_EndGPUCopyPass(copy);
        gpu_submit(cmd, &W.streamFence);
        /* released as soon as the batch no longer needs them */
        for (i = textureFrom; i < texture; i++)
            if (scratch[i - textureFrom])
                SDL_ReleaseGPUTexture(gpu_dev(), scratch[i - textureFrom]);
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

/* What goes up with the frame besides the stream, staged in its texels: the white texel, the
   records, slots and cells that changed, the frame's matrices, waves and water instances, the
   shadow fit's state and its squares cleared with the world; and the buffers they go into. 0
   without memory. */
static int world_prepare(void)
{
    long at;
    if (!world_on())
        return 1;
    if (!W.white) {
        SDL_GPUTextureCreateInfo info;
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
        info.format = GPU_TARGET_FORMAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = info.height = 1;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        W.white = gpu_make_texture(&info);
        if (!W.white)
            return 0;
        W.whiteDue = 1;
    }
    if (W.whiteDue) {
        at = gpu_bytes_take(gpu_texels(), 4, 16);
        if (at < 0)
            return 0;
        memset(gpu_texels()->data + at, 0xff, 4);
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
        at = gpu_bytes_add(gpu_texels(), idBytes + (size_t)W.cullLayout.words * 4, 16);
        if (at < 0)
            return 0;
        p = gpu_texels()->data + at;
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
        at = gpu_bytes_take(gpu_texels(), (size_t)W.slotCount * sizeof *W.slots, 16);
        if (at < 0)
            return 0;
        memcpy(gpu_texels()->data + at, W.slots, (size_t)W.slotCount * sizeof *W.slots);
        W.slotsAt = (uint32_t)at;
    }
    if (W.cellsChanged) {
        if (!world_buffer(&W.cellBuffer, &W.cellBufferSize, W.cellCount * (uint32_t)sizeof *W.cells + sizeof W.sines,
                          0, SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ))
            return 0;
        at = gpu_bytes_take(gpu_texels(), (size_t)W.cellCount * sizeof *W.cells + sizeof W.sines, 16);
        if (at < 0)
            return 0;
        memcpy(gpu_texels()->data + at, W.cells, (size_t)W.cellCount * sizeof *W.cells);
        memcpy(gpu_texels()->data + at + (size_t)W.cellCount * sizeof *W.cells, W.sines, sizeof W.sines);
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
        at = gpu_bytes_take(gpu_texels(), W.animated.size + W.uvs.size + W.waves.size + W.instances.size, 16);
        if (at < 0)
            return 0;
        memcpy(gpu_texels()->data + at, W.animated.data, W.animated.size);
        memcpy(gpu_texels()->data + at + W.animated.size, W.uvs.data, W.uvs.size);
        memcpy(gpu_texels()->data + at + W.animated.size + W.uvs.size, W.waves.data, W.waves.size);
        if (W.instances.size)
            memcpy(gpu_texels()->data + at + W.animated.size + W.uvs.size + W.waves.size, W.instances.data,
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
        at = gpu_bytes_add(gpu_texels(), (SHADOW_FIT_GPU_STATE + SHADOW_FIT_GPU_HISTORY) * 4, 16);
        if (at < 0)
            return 0;
        s = (uint32_t *)(gpu_texels()->data + at);
        shadow_fit_gpu_state(s);
        memset(s + SHADOW_FIT_GPU_STATE, 0, SHADOW_FIT_GPU_HISTORY * 4);
        W.fitStateAt = (uint32_t)at;
    }
    return 1;
}

/* The world as the frame hands it to the render thread: its buffers and arrays as they are now,
   what world_prepare staged and where, whether its compute passes and shadows are recorded, and
   what those read of it. Where they are, the shadow passes' draws are counted here, in the frame
   they belong to (diag/perf.h). 0 without memory for the runs. */
static int world_hand_over(struct world_part *part)
{
    struct world_frame *wf = &part->world;
    int screenW = 0, screenH = 0, screen = gpu_screen_size(&screenW, &screenH);
    size_t i;
    wf->on = world_on();
    wf->generation = W.generation;
    wf->vertices = W.vertices;
    wf->ids = W.ids;
    wf->indices = W.indices;
    wf->recordBuffer = W.recordBuffer;
    wf->cullData = W.cullData;
    wf->draws = W.draws;
    wf->drawn = W.drawn;
    wf->slotBuffer = W.slotBuffer;
    wf->views = W.views;
    wf->placed = W.placed;
    wf->animatedBuffer = W.animatedBuffer;
    wf->uvBuffer = W.uvBuffer;
    wf->cellBuffer = W.cellBuffer;
    wf->waveBuffer = W.waveBuffer;
    wf->instanceBuffer = W.instanceBuffer;
    wf->fitState = W.fitState;
    wf->fitHistory = W.fitHistory;
    wf->fitResult = W.fitResult;
    wf->white = W.white;
    wf->narrays = (uint32_t)W.narrays;
    for (i = 0; i < W.narrays; i++)
        wf->arrays[i] = W.arrays[i].tex;
    wf->whiteDue = wf->on && W.whiteDue;
    wf->recordsChanged = wf->on && W.recordsChanged;
    wf->slotsChanged = wf->on && W.slotsChanged;
    wf->cellsChanged = wf->on && W.cellsChanged;
    wf->frameDue = wf->on && W.frameDue;
    wf->fitStateDue = wf->on && W.fitStateDue;
    wf->stateUp = W.shadowSet && W.computeDue;
    wf->historyDue = W.historyDue;
    wf->whiteAt = W.whiteAt;
    wf->recordsAt = W.recordsAt;
    wf->slotsAt = W.slotsAt;
    wf->cellsAt = W.cellsAt;
    wf->frameAt = W.frameAt;
    wf->fitStateAt = W.fitStateAt;
    wf->idBytes = W.idCount * (uint32_t)sizeof *W.byId;
    wf->cullBytes = W.cullLayout.words * 4;
    wf->slotBytes = W.slotCount * (uint32_t)sizeof *W.slots;
    wf->cellBytes = W.cellCount * (uint32_t)sizeof *W.cells + (uint32_t)sizeof W.sines;
    wf->animatedBytes = (uint32_t)W.animated.size;
    wf->uvBytes = (uint32_t)W.uvs.size;
    wf->waveBytes = (uint32_t)W.waves.size;
    wf->instanceBytes = (uint32_t)W.instances.size;
    wf->compute = wf->on && W.computeDue && W.frameSet && W.slotCount && W.recordCount && W.transform
                  && W.cullPipeline && W.compactPipeline && screen && W.slotBuffer && W.animatedBuffer && W.views
                  && W.placed && W.cullData && W.draws && W.indices && W.drawn && W.fitResult;
    wf->shadowSet = wf->compute && W.shadowSet;
    wf->shadows = wf->shadowSet && shadow_prepare() && W.uvBuffer && W.recordBuffer && W.vertices
                  && W.ids;
    /* the water's draws take the frame's camera, with the compute passes or without */
    wf->camera = W.camera;
    if (!wf->compute)
        return 1;
    wf->frustum = W.frustum;
    wf->shadow = W.shadow;
    wf->screenW = screenW;
    wf->screenH = screenH;
    wf->slotCount = W.slotCount;
    wf->recordCount = W.recordCount;
    wf->sectionIndices = W.sectionIndices;
    wf->nindirect = W.nindirect;
    wf->ncasterRuns = W.ncasterRuns;
    wf->nchunks = W.nchunks;
    wf->cullLayout = W.cullLayout;
    wf->drawLayout = W.drawLayout;
    if (!wf->shadows)
        return 1;
    if (!gpu_grow((void **)&wf->runs, &wf->runCapacity, W.nindirect + 1, sizeof *wf->runs)
        || !gpu_grow((void **)&wf->casterArrays, &wf->casterArrayCapacity, W.ncasterRuns + 1, 1))
        return 0;
    for (i = 0; i < W.nindirect; i++) {
        wf->runs[i].pass = W.runs[i].pass;
        wf->runs[i].cull = W.runs[i].cull;
        wf->runs[i].textureArray = W.runs[i].textureArray;
    }
    memcpy(wf->casterArrays, W.casterArrays, W.ncasterRuns);
    wf->fitFront = W.fitFront;
    wf->fitBack = W.fitBack;
    wf->fitDepth = W.fitDepth;
    wf->shadowMaps = W.shadowMaps;
    wf->shadowDepth = W.shadowDepth;
    wf->fitW = W.fitW;
    wf->fitH = W.fitH;
    /* the depth pass's draws, a run each, and the shadow pass's, a run of the casters' each in
       every cascade */
    g_perf.draws += (unsigned)(W.nindirect + W.ncasterRuns * GFX_SHADOW_CASCADES);
    g_perf.indirectDraws += (unsigned)(W.nindirect + W.ncasterRuns * GFX_SHADOW_CASCADES);
    return 1;
}

static void upload_buffer(SDL_GPUCopyPass *copy, SDL_GPUTransferBuffer *from, uint32_t at, SDL_GPUBuffer *buffer,
                          uint32_t size, int cycle)
{
    SDL_GPUTransferBufferLocation src;
    SDL_GPUBufferRegion dst;
    if (!size || !buffer || !from)
        return;
    src.transfer_buffer = from;
    src.offset = at;
    dst.buffer = buffer;
    dst.offset = 0;
    dst.size = size;
    SDL_UploadToGPUBuffer(copy, &src, &dst, cycle);
}

/* The copy pass's part of the world: what world_prepare staged among the frame's texels. */
static void world_copy(void *p, SDL_GPUCopyPass *copy, SDL_GPUTransferBuffer *texels)
{
    const struct world_frame *wf = &((const struct world_part *)p)->world;
    if (!wf->on || !texels)
        return;
    if (wf->whiteDue) {
        SDL_GPUTextureTransferInfo src;
        SDL_GPUTextureRegion dst;
        SDL_zero(src);
        SDL_zero(dst);
        src.transfer_buffer = texels;
        src.offset = wf->whiteAt;
        dst.texture = wf->white;
        dst.w = dst.h = dst.d = 1;
        SDL_UploadToGPUTexture(copy, &src, &dst, false);
    }
    if (wf->recordsChanged) {
        upload_buffer(copy, texels, wf->recordsAt, wf->recordBuffer, wf->idBytes, 1);
        upload_buffer(copy, texels, wf->recordsAt + wf->idBytes, wf->cullData, wf->cullBytes, 1);
    }
    if (wf->slotsChanged)
        upload_buffer(copy, texels, wf->slotsAt, wf->slotBuffer, wf->slotBytes, 1);
    if (wf->cellsChanged)
        upload_buffer(copy, texels, wf->cellsAt, wf->cellBuffer, wf->cellBytes, 1);
    if (wf->frameDue) {
        uint32_t at = wf->frameAt;
        upload_buffer(copy, texels, at, wf->animatedBuffer, wf->animatedBytes, 1);
        at += wf->animatedBytes;
        upload_buffer(copy, texels, at, wf->uvBuffer, wf->uvBytes, 1);
        at += wf->uvBytes;
        upload_buffer(copy, texels, at, wf->waveBuffer, wf->waveBytes, 1);
        at += wf->waveBytes;
        upload_buffer(copy, texels, at, wf->instanceBuffer, wf->instanceBytes, 1);
    }
    if (wf->fitStateDue) {
        if (wf->stateUp)
            upload_buffer(copy, texels, wf->fitStateAt, wf->fitState, SHADOW_FIT_GPU_STATE * 4, 1);
        /* the history is read and written by the fit itself from frame to frame: cleared in place,
           never cycled away */
        if (wf->historyDue)
            upload_buffer(copy, texels, wf->fitStateAt + SHADOW_FIT_GPU_STATE * 4, wf->fitHistory,
                          SHADOW_FIT_GPU_HISTORY * 4, 0);
    }
}

static void world_shadows(SDL_GPUCommandBuffer *cmd, struct world_part *part);

/* The frame's compute passes, where the frame was handed over with them (world_hand_over): the
   slots' views; the view's cull of the runs, which gives each run's kept records their places in
   the view's section of the frame's buffer of indices and writes the run's one indirect draw; and
   the compaction, which copies the kept records' indices to their places. Sets part->computed
   where they were recorded, without which nothing of the world is drawn; then, in a frame with
   shadows, those (world_shadows). */
static void world_compute(void *p, SDL_GPUCommandBuffer *cmd)
{
    struct world_part *part = p;
    const struct world_frame *wf = &part->world;
    struct WlTransformFrame transform;
    struct WlCullFrame cull;
    struct WlCompactFrame compact;
    uint32_t groups[2];
    float projection[16];
    SDL_GPUStorageBufferReadWriteBinding rw[2];
    SDL_GPUBuffer *read[3];
    SDL_GPUComputePass *pass;
    part->computed = part->shadowFailed = 0;
    if (!wf->compute)
        return;
    wl_projection(projection, wf->camera.proj, wf->camera.centre, wf->camera.nearZ, wf->camera.depthScale,
                  wf->screenW, wf->screenH);
    wl_transform_frame(wf->camera.view, projection, wf->slotCount, &transform);
    SDL_zero(rw);
    rw[0].buffer = wf->views;
    rw[0].cycle = true;
    rw[1].buffer = wf->placed;
    rw[1].cycle = true;
    pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 2);
    if (!pass)
        return;
    SDL_BindGPUComputePipeline(pass, W.transform);
    read[0] = wf->slotBuffer;
    read[1] = wf->animatedBuffer;
    SDL_BindGPUComputeStorageBuffers(pass, 0, read, 2);
    SDL_PushGPUComputeUniformData(cmd, 0, &transform, sizeof transform);
    SDL_DispatchGPUCompute(pass, (wf->slotCount + 63) / 64, 1, 1);
    SDL_EndGPUComputePass(pass);
    if (wf->nindirect) {
        wl_cull_frame(&wf->frustum, (uint32_t)wf->nindirect, &wf->cullLayout, &wf->drawLayout, wf->nindirect,
                      wf->recordCount, wf->sectionIndices, &cull, groups);
        SDL_zero(rw);
        rw[0].buffer = wf->draws;
        rw[0].cycle = true;
        pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 1);
        if (!pass)
            return;
        SDL_BindGPUComputePipeline(pass, W.cullPipeline);
        read[0] = wf->cullData;
        read[1] = wf->placed;
        read[2] = wf->fitResult;
        SDL_BindGPUComputeStorageBuffers(pass, 0, read, 3);
        SDL_PushGPUComputeUniformData(cmd, 0, &cull, sizeof cull);
        SDL_DispatchGPUCompute(pass, groups[0], groups[1], 1);
        SDL_EndGPUComputePass(pass);
        wl_compact_frame((uint32_t)wf->nchunks, 0, 1, &wf->cullLayout, &wf->drawLayout, wf->recordCount, &compact,
                         groups);
        if (groups[0]) {
            SDL_zero(rw);
            rw[0].buffer = wf->drawn;
            rw[0].cycle = true;
            pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 1);
            if (!pass)
                return;
            SDL_BindGPUComputePipeline(pass, W.compactPipeline);
            read[0] = wf->cullData;
            read[1] = wf->draws;
            read[2] = wf->indices;
            SDL_BindGPUComputeStorageBuffers(pass, 0, read, 3);
            SDL_PushGPUComputeUniformData(cmd, 0, &compact, sizeof compact);
            SDL_DispatchGPUCompute(pass, groups[0], groups[1], 1);
            SDL_EndGPUComputePass(pass);
        }
    }
    part->computed = 1;
    if (wf->shadowSet)
        world_shadows(cmd, part);
}

/* Whether a recorded draw of the world's may be replayed: it is of the world as the frame handed it
   over, and what it reads is there. */
static int world_drawable(const struct world_part *part, int program, const struct world_draw *w)
{
    const struct world_frame *wf = &part->world;
    if (w->generation != wf->generation || !wf->on)
        return 0;
    if (program == PROG_WATER)
        return wf->cellBuffer && wf->waveBuffer && wf->instanceBuffer;
    if (!part->computed || !wf->recordBuffer || !wf->vertices || !wf->ids || !wf->uvBuffer || !wf->views)
        return 0;
    return w->indirect ? wf->drawn && wf->draws : wf->indices != NULL;
}

/* After a frame is handed over: what goes up with it is no longer due. */
static void world_flushed(void)
{
    W.recordsChanged = W.slotsChanged = W.cellsChanged = 0;
    W.whiteDue = 0;
    if (W.fitStateDue)
        W.historyDue = 0;
    W.fitStateDue = 0;
}

/* A frame the render thread could not submit: what it was to take up of the world goes up with the
   next, unless the world has been made again since. */
static void world_unflushed(const struct world_frame *wf)
{
    if (!wf->on || wf->generation != W.generation)
        return;
    W.recordsChanged |= wf->recordsChanged;
    W.slotsChanged |= wf->slotsChanged;
    W.cellsChanged |= wf->cellsChanged;
    W.whiteDue |= wf->whiteDue;
    if (wf->fitStateDue)
        W.historyDue |= wf->historyDue;
}

/* ---- shadows ---- */

static int shadow_supported(void)
{
    const char *env = SDL_getenv("RAGEXE_SHADOWS");
    SDL_GPUDevice *dev = gpu_dev();
    if (env && env[0] == '0' && !env[1])
        return 0;
    return dev
           && SDL_GPUTextureSupportsFormat(dev, SDL_GPU_TEXTUREFORMAT_R32_FLOAT, SDL_GPU_TEXTURETYPE_2D_ARRAY,
                                           SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER)
           && SDL_GPUTextureSupportsFormat(dev, SDL_GPU_TEXTUREFORMAT_R32_FLOAT, SDL_GPU_TEXTURETYPE_2D,
                                           SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ);
}

int gfx_shadows_supported(void)
{
    if (!gpu_dev() || W.layer < 0)
        return 0;
    if (W.shadowState == 0)
        W.shadowState = shadow_supported() ? 1 : -1;
    return W.shadowState > 0;
}

/* Shadows are optional, the renderer is not: a shader, pipeline or buffer only the shadows use that
   cannot be made turns them off for the device, said once, and the ground keeps its baked
   shadows. */
static void shadows_off(const char *what)
{
    if (W.shadowState >= 0)
        SDL_Log("gfx: %s: %s; no shadows", what, SDL_GetError());
    W.shadowState = -1;
}

/* a texture of the shadows' given up; the frames handed over may still draw with it (gpu_retire) */
static void release_texture(SDL_GPUTexture **t)
{
    gpu_retire(*t, NULL);
    *t = NULL;
}

/* What a frame's shadows need: the cascades' maps and their depth, made once, and the depth pass's
   two targets of view depth and its depth at the screen's size (with the screen's GPU_MARGIN, so that
   their texels are the screen's pixels), made again when it changes. 0 where they cannot be: the
   frame then draws no shadows. Maps that cannot be made turn the shadows off; targets that cannot
   be made at the screen's size are tried again once it changes. */
static int shadow_prepare(void)
{
    SDL_GPUTextureCreateInfo info;
    int screenW, screenH;
    if (!gfx_shadows_supported() || !gpu_screen_size(&screenW, &screenH))
        return 0;
    if (!W.shadowMaps || !W.shadowDepth) {
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
        info.format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = info.height = GFX_SHADOW_SIZE + GPU_MARGIN;
        info.layer_count_or_depth = GFX_SHADOW_CASCADES;
        info.num_levels = 1;
        W.shadowMaps = gpu_make_texture(&info);
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = gpu_depth_format();
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        info.layer_count_or_depth = 1;
        W.shadowDepth = gpu_make_texture(&info);
        if (!W.shadowMaps || !W.shadowDepth) {
            shadows_off("shadow maps");
            release_texture(&W.shadowMaps);
            release_texture(&W.shadowDepth);
            return 0;
        }
    }
    if (!W.fitFront || W.fitW != screenW || W.fitH != screenH) {
        if (W.fitFailedW == screenW && W.fitFailedH == screenH)
            return 0;
        release_texture(&W.fitFront);
        release_texture(&W.fitBack);
        release_texture(&W.fitDepth);
        SDL_zero(info);
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ;
        info.width = (Uint32)(screenW + GPU_MARGIN);
        info.height = (Uint32)(screenH + GPU_MARGIN);
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        W.fitFront = gpu_make_texture(&info);
        W.fitBack = gpu_make_texture(&info);
        info.format = gpu_depth_format();
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        W.fitDepth = gpu_make_texture(&info);
        if (!W.fitFront || !W.fitBack || !W.fitDepth) {
            SDL_Log("gfx: the shadow fit's depth targets at %dx%d: %s; no shadows at this size", screenW,
                    screenH, SDL_GetError());
            release_texture(&W.fitFront);
            release_texture(&W.fitBack);
            release_texture(&W.fitDepth);
            W.fitW = W.fitH = 0;
            W.fitFailedW = screenW;
            W.fitFailedH = screenH;
            return 0;
        }
        W.fitW = screenW;
        W.fitH = screenH;
        W.fitFailedW = W.fitFailedH = 0;
    }
    return 1;
}

/* The depth pass (world_depth.hlsl): the view's runs the cull kept, every pass's, into the fit's two
   targets of view depth, the opaque passes' and the ground's first. Single-sampled, so that the
   compute passes can read what it draws, and at the main pass's viewport, so that a texel is a
   pixel of the screen. 0 where it could not be recorded. */
static int depth_pass(SDL_GPUCommandBuffer *cmd, const struct world_frame *wf)
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
        colour[i].texture = i ? wf->fitBack : wf->fitFront;
        colour[i].load_op = SDL_GPU_LOADOP_CLEAR;
        colour[i].store_op = SDL_GPU_STOREOP_STORE;
    }
    depth.texture = wf->fitDepth;
    depth.clear_depth = 1.0f;
    depth.load_op = SDL_GPU_LOADOP_CLEAR;
    depth.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    pass = SDL_BeginGPURenderPass(cmd, colour, 2, &depth);
    if (!pass)
        return 0;
    viewport.x = viewport.y = 0.5f;
    viewport.w = (float)wf->fitW;
    viewport.h = (float)wf->fitH;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(pass, &viewport);
    scissor.x = scissor.y = 0;
    scissor.w = wf->fitW;
    scissor.h = wf->fitH;
    SDL_SetGPUScissor(pass, &scissor);
    SDL_zero(binding);
    binding[0].buffer = wf->vertices;
    binding[1].buffer = wf->ids;
    SDL_BindGPUVertexBuffers(pass, 0, binding, 2);
    binding[0].buffer = wf->drawn;
    SDL_BindGPUIndexBuffer(pass, &binding[0], SDL_GPU_INDEXELEMENTSIZE_32BIT);
    read[0] = wf->recordBuffer;
    read[1] = wf->views;
    read[2] = wf->uvBuffer;
    SDL_BindGPUVertexStorageBuffers(pass, 0, read, 3);
    for (i = 0; i < wf->nindirect; i++) {
        const struct world_run *run = &wf->runs[i];
        SDL_GPUGraphicsPipeline *pipe =
            W.depthPipeline[run->pass <= SW_PASS_GROUND][world_cull(run->cull) - GPU_CULL_NONE];
        SDL_GPUTextureSamplerBinding texture;
        int wrap = run->pass == SW_PASS_OPAQUE_WRAP || run->pass == SW_PASS_ALPHA_WRAP;
        if (pipe != bound) {
            SDL_BindGPUGraphicsPipeline(pass, pipe);
            bound = pipe;
        }
        texture.texture = world_array(wf, run->textureArray);
        texture.sampler = gpu_stage_sampler(wrap ? DRAW_ADDRESS_WRAP : DRAW_ADDRESS_CLAMP, DRAW_FILTER_MIPMAPS, 0);
        SDL_BindGPUFragmentSamplers(pass, 0, &texture, 1);
        SDL_DrawGPUIndexedPrimitivesIndirect(pass, wf->draws, wl_view_command((uint32_t)i) * INDIRECT_SIZE, 1);
    }
    SDL_EndGPURenderPass(pass);
    return 1;
}

/* The fit (shadow_range, shadow_extents and shadow_cascades.hlsl) of the depth pass's targets into
   wf->fitResult, from and into the squares and the range of the frames before. 0 where it could not
   be recorded. */
static int fit_passes(SDL_GPUCommandBuffer *cmd, const struct world_frame *wf, const struct ShadowFitGpuFrame *u)
{
    SDL_GPUComputePipeline *pipes[2] = { W.fitRange, W.fitExtents };
    SDL_GPUStorageBufferReadWriteBinding rw[2];
    SDL_GPUTexture *depths[2] = { wf->fitFront, wf->fitBack };
    SDL_GPUComputePass *pass;
    uint32_t groups[2];
    int p;
    shadow_fit_gpu_groups(u->size[0], u->size[1], groups);
    for (p = 0; p < 2; p++) {
        SDL_zero(rw);
        rw[0].buffer = wf->fitState;
        pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 1);
        if (!pass)
            return 0;
        SDL_BindGPUComputePipeline(pass, pipes[p]);
        SDL_BindGPUComputeStorageTextures(pass, 0, depths, 2);
        /* the extents cut the range into slices as the range kept from the frames before says */
        if (p == 1)
            SDL_BindGPUComputeStorageBuffers(pass, 0, &wf->fitHistory, 1);
        SDL_PushGPUComputeUniformData(cmd, 0, u, sizeof *u);
        SDL_DispatchGPUCompute(pass, groups[0], groups[1], 1);
        SDL_EndGPUComputePass(pass);
    }
    SDL_zero(rw);
    rw[0].buffer = wf->fitHistory;
    rw[1].buffer = wf->fitResult;
    rw[1].cycle = true;
    pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 2);
    if (!pass)
        return 0;
    SDL_BindGPUComputePipeline(pass, W.fitCascades);
    SDL_BindGPUComputeStorageBuffers(pass, 0, &wf->fitState, 1);
    SDL_PushGPUComputeUniformData(cmd, 0, u, sizeof *u);
    SDL_DispatchGPUCompute(pass, 1, 1, 1);
    SDL_EndGPUComputePass(pass);
    return 1;
}

/* Each cascade's casters: the cull of the casters' runs against the boxes the fit left, and their
   indices compacted into the cascades' sections, after the view's. 0 where it could not be
   recorded. */
static int caster_cull(SDL_GPUCommandBuffer *cmd, const struct world_frame *wf)
{
    struct WlCullFrame cull;
    struct WlCompactFrame compact;
    SDL_GPUStorageBufferReadWriteBinding rw;
    SDL_GPUBuffer *read[3];
    SDL_GPUComputePass *pass;
    uint32_t groups[2];
    wl_cull_frame(NULL, (uint32_t)wf->ncasterRuns, &wf->cullLayout, &wf->drawLayout, wf->nindirect, wf->recordCount,
                  wf->sectionIndices, &cull, groups);
    SDL_zero(rw);
    rw.buffer = wf->draws;
    pass = SDL_BeginGPUComputePass(cmd, NULL, 0, &rw, 1);
    if (!pass)
        return 0;
    SDL_BindGPUComputePipeline(pass, W.cullPipeline);
    read[0] = wf->cullData;
    read[1] = wf->placed;
    read[2] = wf->fitResult;
    SDL_BindGPUComputeStorageBuffers(pass, 0, read, 3);
    SDL_PushGPUComputeUniformData(cmd, 0, &cull, sizeof cull);
    SDL_DispatchGPUCompute(pass, groups[0], groups[1], 1);
    SDL_EndGPUComputePass(pass);
    wl_compact_frame((uint32_t)wf->nchunks, 1, WL_CASCADES, &wf->cullLayout, &wf->drawLayout, wf->recordCount, &compact,
                     groups);
    if (!groups[0])
        return 1;
    SDL_zero(rw);
    rw.buffer = wf->drawn;
    pass = SDL_BeginGPUComputePass(cmd, NULL, 0, &rw, 1);
    if (!pass)
        return 0;
    SDL_BindGPUComputePipeline(pass, W.compactPipeline);
    read[0] = wf->cullData;
    read[1] = wf->draws;
    read[2] = wf->indices;
    SDL_BindGPUComputeStorageBuffers(pass, 0, read, 3);
    SDL_PushGPUComputeUniformData(cmd, 0, &compact, sizeof compact);
    SDL_DispatchGPUCompute(pass, groups[0], groups[1], 1);
    SDL_EndGPUComputePass(pass);
    return 1;
}

/* The shadow pass (world_caster.hlsl): each cascade's casters into its layer of the maps, a draw a
   run, the alpha-tested runs with their texture array and the others without the alpha test
   (world_caster_solid.hlsl), in the casters' order. The maps are a texel wider and taller than
   GFX_SHADOW_SIZE (GPU_MARGIN), drawn at the same half-pixel viewport as every pass. 0 where it could
   not be recorded. */
static int caster_pass(SDL_GPUCommandBuffer *cmd, const struct world_frame *wf)
{
    SDL_GPUBufferBinding binding[2];
    SDL_GPUBuffer *read[4];
    SDL_GPUViewport viewport;
    SDL_Rect scissor;
    SDL_GPUGraphicsPipeline *bound;
    /* the casters read their texture as the opaque lists do */
    const struct DrawModeDesc *caster = draw_mode(DRAW_OPAQUE_CLAMP);
    int c;
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
        colour.texture = wf->shadowMaps;
        colour.layer_or_depth_plane = (Uint32)c;
        colour.clear_color.r = colour.clear_color.g = colour.clear_color.b = colour.clear_color.a = 1.0f;
        colour.load_op = SDL_GPU_LOADOP_CLEAR;
        colour.store_op = SDL_GPU_STOREOP_STORE;
        depth.texture = wf->shadowDepth;
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
        SDL_zero(binding);
        binding[0].buffer = wf->vertices;
        binding[1].buffer = wf->ids;
        SDL_BindGPUVertexBuffers(pass, 0, binding, 2);
        binding[0].buffer = wf->drawn;
        SDL_BindGPUIndexBuffer(pass, &binding[0], SDL_GPU_INDEXELEMENTSIZE_32BIT);
        read[0] = wf->recordBuffer;
        read[1] = wf->placed;
        read[2] = wf->uvBuffer;
        read[3] = wf->fitResult;
        SDL_BindGPUVertexStorageBuffers(pass, 0, read, 4);
        SDL_PushGPUVertexUniformData(cmd, 0, uniform, sizeof uniform);
        bound = NULL;
        for (k = 0; k < wf->ncasterRuns; k++) {
            int tested = wf->casterArrays[k] != SW_NO_ARRAY;
            if (W.casterPipeline[tested] != bound) {
                bound = W.casterPipeline[tested];
                SDL_BindGPUGraphicsPipeline(pass, bound);
            }
            if (tested) {
                SDL_GPUTextureSamplerBinding texture;
                texture.texture = world_array(wf, wf->casterArrays[k]);
                texture.sampler = gpu_stage_sampler(caster->address[0], caster->filter, 0);
                SDL_BindGPUFragmentSamplers(pass, 0, &texture, 1);
            }
            SDL_DrawGPUIndexedPrimitivesIndirect(
                pass, wf->draws, wl_caster_command(wf->nindirect, wf->ncasterRuns, c, (uint32_t)k) * INDIRECT_SIZE, 1);
        }
        SDL_EndGPURenderPass(pass);
    }
    return 1;
}

/* The frame's shadows, before its own passes: the depth pass, the fit, the casters' cull and the
   shadow pass, each only once the one before it was recorded. The fit's result stays on the GPU,
   where every draw of the world reads it; where what they need was not there when the frame was
   handed over (world_hand_over) or a step could not be recorded, the frame's draws of the world
   receive no shadows (part->shadowFailed, world_draw_hook) and keep the ground's baked ones. */
static void world_shadows(SDL_GPUCommandBuffer *cmd, struct world_part *part)
{
    const struct world_frame *wf = &part->world;
    struct ShadowFitGpuFrame u;
    part->shadowFailed = 1;
    if (!wf->shadows)
        return;
    shadow_fit_gpu_frame(&wf->shadow.fit, wf->fitW, wf->fitH, wf->shadow.ambient, wf->shadow.darkness, &u);
    if (!depth_pass(cmd, wf) || !fit_passes(cmd, wf, &u) || (wf->ncasterRuns && !caster_cull(cmd, wf))
        || !caster_pass(cmd, wf))
        return;
    part->shadowFailed = 0;
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
    if (!gpu_dev() || !out || !gpu_flush() || !W.fittedSet || !W.fitFront || !W.fitState || !W.fitResult)
        return -1;
    depths = out->front && out->back;
    if (depths && (out->width < W.fitW || out->height < W.fitH)) {
        out->width = W.fitW;
        out->height = W.fitH;
        return -1;
    }
    out->width = W.fitW;
    out->height = W.fitH;
    depthBytes = depths ? (uint32_t)W.fitW * (uint32_t)W.fitH * 4 : 0;
    SDL_zero(info);
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    info.size = depthBytes * 2 + (uint32_t)sizeof state + (uint32_t)sizeof g;
    down = SDL_CreateGPUTransferBuffer(gpu_dev(), &info);
    cmd = down ? gpu_commands() : NULL;
    if (!cmd) {
        if (down)
            SDL_ReleaseGPUTransferBuffer(gpu_dev(), down);
        return -1;
    }
    copy = SDL_BeginGPUCopyPass(cmd);
    if (depths) {
        download_depths(copy, W.fitFront, W.fitW, W.fitH, down, 0);
        download_depths(copy, W.fitBack, W.fitW, W.fitH, down, depthBytes);
    }
    download_buffer(copy, W.fitState, (uint32_t)sizeof state, down, depthBytes * 2);
    download_buffer(copy, W.fitResult, (uint32_t)sizeof g, down, depthBytes * 2 + (uint32_t)sizeof state);
    SDL_EndGPUCopyPass(copy);
    if (gpu_submit(cmd, &fence) && SDL_WaitForGPUFences(gpu_dev(), true, &fence, 1)
        && (p = SDL_MapGPUTransferBuffer(gpu_dev(), down, false))) {
        if (depths) {
            memcpy(out->front, p, depthBytes);
            memcpy(out->back, p + depthBytes, depthBytes);
        }
        memcpy(state, p + depthBytes * 2, sizeof state);
        memcpy(&g, p + depthBytes * 2 + sizeof state, sizeof g);
        SDL_UnmapGPUTransferBuffer(gpu_dev(), down);
        ok = 1;
    }
    if (fence)
        SDL_ReleaseGPUFence(gpu_dev(), fence);
    SDL_ReleaseGPUTransferBuffer(gpu_dev(), down);
    if (!ok)
        return -1;
    out->frame = W.fitted.fit;
    shadow_fit_gpu_read(state, &g, &out->fit);
    return 0;
}

/* The storage buffers the world's programs read (src/render/shaders), in the order they declare
   them, and their vertex buffers: the records' the records by id, the slots' views, the shadow
   fit's result and the texture-coordinate matrices, and the shared buffers, whose index buffer
   world_draw_hook binds by the draw (the shared indices, or the frame's compacted ones); the water
   the frame's draw records, its cells with the sines and its waves, and its cells' instances. (The
   depth pass and the shadow pass bind their own: depth_pass, caster_pass.) */
static void bind_world_program(struct GpuPass *pass, const struct world_frame *wf, int program)
{
    SDL_GPUBuffer *vertex[4], *fragment[1];
    SDL_GPUBufferBinding binding[2];
    Uint32 nv = 0;
    fragment[0] = pass->records;
    SDL_zero(binding);
    if (program == PROG_WORLD) {
        vertex[nv++] = wf->recordBuffer;
        vertex[nv++] = wf->views;
        vertex[nv++] = wf->fitResult;
        vertex[nv++] = wf->uvBuffer;
        fragment[0] = wf->fitResult;
        binding[0].buffer = wf->vertices;
        binding[1].buffer = wf->ids;
        SDL_BindGPUVertexBuffers(pass->pass, 0, binding, 2);
    } else {
        vertex[nv++] = pass->records;
        vertex[nv++] = wf->cellBuffer;
        vertex[nv++] = wf->waveBuffer;
        binding[0].buffer = wf->instanceBuffer;
        SDL_BindGPUVertexBuffers(pass->pass, 0, binding, 1);
    }
    SDL_BindGPUVertexStorageBuffers(pass->pass, 0, vertex, nv);
    SDL_BindGPUFragmentStorageBuffers(pass->pass, 0, fragment, 1);
}

/* The uniform a world program's draw takes: its draw record itself, from the frame's uniforms; the
   water's its record, the frame's waves and cells, and the frame's view, which its quads are tested
   in, and that view to clip space (one of the frame's views). */
static void push_uniforms(SDL_GPUCommandBuffer *cmd, const struct world_part *part, int program,
                          const struct world_draw *w)
{
    const struct GpuDraw *r;
    if (program == PROG_WATER) {
        const struct GfxWorldView *camera = &part->world.camera;
        const struct GpuView *v = (const struct GpuView *)part->views.data + w->uniform[3];
        struct GpuWaterFrame u;
        memset(&u, 0, sizeof u);
        u.frame[0] = w->uniform[0];
        u.frame[1] = w->uniform[1];
        u.frame[2] = w->uniform[2];
        memcpy(u.view, camera->view, sizeof u.view);
        u.screen[0] = camera->proj[0];
        u.screen[1] = camera->proj[1];
        u.screen[2] = camera->centre[0];
        u.screen[3] = camera->centre[1];
        memcpy(u.toClip, v->toClip, sizeof u.toClip);
        SDL_PushGPUVertexUniformData(cmd, 0, &u, sizeof u);
        return;
    }
    r = (const struct GpuDraw *)part->uniforms.data + w->uniform[0];
    if (part->shadowFailed && r->mesh[3]) {
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

/* A draw of the world's in a pass of the frame (the draw hook): left out where what it reads is not
   there (world_drawable); its texture arrays as the frame was handed over, and no shadow maps where
   the frame's shadows were not all recorded; its program's buffers, the index buffer it draws from
   and its uniform bound where they are not bound already; then the draw. */
static void world_draw_hook(void *p, struct GpuPass *pass, const struct GpuLayerDraw *d)
{
    struct world_part *part = p;
    const struct world_frame *wf = &part->world;
    SDL_GPUTextureSamplerBinding bind[3];
    struct world_draw w;
    memcpy(&w, d->words, sizeof w);
    if (!world_drawable(part, d->program, &w))
        return;
    memcpy(bind, d->bind, sizeof bind);
    if (d->program == PROG_WORLD) {
        bind[0].texture = world_array(wf, w.arrays[0]);
        bind[1].texture = world_array(wf, w.arrays[1]);
        if (part->shadowFailed)
            bind[2].texture = W.noShadow;
    }
    if (gpu_pass_use(pass, d)) {
        bind_world_program(pass, wf, d->program);
        part->pushedAny = 0;
        part->indices = NULL;
    }
    if (d->program == PROG_WORLD) {
        SDL_GPUBuffer *want = w.indirect ? wf->drawn : wf->indices;
        if (want != part->indices) {
            SDL_GPUBufferBinding binding;
            binding.buffer = want;
            binding.offset = 0;
            SDL_BindGPUIndexBuffer(pass->pass, &binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            part->indices = want;
        }
    }
    gpu_pass_samplers(pass, bind, 3);
    if (!part->pushedAny || memcmp(part->pushed, w.uniform, sizeof part->pushed)) {
        push_uniforms(pass->cmd, part, d->program, &w);
        memcpy(part->pushed, w.uniform, sizeof part->pushed);
        part->pushedAny = 1;
    }
    if (d->program == PROG_WATER)
        SDL_DrawGPUPrimitives(pass->pass, 6, w.instances, 0, w.firstInstance);
    else if (w.indirect)
        SDL_DrawGPUIndexedPrimitivesIndirect(pass->pass, wf->draws, w.firstIndex * INDIRECT_SIZE, 1);
    else
        SDL_DrawGPUIndexedPrimitives(pass->pass, w.count, 1, w.firstIndex, w.baseVertex, 0);
}

int gfx_read_world_level(uint8_t array, uint16_t layer, int level, void *out)
{
    const struct world_array *a;
    if (!gpu_dev() || !gpu_flush())
        return -1;
    a = array < W.narrays ? &W.arrays[array] : NULL;
    if (!a || !a->tex || layer >= a->layers)
        return -1;
    return gpu_read_level(a->tex, layer, a->levels, level, a->width, a->height, out);
}

/* ---- the layer's hooks ---- */

#define ATTR(p, loc, slot, fmt, off) ((p)->attrs[(p)->nattrs].location = (loc), \
                                      (p)->attrs[(p)->nattrs].buffer_slot = (slot), \
                                      (p)->attrs[(p)->nattrs].format = (fmt), (p)->attrs[(p)->nattrs].offset = (off), \
                                      (p)->nattrs++)

/* A pipeline of one of the world's programs, its description zeroed, then its shaders and vertex
   input: the records', the depth pass's and the casters' their vertices, then each vertex's record
   id; the water's a cell and its colour per instance. */
static void world_pipe(struct GpuPipe *p, int program)
{
    memset(p, 0, sizeof *p);
    p->vs = W.vs[program];
    p->fs = W.fs[program];
    p->targets = program == PROG_DEPTH ? 2 : 1;
    if (program == PROG_WATER) {
        p->buffers[0].pitch = WATER_INSTANCE;
        p->buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
        ATTR(p, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
        ATTR(p, 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 4);
        p->nbuffers = 1;
        return;
    }
    p->buffers[0].pitch = sizeof(struct GfxMeshVertex);
    p->buffers[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    p->buffers[1].slot = 1;
    p->buffers[1].pitch = 4;
    p->buffers[1].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    p->nbuffers = 2;
    if (program == PROG_WORLD) {
        ATTR(p, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
        ATTR(p, 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 12);
        ATTR(p, 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 24);
        ATTR(p, 3, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, 28);
        ATTR(p, 4, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 32);
        ATTR(p, 5, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 40);
        ATTR(p, 6, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
    } else {
        ATTR(p, 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
        ATTR(p, 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, 32);
        ATTR(p, 2, 1, SDL_GPU_VERTEXELEMENTFORMAT_UINT, 0);
    }
}

#undef ATTR

/* The world's shaders, compute pipelines and buffers with a new device: the shadows' own, where
   they cannot be made, turn the shadows off; the rest fails the device. What a draw of the world
   binds without shadows, the shadow fit's result, which every draw of the world and the view's cull
   bind, shadows or none; then the fit's passes, its state and its history, which only the shadows
   need. */
static int world_made(void)
{
    static const struct GpuShaderCode *const vs[WORLD_PROGRAMS] = {
        &world_vs, &world_caster_vs, &world_caster_vs, &world_depth_vs, &water_vs
    };
    static const struct GpuShaderCode *const fs[WORLD_PROGRAMS] = {
        &world_fs, &world_caster_fs, &world_caster_solid_fs, &world_depth_fs, &water_fs
    };
    SDL_GPUTextureCreateInfo info;
    SDL_GPUBufferCreateInfo binfo;
    int i;
    for (i = 0; i < WORLD_PROGRAMS; i++) {
        /* the casters share their vertex shader */
        W.vs[i] = i == PROG_CASTER_SOLID ? W.vs[PROG_CASTER] : gpu_shader(vs[i]);
        W.fs[i] = gpu_shader(fs[i]);
        if (W.vs[i] && W.fs[i])
            continue;
        /* the casters' and the depth pass's serve the shadows alone */
        if (i == PROG_CASTER || i == PROG_CASTER_SOLID || i == PROG_DEPTH) {
            shadows_off("the shadows' shaders");
            continue;
        }
        SDL_Log("gfx: SDL_CreateGPUShader: %s", SDL_GetError());
        return 0;
    }
    SDL_zero(info);
    info.type = SDL_GPU_TEXTURETYPE_2D_ARRAY;
    info.format = SDL_GPU_TEXTUREFORMAT_R32_FLOAT;
    info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    info.width = info.height = 1;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    W.noShadow = gpu_make_texture(&info);
    W.transform = gpu_compute(&world_transform_cs);
    W.cullPipeline = gpu_compute(&world_cull_cs);
    W.compactPipeline = gpu_compute(&world_compact_cs);
    SDL_zero(binfo);
    binfo.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE
                  | SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    binfo.size = sizeof(struct ShadowFitGpuCascades);
    W.fitResult = SDL_CreateGPUBuffer(gpu_dev(), &binfo);
    W.fitRange = gpu_compute(&shadow_range_cs);
    W.fitExtents = gpu_compute(&shadow_extents_cs);
    W.fitCascades = gpu_compute(&shadow_cascades_cs);
    if (!W.fitRange || !W.fitExtents || !W.fitCascades)
        shadows_off("the shadow fit's passes");
    binfo.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
    binfo.size = SHADOW_FIT_GPU_STATE * 4;
    W.fitState = SDL_CreateGPUBuffer(gpu_dev(), &binfo);
    binfo.size = SHADOW_FIT_GPU_HISTORY * 4;
    W.fitHistory = SDL_CreateGPUBuffer(gpu_dev(), &binfo);
    if (!W.fitState || !W.fitHistory)
        shadows_off("the shadow fit's buffers");
    W.historyDue = 1;
    if (!W.noShadow || !W.transform || !W.cullPipeline || !W.compactPipeline || !W.fitResult) {
        SDL_Log("gfx: the world's resources: %s", SDL_GetError());
        return 0;
    }
    W.generation++;
    return 1;
}

/* Every pipeline the world draws with, once the screen's samples and depth are known: a mode's
   records in each cull mode and its water, which are drawn among the quads of the alpha lists, in
   world coordinates, both faces of each triangle as the quads; then the depth pass's and the shadow
   casters', without which the device draws no shadows: the depth pass's, single-sampled into two
   targets of view depth, by whether it writes the second (the passes up to the ground) and by cull
   mode, and the casters', without the alpha test and with it. */
static int world_pipelines(void)
{
    struct GpuPipe p;
    int mode, cull, d;
    for (mode = 0; mode < DRAW_MODE_COUNT; mode++) {
        const struct DrawModeDesc *m = draw_mode(mode);
        if (m->src == DRAW_BLEND_FROM_DRAW)
            continue;
        if ((m->into & DRAW_INTO_SCREEN) && (m->into & DRAW_MESHES)) {
            world_pipe(&p, PROG_WATER);
            gpu_pipe_mode(&p, m);
            if (!(W.waterPipeline[mode] = gpu_pipeline(&p)))
                return 0;
        }
        if (m->into & DRAW_MESHES)
            for (cull = GPU_CULL_NONE; cull <= GPU_CULL_CCW; cull++) {
                world_pipe(&p, PROG_WORLD);
                gpu_pipe_mode(&p, m);
                p.cull = cull;
                if (!(W.worldPipeline[mode][cull - GPU_CULL_NONE] = gpu_pipeline(&p)))
                    return 0;
            }
    }
    if (W.shadowState < 0)
        return 1;
    for (d = 0; d < 2; d++)
        for (cull = GPU_CULL_NONE; cull <= GPU_CULL_CCW; cull++) {
            world_pipe(&p, PROG_DEPTH);
            p.depth = DRAW_DEPTH_WRITE;
            p.cull = cull;
            p.mask = GPU_MASK_RGBA;
            p.r32f = 1;
            p.hasDepth = 1;
            p.samples = SDL_GPU_SAMPLECOUNT_1;
            p.back = d;
            if (!(W.depthPipeline[d][cull - GPU_CULL_NONE] = gpu_pipeline(&p))) {
                shadows_off("the depth pass's pipelines");
                return 1;
            }
        }
    for (d = 0; d < 2; d++) {
        world_pipe(&p, d ? PROG_CASTER : PROG_CASTER_SOLID);
        p.depth = DRAW_DEPTH_WRITE;
        p.cull = GPU_CULL_NONE;
        p.mask = GPU_MASK_RGBA;
        p.r32f = 1;
        p.hasDepth = 1;
        p.samples = SDL_GPU_SAMPLECOUNT_1;
        if (!(W.casterPipeline[d] = gpu_pipeline(&p))) {
            shadows_off("the casters' pipelines");
            return 1;
        }
    }
    return 1;
}

/* The device goes: the world is made again for the next device (gfx_world_generation), and what it
   made with this one goes, the pipelines with the device. */
static void world_lost(void)
{
    SDL_GPUDevice *dev = gpu_dev();
    SDL_GPUTexture **textures[] = { &W.white, &W.noShadow, &W.shadowMaps, &W.shadowDepth, &W.fitFront, &W.fitBack,
                                    &W.fitDepth };
    SDL_GPUComputePipeline **computes[] = { &W.transform, &W.cullPipeline, &W.compactPipeline, &W.fitRange,
                                            &W.fitExtents, &W.fitCascades };
    SDL_GPUBuffer **buffers[] = { &W.fitState, &W.fitHistory, &W.fitResult };
    size_t i;
    world_release();
    if (W.streamFence) {
        SDL_WaitForGPUFences(dev, true, &W.streamFence, 1);
        SDL_ReleaseGPUFence(dev, W.streamFence);
    }
    if (W.stream)
        SDL_ReleaseGPUTransferBuffer(dev, W.stream);
    W.streamFence = NULL;
    W.stream = NULL;
    W.streamSize = 0;
    for (i = 0; i < sizeof textures / sizeof textures[0]; i++) {
        if (*textures[i])
            SDL_ReleaseGPUTexture(dev, *textures[i]);
        *textures[i] = NULL;
    }
    for (i = 0; i < sizeof computes / sizeof computes[0]; i++) {
        if (*computes[i])
            SDL_ReleaseGPUComputePipeline(dev, *computes[i]);
        *computes[i] = NULL;
    }
    for (i = 0; i < sizeof buffers / sizeof buffers[0]; i++) {
        if (*buffers[i])
            SDL_ReleaseGPUBuffer(dev, *buffers[i]);
        *buffers[i] = NULL;
    }
    for (i = 0; i < WORLD_PROGRAMS; i++) {
        if (W.vs[i] && (i != PROG_CASTER_SOLID || W.vs[i] != W.vs[PROG_CASTER]))
            SDL_ReleaseGPUShader(dev, W.vs[i]);
        if (W.fs[i])
            SDL_ReleaseGPUShader(dev, W.fs[i]);
    }
    memset(W.vs, 0, sizeof W.vs);
    memset(W.fs, 0, sizeof W.fs);
    memset(W.worldPipeline, 0, sizeof W.worldPipeline);
    memset(W.waterPipeline, 0, sizeof W.waterPipeline);
    memset(W.depthPipeline, 0, sizeof W.depthPipeline);
    W.casterPipeline[0] = W.casterPipeline[1] = NULL;
    W.fitW = W.fitH = W.fitFailedW = W.fitFailedH = 0;
    W.fittedSet = 0;
    W.frameSet = W.shadowSet = 0;
    W.shadowState = 0;
    W.generation++;
}

/* A frame begun into its packet: nothing of the world's in it yet. */
static void world_begin(void *p)
{
    struct world_part *part = p;
    part->views.size = part->uniforms.size = 0;
    W.cameraView = -1;
    W.instances.size = 0;
    W.computeDue = W.frameDue = 0;
}

/* meshes or textures that have not gone up yet */
static int world_pending(void)
{
    return W.nmeshUploads || W.ntextureUploads;
}

/* The world's arrays as large as their layers, its meshes and textures up through the stream, then
   what goes with the frame. */
static void world_flush_prepare(void)
{
    if (!world_on())
        return;
    world_arrays();
    if (!world_stream(1))
        world_failed("uploads");
    else if (!world_prepare())
        world_failed("buffers");
}

static int world_flush_hand_over(void *p)
{
    return world_hand_over(p);
}

/* The frame went whole: what went up with it is not due any more, and the world's frame lasts
   until the frame is shown. */
static void world_flush_done(int presented)
{
    world_flushed();
    W.failures = 0;
    if (presented)
        W.frameSet = W.shadowSet = 0;
}

/* A frame back: what one the render thread could not submit was to take up goes up with the next;
   a frame drawn with its shadows has the ones the GPU fitted last. */
static void world_retired(void *p, int how, unsigned number)
{
    const struct world_part *part = p;
    if (how == GPU_FRAME_UNSUBMITTED)
        world_unflushed(&part->world);
    else if (how == GPU_FRAME_DRAWN && part->world.shadowSet && !part->shadowFailed
             && (int)(number - W.fittedFrom) >= 0) {
        W.fitted = part->world.shadow;
        W.fittedSet = 1;
    }
}

static const struct GpuLayer world_layer = {
    sizeof(struct world_part),
    world_made,
    world_pipelines,
    world_lost,
    world_begin,
    world_pending,
    world_flush_prepare,
    world_flush_hand_over,
    world_flush_done,
    world_retired,
    world_copy,
    world_compute,
    world_draw_hook,
};

int gfx_world_attach(void)
{
    W.layer = gpu_add_layer(&world_layer);
    return W.layer >= 0;
}
