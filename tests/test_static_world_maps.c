/* custom: the static world (render/static_world.h, world/world_gpu.c, render/gfx.h's gfx_world_*)
   on real maps from a game folder (argument 1, the folder that holds data.grf; skipped without
   one), against the draws the Direct3D 9 client's code made of them, and the GPU's passes against
   the C.

   The maps load through the client's own code: the GRF reader and the resource manager, the
   ground (world/ground_load.c) and the models (world/model_load.c, world/models.c, streamed in as
   the loader thread does) and the scene graph. The static world is built by the client's
   world/world_gpu.c, as view_on_render builds it, into a device that keeps what it is given
   (gfx_world_* below): the near models first, then the streamed ones appended. Textures are the
   texture cache's as the client fills it, sized from the game's own bitmaps.

   The reference is the code that queued and drew the ground's and the models' meshes before the
   static world, kept here as it was (the ground's blocks and water, the models' nodes, the walk of
   the scene graph's leaves, the renderer's queues and their sort): the leaves the view reaches,
   their models ticked as the walk ticked them and drawn with their nodes' matrices as the first
   tick of the frame left them, each node's faces as one mesh in runs of one texture and one
   two-sidedness, the ground's blocks under the leaves in runs of one texture and lightmap page, the
   casters it chose, the water quads it built. Each pose's frame is run twice from the same start
   (pose_models; the test keeps the clock, and every tick moves it on by a millisecond, so that a
   model ticked twice in a frame takes two times): once by the client's view_on_render, once by
   the reference. For 12 camera poses on each map:
   - every record the view's cull keeps (sw_record_in_view) that the reference draws has the same
     vertices, textures, lightmap, cull, texture-coordinate matrix and kind, and its transform slot's
     view is the reference's matrix to the bit: the pose the client gave the device is the one the
     model's first tick of the frame left;
   - every draw the reference makes that the cull drops is outside the view: each of its triangles,
     clipped to the screen and the depth range, keeps no area; every record the cull keeps that the
     reference does not draw is measured: whether any of its triangles is in the view, and why the
     reference left it out;
   - no record the view keeps belongs to a model that animates which the frame did not tick, and the
     frame ticks no model beyond the reference's but one that animates and held a record in view;
   - the casters the client queues are the reference's, in its order, and the cascades' cull
     (sw_cull_cascade, part 2 of the plan's phase 5) is measured against them;
   - the alpha-blended records the client queues are those the records' cull keeps, keyed to the
     bit as the reference keyed its alpha draws, and the renderer draws them in the reference's
     order, ties included;
   - the water cells the client queues hold every quad the reference built, keyed to the bit;
   - the order: the ground's and the opaque passes' sequences of texture orders; for the opaque
     draws, the pairs whose order can change pixels (triangles in one plane that overlap) and how
     often the reference's order and the records' differ for them.
   On the first map a skill's model is built still as the walk meets it, set going, given up (its
   nodes poisoned: nothing of it may be read again), and given up again and again until the world
   is built again without the dead records, as it was; the world is built again after the device
   made its own again, and from the textures' new surfaces after a reload; the costs of a model
   appearing and of the records sent are measured. On this machine's GPU, where there is one, with
   the device's own packing (render/world_layout.h): the transform pass (world_transform.hlsl) gives
   every slot the C's chain to the bit, view, clip space and place in the world; the cull and
   compaction passes (world_cull.hlsl, world_compact.hlsl) put into each run exactly the indices of
   the records sw_record_in_view keeps, in their order, and do the same with their dispatches split
   into rows of a few groups, as a dispatch of more groups than one row takes is, and the two are
   timed on the map's first view; the water's quads (water.hlsli, through
   tests/shaders/water_probe.hlsl) are the reference's to the bit in heights, rhw, screen points and
   whether drawn, for every cell of the map; and the SPIR-V of those passes has no operation that
   may part from the C.

   With `--survey` after the folder it surveys the sizes of every map's textures and lightmap pages
   instead (all the .rsw files in data.grf and data\), and the texture arrays a map needs with one
   array a size. */

#include "render/static_world.h"
#include "render/world_layout.h"

#include "actor/sprite_quad.h"
#include "diag/perf.h"
#include "fileio/file.h"
#include "fileio/res_mgr.h"
#include "fileio/resource_api.h"
#include "fileio/resource_errors.h"
#include "navi/grid_uv.h"
#include "platform/filesystem.h"
#include "render/device.h"
#include "render/math3d.h"
#include "render/render_api.h"
#include "render/renderer.h"
#include "render/rp_pool.h"
#include "render/shaders/shader_code.h"
#include "render/shaders/world_cull.h"
#include "render/shaders/world_compact.h"
#include "render/shaders/world_transform.h"
#include "render/texture.h"
#include "render/texture_cache.h"
#include "render/texture_globals.h"
#include "resources/bitmap_types.h"
#include "resources/gnd.h"
#include "resources/model_res.h"
#include "resources/world_res.h"
#include "std/list.h"
#include "std/std_sort.h"
#include "shaders/water_probe.h"
#include "ui/window_manager_state.h"
#include "ui/window_messages.h"
#include "world/ground.h"
#include "world/hover_cell.h"
#include "world/model.h"
#include "world/model_node_util.h"
#include "world/models.h"
#include "world/scene_graph.h"
#include "world/state.h"
#include "world/view_api.h"
#include "world/view_types.h"
#include "world/world_gpu.h"
#include "platform/clock.h"

#include <SDL3/SDL.h>
#include <direct.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FAIL(...) (fprintf(stderr, __VA_ARGS__), fputc('\n', stderr), ++g_failures)

static int g_failures;

static void *grow(void *p, size_t *capacity, size_t needed, size_t size)
{
    size_t c = *capacity;

    if (needed <= c)
        return p;
    c = c ? c : 64;
    while (c < needed)
        c *= 2;
    p = realloc(p, c * size);
    if (!p)
        abort();
    *capacity = c;
    return p;
}

/* ---------------------------------------------------------------------------------------------
   The device: keeps what the client gives it of the static world (render/gfx.h) */

struct gfx_surface {
    struct Texture *texture;
};

#define MAX_ARRAYS 255
#define MAX_LAYERS GFX_WORLD_LAYERS

static struct {
    int generation;
    struct GfxMeshVertex *vertices;
    uint32_t *ids;
    WORD *indices;
    size_t vertexCapacity, idCapacity, indexCapacity;
    uint32_t vertexCount, indexCount;
    struct SwPlacement *places;  /* in the order the meshes came */
    size_t placeCount, placeCapacity;
    struct Texture *layers[MAX_ARRAYS][MAX_LAYERS];
    struct SwRecord *records;    /* the last list given, in draw order */
    size_t recordCount, recordCapacity, idCount;
    /* the arrays declared, each's size */
    int arrayCount;
    int arraySize[MAX_ARRAYS][2];
    int recordCalls;
    struct GfxWorldSlot *slots;
    size_t slotCount, slotCapacity;
    struct GfxWaterCell *cells;
    size_t cellCount, cellCapacity;
    float sines[360];
    /* the frame */
    struct GfxWorldView camera;
    struct SwFrustum frustum;
    float (*animated)[12];
    size_t animatedCount, animatedCapacity;
    float (*uv)[6];
    size_t uvCount, uvCapacity;
    struct GfxWaterWave *waves;
    size_t waveCount, waveCapacity;
    int frames;
} D;

int gfx_world_generation(void) { return D.generation; }

void gfx_world_clear(void)
{
    D.generation++;
    D.vertexCount = D.indexCount = 0;
    D.placeCount = D.recordCount = D.slotCount = D.cellCount = D.idCount = 0;
    D.animatedCount = D.uvCount = D.waveCount = 0;
    D.arrayCount = 0;
    memset(D.layers, 0, sizeof D.layers);
}

int gfx_world_mesh(const struct SwPlacement *place, const struct GfxMeshVertex *verts, const uint32_t *ids,
                   const WORD *indices)
{
    uint32_t v = place->baseVertex + place->vertexCount, i = place->firstIndex + place->indexCount;

    D.vertices = grow(D.vertices, &D.vertexCapacity, v, sizeof *D.vertices);
    D.ids = grow(D.ids, &D.idCapacity, v, sizeof *D.ids);
    D.indices = grow(D.indices, &D.indexCapacity, i, sizeof *D.indices);
    memcpy(D.vertices + place->baseVertex, verts, place->vertexCount * sizeof *verts);
    memcpy(D.ids + place->baseVertex, ids, place->vertexCount * sizeof *ids);
    memcpy(D.indices + place->firstIndex, indices, place->indexCount * sizeof *indices);
    D.vertexCount = v > D.vertexCount ? v : D.vertexCount;
    D.indexCount = i > D.indexCount ? i : D.indexCount;
    D.places = grow(D.places, &D.placeCapacity, D.placeCount + 1, sizeof *D.places);
    D.places[D.placeCount++] = *place;
    return 1;
}

int gfx_get_levels(gfx_surface *s)
{
    (void)s;
    return 1;
}

int gfx_world_array(uint8_t array, gfx_surface *like)
{
    if (!like || array != D.arrayCount || array >= MAX_ARRAYS) {
        FAIL("an array declared out of turn (%d, %d declared)", array, D.arrayCount);
        return 0;
    }
    D.arraySize[array][0] = like->texture->base.width;
    D.arraySize[array][1] = like->texture->base.height;
    D.arrayCount++;
    return 1;
}

int gfx_world_texture(gfx_surface *texture, uint8_t array, uint16_t layer)
{
    if (!texture || array >= D.arrayCount || layer >= MAX_LAYERS)
        return 0;
    if (texture->texture->base.width != D.arraySize[array][0] || texture->texture->base.height != D.arraySize[array][1])
        FAIL("a texture placed in an array of another size");
    if (D.layers[array][layer])
        FAIL("a texture placed where another lies (array %d, layer %d)", array, layer);
    D.layers[array][layer] = texture->texture;
    return 1;
}

int gfx_world_records(const struct SwRecord *records, size_t count, uint32_t ids)
{
    size_t i;

    for (i = 0; i < count; i++)
        if (records[i].id >= ids || (i && !sw_record_before(&records[i - 1], &records[i])))
            FAIL("the records given are not in draw order below their ids");
    D.records = grow(D.records, &D.recordCapacity, count + 1, sizeof *D.records);
    memcpy(D.records, records, count * sizeof *records);
    D.recordCount = count;
    D.idCount = ids;
    D.recordCalls++;
    return 1;
}

int gfx_world_slots(const struct GfxWorldSlot *slots, size_t first, size_t count)
{
    D.slots = grow(D.slots, &D.slotCapacity, first + count + 1, sizeof *D.slots);
    memcpy(D.slots + first, slots, count * sizeof *slots);
    D.slotCount = first + count > D.slotCount ? first + count : D.slotCount;
    return 1;
}

int gfx_world_water(const struct GfxWaterCell *cells, size_t count, const float *sines)
{
    D.cells = grow(D.cells, &D.cellCapacity, count + 1, sizeof *D.cells);
    memcpy(D.cells, cells, count * sizeof *cells);
    D.cellCount = count;
    memcpy(D.sines, sines, sizeof D.sines);
    return 1;
}

void gfx_world_frame(const struct GfxWorldFrame *frame)
{
    D.camera = frame->camera;
    D.frustum = frame->frustum;
    D.animated = grow(D.animated, &D.animatedCapacity, frame->animatedCount + 1, sizeof *D.animated);
    memcpy(D.animated, frame->animated, frame->animatedCount * sizeof *D.animated);
    D.animatedCount = frame->animatedCount;
    D.uv = grow(D.uv, &D.uvCapacity, frame->uvCount + 1, sizeof *D.uv);
    memcpy(D.uv, frame->uv, frame->uvCount * sizeof *D.uv);
    D.uvCount = frame->uvCount;
    D.waves = grow(D.waves, &D.waveCapacity, frame->waveCount + 1, sizeof *D.waves);
    memcpy(D.waves, frame->waves, frame->waveCount * sizeof *D.waves);
    D.waveCount = frame->waveCount;
    D.frames++;
}

int gfx_get_size(gfx_surface *s, int *w, int *h)
{
    *w = s->texture->base.width;
    *h = s->texture->base.height;
    return 0;
}

/* what render/crenderer.c draws with, which the test does not draw */
void gfx_world_draw(int pass, const struct GfxWorldDraw *draw) { (void)pass; (void)draw; }
void gfx_world_draw_record(const struct SwRecord *record, const struct GfxWorldDraw *draw) { (void)record; (void)draw; }
void gfx_world_draw_water(const uint32_t *cells, size_t count, DWORD colour) { (void)cells; (void)count; (void)colour; }
void gfx_world_cast(uint32_t firstIndex, uint32_t count, uint32_t baseVertex, uint8_t textureArray, uint32_t ids)
{
    (void)firstIndex; (void)count; (void)baseVertex; (void)textureArray; (void)ids;
}
void gfx_set_world_camera(const struct GfxWorldView *view) { (void)view; }
void gfx_set_texture(int stage, gfx_surface *s) { (void)stage; (void)s; }
void gfx_set_mode(int mode) { (void)mode; }
void gfx_set_effect(int srcBlend, int destBlend, int combine) { (void)srcBlend; (void)destBlend; (void)combine; }
void gfx_set_fog(const struct GfxFog *fog) { (void)fog; }
void gfx_draw_quad(const struct GfxQuad *quad) { (void)quad; }
void gfx_draw_vertices(const struct TexDrawVertex *v, int n, int strip, int world)
{
    (void)v; (void)n; (void)strip; (void)world;
}
int gfx_begin_scene(void) { return 0; }
void gfx_end_scene(void) {}
void gfx_addref(gfx_surface *s) { (void)s; }
void gfx_release(gfx_surface *s) { (void)s; }
int gfx_shadows_supported(void) { return 1; }
int gfx_shadow_begin(const struct GfxShadow *shadow) { (void)shadow; return 1; }
int gfx_shadow_cascade(int cascade) { (void)cascade; return 1; }
void gfx_shadow_end(void) {}
void gfx_shadow_receive(int receiver) { (void)receiver; }

struct gfx_caps g_gfxCaps;

struct perf_counters g_perf;
void perf_begin(int phase) { (void)phase; }
void perf_end(int phase) { (void)phase; }
void perf_overlay_draw(struct Renderer *r) { (void)r; }
void perf_frame_end(struct Renderer *r) { (void)r; }

/* ---------------------------------------------------------------------------------------------
   Textures: the texture cache (render/tex_cache.c) as it fills, sized from the game's bitmaps */

struct Texture g_whiteTexture;
struct TexCache g_textureCache;
int g_textureDivisor = 1;

#define CACHE_SLOTS (1u << 17)
static struct Texture *g_cached[CACHE_SLOTS];
static size_t g_cachedCount;
static unsigned g_serial;

static void fit(int req, int have, int *out)
{
    unsigned v = (unsigned)req;

    while (v > (unsigned)have)
        v >>= 1;
    *out = (int)v;
}

/* the size the device gives an image (render/c3ddevice.c, c3d_device_clamp_texture_size): each side
   the power of two at or above it, at most 8192 */
static uint16_t power_of_two(int v)
{
    int p = 1;

    while (p < v && p < 8192)
        p <<= 1;
    return (uint16_t)p;
}

static struct Texture *new_texture(int w, int h, int format)
{
    struct Texture *t = calloc(1, sizeof *t);
    struct gfx_surface *s = calloc(1, sizeof *s);
    uint16_t size[2];

    if (!t || !s)
        abort();
    size[0] = power_of_two(w);
    size[1] = power_of_two(h);
    t->base.width = size[0];
    t->base.height = size[1];
    t->base.surface = s;
    t->format = format;
    t->serial = ++g_serial;
    fit(w, t->base.width, &t->reqWidth);
    fit(h, t->base.height, &t->reqHeight);
    s->texture = t;
    return t;
}

static unsigned name_slot(const char *name)
{
    unsigned h = 2166136261u;

    while (*name)
        h = (h ^ (unsigned char)*name++) * 16777619u;
    return h & (CACHE_SLOTS - 1);
}

struct Texture *texture_cache_get(struct TexCache *cache, const char *name, int keyed, int full_size)
{
    struct BitmapRes *res;
    struct Texture *t;
    unsigned slot;

    (void)keyed;
    (void)full_size;
    if (!cache || !name || strlen(name) >= sizeof g_whiteTexture.name)
        return &g_whiteTexture;
    for (slot = name_slot(name); g_cached[slot]; slot = (slot + 1) & (CACHE_SLOTS - 1))
        if (!strcmp(g_cached[slot]->name, name))
            return g_cached[slot];
    if (g_cachedCount + 1 >= CACHE_SLOTS)
        abort();
    res = res_mgr_get(g_resMgr(), name);
    if (!res) {
        t = new_texture(1, 1, 0);
    } else if (res->width <= 0 || res->height <= 0) {
        res_mgr_release(g_resMgr(), &res->res);
        return &g_whiteTexture;
    } else {
        t = new_texture(res->width, res->height, res->hasAlpha != 0);
        res_mgr_release(g_resMgr(), &res->res);
    }
    strcpy(t->name, name);
    g_cached[slot] = t;
    g_cachedCount++;
    return t;
}

struct Texture *texture_from_pixels(int w, int h, const DWORD *bits, int format, int keyed, int full_size)
{
    (void)bits;
    (void)keyed;
    (void)full_size;
    return w > 0 && h > 0 ? new_texture(w, h, format) : NULL;
}

void surface_destroy(struct Surface *surface)
{
    struct Texture *t = (struct Texture *)surface;

    free(surface->surface);
    free(t);
}

/* ---------------------------------------------------------------------------------------------
   What view_on_render and the code it calls reach beyond the world and the renderer */

struct World g_world;
struct World *game_current_world(void) { return &g_world; }
struct Weather g_weather;
struct Session g_session1;
struct NaviManager g_navi;
struct EffectMgr *g_effectMgr;
int g_playerBlind, g_isSkipFrame, g_isFixedCamera, g_hoverCellX, g_hoverCellY, g_hoverCellHidden = 1;
float g_fixedViewDistanceMax = 1000.0f;
unsigned g_hoverCellColor;
float g_spriteLayerDepthStep, g_spriteScaleDepthBias;
const float g_naviCellUVs[8];
int g_lightmap;
struct C3dDevice g_3dDevice;
struct UIWindowMgr g_windowMgr;
void navi_manager_draw_path(struct NaviManager *self, void *view) { (void)self; (void)view; }
void effect_mgr_render(struct EffectMgr *self) { (void)self; }
void sprite_atlas_initialize(struct Renderer *r) { (void)r; }
void sprite_atlas_destroy(struct Renderer *r) { (void)r; }
void sprite_atlas_finish_frame(struct Renderer *r) { (void)r; }
void error_msg(const char *text, int flags)
{
    (void)flags;
    fprintf(stderr, "message box: %s\n", text);
}
intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, intptr_t val1, intptr_t val2, intptr_t val3,
                                 intptr_t val4)
{
    (void)self; (void)msg; (void)val1; (void)val2; (void)val3; (void)val4;
    return 0;
}
void c3d_device_save_frame(struct C3dDevice *self) { (void)self; }
int c3d_device_clear(struct C3dDevice *self, DWORD colour) { (void)self; (void)colour; return 1; }
int c3d_device_clear_z(struct C3dDevice *self) { (void)self; return 1; }
void c3d_device_restore_frame(struct C3dDevice *self) { (void)self; }
int c3d_device_show_frame(struct C3dDevice *self, char vsync) { (void)self; (void)vsync; return 1; }

/* The clock (platform/clock.h), all of it, in place of the platform's: the models' ticks read
   g_now, which each tick moves on by a millisecond (tick_logged), so that a model ticked twice in a
   frame takes two times, as in the client when the clock moves during the walk; the stopwatch and
   the date are real. */
static uint32_t g_now = 100000;
uint32_t platform_ticks_ms(void) { return g_now; }
uint32_t platform_coarse_ticks_ms(void) { return g_now; }
int64_t platform_perf_counter(void) { return (int64_t)SDL_GetPerformanceCounter(); }
int64_t platform_perf_frequency(void) { return (int64_t)SDL_GetPerformanceFrequency(); }
int64_t platform_stopwatch_ns(void) { return (int64_t)SDL_GetTicksNS(); }
unsigned platform_timer_period_begin(void) { return 0; }
void platform_timer_period_end(unsigned period) { (void)period; }
int32_t platform_time_now(void) { return (int32_t)time(NULL); }
struct tm *platform_time_local(int32_t seconds)
{
    time_t t = seconds;
    return localtime(&t);
}
int32_t platform_time_make(struct tm *local) { return (int32_t)mktime(local); }
double platform_time_diff(int32_t end, int32_t start) { return (double)end - (double)start; }
const char *platform_time_text(int32_t seconds)
{
    time_t t = seconds;
    return ctime(&t);
}
void platform_date_time_local(struct PlatformDateTime *out) { memset(out, 0, sizeof *out); }
void platform_date_time_utc(struct PlatformDateTime *out) { memset(out, 0, sizeof *out); }

/* The models' ticks (ModelActorOps.tick), through copies of their operations: each moves the clock
   on, and while g_logTicks marks the model ticked in frame g_tickFrame (mark slot 2); the model
   g_watched has its animated nodes' matrices taken after its first tick of a frame. */
static struct ModelActorOps g_opsCopy[2];
static const struct ModelActorOps *g_realOps[2];
static int g_logTicks, g_tickFrame;
static int *mark(const void *what, int which);
static struct C3dActor *g_watched;
static int g_watchedFrame = -1;
static float g_watchedPose[256][12];
static size_t g_watchedCount;

static int node_has_keys(const struct C3dNode *node, int version)
{
    if (version == 1)
        return ((const struct ModelResNodeV1 *)node->resource)->rotKeyCount > 1;
    {
        const struct ModelResNodeV2 *r = (const struct ModelResNodeV2 *)node->resource;

        return r->scaleKeyCount > 1 || r->rotKeyCount > 1 || r->posKeyCount > 1;
    }
}

/* the matrices of a model's nodes with keys, depth first, as world/world_gpu.c gives them slots */
static void keyed_matrices(const struct C3dNode *node, int version, int depth, float (*out)[12], size_t *count)
{
    size_t child;

    if (depth >= GFX_WORLD_CHAIN)
        return;
    if (node_has_keys(node, version) && *count < 256)
        memcpy(out[(*count)++], node->matrix, sizeof out[0]);
    for (child = 0; child < node->childCount; child++)
        keyed_matrices(node->children[child], version, depth + 1, out, count);
}

static void actor_keyed(const struct C3dActor *a, float (*out)[12], size_t *count)
{
    *count = 0;
    if (a->ops->version == 1) {
        keyed_matrices(((const struct C3dActorV1 *)a)->root, 1, 1, out, count);
    } else {
        const struct C3dActorV2 *v2 = (const struct C3dActorV2 *)a;
        size_t i;

        for (i = 0; i < v2->nodeCount; i++)
            keyed_matrices(v2->nodes[i], 2, 1, out, count);
    }
}

static void tick_logged(struct C3dActor *a)
{
    g_now++;
    g_realOps[a->ops->version - 1]->tick(a);
    if (g_logTicks)
        *mark(a, 2) = g_tickFrame;
    if (a == g_watched && g_logTicks && g_watchedFrame != g_tickFrame) {
        g_watchedFrame = g_tickFrame;
        actor_keyed(a, g_watchedPose, &g_watchedCount);
    }
}

static void wrap_ops(struct C3dActor *a)
{
    int v = a->ops->version - 1;

    if (a->ops == &g_opsCopy[v])
        return;
    if (!g_realOps[v]) {
        g_realOps[v] = a->ops;
        g_opsCopy[v] = *a->ops;
        g_opsCopy[v].tick = tick_logged;
    }
    a->ops = &g_opsCopy[v];
}

/* the resources the maps do not use */
struct Res *animation_resource_create(void) { return NULL; }
struct Res *rma_resource_create(void) { return NULL; }
struct Res *emblem_resource_create(void) { return NULL; }
struct Res *effect_resource_create(void) { return NULL; }
struct Res *imf_resource_create(void) { return NULL; }
struct Res *granny_model_resource_create(void) { return NULL; }
struct Res *palette_resource_create(void) { return NULL; }
struct Res *sprite_resource_create(void) { return NULL; }
struct Res *wave_resource_create(void) { return NULL; }
struct Res *animated_model_resource_create(void) { return NULL; }

/* a texture the models hold stays in the cache */
void texture_lock(struct Texture *t) { (void)t; }
void texture_unlock(struct Texture *t) { (void)t; }
void ui_tex_cache_release(struct TexCache *cache, struct Texture *t) { (void)cache; (void)t; }

/* ---------------------------------------------------------------------------------------------
   The reference's meshes: a model node's faces as world/model_node_render.c built its mesh, a
   ground block's as world/ground_draw.c built it (now c3d_ground15_block_mesh, unchanged) */

/* a run of a node's faces with one texture and one two-sidedness; face is the first of them */
struct RefRun {
    int first, count, face;
    WORD texture;
    BYTE twoSided;
};

struct RefMesh {
    struct GfxMeshVertex *verts;
    WORD *indices;
    int nverts, nindices;
    struct RefRun *runs;           /* a node's */
    struct GndBlockRun *blockRuns; /* or a block's */
    int runCount;
};

struct RefFace {
    int face;
    int key[2];
};

static int ref_face_less(const void *a, const void *b)
{
    const struct RefFace *x = a, *y = b;
    int i;

    for (i = 0; i < 2; i++)
        if (x->key[i] != y->key[i])
            return x->key[i] < y->key[i] ? -1 : 1;
    return x->face < y->face ? -1 : x->face > y->face;
}

static BYTE ref_light_share(const struct C3dNode *node, const struct AnimMeshFace *face, int corner)
{
    const struct AnimMesh *mesh = node->mesh;
    int shadeType = node->actor ? node->actor->shadeType : 0;

    if (shadeType != 1 && shadeType != 2)
        return 0;
    return mesh->texVertCount && face->tv[corner] < mesh->texVertCount && mesh->texVerts[face->tv[corner]].colour != 0
               ? 128
               : 255;
}

/* model_node_render.c's gpu_build: NULL where it built none */
static struct RefMesh *ref_node_mesh(struct C3dNode *node)
{
    struct AnimMesh *mesh = node->mesh;
    struct RefMesh *g;
    struct RefFace *list;
    int f, i, k, n;

    if (!mesh || !mesh->faces || !mesh->positions || mesh->faceCount <= 0 || mesh->faceCount > 0x10000 / 3
        || !node->faceInfo || !node->groundLight)
        return NULL;
    n = mesh->faceCount;
    for (f = 0; f < n; f++)
        for (k = 0; k < 3; k++)
            if (mesh->faces[f].v[k] >= mesh->vertexCount)
                return NULL;
    g = calloc(1, sizeof *g);
    list = malloc((size_t)n * sizeof *list);
    if (!g || !list)
        abort();
    g->verts = malloc((size_t)n * 3 * sizeof *g->verts);
    g->indices = malloc((size_t)n * 3 * sizeof *g->indices);
    g->runs = calloc((size_t)n, sizeof *g->runs);
    if (!g->verts || !g->indices || !g->runs)
        abort();
    for (f = 0; f < n; f++) {
        list[f].face = f;
        list[f].key[0] = mesh->faces[f].texture;
        list[f].key[1] = mesh->faces[f].twoSided != 0;
    }
    qsort(list, (size_t)n, sizeof *list, ref_face_less);
    for (i = 0; i < n; i++) {
        const struct AnimMeshFace *face = &mesh->faces[list[i].face];
        const struct FaceInfo *info = &node->faceInfo[list[i].face];
        const int *li = node->groundLight + list[i].face * 9;
        const Vec3 *a = &mesh->positions[face->v[0]], *b = &mesh->positions[face->v[1]];
        const Vec3 *c = &mesh->positions[face->v[2]];
        float e1[3] = {b->x - a->x, b->y - a->y, b->z - a->z}, e2[3] = {c->x - a->x, c->y - a->y, c->z - a->z};
        float nrm[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
        float len = sqrtf(nrm[0] * nrm[0] + nrm[1] * nrm[1] + nrm[2] * nrm[2]);

        for (k = 0; k < 3 && len > 0.0f; k++)
            nrm[k] /= len;
        for (k = 0; k < 3; k++) {
            struct GfxMeshVertex *v = &g->verts[i * 3 + k];
            const Vec3 *p = &mesh->positions[face->v[k]];

            v->x = p->x;
            v->y = p->y;
            v->z = p->z;
            v->nx = nrm[0];
            v->ny = nrm[1];
            v->nz = nrm[2];
            v->diffuse = (info->colour[k] & 0x00ffffff) | (DWORD)ref_light_share(node, face, k) << 24;
            v->colour2 = pack(0xff, (BYTE)li[k * 3], (BYTE)li[k * 3 + 1], (BYTE)li[k * 3 + 2]);
            v->u0 = info->uv[k][0];
            v->v0 = info->uv[k][1];
            v->u1 = v->v1 = 0.0f;
            g->indices[i * 3 + k] = (WORD)(i * 3 + k);
        }
        if (i == 0 || list[i - 1].key[0] != list[i].key[0] || list[i - 1].key[1] != list[i].key[1]) {
            struct RefRun *d = &g->runs[g->runCount++];

            d->texture = face->texture;
            d->twoSided = (BYTE)list[i].key[1];
            d->first = i * 3;
            d->face = list[i].face;
        }
        g->runs[g->runCount - 1].count += 3;
    }
    g->nverts = n * 3;
    g->nindices = n * 3;
    free(list);
    return g;
}

static struct RefMesh *ref_block_mesh(struct C3dGround15 *ground, int bx, int by)
{
    struct RefMesh *g = calloc(1, sizeof *g);
    int count;

    if (!g)
        abort();
    g->verts = malloc((size_t)GND_BLOCK_SURFACES * 4 * sizeof *g->verts);
    g->indices = malloc((size_t)GND_BLOCK_SURFACES * 6 * sizeof *g->indices);
    g->blockRuns = malloc((size_t)GND_BLOCK_SURFACES * sizeof *g->blockRuns);
    if (!g->verts || !g->indices || !g->blockRuns)
        abort();
    g->runCount = c3d_ground15_block_mesh(ground, bx, by, g->verts, g->indices, &count, g->blockRuns);
    g->nindices = count;
    g->nverts = count / 6 * 4;
    return g;
}

static void ref_mesh_free(struct RefMesh *g)
{
    if (!g)
        return;
    free(g->verts);
    free(g->indices);
    free(g->runs);
    free(g->blockRuns);
    free(g);
}

/* a node's reference mesh, made once */
#define NODE_SLOTS (1u << 18)
static struct {
    const struct C3dNode *node;
    struct RefMesh *mesh;
    int made;
} g_nodeMeshes[NODE_SLOTS];

static struct RefMesh *node_mesh(struct C3dNode *node)
{
    unsigned h = (unsigned)(((uintptr_t)node >> 4) * 2654435761u) & (NODE_SLOTS - 1);

    while (g_nodeMeshes[h].node && g_nodeMeshes[h].node != node)
        h = (h + 1) & (NODE_SLOTS - 1);
    if (!g_nodeMeshes[h].node) {
        g_nodeMeshes[h].node = node;
        g_nodeMeshes[h].mesh = ref_node_mesh(node);
    }
    return g_nodeMeshes[h].mesh;
}

static void node_meshes_free(void)
{
    size_t i;

    for (i = 0; i < NODE_SLOTS; i++)
        ref_mesh_free(g_nodeMeshes[i].mesh);
    memset(g_nodeMeshes, 0, sizeof g_nodeMeshes);
}

/* ---------------------------------------------------------------------------------------------
   The map: the static world as the device has it, and the source of each of its meshes */

/* where a mesh lies in the shared buffers, the reference's mesh of the same triangles, and whose
   it is */
struct Source {
    struct RefMesh *mesh;
    struct SwPlacement place;
    struct GndGpuBlock *block;
    struct C3dNode *node;
    struct C3dActor *actor;
};

struct Map {
    const char *name;
    struct Source *sources;
    size_t sourceCount, sourceCapacity;
    struct RefMesh **blockMeshes;  /* by block, the reference's (NULL: no surfaces) */
    struct SwRecord *records;      /* the device's, in draw order */
    size_t recordCount;
    uint32_t *byIndex;             /* records by firstIndex */
    float (*transforms)[12];       /* by slot: its place in the world (the C's chain), sw_record_in_view's */
    float (*views)[12];            /* by slot: its view, the C's chain from the camera's */
    size_t transformCapacity, viewCapacity;
    const struct GfxMeshVertex *vertices;
    const WORD *indices;
    int twoPassAlpha;
    /* opaque records with one texture whose triangles meet in one plane, unlike in a corner */
    struct CoplanarPair {
        uint32_t a, b;
        int exact;
        /* the poses the reference drew a before b, and b before a */
        int aFirst, bFirst;
    } *pairs;
    size_t pairCount, pairCapacity;
    size_t outsideGraph, textureCount, arrayCount;
    /* by slot: the node whose matrix an animated slot takes, and the model a slot is of */
    struct C3dNode **slotNodes;
    struct C3dActor **slotActors;
    size_t slotNodeCapacity;
    /* by record (draw order): the model it is of, NULL for the ground */
    struct C3dActor **recordActors;
    struct C3dWorldRes *rsw;
    struct WorldLighting lighting;
    double streamMs;
    /* each array's layers after the map's first frame (the near models), which the device makes
       it with */
    uint32_t firstLayers[MAX_ARRAYS];
};

static int by_place(const void *a, const void *b)
{
    uint32_t x = ((const struct SwPlacement *)a)->firstIndex, y = ((const struct SwPlacement *)b)->firstIndex;

    return (x > y) - (x < y);
}

static void add_source(struct Map *map, struct RefMesh *mesh, struct GndGpuBlock *block, struct C3dNode *node,
                       struct C3dActor *actor)
{
    struct Source *s;

    map->sources = grow(map->sources, &map->sourceCapacity, map->sourceCount + 1, sizeof *map->sources);
    s = &map->sources[map->sourceCount++];
    memset(s, 0, sizeof *s);
    s->mesh = mesh;
    s->block = block;
    s->node = node;
    s->actor = actor;
}

static void node_sources(struct Map *map, struct C3dActor *actor, struct C3dNode *node, int depth)
{
    struct RefMesh *mesh;
    size_t child;

    if (depth >= GFX_WORLD_CHAIN)
        return;
    mesh = node_mesh(node);
    if (mesh && mesh->runCount)
        add_source(map, mesh, NULL, node, actor);
    for (child = 0; child < node->childCount; child++)
        node_sources(map, actor, node->children[child], depth + 1);
}

/* The meshes in the order world/world_gpu.c places them: the ground's blocks, then the models it
   took, each node before its children; each against the place the device was given for it, its
   vertices and indices to the byte. */
static void map_sources(struct Map *map)
{
    struct C3dGround15 *ground = g_world.ground;
    size_t i, k;
    int b;

    map->sourceCount = 0;
    for (b = 0; b < ground->gpuCols * ground->gpuRows; b++) {
        if (!map->blockMeshes[b])
            map->blockMeshes[b] = ref_block_mesh(ground, b % ground->gpuCols, b / ground->gpuCols);
        if (map->blockMeshes[b]->runCount)
            add_source(map, map->blockMeshes[b], &ground->gpuBlocks[b], NULL, NULL);
    }
    map->outsideGraph = 0;
    for (i = 0; i < g_world.models.inserted; i++) {
        struct C3dActor *a = g_world.models.data[i];

        if (a->gpuGeneration != D.generation) {
            map->outsideGraph++;
            continue;
        }
        if (a->ops->version == 1) {
            node_sources(map, a, ((struct C3dActorV1 *)a)->root, 1);
        } else {
            struct C3dActorV2 *v2 = (struct C3dActorV2 *)a;

            for (k = 0; k < v2->nodeCount; k++)
                node_sources(map, a, v2->nodes[k], 1);
        }
    }
    if (map->sourceCount != D.placeCount) {
        FAIL("%s: %zu meshes placed, the reference has %zu", map->name, D.placeCount, map->sourceCount);
        abort();
    }
    for (i = 0; i < map->sourceCount; i++) {
        struct Source *s = &map->sources[i];

        s->place = D.places[i];
        if (s->place.vertexCount != (uint32_t)s->mesh->nverts || s->place.indexCount != (uint32_t)s->mesh->nindices
            || memcmp(D.vertices + s->place.baseVertex, s->mesh->verts, s->place.vertexCount * sizeof *D.vertices)
            || memcmp(D.indices + s->place.firstIndex, s->mesh->indices, s->place.indexCount * sizeof *D.indices))
            FAIL("%s: mesh %zu is not the reference's", map->name, i);
        if (s->block && (s->block->firstIndex != s->place.firstIndex || s->block->indexCount != s->place.indexCount
                         || s->block->baseVertex != s->place.baseVertex))
            FAIL("%s: a block's place is not its mesh's", map->name);
    }
    /* placed one behind the other: the place of an index is found by firstIndex */
    for (i = 1; i < map->sourceCount; i++)
        if (map->sources[i].place.firstIndex < map->sources[i - 1].place.firstIndex)
            abort();
    (void)by_place;
}

/* each slot's node and model, in the order world/world_gpu.c makes them: slot 0 the world, then
   each model it took, its own slot, then its nodes, each before its children */
static void node_slots(struct Map *map, struct C3dActor *a, struct C3dNode *node, int depth, size_t *s)
{
    size_t child;

    if (depth >= GFX_WORLD_CHAIN)
        return;
    map->slotNodes = grow(map->slotNodes, &map->slotNodeCapacity, *s + 1, sizeof *map->slotNodes);
    map->slotActors = realloc(map->slotActors, map->slotNodeCapacity * sizeof *map->slotActors);
    if (!map->slotActors)
        abort();
    map->slotNodes[*s] = node;
    map->slotActors[(*s)++] = a;
    for (child = 0; child < node->childCount; child++)
        node_slots(map, a, node->children[child], depth + 1, s);
}

static void map_slots(struct Map *map)
{
    size_t i, k, s = 1;

    map->slotNodes = grow(map->slotNodes, &map->slotNodeCapacity, 1, sizeof *map->slotNodes);
    map->slotActors = realloc(map->slotActors, map->slotNodeCapacity * sizeof *map->slotActors);
    if (!map->slotActors)
        abort();
    map->slotNodes[0] = NULL;
    map->slotActors[0] = NULL;
    for (i = 0; i < g_world.models.inserted; i++) {
        struct C3dActor *a = g_world.models.data[i];

        if (a->gpuGeneration != D.generation)
            continue;
        map->slotNodes = grow(map->slotNodes, &map->slotNodeCapacity, s + 1, sizeof *map->slotNodes);
        map->slotActors = realloc(map->slotActors, map->slotNodeCapacity * sizeof *map->slotActors);
        if (!map->slotActors)
            abort();
        map->slotNodes[s] = NULL;
        map->slotActors[s++] = a;
        if (a->ops->version == 1) {
            node_slots(map, a, ((struct C3dActorV1 *)a)->root, 1, &s);
        } else {
            struct C3dActorV2 *v2 = (struct C3dActorV2 *)a;

            for (k = 0; k < v2->nodeCount; k++)
                node_slots(map, a, v2->nodes[k], 1, &s);
        }
    }
    if (s != D.slotCount) {
        FAIL("%s: %zu slots made, the models' walk has %zu", map->name, D.slotCount, s);
        abort();
    }
    for (i = 0; i < s; i++)
        if ((D.slots[i].animated >= 0) != (map->slotNodes[i] && node_has_keys(map->slotNodes[i], map->slotActors[i]->ops->version)))
            FAIL("%s: slot %zu animated %d, its node has keys %d", map->name, i, D.slots[i].animated >= 0,
                 map->slotNodes[i] && node_has_keys(map->slotNodes[i], map->slotActors[i]->ops->version));
}

/* the records in the device's order, and an index of them by where they lie */
static const struct SwRecord *g_sorting;

static int by_first_index(const void *a, const void *b)
{
    uint32_t x = g_sorting[*(const uint32_t *)a].firstIndex, y = g_sorting[*(const uint32_t *)b].firstIndex;

    return (x > y) - (x < y);
}

static const struct Source *source_of(const struct Map *map, uint32_t firstIndex);

static void map_records(struct Map *map)
{
    size_t i;

    map->records = D.records;
    map->recordCount = D.recordCount;
    map->vertices = D.vertices;
    map->indices = D.indices;
    free(map->byIndex);
    map->byIndex = malloc((map->recordCount + 1) * sizeof *map->byIndex);
    if (!map->byIndex)
        abort();
    for (i = 0; i < map->recordCount; i++)
        map->byIndex[i] = (uint32_t)i;
    g_sorting = map->records;
    qsort(map->byIndex, map->recordCount, sizeof *map->byIndex, by_first_index);
    free(map->recordActors);
    map->recordActors = malloc((map->recordCount + 1) * sizeof *map->recordActors);
    if (!map->recordActors)
        abort();
    for (i = 0; i < map->recordCount; i++)
        map->recordActors[i] = source_of(map, map->records[i].firstIndex)->actor;
    /* every vertex of a record carries its id */
    for (i = 0; i < map->recordCount; i++) {
        const struct SwRecord *r = &map->records[i];
        uint32_t k;

        for (k = 0; k < r->indexCount; k++)
            if (D.ids[map->indices[r->firstIndex + k] + r->baseVertex] != r->id) {
                FAIL("%s: a vertex of record %u carries %u", map->name, r->id,
                     D.ids[map->indices[r->firstIndex + k] + r->baseVertex]);
                break;
            }
    }
}

static long record_at(const struct Map *map, uint32_t firstIndex)
{
    size_t lo = 0, hi = map->recordCount;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;

        if (map->records[map->byIndex[mid]].firstIndex < firstIndex)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo < map->recordCount && map->records[map->byIndex[lo]].firstIndex == firstIndex)
        return (long)map->byIndex[lo];
    return -1;
}

/* the source a record's indices lie in (sources are placed one behind the other) */
static const struct Source *source_of(const struct Map *map, uint32_t firstIndex)
{
    size_t lo = 0, hi = map->sourceCount;

    while (hi - lo > 1) {
        size_t mid = lo + (hi - lo) / 2;

        if (map->sources[mid].place.firstIndex <= firstIndex)
            lo = mid;
        else
            hi = mid;
    }
    return &map->sources[lo];
}

/* the source of a reference mesh */
static const struct RefMesh **g_meshKeys;
static uint32_t *g_meshSources;
static size_t g_meshCount;

static int by_mesh(const void *a, const void *b)
{
    uintptr_t x = (uintptr_t)g_meshKeys[*(const uint32_t *)a], y = (uintptr_t)g_meshKeys[*(const uint32_t *)b];

    return (x > y) - (x < y);
}

static void index_meshes(const struct Map *map)
{
    size_t i;
    const struct RefMesh **keys = malloc((map->sourceCount + 1) * sizeof *keys);
    uint32_t *order = malloc((map->sourceCount + 1) * sizeof *order);

    if (!keys || !order)
        abort();
    for (i = 0; i < map->sourceCount; i++) {
        keys[i] = map->sources[i].mesh;
        order[i] = (uint32_t)i;
    }
    free((void *)g_meshKeys);
    g_meshKeys = keys;
    qsort(order, map->sourceCount, sizeof *order, by_mesh);
    free(g_meshSources);
    g_meshSources = order;
    g_meshCount = map->sourceCount;
}

static const struct Source *mesh_source(const struct Map *map, const struct RefMesh *mesh)
{
    size_t lo = 0, hi = g_meshCount;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;

        if ((uintptr_t)map->sources[g_meshSources[mid]].mesh < (uintptr_t)mesh)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo < g_meshCount && map->sources[g_meshSources[lo]].mesh == mesh ? &map->sources[g_meshSources[lo]] : NULL;
}

/* ---------------------------------------------------------------------------------------------
   The frame's slots as the C computes them: each slot's place in the world and its view, its
   matrix times its parent's (render/gfx.h, GfxWorldSlot), and the check that a slot's matrix is
   its node's as it stands, static nodes' unchanged since the world was built */

static void frame_transforms(struct Map *map, const float *view)
{
    size_t s, i;

    map->transforms = grow(map->transforms, &map->transformCapacity, D.slotCount + 1, sizeof *map->transforms);
    map->views = grow(map->views, &map->viewCapacity, D.slotCount + 1, sizeof *map->views);
    for (s = 0; s < D.slotCount; s++) {
        const struct GfxWorldSlot *slot = &D.slots[s];
        /* before the first frame, an animated slot as it was built */
        const float *local = slot->animated >= 0 && (size_t)slot->animated < D.animatedCount ? D.animated[slot->animated]
                                                                                            : slot->local;

        if (s == 0) {
            memcpy(map->transforms[0], slot->local, sizeof map->transforms[0]);
            memcpy(map->views[0], view, sizeof map->views[0]);
            continue;
        }
        if ((size_t)slot->parent >= s && slot->parent >= 0)
            abort();
        if (slot->parent < 0) {
            memcpy(map->transforms[s], local, sizeof map->transforms[s]);
            mat_multiply(map->views[s], local, view);
        } else {
            mat_multiply(map->transforms[s], local, map->transforms[slot->parent]);
            mat_multiply(map->views[s], local, map->views[slot->parent]);
        }
    }
    /* the nodes' matrices: a static node's as it was (an animated one's is the pose the frame took,
   which compare_draw checks against the reference's) */
    for (i = 0; i < map->sourceCount; i++) {
        const struct Source *src = &map->sources[i];
        long rec = record_at(map, src->place.firstIndex);
        const struct GfxWorldSlot *slot;

        if (!src->node || rec < 0)
            continue;
        slot = &D.slots[map->records[rec].transform];
        if (slot->animated >= 0)
            continue;
        if (memcmp(slot->local, src->node->matrix, sizeof src->node->matrix))
            FAIL("%s: a static node's slot does not hold its matrix", map->name);
    }
}

/* ---------------------------------------------------------------------------------------------
   The reference's frame: what view_on_render queued of the ground and the models and the
   renderer drew, at 75489601 (world/view_camera.c, world/model_actor.c, world/model_node_render.c,
   world/ground_draw.c, render/crenderer.c), on the scene graph's leaves as the client's own walk
   reaches them; the casters it chose while the frame drew shadows (render/renderer.h,
   renderer_shadow_frame, which the client's view_on_render set up); and its water's quads */

/* the cull modes the old device took (render/gfx.h, GFX_CULL_*) */
#define CULL_NONE 1
#define CULL_CW 2
#define CULL_CCW 3

/* a draw of a reference mesh, as the old renderer gave it to the device */
struct RefDraw {
    int kind, first, count, cull;
    float uv[6];
};

struct DeviceDraw {
    int phase, cascade;
    const struct RefMesh *mesh;
    struct RefDraw draw;
    float view[12];
    struct Texture *texture[2];
    DWORD cull, address;
};

static struct DeviceDraw *g_draws;
static size_t g_drawCount, g_drawCapacity;

/* the reference's lists: the renderer's opaque, ground, alpha and caster lists */
enum { RQ_OPAQUE_CLAMP, RQ_OPAQUE_WRAP, RQ_GROUND, RQ_ALPHA_CLAMP, RQ_ALPHA_WRAP, RQ_ALPHA_DEPTH, RQ_CAST, RQ_COUNT };

struct RefEntry {
    struct DeviceDraw d;
    float key;
    unsigned order;
    unsigned sort[2];
};

static struct RefEntry *g_ref[RQ_COUNT];
static size_t g_refCount[RQ_COUNT], g_refCapacity[RQ_COUNT];

/* the cells of water the reference drew a quad over, keyed */
struct RefWater {
    int x, y;
    float key;
};

static struct RefWater *g_water;
static size_t g_waterCount, g_waterCapacity;

/* what the reference's walk marked a frame: models reached (ticked) and cast, blocks drawn and cast,
   cells; and of a pose's models, those the client ticked (2), those that animate (4) and of those
   the ones with a record in view before the frame (3) */
#define MARK_SLOTS (1u << 20)
static struct {
    const void *what;
    int frame[5];
} g_marks[MARK_SLOTS];
static int g_refFrame;

static int *mark(const void *what, int which)
{
    unsigned h = (unsigned)(((uintptr_t)what >> 3) * 2654435761u) & (MARK_SLOTS - 1);

    while (g_marks[h].what && g_marks[h].what != what)
        h = (h + 1) & (MARK_SLOTS - 1);
    if (!g_marks[h].what) {
        g_marks[h].what = what;
        g_marks[h].frame[0] = g_marks[h].frame[1] = g_marks[h].frame[2] = g_marks[h].frame[3] = -1;
        g_marks[h].frame[4] = -1;
    }
    return &g_marks[h].frame[which];
}

static void queue(int list, const struct DeviceDraw *d, float key, const struct Texture *t0, const struct Texture *t1)
{
    struct RefEntry *e;

    g_ref[list] = grow(g_ref[list], &g_refCapacity[list], g_refCount[list] + 1, sizeof *g_ref[list]);
    e = &g_ref[list][g_refCount[list]];
    e->d = *d;
    e->key = key;
    e->order = (unsigned)g_refCount[list]++;
    e->sort[0] = t0 ? t0->serial : 0;
    e->sort[1] = t1 ? t1->serial : 0;
}

static BYTE ref_mirrored(const struct C3dActor *a)
{
    BYTE m = 0.0f > a->scale[0];

    if (0.0f > a->scale[1])
        m = m == 0;
    if (0.0f > a->scale[2])
        m = m == 0;
    return m;
}

/* model_node_render.c's gpu_cast: every run of a node as a caster with its texture */
static void ref_node_cast(struct C3dNode *node, int version, const float *m)
{
    struct RefMesh *g = node_mesh(node);
    struct DeviceDraw d;
    int i;

    if (!g)
        return;
    memset(&d, 0, sizeof d);
    d.phase = -1;
    d.cascade = 0;
    d.mesh = g;
    memcpy(d.view, m, sizeof d.view);
    d.draw.kind = GFX_MESH_MODEL;
    for (i = 0; i < g->runCount; i++) {
        const struct RefRun *r = &g->runs[i];

        uv_turn_matrix(version == 2 ? uv_xform_find((const struct C3dNodeV2 *)node, r->texture) : NULL, d.draw.uv);
        d.draw.first = r->first;
        d.draw.count = r->count;
        d.texture[0] = run_texture(node, r->texture);
        queue(RQ_CAST, &d, 0.0f, d.texture[0], NULL);
    }
}

/* model_node_render.c's gpu_render: each run where renderer_add_prim sent a model's quads, an alpha
   one keyed by the rhw of its first face's first corner */
static void ref_node_render(struct C3dNode *node, int version, const float *m, BYTE mirrored, BYTE layer)
{
    struct RefMesh *g = node_mesh(node);
    /* the model's alpha, which a picked model faded by: whole, as no model is ever pickable */
    BYTE alpha = 0xff;
    struct DeviceDraw d;
    int i, cull;

    if (!g)
        return;
    memset(&d, 0, sizeof d);
    d.cascade = -1;
    d.mesh = g;
    memcpy(d.view, m, sizeof d.view);
    d.draw.kind = GFX_MESH_MODEL;
    cull = (version == 1) == (mirrored == 0) ? CULL_CW : CULL_CCW;
    for (i = 0; i < g->runCount; i++) {
        const struct RefRun *r = &g->runs[i];
        struct Texture *t = run_texture(node, r->texture);
        const struct UvTransform *x = version == 2 ? uv_xform_find((const struct C3dNodeV2 *)node, r->texture) : NULL;
        float key = 0.0f;
        int flags, list;

        if (version == 1)
            flags = alpha >= 0xff && t->format != 1 ? 0 : layer << 15 | 1;
        else
            flags = (x ? 0x20 : 0) | layer << 15 | (alpha < 0xff || t->format == 1);
        if (flags & 1) {
            struct TexDrawVertex corner;

            renderer_project_vertex(g_renderer, &node->mesh->positions[node->mesh->faces[r->face].v[0]], m, &corner);
            key = corner.rhw;
        }
        d.draw.first = r->first;
        d.draw.count = r->count;
        d.draw.cull = r->twoSided ? CULL_NONE : cull;
        uv_turn_matrix(x, d.draw.uv);
        d.texture[0] = t;
        /* render/rp_pool.c's renderer_mesh_pass */
        if (!(flags & 1))
            list = flags & 0x20 ? RQ_OPAQUE_WRAP : RQ_OPAQUE_CLAMP;
        else if (flags & 0x20)
            list = RQ_ALPHA_WRAP;
        else
            list = flags & 0x8000 ? RQ_ALPHA_DEPTH : RQ_ALPHA_CLAMP;
        d.phase = list <= RQ_OPAQUE_WRAP ? PERF_L_GROUND : list == RQ_ALPHA_DEPTH ? PERF_L_ALPHA_WRITE_DEPTH : PERF_L_ALPHATEST;
        d.address = list == RQ_OPAQUE_WRAP || list == RQ_ALPHA_WRAP ? DRAW_ADDRESS_WRAP : DRAW_ADDRESS_CLAMP;
        queue(list, &d, key, t, NULL);
    }
    if (g_renderer->shadowPending)
        ref_node_cast(node, version, m);
}

static void ref_node(struct C3dNode *node, int version, const float *view, BYTE mirrored, BYTE layer, int cast)
{
    float m[12];
    size_t child;

    mat_multiply(m, node->matrix, view);
    if (cast)
        ref_node_cast(node, version, m);
    else
        ref_node_render(node, version, m, mirrored, layer);
    for (child = 0; child < node->childCount; ++child)
        ref_node(node->children[child], version, m, mirrored, layer, cast);
}

/* c3d_actor_v1_render and v2 (cast: only the casters) */
static void ref_actor(struct C3dActor *a, const float *view, int cast)
{
    float m[12];

    mat_multiply(m, a->matrix, view);
    if (a->ops->version == 1) {
        ref_node(((struct C3dActorV1 *)a)->root, 1, m, ref_mirrored(a), a->layer, cast);
    } else {
        struct C3dActorV2 *v2 = (struct C3dActorV2 *)a;
        size_t i;

        for (i = 0; i < v2->nodeCount; i++)
            ref_node(v2->nodes[i], 2, m, ref_mirrored(a), a->layer, cast);
    }
    *mark(a, 1) = g_refFrame;
}

static void ref_blocks(const struct C3dGround15 *ground, const int *cells, int *out)
{
    out[0] = cells[0] < 0 ? 0 : cells[0] / GND_GPU_BLOCK;
    out[1] = cells[1] < 0 ? 0 : cells[1] / GND_GPU_BLOCK;
    out[2] = cells[2] / GND_GPU_BLOCK;
    out[3] = cells[3] / GND_GPU_BLOCK;
    if (out[2] >= ground->gpuCols)
        out[2] = ground->gpuCols - 1;
    if (out[3] >= ground->gpuRows)
        out[3] = ground->gpuRows - 1;
}

/* ground_draw.c's gpu_cast: a block as one caster without a texture */
static void ref_ground_cast(struct Map *map, const struct C3dGround15 *ground, const float *view, const int *cells)
{
    int b4[4], bx, by;
    struct DeviceDraw d;

    memset(&d, 0, sizeof d);
    d.phase = -1;
    d.cascade = 0;
    memcpy(d.view, view, sizeof d.view);
    d.draw.kind = GFX_MESH_GROUND;
    d.draw.uv[0] = d.draw.uv[3] = 1.0f;
    ref_blocks(ground, cells, b4);
    for (by = b4[1]; by <= b4[3]; by++)
        for (bx = b4[0]; bx <= b4[2]; bx++) {
            struct RefMesh *g = map->blockMeshes[by * ground->gpuCols + bx];
            int *cast = mark(g, 1);

            if (*cast == g_refFrame || !g->nindices)
                continue;
            *cast = g_refFrame;
            d.mesh = g;
            d.draw.count = g->nindices;
            queue(RQ_CAST, &d, 0.0f, NULL, NULL);
        }
}

/* ground_draw.c's water_quad: whether the cell's quad is drawn, and its key */
static int ref_water(struct C3dGround15 *self, int x, int y, struct GndCell *cell, const float *view, float *key)
{
    const struct GndSurface *top = cell->top;
    const struct GndWater *w;
    struct TexDrawVertex v[4];
    float h, hPrev, hNext, corner[4][3];
    int64_t diagonal;
    int block, i;

    if (!self->waters)
        return 0;
    block = self->waterPerBlock ? c3d_ground15_get_block_index(self, x, y) : 0;
    if (block < 0 || (size_t)block >= self->waterCount)
        return 0;
    w = self->waters + block;
    diagonal = (int64_t)x + y;
    h = ground_water_height(w, diagonal);
    hPrev = ground_water_height(w, diagonal - 1);
    hNext = ground_water_height(w, diagonal + 1);
    if (!(top->v[0].pos[1] > h || top->v[1].pos[1] > h || top->v[2].pos[1] > h || top->v[3].pos[1] > h))
        return 0;
    c3d_ground15_corner(self, x, y, hPrev, corner[0]);
    c3d_ground15_corner(self, x + 1, y, h, corner[1]);
    c3d_ground15_corner(self, x, y + 1, h, corner[2]);
    c3d_ground15_corner(self, x + 1, y + 1, hNext, corner[3]);
    for (i = 0; i < 4; i++) {
        Vec3 p = {corner[i][0], corner[i][1], corner[i][2]};

        renderer_project_vertex(g_renderer, &p, view, &v[i]);
    }
    if (!(v[0].rhw > 0.0f) || !(v[1].rhw > 0.0f) || !(v[2].rhw > 0.0f) || !(v[3].rhw > 0.0f))
        return 0;
    if (!((v[2].x - v[0].x) * (v[1].y - v[0].y) > (v[1].x - v[0].x) * (v[2].y - v[0].y)))
        return 0;
    *key = v[0].rhw;
    return 1;
}

/* ground_draw.c's c3d_ground15_render: the blocks under the cells, then their water */
static void ref_ground(struct Map *map, struct C3dGround15 *ground, const float *view, const int *cells)
{
    int b4[4], bx, by, x, y, i;
    struct DeviceDraw d;

    memset(&d, 0, sizeof d);
    d.phase = PERF_L_WATER;
    d.cascade = -1;
    d.address = DRAW_ADDRESS_CLAMP;
    memcpy(d.view, view, sizeof d.view);
    d.draw.kind = GFX_MESH_GROUND;
    d.draw.cull = CULL_CW;
    d.draw.uv[0] = d.draw.uv[3] = 1.0f;
    ref_blocks(ground, cells, b4);
    for (by = b4[1]; by <= b4[3]; by++)
        for (bx = b4[0]; bx <= b4[2]; bx++) {
            struct RefMesh *g = map->blockMeshes[by * ground->gpuCols + bx];
            int *drawn = mark(g, 0);

            if (*drawn == g_refFrame)
                continue;
            *drawn = g_refFrame;
            d.mesh = g;
            for (i = 0; i < g->runCount; i++) {
                d.draw.first = g->blockRuns[i].first;
                d.draw.count = g->blockRuns[i].count;
                d.texture[0] = g->blockRuns[i].texture;
                d.texture[1] = g_lightmap ? g->blockRuns[i].lightmap : NULL;
                queue(RQ_GROUND, &d, 0.0f, d.texture[0], d.texture[1]);
            }
        }
    if (g_renderer->shadowPending)
        ref_ground_cast(map, ground, view, cells);
    if (!ground->waters)
        return;
    for (y = cells[1]; y <= cells[3]; y++)
        for (x = cells[0]; x <= cells[2]; x++) {
            struct GndCell *cell = GND_CELL(ground, x, y);
            int *seen = mark(cell, 0);
            float key;

            if (*seen == g_refFrame)
                continue;
            *seen = g_refFrame;
            if (cell->top && ref_water(ground, x, y, cell, view, &key)) {
                g_water = grow(g_water, &g_waterCapacity, g_waterCount + 1, sizeof *g_water);
                g_water[g_waterCount].x = x;
                g_water[g_waterCount].y = y;
                g_water[g_waterCount].key = key;
                g_waterCount++;
            }
        }
}

/* view_camera.c's view_cast_shadows */
static void ref_cast_shadows(struct Map *map, struct SceneGraphNode *node, int level, const float *view)
{
    struct Renderer *r = g_renderer;
    size_t index;
    int i;

    if (!renderer_shadow_reaches(r, node->centre, node->half, NULL))
        return;
    if (level < RSW_TREE_DEPTH) {
        for (i = 0; i < 4; i++)
            if (node->child[i])
                ref_cast_shadows(map, node->child[i], level + 1, view);
        return;
    }
    for (index = 0; index < node->objects.count; ++index) {
        struct C3dActor *a = node->objects.data[index];

        if (*mark(a, 1) == g_refFrame || !renderer_shadow_reaches(r, a->obb.centre, a->obb.extent, a->obb.axis))
            continue;
        if (*mark(a, 0) != g_refFrame) {
            a->ops->tick(a);
            *mark(a, 0) = g_refFrame;
        }
        ref_actor(a, view, 1);
    }
    if (node->ground)
        ref_ground_cast(map, node->ground, view, node->groundCell);
}

/* render/crenderer.c's sorts, in std_sort as it sorted */
static unsigned char ref_keyed_less(const void *a, const void *b, void *pred)
{
    const struct RefEntry *x = a, *y = b;
    (void)pred;
    if (y->key > x->key)
        return 1;
    if (x->key > y->key)
        return 0;
    return x->order < y->order;
}

static unsigned char ref_quad_less(const void *a, const void *b, void *pred)
{
    const struct RefEntry *x = a, *y = b;
    (void)pred;
    return x->sort[0] < y->sort[0];
}

static unsigned char ref_two_tex_less(const void *a, const void *b, void *pred)
{
    const struct RefEntry *x = a, *y = b;
    (void)pred;
    if (x->sort[0] != y->sort[0])
        return x->sort[0] < y->sort[0];
    return x->sort[1] < y->sort[1];
}

static void emit(int list, StdSortLessFn less)
{
    size_t i;

    if (less && g_refCount[list] > 1)
        std_sort(g_ref[list], g_ref[list] + g_refCount[list], sizeof *g_ref[list], less, NULL);
    for (i = 0; i < g_refCount[list]; i++) {
        g_draws = grow(g_draws, &g_drawCapacity, g_drawCount + 1, sizeof *g_draws);
        g_draws[g_drawCount++] = g_ref[list][i].d;
    }
}

/* The reference's frame on the view: g_draws in the order the renderer drew them, the casters
   once (the renderer drew them into every cascade), and g_water. */
static void reference_frame(struct Map *map, struct View *view)
{
    static struct ViewFrustum leaves;
    struct Renderer *r = g_renderer;
    struct FrustumScreen fs = {{r->proj[0], r->proj[1]}, {(float)r->centreX, (float)r->centreY},
                               {(float)r->width, (float)r->height}};
    const float *m = view->viewMatrix;
    size_t i, k;
    int list;

    g_refFrame++;
    for (list = 0; list < RQ_COUNT; list++)
        g_refCount[list] = 0;
    g_drawCount = 0;
    g_waterCount = 0;
    view_frustum_build(&leaves, &fs, m, &g_world.root);
    for (i = 0; i < leaves.crossing_count; i++) {
        struct SceneGraphNode *leaf = leaves.crossing[i];

        for (k = 0; k < leaf->objects.count; k++) {
            struct C3dActor *a = leaf->objects.data[k];
            int *seen = mark(a, 0);

            int res;

            if (*seen == g_refFrame)
                continue;
            res = view_frustum_test_obb(&leaves, &a->obb);
            a->ops->tick(a);
            if (res != FRUSTUM_OUTSIDE)
                ref_actor(a, m, 0);
            *seen = g_refFrame;
        }
        ref_ground(map, leaf->ground, m, leaf->groundCell);
    }
    for (i = 0; i < leaves.inside_count; i++) {
        struct SceneGraphNode *leaf = leaves.inside[i];

        for (k = 0; k < leaf->objects.count; k++) {
            struct C3dActor *a = leaf->objects.data[k];
            int *seen = mark(a, 0);

            a->ops->tick(a);
            if (*seen != g_refFrame) {
                ref_actor(a, m, 0);
                *seen = g_refFrame;
            }
        }
        ref_ground(map, leaf->ground, m, leaf->groundCell);
    }
    if (r->shadowPending)
        ref_cast_shadows(map, &g_world.root, 0, m);
    view_frustum_clear(&leaves);
    emit(RQ_CAST, NULL);
    emit(RQ_OPAQUE_CLAMP, ref_quad_less);
    emit(RQ_OPAQUE_WRAP, ref_quad_less);
    emit(RQ_GROUND, ref_two_tex_less);
    emit(RQ_ALPHA_CLAMP, ref_keyed_less);
    emit(RQ_ALPHA_WRAP, ref_keyed_less);
    emit(RQ_ALPHA_DEPTH, ref_keyed_less);
}

/* ---------------------------------------------------------------------------------------------
   Clipping triangles: what of them is in the view, or in a cascade's box */

struct Plane {
    double n[3], d;
};

static int clip(double (*poly)[3], int n, const struct Plane *p)
{
    double out[12][3];
    int i, k, m = 0;

    for (i = 0; i < n; i++) {
        const double *a = poly[i], *b = poly[(i + 1) % n];
        double fa = p->n[0] * a[0] + p->n[1] * a[1] + p->n[2] * a[2] + p->d;
        double fb = p->n[0] * b[0] + p->n[1] * b[1] + p->n[2] * b[2] + p->d;

        if (fa >= 0)
            for (k = 0; k < 3; k++)
                out[m][k] = a[k];
        if (fa >= 0)
            m++;
        if ((fa >= 0) != (fb >= 0)) {
            double t = fa / (fa - fb);

            for (k = 0; k < 3; k++)
                out[m][k] = a[k] + (b[k] - a[k]) * t;
            m++;
        }
    }
    memcpy(poly, out, (size_t)m * sizeof out[0]);
    return m;
}

/* the area on screen, in pixels, of a triangle given in view space, clipped to the screen and
   the depth range */
static double view_area(const double tri[3][3])
{
    const struct Renderer *r = g_renderer;
    double poly[12][3], area = 0;
    struct Plane planes[6] = {
        {{r->proj[0], 0, r->centreX}, 0},
        {{-r->proj[0], 0, r->width - r->centreX}, 0},
        {{0, r->proj[1], r->centreY}, 0},
        {{0, -r->proj[1], r->height - r->centreY}, 0},
        {{0, 0, 1}, -RENDER_NEAR_Z},
        {{0, 0, -1}, RENDER_FAR_Z},
    };
    int n = 3, i;

    memcpy(poly, tri, 3 * sizeof poly[0]);
    for (i = 0; i < 6 && n >= 3; i++)
        n = clip(poly, n, &planes[i]);
    for (i = 0; i < n; i++) {
        const double *a = poly[i], *b = poly[(i + 1) % n];
        double ax = a[0] * r->proj[0] / a[2], ay = a[1] * r->proj[1] / a[2];
        double bx = b[0] * r->proj[0] / b[2], by = b[1] * r->proj[1] / b[2];

        area += ax * by - bx * ay;
    }
    return n >= 3 ? fabs(area) * 0.5 : 0.0;
}

/* the area across the light, in world units, of a triangle given in the world, clipped to a
   cascade's box */
static double cascade_area(const double tri[3][3], const struct SwCascade *c)
{
    double poly[12][3], area = 0;
    struct Plane planes[6];
    int n = 3, i, k;

    for (i = 0; i < 3; i++)
        for (k = 0; k < 3; k++)
            poly[i][k] = tri[i][0] * c->axis[k][0] + tri[i][1] * c->axis[k][1] + tri[i][2] * c->axis[k][2];
    memset(planes, 0, sizeof planes);
    for (k = 0; k < 2; k++) {
        planes[k * 2].n[k] = 1;
        planes[k * 2].d = -c->lo[k];
        planes[k * 2 + 1].n[k] = -1;
        planes[k * 2 + 1].d = c->hi[k];
    }
    planes[4].n[2] = 1;
    planes[4].d = -c->depth[0];
    planes[5].n[2] = -1;
    planes[5].d = c->depth[1];
    for (i = 0; i < 6 && n >= 3; i++)
        n = clip(poly, n, &planes[i]);
    for (i = 0; i < n; i++) {
        const double *a = poly[i], *b = poly[(i + 1) % n];

        area += a[0] * b[1] - b[0] * a[1];
    }
    return n >= 3 ? fabs(area) * 0.5 : 0.0;
}

static void xform_d(const float *m, const double *p, double *out)
{
    out[0] = p[0] * m[0] + p[1] * m[3] + p[2] * m[6] + m[9];
    out[1] = p[0] * m[1] + p[1] * m[4] + p[2] * m[7] + m[10];
    out[2] = p[0] * m[2] + p[1] * m[5] + p[2] * m[8] + m[11];
}

/* a record's triangle t in the world, from the shared buffers */
static void record_triangle(const struct Map *map, const struct SwRecord *r, uint32_t t, double out[3][3])
{
    int k;

    for (k = 0; k < 3; k++) {
        const struct GfxMeshVertex *v = &map->vertices[map->indices[r->firstIndex + t * 3 + k] + r->baseVertex];
        double p[3] = {v->x, v->y, v->z};

        if (r->transform)
            xform_d(map->transforms[r->transform], p, out[k]);
        else
            memcpy(out[k], p, sizeof p);
    }
}

/* the most of a record's area in the view (screen pixels) or in a cascade's box (world units) */
static double record_view_area(const struct Map *map, const struct SwRecord *r, const float *view)
{
    double most = 0, tri[3][3], v[3][3];
    uint32_t t;
    int k;

    for (t = 0; t < r->indexCount / 3; t++) {
        double a;

        record_triangle(map, r, t, tri);
        for (k = 0; k < 3; k++)
            xform_d(view, tri[k], v[k]);
        a = view_area((const double (*)[3])v);
        most = a > most ? a : most;
    }
    return most;
}

static double record_cascade_area(const struct Map *map, const struct SwRecord *r, const struct SwCascade *c)
{
    double most = 0, tri[3][3];
    uint32_t t;

    for (t = 0; t < r->indexCount / 3; t++) {
        double a;

        record_triangle(map, r, t, tri);
        a = cascade_area((const double (*)[3])tri, c);
        most = a > most ? a : most;
    }
    return most;
}

/* the most of a device draw's area in the view, with today's own view of it */
static double draw_view_area(const struct DeviceDraw *d)
{
    double most = 0, v[3][3];
    int t, k;

    for (t = 0; t < d->draw.count / 3; t++) {
        double a;

        for (k = 0; k < 3; k++) {
            const struct GfxMeshVertex *p = &d->mesh->verts[d->mesh->indices[d->draw.first + t * 3 + k]];
            double q[3] = {p->x, p->y, p->z};

            xform_d(d->view, q, v[k]);
        }
        a = view_area((const double (*)[3])v);
        most = a > most ? a : most;
    }
    return most;
}

/* ---------------------------------------------------------------------------------------------
   Where the order of opaque draws can change pixels: the renderer sorts a pass by texture with
   an unstable sort (std/std_sort.c), so two draws with one texture come in whatever order the
   scene graph's walk and the sort leave them, and the records keep theirs. With the depth test
   LESSEQUAL the later of two draws wins only where both reach a pixel at the same depth: where
   their triangles lie in one plane and overlap. The ground's surfaces never overlap (a cell's
   top, its front and its right wall), so only the models' are looked at. */

struct OrderStats {
    size_t pairs, coplanar, coplanarPairs, exactPairs, identical;
    size_t crossPairs, crossTriangles, crossCoplanar, crossExact, crossTurned;
};

/* within COPLANAR units of one plane: the depth buffer can tie them far off; within EXACT, near too */
#define COPLANAR 0.05
#define EXACT 1e-4

static void cross_d(const double *a, const double *b, double *out)
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

/* whether two triangles lie in one plane and overlap with area: 2 within EXACT units, 1 within
   COPLANAR */
static int coplanar_overlap(const double (*a)[3], const double (*b)[3])
{
    double e1[3], e2[3], n[3], m[3], len, lenB, pa[3][2], pb[3][2], most = 0;
    int i, k, drop, u, v;

    for (k = 0; k < 3; k++) {
        e1[k] = a[1][k] - a[0][k];
        e2[k] = a[2][k] - a[0][k];
    }
    cross_d(e1, e2, n);
    len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    for (k = 0; k < 3; k++) {
        e1[k] = b[1][k] - b[0][k];
        e2[k] = b[2][k] - b[0][k];
    }
    cross_d(e1, e2, m);
    lenB = sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
    if (!(len > 0) || !(lenB > 0))
        return 0;
    for (i = 0; i < 3; i++) {
        double d = (n[0] * (b[i][0] - a[0][0]) + n[1] * (b[i][1] - a[0][1]) + n[2] * (b[i][2] - a[0][2])) / len;

        if (fabs(d) > COPLANAR)
            return 0;
        most = fabs(d) > most ? fabs(d) : most;
    }
    if (fabs(n[0] * m[0] + n[1] * m[1] + n[2] * m[2]) / (len * lenB) < 0.999)
        return 0;
    drop = fabs(n[0]) >= fabs(n[1]) && fabs(n[0]) >= fabs(n[2]) ? 0 : fabs(n[1]) >= fabs(n[2]) ? 1 : 2;
    u = drop == 0 ? 1 : 0;
    v = drop == 2 ? 1 : 2;
    for (i = 0; i < 3; i++) {
        pa[i][0] = a[i][u];
        pa[i][1] = a[i][v];
        pb[i][0] = b[i][u];
        pb[i][1] = b[i][v];
    }
    /* separating axes: the edges' normals of both */
    for (k = 0; k < 6; k++) {
        const double(*t)[2] = k < 3 ? pa : pb;
        double ax = -(t[(k + 1) % 3][1] - t[k % 3][1]), ay = t[(k + 1) % 3][0] - t[k % 3][0];
        double lo[2] = {1e300, 1e300}, hi[2] = {-1e300, -1e300}, slack;

        for (i = 0; i < 3; i++) {
            double x = pa[i][0] * ax + pa[i][1] * ay, y = pb[i][0] * ax + pb[i][1] * ay;

            lo[0] = x < lo[0] ? x : lo[0];
            hi[0] = x > hi[0] ? x : hi[0];
            lo[1] = y < lo[1] ? y : lo[1];
            hi[1] = y > hi[1] ? y : hi[1];
        }
        slack = 1e-9 * (fabs(ax) + fabs(ay)) * (1 + fabs(lo[0]) + fabs(hi[0]));
        if (hi[0] <= lo[1] + slack || hi[1] <= lo[0] + slack)
            return 0;
    }
    return most <= EXACT ? 2 : 1;
}

static double *record_triangles(const struct Map *map, const struct SwRecord *r)
{
    double *t = malloc((size_t)r->indexCount * 3 * sizeof *t);
    uint32_t i;

    if (!t)
        abort();
    for (i = 0; i < r->indexCount / 3; i++)
        record_triangle(map, r, i, (double (*)[3])(t + i * 9));
    return t;
}

static void add_pair(struct Map *map, uint32_t a, uint32_t b, int exact)
{
    map->pairs = grow(map->pairs, &map->pairCapacity, map->pairCount + 1, sizeof *map->pairs);
    map->pairs[map->pairCount].a = a;
    map->pairs[map->pairCount].b = b;
    map->pairs[map->pairCount].exact = exact;
    map->pairs[map->pairCount].aFirst = map->pairs[map->pairCount].bFirst = 0;
    map->pairCount++;
}

/* the records' boxes in the world, an animated record's moved by its transform as it stands */
struct Box {
    float centre[3], half[3];
};

static struct Box *world_boxes(const struct Map *map)
{
    struct Box *boxes = malloc((map->recordCount + 1) * sizeof *boxes);
    size_t i;
    int k;

    if (!boxes)
        abort();
    for (i = 0; i < map->recordCount; i++) {
        const struct SwRecord *r = &map->records[i];
        const float *m = map->transforms[r->transform];

        if (!(r->flags & SW_ANIMATED)) {
            memcpy(boxes[i].centre, r->centre, sizeof r->centre);
            memcpy(boxes[i].half, r->half, sizeof r->half);
            continue;
        }
        for (k = 0; k < 3; k++) {
            boxes[i].centre[k] = r->centre[0] * m[k] + r->centre[1] * m[3 + k] + r->centre[2] * m[6 + k] + m[9 + k];
            boxes[i].half[k] = fabsf(m[k]) * r->half[0] + fabsf(m[3 + k]) * r->half[1] + fabsf(m[6 + k]) * r->half[2];
        }
    }
    return boxes;
}

static int boxes_meet(const struct Box *a, const struct Box *b)
{
    int k;

    for (k = 0; k < 3; k++)
        if (fabs((double)a->centre[k] - b->centre[k]) > (double)a->half[k] + b->half[k] + COPLANAR)
            return 0;
    return 1;
}

/* whether the corners of two triangles carry the same vertex but for where they lie */
static int same_corners(const struct Map *map, const struct SwRecord *a, uint32_t ta, const struct SwRecord *b,
                        uint32_t tb)
{
    int k;

    for (k = 0; k < 3; k++) {
        const struct GfxMeshVertex *x = &map->vertices[map->indices[a->firstIndex + ta * 3 + k] + a->baseVertex];
        const struct GfxMeshVertex *y = &map->vertices[map->indices[b->firstIndex + tb * 3 + k] + b->baseVertex];

        if (x->diffuse != y->diffuse || x->colour2 != y->colour2 || x->u0 != y->u0 || x->v0 != y->v0)
            return 0;
    }
    return 1;
}

/* the triangles of record a within the box of record b (with COPLANAR to spare): indices into a's */
static uint32_t near_triangles(const struct SwRecord *a, const double *ta, const struct Box *b, uint32_t *out)
{
    uint32_t t, n = 0;
    int k, c;

    for (t = 0; t < a->indexCount / 3; t++) {
        const double *p = ta + t * 9;
        int inside = 1;

        for (k = 0; k < 3 && inside; k++) {
            double lo = p[k], hi = p[k];

            for (c = 1; c < 3; c++) {
                lo = p[c * 3 + k] < lo ? p[c * 3 + k] : lo;
                hi = p[c * 3 + k] > hi ? p[c * 3 + k] : hi;
            }
            inside = hi >= (double)b->centre[k] - b->half[k] - COPLANAR
                     && lo <= (double)b->centre[k] + b->half[k] + COPLANAR;
        }
        if (inside)
            out[n++] = t;
    }
    return n;
}

/* 0, or 1 where two records have triangles in one plane that overlap, 2 where within EXACT; alike
   where every such pair of triangles is the same in every corner */
static int pair_coplanar(const struct Map *map, const struct Box *boxes, double **tris, uint32_t i, uint32_t j,
                         int *alike, size_t *triangles)
{
    const struct SwRecord *a = &map->records[i], *b = &map->records[j];
    static uint32_t *na, *nb;
    static size_t capacity;
    uint32_t ca, cb, x, y;
    int found = 0;

    if (!tris[i])
        tris[i] = record_triangles(map, a);
    if (!tris[j])
        tris[j] = record_triangles(map, b);
    if (capacity < a->indexCount / 3 + 1 || capacity < b->indexCount / 3 + 1) {
        capacity = (a->indexCount > b->indexCount ? a->indexCount : b->indexCount) / 3 + 1;
        na = realloc(na, capacity * sizeof *na);
        nb = realloc(nb, capacity * sizeof *nb);
        if (!na || !nb)
            abort();
    }
    ca = near_triangles(a, tris[i], &boxes[j], na);
    cb = near_triangles(b, tris[j], &boxes[i], nb);
    *alike = 1;
    for (x = 0; x < ca; x++)
        for (y = 0; y < cb; y++) {
            const double *ta = tris[i] + na[x] * 9, *tb = tris[j] + nb[y] * 9;
            int how = coplanar_overlap((const double (*)[3])ta, (const double (*)[3])tb);

            if (!how)
                continue;
            found = how > found ? how : found;
            (*triangles)++;
            if (!same_corners(map, a, na[x], b, nb[y]) || memcmp(ta, tb, 9 * sizeof(double)))
                *alike = 0;
        }
    return found;
}

static const struct Box *g_sweep;

static int by_low_x(const void *a, const void *b)
{
    const struct Box *x = &g_sweep[*(const uint32_t *)a], *y = &g_sweep[*(const uint32_t *)b];
    float lx = x->centre[0] - x->half[0], ly = y->centre[0] - y->half[0];

    return (lx > ly) - (lx < ly);
}

/* Pairs of the models' opaque records whose order can change pixels. Those with one texture are
   kept for the poses to look at (today's order between them is the sort's whim). Those with two
   textures are drawn in texture order by both today and the records, but not if the records were
   drawn one texture array after the other: counted, with how many such an order would turn
   round. */
static void order_pairs(struct Map *map, struct OrderStats *os)
{
    double **tris = calloc(map->recordCount + 1, sizeof *tris);
    uint32_t *order = malloc((map->recordCount + 1) * sizeof *order);
    struct Box *boxes = world_boxes(map);
    size_t n = 0, i, j;

    if (!tris || !order)
        abort();
    memset(os, 0, sizeof *os);
    for (i = 0; i < map->recordCount; i++)
        if (map->records[i].pass <= SW_PASS_OPAQUE_WRAP && map->records[i].half[0] < 1e29f)
            order[n++] = (uint32_t)i;
    g_sweep = boxes;
    qsort(order, n, sizeof *order, by_low_x);
    for (i = 0; i < n; i++) {
        const struct SwRecord *a = &map->records[order[i]];
        float reach = boxes[order[i]].centre[0] + boxes[order[i]].half[0] + (float)COPLANAR;

        for (j = i + 1; j < n; j++) {
            const struct SwRecord *b = &map->records[order[j]];
            uint32_t lo = order[i] < order[j] ? order[i] : order[j], hi = order[i] ^ order[j] ^ lo;
            int same, how, alike;

            if (boxes[order[j]].centre[0] - boxes[order[j]].half[0] > reach)
                break;
            if (a->pass != b->pass || !boxes_meet(&boxes[order[i]], &boxes[order[j]]))
                continue;
            same = a->sort[0] == b->sort[0] && a->sort[1] == b->sort[1];
            if (same)
                os->pairs++;
            else
                os->crossPairs++;
            how = pair_coplanar(map, boxes, tris, lo, hi, &alike, same ? &os->coplanar : &os->crossTriangles);
            if (!how)
                continue;
            if (!same) {
                const struct SwRecord *x = &map->records[lo], *y = &map->records[hi];

                os->crossCoplanar++;
                os->crossExact += how == 2;
                if (!alike && x->textureArray != y->textureArray
                    && (x->textureArray < y->textureArray) != (x->sort[0] < y->sort[0]))
                    os->crossTurned++;
                continue;
            }
            os->coplanarPairs++;
            os->exactPairs += how == 2;
            os->identical += alike;
            if (!alike)
                add_pair(map, lo, hi, how == 2);
        }
    }
    for (i = 0; i < map->recordCount; i++)
        free(tris[i]);
    free(tris);
    free(order);
    free(boxes);
}

/* ---------------------------------------------------------------------------------------------
   The GPU's passes, on this machine's GPU where there is one */

struct GpuWaterCell {
    uint32_t cell[4];
    float corners[8];
};

static SDL_GPUDevice *g_dev;
static SDL_GPUComputePipeline *g_transformPass, *g_cullPass, *g_compactPass, *g_waterPass;

static SDL_GPUComputePipeline *compute_pipeline(const struct GpuComputeCode *c)
{
    SDL_GPUComputePipelineCreateInfo info;

    SDL_zero(info);
    info.code = c->code.spirv;
    info.code_size = c->code.spirvSize;
    info.entrypoint = "main";
    info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    info.num_samplers = c->code.samplers;
    info.num_readonly_storage_textures = c->code.storageTextures;
    info.num_readonly_storage_buffers = c->code.storageBuffers;
    info.num_readwrite_storage_textures = c->readwriteStorageTextures;
    info.num_readwrite_storage_buffers = c->readwriteStorageBuffers;
    info.num_uniform_buffers = c->code.uniformBuffers;
    info.threadcount_x = c->threads[0];
    info.threadcount_y = c->threads[1];
    info.threadcount_z = c->threads[2];
    return SDL_CreateGPUComputePipeline(g_dev, &info);
}

static int open_gpu(void)
{
    SDL_PropertiesID props;

    if (!SDL_Init(SDL_INIT_VIDEO))
        return 0;
    props = SDL_CreateProperties();
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_SHADERS_SPIRV_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_CLIP_DISTANCE_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_DEPTH_CLAMPING_BOOLEAN, false);
    SDL_SetBooleanProperty(props, SDL_PROP_GPU_DEVICE_CREATE_FEATURE_INDIRECT_DRAW_FIRST_INSTANCE_BOOLEAN, false);
    g_dev = SDL_CreateGPUDeviceWithProperties(props);
    SDL_DestroyProperties(props);
    if (!g_dev)
        return 0;
    g_transformPass = compute_pipeline(&world_transform_cs);
    g_cullPass = compute_pipeline(&world_cull_cs);
    g_compactPass = compute_pipeline(&world_compact_cs);
    g_waterPass = compute_pipeline(&water_probe_cs);
    if (!g_transformPass || !g_cullPass || !g_compactPass || !g_waterPass) {
        FAIL("the GPU's passes: %s", SDL_GetError());
        return 0;
    }
    printf("static_world_maps: the GPU's passes on %s\n",
           SDL_GetStringProperty(SDL_GetGPUDeviceProperties(g_dev), SDL_PROP_GPU_DEVICE_NAME_STRING, "?"));
    return 1;
}

static void close_gpu(void)
{
    if (!g_dev)
        return;
    SDL_ReleaseGPUComputePipeline(g_dev, g_transformPass);
    SDL_ReleaseGPUComputePipeline(g_dev, g_cullPass);
    SDL_ReleaseGPUComputePipeline(g_dev, g_compactPass);
    SDL_ReleaseGPUComputePipeline(g_dev, g_waterPass);
    SDL_DestroyGPUDevice(g_dev);
    g_dev = NULL;
}

/* One dispatch of a pass, groupsAcross by groupsDown groups: the inputs uploaded into read-only
   storage buffers, the outputs downloaded from read-write ones, which start with every byte 0xab,
   so that what the pass leaves unwritten reads so and not as an earlier dispatch left the memory. */
static void gpu_dispatch(SDL_GPUComputePipeline *pass, const void *const *in, const uint32_t *inSize, int nin,
                         void *const *out, const uint32_t *outSize, int nout, const void *uniform, uint32_t uniformSize,
                         uint32_t groupsAcross, uint32_t groupsDown)
{
    SDL_GPUBuffer *inBuffers[4], *outBuffers[4];
    SDL_GPUTransferBuffer *up, *down;
    SDL_GPUTransferBufferCreateInfo ti;
    SDL_GPUBufferCreateInfo bi;
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    SDL_GPUComputePass *compute;
    SDL_GPUStorageBufferReadWriteBinding rw[4];
    SDL_GPUFence *fence;
    uint32_t upTotal = 0, downTotal = 0, at;
    unsigned char *map;
    int i;

    for (i = 0; i < nin; i++)
        upTotal += (inSize[i] + 15) & ~15u;
    for (i = 0; i < nout; i++)
        downTotal += (outSize[i] + 15) & ~15u;
    upTotal += downTotal;
    SDL_zero(ti);
    ti.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    ti.size = upTotal;
    up = SDL_CreateGPUTransferBuffer(g_dev, &ti);
    ti.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    ti.size = downTotal;
    down = SDL_CreateGPUTransferBuffer(g_dev, &ti);
    if (!up || !down)
        abort();
    map = SDL_MapGPUTransferBuffer(g_dev, up, false);
    for (i = 0, at = 0; i < nin; i++) {
        memcpy(map + at, in[i], inSize[i]);
        at += (inSize[i] + 15) & ~15u;
    }
    memset(map + at, 0xab, downTotal);
    SDL_UnmapGPUTransferBuffer(g_dev, up);
    SDL_zero(bi);
    for (i = 0; i < nin; i++) {
        bi.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
        bi.size = inSize[i];
        inBuffers[i] = SDL_CreateGPUBuffer(g_dev, &bi);
    }
    for (i = 0; i < nout; i++) {
        bi.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
        bi.size = outSize[i];
        outBuffers[i] = SDL_CreateGPUBuffer(g_dev, &bi);
    }
    cmd = SDL_AcquireGPUCommandBuffer(g_dev);
    copy = SDL_BeginGPUCopyPass(cmd);
    for (i = 0, at = 0; i < nin; i++) {
        SDL_GPUTransferBufferLocation src;
        SDL_GPUBufferRegion dst;

        src.transfer_buffer = up;
        src.offset = at;
        dst.buffer = inBuffers[i];
        dst.offset = 0;
        dst.size = inSize[i];
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
        at += (inSize[i] + 15) & ~15u;
    }
    for (i = 0; i < nout; i++) {
        SDL_GPUTransferBufferLocation src;
        SDL_GPUBufferRegion dst;

        src.transfer_buffer = up;
        src.offset = at;
        dst.buffer = outBuffers[i];
        dst.offset = 0;
        dst.size = outSize[i];
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
        at += (outSize[i] + 15) & ~15u;
    }
    SDL_EndGPUCopyPass(copy);
    SDL_zero(rw);
    for (i = 0; i < nout; i++)
        rw[i].buffer = outBuffers[i];
    compute = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, (Uint32)nout);
    SDL_BindGPUComputePipeline(compute, pass);
    SDL_BindGPUComputeStorageBuffers(compute, 0, inBuffers, (Uint32)nin);
    SDL_PushGPUComputeUniformData(cmd, 0, uniform, uniformSize);
    SDL_DispatchGPUCompute(compute, groupsAcross, groupsDown, 1);
    SDL_EndGPUComputePass(compute);
    copy = SDL_BeginGPUCopyPass(cmd);
    for (i = 0, at = 0; i < nout; i++) {
        SDL_GPUBufferRegion src;
        SDL_GPUTransferBufferLocation dst;

        src.buffer = outBuffers[i];
        src.offset = 0;
        src.size = outSize[i];
        dst.transfer_buffer = down;
        dst.offset = at;
        SDL_DownloadFromGPUBuffer(copy, &src, &dst);
        at += (outSize[i] + 15) & ~15u;
    }
    SDL_EndGPUCopyPass(copy);
    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    SDL_WaitForGPUFences(g_dev, true, &fence, 1);
    SDL_ReleaseGPUFence(g_dev, fence);
    map = SDL_MapGPUTransferBuffer(g_dev, down, false);
    for (i = 0, at = 0; i < nout; i++) {
        memcpy(out[i], map + at, outSize[i]);
        at += (outSize[i] + 15) & ~15u;
    }
    SDL_UnmapGPUTransferBuffer(g_dev, down);
    for (i = 0; i < nin; i++)
        SDL_ReleaseGPUBuffer(g_dev, inBuffers[i]);
    for (i = 0; i < nout; i++)
        SDL_ReleaseGPUBuffer(g_dev, outBuffers[i]);
    SDL_ReleaseGPUTransferBuffer(g_dev, up);
    SDL_ReleaseGPUTransferBuffer(g_dev, down);
}

/* the client's 4x3 view (render/gfx.h) as a row-vector 4x4 matrix: a slot's view to view space,
   as world_transform.hlsl writes it beside the clip-space one */
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

struct GpuStats {
    size_t slots, slotsDiffer, draws, drawsDiffer, runs, runsDiffer, records, kept, cells, cellsDiffer, cellsDrawn;
};

static struct GpuStats g_gpuStats;
/* the GPU's cull of the map's first view is to be timed */
static int g_cullTimeDue;

/* the slots' views and places, packed as the device packs them (render/world_layout.h): the C's to
   the bit, and each reference draw's view its record's slot's; returns the GPU's places, which the
   cull takes */
static float (*gpu_transforms(struct Map *map, const float *view))[12]
{
    static struct WlSlot *slots;
    static struct WorldViewOut {
        float toClip[16];
        float toView[16];
    } *views;
    static float (*placed)[12], (*casters)[16];
    static size_t capacity;
    struct WlTransformFrame u;
    const void *in[2];
    void *out[3];
    uint32_t inSize[2], outSize[3], s;
    size_t i;
    float proj[16];

    (void)view;
    if (capacity < D.slotCount + 1) {
        capacity = D.slotCount + 1;
        slots = realloc(slots, capacity * sizeof *slots);
        views = realloc(views, capacity * sizeof *views);
        placed = realloc(placed, capacity * sizeof *placed);
        casters = realloc(casters, capacity * WL_CASCADES * sizeof *casters);
        if (!slots || !views || !placed || !casters)
            abort();
    }
    for (s = 0; s < D.slotCount; s++)
        wl_slot(D.slots[s].local, D.slots[s].parent, D.slots[s].animated, s == 0, &slots[s]);
    wl_projection(proj, D.camera.proj, D.camera.centre, D.camera.nearZ, D.camera.depthScale, g_renderer->width,
                  g_renderer->height);
    wl_transform_frame(D.camera.view, proj, NULL, 0, (uint32_t)D.slotCount, &u);
    in[0] = slots;
    inSize[0] = (uint32_t)(D.slotCount * sizeof *slots);
    in[1] = D.animated;
    inSize[1] = (uint32_t)((D.animatedCount ? D.animatedCount : 1) * sizeof *D.animated);
    out[0] = views;
    outSize[0] = (uint32_t)(D.slotCount * sizeof *views);
    out[1] = placed;
    outSize[1] = (uint32_t)(D.slotCount * sizeof *placed);
    out[2] = casters;
    outSize[2] = (uint32_t)(D.slotCount * WL_CASCADES * sizeof *casters);
    gpu_dispatch(g_transformPass, in, inSize, 2, out, outSize, 3, &u, sizeof u, (uint32_t)(D.slotCount + 63) / 64, 1);
    for (s = 0; s < D.slotCount; s++) {
        float toView[16], toClip[16];

        matrix_4x3(toView, map->views[s]);
        wl_view_clip(toClip, map->views[s], proj);
        g_gpuStats.slots++;
        if (memcmp(toView, views[s].toView, sizeof toView) || memcmp(toClip, views[s].toClip, sizeof toClip)
            || memcmp(placed[s], map->transforms[s], sizeof placed[s])) {
            if (g_gpuStats.slotsDiffer++ < 5)
                FAIL("%s: the GPU's slot %u is not the C's", map->name, s);
        }
    }
    for (i = 0; i < g_drawCount; i++) {
        const struct DeviceDraw *d = &g_draws[i];
        const struct Source *src = d->cascade < 0 ? mesh_source(map, d->mesh) : NULL;
        long rec = src ? record_at(map, src->place.firstIndex + (uint32_t)d->draw.first) : -1;
        float toView[16], toClip[16];

        if (rec < 0)
            continue;
        matrix_4x3(toView, d->view);
        wl_view_clip(toClip, d->view, proj);
        g_gpuStats.draws++;
        if (memcmp(toClip, views[map->records[rec].transform].toClip, sizeof toClip)
            || memcmp(toView, views[map->records[rec].transform].toView, sizeof toView)) {
            if (g_gpuStats.drawsDiffer++ < 5)
                FAIL("%s: a draw's clip space is not the reference's", map->name);
        }
    }
    return placed;
}

/* The cull and the compaction as the device runs them (render/gfx_gpu.c, world_compute), their
   dispatches split as wl_groups splits them, or into rows of `across` groups where it is not 0: the
   kept records' places, the runs' commands and the indices drawn. */
static void gpu_compact(const struct WlCullRecord *cull, size_t records, float (*placed)[12], const struct WlRun *runs,
                        size_t m, const uint32_t *indices, const struct WlChunk *chunks, size_t nchunks,
                        uint32_t drawnIndices, uint32_t across, uint32_t *places, uint32_t *commands, uint32_t *drawn)
{
    struct WlCullFrame u;
    struct WlCompactFrame c;
    const void *in[4];
    void *out[2];
    uint32_t inSize[4], outSize[2], groups[2];

    wl_cull_frame(&D.frustum, (uint32_t)m, &u, groups);
    if (across) {
        u.counts[1] = groups[0] = across;
        groups[1] = (uint32_t)((m + across - 1) / across);
    }
    in[0] = cull;
    inSize[0] = (uint32_t)(records * sizeof *cull);
    in[1] = placed;
    inSize[1] = (uint32_t)(D.slotCount * sizeof *placed);
    in[2] = runs;
    inSize[2] = (uint32_t)(m * sizeof *runs);
    out[0] = places;
    outSize[0] = (uint32_t)(records * sizeof *places);
    out[1] = commands;
    outSize[1] = (uint32_t)(m * 5 * sizeof *commands);
    gpu_dispatch(g_cullPass, in, inSize, 3, out, outSize, 2, &u, sizeof u, groups[0], groups[1]);
    wl_compact_frame((uint32_t)nchunks, &c, groups);
    if (!groups[0])
        return;
    if (across) {
        c.counts[1] = groups[0] = across;
        groups[1] = (uint32_t)((nchunks + across - 1) / across);
    }
    in[1] = places;
    inSize[1] = (uint32_t)(records * sizeof *places);
    in[2] = indices;
    inSize[2] = (uint32_t)(D.indexCount * sizeof *indices);
    in[3] = chunks;
    inSize[3] = (uint32_t)(nchunks * sizeof *chunks);
    out[0] = drawn;
    outSize[0] = (drawnIndices ? drawnIndices : 1) * (uint32_t)sizeof *drawn;
    gpu_dispatch(g_compactPass, in, inSize, 4, out, outSize, 1, &c, sizeof c, groups[0], groups[1]);
}

/* The GPU's time for the cull and the compaction of a view as the device runs them each frame: 100
   of them in one command buffer, less the time an empty one takes, the least of 5 tries of each,
   in milliseconds a view. */
static double gpu_cull_time(const struct WlCullRecord *cull, size_t records, float (*placed)[12],
                            const struct WlRun *runs, size_t m, const uint32_t *indices, const struct WlChunk *chunks,
                            size_t nchunks, uint32_t drawnIndices)
{
    enum { REPS = 100, TRIES = 5 };
    const void *in[5];
    uint32_t inSize[5], outSize[3], groups[2][2], at = 0, total = 0;
    SDL_GPUBuffer *inBuf[5], *outBuf[3];
    SDL_GPUTransferBufferCreateInfo ti;
    SDL_GPUBufferCreateInfo bi;
    SDL_GPUTransferBuffer *up;
    SDL_GPUCommandBuffer *cmd;
    SDL_GPUCopyPass *copy;
    SDL_GPUFence *fence;
    struct WlCullFrame u;
    struct WlCompactFrame c;
    unsigned char *bytes;
    int64_t took[2] = { INT64_MAX, INT64_MAX };
    int i, r, n, t;

    in[0] = cull;
    inSize[0] = (uint32_t)(records * sizeof *cull);
    in[1] = placed;
    inSize[1] = (uint32_t)(D.slotCount * sizeof *placed);
    in[2] = runs;
    inSize[2] = (uint32_t)(m * sizeof *runs);
    in[3] = indices;
    inSize[3] = (uint32_t)(D.indexCount * sizeof *indices);
    in[4] = chunks;
    inSize[4] = (uint32_t)((nchunks ? nchunks : 1) * sizeof *chunks);
    wl_cull_frame(&D.frustum, (uint32_t)m, &u, groups[0]);
    wl_compact_frame((uint32_t)nchunks, &c, groups[1]);
    outSize[0] = (uint32_t)(records * 4);
    outSize[1] = (uint32_t)(m * 5 * 4);
    outSize[2] = (drawnIndices ? drawnIndices : 1) * 4;
    for (i = 0; i < 5; i++)
        total += (inSize[i] + 15) & ~15u;
    SDL_zero(ti);
    ti.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    ti.size = total;
    up = SDL_CreateGPUTransferBuffer(g_dev, &ti);
    SDL_zero(bi);
    for (i = 0; i < 5; i++) {
        bi.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ;
        bi.size = inSize[i];
        inBuf[i] = SDL_CreateGPUBuffer(g_dev, &bi);
        if (!inBuf[i])
            abort();
    }
    for (i = 0; i < 3; i++) {
        bi.usage = SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
        bi.size = outSize[i];
        outBuf[i] = SDL_CreateGPUBuffer(g_dev, &bi);
        if (!outBuf[i])
            abort();
    }
    if (!up || !(bytes = SDL_MapGPUTransferBuffer(g_dev, up, false)))
        abort();
    for (i = 0; i < 5; i++) {
        if (in[i] && inSize[i])
            memcpy(bytes + at, in[i], inSize[i]);
        at += (inSize[i] + 15) & ~15u;
    }
    SDL_UnmapGPUTransferBuffer(g_dev, up);
    cmd = SDL_AcquireGPUCommandBuffer(g_dev);
    copy = SDL_BeginGPUCopyPass(cmd);
    for (i = 0, at = 0; i < 5; i++) {
        SDL_GPUTransferBufferLocation src;
        SDL_GPUBufferRegion dst;

        src.transfer_buffer = up;
        src.offset = at;
        dst.buffer = inBuf[i];
        dst.offset = 0;
        dst.size = inSize[i];
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
        at += (inSize[i] + 15) & ~15u;
    }
    SDL_EndGPUCopyPass(copy);
    fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    SDL_WaitForGPUFences(g_dev, true, &fence, 1);
    SDL_ReleaseGPUFence(g_dev, fence);
    /* an empty command buffer, then one of the passes, by turns */
    for (t = 0; t < 2 * TRIES; t++) {
        int64_t start, ns;

        r = t & 1;

        cmd = SDL_AcquireGPUCommandBuffer(g_dev);
        for (n = 0; r && n < REPS; n++) {
            SDL_GPUStorageBufferReadWriteBinding rw[2];
            SDL_GPUBuffer *read[4];
            SDL_GPUComputePass *pass;

            SDL_zero(rw);
            rw[0].buffer = outBuf[0];
            rw[1].buffer = outBuf[1];
            pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 2);
            SDL_BindGPUComputePipeline(pass, g_cullPass);
            read[0] = inBuf[0];
            read[1] = inBuf[1];
            read[2] = inBuf[2];
            SDL_BindGPUComputeStorageBuffers(pass, 0, read, 3);
            SDL_PushGPUComputeUniformData(cmd, 0, &u, sizeof u);
            SDL_DispatchGPUCompute(pass, groups[0][0], groups[0][1], 1);
            SDL_EndGPUComputePass(pass);
            if (!groups[1][0])
                continue;
            rw[0].buffer = outBuf[2];
            pass = SDL_BeginGPUComputePass(cmd, NULL, 0, rw, 1);
            SDL_BindGPUComputePipeline(pass, g_compactPass);
            read[1] = outBuf[0];
            read[2] = inBuf[3];
            read[3] = inBuf[4];
            SDL_BindGPUComputeStorageBuffers(pass, 0, read, 4);
            SDL_PushGPUComputeUniformData(cmd, 0, &c, sizeof c);
            SDL_DispatchGPUCompute(pass, groups[1][0], groups[1][1], 1);
            SDL_EndGPUComputePass(pass);
        }
        start = (int64_t)SDL_GetTicksNS();
        fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
        SDL_WaitForGPUFences(g_dev, true, &fence, 1);
        ns = (int64_t)SDL_GetTicksNS() - start;
        took[r] = ns < took[r] ? ns : took[r];
        SDL_ReleaseGPUFence(g_dev, fence);
    }
    for (i = 0; i < 5; i++)
        SDL_ReleaseGPUBuffer(g_dev, inBuf[i]);
    for (i = 0; i < 3; i++)
        SDL_ReleaseGPUBuffer(g_dev, outBuf[i]);
    SDL_ReleaseGPUTransferBuffer(g_dev, up);
    return (double)(took[1] - took[0]) / REPS / 1e6;
}

/* The view's cull, packed as the device packs it (render/world_layout.h): each run of the passes
   drawn indirectly holds exactly the indices of the records sw_record_in_view keeps, their base
   vertex added, one after the other in the records' order, and its command draws them; and so it
   does with the dispatches split into rows of 3 and of 7 groups. */
static void gpu_cull(struct Map *map, float (*placed)[12], const int *keptAt)
{
    static struct WlRecord *byId;
    static struct WlCullRecord *cull;
    static struct SwDrawRun *runs;
    static struct WlRun *wlruns;
    static uint32_t *indices, *places, *commands, *drawn, *places2, *commands2, *drawn2;
    static struct WlChunk *chunks;
    static size_t capacity, idCapacity, indexCapacity, drawnCapacity, chunkCapacity;
    uint32_t total, k, records;
    size_t i, nruns, m, nchunks;
    int split;

    if (capacity < map->recordCount + 1) {
        capacity = map->recordCount + 1;
        cull = realloc(cull, capacity * sizeof *cull);
        runs = realloc(runs, capacity * sizeof *runs);
        wlruns = realloc(wlruns, capacity * sizeof *wlruns);
        places = realloc(places, capacity * sizeof *places);
        places2 = realloc(places2, capacity * sizeof *places2);
        commands = realloc(commands, capacity * 5 * sizeof *commands);
        commands2 = realloc(commands2, capacity * 5 * sizeof *commands2);
        if (!cull || !runs || !wlruns || !places || !places2 || !commands || !commands2)
            abort();
    }
    byId = grow(byId, &idCapacity, D.idCount + 1, sizeof *byId);
    indices = grow(indices, &indexCapacity, D.indexCount + 1, sizeof *indices);
    wl_records(map->records, map->recordCount, byId, cull);
    nruns = sw_draw_runs(map->records, map->recordCount, runs);
    m = wl_runs(runs, nruns, wlruns, &total);
    if (!m)
        return;
    records = wlruns[m - 1].first + wlruns[m - 1].count;
    if (drawnCapacity < (size_t)total + 1) {
        drawnCapacity = (size_t)total + 1;
        drawn = realloc(drawn, drawnCapacity * sizeof *drawn);
        drawn2 = realloc(drawn2, drawnCapacity * sizeof *drawn2);
        if (!drawn || !drawn2)
            abort();
    }
    for (i = 0; i < D.indexCount; i++)
        indices[i] = D.indices[i];
    nchunks = wl_chunks(map->records, records, NULL);
    chunks = grow(chunks, &chunkCapacity, nchunks + 1, sizeof *chunks);
    if (wl_chunks(map->records, records, chunks) != nchunks)
        FAIL("%s: the chunks counted are not the chunks written", map->name);
    gpu_compact(cull, records, placed, wlruns, m, indices, chunks, nchunks, total, 0, places, commands, drawn);
    if (g_cullTimeDue) {
        g_cullTimeDue = 0;
        printf("%s: the GPU's cull and compaction of the first view: %.3f ms (%zu runs, %u records, %zu chunks)\n",
               map->name, gpu_cull_time(cull, records, placed, wlruns, m, indices, chunks, nchunks, total), m, records,
               nchunks);
    }
    for (i = 0; i < m; i++) {
        const struct WlRun *r = &wlruns[i];
        const uint32_t *c = &commands[i * 5];
        uint32_t at = r->start, rec;
        int differs = 0;

        g_gpuStats.runs++;
        for (rec = r->first; rec < r->first + r->count; rec++) {
            const struct SwRecord *x = &map->records[rec];

            g_gpuStats.records++;
            if (keptAt[rec] < 0)
                continue;
            g_gpuStats.kept++;
            for (k = 0; k < x->indexCount && !differs; k++)
                differs = drawn[at + k] != D.indices[x->firstIndex + k] + x->baseVertex;
            at += x->indexCount;
        }
        if (differs || c[0] != at - r->start || c[1] != 1 || c[2] != r->start || c[3] != 0 || c[4] != 0) {
            if (g_gpuStats.runsDiffer++ < 5)
                FAIL("%s: the GPU's cull of run %zu (pass %d) is not sw_record_in_view's (%u indices, the C %u)",
                     map->name, i, runs[i].pass, c[0], at - r->start);
        }
    }
    /* the same, both dispatches split into rows of 3 groups, then of 7 */
    for (split = 0; split < 2; split++) {
        int same;

        gpu_compact(cull, records, placed, wlruns, m, indices, chunks, nchunks, total, split ? 7 : 3, places2, commands2,
                    drawn2);
        same = !memcmp(places, places2, records * sizeof *places) && !memcmp(commands, commands2, m * 5 * sizeof *commands);
        for (i = 0; i < m && same; i++)
            same = !memcmp(&drawn[wlruns[i].start], &drawn2[wlruns[i].start], commands[i * 5] * sizeof *drawn);
        if (!same && g_gpuStats.runsDiffer++ < 5)
            FAIL("%s: the GPU's cull split into rows of %d groups is not the one dispatch's", map->name, split ? 7 : 3);
    }
}

/* the water's quads over every cell of the map: the reference's to the bit */
static void gpu_water(struct Map *map, const float *view)
{
    static struct GpuWaterCell *cells;
    static float (*results)[4];
    static size_t capacity;
    struct {
        float camera[16];
        uint32_t counts[4];
    } u;
    const void *in[3];
    void *out[1];
    uint32_t inSize[3], outSize[1];
    struct C3dGround15 *ground = g_world.ground;
    size_t i;
    int k;

    if (!D.cellCount)
        return;
    if (capacity < D.cellCount + 1) {
        capacity = D.cellCount + 1;
        cells = realloc(cells, capacity * sizeof *cells);
        results = realloc(results, capacity * 4 * sizeof *results);
        if (!cells || !results)
            abort();
    }
    for (i = 0; i < D.cellCount; i++) {
        cells[i].cell[0] = (uint32_t)D.cells[i].x;
        cells[i].cell[1] = (uint32_t)D.cells[i].y;
        cells[i].cell[2] = (uint32_t)D.cells[i].block;
        memcpy(&cells[i].cell[3], &D.cells[i].top, 4);
        for (k = 0; k < 4; k++) {
            cells[i].corners[k * 2] = D.cells[i].corners[k][0];
            cells[i].corners[k * 2 + 1] = D.cells[i].corners[k][1];
        }
    }
    memset(&u, 0, sizeof u);
    memcpy(u.camera, view, 12 * sizeof *view);
    u.camera[12] = g_renderer->proj[0];
    u.camera[13] = g_renderer->proj[1];
    u.camera[14] = (float)g_renderer->centreX;
    u.camera[15] = (float)g_renderer->centreY;
    u.counts[0] = (uint32_t)D.cellCount;
    in[0] = cells;
    inSize[0] = (uint32_t)(D.cellCount * sizeof *cells);
    in[1] = D.waves;
    inSize[1] = (uint32_t)((D.waveCount ? D.waveCount : 1) * sizeof *D.waves);
    in[2] = D.sines;
    inSize[2] = sizeof D.sines;
    out[0] = results;
    outSize[0] = (uint32_t)(D.cellCount * 4 * sizeof *results);
    gpu_dispatch(g_waterPass, in, inSize, 3, out, outSize, 1, &u, sizeof u, (uint32_t)(D.cellCount + 63) / 64, 1);
    for (i = 0; i < D.cellCount; i++) {
        const struct GfxWaterCell *c = &D.cells[i];
        const struct GndWater *w = &ground->waters[c->block];
        struct GndCell *cell = GND_CELL(ground, c->x, c->y);
        int64_t diagonal = (int64_t)c->x + c->y;
        float heights[3], want[16], key;
        struct TexDrawVertex v[4];
        int drawn;

        heights[0] = ground_water_height(w, diagonal - 1);
        heights[1] = ground_water_height(w, diagonal);
        heights[2] = ground_water_height(w, diagonal + 1);
        for (k = 0; k < 4; k++) {
            float p[3];
            Vec3 q;

            c3d_ground15_corner(ground, c->x + (k & 1), c->y + (k >> 1), heights[k == 0 ? 0 : k == 3 ? 2 : 1], p);
            q.x = p[0];
            q.y = p[1];
            q.z = p[2];
            renderer_project_vertex(g_renderer, &q, view, &v[k]);
        }
        drawn = ref_water(ground, c->x, c->y, cell, view, &key);
        want[0] = heights[0];
        want[1] = heights[1];
        want[2] = heights[2];
        want[3] = drawn ? 1.0f : 0.0f;
        for (k = 0; k < 4; k++) {
            want[4 + k] = v[k].rhw;
            want[8 + k * 2] = v[k].x;
            want[9 + k * 2] = v[k].y;
        }
        g_gpuStats.cells++;
        g_gpuStats.cellsDrawn += drawn;
        if (memcmp(want, results[i * 4], sizeof want)) {
            if (g_gpuStats.cellsDiffer++ < 5) {
                FAIL("%s: the GPU's water quad over cell %d, %d is not the reference's", map->name, c->x, c->y);
                for (k = 0; k < 16; k++)
                    if (memcmp(&want[k], &results[i * 4][k], sizeof want[k]))
                        fprintf(stderr, "    value %d: %.9g, the GPU %.9g\n", k, want[k], results[i * 4][k]);
            }
        }
    }
}

/* The operations of a shader that may part from the C's results now and then: scalar float
   additions, subtractions, multiplications and divisions without NoContraction (HLSL's precise),
   which the GPU may fuse, reorder or factor; fused multiply-adds (mad()), which Vulkan does not
   promise to fuse; matrix and dot products (mul()), which leave the order of their sums to the
   GPU. */
static int bare_operations(const char *name, const unsigned char *code, size_t size)
{
    const uint32_t *w = (const uint32_t *)code;
    size_t n = size / 4, i;
    uint32_t bound, floatType = 0, *decorated;
    int bare = 0;

    if (n < 5 || w[0] != 0x07230203u)
        return 1;
    bound = w[3];
    decorated = calloc(bound, sizeof *decorated);
    if (!decorated)
        abort();
    for (i = 5; i < n; i += w[i] >> 16) {
        uint32_t op = w[i] & 0xffff;

        if ((w[i] >> 16) == 0)
            break;
        if (op == 22 && w[i + 2] == 32)
            floatType = w[i + 1];
        if (op == 71 && w[i + 2] == 42 && w[i + 1] < bound)
            decorated[w[i + 1]] = 1;
    }
    for (i = 5; i < n; i += w[i] >> 16) {
        uint32_t op = w[i] & 0xffff;
        int arithmetic = op == 129 || op == 131 || op == 133 || op == 136;

        if ((w[i] >> 16) == 0)
            break;
        if (arithmetic && w[i + 1] == floatType && w[i + 2] < bound && !decorated[w[i + 2]])
            bare++;
        if ((op == 12 && w[i + 4] == 50) || op == 144 || op == 145 || op == 146 || op == 148)
            bare++;
    }
    free(decorated);
    printf("static_world_maps: %s: %d operations that may part from the C\n", name, bare);
    return bare;
}

/* ---------------------------------------------------------------------------------------------
   One pose: the reference's draws against the records' */

struct Stats {
    size_t poses, today, kept, both;
    size_t todayOnly, todayOnlySeen;
    size_t keptOnly, keptOnlySeen, keptOnlySeenLeaf, keptOnlySeenBox, keptOnlySeenGround;
    double keptOnlyMostArea;
    size_t casts, castKept, castBoth, castTodayOnly, castTodayOnlyTouching, castKeptOnly, castKeptOnlyTouching;
    size_t castKeptOnlyTouchingGround;
    double castKeptOnlyMostArea;
    size_t keptOnlySeenAnimated, castKeptOnlyTouchingAnimated, pairsBoth, pairsFlipped, exactFlipped;
    size_t sequenceDiffers, tieOrder, alphaOrdered;
    size_t mismatches, viewsDiffer;
    size_t clientCasts, clientAlpha, clientAlphaKeyed, clientAlphaOnly, clientWater, referenceWater;
    size_t untickedMoving, tickedByView;
};

static int device_pass(const struct DeviceDraw *d)
{
    switch (d->phase) {
    case PERF_L_GROUND: return d->address == DRAW_ADDRESS_WRAP ? SW_PASS_OPAQUE_WRAP : SW_PASS_OPAQUE_CLAMP;
    case PERF_L_WATER: return SW_PASS_GROUND;
    case PERF_L_ALPHATEST: return d->address == DRAW_ADDRESS_WRAP ? SW_PASS_ALPHA_WRAP : SW_PASS_ALPHA_CLAMP;
    case PERF_L_ALPHA_WRITE_DEPTH: return SW_PASS_ALPHA_DEPTH;
    default: return -1;
    }
}

static float today_key(const struct DeviceDraw *d)
{
    const struct GfxMeshVertex *p = &d->mesh->verts[d->mesh->indices[d->draw.first]];
    Vec3 v = {p->x, p->y, p->z};
    struct TexDrawVertex corner;

    renderer_project_vertex(g_renderer, &v, d->view, &corner);
    return corner.rhw;
}

static void mismatch(struct Map *map, struct Stats *st, const char *what, const struct SwRecord *r)
{
    if (st->mismatches++ < 20)
        FAIL("%s: record at index %u (pass %d): %s differs from the reference's draw", map->name, r->firstIndex,
             r->pass, what);
    else
        g_failures++;
}

/* a record against the reference's draw of it: the same mesh data, textures, cull, kind and
   texture-coordinate matrix, and its slot's view the reference's matrix to the bit */
static void compare_draw(struct Map *map, struct Stats *st, const struct SwRecord *r, const struct DeviceDraw *d)
{
    const struct Source *s = source_of(map, r->firstIndex);
    static const float identity[6] = {1, 0, 0, 1, 0, 0};
    const float *uv = r->uv == SW_NONE ? identity : D.uv[r->uv];
    struct Texture *texture = d->texture[0] ? d->texture[0] : &g_whiteTexture;
    struct Texture *layer = r->textureArray == SW_NO_ARRAY ? NULL : D.layers[r->textureArray][r->textureLayer];
    DWORD cull = r->flags & SW_CULL_NONE ? CULL_NONE : r->flags & SW_CULL_CCW ? CULL_CCW : CULL_CW;
    uint32_t k;

    if (d->mesh != s->mesh || (uint32_t)d->draw.first != r->firstIndex - s->place.firstIndex
        || (uint32_t)d->draw.count != r->indexCount)
        mismatch(map, st, "the index range", r);
    for (k = 0; k < r->indexCount; k++)
        if (memcmp(&map->vertices[map->indices[r->firstIndex + k] + r->baseVertex],
                   &d->mesh->verts[d->mesh->indices[d->draw.first + k]], sizeof(struct GfxMeshVertex))) {
            mismatch(map, st, "the mesh data", r);
            break;
        }
    /* a texture without pixels has no layer, and the device draws it white */
    if (layer ? layer != texture : texture->base.surface != NULL)
        mismatch(map, st, "the texture", r);
    if (r->pass == SW_PASS_GROUND ? (r->lightmapArray == SW_NO_ARRAY
                                     || D.layers[r->lightmapArray][r->lightmapLayer] != d->texture[1])
                                  : (r->lightmapArray != SW_NO_ARRAY || d->texture[1]))
        mismatch(map, st, "the lightmap", r);
    if ((r->pass == SW_PASS_GROUND) != (d->draw.kind == GFX_MESH_GROUND))
        mismatch(map, st, "the kind", r);
    if (d->draw.cull != (int)cull)
        mismatch(map, st, "the cull", r);
    if (memcmp(d->draw.uv, uv, sizeof d->draw.uv))
        mismatch(map, st, "the texture-coordinate matrix", r);
    if (memcmp(map->views[r->transform], d->view, sizeof d->view)) {
        st->viewsDiffer++;
        mismatch(map, st, "the view", r);
    }
}

/* whether the reference left the actor's draws out: none of its leaves in the view, or only
   crossing ones and its box outside */
static int actor_leaf_culled(const struct ViewFrustum *f, const struct C3dActor *a, int *byBox)
{
    size_t i, k;
    int crossing = 0;

    for (i = 0; i < f->inside_count; i++)
        for (k = 0; k < f->inside[i]->objects.count; k++)
            if (f->inside[i]->objects.data[k] == a)
                return 0;
    for (i = 0; i < f->crossing_count && !crossing; i++)
        for (k = 0; k < f->crossing[i]->objects.count; k++)
            if (f->crossing[i]->objects.data[k] == a)
                crossing = 1;
    *byBox = crossing;
    return 1;
}

/* whether every leaf the actor stands in reaches as high and as low as its box */
static void leaves_hold(const struct SceneGraphNode *node, int level, const struct C3dActor *a, int *held, int *count)
{
    size_t i;

    if (level < SCENE_GRAPH_DEPTH) {
        for (i = 0; i < 4; i++)
            if (node->child[i])
                leaves_hold(node->child[i], level + 1, a, held, count);
        return;
    }
    for (i = 0; i < node->objects.count; i++)
        if (node->objects.data[i] == a) {
            (*count)++;
            if (node->min[1] > a->bboxMin[1] || node->max[1] < a->bboxMax[1])
                *held = 0;
        }
}

#define MAX_DRAWS (1u << 20)
static uint32_t g_out[MAX_DRAWS], g_cast[MAX_DRAWS];

struct Range {
    uint32_t first, count;
};

static int by_range(const void *a, const void *b)
{
    const struct Range *x = a, *y = b;

    if (x->first != y->first)
        return x->first < y->first ? -1 : 1;
    return (x->count > y->count) - (x->count < y->count);
}

/* render/crenderer.c's keyed_less on the client's entries: the order an alpha list draws in */
static unsigned char client_keyed_less(const void *a, const void *b, void *pred)
{
    const struct RenderEntry *x = a, *y = b;
    (void)pred;
    if (y->key > x->key)
        return 1;
    if (x->key > y->key)
        return 0;
    return x->order < y->order;
}

/* the casters, the alpha-blended records and the water the client's view_on_render queued, against
   the reference's and the records' cull: the casters in the reference's order, the alpha-blended
   records in the order the renderer draws them, that of the reference's draws to the entry */
static void client_queues(struct Map *map, struct Stats *st, const int *keptAt, const struct DeviceDraw **drawOf)
{
    static struct Range *client, *reference;
    static size_t clientCapacity, referenceCapacity;
    static struct RenderEntry *sorted;
    static size_t sortedCapacity;
    static long *mine, *theirs;
    static size_t mineCapacity, theirsCapacity;
    static float *waterKeys;
    static size_t waterCapacity;
    const struct RenderQueue *q = &g_renderer->queues[RENDER_SHADOW_CASTERS];
    size_t n = 0, m = 0, i, alpha = 0;
    int list;

    /* the casters: every reference caster, a record's run or a block, once, in its order */
    for (i = 0; i < q->count; i++) {
        const struct RPWorldDraw *w = q->items[i].primitive;

        if (!q->items[i].world || w->kind != RP_WORLD_CAST)
            continue;
        client = grow(client, &clientCapacity, n + 1, sizeof *client);
        client[n].first = w->firstIndex;
        client[n].count = w->count;
        n++;
    }
    for (i = 0; i < g_drawCount; i++) {
        const struct DeviceDraw *d = &g_draws[i];
        const struct Source *s;

        if (d->cascade < 0)
            continue;
        s = mesh_source(map, d->mesh);
        if (!s)
            continue;
        reference = grow(reference, &referenceCapacity, m + 1, sizeof *reference);
        reference[m].first = s->place.firstIndex + (uint32_t)d->draw.first;
        reference[m].count = (uint32_t)d->draw.count;
        m++;
    }
    st->clientCasts += n;
    if (n != m || (n && memcmp(client, reference, n * sizeof *client)))
        FAIL("%s: the client queues %zu casters, the reference %zu, not the same in the same order", map->name, n, m);

    /* the alpha-blended records: those the records' cull keeps, keyed to the bit as the reference
       keyed its draws, and drawn in the reference's order where it drew them, ties too */
    for (list = 0; list < 3; list++) {
        static const int lists[3] = {RENDER_ALPHA_CLAMP, RENDER_ALPHA_WRAP, RENDER_ALPHA_WRITE_DEPTH};
        static const int phases[3] = {PERF_L_ALPHATEST, PERF_L_ALPHATEST, PERF_L_ALPHA_WRITE_DEPTH};
        const struct RenderQueue *a = &g_renderer->queues[lists[list]];
        int pass = lists[list] == RENDER_ALPHA_CLAMP ? SW_PASS_ALPHA_CLAMP
                   : lists[list] == RENDER_ALPHA_WRAP ? SW_PASS_ALPHA_WRAP
                                                      : SW_PASS_ALPHA_DEPTH;
        size_t nm = 0, nt = 0, x, y;

        sorted = grow(sorted, &sortedCapacity, a->count + 1, sizeof *sorted);
        memcpy(sorted, a->items, a->count * sizeof *sorted);
        if (a->count > 1)
            std_sort(sorted, sorted + a->count, sizeof *sorted, client_keyed_less, NULL);
        mine = grow(mine, &mineCapacity, a->count + 1, sizeof *mine);
        for (i = 0; i < a->count; i++) {
            const struct RPWorldDraw *w = sorted[i].primitive;
            long rec;

            if (!sorted[i].world || w->kind != RP_WORLD_RECORD)
                continue;
            alpha++;
            rec = record_at(map, w->record.firstIndex);
            if (rec < 0 || keptAt[rec] < 0 || map->records[rec].pass != pass) {
                FAIL("%s: the client queues an alpha record the cull does not keep", map->name);
                continue;
            }
            if (drawOf[rec]) {
                float key = today_key(drawOf[rec]);

                st->clientAlphaKeyed++;
                if (memcmp(&sorted[i].key, &key, sizeof key))
                    FAIL("%s: the client keys an alpha record %.9g, the reference %.9g", map->name, sorted[i].key, key);
                mine[nm++] = rec;
            } else {
                st->clientAlphaOnly++;
            }
        }
        theirs = grow(theirs, &theirsCapacity, g_drawCount + 1, sizeof *theirs);
        for (i = 0; i < g_drawCount; i++) {
            const struct DeviceDraw *d = &g_draws[i];
            const struct Source *s;
            long rec;

            if (d->cascade >= 0 || d->phase != phases[list] || device_pass(d) != pass)
                continue;
            s = mesh_source(map, d->mesh);
            rec = s ? record_at(map, s->place.firstIndex + (uint32_t)d->draw.first) : -1;
            if (rec >= 0 && keptAt[rec] >= 0)
                theirs[nt++] = rec;
        }
        for (x = y = 0; x < nm && y < nt && mine[x] == theirs[y]; x++, y++)
            ;
        st->alphaOrdered += nm;
        if (nm != nt || x != nm)
            FAIL("%s: pass %d draws its alpha-blended records in another order than the reference (%zu and %zu, "
                 "the same up to %zu)", map->name, pass, nm, nt, x);
    }
    {
        size_t kept = 0;

        for (i = 0; i < map->recordCount; i++)
            kept += keptAt[i] >= 0 && map->records[i].pass >= SW_PASS_ALPHA_CLAMP;
        if (kept != alpha)
            FAIL("%s: the client queues %zu alpha records, the cull keeps %zu", map->name, alpha, kept);
        st->clientAlpha += alpha;
    }

    /* the water: every quad the reference drew, keyed to the bit */
    waterKeys = grow(waterKeys, &waterCapacity, D.cellCount + 1, sizeof *waterKeys);
    for (i = 0; i < D.cellCount; i++)
        waterKeys[i] = NAN;
    q = &g_renderer->queues[RENDER_ALPHA_CLAMP];
    for (i = 0; i < q->count; i++) {
        const struct RPWorldDraw *w = q->items[i].primitive;

        if (!q->items[i].world || w->kind != RP_WORLD_WATER)
            continue;
        if (w->cell >= D.cellCount || !isnan(waterKeys[w->cell]))
            FAIL("%s: a water cell queued twice or outside the map's", map->name);
        else
            waterKeys[w->cell] = q->items[i].key;
        st->clientWater++;
    }
    st->referenceWater += g_waterCount;
    for (i = 0; i < g_waterCount; i++) {
        const struct GndCell *cell = GND_CELL(g_world.ground, g_water[i].x, g_water[i].y);

        if (cell->water < 0 || (size_t)cell->water >= D.cellCount
            || memcmp(&waterKeys[cell->water], &g_water[i].key, sizeof(float)))
            FAIL("%s: the reference's water over cell %d, %d is not queued with its key", map->name, g_water[i].x,
                 g_water[i].y);
    }
}

/* every slot's place in the world with the models' nodes as they stand now (world/world_gpu.c
   takes an animated slot's matrix from its node), into out */
static void transforms_now(const struct Map *map, float (*out)[12])
{
    size_t s;

    for (s = 0; s < D.slotCount; s++) {
        const struct GfxWorldSlot *slot = &D.slots[s];
        const float *local = slot->animated >= 0 && map->slotNodes[s] ? map->slotNodes[s]->matrix : slot->local;

        if (slot->parent < 0)
            memcpy(out[s], local, sizeof out[s]);
        else
            mat_multiply(out[s], local, out[slot->parent]);
    }
}

/* The models that animate (an animated slot or a turning texture) with a record the view keeps as
   they stand now, before the frame's ticks: marked (slot 3) with the frame. */
static void mark_moving_in_view(struct Map *map)
{
    static float (*now)[12];
    static size_t capacity;
    size_t i;

    now = grow(now, &capacity, D.slotCount + 1, sizeof *now);
    transforms_now(map, now);
    for (i = 0; i < D.slotCount; i++)
        if (D.slots[i].animated >= 0 && map->slotActors[i])
            *mark(map->slotActors[i], 4) = g_tickFrame;
    for (i = 0; i < map->recordCount; i++) {
        struct C3dActor *a = map->recordActors[i];

        if (a && map->records[i].uv != SW_NONE)
            *mark(a, 4) = g_tickFrame;
    }
    for (i = 0; i < map->recordCount; i++) {
        struct C3dActor *a = map->recordActors[i];

        if (a && *mark(a, 4) == g_tickFrame && sw_record_in_view(&map->records[i], (const float (*)[12])now, &D.frustum))
            *mark(a, 3) = g_tickFrame;
    }
}

static void pose_models(int pose);

static void run_pose(struct Map *map, struct Stats *st, struct View *view, int pose)
{
    static int *todayAt, *keptAt, *castToday, *castKept;
    static size_t capacity;
    static uint32_t *seq[SW_PASS_COUNT];
    static size_t seqCount[SW_PASS_COUNT], seqCapacity;
    static struct ViewFrustum leaves;
    static const struct DeviceDraw **drawOf;
    struct Renderer *r = g_renderer;
    struct SwCascade cascades[GFX_SHADOW_CASCADES];
    size_t passStart[SW_PASS_COUNT + 1], n, i, j;
    int cascadeCount = 0, p, c;

    if (capacity < map->recordCount) {
        capacity = map->recordCount;
        todayAt = realloc(todayAt, capacity * sizeof(int));
        keptAt = realloc(keptAt, capacity * sizeof(int));
        castToday = realloc(castToday, capacity * sizeof(int));
        castKept = realloc(castKept, capacity * sizeof(int));
        drawOf = realloc((void *)drawOf, capacity * sizeof *drawOf);
        if (!todayAt || !keptAt || !castToday || !castKept || !drawOf)
            abort();
    }
    if (seqCapacity < map->recordCount) {
        seqCapacity = map->recordCount;
        for (p = 0; p < SW_PASS_COUNT; p++)
            if (!(seq[p] = realloc(seq[p], seqCapacity * sizeof(uint32_t))))
                abort();
    }
    for (i = 0; i < map->recordCount; i++) {
        todayAt[i] = keptAt[i] = -1;
        castToday[i] = 0;
        drawOf[i] = NULL;
    }
    memset(seqCount, 0, sizeof seqCount);

    /* the client's frame: view_on_render ticks the models it reaches and the ones whose moving part
       the view holds, queues the casters, the water and the alpha-blended records and gives the
       device the frame (gfx_world_frame) */
    pose_models(pose);
    g_tickFrame++;
    g_logTicks = 1;
    r->frame++;
    view_on_render(view);
    g_logTicks = 0;
    if (r->shadowPending) {
        cascadeCount = r->shadow.cascades;
        for (c = 0; c < cascadeCount; c++) {
            memcpy(cascades[c].axis, r->shadowReach.axis, sizeof cascades[c].axis);
            memcpy(cascades[c].lo, r->shadowReach.lo[c], sizeof cascades[c].lo);
            memcpy(cascades[c].hi, r->shadowReach.hi[c], sizeof cascades[c].hi);
            memcpy(cascades[c].depth, r->shadowReach.depth, sizeof cascades[c].depth);
        }
    }
    frame_transforms(map, view->viewMatrix);

    /* the models as they stood before the frame: which animate with a record in view */
    pose_models(pose);
    mark_moving_in_view(map);

    /* the reference's frame, from the same start, ticked as the base ticked */
    reference_frame(map, view);
    {
        struct FrustumScreen fs = {{r->proj[0], r->proj[1]}, {(float)r->centreX, (float)r->centreY},
                                   {(float)r->width, (float)r->height}};
        view_frustum_build(&leaves, &fs, view->viewMatrix, &g_world.root);
    }
    for (i = 0; i < g_drawCount; i++) {
        const struct DeviceDraw *d = &g_draws[i];
        const struct Source *s = mesh_source(map, d->mesh);
        long rec;

        if (!s) {
            FAIL("%s: a reference draw of a mesh that has no records", map->name);
            continue;
        }
        if (d->cascade >= 0) {
            /* casters: a ground block's as one draw of all its runs */
            uint32_t first = s->place.firstIndex + (uint32_t)d->draw.first, end = first + (uint32_t)d->draw.count;

            for (rec = record_at(map, first); rec >= 0 && map->records[rec].firstIndex < end;
                 rec = record_at(map, map->records[rec].firstIndex + map->records[rec].indexCount)) {
                const struct SwRecord *rr = &map->records[rec];

                castToday[rec] = 1;
                if (!s->block) {
                    static const float identity[6] = {1, 0, 0, 1, 0, 0};
                    const float *uv = rr->uv == SW_NONE ? identity : D.uv[rr->uv];
                    struct Texture *layer = rr->textureArray == SW_NO_ARRAY ? NULL
                                                                            : D.layers[rr->textureArray][rr->textureLayer];

                    if (rr->firstIndex != first || rr->indexCount != (uint32_t)d->draw.count
                        || (layer ? layer != d->texture[0] : d->texture[0]->base.surface != NULL)
                        || memcmp(uv, d->draw.uv, sizeof d->draw.uv) || rr->pass == SW_PASS_GROUND)
                        mismatch(map, st, "the caster", rr);
                } else if (d->texture[0] || rr->pass != SW_PASS_GROUND) {
                    mismatch(map, st, "the ground caster", rr);
                }
            }
            continue;
        }
        p = device_pass(d);
        rec = record_at(map, s->place.firstIndex + (uint32_t)d->draw.first);
        if (p < 0 || rec < 0) {
            FAIL("%s: a reference draw outside the passes or the records (phase %d)", map->name, d->phase);
            continue;
        }
        if (map->records[rec].pass != p)
            mismatch(map, st, "the pass", &map->records[rec]);
        if (todayAt[rec] >= 0) {
            FAIL("%s: the reference drew a record twice in a pass", map->name);
            continue;
        }
        todayAt[rec] = (int)seqCount[p];
        seq[p][seqCount[p]++] = (uint32_t)rec;
        drawOf[rec] = d;
    }

    /* the records: the view's cull with the client's frustum, by pass in record order */
    n = 0;
    for (p = 0; p < SW_PASS_COUNT; p++) {
        passStart[p] = n;
        for (i = 0; i < map->recordCount; i++)
            if (map->records[i].pass == p
                && sw_record_in_view(&map->records[i], (const float (*)[12])map->transforms, &D.frustum)) {
                g_out[n] = (uint32_t)i;
                keptAt[i] = (int)n++;
            }
    }
    passStart[SW_PASS_COUNT] = n;
    client_queues(map, st, keptAt, drawOf);
    st->poses++;
    st->kept += n;

    /* the ticks: no record the view keeps of a model that animates belongs to one the frame did not
       tick, and the frame ticked no model beyond those the reference reached but one that animates
       and held a record in view before the frame */
    for (i = 0; i < map->recordCount; i++) {
        struct C3dActor *a = map->recordActors[i];

        if (keptAt[i] < 0 || !a || *mark(a, 4) != g_tickFrame || *mark(a, 2) == g_tickFrame)
            continue;
        if (st->untickedMoving++ < 5)
            FAIL("%s: pose %zu: a model that animates shows a record the frame did not tick", map->name, st->poses - 1);
    }
    for (i = 0; i < g_world.models.count; i++) {
        struct C3dActor *a = g_world.models.data[i];

        if (*mark(a, 2) != g_tickFrame || *mark(a, 0) == g_refFrame)
            continue;
        st->tickedByView++;
        if (*mark(a, 3) != g_tickFrame)
            FAIL("%s: pose %zu: the frame ticked a model the walk does not reach that shows nothing moving",
                 map->name, st->poses - 1);
    }

    for (p = 0; p < SW_PASS_COUNT; p++) {
        uint32_t *today = seq[p];
        size_t todayCount = seqCount[p], a;
        static uint32_t *common[2];
        static size_t commonCapacity;
        size_t commonCount[2] = {0, 0};

        if (commonCapacity < map->recordCount) {
            commonCapacity = map->recordCount;
            common[0] = realloc(common[0], commonCapacity * sizeof(uint32_t));
            common[1] = realloc(common[1], commonCapacity * sizeof(uint32_t));
            if (!common[0] || !common[1])
                abort();
        }
        st->today += todayCount;
        for (j = passStart[p]; j < passStart[p + 1]; j++) {
            uint32_t rec = g_out[j];

            if (todayAt[rec] >= 0) {
                common[0][commonCount[0]++] = rec;
                st->both++;
                compare_draw(map, st, &map->records[rec], drawOf[rec]);
                continue;
            }
            /* kept by the records, not drawn by the reference: is any of it in the view, and why
               did the reference leave it out */
            st->keptOnly++;
            {
                const struct SwRecord *rr = &map->records[rec];
                double area = record_view_area(map, rr, view->viewMatrix);

                if (area > 0) {
                    const struct Source *s = source_of(map, rr->firstIndex);
                    int byBox = 0;

                    st->keptOnlySeen++;
                    st->keptOnlySeenAnimated += (rr->flags & SW_ANIMATED) != 0;
                    st->keptOnlyMostArea = area > st->keptOnlyMostArea ? area : st->keptOnlyMostArea;
                    if (s->block)
                        st->keptOnlySeenGround++;
                    else if (actor_leaf_culled(&leaves, s->actor, &byBox) && byBox)
                        st->keptOnlySeenBox++;
                    else
                        st->keptOnlySeenLeaf++;
                    if (!s->block) {
                        int held = 1, count = 0;

                        leaves_hold(&g_world.root, 0, s->actor, &held, &count);
                        printf("%s: pose %zu: a model the reference leaves out shows %.3g px (animated %d, ticked %d, "
                               "its box %s the view, in crossing leaves only %d, in %d leaves whose heights %s it)\n",
                               map->name, st->poses - 1, area, (rr->flags & SW_ANIMATED) != 0,
                               *mark(s->actor, 2) == g_tickFrame,
                               view_frustum_test_obb(&leaves, &s->actor->obb) == FRUSTUM_OUTSIDE ? "outside" : "in",
                               byBox, count, held ? "hold" : "do not hold");
                    }
                }
            }
        }
        for (a = 0; a < todayCount; a++) {
            uint32_t rec = today[a];

            if (keptAt[rec] >= 0) {
                common[1][commonCount[1]++] = rec;
                continue;
            }
            /* drawn by the reference, left out by the records: nothing of it may be in the view */
            st->todayOnly++;
            if (draw_view_area(drawOf[rec]) > 0) {
                st->todayOnlySeen++;
                FAIL("%s: the records leave out a draw the reference shows (pass %d, area %g)", map->name, p,
                     draw_view_area(drawOf[rec]));
            }
        }
        /* the order of what both draw: the opaque passes' sequences of texture orders (the alpha
           passes' order is client_queues') */
        if (commonCount[0] != commonCount[1])
            abort();
        for (a = 0; a < commonCount[0] && p < SW_PASS_ALPHA_CLAMP; a++) {
            const struct SwRecord *x = &map->records[common[0][a]], *y = &map->records[common[1][a]];

            if (x->sort[0] != y->sort[0] || x->sort[1] != y->sort[1]) {
                st->sequenceDiffers++;
                FAIL("%s: pass %d draws its textures in another order than the reference", map->name, p);
                break;
            }
            if (common[0][a] != common[1][a])
                st->tieOrder++;
        }
    }

    /* the pairs whose order can change pixels: drawn by both, in another order */
    for (i = 0; i < map->pairCount; i++) {
        struct CoplanarPair *q = &map->pairs[i];

        if (todayAt[q->a] >= 0 && todayAt[q->b] >= 0) {
            q->aFirst += todayAt[q->a] < todayAt[q->b];
            q->bFirst += todayAt[q->a] > todayAt[q->b];
        }
        if (todayAt[q->a] < 0 || todayAt[q->b] < 0 || keptAt[q->a] < 0 || keptAt[q->b] < 0)
            continue;
        st->pairsBoth++;
        if ((todayAt[q->a] < todayAt[q->b]) != (keptAt[q->a] < keptAt[q->b])) {
            st->pairsFlipped++;
            st->exactFlipped += q->exact != 0;
        }
    }

    /* the cascades' casters: the reference cast into every cascade; the records' cull of each */
    for (i = 0; i < map->recordCount; i++)
        st->casts += castToday[i] != 0;
    for (c = 0; c < cascadeCount; c++) {
        size_t cast = sw_cull_cascade(map->records, map->recordCount, (const float (*)[12])map->transforms,
                                      &cascades[c], g_cast);

        for (i = 0; i < map->recordCount; i++)
            castKept[i] = 0;
        for (i = 0; i < cast; i++)
            castKept[g_cast[i]] = 1;
        st->castKept += cast;
        for (i = 0; i < map->recordCount; i++) {
            const struct SwRecord *rr = &map->records[i];

            if (castKept[i] && castToday[i]) {
                st->castBoth++;
            } else if (castKept[i]) {
                double area = record_cascade_area(map, rr, &cascades[c]);

                st->castKeptOnly++;
                if (area > 0) {
                    st->castKeptOnlyTouching++;
                    st->castKeptOnlyTouchingAnimated += (rr->flags & SW_ANIMATED) != 0;
                    st->castKeptOnlyTouchingGround += source_of(map, rr->firstIndex)->block != NULL;
                    st->castKeptOnlyMostArea = area > st->castKeptOnlyMostArea ? area : st->castKeptOnlyMostArea;
                }
            } else if (castToday[i]) {
                st->castTodayOnly++;
                if (record_cascade_area(map, rr, &cascades[c]) > 0) {
                    st->castTodayOnlyTouching++;
                    FAIL("%s: cascade %d leaves out a caster that reaches it", map->name, c);
                }
            }
        }
    }

    /* the GPU's passes on the same frame */
    if (g_dev) {
        float (*placed)[12] = gpu_transforms(map, view->viewMatrix);

        gpu_cull(map, placed, keptAt);
        gpu_water(map, view->viewMatrix);
    }
    view_frustum_clear(&leaves);
    renderer_destroy_all_rp_list(r);
}

/* ---------------------------------------------------------------------------------------------
   A map: loaded as world/world_load.c loads it, its static world built by world/world_gpu.c as the
   map's first frame and the streamed models' arrival build it */

static const float g_identity[12] = {1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
/* what the models and the ground are drawn with, as view_camera.c's world_begin gives them */
static const struct GfxWorldDraw g_modelsDraw = {GFX_MESH_MODEL, 1, {1, 1, 1, 1}};
static const struct GfxWorldDraw g_groundDraw = {GFX_MESH_GROUND, 0, {1, 1, 1, 1}};

static float ground_height(const struct C3dGround15 *g, int x, int y)
{
    const struct GndCell *cell;

    x = x < 0 ? 0 : x >= g->width ? g->width - 1 : x;
    y = y < 0 ? 0 : y >= g->height ? g->height - 1 : y;
    cell = GND_CELL(g, x, y);
    return (cell->height[0] + cell->height[1] + cell->height[2] + cell->height[3]) * 0.25f;
}

struct Pose {
    float fx, fz, latitude, longitude, distance;
};

/* across the map (a share of its width and height), the tilt, the turn and the distance */
static const struct Pose g_poses[] = {
    {0.50f, 0.50f, -45, 0, 300},    {0.50f, 0.50f, -45, 90, 600},   {0.30f, 0.60f, -30, 200, 450},
    {0.70f, 0.40f, -60, 45, 800},   {0.50f, 0.50f, -80, 30, 1500},  {0.20f, 0.20f, -20, 135, 400},
    {0.80f, 0.80f, -50, 300, 2500}, {0.50f, 0.50f, -65, 270, 10000}, {0.40f, 0.55f, -25, 10, 350},
    {0.60f, 0.45f, -45, 180, 1200}, {0.10f, 0.90f, -35, 315, 700},  {0.90f, 0.15f, -55, 225, 5000},
};

/* the view matrix as view_build_view_matrix (world/view_camera.c) makes it */
static void pose_view(struct View *v, const struct C3dGround15 *g, const struct Pose *p)
{
    float m[12];
    int cx = (int)(p->fx * (float)g->width), cy = (int)(p->fz * (float)g->height), i;
    Vec3 eye, at, up = {0, 1, 0};

    v->at[0] = ((float)(cx - g->width / 2) + 0.5f) * g->zoom;
    v->at[1] = ground_height(g, cx, cy);
    v->at[2] = ((float)(cy - g->height / 2) + 0.5f) * g->zoom;
    v->latitude = p->latitude;
    v->longitude = p->longitude;
    v->distance = p->distance;
    mat_set_rotate_x(m, p->latitude / 180.0f * 3.141592f);
    mat_rotate_y(m, p->longitude / 180.0f * 3.141592f);
    for (i = 0; i < 3; i++)
        v->from[i] = v->at[i] - p->distance * m[6 + i];
    eye = (Vec3){v->from[0], v->from[1], v->from[2]};
    at = (Vec3){v->at[0], v->at[1], v->at[2]};
    mat_look_at(v->viewMatrix, &eye, &at, &up);
    mat_invert(v->invViewMatrix, v->viewMatrix);
}

/* The models as their animations stand before a pose's frame, and the water's waves: the clock at
   the pose's start, a version 1 model at a frame of the pose's (its last tick at no time the clock
   shows), a version 2 model started a time of the pose's before the clock. The same each time, so
   that the client's frame and the reference's begin alike. */
static void pose_models(int pose)
{
    size_t i;

    g_now = 100000u + (uint32_t)pose * 10000u;
    for (i = 0; i < g_world.models.count; i++) {
        struct C3dActor *a = g_world.models.data[i];

        if (a->animType != 1 && a->animType != 2)
            continue;
        if (a->ops->version == 1) {
            struct C3dActorV1 *v1 = (struct C3dActorV1 *)a;

            v1->frame = v1->frameCount > 0 ? (pose * 7 + 3) % v1->frameCount : 0;
            v1->lastTick = 0;
            a->ops->animate(a, (double)v1->frame);
        } else {
            ((struct C3dActorV2 *)a)->startTick = g_now - 250u * (uint32_t)(pose + 1);
            a->ops->animate(a, 250.0 * (pose + 1));
        }
    }
    for (i = 0; i < g_world.ground->waterCount; i++)
        g_world.ground->waters[i].phase = (float)((pose * 37 + (int)i * 11) % 360 - 180);
}

static int load_map(struct Map *map, const char *name)
{
    char file[96];
    struct C3dWorldRes *rsw;
    struct GndRes *gnd;
    struct C3dGround15 *ground;
    struct WorldLighting lighting;
    float player[3];
    size_t i, nearCount;

    snprintf(file, sizeof file, "%s.rsw", name);
    rsw = res_mgr_get(g_resMgr(), file);
    if (!rsw)
        return 0;
    memset(&g_world, 0, sizeof g_world);
    list_init(&g_world.gameObjects);
    g_world.prebuilt = rsw->tree;
    for (i = 0; i < 3; i++) {
        lighting.direction[i] = rsw->lightDir[i];
        lighting.diffuse[i] = rsw->diffuse[i];
        lighting.ambient[i] = rsw->ambient[i];
    }
    g_renderer->twoPassAlpha = rsw->twoPassAlpha;
    map->twoPassAlpha = rsw->twoPassAlpha;
    renderer_set_light(g_renderer, lighting.direction, lighting.ambient, rsw->shadowOpacity);
    g_world.attr = res_mgr_get(g_resMgr(), rsw->gatFile);
    gnd = res_mgr_get(g_resMgr(), rsw->gndFile);
    if (!g_world.attr || !gnd)
        return 0;
    g_world.attr->zoom = 5;
    ground = ground_new();
    if (!ground || !c3d_ground15_load(ground, gnd, lighting.direction, lighting.diffuse, lighting.ambient))
        return 0;
    ground_set_attribute_map(ground, g_world.attr);
    {
        struct GroundWaterSettings water;
        const struct GroundWaterSettings *scene = NULL;

        if (gnd->major != 1 || gnd->minor < 8) {
            water.level = rsw->waterLevel;
            water.type = rsw->waterType;
            water.waveHeight = rsw->waveHeight;
            water.waveSpeed = rsw->waveSpeed;
            water.wavePitch = rsw->wavePitch;
            water.frameDelay = rsw->waterAnimSpeed;
            scene = &water;
        }
        c3d_ground15_load_water(ground, gnd, scene);
    }
    g_world.ground = ground;
    res_mgr_release(g_resMgr(), &gnd->res);

    /* the models near the player load at once, the rest on the loader thread, farthest last
       (world/world_load.c); the player stands where the first pose looks */
    player[0] = ((float)(ground->width / 2 - ground->width / 2) + 0.5f) * ground->zoom;
    player[1] = 0;
    player[2] = player[0];
    if (!world_models_prepare(&g_world, rsw->modelCount, &lighting))
        return 0;
    for (i = 0; i < rsw->modelCount; i++) {
        const struct RswModel *m = &rsw->models[i];
        float dx = m->pos[0] - player[0], dy = m->pos[1] - player[1], dz = m->pos[2] - player[2];
        float dist = sqrtf(dy * dy + dx * dx + dz * dz);
        struct C3dActor *actor;

        if (dist > WORLD_NEAR_MODEL_DIST) {
            if (!world_models_queue(&g_world, m, dist, i))
                return 0;
            continue;
        }
        actor = world_model_create(m, &lighting);
        if (actor && !world_models_add(&g_world, actor))
            return 0;
    }
    if (!world_build_scene_graph(&g_world))
        return 0;
    c3d_ground15_build_vertex_colours(ground);

    /* the ground's blocks and the near models in the shared buffers first, as the map's first
       frame builds them */
    map->name = name;
    world_models_start(&g_world);
    nearCount = g_world.models.inserted;
    world_gpu_begin(&g_world, g_identity, &g_modelsDraw, &g_groundDraw);
    {
        size_t a, l;

        for (a = 0; a < MAX_ARRAYS; a++)
            for (l = 0, map->firstLayers[a] = 0; l < MAX_LAYERS; l++)
                map->firstLayers[a] += D.layers[a][l] != NULL;
    }
    if (D.placeCount == 0 || D.recordCount == 0) {
        FAIL("%s: the static world was not built", name);
        return 0;
    }

    /* then the far ones as they arrive, appended behind what is placed */
    world_models_wait(&g_world);
    world_models_process(&g_world);
    if (g_world.models.inserted != g_world.models.count)
        FAIL("%s: %zu of %zu models went into the scene graph", name, g_world.models.inserted, g_world.models.count);
    {
        int64_t start = platform_stopwatch_ns();

        world_gpu_begin(&g_world, g_identity, &g_modelsDraw, &g_groundDraw);
        map->streamMs = (double)(platform_stopwatch_ns() - start) / 1e6;
    }
    for (i = 0; i < g_world.models.count; i++)
        wrap_ops(g_world.models.data[i]);
    map->rsw = rsw;
    map->lighting = lighting;
    map->blockMeshes = calloc((size_t)(ground->gpuCols * ground->gpuRows) + 1, sizeof *map->blockMeshes);
    if (!map->blockMeshes)
        abort();
    map_sources(map);
    map_records(map);
    map_slots(map);
    index_meshes(map);
    {
        size_t a, l;

        map->arrayCount = map->textureCount = 0;
        for (a = 0; a < MAX_ARRAYS; a++)
            for (l = 0; l < MAX_LAYERS; l++)
                if (D.layers[a][l]) {
                    map->textureCount++;
                    if (a + 1 > map->arrayCount)
                        map->arrayCount = a + 1;
                }
    }
    printf("%s: %zu near and %zu streamed models\n", name, nearCount, g_world.models.count - nearCount);
    return 1;
}

static void free_map(struct Map *map)
{
    size_t i;
    int b;

    world_gpu_free(&g_world);
    for (b = 0; b < g_world.ground->gpuCols * g_world.ground->gpuRows; b++)
        ref_mesh_free(map->blockMeshes ? map->blockMeshes[b] : NULL);
    free(map->blockMeshes);
    node_meshes_free();
    for (i = 0; i < g_world.models.count; i++)
        world_model_destroy(g_world.models.data[i]);
    free(g_world.models.data);
    world_free_scene_graph(&g_world);
    ground_free(g_world.ground);
    res_mgr_release(g_resMgr(), &g_world.attr->res);
    free(map->sources);
    free(map->byIndex);
    free(map->transforms);
    free(map->views);
    free(map->pairs);
    free(map->slotNodes);
    free(map->slotActors);
    free(map->recordActors);
    memset(&g_world, 0, sizeof g_world);
    memset(g_marks, 0, sizeof g_marks);
}

/* ---------------------------------------------------------------------------------------------
   A skill's model, the world built again, and textures reloaded */

/* the client's frame alone (view_on_render), its ticks logged, some time after the last */
static void client_frame(struct View *view)
{
    g_now += 16;
    g_tickFrame++;
    g_logTicks = 1;
    g_renderer->frame++;
    view_on_render(view);
    g_logTicks = 0;
    renderer_destroy_all_rp_list(g_renderer);
}

/* a view at `at` from `distance` away, as pose_view makes one */
static void view_at(struct View *v, const float *at, float distance)
{
    float m[12];
    Vec3 eye, look, up = {0, 1, 0};
    int i;

    memcpy(v->at, at, sizeof v->at);
    v->latitude = -40;
    v->longitude = 30;
    v->distance = distance;
    mat_set_rotate_x(m, v->latitude / 180.0f * 3.141592f);
    mat_rotate_y(m, v->longitude / 180.0f * 3.141592f);
    for (i = 0; i < 3; i++)
        v->from[i] = v->at[i] - distance * m[6 + i];
    eye = (Vec3){v->from[0], v->from[1], v->from[2]};
    look = (Vec3){v->at[0], v->at[1], v->at[2]};
    mat_look_at(v->viewMatrix, &eye, &look, &up);
    mat_invert(v->invViewMatrix, v->viewMatrix);
}

static void poison_nodes(struct C3dNode *node)
{
    size_t child;
    int k;

    for (k = 0; k < 12; k++)
        node->matrix[k] = NAN;
    for (child = 0; child < node->childCount; child++)
        poison_nodes(node->children[child]);
}

/* a model of the map's whose nodes have keys, made as a skill makes its trap: still (animType 0),
   in the scene graph; NULL where the map has none */
static struct C3dActor *make_trap(struct Map *map, size_t *model)
{
    static float keyed[256][12];
    size_t k, n;

    for (k = *model; k < map->rsw->modelCount; k++) {
        struct C3dActor *a = world_model_create(&map->rsw->models[k], &map->lighting);

        if (!a)
            continue;
        actor_keyed(a, keyed, &n);
        if (!n) {
            world_model_destroy(a);
            continue;
        }
        *model = k;
        /* as skill.c sets its model up: its speed 1, still */
        a->ops->set_speed(a, 1.0f);
        a->animType = 0;
        wrap_ops(a);
        if (!scene_graph_node_insert(&g_world.root, a, 0))
            abort();
        return a;
    }
    return NULL;
}

/* a model of the map's, the k-th of its file, in the scene graph */
static struct C3dActor *make_model(struct Map *map, size_t k)
{
    struct C3dActor *a = world_model_create(&map->rsw->models[k], &map->lighting);

    if (!a)
        abort();
    wrap_ops(a);
    if (!scene_graph_node_insert(&g_world.root, a, 0))
        abort();
    return a;
}

static size_t node_faces(const struct C3dNode *node)
{
    size_t n = node->mesh ? (size_t)node->mesh->faceCount : 0, child;

    for (child = 0; child < node->childCount; child++)
        n += node_faces(node->children[child]);
    return n;
}

static size_t actor_faces(const struct C3dActor *a)
{
    size_t n = 0, i;

    if (a->ops->version == 1)
        return node_faces(((const struct C3dActorV1 *)a)->root);
    for (i = 0; i < ((const struct C3dActorV2 *)a)->nodeCount; i++)
        n += node_faces(((const struct C3dActorV2 *)a)->nodes[i]);
    return n;
}

static void give_up(struct C3dActor *trap)
{
    scene_graph_node_remove(&g_world.root, trap, 0);
    world_gpu_forget(&g_world, trap);
}

static struct SwRecord *copy_records(size_t *count)
{
    struct SwRecord *c = malloc((D.recordCount + 1) * sizeof *c);

    if (!c)
        abort();
    memcpy(c, D.records, D.recordCount * sizeof *c);
    *count = D.recordCount;
    return c;
}

static int same_records(const struct SwRecord *a, size_t n)
{
    return n == D.recordCount && !memcmp(a, D.records, n * sizeof *a);
}

/* A skill's model (effects/skills/skill.c) as the frame's walk first meets it: built still, the
   slots of its nodes with keys animated all the same; set going (effects/skills/skill_msg.c), each
   frame's pose the one its first tick of the frame left; given up, nothing of it read again (its
   nodes are poisoned before the next frame) and its records drawn no more; given up again and
   again until the vertices of models given up pass 2^20, after which the world is built again as
   it was. The world built again after the device made its own again, the same; and from the
   textures' new surfaces after they were reloaded. */
static void check_traps(struct Map *map)
{
    static float now[256][12];
    struct View view;
    struct C3dActor *trap;
    struct SwRecord *saved;
    size_t model = 0, records0, animated0, keyed, i, savedCount, rounds;
    int f, moved = 0, gen;
    double buildMs = 0, sendMs = 0, sortMs, packMs;

    memset(&view, 0, sizeof view);
    view.world = &g_world;
    view.up[1] = 1.0f;
    trap = make_trap(map, &model);
    if (!trap) {
        printf("%s: no model with keys for a trap\n", map->name);
        return;
    }
    view_at(&view, trap->obb.centre, 120.0f);
    records0 = D.recordCount;
    animated0 = D.animatedCount;
    actor_keyed(trap, now, &keyed);
    g_watched = trap;
    client_frame(&view);
    if (D.recordCount <= records0 || D.animatedCount != animated0 + keyed)
        FAIL("%s: a trap built still has %zu records and %zu animated slots, its nodes with keys %zu", map->name,
             D.recordCount - records0, D.animatedCount - animated0, keyed);
    else if (memcmp(D.animated + animated0, now, keyed * sizeof now[0]))
        FAIL("%s: a still trap's animated slots do not hold its nodes' matrices", map->name);
    trap->animType = 1;
    for (f = 0; f < 6; f++) {
        g_now += 50;
        memcpy(now, D.animated + animated0, keyed * sizeof now[0]);
        client_frame(&view);
        if (*mark(trap, 2) != g_tickFrame || g_watchedFrame != g_tickFrame) {
            FAIL("%s: a trap in view not ticked", map->name);
            break;
        }
        if (memcmp(D.animated + animated0, g_watchedPose, keyed * sizeof now[0]))
            FAIL("%s: frame %d: a trap set going is not drawn in the pose its first tick left", map->name, f);
        moved += memcmp(D.animated + animated0, now, keyed * sizeof now[0]) != 0;
    }
    if (!moved)
        FAIL("%s: a trap set going never moved", map->name);
    /* out of the scene graph but not given up, as a part that moves out of its model's box: the
       walk reaches it no more, and its records in view tick it all the same */
    scene_graph_node_remove(&g_world.root, trap, 0);
    for (f = 0; f < 3; f++) {
        client_frame(&view);
        if (*mark(trap, 2) != g_tickFrame || g_watchedFrame != g_tickFrame)
            FAIL("%s: a model in view that the walk does not reach is not ticked", map->name);
        else if (memcmp(D.animated + animated0, g_watchedPose, keyed * sizeof now[0]))
            FAIL("%s: a model ticked for its records in view is not drawn in the pose of its tick", map->name);
    }
    if (!scene_graph_node_insert(&g_world.root, trap, 0))
        abort();
    g_watched = NULL;

    /* given up: its nodes poisoned, the frame after reads none of them, and its records go */
    give_up(trap);
    if (trap->ops->version == 1) {
        poison_nodes(((struct C3dActorV1 *)trap)->root);
    } else {
        struct C3dActorV2 *v2 = (struct C3dActorV2 *)trap;

        for (i = 0; i < v2->nodeCount; i++)
            poison_nodes(v2->nodes[i]);
    }
    client_frame(&view);
    world_model_destroy(trap);
    if (D.recordCount != records0)
        FAIL("%s: a trap given up leaves %zu records", map->name, D.recordCount - records0);
    for (i = 0; i < D.animatedCount * 12; i++)
        if (isnan(D.animated[i / 12][i % 12])) {
            FAIL("%s: the frame read the nodes of a trap given up", map->name);
            break;
        }

    /* given up again and again: the world built again without them, as it was */
    saved = copy_records(&savedCount);
    {
        static struct SwRecord *all;
        static size_t capacity;
        int64_t t0 = platform_stopwatch_ns();

        all = grow(all, &capacity, D.recordCount + 1, sizeof *all);
        memcpy(all, D.records, D.recordCount * sizeof *all);
        sw_records_sort(all, D.recordCount);
        sortMs = (double)(platform_stopwatch_ns() - t0) / 1e6;
        t0 = platform_stopwatch_ns();
        {
            static struct WlRecord *byId;
            static struct WlCullRecord *cull;
            static size_t idCapacity, cullCapacity;

            byId = grow(byId, &idCapacity, D.idCount + 1, sizeof *byId);
            cull = grow(cull, &cullCapacity, D.recordCount + 1, sizeof *cull);
            wl_records(D.records, D.recordCount, byId, cull);
        }
        packMs = (double)(platform_stopwatch_ns() - t0) / 1e6;
    }
    /* the map's model with the most faces, given up again and again */
    {
        size_t most = 0, k;

        for (k = 0; k < map->rsw->modelCount; k++) {
            struct C3dActor *a = world_model_create(&map->rsw->models[k], &map->lighting);

            if (!a)
                continue;
            if (actor_faces(a) > most) {
                most = actor_faces(a);
                model = k;
            }
            world_model_destroy(a);
        }
    }
    gen = D.generation;
    for (rounds = 0; rounds < 20000 && D.generation == gen; rounds++) {
        int64_t t0;

        trap = make_model(map, model);
        t0 = platform_stopwatch_ns();
        world_gpu_begin(&g_world, view.viewMatrix, &g_modelsDraw, &g_groundDraw);
        /* the second sends the first one's records given up, and builds nothing */
        if (rounds == 1)
            sendMs = (double)(platform_stopwatch_ns() - t0) / 1e6;
        t0 = platform_stopwatch_ns();
        world_gpu_seen(&g_world, trap);
        world_gpu_frame(&g_world, NULL);
        if (!rounds)
            buildMs = (double)(platform_stopwatch_ns() - t0) / 1e6;
        renderer_destroy_all_rp_list(g_renderer);
        give_up(trap);
        world_model_destroy(trap);
        g_renderer->frame++;
    }
    client_frame(&view);
    if (D.generation == gen)
        FAIL("%s: %zu traps given up, and the world was not built again", map->name, rounds);
    else if (!same_records(saved, savedCount))
        FAIL("%s: the world built again after %zu traps were given up is not as it was", map->name, rounds);
    printf("%s: a model appears: built and sent with its frame in %.3f ms; one given up: the records sent in %.3f ms "
           "(all %zu sorted afresh: %.3f ms; packed for the device: %.3f ms); the world built again after %zu models "
           "given up\n",
           map->name, buildMs, sendMs, savedCount, sortMs, packMs, rounds);

    /* the device made its world again: built again as it was */
    gfx_world_clear();
    client_frame(&view);
    if (!same_records(saved, savedCount))
        FAIL("%s: the world built again for a new device is not as it was", map->name);

    /* the textures reloaded at half their size (Texture Detail): the world built again from them */
    {
        static struct gfx_surface *old[CACHE_SLOTS];
        static int oldSize[CACHE_SLOTS][2];
        size_t a, l, halved = 0;

        for (i = 0; i < CACHE_SLOTS; i++) {
            struct Texture *t = g_cached[i];
            struct gfx_surface *s;

            if (!t)
                continue;
            s = calloc(1, sizeof *s);
            if (!s)
                abort();
            s->texture = t;
            old[i] = t->base.surface;
            oldSize[i][0] = t->base.width;
            oldSize[i][1] = t->base.height;
            t->base.surface = s;
            t->base.width = t->base.width > 1 ? t->base.width / 2 : 1;
            t->base.height = t->base.height > 1 ? t->base.height / 2 : 1;
        }
        gfx_world_clear();
        client_frame(&view);
        for (a = 0; a < (size_t)D.arrayCount; a++)
            for (l = 0; l < MAX_LAYERS && D.layers[a][l]; l++) {
                if (D.layers[a][l]->base.width != D.arraySize[a][0])
                    FAIL("%s: a reloaded texture in an array of its old size", map->name);
                halved += D.layers[a][l]->name[0] != 0;
            }
        if (!halved || D.recordCount != savedCount)
            FAIL("%s: the world built from reloaded textures has %zu records (%zu before), %zu textures of the cache",
                 map->name, D.recordCount, savedCount, halved);
        for (i = 0; i < CACHE_SLOTS; i++) {
            struct Texture *t = g_cached[i];

            if (!t)
                continue;
            free(t->base.surface);
            t->base.surface = old[i];
            t->base.width = oldSize[i][0];
            t->base.height = oldSize[i][1];
        }
        gfx_world_clear();
        client_frame(&view);
    }
    free(saved);
    /* the map as the device has it now, for the report */
    map_sources(map);
    map_records(map);
    map_slots(map);
    index_meshes(map);
}

static void report_map(const struct Map *map, const struct Stats *st)
{
    size_t pass[SW_PASS_COUNT] = {0}, animated = 0, uv = 0, runs = 0, i;
    static const char *const names[SW_PASS_COUNT] = {"opaque clamp", "opaque wrap", "ground", "alpha clamp",
                                                     "alpha wrap", "alpha depth"};
    static struct SwDrawRun list[1u << 18];

    for (i = 0; i < map->recordCount; i++) {
        pass[map->records[i].pass]++;
        animated += (map->records[i].flags & SW_ANIMATED) != 0;
        uv += map->records[i].uv != SW_NONE;
    }
    if (map->recordCount <= sizeof list / sizeof list[0]) {
        size_t n = sw_draw_runs(map->records, map->recordCount, list);

        for (i = 0; i < n; i++)
            runs += list[i].pass <= SW_PASS_GROUND;
    }
    printf("%s: %zu records (", map->name, map->recordCount);
    for (i = 0; i < SW_PASS_COUNT; i++)
        printf("%s%s %zu", i ? ", " : "", names[i], pass[i]);
    printf("), %zu animated, %zu with turning texture coordinates; %zu meshes, %u vertices, %u indices, %zu slots, "
           "%zu indirect calls; %zu models outside the scene graph\n",
           animated, uv, map->sourceCount, D.vertexCount, D.indexCount, D.slotCount, runs, map->outsideGraph);
    printf("%s: %zu textures in %zu arrays; %zu cells of water\n", map->name, map->textureCount, map->arrayCount,
           D.cellCount);
    {
        /* the arrays' memory with their mipmaps: as the device makes them (as many layers as the
           first frame places, then the power of two at or above the layers where more come), and
           from four layers doubled */
        double made = 0, before = 0;
        size_t a, l;

        for (a = 0; a < (size_t)D.arrayCount; a++) {
            uint32_t layers = 0, now, old = 4;
            double bytes = (double)D.arraySize[a][0] * D.arraySize[a][1] * 4.0 * 4.0 / 3.0;

            for (l = 0; l < MAX_LAYERS; l++)
                layers += D.layers[a][l] != NULL;
            now = map->firstLayers[a] ? map->firstLayers[a] : layers;
            if (layers > now)
                for (now = 1; now < layers; now <<= 1)
                    ;
            while (old < layers)
                old *= 2;
            now = now > GFX_WORLD_LAYERS ? GFX_WORLD_LAYERS : now;
            old = old > GFX_WORLD_LAYERS ? GFX_WORLD_LAYERS : old;
            made += bytes * now;
            before += bytes * old;
        }
        printf("%s: the texture arrays with mipmaps: %.1f MB as made, %.1f MB from 4 layers by doubling\n", map->name,
               made / 1048576.0, before / 1048576.0);
    }
    printf("%s: %zu poses: the reference %zu draws, records %zu, both %zu; reference only %zu (in view: %zu); records "
           "only %zu (in view: %zu - ground %zu, leaf %zu, box %zu, animated %zu; most %.3g px)\n",
           map->name, st->poses, st->today, st->kept, st->both, st->todayOnly, st->todayOnlySeen, st->keptOnly,
           st->keptOnlySeen, st->keptOnlySeenGround, st->keptOnlySeenLeaf, st->keptOnlySeenBox,
           st->keptOnlySeenAnimated, st->keptOnlyMostArea);
    printf("%s: casters the reference cast %zu over the poses, each into every cascade, the client the same %zu; by "
           "cascade: records %zu, both %zu, reference only %zu (reaching: %zu), records only %zu (reaching: %zu, "
           "ground %zu, animated %zu; most %.3g units)\n",
           map->name, st->casts, st->clientCasts, st->castKept, st->castBoth, st->castTodayOnly,
           st->castTodayOnlyTouching, st->castKeptOnly, st->castKeptOnlyTouching, st->castKeptOnlyTouchingGround,
           st->castKeptOnlyTouchingAnimated, st->castKeptOnlyMostArea);
    {
        size_t seen = 0, both = 0;

        for (i = 0; i < map->pairCount; i++) {
            seen += map->pairs[i].aFirst + map->pairs[i].bFirst > 1;
            both += map->pairs[i].aFirst && map->pairs[i].bFirst;
        }
        printf("%s: pairs in one plane drawn by both %zu, in another order %zu (%zu within %g units); of the pairs "
               "the reference drew in more than one pose, %zu of %zu in both orders\n",
               map->name, st->pairsBoth, st->pairsFlipped, st->exactFlipped, EXACT, both, seen);
    }
    printf("%s: order: texture sequences that differ %zu, ties drawn in another order %zu; alpha-blended records "
           "drawn in the reference's order %zu; views not the reference's %zu; mismatches %zu\n",
           map->name, st->sequenceDiffers, st->tieOrder, st->alphaOrdered, st->viewsDiffer, st->mismatches);
    printf("%s: the client queued %zu alpha records (%zu the reference drew, keyed as it keyed them; %zu of models "
           "it did not draw) and %zu cells of water, of which the reference drew %zu\n",
           map->name, st->clientAlpha, st->clientAlphaKeyed, st->clientAlphaOnly, st->clientWater, st->referenceWater);
    printf("%s: ticks: models ticked for a moving part in view the walk does not reach %zu; records of models that "
           "animate shown unticked %zu; the streamed models built and sent in %.2f ms\n",
           map->name, st->tickedByView, st->untickedMoving, map->streamMs);
}

static int g_trapsChecked;

static void check_map(const char *name)
{
    static struct Map map;
    struct Stats st;
    struct View view;
    size_t i;

    memset(&map, 0, sizeof map);
    memset(&st, 0, sizeof st);
    if (!load_map(&map, name)) {
        FAIL("%s: does not load", name);
        return;
    }
    g_cullTimeDue = 1;
    {
        struct OrderStats os;

        /* the pairs from the records' boxes as they stand at the first pose */
        pose_models(0);
        memset(&view, 0, sizeof view);
        frame_transforms(&map, g_identity);
        order_pairs(&map, &os);
        printf("%s: opaque draws sharing a texture: %zu pairs with meeting boxes, %zu with triangles in one plane that "
               "overlap (%zu within %g units; %zu triangle pairs), %zu of those alike in every corner\n",
               name, os.pairs, os.coplanarPairs, os.exactPairs, EXACT, os.coplanar, os.identical);
        printf("%s: opaque draws with two textures: %zu pairs with meeting boxes, %zu in one plane (%zu within %g "
               "units; %zu triangle pairs), %zu that drawing one texture array after the other would turn round\n",
               name, os.crossPairs, os.crossCoplanar, os.crossExact, EXACT, os.crossTriangles, os.crossTurned);
    }
    memset(&view, 0, sizeof view);
    view.world = &g_world;
    view.up[1] = 1.0f;
    for (i = 0; i < sizeof g_poses / sizeof g_poses[0]; i++) {
        pose_view(&view, g_world.ground, &g_poses[i]);
        run_pose(&map, &st, &view, (int)i);
    }
    /* the traps on the first map */
    if (!g_trapsChecked) {
        g_trapsChecked = 1;
        check_traps(&map);
    }
    report_map(&map, &st);
    free_map(&map);
    res_mgr_purge_unlocked(g_resMgr());
}

#define MAX_TEXTURES 8192

/* ---------------------------------------------------------------------------------------------
   The survey: the sizes of every map's textures and lightmap pages */

struct SizeCount {
    uint16_t w, h;
    size_t textures, maps, mostInMap;
};

static struct SizeCount g_sizes[256];
static size_t g_sizeCount;

static struct SizeCount *size_entry(uint16_t w, uint16_t h)
{
    size_t i;

    for (i = 0; i < g_sizeCount; i++)
        if (g_sizes[i].w == w && g_sizes[i].h == h)
            return &g_sizes[i];
    if (g_sizeCount == 256)
        abort();
    memset(&g_sizes[g_sizeCount], 0, sizeof g_sizes[0]);
    g_sizes[g_sizeCount].w = w;
    g_sizes[g_sizeCount].h = h;
    return &g_sizes[g_sizeCount++];
}

static int add_unique(struct Texture **set, size_t *count, size_t capacity, struct Texture *t)
{
    size_t i;

    for (i = 0; i < *count; i++)
        if (set[i] == t)
            return 0;
    if (*count == capacity)
        abort();
    set[(*count)++] = t;
    return 1;
}

static int by_name(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

static int survey(void)
{
    static struct Texture *seen[CACHE_SLOTS], *mapSet[MAX_TEXTURES];
    static char *names[8192];
    size_t nameCount = 0, seenCount = 0, i, k, maps = 0, lightmapPages = 0, mostArrays = 0, mostLayers = 0;
    size_t arraysHistogram[64] = {0}, over256 = 0;
    double mostBytes = 0, totalBytes = 0;
    const char *mostArraysMap = "", *mostLayersMap = "", *mostBytesMap = "";
    struct GrfArchive *grf = g_fileMgr.archives[0];
    struct PlatformFind *find;

    for (i = 0; i < grf->entry_count; i++) {
        const char *n = grf->entries[i].key.name;
        size_t len = strlen(n);

        if (len > 9 && !strncmp(n, "data\\", 5) && !strcmp(n + len - 4, ".rsw") && !strchr(n + 5, '\\')) {
            if (nameCount == 8192)
                abort();
            names[nameCount++] = _strdup(n + 5);
        }
    }
    for (find = platform_find_first("data\\*.rsw"); find;) {
        char lower[256];

        snprintf(lower, sizeof lower, "%s", platform_find_name(find));
        for (k = 0; lower[k]; k++)
            lower[k] = (char)(lower[k] >= 'A' && lower[k] <= 'Z' ? lower[k] + 32 : lower[k]);
        for (k = 0; k < nameCount && strcmp(names[k], lower); k++) {
        }
        if (k == nameCount && nameCount < 8192)
            names[nameCount++] = _strdup(lower);
        if (!platform_find_next(find)) {
            platform_find_close(find);
            break;
        }
    }
    qsort(names, nameCount, sizeof names[0], by_name);
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("survey: %zu maps\n", nameCount);
    for (i = 0; i < nameCount; i++) {
        struct C3dWorldRes *rsw = res_mgr_get(g_resMgr(), names[i]);
        struct GndRes *gnd;
        struct C3dGround15 *ground;
        size_t mapCount = 0, m, s;
        uint16_t sizesW[64], sizesH[64];
        size_t perSize[64], distinct = 0, layers = 0, pages;
        double bytes = 0;
        int animated = 0, models = 0, version2 = 0, uvNodes = 0, alphaUses = 0;

        fprintf(stderr, "survey: %s\n", names[i]);
        if (!rsw)
            continue;
        gnd = res_mgr_get(g_resMgr(), rsw->gndFile);
        ground = gnd ? ground_new() : NULL;
        if (!ground || !c3d_ground15_load(ground, gnd, rsw->lightDir, rsw->diffuse, rsw->ambient)) {
            printf("survey: %s: no ground\n", names[i]);
            if (ground)
                ground_free(ground);
            if (gnd)
                res_mgr_release(g_resMgr(), &gnd->res);
            res_mgr_release(g_resMgr(), &rsw->res);
            continue;
        }
        maps++;
        for (s = 0; s < ground->surfaceCount; s++)
            if (ground->surfaces[s].texture)
                add_unique(mapSet, &mapCount, MAX_TEXTURES, ground->surfaces[s].texture);
        pages = ground->lightmaps.pageCount;
        for (s = 0; s < pages; s++)
            add_unique(mapSet, &mapCount, MAX_TEXTURES, ground->lightmaps.pages[s]);
        lightmapPages += pages;
        for (m = 0; m < rsw->modelCount; m++) {
            struct C3dModelRes *res = res_mgr_get(g_resMgr(), rsw->models[m].fileName);
            size_t n2;

            if (!res)
                continue;
            models++;
            version2 += res->version == 2;
            for (n2 = 0; n2 < res->nodeCount; n2++) {
                const struct ModelResNode *node = res->nodes[n2];

                for (k = 0; k < node->textureCount; k++) {
                    const struct Texture *t = node->textures[k] ? node->textures[k] : &g_whiteTexture;

                    add_unique(mapSet, &mapCount, MAX_TEXTURES, (struct Texture *)t);
                    alphaUses += t->format == 1;
                }
                uvNodes += res->version == 2 && ((const struct ModelResNodeV2 *)node)->uvKeys.count > 0;
                if (rsw->models[m].animType == 1 || rsw->models[m].animType == 2) {
                    if (res->version == 1 ? ((const struct ModelResNodeV1 *)node)->rotKeyCount > 1
                                          : ((const struct ModelResNodeV2 *)node)->rotKeyCount > 1
                                                || ((const struct ModelResNodeV2 *)node)->posKeyCount > 1
                                                || ((const struct ModelResNodeV2 *)node)->scaleKeyCount > 1)
                        animated++;
                }
            }
            res_mgr_release(g_resMgr(), &res->res);
        }
        /* the map's arrays, one a size */
        for (k = 0; k < mapCount; k++) {
            struct Texture *t = mapSet[k];
            size_t d;
            int isPage = 0;

            for (s = 0; s < pages; s++)
                isPage |= ground->lightmaps.pages[s] == t;
            for (d = 0; d < distinct && (sizesW[d] != t->base.width || sizesH[d] != t->base.height); d++) {
            }
            if (d == distinct) {
                if (distinct == 64)
                    abort();
                sizesW[d] = (uint16_t)t->base.width;
                sizesH[d] = (uint16_t)t->base.height;
                perSize[d] = 0;
                distinct++;
            }
            perSize[d]++;
            bytes += (double)t->base.width * t->base.height * 4.0 * 4.0 / 3.0;
            if (!isPage && add_unique(seen, &seenCount, CACHE_SLOTS, t)) {
                struct SizeCount *e = size_entry((uint16_t)t->base.width, (uint16_t)t->base.height);

                e->textures++;
            }
        }
        for (k = 0; k < distinct; k++) {
            struct SizeCount *e = size_entry(sizesW[k], sizesH[k]);

            e->maps++;
            e->mostInMap = perSize[k] > e->mostInMap ? perSize[k] : e->mostInMap;
            layers = perSize[k] > layers ? perSize[k] : layers;
            over256 += perSize[k] > 256;
        }
        arraysHistogram[distinct < 63 ? distinct : 63]++;
        if (distinct > mostArrays) {
            mostArrays = distinct;
            mostArraysMap = names[i];
        }
        if (layers > mostLayers) {
            mostLayers = layers;
            mostLayersMap = names[i];
        }
        if (bytes > mostBytes) {
            mostBytes = bytes;
            mostBytesMap = names[i];
        }
        totalBytes += bytes;
        printf("survey: %s: %zu textures (%zu lightmap pages) in %zu sizes, %.1f MB; %d models (%d RSM2), %d animated "
               "nodes, %d with turning texture coordinates, %d uses of alpha textures; two-pass alpha %d\n",
               names[i], mapCount, pages, distinct, bytes / 1048576.0, models, version2, animated, uvNodes,
               alphaUses, rsw->twoPassAlpha);
        ground_free(ground);
        res_mgr_release(g_resMgr(), &gnd->res);
        res_mgr_release(g_resMgr(), &rsw->res);
        if (i % 16 == 15)
            res_mgr_purge_unlocked(g_resMgr());
    }
    printf("survey: %zu maps, %zu distinct textures, %zu lightmap pages (256x256)\n", maps, seenCount, lightmapPages);
    printf("survey: sizes (distinct textures over all maps; maps using the size; most in one map, lightmap pages "
           "counted in 256x256):\n");
    for (i = 0; i < g_sizeCount; i++)
        printf("survey:   %5ux%-5u %6zu textures  %4zu maps  most %zu\n", g_sizes[i].w, g_sizes[i].h,
               g_sizes[i].textures, g_sizes[i].maps, g_sizes[i].mostInMap);
    printf("survey: arrays a map needs with one a size:");
    for (i = 0; i < 64; i++)
        if (arraysHistogram[i])
            printf(" %zu:%zu", i, arraysHistogram[i]);
    printf("\nsurvey: most arrays %zu (%s); most layers in one %zu (%s); sizes over 256 layers %zu; "
           "most memory %.1f MB (%s); average %.1f MB\n",
           mostArrays, mostArraysMap, mostLayers, mostLayersMap, over256, mostBytes / 1048576.0, mostBytesMap,
           maps ? totalBytes / maps / 1048576.0 : 0.0);
    for (i = 0; i < nameCount; i++)
        free(names[i]);
    return 0;
}

int main(int argc, char **argv)
{
    /* a city, a field, a room, many animated models, alpha-blended models, RSM2 models with turning
       texture coordinates and two-pass alpha, the largest textures, animated parts that move out of
       their models' boxes, and water */
    static const char *const defaults[] = {"prontera", "prt_fild08", "iz_int03", "comodo",   "dicastes01",
                                           "1@adv",    "rag_fes",    "amatsu",   "alberta", "izlude"};
    static struct Renderer renderer;
    int i;

    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 2 || _chdir(argv[1]) != 0 || !game_file_manager_add_archive(&g_fileMgr, "data.grf")) {
        printf("static_world_maps: skipped, no data.grf in %s\n", argc > 1 ? argv[1] : "(no game folder given)");
        return 0;
    }
    resource_errors_init();
    res_mgr_read_res_name_table(g_resMgr());
    math_init_degree_tables();
    g_whiteTexture = (struct Texture){.base = {.width = 1, .height = 1}, .reqWidth = 1, .reqHeight = 1};
    g_lightmap = 1;
    g_weather.color[0] = g_weather.color[1] = g_weather.color[2] = 1.0f;
    g_weather.ground_lighting = 1;
    if (!renderer_construct(&renderer))
        abort();
    g_renderer = &renderer;
    /* the frames tick the models (game/mode_run.c: a frame with a step of the game's logic) */
    g_isSkipFrame = 1;
    renderer_set_projection(&renderer, 1024, 768);
    if (argc > 2 && !strcmp(argv[2], "--survey"))
        return survey();
    if (open_gpu()) {
        if (bare_operations("world_transform.hlsl", world_transform_cs.code.spirv, world_transform_cs.code.spirvSize)
            || bare_operations("world_cull.hlsl", world_cull_cs.code.spirv, world_cull_cs.code.spirvSize)
            || bare_operations("water.hlsli (water_probe.hlsl)", water_probe_cs.code.spirv, water_probe_cs.code.spirvSize))
            FAIL("a pass that must match the C has operations that may part from it");
    } else {
        printf("static_world_maps: the GPU's passes skipped, no GPU device (%s)\n", SDL_GetError());
        g_dev = NULL;
    }
    if (argc > 2) {
        for (i = 2; i < argc; i++)
            check_map(argv[i]);
    } else {
        for (i = 0; i < (int)(sizeof defaults / sizeof defaults[0]); i++)
            check_map(defaults[i]);
        /* a window of odd size, whose centre lies half a pixel off the middle: the records and
           world/frustum.c both reach its right and bottom edges */
        renderer_set_projection(&renderer, 1025, 769);
        printf("a screen of 1025 by 769 pixels:\n");
        check_map("prontera");
    }
    if (g_dev)
        printf("static_world_maps: on the GPU, slots %zu (not the C's %zu), draws %zu (clip space not the reference's "
               "%zu), runs culled %zu of %zu records, %zu kept (runs not as sw_record_in_view %zu), cells of water %zu "
               "(drawn %zu; not the reference's %zu)\n",
               g_gpuStats.slots, g_gpuStats.slotsDiffer, g_gpuStats.draws, g_gpuStats.drawsDiffer, g_gpuStats.runs,
               g_gpuStats.records, g_gpuStats.kept, g_gpuStats.runsDiffer, g_gpuStats.cells, g_gpuStats.cellsDrawn,
               g_gpuStats.cellsDiffer);
    close_gpu();
    if (g_failures)
        printf("static_world_maps: %d failures\n", g_failures);
    else
        printf("static_world_maps: ok\n");
    return g_failures != 0;
}
