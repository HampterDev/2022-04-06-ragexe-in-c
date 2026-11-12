/* render/gfx_gpu.c through render/gfx.h on a real GPU, in a hidden window: what each draw puts on
   the target, read back through gfx_lock. Skipped (77) where SDL finds no GPU device. The argument
   is RAGEXE_MSAA: the test runs once without multisampling and once with it. */
#include "render/gfx.h"
#include "render/device.h"
#include "render/vertex_fog.h"
#include "diag/perf.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct perf_counters g_perf;
struct C3dDevice g_3dDevice;

#define W 64
#define H 48

struct Vertex {
    float x, y, z, rhw;
    DWORD diffuse;
    float u, v;
};

static DWORD screen_pixel(struct C3dDevice *d, int x, int y)
{
    struct gfx_lock lk;
    DWORD p;
    assert(gfx_lock(d->target, NULL, 0, &lk) == 0);
    assert(lk.bpp == 32 && lk.width == W && lk.height == H);
    memcpy(&p, (const BYTE *)lk.bits + y * lk.pitch + x * 4, 4);
    gfx_unlock(d->target);
    return p & 0xffffff;
}

static void quad(struct Vertex *v, float x0, float y0, float x1, float y1, DWORD diffuse)
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
   the vertex colour; the texture times the vertex colour, filtered linearly; none of them blends or
   touches depth. */
#define TEXTURE DRAW_BLIT
#define COLOUR DRAW_CLEAR_COLOUR
#define FILTERED DRAW_PROBE

static void fill_2x2(gfx_surface *t, int fmt, const DWORD argb[4])
{
    struct gfx_lock lk;
    int i;
    assert(gfx_lock(t, NULL, 0, &lk) == 0);
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
    assert(t && fmt == GFX_FMT_8888 && gfx_lock(t, NULL, 0, &lk) == 0);
    for (y = 0; y < n; y++)
        for (x = 0; x < n; x++)
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &argb, 4);
    gfx_unlock(t);
    return t;
}

static void draw_quad(float x0, float y0, float x1, float y1, DWORD diffuse)
{
    struct Vertex v[4];
    quad(v, x0, y0, x1, y1, diffuse);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
}

static void check_texture(struct C3dDevice *d, int fmt)
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
static void check_texel_centres(struct C3dDevice *d)
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
   the world vertices. */
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
static void check_culling(struct C3dDevice *d)
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
static void check_texture_changed_in_frame(struct C3dDevice *d)
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
static void check_release_in_frame(struct C3dDevice *d)
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

/* A copy from a target into a texture, read back through a lock, and a target's copy saved and
   put back (c3d_device_save_frame and c3d_device_restore_frame). */
static void check_target_copies(struct C3dDevice *d)
{
    gfx_surface *rt = gfx_create_render_target(16, 16), *tex = gfx_create_texture(16, 16, GFX_FMT_8888);
    struct gfx_lock lk;
    RECT r = { 0, 0, 16, 16 };
    DWORD p;
    assert(rt && tex);
    assert(gfx_set_render_target(rt) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    draw_quad(0, 8, 16, 16, 0xff00ff00);
    gfx_end_scene();
    assert(gfx_set_render_target(d->target) == 0);
    assert(gfx_blit(tex, &r, rt, &r, GFX_BLT_WAIT) == 0);
    assert(gfx_lock(tex, NULL, 0, &lk) == 0);
    memcpy(&p, (const BYTE *)lk.bits + 2 * lk.pitch + 2 * 4, 4);
    assert((p & 0xffffff) == 0);
    memcpy(&p, (const BYTE *)lk.bits + 12 * lk.pitch + 2 * 4, 4);
    assert((p & 0xffffff) == 0x00ff00);
    gfx_unlock(tex);

    /* stretched into a texture drawn from before in the frame: the texture is made a colour
       target and keeps its pixels outside the copy; the draw before keeps the old ones */
    {
        RECT half = { 0, 0, 8, 8 }, rest = { 0, 8, 16, 16 };
        gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
        gfx_set_mode(TEXTURE);
        gfx_set_texture(0, tex);
        draw_quad(0, 0, 16, 16, 0xffffffff);
        gfx_set_texture(0, NULL);
        assert(gfx_blit(tex, &half, rt, &rest, GFX_BLT_WAIT) == 0);
        gfx_end_scene();
        assert(screen_pixel(d, 2, 2) == 0 && screen_pixel(d, 2, 12) == 0x00ff00);
        assert(gfx_lock(tex, NULL, 0, &lk) == 0);
        memcpy(&p, (const BYTE *)lk.bits + 2 * lk.pitch + 2 * 4, 4);
        assert((p & 0xffffff) == 0x00ff00);
        memcpy(&p, (const BYTE *)lk.bits + 2 * lk.pitch + 12 * 4, 4);
        assert((p & 0xffffff) == 0);
        memcpy(&p, (const BYTE *)lk.bits + 12 * lk.pitch + 2 * 4, 4);
        assert((p & 0xffffff) == 0x00ff00);
        gfx_unlock(tex);
    }
    /* rectangles outside their surfaces copy nothing */
    {
        RECT outside = { 8, 8, 17, 16 }, negative = { -1, 0, 4, 4 }, empty = { 4, 4, 4, 8 }, beyond = { 60, 40, 70, 48 };
        gfx_set_render_target(rt);
        gfx_clear(GFX_CLEAR_TARGET, 0xffff0000, 1.0f, NULL);
        gfx_set_render_target(d->target);
        assert(gfx_blit(tex, &outside, rt, NULL, GFX_BLT_WAIT) == 0);
        assert(gfx_blit(tex, NULL, rt, &negative, GFX_BLT_WAIT) == 0);
        assert(gfx_blit(tex, &empty, rt, &empty, GFX_BLT_WAIT) == 0);
        assert(gfx_blit(d->target, &beyond, tex, NULL, GFX_BLT_WAIT) == 0);
        assert(gfx_lock(tex, NULL, 0, &lk) == 0);
        memcpy(&p, (const BYTE *)lk.bits + 12 * lk.pitch + 12 * 4, 4);
        assert((p & 0xffffff) == 0x00ff00);
        gfx_unlock(tex);
    }
    gfx_release(rt);
    gfx_release(tex);

    gfx_clear(GFX_CLEAR_TARGET, 0xff203040, 1.0f, NULL);
    assert(gfx_blit(d->save, NULL, d->target, NULL, GFX_BLT_WAIT) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    assert(screen_pixel(d, 30, 30) == 0);
    assert(gfx_blit(d->target, NULL, d->save, NULL, GFX_BLT_WAIT) == 0);
    draw_quad(0, 0, 4, 4, 0xffffffff);
    gfx_end_scene();
    assert(screen_pixel(d, 30, 30) == 0x203040);
    assert(screen_pixel(d, 2, 2) == 0xffffff);
}

/* Pixels written into a locked target stay there, under the draws after them. */
static void check_target_write_back(struct C3dDevice *d)
{
    struct gfx_lock lk;
    DWORD c = 0xff123456;
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    assert(gfx_lock(d->target, NULL, 0, &lk) == 0);
    memcpy((BYTE *)lk.bits + 40 * lk.pitch + 40 * 4, &c, 4);
    gfx_unlock(d->target);
    gfx_set_mode(COLOUR);
    draw_quad(0, 0, 4, 4, 0xffffffff);
    gfx_end_scene();
    assert(screen_pixel(d, 40, 40) == 0x123456);
    assert(screen_pixel(d, 2, 2) == 0xffffff);
}

/* A target of odd size drawn enlarged with linear filtering and clamped addressing reads its own
   last row and column at its edges, nothing past them: its texture is wider than it. */
static void check_target_edges(struct C3dDevice *d)
{
    gfx_surface *rt = gfx_create_render_target(15, 9);
    assert(rt);
    assert(gfx_set_render_target(rt) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xffff0000, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    draw_quad(0, 0, 15, 9, 0xff00ff00);
    gfx_end_scene();
    assert(gfx_set_render_target(d->target) == 0);
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

/* A lock of a rectangle of a target reads that rectangle, and what it writes there goes back;
   rectangles outside the target lock nothing. */
static void check_target_rect_lock(struct C3dDevice *d)
{
    struct gfx_lock lk;
    RECT r = { 20, 10, 30, 14 }, outside = { 60, 40, 65, 44 };
    DWORD c = 0xff654321, p;
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    draw_quad(0, 0, W, H, 0xff0000ff);
    gfx_end_scene();
    assert(gfx_lock(d->target, &outside, 0, &lk) < 0);
    assert(gfx_lock(d->target, &r, 0, &lk) == 0);
    assert(lk.pitch == W * 4);
    memcpy(&p, lk.bits, 4);
    assert((p & 0xffffff) == 0x0000ff);
    memcpy((BYTE *)lk.bits + 3 * lk.pitch + 9 * 4, &c, 4);
    gfx_unlock(d->target);
    assert(screen_pixel(d, 29, 13) == 0x654321 && screen_pixel(d, 28, 13) == 0x0000ff
           && screen_pixel(d, 30, 13) == 0x0000ff && screen_pixel(d, 2, 2) == 0x0000ff);
}

/* A clear of a rectangle after draws in the same pass, of the colour and of the depth alone. */
static void check_clears(struct C3dDevice *d)
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
        struct Vertex v[4];
        quad(v, 0, 0, 32, 32, 0xffff0000);
        gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
        gfx_clear(GFX_CLEAR_ZBUFFER, 0, 1.0f, &r);
        quad(v, 0, 0, 32, 32, 0xff00ff00);
        v[0].z = v[1].z = v[2].z = v[3].z = 0.75f;
        gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    }
    gfx_end_scene();
    assert(screen_pixel(d, 12, 12) == 0x00ff00 && screen_pixel(d, 4, 4) == 0xff0000);
}

/* Mipmaps are made on the GPU from the top level whenever the texels go up: a black and white
   checker drawn at half its size with a mip filter reads the grey of the level below, and again
   after the texels change. */
static void check_mipmaps(struct C3dDevice *d)
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
        assert(gfx_lock(t, NULL, 0, &lk) == 0);
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

/* World vertices and the world's records land on the pixels the same corners pre-transformed
   cover. */
static void check_world(struct C3dDevice *d)
{
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct Vertex v[4];
    struct GfxMeshVertex mv[6];
    static const WORD indices[6] = { 0, 1, 2, 3, 4, 5 };
    static const int corner[6] = { 0, 1, 2, 2, 1, 3 };
    struct GfxWorldDraw draw;
    struct SwRun runs[2];
    int i;
    test_view(&view);
    gfx_set_world_camera(&view);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    /* world x = -8..0, y = -8..0 at depth 100 cover the pixels 24..31, 16..23 */
    quad(v, 0, 0, 1, 1, 0xff00ff00);
    for (i = 0; i < 4; i++) {
        v[i].x = (i & 1) ? 0.0f - 0.5f : -8.0f - 0.5f;
        v[i].y = (i & 2) ? 0.0f - 0.5f : -8.0f - 0.5f;
        v[i].z = 100.0f;
    }
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_WORLD, v, 4, NULL, 0);
    gfx_end_scene();
    assert(screen_pixel(d, 24, 16) == 0x00ff00 && screen_pixel(d, 31, 23) == 0x00ff00);
    assert(screen_pixel(d, 23, 16) == 0 && screen_pixel(d, 32, 23) == 0 && screen_pixel(d, 24, 15) == 0
           && screen_pixel(d, 24, 24) == 0);

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
static void check_slots(struct C3dDevice *d)
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
static void check_water(struct C3dDevice *d)
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

/* A window resized: the screen and its copy take the new size, keep drawing and lock at it, and go
   back. */
static void check_resize(struct C3dDevice *d)
{
    struct gfx_lock lk;
    assert(gfx_resize(d, 80, 40) == 0);
    d->width = 80;
    d->height = 40;
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff405060, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    draw_quad(70, 30, 80, 40, 0xffffffff);
    gfx_end_scene();
    assert(gfx_lock(d->target, NULL, 0, &lk) == 0);
    assert(lk.width == 80 && lk.height == 40 && lk.pitch == 80 * 4);
    assert((((const DWORD *)lk.bits)[0] & 0xffffff) == 0x405060);
    assert((((const DWORD *)((const BYTE *)lk.bits + 39 * lk.pitch))[79] & 0xffffff) == 0xffffff);
    gfx_unlock(d->target);
    assert(gfx_blit(d->save, NULL, d->target, NULL, GFX_BLT_WAIT) == 0);
    assert(gfx_resize(d, W, H) == 0);
    d->width = W;
    d->height = H;
    gfx_clear(GFX_CLEAR_TARGET, 0xff102030, 1.0f, NULL);
    assert(screen_pixel(d, W - 1, H - 1) == 0x102030);
}

/* The fog of the modes that fog, while it is on (render/gfx.h, GfxFog): the quads' by their rhw,
   in 1/255 steps, the meshes' as the Direct3D 9 client's mesh shader had it. */
static void fog_quad(float x, float y, float size, float rhw, int kind)
{
    struct Vertex v[4];
    int i;
    quad(v, x, y, x + size, y + size, 0xffff0000);
    for (i = 0; i < 4; i++) {
        v[i].rhw = rhw;
        if (kind == GFX_VTX_WORLD) {
            /* at depth 100 a unit is a pixel, the view's centre at (32, 24) */
            v[i].x = (i & 1 ? x + size : x) - 32.0f - 0.5f;
            v[i].y = (i & 2 ? y + size : y) - 24.0f - 0.5f;
            v[i].z = 100.0f;
        }
    }
    gfx_draw(GFX_PT_TRIANGLESTRIP, kind, v, 4, NULL, 0);
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

/* The quads' fog to the bit: white quads fogged into black keep the original's step in their red,
   at every step's first rhw and the rhw before it, for the maps' fog (0.2 to 0.8) and for an end
   before the start, in pre-transformed and in world vertices. */
static void check_fog_steps(struct C3dDevice *d)
{
    static const float ranges[2][2] = { { 0.2f, 0.8f }, { 0.7f, 0.3f } };
    static const int kinds[2] = { GFX_VTX_TEX1, GFX_VTX_WORLD };
    struct VertexFogTable table;
    struct GfxWorldView view;
    struct GfxFog fog;
    int r, k, i, wrong = 0;
    test_view(&view);
    gfx_set_world_camera(&view);
    for (r = 0; r < 2; r++)
        for (k = 0; k < 2; k++) {
            int count;
            memset(&fog, 0, sizeof fog);
            fog.on = 1;
            fog.colour = 0xff000000;
            fog.start = ranges[r][0];
            fog.end = ranges[r][1];
            fog.origin = 10.0f;
            fog.span = 1490.0f;
            vertex_fog_table(fog.start, fog.end, fog.origin, fog.span, &table);
            count = (int)table.count;
            assert(count > 200);
            gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
            gfx_set_mode(DRAW_OPAQUE_CLAMP);
            gfx_set_texture(0, NULL);
            gfx_set_fog(&fog);
            /* two pixels square each, 31 to a row off the first row and column (of which the
               viewport holds half): a step's first rhw, then the one before */
            for (i = 0; i < 2 * count; i++)
                fog_quad((float)(1 + i % 31 * 2), (float)(1 + i / 31 * 2), 2.0f,
                         next_float(table.rhw[i / 2], -(i & 1)), kinds[k]);
            gfx_end_scene();
            gfx_set_fog(NULL);
            for (i = 0; i < 2 * count; i++) {
                float rhw = next_float(table.rhw[i / 2], -(i & 1));
                int want = vertex_fog_step(rhw, fog.start, fog.end, fog.origin, fog.span);
                int got = (int)(screen_pixel(d, 1 + i % 31 * 2, 1 + i / 31 * 2) >> 16 & 0xff);
                if (got != want && wrong++ < 8)
                    fprintf(stderr, "fog %g..%g, %s rhw %.9g: %d, the original %d\n", fog.start, fog.end,
                            kinds[k] == GFX_VTX_WORLD ? "world" : "screen", rhw, got, want);
            }
        }
    assert(!wrong);
}

static void check_fog(struct C3dDevice *d)
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
    fog_quad(0, 0, 8, 1.0f / 50, GFX_VTX_TEX1);   /* half way: 127 of 255 kept */
    fog_quad(8, 0, 8, 1.0f / 150, GFX_VTX_TEX1);  /* beyond the end: all fog */
    fog_quad(16, 0, 8, 1.0f, GFX_VTX_TEX1);       /* 0.01 of the way: 252 kept */
    /* a mode without fog */
    gfx_set_mode(DRAW_OVERLAY);
    fog_quad(24, 0, 8, 1.0f / 150, GFX_VTX_TEX1);
    /* the end where the start is: the depth's share alone */
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    fog.start = fog.end = 0.25f;
    gfx_set_fog(&fog);
    fog_quad(32, 0, 8, 1.0f / 50, GFX_VTX_TEX1);
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
    fog_quad(40, 0, 8, 1.0f / 150, GFX_VTX_TEX1);
    gfx_end_scene();
    /* the quads' steps exactly: 127 of red and 128 of the fog's green */
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
static void check_effects(struct C3dDevice *d)
{
    static const DWORD grey[4] = { 0xff404040, 0xff404040, 0xff404040, 0xff404040 };
    static const DWORD half[4] = { 0xff808080, 0xff808080, 0xff808080, 0xff808080 };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, grey), *u = texture_2x2(GFX_FMT_8888, half);
    struct Vertex v[4];
    int i;
    gfx_clear(GFX_CLEAR_TARGET, 0xff102030, 1.0f, NULL);
    gfx_set_mode(DRAW_EFFECT_OVERLAY);
    gfx_set_texture(0, t);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ONE, 3);
    quad(v, 0, 0, 16, 16, 0xff200000);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_set_effect(DRAW_BLEND_SRCALPHA, DRAW_BLEND_DESTALPHA, 0);
    quad(v, 16, 0, 32, 16, 0x80ffffff);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_set_texture(1, u);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ZERO, 5);
    quad(v, 0, 16, 16, 32, 0xffffffff);
    for (i = 0; i < 4; i++) {
        v[i].u *= 2.0f;
        v[i].v *= 2.0f;
    }
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
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
static void check_presets(struct C3dDevice *d)
{
    static const DWORD tc[4] = { 0xc060a030, 0xc060a030, 0xc060a030, 0xc060a030 };
    static const DWORD uc[4] = { 0x709020d0, 0x709020d0, 0x709020d0, 0x709020d0 };
    static const double t[4] = { 0x60 / 255.0, 0xa0 / 255.0, 0x30 / 255.0, 0xc0 / 255.0 };
    static const double u[4] = { 0x90 / 255.0, 0x20 / 255.0, 0xd0 / 255.0, 0x70 / 255.0 };
    static const double diffuse[4] = { 0xb0 / 255.0, 0x50 / 255.0, 0x80 / 255.0, 0xe0 / 255.0 };
    gfx_surface *tt = texture_2x2(GFX_FMT_8888, tc), *ut = texture_2x2(GFX_FMT_8888, uc);
    struct Vertex v[4];
    int preset;
    gfx_clear(GFX_CLEAR_TARGET, 0xffffffff, 1.0f, NULL);
    gfx_set_mode(DRAW_EFFECT_OVERLAY);
    gfx_set_texture(0, tt);
    gfx_set_texture(1, ut);
    for (preset = 0; preset < 16; preset++) {
        int combine = draw_effect_combine(preset, 0);
        gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ZERO, combine);
        quad(v, (float)(preset * 4 + 1), 1, (float)(preset * 4 + 3), 3, 0xe0b05080);
        gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
        gfx_set_effect(DRAW_BLEND_ZERO, DRAW_BLEND_SRCALPHA, combine);
        quad(v, (float)(preset * 4 + 1), 5, (float)(preset * 4 + 3), 7, 0xe0b05080);
        gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
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

/* Two textures of two coordinate sets (GFX_VTX_TEX2), multiplied (the multitexture particles). */
static void check_two_textures(struct C3dDevice *d)
{
    static const DWORD orange[4] = { 0xffff8000, 0xffff8000, 0xffff8000, 0xffff8000 };
    static const DWORD split[4] = { 0xff00ff00, 0xffffffff, 0xff00ff00, 0xffffffff };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, orange), *u = texture_2x2(GFX_FMT_8888, split);
    struct { float x, y, z, rhw; DWORD diffuse; float u0, v0, u1, v1; } v[4];
    int i;
    for (i = 0; i < 4; i++) {
        v[i].x = (i & 1 ? 16.0f : 0.0f) - 0.5f;
        v[i].y = (i & 2 ? 16.0f : 0.0f) - 0.5f;
        v[i].z = 0.5f;
        v[i].rhw = 1.0f;
        v[i].diffuse = 0xffffffff;
        v[i].u0 = v[i].v0 = 0.5f;
        v[i].u1 = (float)(i & 1);
        v[i].v1 = (float)(i >> 1);
    }
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_mode(DRAW_MULTITEXTURE);
    gfx_set_texture(0, t);
    gfx_set_texture(1, u);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX2, v, 4, NULL, 0);
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
static void check_ground(struct C3dDevice *d)
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
static void check_two_pass_alpha(struct C3dDevice *d)
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
static void check_effect_depth(struct C3dDevice *d)
{
    struct Vertex v[4];
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_texture(0, NULL);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    quad(v, 0, 0, 16, 32, 0xffff0000);
    v[0].z = v[1].z = v[2].z = v[3].z = 0.3f;
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_set_mode(DRAW_EFFECT_DEPTH);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ZERO, DRAW_COMBINE_MODULATE);
    quad(v, 0, 0, 32, 16, 0xff00ff00);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    /* drawn after it, behind the effect quad: had that written depth, this would be hidden */
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    quad(v, 16, 0, 32, 8, 0xff0000ff);
    v[0].z = v[1].z = v[2].z = v[3].z = 0.75f;
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_set_mode(DRAW_EFFECT_OVERLAY);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ZERO, DRAW_COMBINE_MODULATE);
    quad(v, 0, 16, 32, 32, 0xffffff00);
    v[0].z = v[1].z = v[2].z = v[3].z = 0.9f;
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_end_scene();
    assert(screen_pixel(d, 4, 4) == 0xff0000 && screen_pixel(d, 20, 12) == 0x00ff00);
    assert(screen_pixel(d, 20, 4) == 0x0000ff);
    assert(screen_pixel(d, 4, 20) == 0xffff00 && screen_pixel(d, 20, 20) == 0xffff00);
}

/* The interface composited into a window's texture (DRAW_COMPOSITE): blended by the source's alpha
   into the target's colour and alpha, texels of alpha 0 dropped. */
static void check_composite(struct C3dDevice *d)
{
    static const DWORD texels[4] = { 0xffffffff, 0x00ffffff, 0xffffffff, 0x00ffffff };
    gfx_surface *rt = gfx_create_render_target(16, 16), *t = texture_2x2(GFX_FMT_8888, texels);
    struct gfx_lock lk;
    DWORD p, q;
    assert(rt && gfx_set_render_target(rt) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0x00000000, 1.0f, NULL);
    gfx_set_mode(DRAW_COMPOSITE);
    gfx_set_texture(0, t);
    draw_quad(0, 0, 16, 16, 0x80ff0000);
    gfx_end_scene();
    assert(gfx_set_render_target(d->target) == 0);
    assert(gfx_lock(rt, NULL, 0, &lk) == 0);
    memcpy(&p, (const BYTE *)lk.bits + 8 * lk.pitch + 2 * 4, 4);
    memcpy(&q, (const BYTE *)lk.bits + 8 * lk.pitch + 13 * 4, 4);
    gfx_unlock(rt);
    /* half red over transparent black: red 0x80, alpha a half of a half */
    assert(near_value(p >> 16 & 0xff, 0x80) && (p & 0xffff) == 0 && near_value(p >> 24, 0x40));
    assert(q == 0);
    gfx_set_texture(0, NULL);
    gfx_release(t);
    gfx_release(rt);
}

/* A clear of colour and depth inside a pass (DRAW_CLEAR) writes both: a later quad behind the
   cleared depth is hidden there and drawn elsewhere. */
static void check_clear_inside(struct C3dDevice *d)
{
    RECT r = { 8, 8, 16, 16 };
    struct Vertex v[4];
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    draw_quad(0, 0, 32, 32, 0xffffffff);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff0000ff, 0.25f, &r);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    gfx_set_texture(0, NULL);
    quad(v, 0, 0, 32, 32, 0xff00ff00);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_end_scene();
    assert(screen_pixel(d, 12, 12) == 0x0000ff && screen_pixel(d, 4, 4) == 0x00ff00 && screen_pixel(d, 20, 12) == 0x00ff00);
}

/* The overlay list and the effect overlay list after it, which starts with the blending and
   combination the overlay's mode has, draw with the same state: their quads are one draw, as the
   Direct3D 9 client's were while no state changed. An effect's factors are taken whole. */
static void check_overlay_batch(struct C3dDevice *d)
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

/* A mode set again, or an effect's blending in a mode that does not take it, changes nothing it
   draws with and counts as skipped (diag/perf.h). */
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
static void check_world_cull(struct C3dDevice *d)
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
static void check_world_growth(struct C3dDevice *d)
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

/* A texture's mipmaps in a layer of an array are what its own texture's are: a record and a quad
   of the same 8 by 8 texture, drawn at half and a quarter of its size, read the same texels. */
static void check_world_mips(struct C3dDevice *d)
{
    static const WORD indices[6] = { 0, 1, 2, 2, 1, 3 };
    struct GfxMeshVertex mv[8];
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct GfxWorldDraw draw;
    struct gfx_lock lk;
    struct SwRun run;
    gfx_surface *t;
    int x, y, s, size;
    gfx_set_mipmaps(1);
    t = gfx_create_texture(8, 8, GFX_FMT_8888);
    gfx_set_mipmaps(0);
    assert(t && gfx_lock(t, NULL, 0, &lk) == 0);
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
    for (s = 0; s < 2; s++) {
        size = s ? 2 : 4;
        for (y = 0; y < size; y++)
            for (x = 0; x < size; x++) {
                DWORD a = screen_pixel(d, 8 + s * 16 + x, 8 + y), b = screen_pixel(d, 8 + s * 16 + x, 24 + y);
                int k;
                assert(a != 0);
                for (k = 0; k < 24; k += 8)
                    assert(near_value(a >> k & 0xff, (int)(b >> k & 0xff)));
            }
    }
    gfx_set_texture(0, NULL);
    gfx_release(t);
}

/* The casters (gfx_world_cast, world_caster.hlsl): a model's record casts where its texture's
   alpha passes the opaque lists' test (0xcf), and the ground drawn while receiving lies in its
   shadow there, darkened to the shadow's darkness, and is lit around it and where the texture's
   alpha drops the caster. */
static void check_world_cast(struct C3dDevice *d)
{
    static const DWORD half[4] = { 0xffffffff, 0x00ffffff, 0xffffffff, 0x00ffffff };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, half), *lm = texture_square(GFX_FMT_8888, 2, 0xff000000);
    struct GfxMeshVertex mv[8];
    struct GfxWorldView view;
    struct GfxWorldFrame frame;
    struct GfxWorldDraw ground;
    struct GfxShadow shadow;
    struct SwRun run;
    DWORD p;
    int i;
    if (!gfx_shadows_supported())
        return;
    world_begin();
    world_texture(t, 0, 0);
    world_texture(lm, 1, 0);
    /* the caster: world x -16 to 0, y -8 to 8, at depth 50, its left half opaque */
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
    /* the ground over the whole screen at depth 100, its lightmap lit (alpha whole, colour black) */
    world_quad(mv + 4, 0, 0, W, H, 0xffffffff);
    for (i = 4; i < 8; i++)
        mv[i].u1 = mv[i].v1 = 0.5f;
    run = model_run(0, 6, SW_CULL_NONE);
    run.pass = SW_PASS_GROUND;
    run.lightmapArray = 1;
    world_add(mv + 4, 4, quad_cw, 6, &run, 1, 0, NULL, 0);
    world_send(NULL, 0);
    /* the light along +z from the eye: x and y across 128 units, depth 0 to 1,000 */
    memset(&shadow, 0, sizeof shadow);
    shadow.cascades = 1;
    shadow.fromView[0][0] = shadow.fromView[0][5] = 1.0f / 64.0f;
    shadow.fromView[0][10] = 1.0f / 1000.0f;
    shadow.fromView[0][15] = 1.0f;
    shadow.farDepth[0] = 1e6f;
    shadow.lightView[2] = -1.0f;
    shadow.bias = 1e-4f;
    shadow.darkness = 0.5f;
    shadow.ambient[0] = shadow.ambient[1] = shadow.ambient[2] = 0.5f;
    test_view(&view);
    memset(&ground, 0, sizeof ground);
    ground.kind = GFX_MESH_GROUND;
    ground.colour[0] = ground.colour[1] = ground.colour[2] = ground.colour[3] = 1.0f;
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    memset(&frame, 0, sizeof frame);
    world_frame(&view, &frame);
    assert(gfx_shadow_begin(&shadow) && gfx_shadow_cascade(0));
    gfx_world_cast(g_world.records[0].firstIndex, g_world.records[0].indexCount, g_world.records[0].baseVertex, 0,
                   (uint32_t)g_world.count);
    gfx_shadow_end();
    gfx_shadow_receive(1);
    gfx_set_mode(DRAW_GROUND_LIGHTMAP);
    gfx_world_draw_record(&g_world.records[1], &ground);
    gfx_shadow_receive(0);
    gfx_end_scene();
    /* the opaque half shades the ground at x -16 to -8 (pixels 16 to 23 across, 16 to 31 down) */
    p = screen_pixel(d, 19, 24);
    assert(near_value(p >> 16 & 0xff, 0x80) && near_value(p >> 8 & 0xff, 0x80) && near_value(p & 0xff, 0x80));
    assert(screen_pixel(d, 29, 24) == 0xffffff && screen_pixel(d, 10, 24) == 0xffffff
           && screen_pixel(d, 19, 10) == 0xffffff);
    gfx_release(t);
    gfx_release(lm);
}

/* A world cleared after its draws were recorded and before the frame went up (gfx_world_clear, as
   textures reloaded or a new map clear it): those draws are left out and read nothing released;
   the world built again draws. */
static void check_world_cleared(struct C3dDevice *d)
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
    struct C3dDevice *d = &g_3dDevice;
    struct Vertex v[4];
    gfx_surface *rt, *sys;
    struct gfx_lock lk;
    int x, y;
    if (argc > 1)
        SDL_setenv_unsafe("RAGEXE_MSAA", argv[1], 1);
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->hwnd = SDL_CreateWindow("gfx test", W, H, SDL_WINDOW_HIDDEN);
    if (!d->hwnd) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->width = W;
    d->height = H;
    assert(gfx_open(d, 0) == 0);
    if (gfx_create_screen(d, 2) != 0) {
        fprintf(stderr, "skipped: no GPU device\n");
        return 77;
    }
    assert(gfx_create_3d(d, 0) == 0);
    assert(gfx_get_caps(d, &g_gfxCaps) == 0);
    assert(gfx_create_zbuffer(d) == 0);

    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff102030, 1.0f, NULL);
    assert(screen_pixel(d, 0, 0) == 0x102030 && screen_pixel(d, W - 1, H - 1) == 0x102030);

    gfx_set_mode(FILTERED);
    gfx_set_texture(0, NULL);
    quad(v, 8, 8, 24, 20, 0xffff0000);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_end_scene();
    assert(screen_pixel(d, 8, 8) == 0xff0000 && screen_pixel(d, 23, 19) == 0xff0000);
    assert(screen_pixel(d, 7, 8) == 0x102030 && screen_pixel(d, 24, 19) == 0x102030 && screen_pixel(d, 8, 20) == 0x102030);

    /* edges through pixel centres: the top-left rule takes the top and left ones, multisampled
       too, as Direct3D 9 drew flat quads without multisampling */
    gfx_clear(GFX_CLEAR_TARGET, 0xff102030, 1.0f, NULL);
    quad(v, 8.5f, 8.5f, 24.5f, 20.5f, 0xff00ff00);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
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
        gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
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

    {
        static const DWORD keyed[4] = { 0x00ff0000, 0xff00ff00, 0xff00ff00, 0x00ff0000 };
        gfx_surface *t = texture_2x2(GFX_FMT_1555, keyed);
        /* the opaque lists' alpha test, at least 0xcf */
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
        gfx_set_mode(DRAW_OPAQUE_CLAMP);
        gfx_set_texture(0, t);
        quad(v, 0, 0, 32, 32, 0xffffffff);
        gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
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
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
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
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_set_mode(COLOUR);
    quad(v, 0, 0, 16, 8, 0xff0000ff);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_end_scene();
    assert(gfx_set_render_target(d->target) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gfx_set_mode(TEXTURE);
    gfx_set_texture(0, rt);
    quad(v, 0, 0, 16, 16, 0xffffffff);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_end_scene();
    assert(screen_pixel(d, 4, 2) == 0x0000ff && screen_pixel(d, 4, 12) == 0);
    gfx_set_texture(0, NULL);
    gfx_release(rt);
    check_modes_counted();

    sys = gfx_create_surface(8, 8);
    assert(sys && gfx_lock(sys, NULL, 0, &lk) == 0);
    for (y = 0; y < 8; y++)
        for (x = 0; x < 8; x++) {
            DWORD c = y < 4 ? 0x00ffff00 : 0x00ff00ff;
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &c, 4);
        }
    gfx_unlock(sys);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    {
        RECT dst = {10, 10, 18, 18};
        assert(gfx_blit(d->target, &dst, sys, NULL, GFX_BLT_WAIT) == 0);
    }
    assert(screen_pixel(d, 12, 11) == 0xffff00 && screen_pixel(d, 12, 16) == 0xff00ff && screen_pixel(d, 9, 9) == 0);
    gfx_release(sys);

    check_mipmaps(d);
    check_target_copies(d);
    check_target_write_back(d);
    check_target_rect_lock(d);
    check_target_edges(d);
    check_clears(d);
    check_world(d);
    check_slots(d);
    check_water(d);
    check_world_cull(d);
    check_world_growth(d);
    check_world_mips(d);
    check_world_cast(d);
    check_world_cleared(d);

    check_resize(d);
    assert(gfx_present(d, 0) == 0);
    gfx_close(d);
    SDL_DestroyWindow(d->hwnd);
    SDL_Quit();
    printf("gfx gpu (RAGEXE_MSAA=%s): clears, edges and flat quads, 8888/565/1555/4444 textures, texel centres, "
           "culling, textures changed and released in a frame, alpha test, fog (the quads' at every step), blending, "
           "the effect modes' blending, the 16 presets against Direct3D 9's stages, two textures, the ground's records, "
           "two-pass alpha, effects behind depth, composites, clears inside a pass, the overlays batched, mipmaps, "
           "render targets and their edges, modes kept from targets "
           "they do not draw into, modes counted, system surface blits, target copies, stretched and out of "
           "bounds, write-back, rectangle locks, world vertices and records, transform slots, texture-coordinate "
           "matrices, the water, the world's cull compacted by run, its buffers and arrays grown, its mipmaps, "
           "casters and shadows, a world cleared in a frame, resizing passed\n",
           argc > 1 ? argv[1] : "4");
    return 0;
}
