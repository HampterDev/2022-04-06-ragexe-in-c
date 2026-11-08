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
#ifdef RENDERER_BASELINE
#else
#include "../src/render/rp_pool.c"
#endif
#undef free
#undef realloc
#undef calloc
#ifdef RENDERER_BASELINE
#else
#include "../src/render/crenderer.c"
#include "../src/effects/multitex_particle.c"
#include "../src/effects/effect_render_hooks.c"
#endif

void *mem_alloc(size_t bytes) { void *p = checked_malloc(bytes); assert(p); return p; }

void *mem_try_alloc(size_t bytes) { return checked_malloc(bytes); }
void mem_free(void *p)
{ checked_free(p); }

void fatal_error(const char *message) { (void)message; abort(); }
static unsigned atlas_starts, atlas_ends, frame_ends, effect_ends;
struct C3dDevice g_3dDevice;
struct gfx_caps g_gfxCaps;
#ifdef RENDERER_BASELINE
void *g_effectMgr;
#else
struct EffectMgr *g_effectMgr;
#endif
int g_lightmap;
BYTE g_waterBumpSupported;
void sprite_atlas_initialize(struct Renderer *r) { assert(r); ++atlas_starts; }
void sprite_atlas_destroy(struct Renderer *r) { assert(r); ++atlas_ends; }
#ifdef RENDERER_BASELINE
#define TEST_PASSES 24
#else
#define TEST_PASSES RENDER_PASS_COUNT
#endif
static size_t queued(const struct Renderer *r, size_t pass)
{
#ifdef RENDERER_BASELINE
    static const BYTE keyed[24] = {0,0,0,1,1,1,1,0,1,1,1,1,1,0,0,0,0,0,0,0,1,1,0,1};
    if (!r->lists[pass].begin) return 0;
    return (size_t)(r->lists[pass].end - r->lists[pass].begin) / (keyed[pass] ? 8 : 4);
#else
    return r->queues[pass].count;
#endif
}
void sprite_atlas_finish_frame(struct Renderer *r)
{
    size_t i;
    assert(effect_ends == frame_ends + 1);
    for (i = 0; i < TEST_PASSES; ++i) assert(!queued(r, i));
    ++frame_ends;
}
#ifdef RENDERER_BASELINE
void effect_mgr_destroy_removed(void *mgr) { assert(mgr == g_effectMgr); ++effect_ends; }
#else
void effect_mgr_destroy_removed(struct EffectMgr *mgr) { assert(mgr == g_effectMgr); ++effect_ends; }
#endif
void error_msg(const char *text, int flags) { (void)text; (void)flags; abort(); }
int32_t mat4_perspective_lh(float *m, float fov, float aspect, float near_z, float far_z)
{ (void)m; (void)fov; (void)aspect; (void)near_z; (void)far_z; abort(); return -1; }
void perf_begin(int phase) { (void)phase; }
void perf_end(int phase) { (void)phase; }
int c3d_device_check_lost(struct C3dDevice *device) { (void)device; abort(); return 0; }
void c3d_device_on_surfaces_lost(struct C3dDevice *device) { (void)device; abort(); }
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
static DWORD states[256];
static void word(unsigned value) { trace = (trace ^ value) * UINT64_C(1099511628211); }
static void bytes(const void *data, size_t count)
{ const BYTE *p = data; size_t i; for (i = 0; i < count; ++i) word(p[i]); }
void gfx_set_render_state(int state, DWORD value)
{ assert((unsigned)state < 256); states[state] = value; word(1); word(state); word(value); }
int gfx_get_render_state(int state, DWORD *out)
{ assert((unsigned)state < 256); *out = states[state]; return 0; }
static DWORD stage_states[2][32];
void gfx_set_stage_state(int stage, int state, DWORD value)
{
    if (stage >= 0 && stage < 2 && state >= 0 && state < 32) stage_states[stage][state] = value;
    word(2); word(stage); word(state); word(value);
}
void gfx_set_texture(int stage, gfx_surface *surface)
{
    assert(stage >= 0 && stage < 2 && (!surface || !surface->allocated || surface->refs));
    bound_surfaces[stage] = surface;
    word(3); word(stage); word(surface ? surface->id : 0);
}
void gfx_set_transform(int which, const struct GfxMatrix *matrix)
{ word(4); word(which); bytes(matrix, sizeof *matrix); }
void gfx_draw(int primitive, int kind, const void *vertices, int count, const WORD *indices, int index_count)
{
    assert(count > 0 && (count <= 4 || capture_projection) && (!index_count || indices));
    if (capture_projection) {
        assert(primitive == GFX_PT_TRIANGLELIST && kind == GFX_VTX_TEX1 && count == 6);
        assert(!index_count && projection_draw_count < 2);
        memcpy(projection_draws[projection_draw_count++], vertices, sizeof projection_draws[0]);
    }
    word(5); word(primitive); word(kind); word(count); word(index_count);
    bytes(vertices, (size_t)count * (kind == GFX_VTX_TEX1 ? sizeof(struct TexDrawVertex) : sizeof(struct RPTwoTexVertex)));
    if (index_count) bytes(indices, (size_t)index_count * sizeof *indices);
    sample_textures();
    ++draws;
}
static int mesh_token;
static const float mesh_view[12] = {12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
void gfx_set_world_view(const struct GfxWorldView *view)
{ assert(!memcmp(view->view, mesh_view, sizeof mesh_view)); word(7); }
void gfx_draw_mesh(gfx_mesh *mesh, const float *model, int first, int count)
{
    assert(mesh == (gfx_mesh *)&mesh_token && !model && first == 0 && count == 3);
    word(6); word((unsigned)first); word((unsigned)count); sample_textures(); ++draws;
}
void renderer_world_view(const struct Renderer *renderer, const float *view, const float *scale,
                         struct GfxWorldView *out)
{
    assert(renderer == g_renderer && scale && scale[0] == 1.0f && scale[1] == 1.0f && scale[2] == 1.0f);
    memset(out, 0, sizeof *out); memcpy(out->view, view, sizeof out->view);
}
int gfx_begin_scene(void) { abort(); }
void gfx_end_scene(void) { abort(); }
void gfx_set_light(const float *dir, const float *diffuse, const float *ambient)
{ (void)dir; (void)diffuse; (void)ambient; abort(); }
gfx_surface *gfx_create_surface(int w, int h) { (void)w; (void)h; abort(); }
int gfx_blit(gfx_surface *dst, const RECT *dstRect, gfx_surface *src, const RECT *srcRect, int flags)
{ (void)dst; (void)dstRect; (void)src; (void)srcRect; (void)flags; abort(); }

static void growth(void)
{
    struct Renderer renderer = {0};
    struct TexDrawPrim *first;
    struct RPTwoTexQuad *two;
    struct RPQuad *quad;
    struct TexDrawVertex *vertices;
    unsigned pass, i, allocations = 0;
#ifndef RENDERER_BASELINE
    renderer.clearColor = 0xaabbccdd;
#endif
    assert(renderer_construct(&renderer));
#ifndef RENDERER_BASELINE
    assert(renderer.clearColor == 0);
#endif
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
#ifndef RENDERER_BASELINE
        assert(quad->verts == vertices);
        assert(!renderer_new_tex_draw_vertices(&renderer, -1));
        assert(!renderer_new_tex_draw_vertices(&renderer, 0));
#endif
        for (i = 1; i < 1100; ++i) assert(renderer_new_quad(&renderer)->nindices == 0);
        two = renderer_new_two_tex_quad(&renderer);
        assert(two->prim.verts == two->quad && two->prim.nverts == 4);
        for (i = 1; i < 8200; ++i) assert(renderer_new_two_tex_quad(&renderer)->prim.nverts == 4);
        if (pass) assert(allocation_calls == allocations);
        allocations = allocation_calls;
        renderer_destroy_all_rp_list(&renderer);
    }
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
        for (i = 0; i < TEST_PASSES; ++i) printf(" %u", (unsigned)queued(&renderer, i));
        putchar('\n'); renderer_destroy_all_rp_list(&renderer);
    }
    renderer_destruct(&renderer); assert(!outstanding);
}

static void blend_stage_arguments(void)
{
    struct Renderer renderer = {0};
    renderer_set_blend_stages(&renderer, 6);
    assert(stage_states[0][GFX_TSS_COLORARG1] == (GFX_TA_TEXTURE | GFX_TA_COMPLEMENT));
    assert(stage_states[1][GFX_TSS_COLORARG1] == (GFX_TA_TEXTURE | GFX_TA_COMPLEMENT));
    assert(stage_states[1][GFX_TSS_COLOROP] == GFX_TOP_MODULATE);
    renderer_set_blend_stages(&renderer, 7);
    assert(stage_states[0][GFX_TSS_COLORARG1] == GFX_TA_TEXTURE);
    assert(stage_states[1][GFX_TSS_COLORARG1] == (GFX_TA_TEXTURE | GFX_TA_ALPHAREPLICATE));
    assert(stage_states[1][GFX_TSS_COLOROP] == GFX_TOP_MODULATE);
    renderer_set_blend_stages(&renderer, 12);
    assert(stage_states[0][GFX_TSS_COLORARG1] == GFX_TA_TEXTURE);
    assert(stage_states[1][GFX_TSS_COLORARG1] == (GFX_TA_TEXTURE | GFX_TA_COMPLEMENT));
    assert(stage_states[1][GFX_TSS_COLOROP] == GFX_TOP_ADD);
}

static void render_trace(void)
{
    static const int flags[] = {0,0x800,0x1000,0x101,1,0x201,0x21,0x8001,0x103,3,0x203,5,0xd,0x209,0x11,0x205};
    struct Renderer renderer = {0};
    struct TexDrawPrim quads[96];
    struct RPTwoTexQuad pairs[8];
    struct Texture textures[4] = {0};
    struct gfx_surface surfaces[4] = {{1},{2},{3},{4}};
    unsigned scenario, i, j;
    assert(renderer_construct(&renderer));
    for (i = 0; i < 4; ++i) textures[i].base.surface = &surfaces[i];
    g_fogNear = 0.2f; g_fogFar = 0.8f;
    for (scenario = 0; scenario < 16; ++scenario) {
        renderer.twoPassAlpha = scenario & 1;
        g_lightmap = (scenario >> 1) & 1;
        g_waterBumpSupported = (BYTE)((scenario >> 2) & 1);
        renderer.fogVertexCaps = (scenario >> 3) & 1;
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
            memset(pairs + i, 0, sizeof pairs[i]); rp_two_tex_quad_construct(pairs + i);
            pairs[i].prim.tex0 = textures + (i % 4); pairs[i].prim.tex1 = textures + ((i + 1) % 4);
            for (j = 0; j < 4; ++j) { pairs[i].quad[j].rhw = 0.00125f; pairs[i].quad[j].diffuse = i + 1; }
            if (i < 4) renderer_add_two_tex_prim(&renderer, &pairs[i].prim, 0);
            else renderer_add_multi_tex_batch(&renderer, &pairs[i].prim);
        }
        memset(states, 0, sizeof states); trace = UINT64_C(1469598103934665603); draws = 0;
        renderer_draw_lists(&renderer);
        printf("render %u %u %016llx\n", scenario, draws, (unsigned long long)trace);
        renderer_destroy_all_rp_list(&renderer);
    }
    renderer_destruct(&renderer); assert(!outstanding);
}

#ifndef RENDERER_BASELINE
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
void renderer_project_multitex_vertex(struct Renderer *renderer, const Vec3 *in,
    const float *view, struct RPTwoTexVertex *out)
{
    assert(renderer == g_renderer && !memcmp(view, test_view, sizeof test_view));
    out->x = in->x + 0.5f; out->y = in->y - 0.5f; out->z = in->z; out->rhw = 0.00125f;
    ++projected_vertices;
}
struct Particle *particle_construct(struct Particle *particle, Vec3 pos, int flags)
{ (void)pos; (void)flags; memset(particle, 0, sizeof *particle); return particle; }
void particle_reset(struct Particle *particle) { assert(particle); }

static void multitex_frame_vertices(void)
{
    struct Renderer renderer = {0};
    struct MultiTexParticle particle;
    struct Texture textures[3] = {0};
    struct RPTwoTex *first, *second;
    struct RPTwoTexVertex saved[4];
    Vec3 origin = {0};
    unsigned allocations, i, before_projection;
    assert(renderer_construct(&renderer)); g_renderer = &renderer;
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
    first = renderer.queues[RENDER_MULTITEXTURE].items[0].primitive;
    assert(first->nverts == 4 && first->prim == 5 && !first->indices && !first->nindices);
    assert(first->tex0 == textures && first->tex1 == textures + 1);
    assert(projected_vertices == before_projection + 4);
    for (i = 0; i < 4; ++i) {
        const struct RPTwoTexVertex *v = first->verts + i;
        assert(v->x == 10.5f + i && v->y == 19.5f - i);
        assert(v->z == (float)i / 4 - k_multitexDepthNudge && v->rhw == 0.00125f);
        assert(v->diffuse == 0xffffffff && v->specular == (DWORD)rp_vertex_fog_alpha(v->rhw) << 24);
        assert(v->u0 == particle.uv0[i].u + 0.25f && v->v0 == particle.uv0[i].v + 0.5f);
        assert(v->u1 == particle.uv1[i].u && v->v1 == particle.uv1[i].v);
    }
    memcpy(saved, first->verts, sizeof saved);
    particle.texture0 = textures + 2; particle.texture1 = NULL;
    particle.u_scroll = 2; expected_cell_x = 30; expected_cell_y = 40;
    multi_tex_particle_render_on_cell(&particle, 30, 40);
    second = renderer.queues[RENDER_MULTITEXTURE].items[1].primitive;
    assert(first != second && second->verts != first->verts);
    assert(second->tex0 == textures + 2 && !second->tex1 && second->verts[0].u0 == 2);
    assert(first->tex0 == textures && first->tex1 == textures + 1 && !memcmp(saved, first->verts, sizeof saved));
    multi_tex_particle_reset(&particle);
    assert(!particle.texture0 && !particle.texture1 && !particle.u_scroll && !particle.v_scroll);
    for (i = 0; i < 8300; ++i) multi_tex_particle_render_on_cell(&particle, 30, 40);
    memset(&particle, 0xdd, sizeof particle);
    assert(renderer.queues[RENDER_MULTITEXTURE].count == 8302);
    assert(first->tex0 == textures && first->tex1 == textures + 1 && !memcmp(saved, first->verts, sizeof saved));
    renderer_destroy_all_rp_list(&renderer);
    allocations = allocation_calls;
    multi_tex_particle_construct(&particle, origin, -1);
    memset(particle.uv0, 0, sizeof particle.uv0); memset(particle.uv1, 0, sizeof particle.uv1);
    multi_tex_particle_render_on_cell(&particle, 30, 40);
    assert(allocation_calls == allocations && renderer.queues[RENDER_MULTITEXTURE].count == 1);
    second = renderer.queues[RENDER_MULTITEXTURE].items[0].primitive;
    assert(!second->tex0 && !second->tex1 && !second->verts[0].u0 && !second->verts[0].v1);
    renderer_destroy_all_rp_list(&renderer); renderer_destruct(&renderer);
    assert(!outstanding && !g_renderer);
    puts("Multitexture particles: frame-owned quads, projection inputs, skipped cells, UVs, fog, repeated draws, particle reuse and pool growth passed.");
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
#endif

#ifndef RENDERER_BASELINE
#include "renderer_texture_owner_test.h"
#include "renderer_granny_projection_test.h"
#endif

int main(int argc, char **argv)
{
    (void)argv;
#ifndef RENDERER_BASELINE
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
#endif
    growth(); routing(); blend_stage_arguments(); render_trace();
    assert(!outstanding && atlas_starts && atlas_ends && frame_ends == effect_ends);
    if (argc == 1) puts("Renderer storage: C ownership, stable growth, reuse, every setup failure, all flag routes and real draw passes passed.");
    return 0;
}
