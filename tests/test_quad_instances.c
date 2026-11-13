/* The client's quads and triangles as instances (render/gfx.h, gfx_draw_quad and
   gfx_draw_vertices; render/shaders/quad.hlsl) on this machine's GPU, against the CPU code they
   replaced (tests/fixtures/quad_reference.c: the vertices render_object_submit_spr_quad,
   particle_render and sprite_particle_render made, and the multisampled pass's snap_flat, as they
   were) and the client's own C code (renderer_project_vertex, renderer_project_depth):

   - The shaders' SPIR-V: no operation that would let the GPU round otherwise than the CPU in the
     code that must match it (render/shaders/instances.hlsli and exact.hlsli, which
     tests/shaders/quad_probe.hlsl runs alone), and in quad.hlsl's vertex shader only the
     operations phase 4's quad shader had too (bare_operations).
   - The exact operations alone against the C expressions they stand for: division over every
     divisor from 1 to 2 and beyond, renderer_project_depth, renderer_project_vertex.
   - Records of every kind, made as the client makes them (sprites by render_object_submit_spr_quad
     itself, actor/sprite_quad.c; particles by particle_render and sprite_particle_render,
     effects/particle.c and sprite_particle.c; the rest by render/quads.c), realistic, edge and
     fuzzed, read on the GPU's vertex stage by quad_vertex, the vertex the quad shader draws: every
     corner's x, y, z, rhw, colour and both stages' coordinates bit for bit, as made and as moved
     onto pixel boundaries (QUAD_SNAP), against the reference; and on the CPU the rhw the lists
     sort sprites and particles by and whether a particle's quad is flat.
   - Pictures through render/gfx_gpu.c itself: scenes of sprites drawn as the instances the client
     now makes them (from a layered texture, an array's layer) and as the reference's vertices (from
     a plain texture of the same texels), and of projected quads drawn as instances and as the
     vertices renderer_project_vertex makes, pixel for pixel, at the sample count asked for.

   usage: test_quad_instances msaa [seed] [records per kind] [exact operations] [scenes]
   Exits 77 where no GPU device can be made. */
#include "actor/types.h"
#include "actor/sprite_render.h"
#include "actor/render_ops.h"
#include "actor/game_actor.h"
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
#include "render/shaders/quad.h"
#include "shaders/quad_probe.h"
#include "fixtures/quad_reference.h"
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
    g_slotCount = g_queuedCount = 0;
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
           and its neighbours (exact.hlsli, pixel_boundary) */
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
static SDL_GPUTexture *g_targets[4];
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
    SDL_GPUColorTargetDescription ct[4];
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
    for (i = 0; i < 4; i++)
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
    info.target_info.num_color_targets = 4;
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
    for (i = 0; i < 4; i++) {
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
    for (i = 0; i < 4; i++)
        SDL_ReleaseGPUTexture(g_dev, g_targets[i]);
    SDL_ReleaseGPUBuffer(g_dev, g_corners);
    SDL_ReleaseGPUBuffer(g_dev, g_identity);
    SDL_ReleaseGPUBuffer(g_dev, g_offsets);
    SDL_ReleaseGPUBuffer(g_dev, g_quads);
    SDL_ReleaseGPUGraphicsPipeline(g_dev, g_probe);
}

/* Runs the batch's instances through the probe for its results `mode` (0: quad_vertex's, 1: the
   exact operations'); out gets each vertex's 16 floats (4 vertices an instance). */
static void probe_run(const struct Batch *b, int mode, float *out)
{
    Uint32 n = (Uint32)b->ninstances, targetBytes = SIDE * SIDE * 16;
    Uint32 sizes[4], offsets[4], total = 0, i;
    SDL_GPUTransferBufferCreateInfo tinfo;
    SDL_GPUTransferBuffer *up, *down;
    SDL_GPUBuffer *dests[4];
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    SDL_GPUFence *fence;
    SDL_GPUColorTargetInfo ct[4];
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
    tinfo.size = targetBytes * 4;
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
    for (i = 0; i < 4; i++) {
        ct[i].texture = g_targets[i];
        ct[i].load_op = SDL_GPU_LOADOP_CLEAR;
        ct[i].store_op = SDL_GPU_STOREOP_STORE;
        ct[i].cycle = true;
    }
    rp = SDL_BeginGPURenderPass(cmd, ct, 4, NULL);
    SDL_BindGPUGraphicsPipeline(rp, g_probe);
    bind[0].buffer = g_corners;
    bind[1].buffer = g_offsets;
    bind[2].buffer = g_identity;
    bind[0].offset = bind[1].offset = bind[2].offset = 0;
    SDL_BindGPUVertexBuffers(rp, 0, bind, 3);
    SDL_BindGPUVertexStorageBuffers(rp, 0, &g_quads, 1);
    probe[0] = SIDE;
    probe[1] = (Uint32)mode;
    probe[2] = 0;
    memcpy(&probe[3], &step, 4);
    SDL_PushGPUVertexUniformData(cmd, 0, probe, sizeof probe);
    SDL_DrawGPUPrimitives(rp, 4, n, 0, 0);
    SDL_EndGPURenderPass(rp);
    copy = SDL_BeginGPUCopyPass(cmd);
    for (i = 0; i < 4; i++) {
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
        for (t = 0; t < 4; t++)
            memcpy(out + (size_t)i * 16 + t * 4, p + (size_t)t * targetBytes + (size_t)i * 16, 16);
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

/* the same float, bit for bit, or both not a number: which NaN an operation makes is the
   hardware's, and a vertex with one covers nothing either way */
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

static const char *const g_fieldNames[12] = {"x", "y", "z", "rhw", "r", "g", "b", "a", "u0", "v0", "u1", "v1"};

struct Tally {
    long instances, vertices, differ[2][12], printed, keys, flat;
};

/* the probe's 16 floats of a vertex, as made (mode 0) or snapped (1): r0 (x, y, z, rhw), r1 (the
   colour), r2 (uv) */
static int compare(struct Tally *t, int mode, const float *got, const struct Expect *want, const char *what)
{
    int f, bad = 0;
    for (f = 0; f < 12; f++)
        if (!same(got[f], want->v[f])) {
            t->differ[mode][f]++;
            if (t->printed < 16) {
                t->printed++;
                printf("  %s, %s: %s cpu %.9g gpu %.9g\n", what, mode ? "snapped" : "as made", g_fieldNames[f],
                       want->v[f], got[f]);
            }
            bad = 1;
        }
    return bad;
}

static int report(const char *name, const struct Tally *t)
{
    int m, f, clean = !t->keys && !t->flat;
    printf("%s: %ld records, %ld vertices; sort keys differ %ld, flatness %ld; differ as made:", name, t->instances,
           t->vertices, t->keys, t->flat);
    for (m = 0; m < 2; m++) {
        if (m)
            printf("; snapped:");
        for (f = 0; f < 12; f++) {
            printf(" %ld", t->differ[m][f]);
            clean = clean && !t->differ[m][f];
        }
    }
    printf("\n");
    return clean;
}

/* ---- cases ---- */

/* One record and what each of its four corners should come back as, made and snapped */
struct Case {
    struct Expect made[4], snapped[4];
};

/* the reference's vertices of a primitive, made and snapped as a flat draw's in a multisampled
   pass: a strip's triangles (0, 1, 2) and (2, 1, 3), a triangle's (0, 1, 2) */
static void expect_primitive(const struct ReferenceVertex *in, int corners, struct Case *c)
{
    static const unsigned short strip[6] = {0, 1, 2, 2, 1, 3};
    struct ReferenceVertex moved[4];
    int i;
    memcpy(moved, in, (size_t)corners * sizeof *in);
    reference_snap_flat(moved, corners, corners == 4 ? strip : NULL, corners == 4 ? 6 : 3);
    for (i = 0; i < 4; i++) {
        int k = i < corners ? i : 2;
        expect_of(&in[k], &c->made[i]);
        expect_of(&moved[k], &c->snapped[i]);
    }
}

/* the rhw the lists sort a queued instance by against those of the vertices it stands for */
static void check_keys(struct Tally *t, const struct Queued *q, const struct TexDrawVertex *v)
{
    if (bits(q->rhw[0]) != bits(v[0].rhw) || bits(q->rhw[1]) != bits(v[1].rhw)) {
        t->keys++;
        if (t->printed++ < 16)
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

/* A primitive of pre-transformed vertices: a rectangle, a strip or a triangle, flat or not, or a
   triangle of world vertices (QUAD_CAMERA, `world`), which the shader hands on as they are for the
   camera's view to take; answers how many cases it made */
static int screen_case(int kind, int world, struct Batch *b, struct Case *c)
{
    struct QuadHeader h = {0, 0, 0, 0};
    struct TexDrawVertex v[4];
    struct ReferenceVertex r[4];
    uint32_t words[QUAD_WORDS_MAX];
    int shape = irange(world ? 2 : 0, 2), corners = shape == 2 ? 3 : 4, flat = chance(70), i, n;
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
        n = quad_vertex_words(corners, corners == 4, world, first, 0, &h, words);
    }
    batch_case(b, words, n, !world);
    for (i = 0; i < 4; i++)
        reference_widen(&v[i], &r[i]);
    expect_primitive(r, corners, c);
    if (world)
        memcpy(c->snapped, c->made, sizeof c->made);
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

/* a GFX_QUAD_WORLD instance's matrix, as the device puts it among the quads: the row it starts at */
static uint32_t add_matrix(struct Batch *b, const struct GfxQuad *q)
{
    float m[16];
    if (q->world.viewSpace)
        memset(m, 0, 12 * sizeof *m);
    else
        memcpy(m, q->world.view, 12 * sizeof *m);
    m[12] = q->world.proj[0];
    m[13] = q->world.proj[1];
    m[14] = q->world.centre[0];
    m[15] = q->world.centre[1];
    return batch_rows(b, m, 16);
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
    q.world.rhw = irange(0, 1);
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
        if (q.world.depth == GFX_DEPTH_ZERO)
            v.z = 0.0f;
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

/* A queued particle quad against the vertices the reference made of it: sort keys and flatness
   here, every corner on the GPU */
static void particle_record(struct Tally *t, const struct Queued *q, const struct TexDrawVertex *v, struct Batch *b,
                            struct Case *c)
{
    struct QuadHeader h = {0, 0, 0, 0};
    struct ReferenceVertex r[4];
    uint32_t words[QUAD_WORDS_MAX];
    int i;
    check_keys(t, q, v);
    if (q->quad.world.flat != quad_vertices_flat(v, 4)) {
        t->flat++;
        if (t->printed++ < 16)
            printf("  flat: cpu %d, instance %d\n", quad_vertices_flat(v, 4), q->quad.world.flat);
    }
    h.matrix = add_matrix(b, &q->quad);
    batch_case(b, words, quad_words(&q->quad, &h, words), 1);
    for (i = 0; i < 4; i++)
        reference_widen(&v[i], &r[i]);
    expect_primitive(r, 4, c);
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
    particle_record(t, &g_queued[0], v, b, c);
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
        particle_record(t, &g_queued[i], v[i], b, c + i);
    return n;
}

/* ---- the shaders' SPIR-V ---- */

/* the operations quad.hlsl's vertex shader has outside instances.hlsli and exact.hlsli, those of
   phase 4's quad shader: snapped_ndc's product, division and sum of both coordinates as vectors,
   then the division and subtraction of each (7), the vertex fog's product, sum and division and
   its product by q (4), QUAD_CAMERA's transform (a product, two fused multiply-adds and a sum) and
   its division by w (5), and the colour and texture coordinates times q (2) */
#define QUAD_VS_BARE 18

/* an operand by its value as far as div_rn's divisions go: |x| as x with the top bit, as dxc may
   make |x| more than once */
static uint32_t operand(const uint32_t *absOf, uint32_t bound, uint32_t id)
{
    return id < bound && absOf[id] ? (absOf[id] - 1) | 0x80000000u : id;
}

/* The operations of a vertex shader that would let the GPU part from the CPU's results now and
   then: float additions, subtractions, multiplications and divisions, of scalars or of vectors,
   without NoContraction (HLSL's precise), which the GPU may fuse, reorder or factor; a division
   that is not one of div_rn's (exact.hlsli), which Vulkan lets be 2.5 units off even where
   precise: div_rn's quotient of a by b is multiplied by b (two_product, remainder), or is the same
   division again (dxc may make one for each of its uses, and of |a| and |b| as well), and its
   correction added to such a quotient by b; a quotient that is only tested for 0 and infinity is
   div_rn's first test, of a copy whose quotient nothing reads (dxc keeps such tests); fused
   multiply-adds (mad()), which Vulkan does not
   promise to fuse; matrix and dot products (mul(), dot()), which leave the order of their sums to
   the GPU. */
static int bare_operations(const char *name, const unsigned char *code, size_t size)
{
    enum { F_ADD = 129, F_SUB = 131, F_MUL = 133, F_DIV = 136, VECTOR_TIMES_SCALAR = 142 };
    const uint32_t *w = (const uint32_t *)code;
    size_t n = size / 4, i, k;
    uint32_t bound, *decorated, *floats, *absOf, *divisor, *dividend, *quotient, *zero;
    int bare = 0, plain = 0;
    if (n < 5 || w[0] != 0x07230203u)
        return -1;
    bound = w[3];
    decorated = calloc(bound, sizeof *decorated);
    floats = calloc(bound, sizeof *floats);
    absOf = calloc(bound, sizeof *absOf);       /* an |x|'s x + 1 */
    divisor = calloc(bound, sizeof *divisor);   /* a precise division's divisor (operand()) + 1 */
    dividend = calloc(bound, sizeof *dividend); /* and its dividend (operand()) */
    quotient = calloc(bound, sizeof *quotient); /* div_rn's quotient */
    zero = calloc(bound, sizeof *zero);         /* the float constant 0 */
    if (!decorated || !floats || !absOf || !divisor || !dividend || !quotient || !zero)
        return -1;
    for (i = 5; i < n; i += w[i] >> 16) {
        uint32_t op = w[i] & 0xffff;
        if ((w[i] >> 16) == 0)
            break;
        if (op == 22 && w[i + 2] == 32 && w[i + 1] < bound)
            floats[w[i + 1]] = 1;
        if (op == 23 && w[i + 2] < bound && floats[w[i + 2]] && w[i + 1] < bound)
            floats[w[i + 1]] = 1;
        if (op == 71 && w[i + 2] == 42 && w[i + 1] < bound)
            decorated[w[i + 1]] = 1;
        if (op == 12 && w[i + 4] == 4 && w[i + 2] < bound)
            absOf[w[i + 2]] = w[i + 5] + 1;
        if (op == 43 && w[i + 2] < bound && w[i + 1] < bound && floats[w[i + 1]] && w[i + 3] == 0)
            zero[w[i + 2]] = 1;
    }
    for (i = 5; i < n; i += w[i] >> 16) {
        uint32_t op = w[i] & 0xffff;
        if ((w[i] >> 16) == 0)
            break;
        if (op == F_DIV && w[i + 2] < bound && w[i + 1] < bound && floats[w[i + 1]] && decorated[w[i + 2]]) {
            divisor[w[i + 2]] = operand(absOf, bound, w[i + 4]) + 1;
            dividend[w[i + 2]] = operand(absOf, bound, w[i + 3]);
        }
    }
    /* div_rn's quotients: multiplied by their divisor */
    for (i = 5; i < n; i += w[i] >> 16) {
        uint32_t op = w[i] & 0xffff;
        if ((w[i] >> 16) == 0)
            break;
        if (op == F_MUL)
            for (k = 0; k < 2; k++) {
                uint32_t a = w[i + 3 + k], b = w[i + 4 - k];
                if (a < bound && divisor[a] == operand(absOf, bound, b) + 1)
                    quotient[a] = 1;
            }
    }
    /* and the same divisions again */
    for (i = 0; i < bound; i++)
        if (divisor[i] && !quotient[i])
            for (k = 0; k < bound; k++)
                if (quotient[k] == 1 && divisor[k] == divisor[i] && dividend[k] == dividend[i]) {
                    quotient[i] = 2;
                    break;
                }
    for (i = 5; i < n; i += w[i] >> 16) {
        uint32_t op = w[i] & 0xffff;
        int arithmetic = op == F_ADD || op == F_SUB || op == F_MUL || op == F_DIV || op == VECTOR_TIMES_SCALAR;
        if ((w[i] >> 16) == 0)
            break;
        if (arithmetic && w[i + 1] < bound && floats[w[i + 1]] && w[i + 2] < bound && !decorated[w[i + 2]])
            bare++;
        if ((op == 12 && w[i + 4] == 50) || (op >= 143 && op <= 148))
            bare++;
        /* a precise division that is neither div_rn's quotient nor its correction, which is added
           to a quotient by the same divisor */
        if (op == F_DIV && w[i + 2] < bound && divisor[w[i + 2]] && !quotient[w[i + 2]]) {
            uint32_t r = w[i + 2];
            int correction = 0, tested = 1;
            size_t j2, m;
            for (j2 = 5; j2 < n; j2 += w[j2] >> 16) {
                uint32_t op2 = w[j2] & 0xffff, count = w[j2] >> 16;
                if (!count)
                    break;
                if (op2 == F_ADD)
                    for (k = 0; k < 2; k++) {
                        uint32_t a = w[j2 + 3 + k], b = w[j2 + 4 - k];
                        if (a == r && b < bound && quotient[b] && divisor[b] == divisor[r])
                            correction = 1;
                    }
                /* every use of it a test for infinity (OpIsInf) or 0 (OpFOrdEqual) */
                for (m = 3; m < count; m++)
                    if (w[j2 + m] == r && !(op2 == 157 || (op2 == 180 && w[j2 + 7 - m] < bound
                                                          && zero[w[j2 + 7 - m]])))
                        tested = 0;
            }
            plain += !correction && !tested;
        }
    }
    free(decorated);
    free(floats);
    free(absOf);
    free(divisor);
    free(dividend);
    free(quotient);
    free(zero);
    printf("%s: %d operations that may part from the CPU (contractible, fused or matrix), %d plain divisions\n",
           name, bare, plain);
    return bare + plain;
}

/* ---- the exact operations alone ---- */

/* div_rn, project_depth and project_point against a / b, renderer_project_depth and
   renderer_project_vertex. The first 2^23 divisions take every float b from 1 to 2 under a = 1
   (every reciprocal), the next 2^23 the same b under a random a from 1 to 2. */
static int test_operations(struct Batch *b, float *out, long count)
{
    static struct View s;
    long done = 0, differ[4] = {0, 0, 0, 0};
    int printed = 0;
    while (done < count) {
        long base = done, i;
        uint32_t matrix;
        batch_clear(b);
        make_view(&s, chance(50) ? KIND_EDGES : KIND_FUZZ);
        {
            struct GfxQuad q;
            memset(&q, 0, sizeof q);
            memcpy(q.world.view, s.view, sizeof q.world.view);
            q.world.proj[0] = s.proj[0];
            q.world.proj[1] = s.proj[1];
            q.world.centre[0] = s.centre[0];
            q.world.centre[1] = s.centre[1];
            matrix = add_matrix(b, &q);
        }
        while (done < count && b->ninstances < PER_BATCH) {
            uint32_t w[12];
            float a, d, rhw, p[3];
            if (done < (1l << 24)) {
                a = done < (1l << 23) ? 1.0f : float_of(0x3f800000u | (uint32_t)(next() & 0x7fffff));
                d = float_of(0x3f800000u + (uint32_t)(done & 0x7fffff));
            } else {
                a = spread(-30, 30, 1);
                d = spread(-30, 30, 1);
            }
            rhw = chance(50) ? spread(-16, 3, 0) : 1.0f / uniform(10.0f, 40000.0f);
            point_at(&s, uniform(-0.5f, 1.5f) * s.centre[0] * 2.0f, uniform(-0.5f, 1.5f) * s.centre[1] * 2.0f,
                     chance(10) ? uniform(-500.0f, 10.0f) : uniform(10.0f, 40000.0f), p);
            memset(w, 0, sizeof w);
            w[3] = matrix;
            w[4] = bits(a);
            w[5] = bits(d);
            w[6] = bits(rhw);
            w[8] = bits(p[0]);
            w[9] = bits(p[1]);
            w[10] = bits(p[2]);
            batch_record(b, w, 12);
            done++;
        }
        probe_run(b, 1, out);
        for (i = 0; i < done - base; i++) {
            const float *g = out + (size_t)i * 4 * 16;
            const uint32_t *w = b->words + (size_t)b->offsets[i] * 4;
            struct TexDrawVertex v;
            Vec3 point;
            float want[6];
            int f;
            point.x = float_of(w[8]);
            point.y = float_of(w[9]);
            point.z = float_of(w[10]);
            renderer_project_vertex(&s.renderer, &point, s.view, &v);
            want[0] = float_of(w[4]) / float_of(w[5]);
            want[1] = renderer_project_depth(float_of(w[6]));
            want[2] = v.x;
            want[3] = v.y;
            want[4] = v.z;
            want[5] = v.rhw;
            for (f = 0; f < 6; f++) {
                float got = f < 2 ? g[f] : g[4 + f - 2];
                if (same(want[f], got))
                    continue;
                differ[f < 2 ? f : f < 5 ? 2 : 3]++;
                if (printed++ < 12)
                    printf("  operation %d: inputs %.9g %.9g %.9g | %.9g %.9g %.9g: cpu %.9g gpu %.9g\n", f,
                           float_of(w[4]), float_of(w[5]), float_of(w[6]), point.x, point.y, point.z, want[f], got);
            }
        }
    }
    printf("operations: %ld inputs each (the first 2^24 divisions over every b from 1 to 2); differ: a / b %ld, "
           "renderer_project_depth %ld, renderer_project_vertex x y z %ld, rhw %ld\n",
           count, differ[0], differ[1], differ[2], differ[3]);
    return !differ[0] && !differ[1] && !differ[2] && !differ[3];
}

/* ---- records of every kind ---- */

static int test_records(struct Batch *b, struct Case *cases, float *out, long perKind)
{
    enum { SPRITES, SCREEN, CAMERA, WORLD, PARTICLES, SPRITE_PARTICLES, WHAT_COUNT };
    static const char *const names[WHAT_COUNT] = {"sprites", "screen vertices", "camera vertices", "projected points",
                                                  "particles", "sprite particles"};
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
                    else if (what == SCREEN || what == CAMERA)
                        n += screen_case(kind, what == CAMERA, b, &cases[n]);
                    else if (what == WORLD)
                        n += world_case(kind, &s, b, &cases[n]);
                    else if (what == PARTICLES)
                        n += particle_case(kind, &s, b, &cases[n], t);
                    else
                        n += sprite_particle_case(kind, &s, b, &cases[n], t);
                    if (what >= WORLD && n % 64 == 0)
                        make_view(&s, kind);
                }
                g_renderer = client;
                probe_run(b, 0, out);
                for (i = 0; i < n; i++) {
                    int c;
                    t->instances++;
                    for (c = 0; c < 4; c++) {
                        char label[96];
                        t->vertices++;
                        snprintf(label, sizeof label, "%s, %s, corner %d", names[what], g_kindNames[kind], c);
                        compare(t, 0, out + ((size_t)i * 8 + (size_t)c) * 16, &cases[i].made[c], label);
                        compare(t, 1, out + ((size_t)i * 8 + 4 + (size_t)c) * 16, &cases[i].snapped[c], label);
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

static gfx_surface *page_texture(int layered, unsigned salt)
{
    gfx_surface *t = layered ? gfx_create_layer_texture(256, 256, GFX_FMT_8888)
                             : gfx_create_texture(256, 256, GFX_FMT_8888);
    struct gfx_lock lk;
    int x, y;
    need(t && gfx_lock(t, NULL, &lk) == 0, "page texture");
    for (y = 0; y < 256; y++)
        for (x = 0; x < 256; x++) {
            uint32_t c = (uint32_t)(x * 73856093u ^ y * 19349663u ^ salt * 83492791u) * 2654435761u;
            /* a quarter see-through, a quarter half */
            c = (c >> 8) | ((x / 8 + y / 8) % 4 == 0 ? 0 : (x / 8 + y / 8) % 4 == 1 ? 0x80000000u : 0xff000000u);
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &c, 4);
        }
    gfx_unlock(t);
    return t;
}

static void read_picture(uint32_t *out)
{
    need(gfx_read_frame(NULL, out, PICTURE_W * 4) == 0, "gfx_read_frame");
}

static long picture_differences(const uint32_t *a, const uint32_t *b)
{
    long i, n = 0;
    for (i = 0; i < PICTURE_W * PICTURE_H; i++)
        n += a[i] != b[i];
    return n;
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
   vertices. */
static int test_pictures(int scenes)
{
    static uint32_t a[PICTURE_W * PICTURE_H], c[PICTURE_W * PICTURE_H];
    static struct TexDrawVertex verts[512][4];
    static struct GfxQuad quads[512];
    static int pages[512];
    gfx_surface *layered[2], *plain[2];
    long differ[4] = {0, 0, 0, 0}, covered = 0;
    int scene, ok = 1, i, side;
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
                        gfx_draw_vertices(verts[i], 4, 1, 0);
                    else
                        gfx_draw_quad(&quads[i]);
                }
                gfx_set_texture(0, NULL);
                gfx_end_scene();
                read_picture(side ? c : a);
            }
            differ[m] += picture_differences(a, c);
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
                        gfx_draw_vertices(verts[i], 4, 1, 0);
                    else
                        gfx_draw_quad(&quads[i]);
                }
                gfx_set_texture(0, NULL);
                gfx_end_scene();
                read_picture(side ? c : a);
            }
            differ[2 + m] += picture_differences(a, c);
            for (i = 0; i < PICTURE_W * PICTURE_H; i++)
                covered += a[i] != 0xff203040u;
        }
    }
    for (i = 0; i < 2; i++) {
        gfx_release(layered[i]);
        gfx_release(plain[i]);
    }
    printf("pictures through render/gfx_gpu.c: %d scenes of %d by %d, %ld pixels drawn; differ: sprites blended %ld, "
           "alpha-tested %ld; projected quads additive %ld, alpha-tested %ld\n",
           scenes, PICTURE_W, PICTURE_H, covered, differ[0], differ[1], differ[2], differ[3]);
    for (i = 0; i < 4; i++)
        ok = ok && !differ[i];
    return ok;
}

int main(int argc, char **argv)
{
    static struct Renderer renderer;
    struct C3dDevice *d = &g_3dDevice;
    uint64_t seed = argc > 2 ? strtoull(argv[2], NULL, 0) : 20261112u;
    long perKind = argc > 3 ? strtol(argv[3], NULL, 0) : 40000;
    long operations = argc > 4 ? strtol(argv[4], NULL, 0) : (1l << 24) + 1000000;
    int scenes = argc > 5 ? (int)strtol(argv[5], NULL, 0) : 12, ok = 1, bare;
    struct Batch batch;
    struct Case *cases = malloc(sizeof *cases * PER_BATCH);
    float *out = malloc(sizeof(float) * 16 * 4 * (size_t)PER_BATCH);

    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc > 1)
        SDL_setenv_unsafe("RAGEXE_MSAA", argv[1], 1);
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

    /* instances.hlsli and exact.hlsli, alone in the probe, keep every rule; quad.hlsl's vertex
       shader adds to them only what phase 4's quad shader had too (QUAD_VS_BARE) */
    ok = bare_operations("tests/shaders/quad_probe.hlsl", quad_probe_vs.spirv, quad_probe_vs.spirvSize) == 0;
    bare = bare_operations("render/shaders/quad.hlsl", quad_vs.spirv, quad_vs.spirvSize);
    ok = ok && bare == QUAD_VS_BARE;

    probe_init();
    if (operations > 0)
        ok = test_operations(&batch, out, operations) && ok;
    if (perKind > 0)
        ok = test_records(&batch, cases, out, perKind) && ok;
    probe_free();
    if (scenes > 0)
        ok = test_pictures(scenes) && ok;

    gfx_close(d);
    SDL_DestroyWindow(d->hwnd);
    SDL_Quit();
    printf("quad_instances: %s\n", ok ? "every vertex and pixel matches" : "DIFFERENCES");
    return ok ? 0 : 1;
}
