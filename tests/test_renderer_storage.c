#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "owned_storage_test_allocator.h"

static void *checked_calloc(size_t count, size_t size)
{
    void *p;
    assert(!size || count <= SIZE_MAX / size);
    p = checked_malloc(count * size);
    if (p) memset(p, 0, count * size);
    return p;
}
static void *checked_realloc(void *memory, size_t size)
{
    void *p = checked_malloc(size);
    if (p && memory) {
        size_t old = ((struct Allocation *)memory - 1)->size;
        memcpy(p, memory, old < size ? old : size);
        checked_free(memory);
    }
    return p;
}

#define calloc checked_calloc
#define realloc checked_realloc
#define free checked_free
#include "../src/render/rp_pool.c"
#undef free
#undef realloc
#undef calloc
#include "../src/render/crenderer.c"
#include "../src/effects/multitex_particle.c"
#include "../src/effects/effect_render_hooks.c"

void *mem_alloc(size_t bytes) { void *p = checked_malloc(bytes); assert(p); return p; }

void *mem_try_alloc(size_t bytes) { return checked_malloc(bytes); }
void mem_free(void *p)
{ checked_free(p); }

void fatal_error(const char *message) { (void)message; abort(); }
static unsigned atlas_starts, atlas_ends, frame_ends, effect_ends;
struct C3dDevice g_3dDevice;
struct EffectMgr *g_effectMgr;
int g_lightmap;
void sprite_atlas_initialize(struct Renderer *r) { assert(r); ++atlas_starts; }
void sprite_atlas_destroy(struct Renderer *r) { assert(r); ++atlas_ends; }

static size_t queued(const struct Renderer *r, size_t pass)
{
    return r->queues[pass].count;
}
void sprite_atlas_finish_frame(struct Renderer *r)
{
    size_t i;
    assert(effect_ends == frame_ends + 1);
    for (i = 0; i < RENDER_PASS_COUNT; ++i) assert(!queued(r, i));
    ++frame_ends;
}
void effect_mgr_destroy_removed(struct EffectMgr *mgr) { assert(mgr == g_effectMgr); ++effect_ends; }
void error_msg(const char *text, int flags) { (void)text; (void)flags; abort(); }
void perf_begin(int phase) { (void)phase; }
void perf_end(int phase) { (void)phase; }
int c3d_device_show_frame(struct C3dDevice *device, char vsync) { (void)device; (void)vsync; abort(); return 0; }
void c3d_device_save_frame(struct C3dDevice *device) { (void)device; abort(); }
void perf_frame_end(struct Renderer *renderer) { (void)renderer; abort(); }
void perf_overlay_draw(struct Renderer *renderer) { (void)renderer; abort(); }

struct gfx_surface {
    unsigned id, refs;
    int allocated, width, height, format;
    DWORD pixels[16 * 16];
};
static unsigned live_surfaces, retained_surfaces, released_surfaces;
static gfx_surface *bound_surfaces[2];
void gfx_addref(gfx_surface *surface)
{
    assert(surface && (!surface->allocated || surface->refs));
    ++surface->refs; ++retained_surfaces;
}
void gfx_release(gfx_surface *surface)
{
    assert(surface && surface->refs);
    --surface->refs; ++released_surfaces;
    if (surface->allocated && !surface->refs) {
        unsigned i;
        for (i = 0; i < 2; ++i) if (bound_surfaces[i] == surface) bound_surfaces[i] = NULL;
        assert(live_surfaces); --live_surfaces; checked_free(surface);
    }
}
static int capture_draws;
static unsigned sampled_count;
static int capture_projection;
static unsigned projection_draw_count;
static struct TexDrawVertex projection_draws[2][6];
static struct { unsigned id[2]; DWORD pixel[2]; } sampled[64];
static void sample_textures(void)
{
    unsigned i;
    if (!capture_draws) return;
    assert(sampled_count < 64);
    for (i = 0; i < 2; ++i) {
        gfx_surface *surface = bound_surfaces[i];
        assert(!surface || surface->refs);
        sampled[sampled_count].id[i] = surface ? surface->id : 0;
        sampled[sampled_count].pixel[i] = surface ? surface->pixels[0] : 0;
    }
    ++sampled_count;
}
static uint64_t trace;
static unsigned draws;
static int current_mode = -1;
static void word(unsigned value) { trace = (trace ^ value) * UINT64_C(1099511628211); }
static void bytes(const void *data, size_t count)
{ const BYTE *p = data; size_t i; for (i = 0; i < count; ++i) word(p[i]); }
void gfx_set_mode(int mode)
{ assert(draw_mode(mode)); current_mode = mode; word(1); word((unsigned)mode); }
void gfx_set_effect(int src, int dst, int combine)
{
    assert(combine >= 0 && combine < DRAW_COMBINE_COUNT && combine != 2);
    word(2); word((unsigned)src); word((unsigned)dst); word((unsigned)combine);
}
void gfx_set_fog(const struct GfxFog *fog)
{ word(4); word((unsigned)fog->on); word(fog->colour); bytes(&fog->start, sizeof fog->start); bytes(&fog->end, sizeof fog->end); }
void gfx_set_texture(int stage, gfx_surface *surface)
{
    assert(stage >= 0 && stage < 2 && (!surface || !surface->allocated || surface->refs));
    bound_surfaces[stage] = surface;
    word(3); word(stage); word(surface ? surface->id : 0);
}
void gfx_draw_vertices(const struct TexDrawVertex *vertices, int count, int strip, int world)
{
    assert(count > 0 && (count <= 4 || capture_projection));
    assert(draw_mode(current_mode) && (draw_mode(current_mode)->into & DRAW_INTO_SCREEN));
    if (capture_projection) {
        assert(!strip && !world && count == 6 && projection_draw_count < 2);
        memcpy(projection_draws[projection_draw_count++], vertices, sizeof projection_draws[0]);
    }
    word(5); word((unsigned)strip); word((unsigned)world); word((unsigned)count);
    bytes(vertices, (size_t)count * sizeof *vertices);
    sample_textures();
    ++draws;
}
void gfx_draw_quad(const struct GfxQuad *quad)
{
    assert(quad && quad->kind == GFX_QUAD_WORLD && quad->world.stage2);
    assert(draw_mode(current_mode) && (draw_mode(current_mode)->into & DRAW_INTO_SCREEN));
    word(8); bytes(quad, sizeof *quad);
    sample_textures();
    ++draws;
}
static int mesh_token;
static const float mesh_view[12] = {12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
void gfx_set_world_view(const struct GfxWorldView *view)
{ assert(!memcmp(view->view, mesh_view, sizeof mesh_view)); word(7); }
void gfx_draw_mesh(gfx_mesh *mesh, const struct GfxMeshDraw *draw)
{
    assert(mesh == (gfx_mesh *)&mesh_token && draw->first == 0 && draw->count == 3);
    assert(draw_mode(current_mode) && (draw_mode(current_mode)->into & DRAW_MESHES));
    word(6); word((unsigned)draw->first); word((unsigned)draw->count); sample_textures(); ++draws;
}
int gfx_shadow_begin(const struct GfxShadow *shadow) { (void)shadow; abort(); }
int gfx_shadow_cascade(int cascade) { (void)cascade; abort(); }
void gfx_shadow_end(void) { abort(); }
void gfx_shadow_receive(int receive) { assert(!receive); }
void gfx_set_world_camera(const struct GfxWorldView *view) { (void)view; abort(); }
void renderer_world_view(const struct Renderer *renderer, const float *view, struct GfxWorldView *out)
{
    assert(renderer == g_renderer);
    memset(out, 0, sizeof *out); memcpy(out->view, view, sizeof out->view);
}
int gfx_begin_scene(void) { abort(); }
void gfx_end_scene(void) { abort(); }
gfx_surface *gfx_create_surface(int w, int h) { (void)w; (void)h; abort(); }
gfx_surface *gfx_create_render_target(int w, int h) { (void)w; (void)h; abort(); }
int gfx_blit(gfx_surface *dst, const RECT *dstRect, gfx_surface *src, const RECT *srcRect)
{ (void)dst; (void)dstRect; (void)src; (void)srcRect; abort(); }

static void growth(void)
{
    struct Renderer renderer = {0};
    struct TexDrawPrim *first, *instance;
    struct RPQuad *quad;
    struct TexDrawVertex *vertices;
    unsigned pass, i, allocations = 0;
    renderer.clearColor = 0xaabbccdd;
    assert(renderer_construct(&renderer));
    assert(renderer.clearColor == 0);
    for (pass = 0; pass < 2; ++pass) {
        first = renderer_new_prim(&renderer);
        assert(first->base.verts == first->quad && first->base.nverts == 4 && first->base.prim == 5);
        first->quad[0].diffuse = 0x12345678;
        for (i = 1; i < 600; ++i) assert(renderer_new_prim(&renderer)->base.nverts == 4);
        assert(first->quad[0].diffuse == 0x12345678);
        quad = renderer_new_quad(&renderer);
        vertices = renderer_new_tex_draw_vertices(&renderer, 10240);
        quad->verts = vertices; quad->nverts = 3;
        vertices[0].x = 123.0f; vertices[10239].diffuse = 0xabc;
        assert(renderer_new_tex_draw_vertices(&renderer, 5000));
        assert(((struct TexDrawVertex *)quad->verts)[0].x == 123.0f);
        assert(((struct TexDrawVertex *)quad->verts)[10239].diffuse == 0xabc);
        assert(quad->verts == vertices);
        assert(!renderer_new_tex_draw_vertices(&renderer, -1));
        assert(!renderer_new_tex_draw_vertices(&renderer, 0));
        for (i = 1; i < 1100; ++i) assert(!renderer_new_quad(&renderer)->instance);
        instance = renderer_new_multi_tex_quad(&renderer);
        assert(instance->base.instance == &instance->instance);
        instance->instance.kind = GFX_QUAD_WORLD;
        for (i = 1; i < 2100; ++i) assert(renderer_new_multi_tex_quad(&renderer)->base.instance);
        assert(instance->instance.kind == GFX_QUAD_WORLD);
        if (pass) assert(allocation_calls == allocations);
        allocations = allocation_calls;
        renderer_destroy_all_rp_list(&renderer);
    }
    renderer_destruct(&renderer);
    assert(!outstanding);
}

/* The quads of sprites and sprite particles are the primitives pool's, taken in the order the
   client asks for them, as when the CPU made their vertices: the quad of the same place in a later
   frame finds the header the last user left there (renderer_new_prim), instance or vertices. */
static void shared_slots(void)
{
    struct Renderer renderer = {0};
    struct TexDrawPrim *a, *b, *c;
    assert(renderer_construct(&renderer));
    a = renderer_new_prim(&renderer);
    b = renderer_new_prim_instance(&renderer);
    c = renderer_new_prim(&renderer);
    assert(b == a + 1 && c == b + 1);
    assert(!a->base.instance && b->base.instance == &b->instance && !c->base.instance);
    a->base.stageMode = 7; a->base.srcBlend = 1; a->base.world = 1;
    b->base.alpha_mode = 0; b->base.srcBlend = 5; b->base.destBlend = 2;
    b->base.texture = (struct Texture *)&renderer;
    memset(&b->instance, 0x5a, sizeof b->instance);
    renderer_destroy_all_rp_list(&renderer);
    /* a sprite's quad where an STR effect's was, a quad of vertices where the sprite's was */
    c = renderer_new_prim_instance(&renderer);
    assert(c == a && c->base.instance == &c->instance && !c->base.world);
    assert(c->base.stageMode == 7 && c->base.srcBlend == 1 && c->base.alpha_mode == 1);
    assert(c->instance.kind == 0 && c->instance.world.viewSpace == 0 && c->instance.world.stage2 == 0);
    c = renderer_new_prim(&renderer);
    assert(c == b && !c->base.instance);
    assert(c->base.alpha_mode == 0 && c->base.srcBlend == 5 && c->base.destBlend == 2);
    assert(c->base.texture == (struct Texture *)&renderer);
    assert(c->base.verts == c->quad && c->base.nverts == 4 && c->base.prim == RP_STRIP);
    renderer_destroy_all_rp_list(&renderer);
    renderer_destruct(&renderer);
    assert(!outstanding);
}

static void routing(void)
{
    struct Renderer renderer = {0};
    struct TexDrawPrim quad = {0};
    unsigned flags, mode, i;
    assert(renderer_construct(&renderer)); rp_quad_batch_construct(&quad);
    quad.quad[0].rhw = 0.5f; quad.quad[1].rhw = 0.75f;
    for (mode = 0; mode < 2; ++mode) {
        quad.base.alpha_mode = (BYTE)mode;
        for (flags = 0; flags < 65536; ++flags) renderer_add_prim(&renderer, &quad.base, flags);
        printf("route %u", mode);
        for (i = 0; i < RENDER_PASS_COUNT; ++i) printf(" %u", (unsigned)queued(&renderer, i));
        putchar('\n'); renderer_destroy_all_rp_list(&renderer);
    }
    renderer_destruct(&renderer); assert(!outstanding);
}

static void render_trace(void)
{
    static const int flags[] = {0,0x20,0x101,1,0x201,0x21,0x8001,0x103,3,0x203,5,0xd,0x209,0x11,0x205};
    struct Renderer renderer = {0};
    struct TexDrawPrim quads[96];
    struct TexDrawPrim pairs[8];
    struct Texture textures[4] = {0};
    struct gfx_surface surfaces[4] = {{1},{2},{3},{4}};
    unsigned scenario, i, j;
    assert(renderer_construct(&renderer));
    for (i = 0; i < 4; ++i) textures[i].base.surface = &surfaces[i];
    g_fogNear = 0.2f; g_fogFar = 0.8f;
    for (scenario = 0; scenario < 8; ++scenario) {
        renderer.twoPassAlpha = scenario & 1;
        g_lightmap = (scenario >> 1) & 1;
        renderer.fogEnable = (scenario >> 2) & 1;
        for (i = 0; i < 96; ++i) {
            struct TexDrawPrim *q = quads + i;
            rp_quad_batch_construct(q);
            q->base.texture = textures + (i * 3 % 4); q->base.srcBlend = 2; q->base.destBlend = 4; q->base.stageMode = i % 9;
            q->base.alpha_mode = (BYTE)(i & 1);
            for (j = 0; j < 4; ++j) {
                q->quad[j].x = (float)i; q->quad[j].y = (float)j;
                q->quad[j].rhw = i & 1 ? 0.0008f + (float)i / 100000.0f : 0.1f + (float)i / 80.0f;
                q->quad[j].u = (float)j / 4.0f; q->quad[j].v = 0.5f;
                q->quad[j].diffuse = i + 0xff000000u;
            }
            renderer_add_prim(&renderer, &q->base, flags[i % (sizeof flags / sizeof flags[0])]);
        }
        for (i = 0; i < 8; ++i) {
            rp_instance_construct(pairs + i);
            pairs[i].base.texture = textures + (i % 4);
            pairs[i].instance.kind = GFX_QUAD_WORLD;
            pairs[i].instance.world.stage2 = 1;
            pairs[i].instance.world.colour = i + 1;
            for (j = 0; j < 4; ++j) pairs[i].instance.world.point[j][2] = 800.0f;
            renderer_add_multi_tex_batch(&renderer, &pairs[i].base, textures + ((i + 1) % 4));
        }
        current_mode = -1; trace = UINT64_C(1469598103934665603); draws = 0;
        renderer_draw_lists(&renderer);
        printf("render %u %u %016llx\n", scenario, draws, (unsigned long long)trace);
        renderer_destroy_all_rp_list(&renderer);
    }
    renderer_destruct(&renderer); assert(!outstanding);
}

static int ground_available = 1, view_available = 1, cell_available = 1;
static unsigned projected_vertices;
static int expected_cell_x, expected_cell_y;
static const float test_view[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
static int ground_token;

struct C3dGround15 *game_current_ground(void)
{ return ground_available ? (struct C3dGround15 *)&ground_token : NULL; }
int game_copy_view_matrix(float *view)
{ if (!view_available) return 0; memcpy(view, test_view, sizeof test_view); return 1; }
BYTE c3d_ground15_get_attr_quad_half(struct C3dGround15 *ground, int x, int y, float *out)
{
    int i;
    assert(ground == (struct C3dGround15 *)&ground_token);
    assert(x == expected_cell_x && y == expected_cell_y);
    if (!cell_available) return 0;
    for (i = 0; i < 4; ++i) {
        out[3*i] = (float)(x + i); out[3*i+1] = (float)(y - i); out[3*i+2] = (float)i / 4;
    }
    return 1;
}
float renderer_project_rhw(const Vec3 *in, const float *view)
{
    assert(!memcmp(view, test_view, sizeof test_view));
    ++projected_vertices;
    return in->z == 0.0f ? 0.00125f : 0.0025f;
}
struct Particle *particle_construct(struct Particle *particle, Vec3 pos, int flags)
{ (void)pos; (void)flags; memset(particle, 0, sizeof *particle); return particle; }
void particle_reset(struct Particle *particle) { assert(particle); }

/* the instance of a multitexture particle's quad and its second texture's sort key in the list */
static const struct GfxQuad *multitex_queued(struct Renderer *renderer, size_t n, const struct RPQuad **quad,
                                             unsigned *second)
{
    const struct RenderEntry *e = renderer->queues[RENDER_MULTITEXTURE].items + n;
    *quad = e->primitive;
    *second = e->textures[1].sort_key;
    return (*quad)->instance;
}

static void multitex_frame_vertices(void)
{
    struct Renderer renderer = {0};
    struct MultiTexParticle particle;
    struct Texture textures[3] = {0};
    const struct RPQuad *first, *second;
    const struct GfxQuad *q, *r;
    struct GfxQuad saved;
    Vec3 origin = {0};
    unsigned allocations, i, before_projection, key;
    assert(renderer_construct(&renderer)); g_renderer = &renderer;
    renderer.proj[0] = 5.0f; renderer.proj[1] = 6.0f; renderer.centreX = 7; renderer.centreY = 8;
    for (i = 0; i < 3; ++i) textures[i].serial = 100 + i;
    allocations = allocation_calls;
    memset(&particle, 0xa5, sizeof particle);
    assert(multi_tex_particle_construct(&particle, origin, -1) == &particle);
    assert(allocation_calls == allocations);
    assert(!particle.texture0 && !particle.texture1 && !particle.u_scroll && !particle.v_scroll);
    particle.texture0 = textures; particle.texture1 = textures + 1;
    particle.u_scroll = 0.25f; particle.v_scroll = 0.5f;
    for (i = 0; i < 4; ++i) {
        particle.uv0[i].u = (float)i / 4; particle.uv0[i].v = (float)i / 8;
        particle.uv1[i].u = (float)i / 16; particle.uv1[i].v = (float)i / 32;
    }
    expected_cell_x = 10; expected_cell_y = 20;
    before_projection = projected_vertices;
    ground_available = 0; multi_tex_particle_render_on_cell(&particle, 10, 20); ground_available = 1;
    view_available = 0; multi_tex_particle_render_on_cell(&particle, 10, 20); view_available = 1;
    cell_available = 0; multi_tex_particle_render_on_cell(&particle, 10, 20); cell_available = 1;
    assert(projected_vertices == before_projection && allocation_calls == allocations);
    assert(!renderer.queues[RENDER_MULTITEXTURE].count);
    multi_tex_particle_render_on_cell(&particle, 10, 20);
    q = multitex_queued(&renderer, 0, &first, &key);
    /* the cell's corners for the device to project through the view, z nudged towards the camera,
       both stages' coordinates; flat only where every corner's rhw is the same (the stub gives the
       first corner, at height 0, another) */
    assert(q && q->kind == GFX_QUAD_WORLD && !q->world.viewSpace && q->world.stage2);
    assert(first->texture == textures && key == 101 && first->instance == q);
    assert(!memcmp(q->world.view, test_view, sizeof test_view));
    assert(q->world.proj[0] == 5.0f && q->world.proj[1] == 6.0f);
    assert(q->world.centre[0] == 7.0f && q->world.centre[1] == 8.0f);
    assert(q->world.rhw == GFX_RHW_PROJECTED && q->world.depth == GFX_DEPTH_ADD);
    assert(q->world.depthValue == -k_multitexDepthNudge && !q->world.flat);
    assert(q->world.colour == 0xffffffff);
    assert(projected_vertices == before_projection + 4);
    for (i = 0; i < 4; ++i) {
        assert(q->world.point[i][0] == (float)(10 + i) && q->world.point[i][1] == (float)(20 - (int)i));
        assert(q->world.point[i][2] == (float)i / 4);
        assert(q->world.u[i] == particle.uv0[i].u + 0.25f && q->world.v[i] == particle.uv0[i].v + 0.5f);
        assert(q->world.u1[i] == particle.uv1[i].u && q->world.v1[i] == particle.uv1[i].v);
    }
    saved = *q;
    particle.texture0 = textures + 2; particle.texture1 = NULL;
    particle.u_scroll = 2; expected_cell_x = 30; expected_cell_y = 40;
    multi_tex_particle_render_on_cell(&particle, 30, 40);
    r = multitex_queued(&renderer, 1, &second, &key);
    assert(first != second && r != q);
    assert(second->texture == textures + 2 && key == 0 && r->world.u[0] == 2);
    assert(first->texture == textures && !memcmp(&saved, q, sizeof saved));
    multi_tex_particle_reset(&particle);
    assert(!particle.texture0 && !particle.texture1 && !particle.u_scroll && !particle.v_scroll);
    for (i = 0; i < 8300; ++i) multi_tex_particle_render_on_cell(&particle, 30, 40);
    memset(&particle, 0xdd, sizeof particle);
    assert(renderer.queues[RENDER_MULTITEXTURE].count == 8302);
    assert(first->texture == textures && !memcmp(&saved, q, sizeof saved));
    renderer_destroy_all_rp_list(&renderer);
    allocations = allocation_calls;
    multi_tex_particle_construct(&particle, origin, -1);
    memset(particle.uv0, 0, sizeof particle.uv0); memset(particle.uv1, 0, sizeof particle.uv1);
    multi_tex_particle_render_on_cell(&particle, 30, 40);
    assert(allocation_calls == allocations && renderer.queues[RENDER_MULTITEXTURE].count == 1);
    r = multitex_queued(&renderer, 0, &second, &key);
    assert(!second->texture && !key && !r->world.u[0] && !r->world.v1[0]);
    renderer_destroy_all_rp_list(&renderer); renderer_destruct(&renderer);
    assert(!outstanding && !g_renderer);
    puts("Multitexture particles: frame-owned instances, projection inputs, skipped cells, UVs, repeated draws, particle reuse and pool growth passed.");
}

static void failures(void)
{
    struct Renderer renderer = {0}; unsigned point;
    for (point = 1; point <= 9; ++point) {
        fail_at = allocation_calls + point;
        assert(!renderer_construct(&renderer));
        assert(!outstanding && !renderer.storage);
        fail_at = 0;
        assert(renderer_construct(&renderer));
        g_renderer = &renderer;
        renderer_destruct(&renderer); renderer_destruct(&renderer);
        assert(!outstanding && !g_renderer);
    }
}

#include "renderer_texture_owner_test.h"
#include "renderer_granny_projection_test.h"

int main(int argc, char **argv)
{
    (void)argv;
    g_renderBeforeClearLists = effect_mgr_destroy_removed_effects;
    if (argc > 1 && (!strcmp(argv[1], "growth_failure") || !strcmp(argv[1], "queue_failure"))) {
        struct Renderer renderer = {0}; struct TexDrawPrim *quad; unsigned i;
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        assert(renderer_construct(&renderer));
        if (!strcmp(argv[1], "growth_failure")) {
            for (i = 0; i < 512; ++i) renderer_new_prim(&renderer);
            fail_at = allocation_calls + 1; renderer_new_prim(&renderer);
        } else {
            quad = renderer_new_prim(&renderer); fail_at = allocation_calls + 1;
            renderer_add_prim(&renderer, &quad->base, 0);
        }
        return 99;
    }
    if (argc == 1) { failures(); multitex_frame_vertices(); queued_texture_ownership(); granny_projection_ownership(); }
    growth(); shared_slots(); routing(); render_trace();
    assert(!outstanding && atlas_starts && atlas_ends && frame_ends == effect_ends);
    if (argc == 1) puts("Renderer storage: C ownership, stable growth, reuse, every setup failure, all flag routes and real draw passes passed.");
    return 0;
}
