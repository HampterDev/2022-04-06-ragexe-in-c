/* The graphics device (render/gpu_device.c) through render/gfx.h, with the static world attached
   (render/gfx_world.h, render/gpu_world.c), on a real GPU, in a hidden window: what each draw puts
   on the screen, downloaded through gfx_read_frame, and the window claimed and presented into where
   the video driver lets it be shown without showing anything (check_present). Skipped (77) where
   SDL finds no GPU device. The argument is RAGEXE_MSAA: the test runs once without multisampling
   and once with it. */
#include "render/gfx.h"
#include "render/gfx_world.h"
#include "render/gpu_layer.h"
#include "render/screen_wobble.h"
#include "render/skin_layout.h"
#include "diag/perf_counters.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct perf_counters g_perf;
/* What the test keeps of the device it draws with: the hidden window, its size and the screen
   (gfx_create_screen). */
struct TestScreen {
    SDL_Window *window;
    int width, height;
    gfx_surface *screen;
};
static struct TestScreen g_screen;
static int multisampled;  /* RAGEXE_MSAA asks for more than one sample */

#define W 64
#define H 48

static DWORD screen_pixel(struct TestScreen *d, int x, int y)
{
    RECT r;
    DWORD p;
    (void)d;
    r.left = x;
    r.top = y;
    r.right = x + 1;
    r.bottom = y + 1;
    assert(gfx_read_frame(&r, &p, 4) == 0);
    return p & 0xffffff;
}

/* The whole screen's pixels, W by H, without their alpha, which is 255. */
static void read_screen(struct TestScreen *d, DWORD *out)
{
    int i;
    (void)d;
    assert(gfx_read_frame(NULL, out, W * 4) == 0);
    for (i = 0; i < W * H; i++) {
        assert(out[i] >> 24 == 0xff);
        out[i] &= 0xffffff;
    }
}

static void quad(struct TexDrawVertex *v, float x0, float y0, float x1, float y1, DWORD diffuse)
{
    int i;
    for (i = 0; i < 4; i++) {
        v[i].z = 0.5f;
        v[i].rhw = 1.0f;
        v[i].diffuse = diffuse;
    }
    v[0].x = x0 - 0.5f; v[0].y = y0 - 0.5f; v[0].u = 0.0f; v[0].v = 0.0f;
    v[1].x = x1 - 0.5f; v[1].y = y0 - 0.5f; v[1].u = 1.0f; v[1].v = 0.0f;
    v[2].x = x0 - 0.5f; v[2].y = y1 - 0.5f; v[2].u = 0.0f; v[2].v = 1.0f;
    v[3].x = x1 - 0.5f; v[3].y = y1 - 0.5f; v[3].u = 1.0f; v[3].v = 1.0f;
}

/* The modes these checks draw in (render/draw_modes.h): the texture as it is, by the nearest texel;
   the vertex colour; the texture times the vertex colour, filtered linearly, blended by its alpha
   (the interface's mode; what these checks draw with it is opaque). None of them touches depth. */
#define TEXTURE DRAW_BLIT
#define COLOUR DRAW_CLEAR_COLOUR
#define FILTERED DRAW_OVERLAY

static void fill_2x2(gfx_surface *t, int fmt, const DWORD argb[4])
{
    struct gfx_lock lk;
    int i;
    assert(gfx_lock(t, NULL, &lk) == 0);
    for (i = 0; i < 4; i++) {
        BYTE *p = (BYTE *)lk.bits + (i / 2) * lk.pitch + (i % 2) * (lk.bpp / 8);
        DWORD c = argb[i], a = c >> 24, r = (c >> 16) & 0xff, g = (c >> 8) & 0xff, b = c & 0xff;
        if (fmt == GFX_FMT_8888) {
            memcpy(p, &c, 4);
        } else {
            WORD w = fmt == GFX_FMT_565 ? (WORD)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
                   : fmt == GFX_FMT_4444 ? (WORD)(((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4))
                   : (WORD)(((a >> 7) << 15) | ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3));
            memcpy(p, &w, 2);
        }
    }
    gfx_unlock(t);
}

static gfx_surface *texture_2x2(int fmt, const DWORD argb[4])
{
    gfx_surface *t = gfx_create_texture(2, 2, fmt);
    assert(t);
    fill_2x2(t, fmt, argb);
    return t;
}

/* an n by n texture of one colour */
static gfx_surface *texture_square(int fmt, int n, DWORD argb)
{
    gfx_surface *t = gfx_create_texture(n, n, fmt);
    struct gfx_lock lk;
    int x, y;
    assert(t && fmt == GFX_FMT_8888 && gfx_lock(t, NULL, &lk) == 0);
    for (y = 0; y < n; y++)
        for (x = 0; x < n; x++)
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &argb, 4);
    gfx_unlock(t);
    return t;
}

static void draw_quad(float x0, float y0, float x1, float y1, DWORD diffuse)
{
    struct TexDrawVertex v[4];
    quad(v, x0, y0, x1, y1, diffuse);
    gfx_draw_vertices(v, 4, 1);
}

static void check_texture(struct TestScreen *d, int fmt)
{
    static const DWORD texels[4] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff };
    gfx_surface *t = texture_2x2(fmt, texels);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0, 1.0f, NULL);
    gfx_set_mode(TEXTURE);
    gfx_set_texture(0, t);
    draw_quad(0, 0, 32, 32, 0xffffffff);
    gfx_end_scene();
    assert(screen_pixel(d, 4, 4) == 0xff0000);
    assert(screen_pixel(d, 28, 4) == 0x00ff00);
    assert(screen_pixel(d, 4, 28) == 0x0000ff);
    assert(screen_pixel(d, 28, 28) == 0xffffff);
    assert(screen_pixel(d, 40, 40) == 0);
    gfx_set_texture(0, NULL);
    gfx_release(t);
}

/* A texture filtered linearly reads a texel's own colour at the pixel whose centre lands on the
   texel's centre, as Direct3D 9 places its pixel centres: drawn three pixels to a texel, pixels 1
   and 4 of the six, and a mix between them. Magnified, the filter is bilinear; at one texel per
   pixel, and where a pixel's samples lie on both of a quad's triangles when multisampling, the
   mode's anisotropic filtering is the driver's own (Mesa's lavapipe mixes in a few per cent of the
   neighbours there), so the pixels read lie on the diagonal the triangles do not share. */
static void check_texel_centres(struct TestScreen *d)
{
    static const DWORD texels[4] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, texels);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0, 1.0f, NULL);
    gfx_set_mode(FILTERED);
    gfx_set_texture(0, t);
    draw_quad(10, 10, 16, 16, 0xffffffff);
    gfx_end_scene();
    assert(screen_pixel(d, 11, 11) == 0xff0000);
    assert(screen_pixel(d, 14, 14) == 0xffffff);
    assert(screen_pixel(d, 12, 12) != 0xff0000 && screen_pixel(d, 12, 12) != 0xffffff);
    gfx_set_texture(0, NULL);
    gfx_release(t);
}

/* A view that looks down +z from the origin, a pixel per unit at depth 100 */
static void test_view(struct GfxWorldView *v)
{
    memset(v, 0, sizeof *v);
    v->view[0] = 1.0f;
    v->view[4] = 1.0f;
    v->view[8] = 1.0f;
    v->proj[0] = 100.0f;
    v->proj[1] = 100.0f;
    v->centre[0] = W / 2.0f;
    v->centre[1] = H / 2.0f;
    v->nearZ = 10.0f;
    v->depthScale = 1.0f;
}

/* The static world (render/gfx.h, gfx_world_*) these checks draw: its records by id, as
   render/static_world.h builds them from meshes and their runs. */
#define WORLD_RECORDS 64
static struct {
    struct SwLayout layout;
    struct SwRecord records[WORLD_RECORDS];
    size_t count;
    int arrays;
} g_world;

static void world_begin(void)
{
    gfx_world_clear();
    memset(&g_world, 0, sizeof g_world);
}

/* A texture into layer `layer` of array `array`, the array made from it where it is the next. */
static void world_texture(gfx_surface *t, uint8_t array, uint16_t layer)
{
    if (array == g_world.arrays) {
        assert(gfx_world_array(array, t));
        g_world.arrays++;
    }
    assert(gfx_world_texture(t, array, layer));
}

/* A mesh whose runs draw in transform slot `slot`: placed in the world by `placed` (NULL: as it
   is), or with `animated` moved by its slot from frame to frame. */
static void world_add(const struct GfxMeshVertex *mv, uint32_t nverts, const WORD *indices, uint32_t nindices,
                      const struct SwRun *runs, size_t nruns, uint16_t slot, const float *placed, int animated)
{
    struct SwPlacement place;
    struct SwMesh mesh;
    uint32_t ids[16];
    assert(nverts <= 16 && g_world.count + nruns <= WORLD_RECORDS);
    memset(&mesh, 0, sizeof mesh);
    mesh.positions = &mv[0].x;
    mesh.stride = sizeof *mv / sizeof(float);
    mesh.vertexCount = nverts;
    mesh.indices = indices;
    mesh.indexCount = nindices;
    mesh.transform = slot;
    mesh.animated = animated;
    mesh.placed = placed;
    assert(sw_layout_place(&g_world.layout, nverts, nindices, &place));
    assert(sw_records_build(&mesh, &place, runs, nruns, (uint32_t)g_world.count, g_world.records + g_world.count));
    assert(sw_vertex_ids(&mesh, runs, nruns, (uint32_t)g_world.count, ids));
    assert(gfx_world_mesh(&place, mv, ids, indices));
    g_world.count += nruns;
}

/* The records, in the order the passes draw them, and the slots: slot 0 the world, then `slots`. */
static void world_send(const struct GfxWorldSlot *slots, size_t nslots)
{
    struct SwRecord sorted[WORLD_RECORDS];
    struct GfxWorldSlot all[4];
    assert(nslots < 4);
    memcpy(sorted, g_world.records, g_world.count * sizeof *sorted);
    sw_records_sort(sorted, g_world.count);
    assert(gfx_world_records(sorted, g_world.count, (uint32_t)g_world.count));
    memset(all, 0, sizeof all);
    all[0].local[0] = all[0].local[4] = all[0].local[8] = 1.0f;
    all[0].parent = all[0].animated = -1;
    if (nslots)
        memcpy(all + 1, slots, nslots * sizeof *slots);
    assert(gfx_world_slots(all, 0, nslots + 1));
}

/* The frame (its matrices and waves as the caller set them) seen through `view`, which also draws
   the water. */
static void world_frame(const struct GfxWorldView *view, struct GfxWorldFrame *frame)
{
    struct SwProjection p;
    frame->camera = *view;
    p.proj[0] = view->proj[0];
    p.proj[1] = view->proj[1];
    p.centre[0] = view->centre[0];
    p.centre[1] = view->centre[1];
    p.size[0] = W;
    p.size[1] = H;
    p.nearZ = view->nearZ;
    p.farZ = 1e6f;
    sw_frustum(&frame->frustum, &p, view->view);
    gfx_world_frame(frame);
    gfx_set_world_camera(view);
}

/* A run of a mesh's indices in the models' opaque pass with clamped coordinates, without a texture
   or texture-coordinate matrix, culled by `flags` (SW_CULL_*). */
static struct SwRun model_run(uint32_t first, uint32_t count, int flags)
{
    struct SwRun r;
    memset(&r, 0, sizeof r);
    r.first = first;
    r.count = count;
    r.uv = SW_NONE;
    r.textureArray = r.lightmapArray = SW_NO_ARRAY;
    r.pass = SW_PASS_OPAQUE_CLAMP;
    r.flags = (uint8_t)flags;
    return r;
}

/* A model's draw, white where its texture is white. */
static void model_draw(struct GfxWorldDraw *draw)
{
    memset(draw, 0, sizeof *draw);
    draw->kind = GFX_MESH_MODEL;
    draw->colour[0] = draw->colour[1] = draw->colour[2] = draw->colour[3] = 1.0f;
}

/* Direct3D 9 culls by the winding on the screen: a record's flags drop the triangles whose corners
   go clockwise there (y down), with SW_CULL_CCW the others, with SW_CULL_NONE none; quads are never
   culled. A record drawn by itself and its pass culled by the GPU cull alike. */
static void check_culling(struct TestScreen *d)
{
    /* at depth 100 a unit is a pixel, the view's centre at (32, 24) */
    static const float corners[6][2] = { { 4, 4 }, { 20, 4 }, { 4, 20 }, { 30, 4 }, { 30, 20 }, { 46, 4 } };
    static const WORD indices[6] = { 0, 1, 2, 3, 4, 5 };
    static const int flags[3] = { SW_CULL_NONE, 0, SW_CULL_CCW };
    struct GfxMeshVertex mv[6];
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct GfxWorldDraw draw;
    struct SwRun run;
    int c, way, i;
    memset(mv, 0, sizeof mv);
    for (i = 0; i < 6; i++) {
        mv[i].x = corners[i][0] - W / 2.0f;
        mv[i].y = corners[i][1] - H / 2.0f;
        mv[i].z = 100.0f;
        mv[i].diffuse = 0xffffffff;
    }
    test_view(&view);
    model_draw(&draw);
    for (c = 0; c < 3; c++) {
        world_begin();
        run = model_run(0, 6, flags[c]);
        world_add(mv, 6, indices, 6, &run, 1, 0, NULL, 0);
        world_send(NULL, 0);
        for (way = 0; way < 2; way++) {
            gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0, 1.0f, NULL);
            memset(&frame, 0, sizeof frame);
            world_frame(&view, &frame);
            gfx_set_mode(DRAW_OPAQUE_CLAMP);
            gfx_set_texture(0, NULL);
            if (way)
                gfx_world_draw(SW_PASS_OPAQUE_CLAMP, &draw);
            else
                gfx_world_draw_record(&g_world.records[0], &draw);
            gfx_end_scene();
            assert((screen_pixel(d, 7, 7) != 0) == (flags[c] != 0));
            assert((screen_pixel(d, 33, 7) != 0) == (flags[c] != SW_CULL_CCW));
        }
    }
}

/* A texture changed after a draw used it keeps its old texels for that draw and gives the new
   ones to the draws after the change, in one frame. */
static void check_texture_changed_in_frame(struct TestScreen *d)
{
    static const DWORD red[4] = { 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000 };
    static const DWORD green[4] = { 0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00 };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, red);
    gfx_clear(GFX_CLEAR_TARGET, 0, 1.0f, NULL);
    gfx_set_mode(TEXTURE);
    gfx_set_texture(0, t);
    draw_quad(0, 0, 8, 8, 0xffffffff);
    gfx_end_scene();
    fill_2x2(t, GFX_FMT_8888, green);
    gfx_set_texture(0, NULL);
    gfx_set_texture(0, t);
    draw_quad(16, 0, 24, 8, 0xffffffff);
    gfx_end_scene();
    assert(screen_pixel(d, 4, 4) == 0xff0000);
    assert(screen_pixel(d, 20, 4) == 0x00ff00);
    gfx_set_texture(0, NULL);
    gfx_release(t);
}

/* A texture released while a recorded draw still uses it is drawn, then freed. */
static void check_release_in_frame(struct TestScreen *d)
{
    static const DWORD blue[4] = { 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff };
    gfx_surface *t = texture_2x2(GFX_FMT_565, blue);
    gfx_clear(GFX_CLEAR_TARGET, 0, 1.0f, NULL);
    gfx_set_mode(TEXTURE);
    gfx_set_texture(0, t);
    draw_quad(0, 0, 8, 8, 0xffffffff);
    gfx_end_scene();
    gfx_set_texture(0, NULL);
    gfx_release(t);
    assert(screen_pixel(d, 4, 4) == 0x0000ff);
}

/* A texture held twice (as the renderer's queues hold the textures their draws name,
   render/rp_pool.c) and released by its owner is still drawn after its release, and goes when the
   holds are let go; a texture released while another is held goes at once, after its draw. */
static void check_release_while_held(struct TestScreen *d)
{
    static const DWORD green[4] = { 0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00 };
    static const DWORD white[4] = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, green), *u;
    gfx_clear(GFX_CLEAR_TARGET, 0, 1.0f, NULL);
    gfx_hold(t);
    gfx_hold(t);
    gfx_release(t);
    gfx_set_mode(TEXTURE);
    gfx_set_texture(0, t);
    draw_quad(0, 0, 8, 8, 0xffffffff);
    gfx_set_texture(0, NULL);
    u = texture_2x2(GFX_FMT_8888, white);
    gfx_set_texture(0, u);
    draw_quad(16, 0, 24, 8, 0xffffffff);
    gfx_set_texture(0, NULL);
    gfx_release(u);
    gfx_release_holds();
    gfx_end_scene();
    assert(screen_pixel(d, 4, 4) == 0x00ff00);
    assert(screen_pixel(d, 20, 4) == 0xffffff);
}

/* Copies between targets stay on the GPU: a render target's rectangle into the screen, a copy into
   a texture refused, rectangles outside their surfaces copying nothing, and the frame saved into a
   render target and put back, under the draws after it (c3d_device_save_frame and
   c3d_device_restore_frame). */
static void check_target_copies(struct TestScreen *d)
{
    gfx_surface *rt = gfx_create_render_target(16, 16), *tex = gfx_create_texture(16, 16, GFX_FMT_8888);
    gfx_surface *save = gfx_create_render_target(W, H);
    RECT r = { 0, 0, 16, 16 }, into = { 20, 10, 36, 26 };
    assert(rt && tex && save);
    assert(gfx_set_render_target(rt) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    draw_quad(0, 8, 16, 16, 0xff00ff00);
    gfx_end_scene();
    assert(gfx_set_render_target(d->screen) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff0000ff, 1.0f, NULL);
    assert(gfx_blit(d->screen, &into, rt, &r) == 0);
    assert(gfx_blit(tex, &r, rt, &r) < 0);
    gfx_end_scene();
    assert(screen_pixel(d, 22, 12) == 0 && screen_pixel(d, 22, 22) == 0x00ff00 && screen_pixel(d, 19, 22) == 0x0000ff
           && screen_pixel(d, 36, 22) == 0x0000ff);
    /* rectangles outside their surfaces copy nothing */
    {
        RECT outside = { 8, 8, 17, 16 }, negative = { -1, 0, 4, 4 }, empty = { 4, 4, 4, 8 }, beyond = { 60, 40, 70, 48 };
        assert(gfx_blit(rt, &outside, d->screen, NULL) == 0);
        assert(gfx_blit(d->screen, NULL, rt, &negative) == 0);
        assert(gfx_blit(d->screen, &empty, rt, &empty) == 0);
        assert(gfx_blit(d->screen, &beyond, rt, NULL) == 0);
        gfx_end_scene();
        assert(screen_pixel(d, 22, 22) == 0x00ff00 && screen_pixel(d, 62, 42) == 0x0000ff);
    }
    gfx_release(rt);
    gfx_release(tex);

    gfx_clear(GFX_CLEAR_TARGET, 0xff203040, 1.0f, NULL);
    assert(gfx_blit(save, NULL, d->screen, NULL) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    assert(screen_pixel(d, 30, 30) == 0);
    assert(gfx_blit(d->screen, NULL, save, NULL) == 0);
    draw_quad(0, 0, 4, 4, 0xffffffff);
    gfx_end_scene();
    assert(screen_pixel(d, 30, 30) == 0x203040);
    assert(screen_pixel(d, 2, 2) == 0xffffff);
    gfx_release(save);
}

/* A target of odd size drawn enlarged with linear filtering and clamped addressing reads its own
   last row and column at its edges, nothing past them: its texture is wider than it. */
static void check_target_edges(struct TestScreen *d)
{
    gfx_surface *rt = gfx_create_render_target(15, 9);
    assert(rt);
    assert(gfx_set_render_target(rt) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xffff0000, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    draw_quad(0, 0, 15, 9, 0xff00ff00);
    gfx_end_scene();
    assert(gfx_set_render_target(d->screen) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gfx_set_mode(FILTERED);
    gfx_set_texture(0, rt);
    draw_quad(0, 0, 45, 27, 0xffffffff);
    gfx_end_scene();
    assert(screen_pixel(d, 44, 13) == 0x00ff00 && screen_pixel(d, 22, 26) == 0x00ff00
           && screen_pixel(d, 44, 26) == 0x00ff00 && screen_pixel(d, 1, 1) == 0x00ff00);
    gfx_set_texture(0, NULL);
    gfx_release(rt);
}

/* The screen's picture is read, a rectangle of it or the whole, with what was drawn before; a
   rectangle outside it, an empty one and a pitch narrower than the rows read nothing, and a target
   locks nothing: its pixels are the GPU's. */
static void check_read_frame(struct TestScreen *d)
{
    static DWORD all[W * H];
    DWORD part[4 * 10];
    RECT r = { 20, 10, 30, 14 }, outside = { 60, 40, 65, 44 }, empty = { 5, 5, 5, 9 };
    struct gfx_lock lk;
    int x, y;
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    draw_quad(0, 0, W, H, 0xff0000ff);
    draw_quad(29, 13, 30, 14, 0xff654321);
    assert(gfx_read_frame(&outside, part, 40) < 0 && gfx_read_frame(&empty, part, 40) < 0);
    assert(gfx_read_frame(&r, part, 36) < 0);
    assert(gfx_read_frame(&r, part, 40) == 0);
    for (y = 0; y < 4; y++)
        for (x = 0; x < 10; x++)
            assert((part[y * 10 + x] & 0xffffff) == (x == 9 && y == 3 ? 0x654321u : 0x0000ffu));
    read_screen(d, all);
    assert(all[13 * W + 29] == 0x654321 && all[13 * W + 28] == 0x0000ff && all[W + 1] == 0x0000ff);
    assert(gfx_lock(d->screen, NULL, &lk) < 0);
}

/* A clear of a rectangle after draws in the same pass, of the colour and of the depth alone. */
static void check_clears(struct TestScreen *d)
{
    RECT r = { 8, 8, 16, 16 };
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    draw_quad(0, 0, 32, 32, 0xffffffff);
    gfx_clear(GFX_CLEAR_TARGET, 0xff0000ff, 1.0f, &r);
    gfx_end_scene();
    assert(screen_pixel(d, 12, 12) == 0x0000ff && screen_pixel(d, 7, 12) == 0xffffff && screen_pixel(d, 16, 12) == 0xffffff);
    /* depth: a near quad, the depth cleared under part of it, then a far one */
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    gfx_set_texture(0, NULL);
    {
        struct TexDrawVertex v[4];
        quad(v, 0, 0, 32, 32, 0xffff0000);
        gfx_draw_vertices(v, 4, 1);
        gfx_clear(GFX_CLEAR_ZBUFFER, 0, 1.0f, &r);
        quad(v, 0, 0, 32, 32, 0xff00ff00);
        v[0].z = v[1].z = v[2].z = v[3].z = 0.75f;
        gfx_draw_vertices(v, 4, 1);
    }
    gfx_end_scene();
    assert(screen_pixel(d, 12, 12) == 0x00ff00 && screen_pixel(d, 4, 4) == 0xff0000);
}

/* Mipmaps are made on the GPU from the top level whenever the texels go up: a black and white
   checker drawn at half its size with a mip filter reads the grey of the level below, and again
   after the texels change. */
static void check_mipmaps(struct TestScreen *d)
{
    gfx_surface *t;
    struct gfx_lock lk;
    int x, y, pass;
    DWORD p;
    gfx_set_mipmaps(1);
    t = gfx_create_texture(8, 8, GFX_FMT_8888);
    gfx_set_mipmaps(0);
    assert(t);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    for (pass = 0; pass < 2; pass++) {
        DWORD dark = pass ? 0xff0000ffu : 0xff000000u;
        assert(gfx_lock(t, NULL, &lk) == 0);
        for (y = 0; y < 8; y++)
            for (x = 0; x < 8; x++) {
                DWORD c = (x + y) & 1 ? 0xffffffffu : dark;
                memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &c, 4);
            }
        gfx_unlock(t);
        gfx_refresh_mips(t, NULL);
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
        gfx_set_texture(0, t);
        draw_quad(0, 0, 4, 4, 0xffffffff);
        gfx_end_scene();
        p = screen_pixel(d, 1, 1);
        if (pass == 0)
            assert((p & 0xff) >= 0x7e && (p & 0xff) <= 0x81 && p == (p & 0xff) * 0x010101u);
        else
            assert((p & 0xff) == 0xff && (p >> 8 & 0xff) >= 0x7e && (p >> 8 & 0xff) <= 0x81);
    }
    gfx_set_texture(0, NULL);
    gfx_release(t);
}

/* A texture of distinct texels, w by h, made layered or not: texel (x, y) gets colour hash of
   salt, x and y, opaque or, where `alpha`, half of them see-through. */
static gfx_surface *pattern_texture(int layered, int w, int h, unsigned salt, int alpha)
{
    gfx_surface *t = layered ? gfx_create_layer_texture(w, h, GFX_FMT_8888) : gfx_create_texture(w, h, GFX_FMT_8888);
    struct gfx_lock lk;
    int x, y;
    assert(t && gfx_lock(t, NULL, &lk) == 0);
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            DWORD c = (DWORD)(x * 73856093u ^ y * 19349663u ^ salt * 83492791u);
            c = (c * 2654435761u) >> 8 | (alpha && (x ^ y) & 1 ? 0 : 0xff000000u);
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &c, 4);
        }
    gfx_unlock(t);
    return t;
}

/* The quad of a texture drawn at (x, y), w by h pixels, in the current mode */
static void texture_quad(gfx_surface *t, float x, float y, float w, float h)
{
    struct TexDrawVertex v[4];
    gfx_set_texture(0, t);
    quad(v, x, y, x + w, y + h, 0xffffffff);
    gfx_draw_vertices(v, 4, 1);
}

/* Layered textures (gfx_create_layer_texture) read as the same texels in plain ones do, pixel for
   pixel, by the nearest texel and filtered, enlarged and shrunk, with mipmaps too; quads of several
   layers of one array are one draw; a full array leaves the next texture to a second one; a layer
   given up is taken by the next texture, which reads its own texels; large textures share an array
   with fewer others, the largest with none. */
static void check_layers(struct TestScreen *d)
{
    static DWORD a[W * H], b[W * H];
    gfx_surface *layer[3], *plain[3];
    unsigned draws;
    int i, mode, mipmaps;
    for (mipmaps = 0; mipmaps < 2; mipmaps++) {
        gfx_set_mipmaps(mipmaps);
        for (i = 0; i < 3; i++) {
            layer[i] = pattern_texture(1, 16, 16, (unsigned)i, i == 2);
            plain[i] = pattern_texture(0, 16, 16, (unsigned)i, i == 2);
        }
        gfx_set_mipmaps(0);
        for (mode = 0; mode < 3; mode++) {
            static const int modes[3] = { TEXTURE, FILTERED, DRAW_ALPHA_CLAMP };
            int k;
            for (k = 0; k < 2; k++) {
                gfx_surface **set = k ? plain : layer;
                gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
                gfx_set_mode(modes[mode]);
                draws = g_perf.draws;
                /* enlarged, one texel to a pixel, shrunk to a half, a quarter and an eighth */
                texture_quad(set[0], 0.0f, 0.0f, 37.5f, 29.25f);
                texture_quad(set[1], 38.0f, 0.0f, 16.0f, 16.0f);
                texture_quad(set[2], 38.0f, 17.0f, 8.0f, 8.0f);
                texture_quad(set[0], 47.0f, 17.0f, 4.0f, 4.0f);
                texture_quad(set[1], 52.0f, 17.0f, 2.0f, 2.0f);
                texture_quad(set[2], 1.25f, 30.5f, 61.0f, 17.0f);
                gfx_end_scene();
                read_screen(d, k ? b : a);
                /* the plain textures make a draw each; the layers of one array one */
                assert(g_perf.draws - draws == (k ? 6u : 1u));
            }
            assert(!memcmp(a, b, sizeof a));
        }
        gfx_set_texture(0, NULL);
        for (i = 0; i < 3; i++) {
            gfx_release(layer[i]);
            gfx_release(plain[i]);
        }
    }
    /* an array full (64 textures of 16 by 16): the next is a layer of a second array, read as
       its plain twin, and a quad of each array is a draw of its own */
    {
        static gfx_surface *many[65];
        gfx_surface *twin = pattern_texture(0, 16, 16, 64, 0);
        int side;
        for (i = 0; i < 65; i++)
            many[i] = pattern_texture(1, 16, 16, (unsigned)i, 0);
        for (side = 0; side < 2; side++) {
            gfx_set_mode(TEXTURE);
            gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
            draws = g_perf.draws;
            for (i = 0; i < 65; i++)
                texture_quad(side && i == 64 ? twin : many[i], (float)(i % 8 * 8), (float)(i / 8 * 5), 8, 5);
            gfx_end_scene();
            read_screen(d, side ? b : a);
            /* the first 64 share one array; the 65th is another (the plain twin a draw of its own) */
            assert(g_perf.draws - draws == 2u);
        }
        assert(!memcmp(a, b, sizeof a));
        gfx_set_texture(0, NULL);
        for (i = 0; i < 65; i++)
            gfx_release(many[i]);
        gfx_release(twin);
    }
    /* a full array whose layer is given up after the frame that read it: the next layered texture of
       its size takes that layer, so that the 64 are one draw again, each with its own texels */
    {
        static gfx_surface *many[64], *twins[64];
        int side;
        for (i = 0; i < 64; i++)
            many[i] = pattern_texture(1, 16, 16, (unsigned)i, 0);
        gfx_set_mode(TEXTURE);
        gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
        for (i = 0; i < 64; i++)
            texture_quad(many[i], (float)(i % 8 * 8), (float)(i / 8 * 5), 8, 5);
        gfx_end_scene();
        read_screen(d, a);  /* the frame goes to the GPU: nothing recorded holds the layer now */
        gfx_set_texture(0, NULL);
        gfx_release(many[37]);
        many[37] = pattern_texture(1, 16, 16, 100, 0);
        for (i = 0; i < 64; i++)
            twins[i] = pattern_texture(0, 16, 16, i == 37 ? 100u : (unsigned)i, 0);
        for (side = 0; side < 2; side++) {
            gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
            draws = g_perf.draws;
            for (i = 0; i < 64; i++)
                texture_quad(side ? twins[i] : many[i], (float)(i % 8 * 8), (float)(i / 8 * 5), 8, 5);
            gfx_end_scene();
            read_screen(d, side ? b : a);
            assert(g_perf.draws - draws == (side ? 64u : 1u));
        }
        assert(!memcmp(a, b, sizeof a));
        gfx_set_texture(0, NULL);
        for (i = 0; i < 64; i++) {
            gfx_release(many[i]);
            gfx_release(twins[i]);
        }
    }
    /* an array holds as many layers as make 16 MB: four of 1024 by 1024, one of 2048 by 2048 */
    {
        static const int sizes[2] = { 1024, 2048 }, expected[2] = { 2, 3 };
        gfx_surface *big[5];
        int k;
        for (k = 0; k < 2; k++) {
            int n = k ? 3 : 5;
            for (i = 0; i < n; i++)
                big[i] = pattern_texture(1, sizes[k], sizes[k], (unsigned)i, 0);
            gfx_set_mode(TEXTURE);
            gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
            draws = g_perf.draws;
            for (i = 0; i < n; i++)
                texture_quad(big[i], (float)(i * 8), 0, 8, 8);
            gfx_end_scene();
            assert(g_perf.draws - draws == (unsigned)expected[k]);
            gfx_set_texture(0, NULL);
            for (i = 0; i < n; i++)
                gfx_release(big[i]);
        }
    }
}

#define PART 64

/* Texel (x, y) of a texture of PART by PART texels, as the device holds level 0 of it */
static DWORD *texel_at(DWORD *level, int x, int y)
{
    return &level[y * PART + x];
}

#define BEHIND 0xff123456u

/* The cell r of t locked and filled with texels of `salt`, and likewise in cpu, the texture's
   texels as the CPU holds them; with `behind`, texel (0, 0) and row 30 changed too, outside the
   lock, where the device is not told of them. */
static void write_cell(gfx_surface *t, DWORD *cpu, LONG left, LONG top, LONG right, LONG bottom, unsigned salt,
                       int behind)
{
    RECT r;
    struct gfx_lock lk;
    BYTE *base;
    int x, y;
    r.left = left;
    r.top = top;
    r.right = right;
    r.bottom = bottom;
    assert(gfx_lock(t, &r, &lk) == 0 && lk.pitch == PART * 4);
    base = (BYTE *)lk.bits - (size_t)top * lk.pitch - (size_t)left * 4;
    for (y = top; y < bottom; y++)
        for (x = left; x < right; x++) {
            *texel_at(cpu, x, y) = 0xff000000u | (DWORD)(x * 2654435761u ^ y * 40503u ^ salt * 83492791u) >> 8;
            memcpy(base + (size_t)y * lk.pitch + (size_t)x * 4, texel_at(cpu, x, y), 4);
        }
    if (behind) {
        *texel_at(cpu, 0, 0) = BEHIND;
        for (x = 0; x < PART; x++)
            *texel_at(cpu, x, 30) = BEHIND;
        memcpy(base, texel_at(cpu, 0, 0), 4);
        memcpy(base + 30 * (size_t)lk.pitch, texel_at(cpu, 0, 30), PART * 4);
    }
    gfx_unlock(t);
}

/* t drawn and the frame sent, so that what it holds goes up */
static void send_texture(struct TestScreen *d, gfx_surface *t)
{
    static DWORD screen[W * H];
    gfx_set_mode(TEXTURE);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    texture_quad(t, 0.0f, 0.0f, 8.0f, 8.0f);
    gfx_end_scene();
    read_screen(d, screen);
    gfx_set_texture(0, NULL);
}

/* t's lower levels as the device holds them are those of a plain texture of its size and levels
   whose top level is `top`, made whole */
static void same_levels(struct TestScreen *d, gfx_surface *t, const DWORD *top)
{
    static DWORD level[2][PART * PART];
    struct gfx_lock lk;
    gfx_surface *twin;
    int k;
    if (gfx_get_levels(t) <= 1)
        return;
    gfx_set_mipmaps(1);
    twin = gfx_create_texture(PART, PART, GFX_FMT_8888);
    gfx_set_mipmaps(0);
    assert(twin && gfx_get_levels(twin) == gfx_get_levels(t) && gfx_lock(twin, NULL, &lk) == 0);
    memcpy(lk.bits, top, PART * PART * 4);
    gfx_unlock(twin);
    send_texture(d, twin);
    for (k = 1; k < gfx_get_levels(t); k++) {
        assert(gfx_read_texture_level(t, k, level[0]) == 0 && gfx_read_texture_level(twin, k, level[1]) == 0);
        assert(!memcmp(level[0], level[1], (size_t)(PART >> k) * (size_t)(PART >> k) * 4));
    }
    gfx_release(twin);
}

/* Only the texels written go up: a texture sent once, then two cells far apart written through
   locks of their own while texels outside them change behind the device's back, keeps those
   texels as they were on the GPU and takes the cells' new ones; plain and layered, with and
   without mipmaps, whose lower levels are those a plain texture made whole from the same top level
   gets. Nine cells written apart, more rectangles than the device keeps, all go up too. */
static void check_partial_uploads(struct TestScreen *d)
{
    static DWORD before[PART * PART], after[PART * PART], cpu[PART * PART], level[PART * PART];
    int layered, mipmaps, i, x, y;
    for (layered = 0; layered < 2; layered++)
        for (mipmaps = 0; mipmaps < 2; mipmaps++) {
            gfx_surface *t;
            gfx_set_mipmaps(mipmaps);
            t = pattern_texture(layered, PART, PART, 7, 0);
            gfx_set_mipmaps(0);
            assert((gfx_get_levels(t) > 1) == mipmaps);
            send_texture(d, t);
            assert(gfx_read_texture_level(t, 0, before) == 0);
            memcpy(cpu, before, sizeof cpu);
            write_cell(t, cpu, 4, 4, 12, 12, 1, 1);
            write_cell(t, cpu, 40, 44, 56, 60, 2, 0);
            send_texture(d, t);
            assert(gfx_read_texture_level(t, 0, after) == 0);
            for (y = 0; y < PART; y++)
                for (x = 0; x < PART; x++) {
                    int cell = (x >= 4 && x < 12 && y >= 4 && y < 12) || (x >= 40 && x < 56 && y >= 44 && y < 60);
                    assert(*texel_at(after, x, y) == *texel_at(cell ? cpu : before, x, y));
                }
            assert(*texel_at(cpu, 0, 0) != *texel_at(after, 0, 0) && *texel_at(cpu, 5, 30) != *texel_at(after, 5, 30));
            same_levels(d, t, after);
            /* nine cells apart, one more than the rectangles the device keeps apart: all go up; the
               texels changed behind its back stay as they were but where a cell or the rectangle
               two cells became takes them in */
            for (i = 0; i < 9; i++)
                write_cell(t, cpu, i % 3 * 24, i / 3 * 24, i % 3 * 24 + 4, i / 3 * 24 + 4, 10 + (unsigned)i, 0);
            send_texture(d, t);
            assert(gfx_read_texture_level(t, 0, level) == 0);
            for (y = 0; y < PART; y++)
                for (x = 0; x < PART; x++)
                    assert(*texel_at(level, x, y) == *texel_at(cpu, x, y)
                           || (y == 30 && *texel_at(level, x, y) == *texel_at(after, x, y)));
            gfx_release(t);
        }
}

/* A texture not yet sent goes up as what was written on it, its other texels cleared on the GPU to
   the texel its zero pixels widen to: a cell written through a lock of its own, texels changed
   outside it behind the device's back staying cleared; plain and layered, with and without
   mipmaps, whose lower levels are a plain texture's made whole from the same top level; and a 565
   texture, whose zero pixels widen to opaque black. */
static void check_fresh_uploads(struct TestScreen *d)
{
    static DWORD gpu[PART * PART], cpu[PART * PART];
    struct gfx_lock lk;
    gfx_surface *t;
    RECT r;
    int layered, mipmaps, x, y;
    for (layered = 0; layered < 2; layered++)
        for (mipmaps = 0; mipmaps < 2; mipmaps++) {
            gfx_set_mipmaps(mipmaps);
            t = layered ? gfx_create_layer_texture(PART, PART, GFX_FMT_8888)
                        : gfx_create_texture(PART, PART, GFX_FMT_8888);
            gfx_set_mipmaps(0);
            assert(t);
            memset(cpu, 0, sizeof cpu);
            write_cell(t, cpu, 20, 8, 36, 24, 3, 1);
            send_texture(d, t);
            assert(gfx_read_texture_level(t, 0, gpu) == 0);
            for (y = 0; y < PART; y++)
                for (x = 0; x < PART; x++)
                    assert(*texel_at(gpu, x, y) == (x >= 20 && x < 36 && y >= 8 && y < 24 ? *texel_at(cpu, x, y) : 0u));
            same_levels(d, t, gpu);
            gfx_release(t);
        }
    t = gfx_create_texture(PART, PART, GFX_FMT_565);
    r.left = r.top = 8;
    r.right = r.bottom = 16;
    assert(t && gfx_lock(t, &r, &lk) == 0 && lk.bpp == 16);
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++)
            ((WORD *)((BYTE *)lk.bits + (size_t)y * lk.pitch))[x] = 0xf800;
    gfx_unlock(t);
    send_texture(d, t);
    assert(gfx_read_texture_level(t, 0, gpu) == 0);
    for (y = 0; y < PART; y++)
        for (x = 0; x < PART; x++)
            assert(*texel_at(gpu, x, y) == (x >= 8 && x < 16 && y >= 8 && y < 16 ? 0xffff0000u : 0xff000000u));
    gfx_release(t);
}

/* A list of triangles draws each of them, flat or not, and nothing of the quad they would close. */
static void check_triangles(struct TestScreen *d)
{
    struct TexDrawVertex v[6];
    int i, flat;
    for (flat = 0; flat < 2; flat++) {
        struct TexDrawVertex second[4];
        quad(v, 0, 0, 16, 16, 0xffff0000);
        quad(second, 20, 0, 36, 16, 0xff00ff00);
        v[3] = second[1];
        v[4] = second[3];
        v[5] = second[2];
        for (i = 0; i < 6; i++)
            v[i].rhw = flat ? 1.0f : 1.0f / (float)(1 + i);
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
        gfx_set_mode(COLOUR);
        gfx_set_texture(0, NULL);
        gfx_draw_vertices(v, 6, 0);
        gfx_end_scene();
        /* the first above the diagonal of its square, the second below the other one */
        assert(screen_pixel(d, 2, 2) == 0xff0000 && screen_pixel(d, 13, 13) == 0);
        assert(screen_pixel(d, 33, 13) == 0x00ff00 && screen_pixel(d, 22, 2) == 0);
    }
}

/* Granny's models (render/gfx.h, gfx_create_skin, gfx_draw_skin; render/skin_layout.h): a model of
   three meshes, a quad skinned on bone 0 (its second bone 1 with the weight 0) and the same quad 8
   rows down rigid on bone 1, then a triangle; at depth 100 through test_view a unit is a pixel, the
   view's centre at (32, 24). */
static gfx_skin *skin_model(void)
{
    struct SkinVertex v[11];
    uint32_t indices[15];
    static const uint32_t corner[6] = { 0, 1, 2, 2, 1, 3 };
    int i;
    memset(v, 0, sizeof v);
    for (i = 0; i < 8; i++) {
        v[i].position[0] = (i & 1) ? 0.0f - 0.5f : -8.0f - 0.5f;
        v[i].position[1] = ((i & 2) ? 0.0f - 0.5f : -8.0f - 0.5f) + (i >= 4 ? 8.0f : 0.0f);
        v[i].position[2] = 100.0f;
        v[i].normal[2] = -1.0f;
        v[i].weights[0] = 1.0f;
        v[i].bones[0] = (uint8_t)(i >= 4);
        v[i].bones[1] = 1;
        v[i].flags = i >= 4 ? SKIN_RIGID : 0;
    }
    /* a triangle right of the first quad, on bones 0 and 1 by halves */
    for (i = 8; i < 11; i++) {
        v[i].position[0] = 8.0f + (i == 9 ? 8.0f : 0.0f) - 0.5f;
        v[i].position[1] = (i == 10 ? 8.0f : 0.0f) - 8.5f;
        v[i].position[2] = 100.0f;
        v[i].normal[2] = -1.0f;
        v[i].weights[0] = v[i].weights[1] = 0.5f;
        v[i].bones[1] = 1;
    }
    for (i = 0; i < 12; i++)
        indices[i] = corner[i % 6] + (i >= 6 ? 4 : 0);
    indices[12] = 8;
    indices[13] = 9;
    indices[14] = 10;
    return gfx_create_skin(v, 11, indices, 15);
}

/* A pose of two bones, bone 1 moved by `move` pixels across, the model where it is, lit straight
   on (b = scale + offset), its colour `add` added. */
static int skin_pose(float move, float scale, float offset, uint32_t add, int still)
{
    float bones[2][16], rows[SKIN_POSE_HEADER + 6][4];
    struct SkinPose pose;
    int b;
    memset(bones, 0, sizeof bones);
    memset(&pose, 0, sizeof pose);
    for (b = 0; b < 2; b++)
        bones[b][0] = bones[b][5] = bones[b][10] = bones[b][15] = 1.0f;
    bones[1][12] = move;
    pose.bones = &bones[0][0];
    pose.boneCount = 2;
    /* test_view's: the view where the meshes are, its projection */
    pose.view[0] = pose.view[4] = pose.view[8] = 1.0f;
    pose.proj[0] = pose.proj[1] = 100.0f;
    pose.centre[0] = W / 2.0f;
    pose.centre[1] = H / 2.0f;
    pose.nearZ = 10.0f;
    pose.depthScale = 1.0f;
    pose.light[2] = -1.0f;
    pose.scale = scale;
    pose.offset = offset;
    pose.add = add;
    pose.alpha = 0xff;
    pose.rigidStill = still;
    assert(skin_pose_rows(&pose, rows) == skin_pose_size(2));
    return gfx_skin_pose((const float (*)[4])rows, skin_pose_size(2));
}

/* Granny's meshes land where the CPU put their corners and take its colours, in one draw where they
   run on in the model or are one range in several poses, both faces of every triangle; a rigid
   mesh moves with its bone, or lies still in a pose that says so; a bone a vertex weights by 0
   moves it not even where it is not a number; a mode that draws no meshes, or a render target,
   takes none; a model given up after its draws were recorded is drawn and goes, and so is one held
   (gfx_hold_skin, as the renderer's queues hold the models their draws name) and given up by its
   owner before its draw, which goes when the holds are let go. */
static void check_skin(struct TestScreen *d)
{
    gfx_skin *model = skin_model(), *other = skin_model();
    gfx_surface *rt;
    unsigned draws;
    int lit, still, two;
    assert(model && other);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    gfx_set_texture(0, NULL);
    /* b = 128: blue 128, green 128 + 0x40, red 128 + 0x80 modulo 256 */
    lit = skin_pose(16.0f, 100.0f, 28.5f, 0x804000, 0);
    assert(lit >= 0);
    draws = g_perf.draws;
    gfx_draw_skin(model, 0, 6, lit);
    gfx_draw_skin(model, 6, 6, lit);
    gfx_draw_skin(model, 12, 3, lit);
    gfx_end_scene();
    assert(g_perf.draws == draws + 1);
    /* the quad on bone 0: x 24..31, y 16..23; the rigid one moved 16 across: 40..47, 24..31 */
    assert(screen_pixel(d, 25, 17) == 0x00c080 && screen_pixel(d, 30, 22) == 0x00c080);
    assert(screen_pixel(d, 41, 25) == 0x00c080 && screen_pixel(d, 46, 30) == 0x00c080);
    assert(screen_pixel(d, 22, 17) == 0 && screen_pixel(d, 33, 22) == 0 && screen_pixel(d, 30, 26) == 0);
    /* the triangle, by halves of bones 0 and 1 moved 8 across: the upper left half of x 48 to 55, y 16
       to 23 */
    assert(screen_pixel(d, 50, 17) == 0x00c080 && screen_pixel(d, 50, 22) == 0);
    /* the same range in a second pose: one draw of two instances; the rigid quad lying still in the
       first (x 24..31, white), on its bone moved 16 across in the second (x 40..47) */
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    still = skin_pose(16.0f, 0.0f, 255.0f, 0, 1);
    two = skin_pose(16.0f, 0.0f, 64.0f, 0, 0);
    assert(still >= 0 && two >= 0);
    draws = g_perf.draws;
    gfx_draw_skin(model, 6, 6, still);
    gfx_draw_skin(model, 6, 6, two);
    gfx_end_scene();
    assert(g_perf.draws == draws + 1);
    assert(screen_pixel(d, 25, 25) == 0xffffff && screen_pixel(d, 41, 25) == 0x404040);
    /* bone 1 not a number: the skinned quad, which weights it by 0, where it was; the triangle,
       which weights it by a half, nowhere */
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    two = skin_pose(NAN, 0.0f, 255.0f, 0, 0);
    assert(two >= 0);
    gfx_draw_skin(model, 0, 6, two);
    gfx_draw_skin(model, 12, 3, two);
    gfx_end_scene();
    assert(screen_pixel(d, 25, 17) == 0xffffff && screen_pixel(d, 30, 22) == 0xffffff);
    assert(screen_pixel(d, 50, 17) == 0);
    /* another model, a mode without meshes, a render target: draws apart, or none */
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    lit = skin_pose(16.0f, 100.0f, 28.5f, 0x804000, 0);
    draws = g_perf.draws;
    gfx_draw_skin(model, 0, 6, lit);
    gfx_draw_skin(other, 6, 6, lit);
    gfx_set_mode(DRAW_OVERLAY);
    gfx_draw_skin(model, 12, 3, lit);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    rt = gfx_create_render_target(16, 16);
    assert(rt && gfx_set_render_target(rt) == 0);
    gfx_draw_skin(model, 12, 3, lit);
    assert(gfx_set_render_target(d->screen) == 0);
    gfx_release(rt);
    /* given up while its draw is recorded */
    gfx_skin_release(other);
    gfx_end_scene();
    assert(g_perf.draws == draws + 2);
    assert(screen_pixel(d, 25, 17) == 0x00c080 && screen_pixel(d, 41, 25) == 0x00c080);
    assert(screen_pixel(d, 50, 17) == 0);
    /* ranges outside the model, a pose of none: nothing */
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    lit = skin_pose(16.0f, 100.0f, 28.5f, 0x804000, 0);
    draws = g_perf.draws;
    gfx_draw_skin(model, 12, 6, lit);
    gfx_draw_skin(model, 16, 3, lit);
    gfx_draw_skin(model, 0, 6, -1);
    gfx_draw_skin(NULL, 0, 6, lit);
    gfx_end_scene();
    assert(g_perf.draws == draws && screen_pixel(d, 25, 17) == 0);
    /* held twice and given up by its owner: still drawn, and the holds let go after its draw */
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    other = skin_model();
    assert(other);
    gfx_hold_skin(other);
    gfx_hold_skin(other);
    gfx_skin_release(other);
    lit = skin_pose(16.0f, 100.0f, 28.5f, 0x804000, 0);
    assert(lit >= 0);
    draws = g_perf.draws;
    gfx_draw_skin(other, 0, 6, lit);
    gfx_release_holds();
    gfx_end_scene();
    assert(g_perf.draws == draws + 1);
    assert(screen_pixel(d, 25, 17) == 0x00c080 && screen_pixel(d, 30, 22) == 0x00c080);
    gfx_skin_release(model);
}

/* The effect primitives' triangles of shared corners (render/quads.h, GFX_QUAD_CORNERS): a set of
   four corners at depth 100 through an identity view (a unit a pixel, the view's centre at (32, 24))
   and its two triangles, one draw, the set put among the frame's quads once with its matrix (64
   bytes, the set 96 and each triangle 20); in the next frame put up again; a corner the set does not
   have, or no set, draws nothing. */
static void check_corners(struct TestScreen *d)
{
    static const float at[4][2] = { { -8, -8 }, { 0, -8 }, { -8, 0 }, { 0, 0 } };
    struct GfxCornerSet set;
    struct GfxQuad q[2];
    unsigned draws, bytes;
    int i, frame;
    memset(&set, 0, sizeof set);
    set.row = -1;
    set.corners.count = 4;
    for (i = 0; i < 4; i++) {
        set.corners.point[i][0] = at[i][0] - 0.5f;
        set.corners.point[i][1] = at[i][1] - 0.5f;
        set.corners.point[i][2] = 100.0f;
    }
    set.corners.view[0] = set.corners.view[4] = set.corners.view[8] = 1.0f;
    set.corners.proj[0] = set.corners.proj[1] = 100.0f;
    set.corners.centre[0] = W / 2.0f;
    set.corners.centre[1] = H / 2.0f;
    set.corners.colour = 0xff00ff00;
    memset(q, 0, sizeof q);
    for (i = 0; i < 2; i++) {
        q[i].kind = GFX_QUAD_CORNERS;
        q[i].corners.set = &set;
        q[i].corners.flat = 1;
    }
    q[0].corners.corner[0] = 0;
    q[0].corners.corner[1] = 1;
    q[0].corners.corner[2] = 2;
    q[1].corners.corner[0] = 3;
    q[1].corners.corner[1] = 2;
    q[1].corners.corner[2] = 1;
    for (frame = 0; frame < 2; frame++) {
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
        gfx_set_mode(DRAW_OVERLAY);
        gfx_set_texture(0, NULL);
        draws = g_perf.draws;
        bytes = g_perf.instanceBytes;
        gfx_draw_quad(&q[0]);
        gfx_draw_quad(&q[1]);
        gfx_end_scene();
        assert(g_perf.draws == draws + 1 && g_perf.instanceBytes == bytes + 64 + 96 + 2 * 20);
        /* the square x 24 to 31, y 16 to 23, both halves */
        assert(screen_pixel(d, 25, 17) == 0x00ff00 && screen_pixel(d, 30, 22) == 0x00ff00);
        assert(screen_pixel(d, 22, 17) == 0 && screen_pixel(d, 33, 22) == 0);
    }
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    q[0].corners.corner[2] = 4;
    q[1].corners.set = NULL;
    gfx_draw_quad(&q[0]);
    gfx_draw_quad(&q[1]);
    gfx_end_scene();
    assert(screen_pixel(d, 25, 17) == 0 && screen_pixel(d, 30, 22) == 0);
}

/* A white skinned square two pixels wide whose top left pixel is (x, y), at the view depth
   `depth`: a pose of skin_pose's projection that scales and moves a unit square of the model
   (fog_square) there */
static int fog_square_pose(float x, float y, float depth)
{
    float bone[16], rows[SKIN_POSE_HEADER + 3][4];
    struct SkinPose pose;
    memset(bone, 0, sizeof bone);
    memset(&pose, 0, sizeof pose);
    bone[0] = bone[5] = bone[10] = bone[15] = 1.0f;
    pose.bones = bone;
    pose.boneCount = 1;
    pose.view[0] = pose.view[4] = 2.0f * depth / 100.0f;
    pose.view[9] = (x - 0.5f - W / 2.0f) * depth / 100.0f;
    pose.view[10] = (y - 0.5f - H / 2.0f) * depth / 100.0f;
    pose.view[11] = depth;
    pose.proj[0] = pose.proj[1] = 100.0f;
    pose.centre[0] = W / 2.0f;
    pose.centre[1] = H / 2.0f;
    pose.nearZ = 10.0f;
    pose.depthScale = 1.0f;
    pose.light[2] = -1.0f;
    pose.offset = 255.0f;
    pose.alpha = 0xff;
    assert(skin_pose_rows(&pose, rows) == skin_pose_size(1));
    return gfx_skin_pose((const float (*)[4])rows, skin_pose_size(1));
}

/* a model of one unit square on bone 0 */
static gfx_skin *fog_square(void)
{
    struct SkinVertex v[4];
    static const uint32_t indices[6] = { 0, 1, 2, 2, 1, 3 };
    int i;
    memset(v, 0, sizeof v);
    for (i = 0; i < 4; i++) {
        v[i].position[0] = (float)(i & 1);
        v[i].position[1] = (float)(i >> 1);
        v[i].normal[2] = -1.0f;
        v[i].weights[0] = 1.0f;
    }
    return gfx_create_skin(v, 4, indices, 6);
}

/* The world's records land on the pixels the same corners pre-transformed cover. */
static void check_world(struct TestScreen *d)
{
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct GfxMeshVertex mv[6];
    static const WORD indices[6] = { 0, 1, 2, 3, 4, 5 };
    static const int corner[6] = { 0, 1, 2, 2, 1, 3 };
    struct GfxWorldDraw draw;
    struct SwRun runs[2];
    int i;
    test_view(&view);
    gfx_set_world_camera(&view);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);

    /* a quad of two triangles that share no vertex, a record each */
    memset(mv, 0, sizeof mv);
    for (i = 0; i < 6; i++) {
        mv[i].x = (corner[i] & 1) ? 8.0f - 0.5f : 0.0f - 0.5f;
        mv[i].y = (corner[i] & 2) ? 8.0f - 0.5f : 0.0f - 0.5f;
        mv[i].z = 100.0f;
        mv[i].diffuse = 0xff4080c0;
    }
    world_begin();
    runs[0] = model_run(0, 3, SW_CULL_NONE);
    runs[1] = model_run(3, 3, SW_CULL_NONE);
    world_add(mv, 6, indices, 6, runs, 2, 0, NULL, 0);
    world_send(NULL, 0);
    memset(&frame, 0, sizeof frame);
    world_frame(&view, &frame);
    model_draw(&draw);
    gfx_set_texture(0, NULL);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    gfx_world_draw(SW_PASS_OPAQUE_CLAMP, &draw);
    gfx_end_scene();
    assert(screen_pixel(d, 32, 24) == 0x4080c0 && screen_pixel(d, 39, 31) == 0x4080c0);
    assert(screen_pixel(d, 31, 24) == 0 && screen_pixel(d, 40, 31) == 0 && screen_pixel(d, 32, 32) == 0);
    /* a record from an odd index: the second triangle alone, below the diagonal from top right */
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    assert(g_world.records[1].firstIndex == 3);
    gfx_world_draw_record(&g_world.records[1], &draw);
    gfx_end_scene();
    assert(screen_pixel(d, 39, 31) == 0x4080c0 && screen_pixel(d, 32, 24) == 0);
}

/* Transform slots and texture-coordinate matrices: a model's mesh lands where its matrix puts it,
   an animated node's where the frame's matrix puts it under its parent, culled where it is; a
   record's texture coordinates go through its uv slot's matrix; one texture array serves them all. */
static void check_slots(struct TestScreen *d)
{
    static const DWORD texels[4] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff };
    static const WORD indices[6] = { 0, 1, 2, 2, 1, 3 };
    /* u' = 1 - u */
    static const float flip[1][6] = { { -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f } };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, texels);
    struct GfxMeshVertex mv[4], right[4];
    struct GfxWorldSlot slots[2];
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct GfxWorldDraw draw;
    float down[1][12];
    struct SwRun run;
    int i;
    /* a quad 8 pixels square, its corner at the view's centre, its texture across it */
    memset(mv, 0, sizeof mv);
    for (i = 0; i < 4; i++) {
        mv[i].x = (i & 1) ? 8.0f - 0.5f : -0.5f;
        mv[i].y = (i & 2) ? 8.0f - 0.5f : -0.5f;
        mv[i].z = 100.0f;
        mv[i].diffuse = 0xffffffff;
        mv[i].u0 = (float)(i & 1);
        mv[i].v0 = (float)(i >> 1);
        right[i] = mv[i];
        right[i].x += 16.0f;
    }
    /* slot 1: a model 16 to the left; slot 2: a node of it, animated, which the frame moves 16 up */
    memset(slots, 0, sizeof slots);
    for (i = 0; i < 2; i++)
        slots[i].local[0] = slots[i].local[4] = slots[i].local[8] = 1.0f;
    slots[0].local[9] = -16.0f;
    slots[0].parent = -1;
    slots[0].animated = -1;
    slots[1].parent = 1;
    slots[1].animated = 0;
    memset(down, 0, sizeof down);
    down[0][0] = down[0][4] = down[0][8] = 1.0f;
    down[0][10] = -16.0f;
    world_begin();
    world_texture(t, 0, 0);
    run = model_run(0, 6, SW_CULL_NONE);
    run.textureArray = 0;
    run.textureLayer = 0;
    world_add(mv, 4, indices, 6, &run, 1, 0, NULL, 0);
    world_add(mv, 4, indices, 6, &run, 1, 1, slots[0].local, 0);
    world_add(mv, 4, indices, 6, &run, 1, 2, NULL, 1);
    run.uv = 0;
    world_add(right, 4, indices, 6, &run, 1, 0, NULL, 0);
    world_send(slots, 2);
    test_view(&view);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    memset(&frame, 0, sizeof frame);
    frame.animated = (const float (*)[12])down;
    frame.animatedCount = 1;
    frame.uv = flip;
    frame.uvCount = 1;
    world_frame(&view, &frame);
    model_draw(&draw);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    gfx_world_draw(SW_PASS_OPAQUE_CLAMP, &draw);
    gfx_end_scene();
    /* in the world's slot, the pixels 32 to 39 by 24 to 31: red at the top left, green right */
    assert(screen_pixel(d, 33, 25) == 0xff0000 && screen_pixel(d, 38, 25) == 0x00ff00);
    assert(screen_pixel(d, 33, 30) == 0x0000ff && screen_pixel(d, 38, 30) == 0xffffff);
    /* the model's, 16 to the left; its node's, 16 up from that */
    assert(screen_pixel(d, 17, 25) == 0xff0000 && screen_pixel(d, 22, 30) == 0xffffff);
    assert(screen_pixel(d, 17, 9) == 0xff0000 && screen_pixel(d, 22, 14) == 0xffffff);
    assert(screen_pixel(d, 17, 17) == 0 && screen_pixel(d, 33, 9) == 0);
    /* the texture turned left to right */
    assert(screen_pixel(d, 49, 25) == 0x00ff00 && screen_pixel(d, 54, 25) == 0xff0000);
    gfx_set_texture(0, NULL);
    gfx_release(t);
}

/* The map's water (gfx_world_draw_water): a cell's quad at its wave's height, the wave's degree
   looked up in the table of sines given, through the camera, in the draw's colour; a cell whose
   ground lies above the wave (y grows downward) is not drawn. */
static void check_water(struct TestScreen *d)
{
    static float sines[360];
    static const uint32_t which[2] = { 0, 1 };
    struct GfxWaterCell cells[2];
    struct GfxWaterWave wave;
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    int c, k;
    /* a view down the y axis: x across, z down the screen, y the depth */
    memset(&view, 0, sizeof view);
    view.view[0] = view.view[7] = view.view[5] = 1.0f;
    view.proj[0] = view.proj[1] = 100.0f;
    view.centre[0] = W / 2.0f;
    view.centre[1] = H / 2.0f;
    view.nearZ = 10.0f;
    view.depthScale = 1.0f;
    /* the cells (0, 0) and (1, 0), 8 units square; the second's ground nearer the eye than the
       wave, above it */
    memset(cells, 0, sizeof cells);
    for (c = 0; c < 2; c++) {
        cells[c].x = c;
        cells[c].top = c ? 150.0f : 300.0f;
        for (k = 0; k < 4; k++) {
            cells[c].corners[k][0] = (k & 1 ? -0.5f : -8.5f) + (float)c * 16.0f;
            cells[c].corners[k][1] = k & 2 ? -8.5f : -0.5f;
        }
    }
    /* every corner of these at 270 degrees: -450, -90 and 270 before, at and after the diagonal 0,
       where the table says 25: 100 + 25 * 4 */
    memset(sines, 0, sizeof sines);
    sines[270] = 25.0f;
    wave.phase = -90.0f;
    wave.pitch = 360.0f;
    wave.height = 4.0f;
    wave.level = 100.0f;
    world_begin();
    assert(gfx_world_water(cells, 2, sines));
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    memset(&frame, 0, sizeof frame);
    frame.waves = &wave;
    frame.waveCount = 1;
    world_frame(&view, &frame);
    gfx_set_texture(0, NULL);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    gfx_world_draw_water(which, 2, 0xff20c040);
    gfx_end_scene();
    /* at depth 200 the corners -8.5 and -0.5 land on 27.75 and 31.75 across, 19.75 and 23.75 down
       (the pixels beside those edges, whose samples it partly covers when multisampled, aside) */
    assert(screen_pixel(d, 29, 21) == 0x20c040 && screen_pixel(d, 30, 22) == 0x20c040);
    assert(screen_pixel(d, 27, 21) == 0 && screen_pixel(d, 29, 19) == 0 && screen_pixel(d, 24, 16) == 0);
    assert(screen_pixel(d, 33, 22) == 0 && screen_pixel(d, 30, 25) == 0);
    assert(screen_pixel(d, 37, 21) == 0 && screen_pixel(d, 38, 22) == 0);
}

/* A window resized: the screen takes the new size, keeps drawing and is read at it, the wobble
   covers it, and it goes back. */
/* rows 0 to 19 from row 39 moved right by 8, the others as they are */
static void fill_resized_rows(void *context)
{
    struct WobbleRow *rows = context;
    int y;
    for (y = 0; y < 40; y++) {
        rows[y].source = y < 20 ? 39 : y;
        rows[y].shift = y < 20 ? 8 : 0;
    }
}

static void check_resize(struct TestScreen *d)
{
    static DWORD frame[80 * 40];
    struct WobbleRow rows[40];
    RECT beyond = { 0, 0, 81, 40 };
    int w = 0, h = 0;
    assert(gfx_resize(80, 40) == 0);
    d->width = 80;
    d->height = 40;
    assert(gfx_get_size(d->screen, &w, &h) == 0 && w == 80 && h == 40);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff405060, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    draw_quad(70, 30, 80, 40, 0xffffffff);
    assert(gfx_wobble(rows, 40, fill_resized_rows, rows) == 1);
    gfx_end_scene();
    assert(gfx_read_frame(&beyond, frame, 81 * 4) < 0);
    assert(gfx_read_frame(NULL, frame, 80 * 4) == 0);
    assert((frame[0] & 0xffffff) == 0x405060 && (frame[39 * 80 + 79] & 0xffffff) == 0xffffff);
    /* rows 0 to 19 show row 39 moved right by 8: its white from 78 on */
    assert((frame[5 * 80 + 78] & 0xffffff) == 0xffffff && (frame[5 * 80 + 77] & 0xffffff) == 0x405060);
    assert((frame[19 * 80 + 79] & 0xffffff) == 0xffffff && (frame[20 * 80 + 79] & 0xffffff) == 0x405060);
    assert(gfx_resize(W, H) == 0);
    d->width = W;
    d->height = H;
    gfx_clear(GFX_CLEAR_TARGET, 0xff102030, 1.0f, NULL);
    assert(screen_pixel(d, W - 1, H - 1) == 0x102030);
}

/* The fog of the modes that fog, while it is on (render/gfx.h, GfxFog): the quads' by their rhw,
   in 1/255 steps, the meshes' as the Direct3D 9 client's mesh shader had it. */
static void fog_quad(float x, float y, float size, float rhw)
{
    struct TexDrawVertex v[4];
    int i;
    quad(v, x, y, x + size, y + size, 0xffff0000);
    for (i = 0; i < 4; i++)
        v[i].rhw = rhw;
    gfx_draw_vertices(v, 4, 1);
}

static int near_value(DWORD got, int want)
{
    return (int)got >= want - 1 && (int)got <= want + 1;
}

static float next_float(float value, int by)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    bits += (uint32_t)by;
    memcpy(&value, &bits, sizeof value);
    return value;
}

/* The original's vertex fog as the client computed it on the CPU before the fog went into the
   shaders (render/rp_pool.c, rp_vertex_fog_alpha), for a fog of any start and end: the step, from
   0 (all fog) to 255 (none), of a vertex of reciprocal view depth rhw. */
static int original_fog_step(float rhw, const struct GfxFog *fog)
{
    float range = fog->end - fog->start;
    float t = (1.0f / rhw - fog->origin) / fog->span;
    if (range != 0.0f)
        t = (t - fog->start) / range;
    t = 0.0f > t ? 0.0f : t;
    t = 1.0f < t ? 1.0f : t;
    t = (1.0f - t) * 255.0f;
    return (int)t;
}

/* The rhw at which each step of the fog begins, from the nearest view depth's on (the steps are
   monotonic in rhw, as each operation of the original's is): the first float rhw whose step is
   `levels` past the smallest normal rhw's, found by its bits. Answers how many steps it found. */
static int fog_step_starts(const struct GfxFog *fog, float *starts, int most)
{
    int base = original_fog_step(1.17549435e-38f, fog), last = original_fog_step(1e30f, fog);
    int direction = last >= base ? 1 : -1, count = (last - base) * direction, levels;
    for (levels = 1; levels <= count && levels <= most; levels++) {
        uint32_t low = 0x00800000u, high = 0x7f800000u;
        while (low < high) {
            uint32_t middle = low + (high - low) / 2;
            float rhw;
            memcpy(&rhw, &middle, sizeof rhw);
            if ((original_fog_step(rhw, fog) - base) * direction >= levels)
                high = middle;
            else
                low = middle + 1;
        }
        memcpy(&starts[levels - 1], &low, sizeof low);
    }
    return levels - 1;
}

/* The fog of the quads and Granny's meshes within a step of the original's: white quads fogged
   into black keep the original's step in their red, but for a step where the GPU's division of
   the rhw lands on the other side of a step's start, at every step's first rhw and the rhw before
   it, for the maps' fog (0.2 to 0.8) and for an end before the start; white squares of one skinned
   mesh at those depths, one instanced draw, likewise; it says how many were a step off. */
static void check_fog_steps(struct TestScreen *d)
{
    static const float ranges[2][2] = { { 0.2f, 0.8f }, { 0.7f, 0.3f } };
    static float starts[256];
    struct GfxFog fog;
    gfx_skin *square = fog_square();
    unsigned draws;
    int r, i, wrong = 0, skinWrong = 0, steps = 0, skinSteps = 0;
    assert(square);
    for (r = 0; r < 2; r++) {
        int count;
        memset(&fog, 0, sizeof fog);
        fog.on = 1;
        fog.colour = 0xff000000;
        fog.start = ranges[r][0];
        fog.end = ranges[r][1];
        fog.origin = 10.0f;
        fog.span = 1490.0f;
        count = fog_step_starts(&fog, starts, 256);
        assert(count > 200);
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
        gfx_set_mode(DRAW_OPAQUE_CLAMP);
        gfx_set_texture(0, NULL);
        gfx_set_fog(&fog);
        /* two pixels square each, 31 to a row off the first row and column (of which the
           viewport holds half): a step's first rhw, then the one before */
        for (i = 0; i < 2 * count; i++)
            fog_quad((float)(1 + i % 31 * 2), (float)(1 + i / 31 * 2), 2.0f, next_float(starts[i / 2], -(i & 1)));
        gfx_end_scene();
        gfx_set_fog(NULL);
        for (i = 0; i < 2 * count; i++) {
            float rhw = next_float(starts[i / 2], -(i & 1));
            int want = original_fog_step(rhw, &fog);
            int got = (int)(screen_pixel(d, 1 + i % 31 * 2, 1 + i / 31 * 2) >> 16 & 0xff);
            steps += got != want;
            if ((got < want - 1 || got > want + 1) && wrong++ < 8)
                fprintf(stderr, "fog %g..%g, rhw %.9g: %d, the original %d\n", fog.start, fog.end, rhw, got, want);
        }
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
        gfx_set_fog(&fog);
        draws = g_perf.draws;
        for (i = 0; i < 2 * count; i++) {
            int pose = fog_square_pose((float)(1 + i % 31 * 2), (float)(1 + i / 31 * 2),
                                       1.0f / next_float(starts[i / 2], -(i & 1)));
            assert(pose >= 0);
            gfx_draw_skin(square, 0, 6, pose);
        }
        gfx_end_scene();
        gfx_set_fog(NULL);
        assert(g_perf.draws == draws + 1);
        for (i = 0; i < 2 * count; i++) {
            float rhw = next_float(starts[i / 2], -(i & 1));
            int want = original_fog_step(rhw, &fog);
            int got = (int)(screen_pixel(d, 1 + i % 31 * 2, 1 + i / 31 * 2) >> 16 & 0xff);
            skinSteps += got != want;
            if ((got < want - 1 || got > want + 1) && skinWrong++ < 8)
                fprintf(stderr, "skinned fog %g..%g, rhw %.9g: %d, the original %d\n", fog.start, fog.end, rhw, got,
                        want);
        }
    }
    gfx_skin_release(square);
    printf("gfx_gpu: the fog at each step's first rhw and the one before: %d of the quads' a step off the original's, "
           "%d of the skinned squares'\n", steps, skinSteps);
    assert(!wrong && !skinWrong);
}

static void check_fog(struct TestScreen *d)
{
    static const WORD indices[6] = { 0, 1, 2, 2, 1, 3 };
    struct GfxFog fog;
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct GfxMeshVertex mv[4];
    struct GfxWorldDraw draw;
    struct SwRun run;
    DWORD p;
    int i;
    memset(&fog, 0, sizeof fog);
    fog.on = 1;
    fog.colour = 0xff00ff00;
    fog.end = 1.0f;
    fog.span = 100.0f;
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    gfx_set_texture(0, NULL);
    gfx_set_fog(&fog);
    fog_quad(0, 0, 8, 1.0f / 50);   /* half way: 127 of 255 kept */
    fog_quad(8, 0, 8, 1.0f / 150);  /* beyond the end: all fog */
    fog_quad(16, 0, 8, 1.0f);       /* 0.01 of the way: 252 kept */
    /* a mode without fog */
    gfx_set_mode(DRAW_OVERLAY);
    fog_quad(24, 0, 8, 1.0f / 150);
    /* the end where the start is: the depth's share alone */
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    fog.start = fog.end = 0.25f;
    gfx_set_fog(&fog);
    fog_quad(32, 0, 8, 1.0f / 50);
    /* a record at depth 50 between 40 and 60: half fog (the pixels 8 to 15 by 12 to 19) */
    memset(mv, 0, sizeof mv);
    for (i = 0; i < 4; i++) {
        mv[i].x = (i & 1) ? -16.0f : -24.0f;
        mv[i].y = (i & 2) ? -4.0f : -12.0f;
        mv[i].z = 50.0f;
        mv[i].diffuse = 0xffff0000;
    }
    world_begin();
    run = model_run(0, 6, SW_CULL_NONE);
    world_add(mv, 4, indices, 6, &run, 1, 0, NULL, 0);
    world_send(NULL, 0);
    memset(&view, 0, sizeof view);
    view.view[0] = view.view[4] = view.view[8] = 1.0f;
    view.proj[0] = view.proj[1] = 50.0f;
    view.centre[0] = W / 2.0f;
    view.centre[1] = H / 2.0f;
    view.nearZ = 10.0f;
    view.depthScale = 1.0f;
    memset(&frame, 0, sizeof frame);
    world_frame(&view, &frame);
    fog.start = 0.4f;
    fog.end = 0.6f;
    gfx_set_fog(&fog);
    model_draw(&draw);
    gfx_world_draw_record(&g_world.records[0], &draw);
    /* the fog off */
    gfx_set_fog(NULL);
    fog_quad(40, 0, 8, 1.0f / 150);
    gfx_end_scene();
    /* the quads' steps, their rhw away from a step's start: 127 of red and 128 of the fog's green */
    assert(screen_pixel(d, 4, 4) == 0x7f8000);
    assert(screen_pixel(d, 12, 4) == 0x00ff00);
    assert(screen_pixel(d, 20, 4) == 0xfc0300);
    assert(screen_pixel(d, 28, 4) == 0xff0000);
    assert(screen_pixel(d, 36, 4) == 0x7f8000);
    /* the mesh's fog is the GPU's arithmetic, as the Direct3D 9 client's mesh shader's was */
    p = screen_pixel(d, 12, 16);
    assert(near_value(p >> 16 & 0xff, 128) && near_value(p >> 8 & 0xff, 128));
    assert(screen_pixel(d, 44, 4) == 0xff0000);
    check_fog_steps(d);
}

/* The effect modes blend by the D3DBLEND factors and combine by the preset gfx_set_effect gives:
   added (ONE, ONE), the texture plus the colour (preset 3), and on the screen, whose alpha is 1,
   DESTALPHA as ONE; with preset 5 the second texture times the first and the colour, read at the
   same coordinates, opaque black outside 0 to 1. */
static void check_effects(struct TestScreen *d)
{
    static const DWORD grey[4] = { 0xff404040, 0xff404040, 0xff404040, 0xff404040 };
    static const DWORD half[4] = { 0xff808080, 0xff808080, 0xff808080, 0xff808080 };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, grey), *u = texture_2x2(GFX_FMT_8888, half);
    struct TexDrawVertex v[4];
    int i;
    gfx_clear(GFX_CLEAR_TARGET, 0xff102030, 1.0f, NULL);
    gfx_set_mode(DRAW_EFFECT_OVERLAY);
    gfx_set_texture(0, t);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ONE, 3);
    quad(v, 0, 0, 16, 16, 0xff200000);
    gfx_draw_vertices(v, 4, 1);
    gfx_set_effect(DRAW_BLEND_SRCALPHA, DRAW_BLEND_DESTALPHA, 0);
    quad(v, 16, 0, 32, 16, 0x80ffffff);
    gfx_draw_vertices(v, 4, 1);
    gfx_set_texture(1, u);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ZERO, 5);
    quad(v, 0, 16, 16, 32, 0xffffffff);
    for (i = 0; i < 4; i++) {
        v[i].u *= 2.0f;
        v[i].v *= 2.0f;
    }
    gfx_draw_vertices(v, 4, 1);
    gfx_end_scene();
    /* 0x40 + 0x20, 0x40 and 0x40, added to 0x10, 0x20 and 0x30 */
    assert(screen_pixel(d, 4, 4) == 0x706070);
    /* half of 0x40 a channel added to all of it */
    i = (int)(screen_pixel(d, 20, 4) & 0xff);
    assert(i >= 0x4e && i <= 0x52);
    /* half of 0x40 inside the texture's coordinates, black beyond them */
    i = (int)(screen_pixel(d, 4, 20) & 0xff);
    assert(i >= 0x1f && i <= 0x21 && screen_pixel(d, 12, 28) == 0);
    gfx_set_texture(0, NULL);
    gfx_set_texture(1, NULL);
    gfx_release(t);
    gfx_release(u);
}

/* The effect presets against Direct3D 9's texture stages: the states the Direct3D 9 client set for
   each (renderer_set_blend_stages, render/crenderer.c before the modes, 02fc9e5e) evaluated as
   Direct3D 9's fixed function evaluates them. Arguments: 0 the diffuse colour, 1 the current one, 2
   the stage's texture, +0x10 complemented, +0x20 its alpha replicated. Operations: 1 off, 2 the first
   argument, 3 the second, 4 their product, 5 twice it, 7 their sum, 8 their sum less a half, 12 and
   13 the first over the second by the diffuse's or the texture's alpha. */
struct StageStates {
    int op, arg1, arg2, aop, aarg1, aarg2;
};

static const struct StageStates preset_stages[16][2] = {
    { { 4, 2, 0, 4, 2, 0 }, { 1, 2, 1, 1, 2, 1 } },
    { { 4, 2, 0, 2, 2, 0 }, { 1, 2, 1, 1, 2, 1 } },
    { { 4, 2, 0, 2, 2, 0 }, { 1, 2, 1, 1, 2, 1 } },
    { { 7, 2, 0, 3, 2, 0 }, { 1, 2, 1, 1, 2, 1 } },
    { { 13, 2, 0, 3, 2, 0 }, { 1, 2, 1, 1, 2, 1 } },
    { { 4, 2, 0, 3, 2, 0 }, { 4, 2, 1, 2, 2, 1 } },
    { { 4, 0x12, 0, 3, 2, 0 }, { 4, 0x12, 1, 2, 2, 1 } },
    { { 2, 2, 0, 2, 2, 0 }, { 4, 0x22, 1, 2, 2, 1 } },
    { { 4, 2, 0, 2, 2, 0 }, { 7, 2, 1, 2, 2, 1 } },
    { { 4, 2, 0, 3, 2, 0 }, { 13, 2, 1, 2, 2, 1 } },
    { { 4, 2, 0, 3, 2, 0 }, { 12, 2, 1, 2, 2, 1 } },
    { { 4, 2, 0, 3, 2, 0 }, { 7, 2, 1, 2, 2, 1 } },
    { { 4, 2, 0, 3, 2, 0 }, { 7, 0x12, 1, 2, 2, 1 } },
    { { 7, 2, 0, 2, 2, 0 }, { 4, 2, 1, 2, 2, 1 } },
    { { 4, 2, 0, 3, 2, 0 }, { 5, 2, 1, 2, 2, 1 } },
    { { 4, 2, 0, 3, 2, 0 }, { 8, 2, 1, 2, 2, 1 } },
};

static double clamp01(double x)
{
    return x < 0 ? 0 : x > 1 ? 1 : x;
}

static double argument(int a, int channel, const double *diffuse, const double *current, const double *tex)
{
    const double *v = (a & 15) == 0 ? diffuse : (a & 15) == 2 ? tex : current;
    double x = (a & 0x20) ? v[3] : v[channel];
    return (a & 0x10) ? 1 - x : x;
}

static double operation(int op, double a1, double a2, double current, const double *diffuse, const double *tex)
{
    switch (op) {
    case 2: return a1;
    case 3: return a2;
    case 4: return a1 * a2;
    case 5: return a1 * a2 * 2;
    case 7: return a1 + a2;
    case 8: return a1 + a2 - 0.5;
    case 12: return a1 * diffuse[3] + a2 * (1 - diffuse[3]);
    case 13: return a1 * tex[3] + a2 * (1 - tex[3]);
    default: return current;
    }
}

/* The colour (red, green, blue, alpha, 0 to 1) Direct3D 9's two stages make of the textures and
   the diffuse colour. */
static void d3d9_stages(const struct StageStates *stages, const double *t, const double *u, const double *diffuse,
                        double *out)
{
    double current[4];
    int s, c;
    memcpy(current, diffuse, sizeof current);
    for (s = 0; s < 2 && stages[s].op != 1; s++) {
        const struct StageStates *st = &stages[s];
        const double *tex = s ? u : t;
        double next[4];
        for (c = 0; c < 3; c++)
            next[c] = clamp01(operation(st->op, argument(st->arg1, c, diffuse, current, tex),
                                        argument(st->arg2, c, diffuse, current, tex), current[c], diffuse, tex));
        next[3] = st->aop == 1 ? current[3]
                : clamp01(operation(st->aop, argument(st->aarg1, 3, diffuse, current, tex),
                                    argument(st->aarg2, 3, diffuse, current, tex), current[3], diffuse, tex));
        memcpy(current, next, sizeof current);
    }
    memcpy(out, current, sizeof current);
}

static int near_byte(DWORD got, double want)
{
    int w = (int)(want * 255 + 0.5), g = (int)got;
    return g >= w - 1 && g <= w + 1;
}

/* Each preset drawn by an effect mode: its colour (blending ONE, ZERO) and its alpha (ZERO, SRCALPHA
   over white), against Direct3D 9's stages evaluated exactly, within a step a channel: the GPU's
   sampling and arithmetic are its own, as Direct3D 9's hardware's were (preset 13's blue comes out
   143 where the exact value is 143.56 on the GTX 970). The quads keep off the first
   row and column, of which the viewport holds half (a GPU that clips to it covers only their other
   samples when multisampling). */
static void check_presets(struct TestScreen *d)
{
    static const DWORD tc[4] = { 0xc060a030, 0xc060a030, 0xc060a030, 0xc060a030 };
    static const DWORD uc[4] = { 0x709020d0, 0x709020d0, 0x709020d0, 0x709020d0 };
    static const double t[4] = { 0x60 / 255.0, 0xa0 / 255.0, 0x30 / 255.0, 0xc0 / 255.0 };
    static const double u[4] = { 0x90 / 255.0, 0x20 / 255.0, 0xd0 / 255.0, 0x70 / 255.0 };
    static const double diffuse[4] = { 0xb0 / 255.0, 0x50 / 255.0, 0x80 / 255.0, 0xe0 / 255.0 };
    gfx_surface *tt = texture_2x2(GFX_FMT_8888, tc), *ut = texture_2x2(GFX_FMT_8888, uc);
    struct TexDrawVertex v[4];
    int preset;
    gfx_clear(GFX_CLEAR_TARGET, 0xffffffff, 1.0f, NULL);
    gfx_set_mode(DRAW_EFFECT_OVERLAY);
    gfx_set_texture(0, tt);
    gfx_set_texture(1, ut);
    for (preset = 0; preset < 16; preset++) {
        int combine = draw_effect_combine(preset, 0);
        gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ZERO, combine);
        quad(v, (float)(preset * 4 + 1), 1, (float)(preset * 4 + 3), 3, 0xe0b05080);
        gfx_draw_vertices(v, 4, 1);
        gfx_set_effect(DRAW_BLEND_ZERO, DRAW_BLEND_SRCALPHA, combine);
        quad(v, (float)(preset * 4 + 1), 5, (float)(preset * 4 + 3), 7, 0xe0b05080);
        gfx_draw_vertices(v, 4, 1);
    }
    gfx_end_scene();
    for (preset = 0; preset < 16; preset++) {
        double want[4];
        DWORD colour = screen_pixel(d, preset * 4 + 1, 1), alpha = screen_pixel(d, preset * 4 + 1, 5);
        d3d9_stages(preset_stages[preset], t, u, diffuse, want);
        if (!near_byte(colour >> 16 & 0xff, want[0]) || !near_byte(colour >> 8 & 0xff, want[1])
            || !near_byte(colour & 0xff, want[2]) || !near_byte(alpha & 0xff, want[3])) {
            fprintf(stderr, "preset %d: %06lx alpha %02lx, Direct3D 9 %.1f %.1f %.1f alpha %.1f\n", preset,
                    (unsigned long)colour, (unsigned long)(alpha & 0xff), want[0] * 255, want[1] * 255,
                    want[2] * 255, want[3] * 255);
            assert(0);
        }
    }
    gfx_set_texture(0, NULL);
    gfx_set_texture(1, NULL);
    gfx_release(tt);
    gfx_release(ut);
}

/* Two textures of two coordinate sets (a GFX_QUAD_WORLD with stage2), multiplied (the multitexture
   particles): points in view space at depth 1, which a projection of 1 puts on their own pixels. */
static void check_two_textures(struct TestScreen *d)
{
    static const DWORD orange[4] = { 0xffff8000, 0xffff8000, 0xffff8000, 0xffff8000 };
    static const DWORD split[4] = { 0xff00ff00, 0xffffffff, 0xff00ff00, 0xffffffff };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, orange), *u = texture_2x2(GFX_FMT_8888, split);
    struct GfxQuad q;
    int i;
    memset(&q, 0, sizeof q);
    q.kind = GFX_QUAD_WORLD;
    q.world.viewSpace = 1;
    q.world.proj[0] = q.world.proj[1] = 1.0f;
    q.world.depth = GFX_DEPTH_VALUE;
    q.world.colour = 0xffffffff;
    q.world.stage2 = 1;
    q.world.flat = 1;
    for (i = 0; i < 4; i++) {
        q.world.point[i][0] = (i & 1 ? 16.0f : 0.0f) - 0.5f;
        q.world.point[i][1] = (i & 2 ? 16.0f : 0.0f) - 0.5f;
        q.world.point[i][2] = 1.0f;
        q.world.u[i] = q.world.v[i] = 0.5f;
        q.world.u1[i] = (float)(i & 1);
        q.world.v1[i] = (float)(i >> 1);
    }
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_mode(DRAW_MULTITEXTURE);
    gfx_set_texture(0, t);
    gfx_set_texture(1, u);
    gfx_draw_quad(&q);
    gfx_end_scene();
    assert(screen_pixel(d, 2, 2) == 0x008000 && screen_pixel(d, 13, 13) == 0xff8000);
    gfx_set_texture(0, NULL);
    gfx_set_texture(1, NULL);
    gfx_release(t);
    gfx_release(u);
}

/* The ground's records (GFX_MESH_GROUND): with the lightmap mode, texture times colour times the
   lightmap's alpha plus its colour, the lightmap opaque black outside its coordinates; without it,
   texture times colour. The texture and the lightmap lie in arrays of their own, of two sizes, so
   that a stage given the other's array shows. */
static void check_ground(struct TestScreen *d)
{
    static const DWORD grey[4] = { 0xff808080, 0xff808080, 0xff808080, 0xff808080 };
    static const DWORD light[4] = { 0x80402010, 0x80402010, 0x80402010, 0x80402010 };
    static const WORD corners[6] = { 0, 1, 2, 2, 1, 3 };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, grey), *lm = texture_square(GFX_FMT_8888, 4, light[0]);
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct GfxMeshVertex mv[12];
    struct GfxWorldDraw draw;
    struct SwRun runs[2];
    WORD indices[18];
    DWORD p;
    int q, i, way;
    /* three quads of 8 pixels: the lightmap inside its coordinates, outside them, and none */
    memset(mv, 0, sizeof mv);
    for (q = 0; q < 3; q++) {
        for (i = 0; i < 4; i++) {
            struct GfxMeshVertex *v = &mv[q * 4 + i];
            v->x = (float)(q * 8 + (i & 1 ? 8 : 0)) - 32.0f - 0.5f;
            v->y = (float)(i & 2 ? 8 : 0) - 24.0f - 0.5f;
            v->z = 100.0f;
            v->diffuse = 0xffffffff;
            v->u0 = v->v0 = 0.5f;
            v->u1 = v->v1 = q == 1 ? 2.5f : 0.5f;
        }
        for (i = 0; i < 6; i++)
            indices[q * 6 + i] = (WORD)(corners[i] + q * 4);
    }
    world_begin();
    world_texture(t, 0, 0);
    world_texture(lm, 1, 0);
    for (i = 0; i < 2; i++) {
        runs[i] = model_run(i ? 12 : 0, i ? 6 : 12, SW_CULL_NONE);
        runs[i].pass = SW_PASS_GROUND;
        runs[i].textureArray = 0;
        runs[i].lightmapArray = 1;
    }
    world_add(mv, 12, indices, 18, runs, 2, 0, NULL, 0);
    world_send(NULL, 0);
    test_view(&view);
    memset(&draw, 0, sizeof draw);
    draw.kind = GFX_MESH_GROUND;
    draw.colour[0] = draw.colour[1] = draw.colour[2] = draw.colour[3] = 1.0f;
    /* the first two quads with the lightmap, the third without it; then the ground's pass, the
       third quad with the lightmap too */
    for (way = 0; way < 2; way++) {
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
        memset(&frame, 0, sizeof frame);
        world_frame(&view, &frame);
        gfx_set_mode(DRAW_GROUND_LIGHTMAP);
        if (way) {
            gfx_world_draw(SW_PASS_GROUND, &draw);
        } else {
            gfx_world_draw_record(&g_world.records[0], &draw);
            gfx_set_mode(DRAW_GROUND);
            gfx_world_draw_record(&g_world.records[1], &draw);
        }
        gfx_end_scene();
        /* 0x40 + 0x80 / 255 * 0x80, 0x20 + the same, 0x10 + the same */
        p = screen_pixel(d, 4, 4);
        assert(near_value(p >> 16 & 0xff, 0x80) && near_value(p >> 8 & 0xff, 0x60) && near_value(p & 0xff, 0x50));
        assert(screen_pixel(d, 12, 4) == 0x808080);
        assert(screen_pixel(d, 20, 4) == (way ? p : 0x808080));
    }
    gfx_release(t);
    gfx_release(lm);
}

/* Two-pass alpha: the solid pass draws the texels of alpha 0xf0 and more and writes their depth,
   the fringe pass the others ("below" the reference), blended. */
static void check_two_pass_alpha(struct TestScreen *d)
{
    static const DWORD texels[4] = { 0xffff0000, 0x80ff0000, 0xffff0000, 0x80ff0000 };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, texels);
    DWORD p;
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_texture(0, t);
    gfx_set_mode(DRAW_SOLID_CLAMP);
    draw_quad(0, 0, 16, 16, 0xffffffff);
    gfx_set_mode(DRAW_FRINGE_CLAMP);
    draw_quad(16, 0, 32, 16, 0xffffffff);
    gfx_end_scene();
    /* clamped at the texels' outer halves: the opaque texel left, the half one right */
    assert(screen_pixel(d, 2, 8) == 0xff0000 && screen_pixel(d, 13, 8) == 0);
    assert(screen_pixel(d, 18, 8) == 0);
    p = screen_pixel(d, 29, 8);
    assert(near_value(p >> 16 & 0xff, 0x80) && (p & 0xffff) == 0);
    gfx_set_texture(0, NULL);
    gfx_release(t);
}

/* An effect mode that tests depth draws behind nothing nearer and writes none; the overlay's does
   not test it. */
static void check_effect_depth(struct TestScreen *d)
{
    struct TexDrawVertex v[4];
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_texture(0, NULL);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    quad(v, 0, 0, 16, 32, 0xffff0000);
    v[0].z = v[1].z = v[2].z = v[3].z = 0.3f;
    gfx_draw_vertices(v, 4, 1);
    gfx_set_mode(DRAW_EFFECT_DEPTH);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ZERO, DRAW_COMBINE_MODULATE);
    quad(v, 0, 0, 32, 16, 0xff00ff00);
    gfx_draw_vertices(v, 4, 1);
    /* drawn after it, behind the effect quad: had that written depth, this would be hidden */
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    quad(v, 16, 0, 32, 8, 0xff0000ff);
    v[0].z = v[1].z = v[2].z = v[3].z = 0.75f;
    gfx_draw_vertices(v, 4, 1);
    gfx_set_mode(DRAW_EFFECT_OVERLAY);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ZERO, DRAW_COMBINE_MODULATE);
    quad(v, 0, 16, 32, 32, 0xffffff00);
    v[0].z = v[1].z = v[2].z = v[3].z = 0.9f;
    gfx_draw_vertices(v, 4, 1);
    gfx_end_scene();
    assert(screen_pixel(d, 4, 4) == 0xff0000 && screen_pixel(d, 20, 12) == 0x00ff00);
    assert(screen_pixel(d, 20, 4) == 0x0000ff);
    assert(screen_pixel(d, 4, 20) == 0xffff00 && screen_pixel(d, 20, 20) == 0xffff00);
}

/* The interface composited into a window's texture (DRAW_COMPOSITE): blended by the source's alpha
   into the target's colour and alpha, texels of alpha 0 dropped. The window then draws the target as
   a texture: copied as it is, its colour shows; drawn in the interface's mode over blue, its alpha
   blends it. */
static void check_composite(struct TestScreen *d)
{
    static const DWORD texels[4] = { 0xffffffff, 0x00ffffff, 0xffffffff, 0x00ffffff };
    gfx_surface *rt = gfx_create_render_target(16, 16), *t = texture_2x2(GFX_FMT_8888, texels);
    DWORD p, q;
    assert(rt && gfx_set_render_target(rt) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0x00000000, 1.0f, NULL);
    gfx_set_mode(DRAW_COMPOSITE);
    gfx_set_texture(0, t);
    draw_quad(0, 0, 16, 16, 0x80ff0000);
    gfx_end_scene();
    assert(gfx_set_render_target(d->screen) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff0000ff, 1.0f, NULL);
    gfx_set_texture(0, rt);
    gfx_set_mode(TEXTURE);
    draw_quad(0, 0, 16, 16, 0xffffffff);
    gfx_set_mode(DRAW_OVERLAY);
    draw_quad(20, 0, 36, 16, 0xffffffff);
    gfx_end_scene();
    /* half red over transparent black: red 0x80, alpha a half of a half, which over blue leaves a
       quarter of the red and three quarters of the blue */
    p = screen_pixel(d, 2, 8);
    q = screen_pixel(d, 13, 8);
    assert(near_value(p >> 16 & 0xff, 0x80) && (p & 0xffff) == 0 && q == 0);
    p = screen_pixel(d, 22, 8);
    q = screen_pixel(d, 33, 8);
    assert(near_value(p >> 16 & 0xff, 0x20) && (p >> 8 & 0xff) == 0 && near_value(p & 0xff, 0xbf) && q == 0x0000ff);
    gfx_set_texture(0, NULL);
    gfx_release(t);
    gfx_release(rt);
}

/* A clear of colour and depth inside a pass (DRAW_CLEAR) writes both: a later quad behind the
   cleared depth is hidden there and drawn elsewhere. */
static void check_clear_inside(struct TestScreen *d)
{
    RECT r = { 8, 8, 16, 16 };
    struct TexDrawVertex v[4];
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    draw_quad(0, 0, 32, 32, 0xffffffff);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff0000ff, 0.25f, &r);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    gfx_set_texture(0, NULL);
    quad(v, 0, 0, 32, 32, 0xff00ff00);
    gfx_draw_vertices(v, 4, 1);
    gfx_end_scene();
    assert(screen_pixel(d, 12, 12) == 0x0000ff && screen_pixel(d, 4, 4) == 0x00ff00 && screen_pixel(d, 20, 12) == 0x00ff00);
}

/* The overlay list and the effect overlay list after it, which starts with the blending and
   combination the overlay's mode has, draw with the same state: their quads are one draw, as the
   Direct3D 9 client's were while no state changed. An effect's factors are taken whole. */
static void check_overlay_batch(struct TestScreen *d)
{
    unsigned draws;
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gfx_set_texture(0, NULL);
    draws = g_perf.draws;
    gfx_set_mode(DRAW_OVERLAY);
    draw_quad(0, 0, 8, 8, 0xffffffff);
    gfx_set_effect(DRAW_BLEND_SRCALPHA, DRAW_BLEND_INVSRCALPHA, DRAW_COMBINE_MODULATE);
    gfx_set_mode(DRAW_EFFECT_OVERLAY);
    draw_quad(8, 0, 16, 8, 0xffffffff);
    gfx_end_scene();
    assert(g_perf.draws == draws + 1);
    /* another factor ends the batch */
    draws = g_perf.draws;
    gfx_set_mode(DRAW_OVERLAY);
    draw_quad(0, 0, 8, 8, 0xffffffff);
    gfx_set_mode(DRAW_EFFECT_OVERLAY);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ONE, DRAW_COMBINE_MODULATE);
    draw_quad(8, 0, 16, 8, 0xffffffff);
    gfx_end_scene();
    assert(g_perf.draws == draws + 2);
    /* 0x105 is no factor and blends as ONE, not as SRCALPHA (5); 0x205 is another number and ends
       the batch; a combination that is none keeps the one before */
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    draws = g_perf.draws;
    gfx_set_effect(0x105, DRAW_BLEND_ZERO, DRAW_COMBINE_MODULATE);
    draw_quad(0, 0, 8, 8, 0x01ffffff);
    gfx_set_effect(0x205, DRAW_BLEND_ZERO, 99);
    draw_quad(8, 0, 16, 8, 0x01ffffff);
    gfx_end_scene();
    assert(g_perf.draws == draws + 2);
    assert(screen_pixel(d, 4, 4) == 0xffffff && screen_pixel(d, 12, 4) == 0xffffff);
}

/* The rows gfx_wobble draws with, filled when it asks: row y from row (7y + 3) mod H, moved right by
   4 (y mod 8) pixels, row 5 by more than the row is wide. */
struct TestRows {
    struct WobbleRow *rows;
    int count, fills;
};

static void fill_test_rows(void *context)
{
    struct TestRows *t = context;
    int y;
    for (y = 0; y < t->count; y++) {
        t->rows[y].source = (y * 7 + 3) % H;
        t->rows[y].shift = y == 5 ? W + 30 : (y % 8) * 4;
    }
    t->fills++;
}

static DWORD wobble_pattern(int x, int y)
{
    return (DWORD)(x << 16 | y << 8 | ((x * 7 + y * 13) & 0xff));
}

/* One frame of check_wobble: the pattern from sys, a red quad, the wobble, a white quad; with
   `other`, a pass into that target between the pattern and the red quad. */
static void check_wobble_frame(struct TestScreen *d, gfx_surface *sys, gfx_surface *other)
{
    static DWORD after[W * H];
    struct WobbleRow rows[H];
    struct TestRows t;
    int x, y;
    memset(rows, 0xff, sizeof rows);
    t.rows = rows;
    t.count = H - 2;
    t.fills = 0;
    assert(gfx_wobble(rows, 0, fill_test_rows, &t) == 0 && gfx_wobble(NULL, H, fill_test_rows, &t) == 0);
    assert(t.fills == 0);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    assert(gfx_blit(d->screen, NULL, sys, NULL) == 0);
    if (other) {
        assert(gfx_set_render_target(other) == 0);
        gfx_clear(GFX_CLEAR_TARGET, 0, 0.0f, NULL);
        assert(gfx_set_render_target(d->screen) == 0);
    }
    gfx_set_mode(COLOUR);
    draw_quad(30, 20, 40, 24, 0xffff0000);
    assert(gfx_wobble(rows, t.count, fill_test_rows, &t) == 1 && t.fills == 1);
    draw_quad(4, 4, 8, 8, 0xffffffff);
    gfx_end_scene();
    read_screen(d, after);
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++) {
            int sx = x, sy = y;
            DWORD want;
            if (y < t.count && x >= rows[y].shift) {
                sx = x - rows[y].shift;
                sy = rows[y].source;
            }
            want = sx >= 30 && sx < 40 && sy >= 20 && sy < 24 ? 0xff0000 : wobble_pattern(sx, sy);
            if (x >= 4 && x < 8 && y >= 4 && y < 8)
                want = 0xffffff;
            else if (multisampled && (sx == 0 || sy == 0))
                continue;
            assert(after[y * W + x] == want);
        }
}

/* the second table of check_wobble_twice: row y from row (5y + 1) mod H, moved right by 3 (y mod 4) */
static void fill_other_rows(void *context)
{
    struct TestRows *t = context;
    int y;
    for (y = 0; y < t->count; y++) {
        t->rows[y].source = (y * 5 + 1) % H;
        t->rows[y].shift = (y % 4) * 3;
    }
    t->fills++;
}

/* Two wobbles in one frame, each with a table of its own: the second's table lies after the first's
   among the frame's tables, so each draw must find its own record (through the identity instances
   the frame's upload fills). */
static void check_wobble_twice(struct TestScreen *d, gfx_surface *sys)
{
    static DWORD after[W * H], first[W * H];
    static int edge[W * H];
    struct WobbleRow rows[H], other[H];
    struct TestRows t, u;
    int x, y;
    t.rows = rows;
    t.count = H - 2;
    t.fills = 0;
    u.rows = other;
    u.count = H;
    u.fills = 0;
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    assert(gfx_blit(d->screen, NULL, sys, NULL) == 0);
    assert(gfx_wobble(rows, t.count, fill_test_rows, &t) == 1);
    assert(gfx_wobble(other, u.count, fill_other_rows, &u) == 1);
    gfx_end_scene();
    read_screen(d, after);
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++) {
            int sx = x, sy = y;
            if (y < t.count && x >= rows[y].shift) {
                sx = x - rows[y].shift;
                sy = rows[y].source;
            }
            first[y * W + x] = wobble_pattern(sx, sy);
            edge[y * W + x] = sx == 0 || sy == 0;
        }
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++) {
            int sx = x, sy = y;
            if (x >= other[y].shift) {
                sx = x - other[y].shift;
                sy = other[y].source;
            }
            if (multisampled && (sx == 0 || sy == 0 || edge[sy * W + sx]))
                continue;
            assert(after[y * W + x] == first[sy * W + sx]);
        }
}

/* The screen wobble (gfx_wobble) in a frame as the client records it: the picture drawn in a pass
   that resolves, the wobble, and a quad after it, in one command buffer, read once at the end. Each
   row is drawn again from its source row moved right by its shift, the pixels left of the shift
   keeping the row's own, a shift past the row keeping all of it, rows past the table's count kept;
   multisampled too, every sample of a pixel the same, so the screen resolves to the frame's pixels
   exactly; what is drawn after the wobble lands on top of it. The quad still batched when the
   wobble comes goes before it, into the screen's pass and, the second time, into a pass of its own
   after one into another target. The rows are filled only when the device asks, after everything
   else is in place, and not at all for a wobble it cannot draw. Multisampled, the pixels that come
   from the picture's first column or row are left out: a GPU that clips to the viewport covers
   only some of their samples, which the passes' half-pixel viewport leaves out in part
   (render/gpu_layer.h, GPU_MARGIN). */
static void check_wobble(struct TestScreen *d)
{
    gfx_surface *sys = gfx_create_surface(W, H), *other = gfx_create_render_target(4, 4);
    struct gfx_lock lk;
    int x, y, round;
    assert(sys && other && gfx_lock(sys, NULL, &lk) == 0);
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++) {
            DWORD c = wobble_pattern(x, y);
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &c, 4);
        }
    gfx_unlock(sys);
    for (round = 0; round < 2; round++)
        check_wobble_frame(d, sys, round ? other : NULL);
    check_wobble_twice(d, sys);
    gfx_release(other);
    gfx_release(sys);
}

/* A composited window's target drawn as the window's quad draws it (ui_window_make_quad_base,
   texture_draw_surface): at the window's place and size, its texture coordinates from the first
   texel's centre to a texel past the last one's (render/render_api.h, TEXEL_CENTRE). Every pixel
   shows its own texel exactly, for a target of odd size, and nothing around the window changes: by
   the nearest texel, and filtered linearly in the interface's mode, as the client draws it. Mesa's
   lavapipe filters anisotropically even where a pixel covers one texel when the coordinates are a
   target's, which the device scales to the target's part of its texture (render/gpu_layer.h,
   GPU_MARGIN), and mixes the neighbours in (docs/custom-review.md, "No read-backs"): there the
   linear read is not compared. Answers whether it was. */
static int check_window_target(struct TestScreen *d)
{
    enum { TW = 15, TH = 9, X = 21, Y = 13 };
    static DWORD all[W * H];
    gfx_surface *rt = gfx_create_render_target(TW, TH), *sys = gfx_create_surface(TW, TH);
    const char *gpu = SDL_GetStringProperty(SDL_GetGPUDeviceProperties(gpu_dev()), SDL_PROP_GPU_DEVICE_NAME_STRING, "");
    int linear = !strstr(gpu, "llvmpipe");
    struct gfx_lock lk;
    struct TexDrawVertex v[4];
    int x, y, i, pass;
    assert(rt && sys && gfx_lock(sys, NULL, &lk) == 0);
    for (y = 0; y < TH; y++)
        for (x = 0; x < TW; x++) {
            DWORD c = 0xff000000u | (DWORD)(x * 17 + 1) << 16 | (DWORD)(y * 29 + 3) << 8 | (DWORD)((x * y * 5 + 7) & 0xff);
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &c, 4);
        }
    gfx_unlock(sys);
    assert(gfx_set_render_target(rt) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0, 0.0f, NULL);
    assert(gfx_blit(rt, NULL, sys, NULL) == 0);
    for (pass = 0; pass < 1 + linear; pass++) {
        assert(gfx_set_render_target(d->screen) == 0);
        gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
        gfx_set_mode(pass ? DRAW_OVERLAY : TEXTURE);
        gfx_set_texture(0, rt);
        for (i = 0; i < 4; i++) {
            v[i].x = (float)(X + (i & 1 ? TW : 0));
            v[i].y = (float)(Y + (i & 2 ? TH : 0));
            v[i].z = 0.5f;
            v[i].rhw = 1.0f;
            v[i].diffuse = 0xffffffff;
            v[i].u = ((float)(i & 1 ? TW : 0) + 0.5f) / (float)TW;
            v[i].v = ((float)(i & 2 ? TH : 0) + 0.5f) / (float)TH;
        }
        gfx_draw_vertices(v, 4, 1);
        gfx_end_scene();
        read_screen(d, all);
        for (y = 0; y < H; y++)
            for (x = 0; x < W; x++) {
                DWORD want = 0;
                if (x >= X && x < X + TW && y >= Y && y < Y + TH) {
                    int tx = x - X, ty = y - Y;
                    want = (DWORD)(tx * 17 + 1) << 16 | (DWORD)(ty * 29 + 3) << 8 | (DWORD)((tx * ty * 5 + 7) & 0xff);
                }
                assert(all[y * W + x] == want);
            }
    }
    gfx_set_texture(0, NULL);
    gfx_release(sys);
    gfx_release(rt);
    return linear;
}

/* The window is claimed for the device, and gets its swapchain, at the first present while it is
   shown, and never while it is hidden, as the scene harness's window never is shown: its frames
   are drawn and read all the same. SDL says whether it keeps a window for the device by knowing its
   swapchain's format. A window is shown only under the offscreen video driver, which shows nothing
   on the desktop; where that driver cannot give a window a swapchain (a Vulkan without
   VK_EXT_headless_surface), SDL itself must fail to claim it. Answers whether presenting into a
   swapchain was checked. */
static int check_present(struct TestScreen *d)
{
    SDL_GPUDevice *dev = gpu_dev();
    const char *driver = SDL_GetCurrentVideoDriver();
    int i;
    gfx_clear(GFX_CLEAR_TARGET, 0xff334455, 1.0f, NULL);
    assert(gfx_present(0) == 0 && gfx_present(1) == 0);
    assert(SDL_GetGPUSwapchainTextureFormat(dev, d->window) == SDL_GPU_TEXTUREFORMAT_INVALID);
    assert(screen_pixel(d, 5, 5) == 0x334455);
    if (!driver || strcmp(driver, "offscreen"))
        return 0;
    assert(SDL_ShowWindow(d->window));
    assert(gfx_present(1) == 0);
    if (SDL_GetGPUSwapchainTextureFormat(dev, d->window) == SDL_GPU_TEXTUREFORMAT_INVALID) {
        assert(!SDL_ClaimWindowForGPUDevice(dev, d->window));
        SDL_HideWindow(d->window);
        return 0;
    }
    for (i = 0; i < 4; i++) {
        gfx_clear(GFX_CLEAR_TARGET, 0xff000000 | (DWORD)i * 0x101010, 1.0f, NULL);
        gfx_set_mode(COLOUR);
        draw_quad(2, 2, 10, 10, 0xffffffff);
        assert(gfx_present(i & 1) == 0);
        assert(screen_pixel(d, 5, 5) == 0xffffff && screen_pixel(d, 20, 20) == (DWORD)i * 0x101010);
    }
    /* hidden again, the window keeps its claim and takes no pictures */
    assert(SDL_HideWindow(d->window));
    assert(gfx_present(1) == 0 && gfx_present(0) == 0);
    assert(SDL_GetGPUSwapchainTextureFormat(dev, d->window) != SDL_GPU_TEXTUREFORMAT_INVALID);
    return 1;
}

/* A mode set again, or an effect's blending in a mode that does not take it, changes nothing it
   draws with and counts as skipped (diag/perf_counters.h). */
static void check_modes_counted(void)
{
    unsigned sent, skipped;
    gfx_set_mode(DRAW_OVERLAY);
    sent = g_perf.modesSent;
    skipped = g_perf.modesSkipped;
    gfx_set_mode(DRAW_OVERLAY);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ONE, 7);
    assert(g_perf.modesSent == sent && g_perf.modesSkipped == skipped + 2);
    gfx_set_mode(DRAW_ADDITIVE);
    gfx_set_mode(DRAW_EFFECT_WRAP);
    assert(g_perf.modesSent == sent + 2);
}

/* A quad of the world's over the pixels x0..x1 by y0..y1 at depth 100 (test_view), its texture
   across it; corners 0 1 2 3 left to right, then top to bottom. */
static void world_quad(struct GfxMeshVertex *mv, float x0, float y0, float x1, float y1, DWORD diffuse)
{
    int i;
    memset(mv, 0, 4 * sizeof *mv);
    for (i = 0; i < 4; i++) {
        mv[i].x = (i & 1 ? x1 : x0) - W / 2.0f - 0.5f;
        mv[i].y = (i & 2 ? y1 : y0) - H / 2.0f - 0.5f;
        mv[i].z = 100.0f;
        mv[i].diffuse = diffuse;
        mv[i].u0 = (float)(i & 1);
        mv[i].v0 = (float)(i >> 1);
    }
}

/* the quad's triangles turning clockwise on the screen (y down), and counter-clockwise */
static const WORD quad_cw[6] = { 0, 1, 2, 2, 1, 3 };
static const WORD quad_ccw[6] = { 0, 2, 1, 2, 3, 1 };

static void identity_slot(struct GfxWorldSlot *s, int animated)
{
    memset(s, 0, sizeof *s);
    s->local[0] = s->local[4] = s->local[8] = 1.0f;
    s->parent = -1;
    s->animated = animated;
}

/* The GPU's cull of a pass, run by run: a frustum narrower than the screen (its left half) leaves
   out a record wholly beyond it though it is on the screen, keeps an animated record its slot
   moves into it and leaves it out moved beyond; and each run of kept records draws with its own
   texture array and cull mode (two arrays, two layers of one, and the three cull modes in one
   pass). */
static void check_world_cull(struct TestScreen *d)
{
    gfx_surface *red = texture_square(GFX_FMT_8888, 2, 0xffff0000), *blue = texture_square(GFX_FMT_8888, 2, 0xff0000ff);
    gfx_surface *green = texture_square(GFX_FMT_8888, 4, 0xff00ff00);
    struct GfxMeshVertex mv[4];
    struct GfxWorldSlot slot;
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct GfxWorldDraw draw;
    struct SwProjection p;
    struct SwRun run;
    float move[1][12];
    int way;
    world_begin();
    world_texture(red, 0, 0);
    world_texture(blue, 0, 1);
    world_texture(green, 1, 0);
    /* red at the top left, counter-clockwise, dropping the clockwise ones */
    world_quad(mv, 4, 4, 12, 12, 0xffffffff);
    run = model_run(0, 6, 0);
    run.textureArray = 0;
    run.textureLayer = 0;
    world_add(mv, 4, quad_ccw, 6, &run, 1, 0, NULL, 0);
    /* green at the top right, both sides */
    world_quad(mv, 40, 4, 48, 12, 0xffffffff);
    run = model_run(0, 6, SW_CULL_NONE);
    run.textureArray = 1;
    world_add(mv, 4, quad_cw, 6, &run, 1, 0, NULL, 0);
    /* blue at the bottom left, clockwise, dropping the counter-clockwise ones */
    world_quad(mv, 4, 32, 12, 40, 0xffffffff);
    run = model_run(0, 6, SW_CULL_CCW);
    run.textureArray = 0;
    run.textureLayer = 1;
    world_add(mv, 4, quad_cw, 6, &run, 1, 0, NULL, 0);
    /* green, animated in slot 1, at the right in its slot's own space */
    world_quad(mv, 40, 16, 48, 24, 0xffffffff);
    run = model_run(0, 6, SW_CULL_NONE);
    run.textureArray = 1;
    world_add(mv, 4, quad_cw, 6, &run, 1, 1, NULL, 1);
    identity_slot(&slot, 0);
    world_send(&slot, 1);
    test_view(&view);
    model_draw(&draw);
    gfx_set_texture(0, NULL);
    for (way = 0; way < 3; way++) {
        /* 0: the whole screen, the animated quad where it is; 1: the left half, the animated quad
           moved 32 left into it; 2: the left half, the animated quad moved 16 right, on the screen
           but beyond the half */
        memset(move, 0, sizeof move);
        move[0][0] = move[0][4] = move[0][8] = 1.0f;
        move[0][9] = way == 1 ? -32.0f : way == 2 ? 16.0f : 0.0f;
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
        memset(&frame, 0, sizeof frame);
        frame.animated = (const float (*)[12])move;
        frame.animatedCount = 1;
        world_frame(&view, &frame);
        if (way) {
            p.proj[0] = view.proj[0];
            p.proj[1] = view.proj[1];
            p.centre[0] = view.centre[0];
            p.centre[1] = view.centre[1];
            p.size[0] = W / 2;
            p.size[1] = H;
            p.nearZ = view.nearZ;
            p.farZ = 1e6f;
            sw_frustum(&frame.frustum, &p, view.view);
            gfx_world_frame(&frame);
        }
        gfx_set_mode(DRAW_OPAQUE_CLAMP);
        gfx_world_draw(SW_PASS_OPAQUE_CLAMP, &draw);
        gfx_end_scene();
        assert(screen_pixel(d, 7, 7) == 0xff0000 && screen_pixel(d, 7, 35) == 0x0000ff);
        assert(screen_pixel(d, 43, 7) == (way ? 0u : 0x00ff00u));
        assert(screen_pixel(d, 43, 19) == (way ? 0u : 0x00ff00u));
        assert(screen_pixel(d, 11, 19) == (way == 1 ? 0x00ff00u : 0u));
        assert(screen_pixel(d, 59, 19) == 0);
    }
    gfx_release(red);
    gfx_release(blue);
    gfx_release(green);
}

/* The world's buffers and arrays grow while they hold what earlier frames put up: a mesh and a
   layer drawn in one frame still draw after a later frame placed so many vertices and indices
   that the shared buffers, and so many layers that the array (made with one), were made again,
   larger. */
static void check_world_growth(struct TestScreen *d)
{
    static const DWORD colours[4] = { 0xff00ff00, 0xff0000ff, 0xffffffff, 0xffffff00 };
    static WORD many[1800];
    gfx_surface *red = texture_square(GFX_FMT_8888, 2, 0xffff0000), *more[4];
    struct GfxMeshVertex mv[4];
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct GfxWorldDraw draw;
    struct SwRun run;
    int i, frameNo;
    world_begin();
    world_texture(red, 0, 0);
    world_quad(mv, 4, 4, 12, 12, 0xffffffff);
    run = model_run(0, 6, SW_CULL_NONE);
    run.textureArray = 0;
    world_add(mv, 4, quad_cw, 6, &run, 1, 0, NULL, 0);
    world_send(NULL, 0);
    test_view(&view);
    model_draw(&draw);
    gfx_set_texture(0, NULL);
    for (frameNo = 0; frameNo < 2; frameNo++) {
        if (frameNo) {
            /* 40 quads more, 164 vertices of 48 bytes where the vertex buffer began with 4,096
               bytes, a mesh of 1,800 indices where the index buffer began with 1,024, and four
               layers more */
            for (i = 0; i < 4; i++) {
                more[i] = texture_square(GFX_FMT_8888, 2, colours[i]);
                world_texture(more[i], 0, (uint16_t)(i + 1));
            }
            for (i = 0; i < 40; i++) {
                float x = i < 4 ? 16.0f + (float)i * 10.0f : 60.0f;
                world_quad(mv, x, 20, x + 4, 28, 0xffffffff);
                run = model_run(0, 6, SW_CULL_NONE);
                run.textureArray = 0;
                run.textureLayer = (uint16_t)(i < 4 ? i + 1 : 0);
                world_add(mv, 4, quad_cw, 6, &run, 1, 0, NULL, 0);
            }
            for (i = 0; i < 1800; i++)
                many[i] = quad_cw[i % 6];
            world_quad(mv, 60, 40, 62, 42, 0xffffffff);
            run = model_run(0, 1800, SW_CULL_NONE);
            world_add(mv, 4, many, 1800, &run, 1, 0, NULL, 0);
            world_send(NULL, 0);
        }
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
        memset(&frame, 0, sizeof frame);
        world_frame(&view, &frame);
        gfx_set_mode(DRAW_OPAQUE_CLAMP);
        gfx_world_draw(SW_PASS_OPAQUE_CLAMP, &draw);
        gfx_end_scene();
        assert(screen_pixel(d, 7, 7) == 0xff0000);
        for (i = 0; frameNo && i < 4; i++)
            assert(screen_pixel(d, 17 + i * 10, 23) == (colours[i] & 0xffffff));
        assert(screen_pixel(d, 61, 41) == (frameNo ? 0xffffffu : 0u));
    }
    gfx_release(red);
    for (i = 0; i < 4; i++)
        gfx_release(more[i]);
}

/* A texture's mipmaps in a layer of an array are what its own texture's are: once a record and a
   quad of the same 8 by 8 texture have drawn it, every level of the layer, downloaded, is the same
   bytes as the level of the texture. */
static void check_world_mips(struct TestScreen *d)
{
    static const WORD indices[6] = { 0, 1, 2, 2, 1, 3 };
    struct GfxMeshVertex mv[8];
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct GfxWorldDraw draw;
    struct gfx_lock lk;
    struct SwRun run;
    gfx_surface *t;
    DWORD layer[64], own[64];
    int x, y, s, size, level, levels;
    (void)d;
    gfx_set_mipmaps(1);
    t = gfx_create_texture(8, 8, GFX_FMT_8888);
    gfx_set_mipmaps(0);
    assert(t && gfx_lock(t, NULL, &lk) == 0);
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++) {
            DWORD c = 0xff000000u | (DWORD)(x * 36) << 16 | (DWORD)(y * 36) << 8 | (DWORD)((x ^ y) * 36);
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &c, 4);
        }
    gfx_unlock(t);
    world_begin();
    world_texture(t, 0, 0);
    for (s = 0; s < 2; s++) {
        size = s ? 2 : 4;
        world_quad(mv + s * 4, 8.0f + (float)s * 16.0f, 8, 8.0f + (float)s * 16.0f + (float)size, 8.0f + (float)size,
                   0xffffffff);
    }
    run = model_run(0, 6, SW_CULL_NONE);
    run.textureArray = 0;
    world_add(mv, 4, indices, 6, &run, 1, 0, NULL, 0);
    world_add(mv + 4, 4, indices, 6, &run, 1, 0, NULL, 0);
    world_send(NULL, 0);
    test_view(&view);
    model_draw(&draw);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    memset(&frame, 0, sizeof frame);
    world_frame(&view, &frame);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    gfx_world_draw(SW_PASS_OPAQUE_CLAMP, &draw);
    gfx_set_texture(0, t);
    draw_quad(8, 24, 12, 28, 0xffffffff);
    draw_quad(24, 24, 26, 26, 0xffffffff);
    gfx_end_scene();
    /* the client's rule gives an 8 by 8 texture three levels, 8, 4 and 2 texels wide */
    levels = gfx_get_levels(t);
    assert(levels == 3);
    for (level = 0; level < levels; level++) {
        size = 8 >> level;
        memset(layer, 0, sizeof layer);
        memset(own, 0xff, sizeof own);
        assert(gfx_read_world_level(0, 0, level, layer) == 0 && gfx_read_texture_level(t, level, own) == 0);
        assert(!memcmp(layer, own, (size_t)size * size * 4));
        if (level == 0)
            for (y = 0; y < 8; y++)
                for (x = 0; x < 8; x++)
                    assert((layer[y * 8 + x] & 0xffffff) == ((DWORD)(x * 36) << 16 | (DWORD)(y * 36) << 8 | (DWORD)((x ^ y) * 36)));
    }
    assert(gfx_read_world_level(0, 0, levels, layer) < 0 && gfx_read_texture_level(t, levels, own) < 0);
    assert(gfx_read_world_level(0, 1, 0, layer) < 0 && gfx_read_world_level(7, 0, 0, layer) < 0);
    gfx_set_texture(0, NULL);
    gfx_release(t);
}

/* The world of check_world_shadows: a caster over the ground at depth 50, world x -16 to 0 and y -8
   to 8, the left half of its texture opaque; the ground over the whole screen at depth 100, its
   lightmap baked in shadow (alpha 0x80, colour black); and a caster without a texture beside the
   first, x 4 to 20, which the shadow pass draws without the alpha test as it draws the ground. */
static void shadow_world(gfx_surface *t, gfx_surface *lm)
{
    struct GfxMeshVertex mv[12];
    struct SwRun run;
    int i;
    world_begin();
    world_texture(t, 0, 0);
    world_texture(lm, 1, 0);
    memset(mv, 0, sizeof mv);
    for (i = 0; i < 4; i++) {
        mv[i].x = i & 1 ? 0.0f : -16.0f;
        mv[i].y = i & 2 ? 8.0f : -8.0f;
        mv[i].z = 50.0f;
        mv[i].diffuse = 0xffffffff;
        mv[i].u0 = (float)(i & 1);
        mv[i].v0 = (float)(i >> 1);
    }
    run = model_run(0, 6, SW_CULL_NONE);
    run.textureArray = 0;
    world_add(mv, 4, quad_cw, 6, &run, 1, 0, NULL, 0);
    world_quad(mv + 4, 0, 0, W, H, 0xffffffff);
    for (i = 4; i < 8; i++)
        mv[i].u1 = mv[i].v1 = 0.5f;
    run = model_run(0, 6, SW_CULL_NONE);
    run.pass = SW_PASS_GROUND;
    run.lightmapArray = 1;
    world_add(mv + 4, 4, quad_cw, 6, &run, 1, 0, NULL, 0);
    for (i = 8; i < 12; i++) {
        mv[i] = mv[i - 8];
        mv[i].x += 20.0f;
    }
    run = model_run(0, 6, SW_CULL_NONE);
    world_add(mv + 8, 4, quad_cw, 6, &run, 1, 0, NULL, 0);
    world_send(NULL, 0);
}

/* A frame of the shadow world: 0 with shadows; 1 with shadows whose reach (the far plane, 40) ends
   before every receiver; 2 without; the light along +z from the eye, the camera 100 from its focus. */
static void shadow_frame(int way)
{
    static const float light[3] = { 0.0f, 0.0f, 1.0f };
    static const struct ShadowFitBox bounds = { { -40.0f, -30.0f, 40.0f }, { 40.0f, 30.0f, 110.0f } };
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct GfxWorldDraw ground;
    struct GfxShadow shadow;
    struct ShadowFitCamera camera;
    test_view(&view);
    memset(&camera, 0, sizeof camera);
    memcpy(camera.view, view.view, sizeof camera.view);
    memcpy(camera.proj, view.proj, sizeof camera.proj);
    memcpy(camera.centre, view.centre, sizeof camera.centre);
    camera.width = W;
    camera.height = H;
    camera.nearZ = view.nearZ;
    camera.farZ = way == 1 ? 40.0f : 1e6f;
    camera.distance = 100.0f;
    memset(&ground, 0, sizeof ground);
    ground.kind = GFX_MESH_GROUND;
    ground.colour[0] = ground.colour[1] = ground.colour[2] = ground.colour[3] = 1.0f;
    memset(&shadow, 0, sizeof shadow);
    assert(shadow_fit_frame(&shadow.fit, &camera, light) && shadow_fit_bounds(&shadow.fit, &bounds, 1));
    shadow.darkness = 0.5f;
    shadow.ambient[0] = shadow.ambient[1] = shadow.ambient[2] = 0.5f;
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    memset(&frame, 0, sizeof frame);
    frame.shadow = way < 2 ? &shadow : NULL;
    world_frame(&view, &frame);
    gfx_set_mode(DRAW_GROUND_LIGHTMAP);
    gfx_world_draw_record(&g_world.records[1], &ground);
    gfx_end_scene();
}

/* The frame's shadows, fitted, culled and drawn by the device itself (render/gfx.h; world_depth.hlsl,
   shadow_*.hlsl, world_cull.hlsl, world_caster.hlsl, world_caster_solid.hlsl): a model's record
   casts where its texture's alpha passes the opaque lists' test (0xcf), one without a texture
   everywhere, both in one cascade beside the ground, and the ground drawn in that frame lies in
   their shadows there, darkened to the shadow's darkness, and is lit around them and where the
   texture's alpha drops the caster; its lightmap's baked shadow counts as lit down to the darkness. A frame
   whose receivers all lie beyond the shadows' reach keeps the baked shadow everywhere, and so does
   a frame given no shadows. The fit keeps its squares and its range from frame to frame
   (gfx_read_shadow_fit), and a world made again (gfx_world_clear) starts from none. */
static void check_world_shadows(struct TestScreen *d)
{
    static const DWORD half[4] = { 0xffffffff, 0x00ffffff, 0xffffffff, 0x00ffffff };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, half), *lm = texture_square(GFX_FMT_8888, 2, 0x80000000);
    struct GfxShadowFitRead read;
    DWORD p;
    int way, c, used = 0;
    if (!gfx_shadows_supported())
        return;
    shadow_world(t, lm);
    for (way = 0; way < 3; way++) {
        shadow_frame(way);
        /* the opaque half shades the ground at x -16 to -8 (pixels 16 to 23 across, 16 to 31 down),
           and the caster without a texture at x 4 to 20 (pixels 36 to 51) */
        p = screen_pixel(d, 19, 24);
        assert(near_value(p >> 16 & 0xff, 0x80) && near_value(p >> 8 & 0xff, 0x80) && near_value(p & 0xff, 0x80));
        p = screen_pixel(d, 44, 24);
        assert(near_value(p >> 16 & 0xff, 0x80) && near_value(p >> 8 & 0xff, 0x80) && near_value(p & 0xff, 0x80));
        assert(screen_pixel(d, 33, 24) == (way ? 0x808080u : 0xffffffu));
        p = way ? 0x808080 : 0xffffff;
        assert(screen_pixel(d, 29, 24) == p && screen_pixel(d, 10, 24) == p && screen_pixel(d, 19, 10) == p);
    }
    /* a frame without shadows has no fit to read */
    memset(&read, 0, sizeof read);
    assert(gfx_read_shadow_fit(&read) < 0);
    /* two frames with them: the second's squares and range kept from the first */
    shadow_frame(0);
    shadow_frame(0);
    assert(gfx_read_shadow_fit(&read) == 0 && read.fit.cascades.cascades == GFX_SHADOW_CASCADES);
    assert(read.fit.previousRange.nearest > 0.0f && read.fit.previousRange.farthest >= read.fit.previousRange.nearest);
    assert(!memcmp(&read.fit.previousRange, &read.fit.kept, sizeof read.fit.kept));
    for (c = 0; c < GFX_SHADOW_CASCADES; c++)
        if (read.fit.cascades.lo[c][0] <= read.fit.cascades.hi[c][0]) {
            assert(read.fit.previous[c] > 0.0f && read.fit.previous[c] == read.fit.cascades.kept[c]);
            used++;
        }
    assert(used > 0);
    /* the world made again: nothing kept, and nothing to read until a frame fits again */
    shadow_world(t, lm);
    assert(gfx_read_shadow_fit(&read) < 0);
    shadow_frame(0);
    assert(gfx_read_shadow_fit(&read) == 0 && read.fit.cascades.cascades == GFX_SHADOW_CASCADES);
    assert(read.fit.previousRange.nearest == 0.0f && read.fit.previousRange.farthest == 0.0f);
    for (c = 0; c < GFX_SHADOW_CASCADES; c++)
        assert(read.fit.previous[c] == 0.0f);
    gfx_release(t);
    gfx_release(lm);
}

/* A world cleared after its draws were recorded and before the frame went up (gfx_world_clear, as
   textures reloaded or a new map clear it): those draws are left out and read nothing released;
   the world built again draws. */
static void check_world_cleared(struct TestScreen *d)
{
    struct GfxMeshVertex mv[4];
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct GfxWorldDraw draw;
    struct SwRun run;
    int again;
    test_view(&view);
    model_draw(&draw);
    gfx_set_texture(0, NULL);
    for (again = 0; again < 2; again++) {
        world_begin();
        world_quad(mv, 4, 4, 12, 12, 0xffff0000);
        run = model_run(0, 6, SW_CULL_NONE);
        world_add(mv, 4, quad_cw, 6, &run, 1, 0, NULL, 0);
        world_send(NULL, 0);
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
        memset(&frame, 0, sizeof frame);
        world_frame(&view, &frame);
        gfx_set_mode(DRAW_OPAQUE_CLAMP);
        gfx_world_draw(SW_PASS_OPAQUE_CLAMP, &draw);
        gfx_world_draw_record(&g_world.records[0], &draw);
        if (!again)
            gfx_world_clear();
        gfx_end_scene();
        assert(screen_pixel(d, 7, 7) == (again ? 0xff0000u : 0u));
    }
}

int main(int argc, char **argv)
{
    struct TestScreen *d = &g_screen;
    struct TexDrawVertex v[4];
    gfx_surface *rt, *sys;
    struct gfx_lock lk;
    int x, y, presented, linear;
    if (argc > 1)
        SDL_setenv_unsafe("RAGEXE_MSAA", argv[1], 1);
    multisampled = argc > 1 ? atoi(argv[1]) > 1 : 1;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->window = SDL_CreateWindow("gfx test", W, H, SDL_WINDOW_HIDDEN);
    if (!d->window) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->width = W;
    d->height = H;
    assert(gfx_world_attach());
    assert(gfx_open(d->window) == 0);
    d->screen = gfx_create_screen(d->width, d->height);
    if (!d->screen) {
        fprintf(stderr, "skipped: no GPU device\n");
        return 77;
    }
    assert(gfx_get_caps(&g_gfxCaps) == 0);
    assert(gfx_create_zbuffer() == 0);

    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff102030, 1.0f, NULL);
    assert(screen_pixel(d, 0, 0) == 0x102030 && screen_pixel(d, W - 1, H - 1) == 0x102030);

    gfx_set_mode(FILTERED);
    gfx_set_texture(0, NULL);
    quad(v, 8, 8, 24, 20, 0xffff0000);
    gfx_draw_vertices(v, 4, 1);
    gfx_end_scene();
    assert(screen_pixel(d, 8, 8) == 0xff0000 && screen_pixel(d, 23, 19) == 0xff0000);
    assert(screen_pixel(d, 7, 8) == 0x102030 && screen_pixel(d, 24, 19) == 0x102030 && screen_pixel(d, 8, 20) == 0x102030);

    /* edges through pixel centres: the top-left rule takes the top and left ones, multisampled
       too, as Direct3D 9 drew flat quads without multisampling */
    gfx_clear(GFX_CLEAR_TARGET, 0xff102030, 1.0f, NULL);
    quad(v, 8.5f, 8.5f, 24.5f, 20.5f, 0xff00ff00);
    gfx_draw_vertices(v, 4, 1);
    gfx_end_scene();
    assert(screen_pixel(d, 8, 8) == 0x00ff00 && screen_pixel(d, 23, 19) == 0x00ff00);
    assert(screen_pixel(d, 7, 8) == 0x102030 && screen_pixel(d, 8, 7) == 0x102030);
    assert(screen_pixel(d, 24, 19) == 0x102030 && screen_pixel(d, 23, 20) == 0x102030);
    /* and a flat quad between pixel centres keeps its texels where the centres read them */
    {
        static const DWORD texels[4] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff };
        gfx_surface *t = texture_2x2(GFX_FMT_8888, texels);
        gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
        gfx_set_mode(TEXTURE);
        gfx_set_texture(0, t);
        quad(v, 10.25f, 10.25f, 30.25f, 30.25f, 0xffffffff);
        gfx_draw_vertices(v, 4, 1);
        gfx_end_scene();
        /* Direct3D 9 x and y from 9.75 to 29.75: the centres 10 to 29, texels changing after 19 */
        assert(screen_pixel(d, 9, 10) == 0 && screen_pixel(d, 10, 9) == 0);
        assert(screen_pixel(d, 10, 10) == 0xff0000 && screen_pixel(d, 19, 19) == 0xff0000);
        assert(screen_pixel(d, 20, 10) == 0x00ff00 && screen_pixel(d, 10, 20) == 0x0000ff && screen_pixel(d, 29, 29) == 0xffffff);
        assert(screen_pixel(d, 30, 29) == 0 && screen_pixel(d, 29, 30) == 0);
        gfx_set_texture(0, NULL);
        gfx_release(t);
    }

    check_texture(d, GFX_FMT_8888);
    check_texture(d, GFX_FMT_565);
    check_texture(d, GFX_FMT_1555);
    check_texture(d, GFX_FMT_4444);
    check_texel_centres(d);
    check_culling(d);
    check_texture_changed_in_frame(d);
    check_release_in_frame(d);
    check_release_while_held(d);

    {
        static const DWORD keyed[4] = { 0x00ff0000, 0xff00ff00, 0xff00ff00, 0x00ff0000 };
        gfx_surface *t = texture_2x2(GFX_FMT_1555, keyed);
        /* the opaque lists' alpha test, at least 0xcf */
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
        gfx_set_mode(DRAW_OPAQUE_CLAMP);
        gfx_set_texture(0, t);
        quad(v, 0, 0, 32, 32, 0xffffffff);
        gfx_draw_vertices(v, 4, 1);
        gfx_end_scene();
        assert(screen_pixel(d, 4, 4) == 0 && screen_pixel(d, 28, 4) == 0x00ff00 && screen_pixel(d, 28, 28) == 0);
        gfx_set_texture(0, NULL);
        gfx_release(t);
    }

    check_fog(d);

    /* blended by the source's alpha (the interface's mode) */
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gfx_set_mode(DRAW_OVERLAY);
    quad(v, 0, 0, 16, 16, 0x80ffffff);
    gfx_draw_vertices(v, 4, 1);
    gfx_end_scene();
    x = (int)(screen_pixel(d, 4, 4) & 0xff);
    assert(x >= 0x7e && x <= 0x82);

    check_effects(d);
    check_presets(d);
    check_two_textures(d);
    check_ground(d);
    check_two_pass_alpha(d);
    check_effect_depth(d);
    check_composite(d);
    check_clear_inside(d);
    check_overlay_batch(d);

    rt = gfx_create_render_target(16, 16);
    assert(rt);
    assert(gfx_set_render_target(rt) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    /* a mode that does not draw into a render target's texture draws nothing there */
    gfx_set_mode(DRAW_OVERLAY);
    quad(v, 0, 8, 16, 16, 0xffff0000);
    gfx_draw_vertices(v, 4, 1);
    gfx_set_mode(COLOUR);
    quad(v, 0, 0, 16, 8, 0xff0000ff);
    gfx_draw_vertices(v, 4, 1);
    gfx_end_scene();
    assert(gfx_set_render_target(d->screen) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gfx_set_mode(TEXTURE);
    gfx_set_texture(0, rt);
    quad(v, 0, 0, 16, 16, 0xffffffff);
    gfx_draw_vertices(v, 4, 1);
    gfx_end_scene();
    assert(screen_pixel(d, 4, 2) == 0x0000ff && screen_pixel(d, 4, 12) == 0);
    gfx_set_texture(0, NULL);
    gfx_release(rt);
    check_modes_counted();

    sys = gfx_create_surface(8, 8);
    assert(sys && gfx_lock(sys, NULL, &lk) == 0);
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++) {
            DWORD c = y < 4 ? 0x00ffff00 : 0x00ff00ff;
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &c, 4);
        }
    gfx_unlock(sys);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    {
        RECT dst = {10, 10, 18, 18};
        assert(gfx_blit(d->screen, &dst, sys, NULL) == 0);
    }
    assert(screen_pixel(d, 12, 11) == 0xffff00 && screen_pixel(d, 12, 16) == 0xff00ff && screen_pixel(d, 9, 9) == 0);
    gfx_release(sys);

    check_mipmaps(d);
    check_layers(d);
    check_partial_uploads(d);
    check_fresh_uploads(d);
    check_triangles(d);
    check_target_copies(d);
    check_read_frame(d);
    check_target_edges(d);
    linear = check_window_target(d);
    check_wobble(d);
    check_clears(d);
    check_skin(d);
    check_corners(d);
    check_world(d);
    check_slots(d);
    check_water(d);
    check_world_cull(d);
    check_world_growth(d);
    check_world_mips(d);
    check_world_shadows(d);
    check_world_cleared(d);

    check_resize(d);
    presented = check_present(d);
    gfx_close();
    gfx_release(d->screen);
    d->screen = NULL;
    SDL_DestroyWindow(d->window);
    SDL_Quit();
    printf("gfx gpu (RAGEXE_MSAA=%s): clears, edges and flat quads, 8888/565/1555/4444 textures, texel centres, "
           "culling, textures changed, released and held in a frame, alpha test, fog (the quads' within a step at every "
           "step's start), blending, the effect modes' blending, the 16 presets against Direct3D 9's stages, two "
           "textures, the ground's records, "
           "two-pass alpha, effects behind depth, composites, clears inside a pass, the overlays batched, mipmaps, "
           "layered textures read as plain ones (filtered and mipmapped, one draw of several layers, a full "
           "array and a second, a layer given up and taken, arrays of 16 MB), only the texels written going up "
           "(plain and layered, with and without mipmaps, more cells than rectangles kept; a texture not yet "
           "sent cleared on the GPU but where it was written), lists of triangles, "
           "render targets and their edges, a window's target read 1:1 (%s), the wobble in a frame and twice in "
           "one, modes kept from targets "
           "they do not draw into, modes counted, system surface blits, copies between targets and out of "
           "bounds, frames read whole and in part, Granny's models skinned, batched and held, the world's records, transform "
           "slots, texture-coordinate "
           "matrices, the water, the world's cull compacted by run, its buffers and arrays grown, its mipmaps, "
           "casters and shadows, a world cleared in a frame, resizing, %s passed\n",
           argc > 1 ? argv[1] : "4", linear ? "by the nearest texel and filtered" : "by the nearest texel only",
           presented ? "the window claimed when shown and presented into"
                     : "the window unclaimed while hidden (no swapchain to present into on this video driver)");
    return 0;
}
