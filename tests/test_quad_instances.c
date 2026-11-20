/* The client's quads and triangles as instances (render/gfx.h, gfx_draw_quad and
   gfx_draw_vertices; render/shaders/quad.hlsl) on this machine's GPU, against the CPU code they
   replaced (tests/fixtures/quad_reference.c: the vertices render_object_submit_spr_quad,
   particle_render, sprite_particle_render, the effect primitives' prim_draw_quad and the ground's
   cell tiles made, and the multisampled pass's snap_flat, as they were) and the client's own C
   code (renderer_project_vertex, renderer_project_depth), within the tolerance plans/gpu-renderer.md
   sets for the GPU's floats:

   - Records of every kind, made as the client makes them (sprites by render_object_submit_spr_quad
     itself, actors/sprite_quad.c; particles by particle_render and sprite_particle_render,
     effects/particle.c and sprite_particle.c; the effect primitives' triangles by prim_draw_quad,
     effects/rag/effect_prim_draw.c, in each of its modes; the ground's cell tiles by
     world/ground_draw.c; the rest by render/quads.c), realistic, edge and fuzzed, read on the
     GPU's vertex stage by quad_vertex, the vertex the quad shader draws, as made and as moved onto
     pixel boundaries (QUAD_SNAP), against the reference (compare): every corner's x and y within
     1/16 of a pixel, its rhw within 1e-5 of the CPU's relative, its depth within 1e-5 relative
     from 0.5 on and 1e-5 absolute nearer, each colour byte within a step, both stages'
     coordinates within 1/4096; beyond the guard band of 8192 pixels its place within 1e-5 of its
     largest coordinate. Where the client hands the depth over (a sprite's rows, a particle's or a
     ground tile's corners: GFX_DEPTH_CORNERS), the shader takes it as it is.
     The CPU's floats are rounded too. Where the device projects the corner, one that is not
     within the tolerance of the CPU's is held to the exact value of the same arithmetic (in
     double) instead: within the tolerance, or twice the CPU's own distance from it, and the
     rounding any float computation of it may carry (exact_point: a few units in the last place of
     each sum's terms, carried through the division by the view depth, which for a point near the
     eye seen from a camera thousands of units away is the small sum of large terms). A snapped
     corner's place may lie a pixel from the CPU's within that, and its values, which its
     triangle's plane gives it, differ besides by how far that plane moves with the GPU's corners
     as made and by the rounding of the plane's evaluation (plane_slack, plane_bound). Such corners
     are counted.
     Counted apart, not compared: a corner the CPU puts behind the eye, outside the depth range
     (which clips it) or makes no number of, which draws nothing; a snapped corner whose plane is
     ill-conditioned (its triangle's longest edge squared more than 64 times its area, or a pixel's
     move longer than its shortest edge), whose values there are the rounding's; and a snapped
     corner on the pixel boundary next to the CPU's where the corner it moves from lies within
     reach of the snap's threshold, so that the GPU's own corner is on the other side of it. A
     triangle the GPU finds without area within the rounding of its corners moves none of them
     (instances.hlsli, no_area).
     On the CPU, which decides the lists' order and flatness, the rhw the lists sort sprites,
     particles, triangles and tiles by and whether a projected one is flat are the reference's to
     the bit.
   - Pictures through render/gfx_gpu.c itself: scenes of sprites drawn as the instances the client
     now makes them (from a layered texture, an array's layer) and as the reference's vertices (from
     a plain texture of the same texels), and of projected quads drawn as instances and as the
     vertices renderer_project_vertex makes, at the sample count asked for, within the captures'
     budget (tests/picture_budget.h).

   usage: test_quad_instances msaa [seed] [records per kind] [scenes]
   Exits 77 where no GPU device can be made. */
#include "actors/types.h"
#include "actors/sprite_render.h"
#include "actors/render_ops.h"
#include "actors/game_actor.h"
#include "render/renderer.h"
#include "render/render_api.h"
#include "render/projection.h"
#include "render/math3d.h"
#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "render/texture_types.h"
#include "render/quads.h"
#include "render/gfx.h"
#include "render/device.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "fileio/resource_api.h"
#include "effects/particle.h"
#include "game/context.h"
#include "render/texture_cache.h"
#include "std/string.h"
#include "diag/perf.h"
#include "render/shaders/shader_code.h"
#include "shaders/quad_probe.h"
#include "fixtures/quad_reference.h"
#include "picture_budget.h"
#include "effects/rag/rag_effect.h"
#include "actors/attr_state.h"
#include "actors/weather.h"
#include "resources/attr.h"
#include "world/ground.h"
#include "world/ground_api.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- what the client files reach ---- */

struct perf_counters g_perf;
struct Renderer *g_renderer;
DWORD g_fogColor;
float g_fogNear, g_fogFar;
int g_lightmap;
struct C3dDevice g_3dDevice;

int c3d_device_clear(struct C3dDevice *device, DWORD color) { (void)device; (void)color; return 0; }
int c3d_device_clear_z(struct C3dDevice *device) { (void)device; return 0; }
void c3d_device_restore_frame(struct C3dDevice *device) { (void)device; }
struct C3dGround15 *game_current_ground(void) { return NULL; }
int game_world_to_cell(float x, float z, int *cell_x, int *cell_y)
{
    (void)x; (void)z; *cell_x = *cell_y = 0;
    return 0;
}
int c3d_ground15_get_vertex_colour(struct C3dGround15 *self, int ax, int ay, float *r, float *g, float *b)
{
    (void)self; (void)ax; (void)ay; *r = *g = *b = 1.0f;
    return 0;
}

static struct Texture g_texture;
static struct SpriteRegion g_region;

/* The instances the client files queue (renderer_add_prim), as queued, with the rhw of the first
   two corners the lists sort them by */
#define QUEUE_MOST 16
static struct TexDrawPrim g_slots[QUEUE_MOST];
static struct Queued {
    struct GfxQuad quad;
    float rhw[2];
} g_queued[QUEUE_MOST];
static int g_slotCount, g_queuedCount;

struct Texture *sprite_atlas_resolve(struct Renderer *renderer, const struct SprImg *image, const DWORD *palette,
                                     struct SpriteRegion *region)
{
    (void)renderer; (void)image; (void)palette;
    *region = g_region;
    return &g_texture;
}

struct TexDrawPrim *renderer_new_prim_instance(struct Renderer *self)
{
    struct TexDrawPrim *p;
    (void)self;
    if (g_slotCount >= QUEUE_MOST)
        abort();
    p = &g_slots[g_slotCount++];
    memset(p, 0, sizeof *p);
    p->base.instance = &p->instance;
    return p;
}

struct TexDrawPrim *renderer_new_triangle_instance(struct Renderer *self)
{
    return renderer_new_prim_instance(self);
}

/* the corner sets the effect primitives' triangles share, as queued */
static struct GfxCornerSet g_sets[QUEUE_MOST];
static int g_setCount;

struct GfxCornerSet *renderer_new_corner_set(struct Renderer *self)
{
    struct GfxCornerSet *set;
    (void)self;
    if (g_setCount >= QUEUE_MOST)
        abort();
    set = &g_sets[g_setCount++];
    memset(set, 0, sizeof *set);
    set->row = -1;
    return set;
}

struct TexDrawPrim *rp_instance_construct(struct TexDrawPrim *self)
{
    memset(self, 0, sizeof *self);
    self->base.verts = self->quad;
    self->base.nverts = 4;
    self->base.instance = &self->instance;
    return self;
}

void renderer_add_prim(struct Renderer *self, struct RPQuad *prim, int flags)
{
    (void)self; (void)flags;
    if (!prim->instance || g_queuedCount >= QUEUE_MOST)
        abort();
    g_queued[g_queuedCount].quad = *prim->instance;
    g_queued[g_queuedCount].rhw[0] = prim->rhw[0];
    g_queued[g_queuedCount].rhw[1] = prim->rhw[1];
    g_queuedCount++;
}

static void queue_clear(void)
{
    g_slotCount = g_queuedCount = g_setCount = 0;
}

/* the camera particles see (game/context.h) */
static float g_view[12], g_heading, g_pitch;

int game_copy_view_matrix(float matrix[12])
{
    memcpy(matrix, g_view, sizeof g_view);
    return 1;
}
float game_camera_heading(void) { return g_heading; }
float game_camera_pitch(void) { return g_pitch; }

/* what a sprite particle draws (resources/animation.h): its motion's clips */
static struct Motion g_motion;
static struct SprClip g_clips[3];
static struct SprImg g_images[3];
static struct SprRes g_spr;
static int g_act;

struct Motion *act_res_get_motion(const struct ActRes *self, unsigned action, unsigned motion)
{
    (void)self; (void)action; (void)motion;
    return &g_motion;
}
struct SprClip *motion_get_clip(const struct Motion *self, unsigned index)
{
    return index < self->clip_count ? &self->clips[index] : NULL;
}

/* what the particle files reach and these cases never do */
struct TexCache g_textureCache;
struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int keyed, int full_size)
{ (void)cache; (void)name; (void)keyed; (void)full_size; abort(); }
void string_free(struct String *s) { (void)s; abort(); }
struct ResMgr *g_resMgr(void) { abort(); }
void *res_mgr_get(struct ResMgr *self, const char *name) { (void)self; (void)name; abort(); }
void res_lock(struct Res *self) { (void)self; abort(); }
void res_unlock(struct Res *self) { (void)self; abort(); }
unsigned actres_frame_count(const struct ActRes *self, unsigned action) { (void)self; (void)action; abort(); }

/* the ground's cell tiles (world/ground_draw.c): the cell's quad and the marker's heights the cases
   set, and what its water and casters reach, which these cases never do */
static float g_cellQuad[12];
static struct AttrCell g_cell;
struct Weather g_weather;
int g_playerBlind;
const float g_naviCellUVs[8] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
BYTE c3d_ground15_get_attr_quad(struct C3dGround15 *self, int ax, int ay, float *quad)
{
    (void)self; (void)ax; (void)ay;
    memcpy(quad, g_cellQuad, sizeof g_cellQuad);
    return 1;
}
struct AttrCell *c3d_attr_get_cell(const struct C3dAttr *self, int x, int y)
{
    (void)self; (void)x; (void)y;
    return &g_cell;
}
int c3d_ground15_water_block(struct C3dGround15 *self, int x, int y) { (void)self; (void)x; (void)y; abort(); }
float ground_water_height(const struct GndWater *water, int64_t diagonal) { (void)water; (void)diagonal; abort(); }
void renderer_add_world_draw(struct Renderer *self, int queue, const struct RPWorldDraw *draw, struct Texture *texture,
                             float key)
{ (void)self; (void)queue; (void)draw; (void)texture; (void)key; abort(); }

/* ---- random numbers ---- */

static uint64_t g_rng;

static uint64_t next(void)
{
    uint64_t z = (g_rng += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

static float uniform(float lo, float hi)
{
    return lo + (hi - lo) * (float)((double)(next() >> 11) / 9007199254740992.0);
}

static int irange(int lo, int hi)
{
    return lo + (int)(next() % (uint64_t)(hi - lo + 1));
}

static int chance(int percent)
{
    return (int)(next() % 100) < percent;
}

static float float_of(uint32_t u)
{
    float f;
    memcpy(&f, &u, 4);
    return f;
}

static uint32_t bits(float f)
{
    uint32_t u;
    memcpy(&u, &f, 4);
    return u;
}

/* a float with any mantissa and a power of two from 2^lo to 2^hi, of either sign when `signs` */
static float spread(int lo, int hi, int signs)
{
    float f = float_of((uint32_t)(next() & 0x7fffff) | (uint32_t)(127 + irange(lo, hi)) << 23);
    return signs && chance(50) ? -f : f;
}

enum { KIND_REALISTIC, KIND_EDGES, KIND_FUZZ, KIND_COUNT };
static const char *const g_kindNames[KIND_COUNT] = {"realistic", "edges", "fuzz"};

/* a coordinate on or near a screen of `size` pixels, or anywhere */
static float coordinate(int kind, int size)
{
    if (kind == KIND_REALISTIC)
        return uniform(-0.25f, 1.25f) * (float)size;
    if (kind == KIND_EDGES) {
        static const float edges[] = {0.0f, -0.5f, 0.5f, -1.0f, 1.0f, 255.996f, 32767.5f, -32768.0f, 40000.25f};
        /* a hair under half a 256th of a pixel, whose 256 times plus a half rounds up to 1 in float,
           and its neighbours (instances.hlsli, pixel_boundary) */
        static const uint32_t halves[] = {0x3affffffu, 0xbaffffffu, 0x3b000000u, 0x3b000001u, 0x3afffffeu};
        if (chance(5))
            return float_of(halves[irange(0, 4)]);
        return chance(50) ? edges[irange(0, 8)] + (chance(50) ? (float)size : 0.0f)
                          : (float)irange(-2000, 4000) + (float)irange(0, 255) / 256.0f;
    }
    return chance(30) ? spread(-20, 20, 1) : uniform(-1e5f, 1e5f);
}

/* an rhw a sprite or a point on screen takes: 1 / depth for a depth in view, or anything */
static float some_rhw(int kind)
{
    if (kind == KIND_REALISTIC)
        return 1.0f / uniform(15.0f, 3000.0f);
    if (kind == KIND_EDGES) {
        static const float depths[] = {10.0f, 40000.0f, 10.000001f, 39999.996f, 300.0f, 1e-3f, -50.0f};
        return chance(50) ? 1.0f / depths[irange(0, 6)] : 1.0f / uniform(10.0f, 40000.0f);
    }
    return chance(80) ? spread(-20, 2, 1) : chance(50) ? 0.0f : 1.0f / uniform(-1000.0f, 50000.0f);
}

/* ---- the GPU's vertex stage, through tests/shaders/quad_probe.hlsl ---- */

#define SIDE 512
#define PER_BATCH (SIDE * SIDE / 4)

static SDL_GPUDevice *g_dev;
static SDL_GPUGraphicsPipeline *g_probe;
static SDL_GPUTexture *g_targets[3];
static SDL_GPUBuffer *g_corners, *g_identity, *g_offsets, *g_quads;

/* What a batch of records holds for the probe: the frame's quads as the device makes them, the
   records with the matrices and vertices they name, each on rows of four words; and the row each
   record starts at */
struct Batch {
    uint32_t *words, *offsets;
    size_t nwords, ninstances;
};

#define BATCH_WORDS (PER_BATCH * 64)

static SDL_GPUShader *shader(const struct GpuShaderCode *code)
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
    return SDL_CreateGPUShader(g_dev, &info);
}

static SDL_GPUBuffer *buffer(SDL_GPUBufferUsageFlags usage, Uint32 size)
{
    SDL_GPUBufferCreateInfo info;
    SDL_zero(info);
    info.usage = usage;
    info.size = size;
    return SDL_CreateGPUBuffer(g_dev, &info);
}

static void need(int ok, const char *what)
{
    if (!ok) {
        fprintf(stderr, "quad_instances: %s: %s\n", what, SDL_GetError());
        exit(1);
    }
}

static void probe_init(void)
{
    SDL_GPUShader *vs = shader(&quad_probe_vs), *fs = shader(&quad_probe_fs);
    SDL_GPUGraphicsPipelineCreateInfo info;
    SDL_GPUVertexBufferDescription vb[3];
    SDL_GPUVertexAttribute va[3];
    SDL_GPUColorTargetDescription ct[3];
    SDL_GPUTextureCreateInfo tex;
    int i;
    need(vs && fs, "probe shaders");
    SDL_zero(info);
    SDL_zero(vb);
    SDL_zero(va);
    SDL_zero(ct);
    for (i = 0; i < 3; i++) {
        vb[i].slot = (Uint32)i;
        vb[i].pitch = 4;
        vb[i].input_rate = i ? SDL_GPU_VERTEXINPUTRATE_INSTANCE : SDL_GPU_VERTEXINPUTRATE_VERTEX;
        va[i].location = (Uint32)i;
        va[i].buffer_slot = (Uint32)i;
        va[i].format = SDL_GPU_VERTEXELEMENTFORMAT_UINT;
    }
    for (i = 0; i < 3; i++)
        ct[i].format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
    info.vertex_shader = vs;
    info.fragment_shader = fs;
    info.vertex_input_state.vertex_buffer_descriptions = vb;
    info.vertex_input_state.num_vertex_buffers = 3;
    info.vertex_input_state.vertex_attributes = va;
    info.vertex_input_state.num_vertex_attributes = 3;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_POINTLIST;
    info.rasterizer_state.enable_depth_clip = true;
    info.target_info.color_target_descriptions = ct;
    info.target_info.num_color_targets = 3;
    g_probe = SDL_CreateGPUGraphicsPipeline(g_dev, &info);
    SDL_ReleaseGPUShader(g_dev, vs);
    SDL_ReleaseGPUShader(g_dev, fs);
    need(g_probe != NULL, "probe pipeline");
    SDL_zero(tex);
    tex.type = SDL_GPU_TEXTURETYPE_2D;
    tex.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_FLOAT;
    tex.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    tex.width = tex.height = SIDE;
    tex.layer_count_or_depth = 1;
    tex.num_levels = 1;
    for (i = 0; i < 3; i++) {
        g_targets[i] = SDL_CreateGPUTexture(g_dev, &tex);
        need(g_targets[i] != NULL, "probe targets");
    }
    g_corners = buffer(SDL_GPU_BUFFERUSAGE_VERTEX, 16);
    g_identity = buffer(SDL_GPU_BUFFERUSAGE_VERTEX, PER_BATCH * 4);
    g_offsets = buffer(SDL_GPU_BUFFERUSAGE_VERTEX, PER_BATCH * 4);
    g_quads = buffer(SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ, BATCH_WORDS * 4);
    need(g_corners && g_identity && g_offsets && g_quads, "probe buffers");
}

static void probe_free(void)
{
    int i;
    for (i = 0; i < 3; i++)
        SDL_ReleaseGPUTexture(g_dev, g_targets[i]);
    SDL_ReleaseGPUBuffer(g_dev, g_corners);
    SDL_ReleaseGPUBuffer(g_dev, g_identity);
    SDL_ReleaseGPUBuffer(g_dev, g_offsets);
    SDL_ReleaseGPUBuffer(g_dev, g_quads);
    SDL_ReleaseGPUGraphicsPipeline(g_dev, g_probe);
}

/* Runs the batch's instances through the probe; out gets each vertex's 12 floats, quad_vertex's
   (4 vertices an instance): x, y, z, rhw; the colour; both stages' coordinates. */
#define PROBE_FLOATS 12

static void probe_run(const struct Batch *b, float *out)
{
    Uint32 n = (Uint32)b->ninstances, targetBytes = SIDE * SIDE * 16;
    Uint32 sizes[4], offsets[4], total = 0, i;
    SDL_GPUTransferBufferCreateInfo tinfo;
    SDL_GPUTransferBuffer *up, *down;
    SDL_GPUBuffer *dests[4];
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    SDL_GPUFence *fence;
    SDL_GPUColorTargetInfo ct[3];
    SDL_GPURenderPass *rp;
    SDL_GPUBufferBinding bind[3];
    Uint32 probe[4];
    float step = 2.0f / (float)SIDE;
    unsigned char *p;
    int t;
    sizes[0] = 16;
    sizes[1] = PER_BATCH * 4;
    sizes[2] = n * 4;
    sizes[3] = (Uint32)(b->nwords ? b->nwords : 4) * 4;
    dests[0] = g_corners;
    dests[1] = g_identity;
    dests[2] = g_offsets;
    dests[3] = g_quads;
    for (i = 0; i < 4; i++) {
        offsets[i] = total;
        total += (sizes[i] + 15) & ~15u;
    }
    SDL_zero(tinfo);
    tinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tinfo.size = total;
    up = SDL_CreateGPUTransferBuffer(g_dev, &tinfo);
    tinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    tinfo.size = targetBytes * 3;
    down = SDL_CreateGPUTransferBuffer(g_dev, &tinfo);
    need(up && down, "transfer buffers");
    p = SDL_MapGPUTransferBuffer(g_dev, up, false);
    memset(p, 0, total);
    for (i = 0; i < 4; i++)
        ((Uint32 *)(p + offsets[0]))[i] = i;
    for (i = 0; i < PER_BATCH; i++)
        ((Uint32 *)(p + offsets[1]))[i] = i;
    memcpy(p + offsets[2], b->offsets, (size_t)n * 4);
    memcpy(p + offsets[3], b->words, b->nwords * 4);
    SDL_UnmapGPUTransferBuffer(g_dev, up);

    cmd = SDL_AcquireGPUCommandBuffer(g_dev);
    copy = SDL_BeginGPUCopyPass(cmd);
    for (i = 0; i < 4; i++) {
        SDL_GPUTransferBufferLocation src;
        SDL_GPUBufferRegion dst;
        src.transfer_buffer = up;
        src.offset = offsets[i];
        dst.buffer = dests[i];
        dst.offset = 0;
        dst.size = sizes[i];
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    }
    SDL_EndGPUCopyPass(copy);
    SDL_zero(ct);
    for (i = 0; i < 3; i++) {
        ct[i].texture = g_targets[i];
        ct[i].load_op = SDL_GPU_LOADOP_CLEAR;
        ct[i].store_op = SDL_GPU_STOREOP_STORE;
        ct[i].cycle = true;
    }
    rp = SDL_BeginGPURenderPass(cmd, ct, 3, NULL);
    SDL_BindGPUGraphicsPipeline(rp, g_probe);
    bind[0].buffer = g_corners;
    bind[1].buffer = g_offsets;
    bind[2].buffer = g_identity;
    bind[0].offset = bind[1].offset = bind[2].offset = 0;
    SDL_BindGPUVertexBuffers(rp, 0, bind, 3);
    SDL_BindGPUVertexStorageBuffers(rp, 0, &g_quads, 1);
    probe[0] = SIDE;
    memcpy(&probe[1], &step, 4);
    probe[2] = probe[3] = 0;
    SDL_PushGPUVertexUniformData(cmd, 0, probe, sizeof probe);
    SDL_DrawGPUPrimitives(rp, 4, n, 0, 0);
    SDL_EndGPURenderPass(rp);
    copy = SDL_BeginGPUCopyPass(cmd);
    for (i = 0; i < 3; i++) {
        SDL_GPUTextureRegion src;
        SDL_GPUTextureTransferInfo dst;
        SDL_zero(src);
        SDL_zero(dst);
        src.texture = g_targets[i];
        src.w = src.h = SIDE;
        src.d = 1;
        dst.transfer_buffer = down;
        dst.offset = i * targetBytes;
        SDL_DownloadFromGPUTexture(copy, &src, &dst);
    }
    SDL_EndGPUCopyPass(copy);
    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    need(fence && SDL_WaitForGPUFences(g_dev, true, &fence, 1), "probe submit");
    SDL_ReleaseGPUFence(g_dev, fence);
    p = SDL_MapGPUTransferBuffer(g_dev, down, false);
    for (i = 0; i < n * 4; i++)
        for (t = 0; t < 3; t++)
            memcpy(out + (size_t)i * PROBE_FLOATS + t * 4, p + (size_t)t * targetBytes + (size_t)i * 16, 16);
    SDL_UnmapGPUTransferBuffer(g_dev, down);
    SDL_ReleaseGPUTransferBuffer(g_dev, up);
    SDL_ReleaseGPUTransferBuffer(g_dev, down);
}

static void batch_clear(struct Batch *b)
{
    b->nwords = b->ninstances = 0;
}

/* room for a few more cases: their records, a matrix each and their vertices */
static int batch_room(const struct Batch *b)
{
    return b->ninstances + 8 < PER_BATCH && b->nwords + 8 * (QUAD_WORDS_MAX + 16 + 32) < BATCH_WORDS;
}

/* n words at the next row; answers the row they start at */
static uint32_t batch_rows(struct Batch *b, const void *words, size_t n)
{
    uint32_t row = (uint32_t)(b->nwords / 4);
    memcpy(b->words + b->nwords, words, n * 4);
    memset(b->words + b->nwords + n, 0, (size_t)((4 - n % 4) % 4) * 4);
    b->nwords += (n + 3) / 4 * 4;
    return row;
}

/* a record: the row it starts at among the instances */
static void batch_record(struct Batch *b, const uint32_t *words, int n)
{
    b->offsets[b->ninstances++] = batch_rows(b, words, (size_t)n);
}

/* A case's record twice, as the device gives it: as made, and as a flat draw's in a multisampled
   pass gets it (QUAD_SNAP), where it can (`snap`) */
static void batch_case(struct Batch *b, uint32_t *words, int n, int snap)
{
    batch_record(b, words, n);
    if (snap)
        words[0] |= 32u;
    batch_record(b, words, n);
}

/* ---- the comparison ---- */

/* the same float, bit for bit, or both not a number: what the CPU decides from its own floats */
static int same(float a, float b)
{
    return bits(a) == bits(b) || (a != a && b != b);
}

/* What a vertex should come back as: x, y, z, rhw; the colour as Direct3D 9 made floats of its
   bytes (r, g, b, a, each byte / 255); u0, v0, u1, v1 */
struct Expect {
    float v[12];
};

static void expect_of(const struct ReferenceVertex *r, struct Expect *e)
{
    e->v[0] = r->x;
    e->v[1] = r->y;
    e->v[2] = r->z;
    e->v[3] = r->rhw;
    e->v[4] = (float)r->bytes[2] / 255.0f;
    e->v[5] = (float)r->bytes[1] / 255.0f;
    e->v[6] = (float)r->bytes[0] / 255.0f;
    e->v[7] = (float)r->bytes[3] / 255.0f;
    e->v[8] = r->u0;
    e->v[9] = r->v0;
    e->v[10] = r->u1;
    e->v[11] = r->v1;
}

/* One record and what each of its four corners should come back as, made and snapped; whether it
   is a triangle (its corner 3 is its corner 2); and what its corners are without the rounding of
   the C's floats */
struct Case {
    struct Expect made[4], snapped[4];
    int corners;
    double exact[4][4];  /* x, y, z and rhw as made without rounding where the device projects
                            the corner (exact_corners), else the CPU's own */
    double bound[4][4];  /* how far a float computation of them may lie from those: 0 where the CPU's
                            are the exact ones */
};

/* the tolerance (plans/gpu-renderer.md, Decisions) and the guard band within which a corner's place
   is held to pixels */
#define PIXEL_TOLERANCE (1.0 / 16)
#define RELATIVE_TOLERANCE 1e-5
#define UV_TOLERANCE (1.0 / 4096)
#define GUARD_BAND 8192.0
/* four units in the last place of a float, relative: what each of a few float operations may round */
#define ROUNDING (4.0 / 16777216.0)

/* Per kind of record: the largest differences, as made ([0]) and snapped ([1]), and the corners
   counted apart */
struct Tally {
    long instances, vertices, printed[2], keys, flat;
    long beyond[2];      /* corners beyond the tolerance */
    long undrawable[2];  /* the CPU's corner behind the eye, outside the depth range or not a number:
                            nothing drawn, not compared */
    long bounded[2];     /* beyond the tolerance of the CPU's, within the rounding of the exact value */
    long illPlane;       /* snapped from an ill-conditioned plane: not compared */
    long off[2];         /* beyond the guard band: held to RELATIVE_TOLERANCE of its largest coordinate */
    long across;         /* snapped onto the boundary next to the CPU's, from a corner within the
                            tolerance of the snap's threshold */
    double pixel[2], rhw[2], depth[2], nearDepth[2], uv[2], offRelative[2];
    int colour[2];
};

static double relative(double got, double want)
{
    double scale = fabs(want) > 1e-30 ? fabs(want) : 1e-30;
    return fabs(got - want) / scale;
}

static void most(double *into, double d)
{
    if (d > *into || d != d)
        *into = d;
}

/* The triangle a snapped corner of the case takes its plane from (instances.hlsli, snapped; the
   reference's snap_flat): (0, 1, 2) for corners 0 to 2, (2, 1, 3) for corner 3 and for corners 1
   and 2 where the CPU found the first without area, of the CPU's own corners as made, into k.
   Answers whether its plane is well-conditioned: its longest edge squared at most 64 times its
   area, and its shortest edge no shorter than the pixel a corner moves at most. */
static int snap_triangle(const struct Case *cs, int corner, int k[3])
{
    const float *a, *b, *c;
    double dx1, dy1, dx2, dy2, det, e1, e2, e3, longest, shortest;
    float fx1, fy1, fx2, fy2, fdet;
    k[0] = corner == 3 ? 2 : 0;
    k[1] = 1;
    k[2] = corner == 3 ? 3 : 2;
    a = cs->made[k[0]].v;
    b = cs->made[k[1]].v;
    c = cs->made[k[2]].v;
    fx1 = b[0] - a[0];
    fy1 = b[1] - a[1];
    fx2 = c[0] - a[0];
    fy2 = c[1] - a[1];
    fdet = fx1 * fy2 - fx2 * fy1;
    if (fdet > -1e-6f && fdet < 1e-6f && (corner == 1 || corner == 2) && cs->corners == 4) {
        k[0] = 2;
        k[2] = 3;
        a = cs->made[2].v;
        c = cs->made[3].v;
    }
    dx1 = (double)b[0] - a[0];
    dy1 = (double)b[1] - a[1];
    dx2 = (double)c[0] - a[0];
    dy2 = (double)c[1] - a[1];
    det = fabs(dx1 * dy2 - dx2 * dy1);
    e1 = dx1 * dx1 + dy1 * dy1;
    e2 = dx2 * dx2 + dy2 * dy2;
    e3 = (dx2 - dx1) * (dx2 - dx1) + (dy2 - dy1) * (dy2 - dy1);
    longest = e1 > e2 ? e1 : e2;
    longest = e3 > longest ? e3 : longest;
    shortest = e1 < e2 ? e1 : e2;
    shortest = e3 < shortest ? e3 : shortest;
    return det > 0.0 && longest <= 64.0 * det && shortest >= 1.0;
}

/* How far value f (of the probe's 12) of triangle k's plane, of the CPU's corners as made, moves at
   (px, py) when the corners move by up to `shift` pixels and their values by up to `value`: the
   magnitudes of the point's barycentric weights times the gradient times the shift and the value's
   move, to first order. */
static double plane_slack(const struct Case *cs, const int k[3], int f, double px, double py, double shift,
                          double value)
{
    const float *a = cs->made[k[0]].v, *b = cs->made[k[1]].v, *c = cs->made[k[2]].v;
    double dx1 = (double)b[0] - a[0], dy1 = (double)b[1] - a[1], dx2 = (double)c[0] - a[0];
    double dy2 = (double)c[1] - a[1], det = dx1 * dy2 - dx2 * dy1, ox = px - a[0], oy = py - a[1];
    double d1 = (double)b[f] - a[f], d2 = (double)c[f] - a[f];
    double gx = (d1 * dy2 - d2 * dy1) / det, gy = (d2 * dx1 - d1 * dx2) / det;
    double l1 = (ox * dy2 - oy * dx2) / det, l2 = (oy * dx1 - ox * dy1) / det;
    return ((fabs(gx) + fabs(gy)) * shift + value) * (fabs(1.0 - l1 - l2) + fabs(l1) + fabs(l2));
}

/* How far a float evaluation of value f of triangle k's plane at (px, py) may lie from the exact
   one: ROUNDING of the values its corners weigh in (the magnitudes of the point's barycentric
   weights), times the corners' largest coordinate over the triangle's shortest edge, by which the
   rounding of a coordinate moves the weights. */
static double plane_bound(const struct Case *cs, const int k[3], int f, double px, double py)
{
    const float *a = cs->made[k[0]].v, *b = cs->made[k[1]].v, *c = cs->made[k[2]].v;
    double dx1 = (double)b[0] - a[0], dy1 = (double)b[1] - a[1], dx2 = (double)c[0] - a[0];
    double dy2 = (double)c[1] - a[1], det = dx1 * dy2 - dx2 * dy1, ox = px - a[0], oy = py - a[1];
    double l1 = (ox * dy2 - oy * dx2) / det, l2 = (oy * dx1 - ox * dy1) / det;
    double e1 = sqrt(dx1 * dx1 + dy1 * dy1), e2 = sqrt(dx2 * dx2 + dy2 * dy2);
    double e3 = sqrt((dx2 - dx1) * (dx2 - dx1) + (dy2 - dy1) * (dy2 - dy1)), shortest = fmin(e1, fmin(e2, e3));
    double corners = fmax(fmax(fmax(fabs(a[0]), fabs(a[1])), fmax(fabs(b[0]), fabs(b[1]))),
                          fmax(fabs(c[0]), fabs(c[1])));
    double largest = fmax(fmax(fabs(px), fabs(py)), corners);
    double values = fmax(fabs(a[f]), fmax(fabs(b[f]), fabs(c[f])));
    return ROUNDING * values * (fabs(1.0 - l1 - l2) + fabs(l1) + fabs(l2)) * (1.0 + largest / shortest);
}

/* value f of triangle k's plane, of the CPU's corners as made, at (px, py), in double */
static double plane_exact(const struct Case *cs, const int k[3], int f, double px, double py)
{
    const float *a = cs->made[k[0]].v, *b = cs->made[k[1]].v, *c = cs->made[k[2]].v;
    double dx1 = (double)b[0] - a[0], dy1 = (double)b[1] - a[1], dx2 = (double)c[0] - a[0];
    double dy2 = (double)c[1] - a[1], det = dx1 * dy2 - dx2 * dy1, ox = px - a[0], oy = py - a[1];
    double d1 = (double)b[f] - a[f], d2 = (double)c[f] - a[f];
    return a[f] + (d1 * dy2 - d2 * dy1) / det * ox + (d2 * dx1 - d1 * dx2) / det * oy;
}

/* whether the snap of a corner at v (instances.hlsli, pixel_boundary: v to 256ths rounding halves
   up, then the boundary before the next pixel centre) changes within `reach` of v: a whole pixel
   less one 512th, k - 1/512, lies that near */
static int near_threshold(double v, double reach)
{
    double t = floor(v + 1.0 / 512) - 1.0 / 512, u = t + 1.0;
    return fabs(v - t) <= reach || fabs(v - u) <= reach;
}

/* Whether `gpu` keeps the tolerance `tolerance` of `want`, the CPU's value (0), or, where `exact`
   is the value without rounding, `bound` of it (the rounding a float computation of it may carry)
   and the tolerance, or twice the CPU's own distance from it if that is more (1); `slack` more
   besides either way. Answers 2 where neither holds. */
static int within(double gpu, double want, double exact, double bound, double tolerance, double slack)
{
    double own = 2.0 * fabs(want - exact);
    if (fabs(gpu - want) <= tolerance + slack)
        return 0;
    return fabs(gpu - exact) <= (tolerance > own ? tolerance : own) + bound + slack ? 1 : 2;
}

/* The probe's 12 floats of corner `corner` of the case, as made (mode 0) or snapped (1), against
   what it should be; `made` the probe's floats of the case's four corners as made (within: the
   exact value where the device projects the corner; a snapped corner's values from its plane
   besides by how far that plane moves with the GPU's corners as made, plane_slack). Answers
   whether it is within the tolerance. */
static int compare(struct Tally *t, int mode, const float *got, const float *made, const struct Case *cs, int corner,
                   const char *what)
{
    const float *cpu = cs->made[corner].v, *w = mode ? cs->snapped[corner].v : cpu;
    double exact[12], bound[12], pixel = 0.0, uv = 0.0, shift = 0.0, slack[12];
    int f, colour = 0, bad = 0, drawable, moved, k[3], i, how = 0;
    drawable = w[3] == w[3] && w[3] > 0.0f && w[3] < 3.4e38f && w[0] == w[0] && w[1] == w[1] && w[2] == w[2]
               && fabs(w[0]) < 3.4e38 && fabs(w[1]) < 3.4e38 && w[2] >= 0.0f && w[2] <= 1.0f;
    if (!drawable) {
        t->undrawable[mode]++;
        return 1;
    }
    for (f = 0; f < 12; f++) {
        exact[f] = f < 4 && !mode ? cs->exact[corner][f] : w[f];
        bound[f] = f < 4 && !mode ? cs->bound[corner][f] : 0.0;
        slack[f] = 0.0;
    }
    moved = mode && (got[0] != cpu[0] || got[1] != cpu[1] || w[0] != cpu[0] || w[1] != cpu[1]);
    if (mode) {
        /* a snapped corner's place is the pixel boundary at most a pixel from its own place, its rhw
           its own */
        for (f = 0; f < 2; f++) {
            exact[f] = cs->exact[corner][f];
            bound[f] = cs->bound[corner][f] + 1.0;
        }
        exact[3] = cs->exact[corner][3];
        bound[3] = cs->bound[corner][3];
        if (!moved) {
            exact[2] = cs->exact[corner][2];
            bound[2] = cs->bound[corner][2];
        }
    }
    if (moved) {
        double reach;
        if (!snap_triangle(cs, corner, k)) {
            t->illPlane++;
            return 1;
        }
        for (i = 0; i < 3; i++) {
            const float *g = made + k[i] * PROBE_FLOATS, *c = cs->made[k[i]].v;
            double d = fabs((double)g[0] - c[0]) > fabs((double)g[1] - c[1]) ? fabs((double)g[0] - c[0])
                                                                                : fabs((double)g[1] - c[1]);
            shift = d > shift ? d : shift;
        }
        /* the plane's values move with the GPU's corners and with the GPU's boundary, where that
           lies elsewhere within the bound of the place */
        for (f = 2; f < 12; f++)
            if (f != 3) {
                double moveX = fabs((double)got[0] - w[0]), moveY = fabs((double)got[1] - w[1]), value = 0.0;
                for (i = 0; i < 3; i++)
                    value = fmax(value, fabs((double)made[k[i] * PROBE_FLOATS + f] - cs->made[k[i]].v[f]));
                slack[f] = (plane_slack(cs, k, f, w[0], w[1], shift, value)
                            + fabs(plane_exact(cs, k, f, w[0] + 1.0, w[1]) - plane_exact(cs, k, f, w[0], w[1])) * moveX
                            + fabs(plane_exact(cs, k, f, w[0], w[1] + 1.0) - plane_exact(cs, k, f, w[0], w[1])) * moveY)
                           * (f >= 4 && f < 8 ? 255.0 : 1.0);
                exact[f] = plane_exact(cs, k, f, w[0], w[1]);
                bound[f] = plane_bound(cs, k, f, w[0], w[1]) * (f >= 4 && f < 8 ? 255.0 : 1.0);
            }
        reach = PIXEL_TOLERANCE + fmax(fabs(cpu[0] - cs->exact[corner][0]), fabs(cpu[1] - cs->exact[corner][1]))
                + fmax(cs->bound[corner][0], cs->bound[corner][1]);
        if ((fabs((double)got[0] - w[0]) == 1.0 && near_threshold(cpu[0], reach))
            || (fabs((double)got[1] - w[1]) == 1.0 && near_threshold(cpu[1], reach))) {
            /* the GPU's corner on the other side of the snap's threshold: the values its plane gives
               there are another point's */
            t->across++;
            return 1;
        }
    }
    for (f = 4; f < 8; f++) {
        int d = abs((int)floor(got[f] * 255.0 + 0.5) - (int)floor(w[f] * 255.0 + 0.5));
        if (got[f] != got[f])
            d = 255;
        if (d > colour)
            colour = d;
        i = within(got[f] * 255.0, floor(w[f] * 255.0 + 0.5), exact[f] * 255.0, bound[f], 1.0, slack[f]);
        how = i > how ? i : how;
    }
    for (f = 8; f < 12; f++) {
        most(&uv, fabs((double)got[f] - w[f]));
        i = within(got[f], w[f], exact[f], bound[f], UV_TOLERANCE, slack[f]);
        how = i > how ? i : how;
    }
    if (fabs(w[0]) <= GUARD_BAND && fabs(w[1]) <= GUARD_BAND) {
        most(&pixel, fabs((double)got[0] - w[0]));
        most(&pixel, fabs((double)got[1] - w[1]));
        most(&t->pixel[mode], pixel);
        for (f = 0; f < 2; f++) {
            i = within(got[f], w[f], exact[f], bound[f], PIXEL_TOLERANCE, 0.0);
            how = i > how ? i : how;
        }
    } else {
        double scale = fabs(w[0]) > fabs(w[1]) ? fabs(w[0]) : fabs(w[1]);
        double r = (fabs((double)got[0] - w[0]) > fabs((double)got[1] - w[1]) ? fabs((double)got[0] - w[0])
                                                                                : fabs((double)got[1] - w[1])) / scale;
        t->off[mode]++;
        most(&t->offRelative[mode], r);
        for (f = 0; f < 2; f++) {
            i = within(got[f], w[f], exact[f], bound[f], RELATIVE_TOLERANCE * scale, 0.0);
            how = i > how ? i : how;
        }
    }
    most(&t->rhw[mode], relative(got[3], w[3]));
    i = within(got[3], w[3], exact[3], bound[3], RELATIVE_TOLERANCE * fabs(w[3]), 0.0);
    how = i > how ? i : how;
    if (fabs(w[2]) >= 0.5) {
        most(&t->depth[mode], relative(got[2], w[2]));
        i = within(got[2], w[2], exact[2], bound[2], RELATIVE_TOLERANCE * fabs(w[2]), slack[2]);
    } else {
        most(&t->nearDepth[mode], fabs((double)got[2] - w[2]));
        i = within(got[2], w[2], exact[2], bound[2], RELATIVE_TOLERANCE, slack[2]);
    }
    how = i > how ? i : how;
    bad = how == 2;
    t->bounded[mode] += how == 1;
    most(&t->uv[mode], uv);
    if (colour > t->colour[mode])
        t->colour[mode] = colour;
    if (bad) {
        t->beyond[mode]++;
        if (t->printed[mode] < 6) {
            t->printed[mode]++;
            printf("  %s, %s: cpu", what, mode ? "snapped" : "as made");
            for (f = 0; f < 12; f++)
                printf(" %.9g", w[f]);
            printf("; gpu");
            for (f = 0; f < 12; f++)
                printf(" %.9g", got[f]);
            printf("; exact %.9g %.9g %.9g %.9g\n", cs->exact[corner][0], cs->exact[corner][1], cs->exact[corner][2],
                   cs->exact[corner][3]);
        }
    }
    return !bad;
}

static int report(const char *name, const struct Tally *t)
{
    int m, clean = !t->keys && !t->flat && !t->beyond[0] && !t->beyond[1];
    printf("%s: %ld records, %ld vertices; sort keys differ %ld, flatness %ld;", name, t->instances, t->vertices,
           t->keys, t->flat);
    for (m = 0; m < 2; m++)
        printf(" %s: pixel %.3g, rhw %.3g, depth %.3g (nearer %.3g), uv %.3g, colour %d, off the guard band %ld "
               "(%.3g), not drawn %ld, within the rounding of the exact value %ld, beyond %ld;",
               m ? "snapped" : "as made", t->pixel[m], t->rhw[m], t->depth[m], t->nearDepth[m], t->uv[m], t->colour[m],
               t->off[m], t->offRelative[m], t->undrawable[m], t->bounded[m], t->beyond[m]);
    printf(" ill-conditioned planes %ld, across a snap's threshold %ld\n", t->illPlane, t->across);
    return clean;
}

/* ---- cases ---- */

/* the reference's vertices of a primitive, made and snapped as a flat draw's in a multisampled
   pass: a strip's triangles (0, 1, 2) and (2, 1, 3), a triangle's (0, 1, 2) */
static void expect_primitive(const struct ReferenceVertex *in, int corners, struct Case *c)
{
    static const unsigned short strip[6] = {0, 1, 2, 2, 1, 3};
    struct ReferenceVertex moved[4];
    int i;
    memcpy(moved, in, (size_t)corners * sizeof *in);
    reference_snap_flat(moved, corners, corners == 4 ? strip : NULL, corners == 4 ? 6 : 3);
    c->corners = corners;
    for (i = 0; i < 4; i++) {
        int k = i < corners ? i : 2, f;
        expect_of(&in[k], &c->made[i]);
        expect_of(&moved[k], &c->snapped[i]);
        for (f = 0; f < 4; f++) {
            c->exact[i][f] = c->made[i].v[f];
            c->bound[i][f] = 0.0;
        }
    }
}

/* the rhw the lists sort a queued instance by against those of the vertices it stands for */
static void check_keys(struct Tally *t, const struct Queued *q, const struct TexDrawVertex *v)
{
    if (bits(q->rhw[0]) != bits(v[0].rhw) || bits(q->rhw[1]) != bits(v[1].rhw)) {
        t->keys++;
        if (t->printed[0]++ < 6)
            printf("  sort keys: cpu %.9g %.9g, instance %.9g %.9g\n", v[0].rhw, v[1].rhw, q->rhw[0], q->rhw[1]);
    }
}

/* A sprite as the client makes it now: render_object_submit_spr_quad's instance, against its
   vertices as it made them; answers how many cases it made */
static int sprite_case(int kind, struct Batch *b, struct Case *c, struct Tally *t)
{
    struct RenderObject obj;
    struct SprImg img;
    struct SqQuad quad;
    struct QuadHeader h = {0, 0, 0, 0};
    struct TexDrawVertex v[4];
    struct ReferenceVertex r[4];
    uint32_t words[QUAD_WORDS_MAX];
    float x = 0.0f, y = 0.0f, w, hgt;
    int palette = chance(10) ? 0xd77b : 0x1234, i, n;

    memset(&obj, 0, sizeof obj);
    memset(&img, 0, sizeof img);
    memset(&quad, 0, sizeof quad);
    quad.image = &img;
    quad.x0 = coordinate(kind, 1600);
    quad.top = coordinate(kind, 900);
    w = kind == KIND_FUZZ ? uniform(-5000.0f, 5000.0f) : uniform(0.0f, 400.0f);
    hgt = kind == KIND_FUZZ ? uniform(-5000.0f, 5000.0f) : uniform(0.0f, 400.0f);
    quad.x1 = kind == KIND_EDGES && chance(20) ? quad.x0 : quad.x0 + w;
    quad.bottom = kind == KIND_EDGES && chance(20) ? quad.top : quad.top + hgt;
    quad.depth_b = some_rhw(kind);
    quad.depth_a = chance(30) ? quad.depth_b
                 : quad.depth_b + (kind == KIND_FUZZ ? spread(-30, 0, 1) : uniform(-1e-5f, 1e-4f));
    quad.color = (unsigned)next();
    if (chance(kind == KIND_REALISTIC ? 15 : 40)) {
        quad.angle = chance(30) ? 180.0f : (float)irange(1, 359);
        if (chance(50)) {
            x = coordinate(kind, 1600) - quad.x0;
            y = coordinate(kind, 900) - quad.top;
        }
    }
    g_region.u = uniform(0.0f, 0.9f);
    g_region.v = uniform(0.0f, 0.9f);
    g_region.u1 = g_region.u + uniform(0.0f, 0.1f);
    g_region.v1 = g_region.v + uniform(0.0f, 0.1f);
    queue_clear();
    render_object_submit_spr_quad(&obj, &quad, &palette, 0x101, x, y);
    if (g_queuedCount != 1) {
        fprintf(stderr, "quad_instances: submit drew %d quads\n", g_queuedCount);
        exit(1);
    }
    n = quad_words(&g_queued[0].quad, &h, words);
    batch_case(b, words, n, 1);
    reference_sprite_vertices(&quad, &g_region, palette, x, y, v);
    check_keys(t, &g_queued[0], v);
    for (i = 0; i < 4; i++)
        reference_widen(&v[i], &r[i]);
    expect_primitive(r, 4, c);
    return 1;
}

/* A primitive of pre-transformed vertices: a rectangle, a strip or a triangle, flat or not;
   answers how many cases it made */
static int screen_case(int kind, struct Batch *b, struct Case *c)
{
    struct QuadHeader h = {0, 0, 0, 0};
    struct TexDrawVertex v[4];
    struct ReferenceVertex r[4];
    uint32_t words[QUAD_WORDS_MAX];
    int shape = irange(0, 2), corners = shape == 2 ? 3 : 4, flat = chance(70), i, n;
    float rhw = some_rhw(kind);
    for (i = 0; i < 4; i++) {
        v[i].x = coordinate(kind, 1600);
        v[i].y = coordinate(kind, 900);
        v[i].z = kind == KIND_FUZZ ? spread(-20, 2, 1) : uniform(0.0f, 1.0f);
        v[i].rhw = flat ? rhw : some_rhw(kind);
        v[i].diffuse = i && chance(50) ? v[0].diffuse : (uint32_t)next();
        v[i].u = uniform(-1.0f, 2.0f);
        v[i].v = uniform(-1.0f, 2.0f);
    }
    if (shape == 0) {
        /* a rectangle with one rhw and colour, z by row and texture coordinates by column and row */
        v[2].x = v[0].x;
        v[3].x = v[1].x;
        v[1].y = v[0].y;
        v[3].y = v[2].y;
        v[1].z = v[0].z;
        v[3].z = v[2].z;
        v[1].rhw = v[2].rhw = v[3].rhw = v[0].rhw;
        v[1].diffuse = v[2].diffuse = v[3].diffuse = v[0].diffuse;
        v[2].u = v[0].u;
        v[3].u = v[1].u;
        v[1].v = v[0].v;
        v[3].v = v[2].v;
        n = quad_rect_words(v, &h, words);
        if (n != 16) {
            fprintf(stderr, "quad_instances: a rectangle not taken as one\n");
            exit(1);
        }
    } else {
        uint32_t first = batch_rows(b, v, (size_t)corners * 7) * 4;
        n = quad_vertex_words(corners, corners == 4, first, 0, &h, words);
    }
    batch_case(b, words, n, 1);
    for (i = 0; i < 4; i++)
        reference_widen(&v[i], &r[i]);
    expect_primitive(r, corners, c);
    return 1;
}

/* A camera, as world/view_camera.c makes one, and its screen */
struct View {
    float view[12];
    float proj[2], centre[2];
    struct Renderer renderer;
};

static void make_view(struct View *s, int kind)
{
    static const int sizes[][2] = {{1024, 664}, {1280, 720}, {1600, 900}, {1920, 1080}, {3840, 2160}, {800, 600}};
    float m[12], off[3], latitude, longitude, distance;
    Vec3 eye, target, up = {0.0f, 1.0f, 0.0f};
    int i = irange(0, 5);
    memset(s, 0, sizeof *s);
    renderer_set_projection(&s->renderer, sizes[i][0], sizes[i][1]);
    latitude = kind == KIND_FUZZ ? uniform(-89.0f, -1.0f) : uniform(-85.0f, -5.0f);
    distance = kind == KIND_FUZZ ? uniform(20.0f, 20000.0f) : uniform(100.0f, 2500.0f);
    longitude = uniform(0.0f, 360.0f);
    mat_set_rotate_x(m, latitude / 180.0f * 3.141592f);
    mat_rotate_y(m, longitude / 180.0f * 3.141592f);
    for (i = 0; i < 3; i++)
        off[i] = -distance * m[6 + i] + m[9 + i];
    target.x = uniform(-1000.0f, 1000.0f);
    target.y = uniform(-60.0f, 20.0f);
    target.z = uniform(-1000.0f, 1000.0f);
    eye.x = off[0] + target.x;
    eye.y = target.y + off[1];
    eye.z = target.z + off[2];
    mat_look_at(s->view, &eye, &target, &up);
    s->proj[0] = s->renderer.proj[0];
    s->proj[1] = s->renderer.proj[1];
    s->centre[0] = (float)s->renderer.centreX;
    s->centre[1] = (float)s->renderer.centreY;
}

/* a world point the view sees at screen point (px, py) and view depth `depth` (by its inverse) */
static void point_at(const struct View *s, float px, float py, float depth, float *out)
{
    const float *m = s->view;
    float rx = (px - s->centre[0]) / s->proj[0], ry = (py - s->centre[1]) / s->proj[1];
    float eye[3];
    int i;
    /* the eye: where the view takes the origin from, through the transposed rotation */
    for (i = 0; i < 3; i++)
        eye[i] = -(m[9] * m[3 * i] + m[10] * m[3 * i + 1] + m[11] * m[3 * i + 2]);
    for (i = 0; i < 3; i++)
        out[i] = eye[i] + (m[3 * i + 2] + rx * m[3 * i] + ry * m[3 * i + 1]) * depth;
}

/* A point through the client's 4x3 view (NULL: in view space already) and its projection, in
   double: x, y and rhw into out[0], out[1] and out[3]; into bound[0], [1] and [3] how far a float
   computation of the same may lie from them, to first order: each sum off by ROUNDING of its
   terms' magnitudes, the view depth's error carried through its reciprocal, each product and sum
   after it off by ROUNDING of its own size. */
static void exact_point(const float *p, const float *view, const float *proj, const float *centre, double *out,
                        double *bound)
{
    double v[3], e[3], rhw, erhw;
    int k;
    for (k = 0; k < 3; k++) {
        if (view) {
            double t0 = (double)view[k] * p[0], t1 = (double)view[3 + k] * p[1], t2 = (double)view[6 + k] * p[2];
            v[k] = t0 + t1 + t2 + view[9 + k];
            e[k] = ROUNDING * (fabs(t0) + fabs(t1) + fabs(t2) + fabs(view[9 + k]));
        } else {
            v[k] = p[k];
            e[k] = 0.0;
        }
    }
    rhw = 1.0 / v[2];
    erhw = fabs(rhw) * (e[2] / fabs(v[2]) + ROUNDING);
    out[3] = rhw;
    bound[3] = erhw;
    for (k = 0; k < 2; k++) {
        out[k] = v[k] * rhw * proj[k] + centre[k];
        bound[k] = fabs(proj[k]) * (e[k] * fabs(rhw) + fabs(v[k]) * erhw) + ROUNDING * (fabs(out[k]) + fabs(centre[k]));
    }
}

/* The rhw and depth rules (render/quads.h, GFX_RHW_* and GFX_DEPTH_*) on a projected point, in
   double, with their bounds; the depth as renderer_project_depth computes it, with its float
   constant: (1 / rhw - 10) S rhw, whose rounding is about S times twice the rhw's relative one; or
   the z given (GFX_DEPTH_CORNERS), exactly */
static void exact_rules(double *out, double *bound, int rhwRule, int depthRule, float rhwValue, float depthValue,
                        float given)
{
    double rhw = out[3], relative;
    if (rhwRule == GFX_RHW_ADD) {
        rhw += rhwValue;
        bound[3] += ROUNDING * fabs(rhw);
    } else if (rhwRule == GFX_RHW_VALUE) {
        rhw = rhwValue;
        bound[3] = 0.0;
    }
    out[3] = rhw;
    relative = rhw != 0.0 ? bound[3] / fabs(rhw) : 1.0;
    if (depthRule == GFX_DEPTH_VALUE || depthRule == GFX_DEPTH_CORNERS) {
        out[2] = depthRule == GFX_DEPTH_VALUE ? depthValue : given;
        bound[2] = 0.0;
    } else {
        out[2] = (1.0 / rhw - RENDER_NEAR_Z) * (double)RENDER_DEPTH_SCALE * rhw
                 + (depthRule == GFX_DEPTH_ADD ? depthValue : 0.0);
        bound[2] = (double)RENDER_DEPTH_SCALE * (2.0 * relative + ROUNDING) + ROUNDING * fabs(out[2]);
    }
}

/* A quad's corners as the device projects them, without the rounding of floats (GFX_QUAD_WORLD and
   GFX_QUAD_CORNERS; instances.hlsli, corner_of), into the case's exact corners */
static void exact_corners(const struct GfxQuad *q, int corners, struct Case *c)
{
    int i;
    if (q->kind == GFX_QUAD_WORLD) {
        for (i = 0; i < 4; i++) {
            exact_point(q->world.point[i], q->world.viewSpace ? NULL : q->world.view, q->world.proj, q->world.centre,
                        c->exact[i], c->bound[i]);
            exact_rules(c->exact[i], c->bound[i], q->world.rhw, q->world.depth, q->world.rhwValue, q->world.depthValue,
                        q->world.z[i]);
        }
    } else if (q->kind == GFX_QUAD_CORNERS) {
        const struct GfxCorners *set = &q->corners.set->corners;
        double up[4], c0[4], c1[4], eu[4], e0[4], e1[4];
        exact_point(set->uprightPoint, set->view, set->proj, set->centre, up, eu);
        exact_point(set->point[q->corners.corner[0]], set->view, set->proj, set->centre, c0, e0);
        exact_point(set->point[q->corners.corner[1]], set->view, set->proj, set->centre, c1, e1);
        for (i = 0; i < 4; i++) {
            int k = i < corners ? i : 2;
            exact_point(set->point[q->corners.corner[k]], set->view, set->proj, set->centre, c->exact[i], c->bound[i]);
            exact_rules(c->exact[i], c->bound[i], set->rhw, set->depth, set->rhwValue, set->depthValue, 0.0f);
            if (set->upright) {
                double side = k == 0 || (k == 2 && q->corners.right) ? 1.0 : -1.0;
                c->exact[i][0] = up[0] + side * fabs(c0[0] - c1[0]) * 0.5;
                c->bound[i][0] = eu[0] + 0.5 * (e0[0] + e1[0]) + ROUNDING * (fabs(c->exact[i][0]) + fabs(up[0]));
                c->exact[i][1] += set->lift;
                c->bound[i][1] += ROUNDING * (fabs(c->exact[i][1]) + fabs(set->lift));
            }
        }
    }
}

/* a matrix and projection as the device puts them among the quads (view NULL for points in view
   space): the row it starts at */
static uint32_t add_view(struct Batch *b, const float *view, const float *proj, const float *centre)
{
    float m[16];
    if (view)
        memcpy(m, view, 12 * sizeof *m);
    else
        memset(m, 0, 12 * sizeof *m);
    m[12] = proj[0];
    m[13] = proj[1];
    m[14] = centre[0];
    m[15] = centre[1];
    return batch_rows(b, m, 16);
}

/* a GFX_QUAD_WORLD instance's matrix, as the device puts it among the quads: the row it starts at */
static uint32_t add_matrix(struct Batch *b, const struct GfxQuad *q)
{
    return add_view(b, q->world.viewSpace ? NULL : q->world.view, q->world.proj, q->world.centre);
}

/* A quad of points projected as renderer_project_vertex projects them (particles): world points
   through the view, or points in view space already, with every rhw and z rule; answers how many
   cases it made */
static int world_case(int kind, const struct View *s, struct Batch *b, struct Case *c)
{
    struct GfxQuad q;
    struct QuadHeader h = {0, 0, 0, 0};
    struct ReferenceVertex r[4];
    uint32_t words[QUAD_WORDS_MAX];
    float cx = uniform(-0.2f, 1.2f) * s->centre[0] * 2.0f, cy = uniform(-0.2f, 1.2f) * s->centre[1] * 2.0f;
    float depth = kind == KIND_REALISTIC ? uniform(30.0f, 3000.0f) : chance(10) ? uniform(-400.0f, 10.0f)
                : uniform(10.0f, 40000.0f);
    float size = kind == KIND_FUZZ ? uniform(0.0f, 5000.0f) : uniform(0.0f, 60.0f);
    int i;
    memset(&q, 0, sizeof q);
    q.kind = GFX_QUAD_WORLD;
    q.world.viewSpace = chance(50);
    memcpy(q.world.view, s->view, sizeof q.world.view);
    q.world.proj[0] = s->proj[0];
    q.world.proj[1] = s->proj[1];
    q.world.centre[0] = s->centre[0];
    q.world.centre[1] = s->centre[1];
    q.world.rhw = irange(0, 2);
    q.world.depth = irange(0, 2);
    q.world.rhwValue = chance(30) ? 0.0f : uniform(-1e-4f, 1e-4f);
    q.world.depthValue = chance(50) ? uniform(-1e-4f, 1e-4f) : uniform(0.0f, 1.0f);
    q.world.colour = (uint32_t)next();
    q.world.stage2 = chance(30);
    for (i = 0; i < 4; i++) {
        float p[3];
        Vec3 point;
        struct TexDrawVertex v;
        point_at(s, cx + (i & 1 ? size : 0.0f), cy + (i & 2 ? size : 0.0f),
                 chance(80) ? depth : depth + uniform(-5, 5), p);
        point.x = p[0];
        point.y = p[1];
        point.z = p[2];
        if (q.world.viewSpace)
            renderer_project_view(&point, s->view, q.world.point[i]);
        else
            memcpy(q.world.point[i], p, sizeof p);
        q.world.u[i] = uniform(-1.0f, 2.0f);
        q.world.v[i] = uniform(-1.0f, 2.0f);
        q.world.u1[i] = uniform(-1.0f, 2.0f);
        q.world.v1[i] = uniform(-1.0f, 2.0f);
        /* the reference: renderer_project_vertex, then the rhw and z rules as the client applied
           them to its vertex */
        renderer_project_vertex((struct Renderer *)&s->renderer, &point, s->view, &v);
        if (q.world.rhw == GFX_RHW_ADD)
            v.rhw += q.world.rhwValue;
        else if (q.world.rhw == GFX_RHW_VALUE)
            v.rhw = q.world.rhwValue;
        if (q.world.depth == GFX_DEPTH_VALUE)
            v.z = q.world.depthValue;
        else {
            if (q.world.rhw != GFX_RHW_PROJECTED)
                v.z = renderer_project_depth(v.rhw);
            /* as the multitexture particles take their nudge off (v->z -= nudge), the instance
               holding the nudge negated */
            if (q.world.depth == GFX_DEPTH_ADD)
                v.z -= -q.world.depthValue;
        }
        v.diffuse = q.world.colour;
        v.u = q.world.u[i];
        v.v = q.world.v[i];
        reference_widen(&v, &r[i]);
        if (q.world.stage2) {
            r[i].u1 = q.world.u1[i];
            r[i].v1 = q.world.v1[i];
        }
    }
    h.matrix = add_matrix(b, &q);
    batch_case(b, words, quad_words(&q, &h, words), 1);
    expect_primitive(r, 4, c);
    exact_corners(&q, 4, c);
    return 1;
}

/* The camera of particle cases: the view's, as game_copy_view_matrix gives it, its projection as
   the renderer's, and a heading and pitch for the billboards */
static void camera_of(struct View *s)
{
    memcpy(g_view, s->view, sizeof g_view);
    g_renderer = &s->renderer;
    g_heading = uniform(0.0f, 360.0f);
    g_pitch = uniform(-89.0f, -1.0f);
}

/* A queued instance of points projected, a GFX_QUAD_WORLD quad or a GFX_QUAD_CORNERS triangle (its
   `corners` corners), against the vertices the reference made of it: sort keys and flatness here,
   every corner on the GPU */
static void world_record(struct Tally *t, const struct Queued *q, const struct TexDrawVertex *v, int corners,
                         struct Batch *b, struct Case *c)
{
    struct QuadHeader h = {0, 0, 0, 0};
    struct ReferenceVertex r[4];
    uint32_t words[QUAD_WORDS_MAX];
    int i, flat = q->quad.kind == GFX_QUAD_CORNERS ? q->quad.corners.flat : q->quad.world.flat;
    check_keys(t, q, v);
    if (flat != quad_vertices_flat(v, corners)) {
        t->flat++;
        if (t->printed[0]++ < 6)
            printf("  flat: cpu %d, instance %d\n", quad_vertices_flat(v, corners), flat);
    }
    if (q->quad.kind == GFX_QUAD_CORNERS) {
        const struct GfxCorners *set = &q->quad.corners.set->corners;
        uint32_t setWords[QUAD_CORNER_WORDS_MAX];
        uint32_t matrix = add_view(b, set->view, set->proj, set->centre);
        h.matrix = batch_rows(b, setWords, (size_t)quad_corner_words(set, matrix, setWords));
    } else {
        h.matrix = add_matrix(b, &q->quad);
    }
    batch_case(b, words, quad_words(&q->quad, &h, words), 1);
    for (i = 0; i < corners; i++)
        reference_widen(&v[i], &r[i]);
    expect_primitive(r, corners, c);
    exact_corners(&q->quad, corners, c);
}

/* A particle as particle_render draws it now, against the vertices it made: corners round the
   particle (a unit quad grown, or anything), turned to face the camera or not, through a matrix of
   size and turn, its depth bias and z on or off; answers how many cases it made */
static int particle_case(int kind, struct View *s, struct Batch *b, struct Case *c, struct Tally *t)
{
    static struct Particle p;
    struct TexDrawVertex v[4];
    Vec3 pos = {0.0f, 0.0f, 0.0f};
    float at[3], size = kind == KIND_FUZZ ? uniform(0.0f, 500.0f) : uniform(0.5f, 40.0f);
    float vt = uniform(0.0f, 1.0f), vb = uniform(0.0f, 1.0f), ul = uniform(0.0f, 1.0f), ur = uniform(0.0f, 1.0f);
    unsigned char facing = (unsigned char)chance(30), depth = (unsigned char)chance(70);
    int i;
    camera_of(s);
    point_at(s, uniform(0.0f, 2.0f) * s->centre[0], uniform(0.0f, 2.0f) * s->centre[1],
             kind == KIND_REALISTIC ? uniform(30.0f, 3000.0f) : uniform(10.0f, 40000.0f), at);
    pos.x = at[0];
    pos.y = at[1];
    pos.z = at[2];
    particle_construct(&p, pos, 1);
    p.texture = &g_texture;
    p.alpha = irange(0, 255);
    p.r = (short)irange(0, 255);
    p.g = (short)irange(0, 255);
    p.b = (short)irange(0, 255);
    p.depth_bias = chance(40) ? 0.0f : kind == KIND_FUZZ ? spread(-30, 0, 1) : uniform(-1e-4f, 1e-4f);
    for (i = 0; i < 4; i++) {
        p.corners[i].x = (i & 1 ? size : -size) + (kind == KIND_REALISTIC ? 0.0f : uniform(-size, size));
        p.corners[i].y = (i & 2 ? -size : size) + (kind == KIND_REALISTIC ? 0.0f : uniform(-size, size));
        p.corners[i].z = kind == KIND_REALISTIC ? 0.0f : uniform(-size, size);
    }
    mat_set_scale(p.matrix, uniform(0.2f, 3.0f), uniform(0.2f, 3.0f), uniform(0.2f, 3.0f));
    mat_rotate_z(p.matrix, uniform(0.0f, 6.28f));
    if (facing) {
        p.matrix[9] = pos.x;
        p.matrix[10] = pos.y;
        p.matrix[11] = pos.z;
    }
    queue_clear();
    particle_render(&p, facing, vt, vb, ul, ur, depth);
    if (!reference_particle_vertices(&p, facing, vt, vb, ul, ur, depth, v) || g_queuedCount != 1) {
        fprintf(stderr, "quad_instances: a particle drew %d quads\n", g_queuedCount);
        exit(1);
    }
    world_record(t, &g_queued[0], v, 4, b, c);
    return 1;
}

/* A sprite particle as sprite_particle_render draws it now, a quad of each of its motion's clips,
   against the vertices it made of each: clips moved, zoomed, turned, mirrored and tinted, the
   particle scaled and rolled; answers how many cases it made */
static int sprite_particle_case(int kind, struct View *s, struct Batch *b, struct Case *c, struct Tally *t)
{
    static struct SpriteParticle p;
    static struct TexDrawVertex v[3][4];
    Vec3 pos = {0.0f, 0.0f, 0.0f};
    float at[3];
    int i, n;
    camera_of(s);
    point_at(s, uniform(0.0f, 2.0f) * s->centre[0], uniform(0.0f, 2.0f) * s->centre[1],
             kind == KIND_REALISTIC ? uniform(30.0f, 3000.0f) : uniform(10.0f, 40000.0f), at);
    pos.x = at[0];
    pos.y = at[1];
    pos.z = at[2];
    sprite_particle_construct(&p, pos, chance(50) ? 1 : 5);
    p.spr = &g_spr;
    p.act = &g_act;
    p.loop = SPRITE_STILL;
    p.base.alpha = irange(0, 255);
    p.base.scale = kind == KIND_FUZZ ? uniform(0.0f, 20.0f) : uniform(0.5f, 2.0f);
    p.base.roll = uniform(0.0f, 360.0f);
    p.base.depth_bias = chance(40) ? 0.0f : kind == KIND_FUZZ ? spread(-30, 0, 1) : uniform(-1e-4f, 1e-4f);
    p.base.flags = chance(50) ? PARTICLE_TINT_BY_CLIP : 0;
    g_motion.clips = g_clips;
    g_motion.clip_count = (size_t)irange(1, 3);
    g_spr.images[0].items = g_images;
    g_spr.images[0].count = 3;
    for (i = 0; i < 3; i++) {
        g_images[i].width = (short)irange(1, 300);
        g_images[i].height = (short)irange(1, 300);
        g_images[i].texture = NULL;
        g_clips[i].x = irange(-60, 60);
        g_clips[i].y = irange(-60, 60);
        g_clips[i].sprIndex = chance(10) ? -1 : i;
        g_clips[i].clipType = 0;
        g_clips[i].flags = (unsigned)chance(30);
        g_clips[i].zoomX = uniform(0.25f, 2.0f);
        g_clips[i].zoomY = uniform(0.25f, 2.0f);
        g_clips[i].angle = chance(50) ? 0 : irange(0, 359);
        g_clips[i].color[0] = (unsigned char)irange(0, 255);
        g_clips[i].color[1] = (unsigned char)irange(0, 255);
        g_clips[i].color[2] = (unsigned char)irange(0, 255);
        g_clips[i].color[3] = (unsigned char)irange(0, 255);
    }
    g_region.u = uniform(0.0f, 0.9f);
    g_region.v = uniform(0.0f, 0.9f);
    g_region.u1 = g_region.u + uniform(0.0f, 0.1f);
    g_region.v1 = g_region.v + uniform(0.0f, 0.1f);
    queue_clear();
    sprite_particle_render(&p);
    n = reference_sprite_particle_vertices(&p, v, 3);
    if (n != g_queuedCount) {
        fprintf(stderr, "quad_instances: a sprite particle drew %d quads, its reference %d\n", g_queuedCount, n);
        exit(1);
    }
    for (i = 0; i < n; i++)
        world_record(t, &g_queued[i], v[i], 4, b, c + i);
    return n;
}

/* An effect primitive's quad as prim_draw_quad draws it now (effects/rag/effect_prim_draw.c), two
   triangles of the world, against the vertices it made of them: corners about the primitive's
   point, given in the world or about it, in every mode (projected, in front, the depth biased,
   standing up on the screen); answers how many cases it made */
static int prim_quad_case(int kind, struct View *s, struct Batch *b, struct Case *c, struct Tally *t)
{
    static struct EffectPrim prim;
    static struct RagEffect master;
    static struct Texture *textures[1] = {&g_texture};
    float p[4][3], at[3], u[2], v[2], size = kind == KIND_FUZZ ? uniform(0.0f, 2000.0f) : uniform(1.0f, 80.0f);
    unsigned char mode = (unsigned char)irange(0, 3), inWorld = (unsigned char)chance(50);
    short alpha = (short)irange(0, 255);
    unsigned char red = (unsigned char)irange(0, 255), green = (unsigned char)irange(0, 255);
    unsigned char blue = (unsigned char)irange(0, 255);
    unsigned argb;
    int count = 0, i, k;
    Vec3 q[4];
    camera_of(s);
    point_at(s, uniform(-0.2f, 1.2f) * 2.0f * s->centre[0], uniform(-0.2f, 1.2f) * 2.0f * s->centre[1],
             kind == KIND_REALISTIC ? uniform(30.0f, 3000.0f) : chance(10) ? uniform(-400.0f, 10.0f)
                                                               : uniform(10.0f, 40000.0f), at);
    master.color_r = master.color_g = master.color_b = master.dest_blend = -1;
    prim.master = &master;
    prim.textures = textures;
    prim.list_flags = chance(50) ? 1 : 5;
    prim.base.pos.x = at[0];
    prim.base.pos.y = at[1];
    prim.base.pos.z = at[2];
    prim.base.scale = kind == KIND_FUZZ ? uniform(-4.0f, 4.0f) : uniform(0.5f, 2.0f);
    /* q: the corners given, a quad lying about the primitive's point, in the world or about it;
       p: the world points the reference projects, as prim_draw_quad makes them of q */
    for (i = 0; i < 4; i++) {
        float o[3];
        for (k = 0; k < 3; k++)
            o[k] = (kind == KIND_REALISTIC ? 0.0f : uniform(-size, size))
                   + (k == 1 ? 0.0f : i & (k ? 2 : 1) ? size : -size);
        q[i].x = inWorld ? at[0] + o[0] : o[0];
        q[i].y = inWorld ? at[1] + o[1] : o[1];
        q[i].z = inWorld ? at[2] + o[2] : o[2];
        p[i][0] = inWorld ? q[i].x : prim.base.pos.x + q[i].x;
        p[i][1] = inWorld ? q[i].y : prim.base.pos.y + q[i].y;
        p[i][2] = inWorld ? q[i].z : prim.base.pos.z + q[i].z;
    }
    u[0] = uniform(-1.0f, 2.0f);
    u[1] = uniform(-1.0f, 2.0f);
    v[0] = uniform(-1.0f, 2.0f);
    v[1] = uniform(-1.0f, 2.0f);
    queue_clear();
    prim_draw_quad(&prim, &count, alpha, q[0], q[1], q[2], q[3], s->view, PRIM_BLEND_ADDITIVE, 0, inWorld, red, green,
                   blue, v[0], v[1], u[0], u[1], mode);
    if (count != 2 || g_queuedCount != 2 || g_queued[0].quad.kind != GFX_QUAD_CORNERS
        || g_queued[1].quad.corners.set != g_queued[0].quad.corners.set || g_sets[0].corners.count != 4) {
        fprintf(stderr, "quad_instances: an effect quad drew %d triangles, not two of one set of four corners\n",
                g_queuedCount);
        exit(1);
    }
    argb = ((((unsigned)(int)alpha << 8) + red) << 8 | green) << 8 | blue;
    for (k = 0; k < 2; k++) {
        /* (p0, p1, p2), then (p3, p2, p0), with prim_draw_quad's texture coordinates */
        static const int order[2][3] = {{0, 1, 2}, {3, 2, 0}};
        float corner[3][3], cu[3], cv[3];
        struct TexDrawVertex made[3];
        for (i = 0; i < 3; i++) {
            int n = order[k][i];
            memcpy(corner[i], p[n], sizeof corner[i]);
            cu[i] = n == 0 || n == 3 ? u[0] : u[1];
            cv[i] = n == 0 || n == 1 ? v[1] : v[0];
        }
        reference_prim_triangle((const float (*)[3])corner, cu, cv, argb, s->view, mode, at, prim.base.scale, k, made);
        world_record(t, &g_queued[k], made, 3, b, c + k);
    }
    return 2;
}

/* The ground's cell tiles as world/ground_draw.c draws them now: a quarter of a cell's quad (the
   cursor's cell, a ground skill's area, the navigation path, raised), or the navigation marker,
   against the vertices it made of them; answers how many cases it made */
static int cell_case(int kind, struct View *s, struct Batch *b, struct Case *c, struct Tally *t)
{
    static struct C3dGround15 ground;
    struct TexDrawVertex made[4];
    float at[3], size = kind == KIND_FUZZ ? uniform(0.0f, 500.0f) : uniform(1.0f, 10.0f), uv[8];
    int ax = irange(0, 799), ay = irange(0, 799), what = irange(0, 2), i, drawn;
    DWORD colour = (DWORD)next();
    camera_of(s);
    point_at(s, uniform(-0.2f, 1.2f) * 2.0f * s->centre[0], uniform(-0.2f, 1.2f) * 2.0f * s->centre[1],
             kind == KIND_REALISTIC ? uniform(30.0f, 3000.0f) : uniform(10.0f, 40000.0f), at);
    for (i = 0; i < 4; i++) {
        g_cellQuad[i * 3] = at[0] + (i & 1 ? size : 0.0f);
        g_cellQuad[i * 3 + 1] = at[1] + (kind == KIND_REALISTIC ? uniform(-2.0f, 2.0f) : uniform(-size, size));
        g_cellQuad[i * 3 + 2] = at[2] + (i & 2 ? size : 0.0f);
        g_cell.height[i] = g_cellQuad[i * 3 + 1];
        uv[i * 2] = uniform(0.0f, 1.0f);
        uv[i * 2 + 1] = uniform(0.0f, 1.0f);
    }
    ground.width = ground.height = 400;
    ground.zoom = kind == KIND_FUZZ ? uniform(1.0f, 50.0f) : 10.0f;
    g_texture.reqWidth = irange(1, 256);
    g_texture.reqHeight = irange(1, 256);
    g_texture.base.width = (unsigned)irange(g_texture.reqWidth, 512);
    g_texture.base.height = (unsigned)irange(g_texture.reqHeight, 512);
    queue_clear();
    if (what == 0) {
        c3d_ground15_draw_attr_quad(&ground, s->view, ax, ay, colour, &g_texture, uv);
        drawn = reference_attr_quad(&ground, s->view, ax, ay, colour, &g_texture, uv, 3.0517578e-5f, made);
    } else if (what == 1) {
        c3d_ground15_draw_attr_quad_raised(&ground, s->view, ax, ay, colour, &g_texture, uv);
        drawn = reference_attr_quad(&ground, s->view, ax, ay, colour, &g_texture, uv, 0.10003052f, made);
    } else {
        c3d_ground15_draw_attr_marker(&ground, s->view, ax, ay, colour, &g_texture);
        drawn = reference_attr_marker(&ground, s->view, ax, ay, colour, made);
    }
    if (drawn != g_queuedCount) {
        fprintf(stderr, "quad_instances: a cell tile drew %d quads, its reference %d\n", g_queuedCount, drawn);
        exit(1);
    }
    if (!drawn)
        return 0;
    world_record(t, &g_queued[0], made, 4, b, c);
    return 1;
}

/* ---- records of every kind ---- */

static int test_records(struct Batch *b, struct Case *cases, float *out, long perKind)
{
    enum { SPRITES, SCREEN, WORLD, PARTICLES, SPRITE_PARTICLES, EFFECT_QUADS, CELLS, WHAT_COUNT };
    static const char *const names[WHAT_COUNT] = {"sprites", "screen vertices", "projected points", "particles",
                                                  "sprite particles", "effect primitives' triangles",
                                                  "ground cell tiles"};
    static struct Tally tallies[WHAT_COUNT][KIND_COUNT];
    struct Renderer *client = g_renderer;
    int what, kind, ok = 1;
    memset(tallies, 0, sizeof tallies);
    for (what = 0; what < WHAT_COUNT; what++)
        for (kind = 0; kind < KIND_COUNT; kind++) {
            struct Tally *t = &tallies[what][kind];
            long done = 0;
            while (done < perKind) {
                long n = 0, i;
                static struct View s;
                batch_clear(b);
                make_view(&s, kind);
                while (done + n < perKind && batch_room(b)) {
                    if (what == SPRITES)
                        n += sprite_case(kind, b, &cases[n], t);
                    else if (what == SCREEN)
                        n += screen_case(kind, b, &cases[n]);
                    else if (what == WORLD)
                        n += world_case(kind, &s, b, &cases[n]);
                    else if (what == PARTICLES)
                        n += particle_case(kind, &s, b, &cases[n], t);
                    else if (what == SPRITE_PARTICLES)
                        n += sprite_particle_case(kind, &s, b, &cases[n], t);
                    else if (what == EFFECT_QUADS)
                        n += prim_quad_case(kind, &s, b, &cases[n], t);
                    else
                        n += cell_case(kind, &s, b, &cases[n], t);
                    if (what >= WORLD && n % 64 == 0)
                        make_view(&s, kind);
                }
                g_renderer = client;
                probe_run(b, out);
                for (i = 0; i < n; i++) {
                    int c;
                    t->instances++;
                    for (c = 0; c < 4; c++) {
                        char label[96];
                        t->vertices++;
                        snprintf(label, sizeof label, "%s, %s, corner %d", names[what], g_kindNames[kind], c);
                        const float *made = out + (size_t)i * 8 * PROBE_FLOATS;
                        compare(t, 0, made + (size_t)c * PROBE_FLOATS, made, &cases[i], c, label);
                        compare(t, 1, made + (size_t)(4 + c) * PROBE_FLOATS, made, &cases[i], c, label);
                    }
                }
                done += n;
            }
        }
    for (what = 0; what < WHAT_COUNT; what++)
        for (kind = 0; kind < KIND_COUNT; kind++) {
            char name[96];
            snprintf(name, sizeof name, "%s, %s", names[what], g_kindNames[kind]);
            ok = report(name, &tallies[what][kind]) && ok;
        }
    return ok;
}

/* ---- pictures through render/gfx_gpu.c ---- */

#define PICTURE_W 1280
#define PICTURE_H 720

/* a value that rises and falls by one step from t to t + 1, 0 to 255 */
static uint32_t wave(int t)
{
    t &= 511;
    return (uint32_t)(t < 256 ? t : 511 - t);
}

/* A page of gentle gradients: each channel a step at most from a texel to its neighbours across or
   along (red along u, green along v, blue and alpha along both, every alpha from see-through to
   opaque), so that coordinates interpolated a hair apart read the same colour within a step and
   the pictures show where the quads cover the screen. */
static gfx_surface *page_texture(int layered, unsigned salt)
{
    gfx_surface *t = layered ? gfx_create_layer_texture(256, 256, GFX_FMT_8888)
                             : gfx_create_texture(256, 256, GFX_FMT_8888);
    struct gfx_lock lk;
    int x, y, k = (int)salt * 97;
    need(t && gfx_lock(t, NULL, &lk) == 0, "page texture");
    for (y = 0; y < 256; y++)
        for (x = 0; x < 256; x++) {
            uint32_t c = wave(x + k) << 16 | wave(y + 2 * k) << 8 | wave((x + y) / 2 + k)
                         | wave((x + 2 * y) / 3 + 3 * k) << 24;
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &c, 4);
        }
    gfx_unlock(t);
    return t;
}

static void read_picture(uint32_t *out)
{
    need(gfx_read_frame(NULL, out, PICTURE_W * 4) == 0, "gfx_read_frame");
}

/* A quad a particle makes on the picture's screen: world points the view sees, for the device to
   project, given as they are or in view space, and the vertices renderer_project_vertex makes of
   them */
static void picture_world_quad(const struct View *s, struct GfxQuad *q, struct TexDrawVertex *v)
{
    float cx = uniform(-0.1f, 1.1f) * (float)PICTURE_W, cy = uniform(-0.1f, 1.1f) * (float)PICTURE_H;
    float depth = uniform(30.0f, 3000.0f), size = uniform(5.0f, 120.0f), rhw[4];
    int i;
    memset(q, 0, sizeof *q);
    q->kind = GFX_QUAD_WORLD;
    q->world.viewSpace = chance(50);
    memcpy(q->world.view, s->view, sizeof q->world.view);
    q->world.proj[0] = s->proj[0];
    q->world.proj[1] = s->proj[1];
    q->world.centre[0] = s->centre[0];
    q->world.centre[1] = s->centre[1];
    q->world.colour = (uint32_t)next() | 0xc0000000u;
    for (i = 0; i < 4; i++) {
        float p[3];
        Vec3 point;
        point_at(s, cx + (i & 1 ? size : 0.0f), cy + (i & 2 ? size * uniform(0.5f, 1.5f) : 0.0f),
                 depth + uniform(-20.0f, 20.0f), p);
        point.x = p[0];
        point.y = p[1];
        point.z = p[2];
        if (q->world.viewSpace)
            renderer_project_view(&point, s->view, q->world.point[i]);
        else
            memcpy(q->world.point[i], p, sizeof p);
        renderer_project_vertex((struct Renderer *)&s->renderer, &point, s->view, &v[i]);
        v[i].diffuse = q->world.colour;
        v[i].u = q->world.u[i] = uniform(0.0f, 1.0f);
        v[i].v = q->world.v[i] = uniform(0.0f, 1.0f);
        rhw[i] = v[i].rhw;
    }
    q->world.flat = quad_rhw_flat(rhw, 4);
}

/* Scenes of sprites in two modes (blended with depth written; the alpha test without blending),
   drawn as instances from layered textures and as the reference's vertices from plain ones; and
   of projected quads (blended additively, and alpha-tested, which multisampling covers by alpha
   to coverage where they are not flat) drawn as instances and as renderer_project_vertex's
   vertices: each pair within the captures' budget. */

/* A picture beyond the budget kept for a look: quad_<scene>_<kind>_<side>.bmp in the working
   directory */
static void save_picture(const uint32_t *pixels, int scene, int kind, const char *side)
{
    SDL_Surface *s = SDL_CreateSurfaceFrom(PICTURE_W, PICTURE_H, SDL_PIXELFORMAT_XRGB8888, (void *)pixels,
                                           PICTURE_W * 4);
    char name[64];
    snprintf(name, sizeof name, "quad_%d_%d_%s.bmp", scene, kind, side);
    if (s) {
        SDL_SaveBMP(s, name);
        SDL_DestroySurface(s);
    }
}

/* the distance from (px, py) to the segment from a to b */
static double segment_distance(double px, double py, const struct TexDrawVertex *a, const struct TexDrawVertex *b)
{
    double dx = (double)b->x - a->x, dy = (double)b->y - a->y, l = dx * dx + dy * dy, t = 0.0, ex, ey;
    if (l > 0.0)
        t = ((px - a->x) * dx + (py - a->y) * dy) / l;
    t = t < 0.0 ? 0.0 : t > 1.0 ? 1.0 : t;
    ex = a->x + t * dx - px;
    ey = a->y + t * dy - py;
    return sqrt(ex * ex + ey * ey);
}

/* The depth of quad q (a strip of four corners, triangles (0, 1, 2) and (2, 1, 3)) at (px, py),
   interpolated on the screen as the rasterizer interpolates it; answers 0 where the quad does not
   cover the point. */
static int quad_depth_at(const struct TexDrawVertex *q, double px, double py, double *z)
{
    static const int tris[2][3] = {{0, 1, 2}, {2, 1, 3}};
    int t;
    for (t = 0; t < 2; t++) {
        const struct TexDrawVertex *a = &q[tris[t][0]], *b = &q[tris[t][1]], *c = &q[tris[t][2]];
        double dx1 = (double)b->x - a->x, dy1 = (double)b->y - a->y, dx2 = (double)c->x - a->x;
        double dy2 = (double)c->y - a->y, det = dx1 * dy2 - dx2 * dy1, ox = px - a->x, oy = py - a->y, l1, l2;
        if (det == 0.0)
            continue;
        l1 = (ox * dy2 - oy * dx2) / det;
        l2 = (oy * dx1 - ox * dy1) / det;
        if (l1 >= 0.0 && l2 >= 0.0 && l1 + l2 <= 1.0) {
            *z = a->z + l1 * ((double)b->z - a->z) + l2 * ((double)c->z - a->z);
            return 1;
        }
    }
    return 0;
}

/* Whether pixel (x, y)'s difference is the vertices' tolerance's: an edge of the n quads (verts) comes
   within `reach` of its centre, or two quads that cover it lie at depths there within the depth
   tolerance of each other, where the depth test may pick either. */
static int by_edge(int x, int y, const struct TexDrawVertex (*verts)[4], int n, double reach)
{
    static const int edges[5][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0}, {1, 2}};
    double depths[512];
    int i, e, j, covering = 0;
    for (i = 0; i < n; i++)
        for (e = 0; e < 5; e++)
            if (segment_distance(x, y, &verts[i][edges[e][0]], &verts[i][edges[e][1]]) <= reach)
                return 1;
    for (i = 0; i < n && covering < 512; i++)
        if (quad_depth_at(verts[i], x, y, &depths[covering]))
            covering++;
    for (i = 0; i < covering; i++)
        for (j = i + 1; j < covering; j++)
            if (fabs(depths[i] - depths[j]) <= RELATIVE_TOLERANCE * fmax(fabs(depths[i]), 0.5))
                return 1;
    return 0;
}

/* The pixels more than a step apart in groups of more than 16 that touch, the budget's groups,
   which do not lie wholly where the vertices' tolerance changes pixels (by_edge): within `reach` of
   an edge, where an edge moved within the tolerance covers the centre of a pixel, or of one of its
   samples, otherwise, so that a long straight edge that lies that near a row of pixel or sample
   centres changes the whole row; and along the line where two quads' depths cross, within the
   depth tolerance (a sprite's depth changes along its rows alone, so that line is a row). */
static long groups_off_edges(const uint32_t *a, const uint32_t *b, const struct TexDrawVertex (*verts)[4], int n,
                             double reach)
{
    static unsigned char mark[PICTURE_W * PICTURE_H];
    static int stack[PICTURE_W * PICTURE_H], group[PICTURE_W * PICTURE_H];
    long off = 0;
    int i, c;
    for (i = 0; i < PICTURE_W * PICTURE_H; i++) {
        int d = 0;
        for (c = 0; c < 24; c += 8) {
            int e = abs((int)(a[i] >> c & 255) - (int)(b[i] >> c & 255));
            d = e > d ? e : d;
        }
        mark[i] = d > 1;
    }
    for (i = 0; i < PICTURE_W * PICTURE_H; i++) {
        int top = 0, size = 0, k, along = 1;
        if (mark[i] != 1)
            continue;
        stack[top++] = i;
        mark[i] = 2;
        while (top) {
            int at = stack[--top], x = at % PICTURE_W, y = at / PICTURE_W, dx, dy;
            group[size++] = at;
            for (dy = -1; dy <= 1; dy++)
                for (dx = -1; dx <= 1; dx++) {
                    int xx = x + dx, yy = y + dy;
                    if (xx >= 0 && xx < PICTURE_W && yy >= 0 && yy < PICTURE_H && mark[yy * PICTURE_W + xx] == 1) {
                        mark[yy * PICTURE_W + xx] = 2;
                        stack[top++] = yy * PICTURE_W + xx;
                    }
                }
        }
        if (size <= 16)
            continue;
        for (k = 0; k < size && along; k++)
            along = by_edge(group[k] % PICTURE_W, group[k] / PICTURE_W, verts, n, reach);
        off += along ? 0 : size;
    }
    return off;
}

/* A pair of pictures held to the budget, counted into the kind's differing pixels (differ[0]), its
   pixels more than a step apart (differ[1]), the largest group of those (differ[2]) and the pixels
   of groups of more than 16 that do not lie along edges (differ[3]): beyond a step at most 0.05 %
   of the screen, and every group of more than 16 such pixels along the edges of the quads drawn or
   where their depths cross (groups_off_edges): in these scenes of hundreds of quads drawn at random,
   long straight edges and crossings lie within the vertices' tolerance of rows of pixel centres far
   more often than in a game's frame. A
   scene beyond the budget leaves its two pictures in the working directory
   (quad_<scene>_<kind>_device.bmp and quad_<scene>_<kind>_cpu.bmp). Answers whether it keeps it. */
static int budget(const uint32_t *a, const uint32_t *b, int scene, int kind, const struct TexDrawVertex (*verts)[4],
                  int n, double reach, long *differ, int *most)
{
    struct PictureDifference d;
    long away;
    int ok;
    picture_difference(a, b, PICTURE_W, PICTURE_H, &d);
    away = d.largest > 16 ? groups_off_edges(a, b, verts, n, reach) : 0;
    ok = d.over * 2000 <= (long)PICTURE_W * PICTURE_H && !away;
    if (!ok || !picture_within_budget(&d, PICTURE_W, PICTURE_H)) {
        printf("  scene %d, kind %d: %ld pixels more than a step apart, the largest group %ld, %ld in groups of more "
               "than 16 off the edges\n", scene, kind, d.over, d.largest, away);
        if (!ok) {
            save_picture(a, scene, kind, "device");
            save_picture(b, scene, kind, "cpu");
        }
    }
    differ[0] += d.differ;
    differ[1] += d.over;
    if (d.largest > differ[2])
        differ[2] = d.largest;
    differ[3] += away;
    if (d.most > *most)
        *most = d.most;
    return ok;
}
/* the samples a pixel of the pictures has (RAGEXE_MSAA) */
static int g_samples = 1;

static int test_pictures(int scenes)
{
    static uint32_t a[PICTURE_W * PICTURE_H], c[PICTURE_W * PICTURE_H];
    static struct TexDrawVertex verts[512][4];
    static struct GfxQuad quads[512];
    static int pages[512];
    gfx_surface *layered[2], *plain[2];
    long differ[4][4], covered = 0;
    int scene, ok = 1, i, side, most[4] = {0, 0, 0, 0};
    /* a pixel's samples lie within half a pixel of its centre; a flat draw's corner moved onto the
       boundary next to the CPU's moves its edge by a pixel more (instances.hlsli, snapped) */
    double reach = PIXEL_TOLERANCE + (g_samples > 1 ? 1.5 : 0.5);
    memset(differ, 0, sizeof differ);
    for (i = 0; i < 2; i++) {
        layered[i] = page_texture(1, (unsigned)i);
        plain[i] = page_texture(0, (unsigned)i);
    }
    for (scene = 0; scene < scenes; scene++) {
        static const int sprite_modes[2] = {DRAW_ALPHA_DEPTH, DRAW_OPAQUE_CLAMP};
        static const int world_modes[2] = {DRAW_ADDITIVE, DRAW_OPAQUE_CLAMP};
        struct View s;
        int n = irange(50, 400), m;
        /* sprites, made by render_object_submit_spr_quad and by the reference */
        for (i = 0; i < n; i++) {
            struct SprImg img;
            struct RenderObject obj;
            struct SqQuad quad;
            float x = 0.0f, y = 0.0f;
            int palette = 0x1234;
            memset(&obj, 0, sizeof obj);
            memset(&img, 0, sizeof img);
            memset(&quad, 0, sizeof quad);
            quad.image = &img;
            quad.x0 = uniform(-50.0f, (float)PICTURE_W);
            quad.top = uniform(-50.0f, (float)PICTURE_H);
            quad.x1 = quad.x0 + uniform(1.0f, 200.0f);
            quad.bottom = quad.top + uniform(1.0f, 200.0f);
            quad.depth_b = 1.0f / uniform(15.0f, 3000.0f);
            quad.depth_a = quad.depth_b + uniform(0.0f, 1e-4f);
            quad.color = (unsigned)next() | 0xc0000000u;
            if (chance(20)) {
                quad.angle = chance(50) ? 180.0f : (float)irange(1, 359);
                x = uniform(-20.0f, 20.0f);
                y = uniform(-20.0f, 20.0f);
            }
            g_region.u = (float)irange(0, 7) * 0.125f;
            g_region.v = (float)irange(0, 7) * 0.125f;
            g_region.u1 = g_region.u + 0.125f * (float)irange(1, 2);
            g_region.v1 = g_region.v + 0.125f * (float)irange(1, 2);
            queue_clear();
            render_object_submit_spr_quad(&obj, &quad, &palette, 0x101, x, y);
            quads[i] = g_queued[0].quad;
            reference_sprite_vertices(&quad, &g_region, palette, x, y, verts[i]);
            pages[i] = (int)(next() & 1);
        }
        for (m = 0; m < 2; m++) {
            for (side = 0; side < 2; side++) {
                gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff203040, 1.0f, NULL);
                gfx_set_mode(sprite_modes[m]);
                for (i = 0; i < n; i++) {
                    gfx_set_texture(0, side ? plain[pages[i]] : layered[pages[i]]);
                    if (side)
                        gfx_draw_vertices(verts[i], 4, 1);
                    else
                        gfx_draw_quad(&quads[i]);
                }
                gfx_set_texture(0, NULL);
                gfx_end_scene();
                read_picture(side ? c : a);
            }
            ok = budget(a, c, scene, m, (const struct TexDrawVertex (*)[4])verts, n, reach, differ[m], &most[m]) && ok;
            for (i = 0; i < PICTURE_W * PICTURE_H; i++)
                covered += a[i] != 0xff203040u;
        }
        /* projected quads */
        make_view(&s, KIND_REALISTIC);
        renderer_set_projection(&s.renderer, PICTURE_W, PICTURE_H);
        s.proj[0] = s.renderer.proj[0];
        s.proj[1] = s.renderer.proj[1];
        s.centre[0] = (float)s.renderer.centreX;
        s.centre[1] = (float)s.renderer.centreY;
        for (i = 0; i < n; i++)
            picture_world_quad(&s, &quads[i], verts[i]);
        for (m = 0; m < 2; m++) {
            for (side = 0; side < 2; side++) {
                gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff203040, 1.0f, NULL);
                gfx_set_mode(world_modes[m]);
                gfx_set_texture(0, plain[0]);
                for (i = 0; i < n; i++) {
                    if (side)
                        gfx_draw_vertices(verts[i], 4, 1);
                    else
                        gfx_draw_quad(&quads[i]);
                }
                gfx_set_texture(0, NULL);
                gfx_end_scene();
                read_picture(side ? c : a);
            }
            ok = budget(a, c, scene, 2 + m, (const struct TexDrawVertex (*)[4])verts, n, reach, differ[2 + m],
                        &most[2 + m]) && ok;
            for (i = 0; i < PICTURE_W * PICTURE_H; i++)
                covered += a[i] != 0xff203040u;
        }
    }
    for (i = 0; i < 2; i++) {
        gfx_release(layered[i]);
        gfx_release(plain[i]);
    }
    printf("pictures through render/gfx_gpu.c: %d scenes of %d by %d, %ld pixels drawn; differing pixels, those "
           "more than a step apart, the largest group of those, those in groups of more than 16 off the edges and the "
           "largest difference: "
           "sprites blended %ld, %ld, %ld, %ld, %d; alpha-tested %ld, %ld, %ld, %ld, %d; projected quads additive %ld, "
           "%ld, %ld, %ld, %d; alpha-tested %ld, %ld, %ld, %ld, %d\n",
           scenes, PICTURE_W, PICTURE_H, covered, differ[0][0], differ[0][1], differ[0][2], differ[0][3], most[0],
           differ[1][0], differ[1][1], differ[1][2], differ[1][3], most[1], differ[2][0], differ[2][1], differ[2][2],
           differ[2][3], most[2], differ[3][0], differ[3][1], differ[3][2], differ[3][3], most[3]);
    return ok;
}

int main(int argc, char **argv)
{
    static struct Renderer renderer;
    struct C3dDevice *d = &g_3dDevice;
    uint64_t seed = argc > 2 ? strtoull(argv[2], NULL, 0) : 20261112u;
    long perKind = argc > 3 ? strtol(argv[3], NULL, 0) : 40000;
    int scenes = argc > 4 ? (int)strtol(argv[4], NULL, 0) : 12, ok = 1;
    struct Batch batch;
    struct Case *cases = malloc(sizeof *cases * PER_BATCH);
    float *out = malloc(sizeof(float) * PROBE_FLOATS * 4 * (size_t)PER_BATCH);

    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc > 1) {
        SDL_setenv_unsafe("RAGEXE_MSAA", argv[1], 1);
        g_samples = atoi(argv[1]);
    }
    memset(&batch, 0, sizeof batch);
    batch.words = malloc(BATCH_WORDS * 4);
    batch.offsets = malloc(PER_BATCH * 4);
    if (!cases || !out || !batch.words || !batch.offsets)
        return 1;
    math_init_degree_tables();
    g_rng = seed;
    /* what render_object_submit_spr_quad reads of the renderer: whether alpha is drawn in two
       passes */
    renderer_set_projection(&renderer, PICTURE_W, PICTURE_H);
    g_renderer = &renderer;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("quad_instances: skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->hwnd = SDL_CreateWindow("quad instances", PICTURE_W, PICTURE_H, SDL_WINDOW_HIDDEN);
    if (!d->hwnd) {
        printf("quad_instances: skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->width = PICTURE_W;
    d->height = PICTURE_H;
    need(gfx_open(d) == 0, "gfx_open");
    if (gfx_create_screen(d) != 0) {
        printf("quad_instances: skipped: no GPU device\n");
        return 77;
    }
    need(gfx_get_caps(&g_gfxCaps) == 0 && gfx_create_zbuffer(d) == 0, "the device's screen");
    g_dev = (SDL_GPUDevice *)d->dev;
    printf("quad_instances: %s on %s, seed %llu, RAGEXE_MSAA %s\n", SDL_GetGPUDeviceDriver(g_dev),
           SDL_GetStringProperty(SDL_GetGPUDeviceProperties(g_dev), SDL_PROP_GPU_DEVICE_NAME_STRING, "?"),
           (unsigned long long)seed, argc > 1 ? argv[1] : "unset");

    probe_init();
    if (perKind > 0)
        ok = test_records(&batch, cases, out, perKind) && ok;
    probe_free();
    if (scenes > 0)
        ok = test_pictures(scenes) && ok;

    gfx_close(d);
    SDL_DestroyWindow(d->hwnd);
    SDL_Quit();
    printf("quad_instances: %s\n", ok ? "every vertex within the tolerance, every picture within the budget"
                                       : "DIFFERENCES");
    return ok ? 0 : 1;
}
