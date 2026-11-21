/* The static world's layer (render/gpu_world.c) of the graphics device (render/gpu_device.c) on a
   real GPU, in a hidden window, with the draws of its shadow pass watched through the SDL calls
   that record them: in a frame with shadows, each draw into each cascade is
   made with the pipeline its records need, the one that tests their texels' alpha for a run of the
   models' textured records and the one that does not for the ground and the records without a
   texture, and an alpha-tested draw reads its own run's texture array. The world holds, in one texture array, a model's record of an opaque pass, the
   ground and a model's record of an alpha pass, so that the shadow pass's runs of that array take
   the two pipelines by turns; then a model's record of a second array, whose run follows the
   first array's last with the same pipeline and another texture; and a model's record without a
   texture. Skipped (77) where SDL finds no GPU device or the device draws no shadows. */
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* a draw of a pass the test watches: the pass's first colour target, the pipeline bound, the
   texture of the fragment shader's first sampler bound in the pass (NULL: none), the command's
   place in the indirect buffer */
struct watched_draw {
    SDL_GPUTexture *target, *texture;
    SDL_GPUGraphicsPipeline *pipeline;
    Uint32 offset;
};

#define DRAWS_MAX 256
static struct watched_draw draws[DRAWS_MAX];
static int ndraws, overflow;
/* what the render thread's current pass has bound, which only that thread touches */
static SDL_GPUTexture *passTarget, *passTexture;
static SDL_GPUGraphicsPipeline *passPipeline;

static int watched(SDL_GPUTexture *target);

static SDL_GPURenderPass *f_BeginGPURenderPass(SDL_GPUCommandBuffer *cmd, const SDL_GPUColorTargetInfo *colours,
                                              Uint32 n, const SDL_GPUDepthStencilTargetInfo *depth)
{
    passTarget = n ? colours[0].texture : NULL;
    passTexture = NULL;
    passPipeline = NULL;
    return SDL_BeginGPURenderPass(cmd, colours, n, depth);
}

static void f_BindGPUGraphicsPipeline(SDL_GPURenderPass *pass, SDL_GPUGraphicsPipeline *pipeline)
{
    passPipeline = pipeline;
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
}

static void f_BindGPUFragmentSamplers(SDL_GPURenderPass *pass, Uint32 first, const SDL_GPUTextureSamplerBinding *b,
                                      Uint32 n)
{
    if (first == 0 && n)
        passTexture = b[0].texture;
    SDL_BindGPUFragmentSamplers(pass, first, b, n);
}

static void f_DrawGPUIndexedPrimitivesIndirect(SDL_GPURenderPass *pass, SDL_GPUBuffer *buffer, Uint32 offset,
                                               Uint32 count)
{
    if (watched(passTarget)) {
        if (ndraws < DRAWS_MAX) {
            draws[ndraws].target = passTarget;
            draws[ndraws].texture = passTexture;
            draws[ndraws].pipeline = passPipeline;
            draws[ndraws].offset = offset;
            ndraws++;
        } else {
            overflow = 1;
        }
    }
    SDL_DrawGPUIndexedPrimitivesIndirect(pass, buffer, offset, count);
}

#define SDL_BeginGPURenderPass f_BeginGPURenderPass
#define SDL_BindGPUGraphicsPipeline f_BindGPUGraphicsPipeline
#define SDL_BindGPUFragmentSamplers f_BindGPUFragmentSamplers
#define SDL_DrawGPUIndexedPrimitivesIndirect f_DrawGPUIndexedPrimitivesIndirect
#include "../src/render/gpu_device.c"
#include "../src/render/gpu_world.c"

struct perf_counters g_perf;
/* What the test keeps of the device it draws with: the hidden window, its size and the screen
   (gfx_create_screen). */
struct TestScreen {
    SDL_Window *window;
    int width, height;
    gfx_surface *screen;
};
static struct TestScreen g_screen;

#define SCREEN_W 64
#define SCREEN_H 48
#define RECORDS 5

/* the shadow pass's draws */
static int watched(SDL_GPUTexture *target)
{
    return target && target == W.shadowMaps;
}

static const WORD quad[6] = { 0, 1, 2, 2, 1, 3 };
static struct SwRecord records[RECORDS];  /* by id, as built */
static struct SwRecord sorted[RECORDS];   /* in draw order, as the device has them */
static struct SwLayout layout;

/* a 2 by 2 texture whose left texels are opaque and right ones `right` */
static gfx_surface *texture_2x2(DWORD right)
{
    gfx_surface *t = gfx_create_texture(2, 2, GFX_FMT_8888);
    struct gfx_lock lk;
    int i;
    assert(t && gfx_lock(t, NULL, &lk) == 0);
    for (i = 0; i < 4; i++) {
        DWORD c = i & 1 ? right : 0xffffffffu;
        memcpy((BYTE *)lk.bits + (i / 2) * lk.pitch + (i % 2) * 4, &c, 4);
    }
    gfx_unlock(t);
    return t;
}

static gfx_surface *texture_4x4(DWORD argb)
{
    gfx_surface *t = gfx_create_texture(4, 4, GFX_FMT_8888);
    struct gfx_lock lk;
    int x, y;
    assert(t && gfx_lock(t, NULL, &lk) == 0);
    for (y = 0; y < 4; y++)
        for (x = 0; x < 4; x++)
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &argb, 4);
    gfx_unlock(t);
    return t;
}

/* record `id`: a quad from x0 to x1 and y0 to y1 at depth z in the given pass, texture array and
   layer 0 */
static void add(uint32_t id, float x0, float y0, float x1, float y1, float z, uint8_t pass, uint8_t array)
{
    struct GfxMeshVertex mv[4];
    struct SwPlacement place;
    struct SwMesh mesh;
    struct SwRun run;
    uint32_t ids[4];
    int i;
    memset(mv, 0, sizeof mv);
    for (i = 0; i < 4; i++) {
        mv[i].x = i & 1 ? x1 : x0;
        mv[i].y = i & 2 ? y1 : y0;
        mv[i].z = z;
        mv[i].diffuse = 0xffffffff;
        mv[i].u0 = (float)(i & 1);
        mv[i].v0 = (float)(i >> 1);
    }
    memset(&run, 0, sizeof run);
    run.count = 6;
    run.uv = SW_NONE;
    run.textureArray = array;
    run.lightmapArray = SW_NO_ARRAY;
    run.pass = pass;
    run.flags = SW_CULL_NONE;
    memset(&mesh, 0, sizeof mesh);
    mesh.positions = &mv[0].x;
    mesh.stride = sizeof *mv / sizeof(float);
    mesh.vertexCount = 4;
    mesh.indices = quad;
    mesh.indexCount = 6;
    assert(sw_layout_place(&layout, 4, 6, &place));
    assert(sw_records_build(&mesh, &place, &run, 1, id, &records[id]));
    assert(sw_vertex_ids(&mesh, &run, 1, id, ids));
    assert(gfx_world_mesh(&place, mv, ids, quad));
}

/* The world: in array 0 (2 by 2) a model of an opaque pass, the ground and a model of an alpha
   pass; in array 1 (4 by 4) a model of an opaque pass; and a model without a texture. */
static void world(gfx_surface *half, gfx_surface *other)
{
    struct GfxWorldSlot slot;
    gfx_world_clear();
    memset(&layout, 0, sizeof layout);
    assert(gfx_world_array(0, half) && gfx_world_texture(half, 0, 0));
    assert(gfx_world_array(1, other) && gfx_world_texture(other, 1, 0));
    add(0, -16.0f, -8.0f, 0.0f, 8.0f, 50.0f, SW_PASS_OPAQUE_CLAMP, 0);
    add(1, -40.0f, -30.0f, 40.0f, 30.0f, 100.0f, SW_PASS_GROUND, 0);
    add(2, 4.0f, -8.0f, 12.0f, 8.0f, 60.0f, SW_PASS_ALPHA_CLAMP, 0);
    add(3, -16.0f, 10.0f, 0.0f, 16.0f, 55.0f, SW_PASS_OPAQUE_CLAMP, 1);
    add(4, 14.0f, -8.0f, 20.0f, 8.0f, 65.0f, SW_PASS_OPAQUE_CLAMP, SW_NO_ARRAY);
    memcpy(sorted, records, sizeof sorted);
    sw_records_sort(sorted, RECORDS);
    assert(gfx_world_records(sorted, RECORDS, RECORDS));
    memset(&slot, 0, sizeof slot);
    slot.local[0] = slot.local[4] = slot.local[8] = 1.0f;
    slot.parent = slot.animated = -1;
    assert(gfx_world_slots(&slot, 0, 1));
}

/* A frame with shadows, the light along +z from the eye, the camera 100 from its focus, the
   ground drawn; then its picture read, which waits for the render thread to record it. */
static void frame(void)
{
    static const float light[3] = { 0.0f, 0.0f, 1.0f };
    static const struct ShadowFitBox bounds = { { -40.0f, -30.0f, 40.0f }, { 40.0f, 30.0f, 110.0f } };
    struct GfxWorldView view;
    struct GfxWorldFrame f;
    struct GfxWorldDraw ground;
    struct GfxShadow shadow;
    struct ShadowFitCamera camera;
    struct SwProjection p;
    RECT r;
    DWORD pixel;
    memset(&view, 0, sizeof view);
    view.view[0] = view.view[4] = view.view[8] = 1.0f;
    view.proj[0] = view.proj[1] = 100.0f;
    view.centre[0] = SCREEN_W / 2.0f;
    view.centre[1] = SCREEN_H / 2.0f;
    view.nearZ = 10.0f;
    view.depthScale = 1.0f;
    memset(&camera, 0, sizeof camera);
    memcpy(camera.view, view.view, sizeof camera.view);
    memcpy(camera.proj, view.proj, sizeof camera.proj);
    memcpy(camera.centre, view.centre, sizeof camera.centre);
    camera.width = SCREEN_W;
    camera.height = SCREEN_H;
    camera.nearZ = view.nearZ;
    camera.farZ = 1e6f;
    camera.distance = 100.0f;
    memset(&shadow, 0, sizeof shadow);
    assert(shadow_fit_frame(&shadow.fit, &camera, light) && shadow_fit_bounds(&shadow.fit, &bounds, 1));
    shadow.darkness = 0.5f;
    shadow.ambient[0] = shadow.ambient[1] = shadow.ambient[2] = 0.5f;
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    memset(&f, 0, sizeof f);
    f.shadow = &shadow;
    f.camera = view;
    p.proj[0] = p.proj[1] = 100.0f;
    p.centre[0] = view.centre[0];
    p.centre[1] = view.centre[1];
    p.size[0] = SCREEN_W;
    p.size[1] = SCREEN_H;
    p.nearZ = view.nearZ;
    p.farZ = 1e6f;
    sw_frustum(&f.frustum, &p, view.view);
    gfx_world_frame(&f);
    gfx_set_world_camera(&view);
    memset(&ground, 0, sizeof ground);
    ground.kind = GFX_MESH_GROUND;
    ground.colour[0] = ground.colour[1] = ground.colour[2] = ground.colour[3] = 1.0f;
    gfx_set_mode(DRAW_GROUND_LIGHTMAP);
    gfx_world_draw_record(&records[1], &ground);
    gfx_end_scene();
    r.left = r.top = 0;
    r.right = r.bottom = 1;
    assert(gfx_read_frame(&r, &pixel, 4) == 0);
}

/* whether the shadow pass applies the alpha test to a record */
static int caster_tested(const struct SwRecord *r)
{
    return r->textureArray != SW_NO_ARRAY && r->pass != SW_PASS_GROUND;
}

/* The frame's draws into the cascades against what their runs need: answers how many took each
   pipeline, casters[0] the alpha-tested one's. */
static void check_draws(int casters[2])
{
    int i;
    assert(!overflow);
    casters[0] = casters[1] = 0;
    for (i = 0; i < ndraws; i++) {
        const struct watched_draw *d = &draws[i];
        uint32_t command = d->offset / INDIRECT_SIZE;
        /* the casters' run `command` of its cascade: every record of it in the casters' order */
        uint32_t k = (command - (uint32_t)W.nindirect) % (uint32_t)W.ncasterRuns;
        const struct WlRun *run = &W.casterRuns[k];
        const struct SwRecord *r = &sorted[W.order[run->first]];
        int tested = caster_tested(r);
        uint32_t j;
        assert(d->offset % INDIRECT_SIZE == 0);
        assert(command >= W.nindirect && command < W.nindirect + W.ncasterRuns * GFX_SHADOW_CASCADES);
        for (j = 1; j < run->count; j++)
            assert(caster_tested(&sorted[W.order[run->first + j]]) == tested);
        assert(d->pipeline == W.casterPipeline[tested]);
        assert(!tested || d->texture == W.arrays[r->textureArray].tex);
        casters[!tested]++;
    }
}

int main(void)
{
    struct TestScreen *d = &g_screen;
    gfx_surface *half, *other;
    int casters[2];
    SDL_setenv_unsafe("RAGEXE_MSAA", "1", 1);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->window = SDL_CreateWindow("gfx passes", SCREEN_W, SCREEN_H, SDL_WINDOW_HIDDEN);
    if (!d->window) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->width = SCREEN_W;
    d->height = SCREEN_H;
    assert(gfx_world_attach());
    assert(gfx_open(d->window) == 0);
    d->screen = gfx_create_screen(d->width, d->height);
    if (!d->screen) {
        fprintf(stderr, "skipped: no GPU device\n");
        return 77;
    }
    assert(gfx_get_caps(&g_gfxCaps) == 0);
    assert(gfx_create_zbuffer() == 0);
    if (!gfx_shadows_supported()) {
        fprintf(stderr, "skipped: the device draws no shadows\n");
        return 77;
    }
    half = texture_2x2(0x00ffffffu);
    other = texture_4x4(0xffffffffu);
    world(half, other);
    frame();
    ndraws = 0;
    frame();
    check_draws(casters);
    /* in array 0 the opaque pass's model, the ground and the alpha pass's model take the casters'
       pipelines by turns, then array 1's model the alpha-tested one again with its own texture, and
       the model without a texture the other: five runs a cascade, three alpha-tested */
    assert(W.ncasterRuns == 5 && casters[0] == 3 * GFX_SHADOW_CASCADES && casters[1] == 2 * GFX_SHADOW_CASCADES);
    gfx_release(half);
    gfx_release(other);
    gfx_close();
    gfx_release(d->screen);
    SDL_DestroyWindow(d->window);
    SDL_Quit();
    printf("gfx gpu passes: %d shadow pass draws, %d of them alpha-tested, each with its run's pipeline and "
           "texture\n",
           casters[0] + casters[1], casters[0]);
    return 0;
}
