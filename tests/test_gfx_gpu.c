/* render/gfx_gpu.c through render/gfx.h on a real GPU, in a hidden window: what each draw puts on
   the screen, downloaded through gfx_read_frame, and the window claimed and presented into where
   the video driver lets it be shown without showing anything (check_present). Skipped (77) where
   SDL finds no GPU device. The argument is RAGEXE_MSAA: the test runs once without multisampling
   and once with it. */
#include "render/gfx.h"
#include "render/device.h"
#include "render/screen_wobble.h"
#include "render/vertex_fog.h"
#include "diag/perf.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct perf_counters g_perf;
struct C3dDevice g_3dDevice;
static int multisampled;  /* RAGEXE_MSAA asks for more than one sample */

#define W 64
#define H 48

static DWORD screen_pixel(struct C3dDevice *d, int x, int y)
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
static void read_screen(struct C3dDevice *d, DWORD *out)
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

static void draw_quad(float x0, float y0, float x1, float y1, DWORD diffuse)
{
    struct TexDrawVertex v[4];
    quad(v, x0, y0, x1, y1, diffuse);
    gfx_draw_vertices(v, 4, 1, 0);
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

/* A model's mesh draw in a mode that draws meshes, white where its texture is white. */
static void model_draw(struct GfxMeshDraw *draw, int first, int count, int cull)
{
    memset(draw, 0, sizeof *draw);
    draw->kind = GFX_MESH_MODEL;
    draw->first = first;
    draw->count = count;
    draw->cull = cull;
    draw->colour[0] = draw->colour[1] = draw->colour[2] = draw->colour[3] = 1.0f;
    draw->uv[0] = draw->uv[3] = 1.0f;
}

/* Direct3D 9 culls by the winding on the screen: GFX_CULL_CW drops the triangles whose corners go
   clockwise there (y down), GFX_CULL_CCW the others. A mesh's draw says how it culls; quads are
   never culled. */
static void check_culling(struct C3dDevice *d)
{
    /* at depth 100 a unit is a pixel, the view's centre at (32, 24) */
    static const float corners[6][2] = { { 4, 4 }, { 20, 4 }, { 4, 20 }, { 30, 4 }, { 30, 20 }, { 46, 4 } };
    static const WORD indices[6] = { 0, 1, 2, 3, 4, 5 };
    struct GfxMeshVertex mv[6];
    struct GfxWorldView view;
    struct GfxMeshDraw draw;
    gfx_mesh *mesh;
    int cull, i;
    memset(mv, 0, sizeof mv);
    for (i = 0; i < 6; i++) {
        mv[i].x = corners[i][0] - W / 2.0f;
        mv[i].y = corners[i][1] - H / 2.0f;
        mv[i].z = 100.0f;
        mv[i].diffuse = 0xffffffff;
    }
    mesh = gfx_mesh_create(mv, 6, indices, 6);
    assert(mesh);
    test_view(&view);
    gfx_set_world_view(&view);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    gfx_set_texture(0, NULL);
    for (cull = GFX_CULL_NONE; cull <= GFX_CULL_CCW; cull++) {
        gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0, 1.0f, NULL);
        model_draw(&draw, 0, 6, cull);
        gfx_draw_mesh(mesh, &draw);
        gfx_end_scene();
        assert((screen_pixel(d, 7, 7) != 0) == (cull != GFX_CULL_CW));
        assert((screen_pixel(d, 33, 7) != 0) == (cull != GFX_CULL_CCW));
    }
    gfx_mesh_destroy(mesh);
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

/* Copies between targets stay on the GPU: a render target's rectangle into the screen, a copy into
   a texture refused, rectangles outside their surfaces copying nothing, and the frame saved into a
   render target and put back, under the draws after it (c3d_device_save_frame and
   c3d_device_restore_frame). */
static void check_target_copies(struct C3dDevice *d)
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
    assert(gfx_set_render_target(d->target) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff0000ff, 1.0f, NULL);
    assert(gfx_blit(d->target, &into, rt, &r) == 0);
    assert(gfx_blit(tex, &r, rt, &r) < 0);
    gfx_end_scene();
    assert(screen_pixel(d, 22, 12) == 0 && screen_pixel(d, 22, 22) == 0x00ff00 && screen_pixel(d, 19, 22) == 0x0000ff
           && screen_pixel(d, 36, 22) == 0x0000ff);
    /* rectangles outside their surfaces copy nothing */
    {
        RECT outside = { 8, 8, 17, 16 }, negative = { -1, 0, 4, 4 }, empty = { 4, 4, 4, 8 }, beyond = { 60, 40, 70, 48 };
        assert(gfx_blit(rt, &outside, d->target, NULL) == 0);
        assert(gfx_blit(d->target, NULL, rt, &negative) == 0);
        assert(gfx_blit(d->target, &empty, rt, &empty) == 0);
        assert(gfx_blit(d->target, &beyond, rt, NULL) == 0);
        gfx_end_scene();
        assert(screen_pixel(d, 22, 22) == 0x00ff00 && screen_pixel(d, 62, 42) == 0x0000ff);
    }
    gfx_release(rt);
    gfx_release(tex);

    gfx_clear(GFX_CLEAR_TARGET, 0xff203040, 1.0f, NULL);
    assert(gfx_blit(save, NULL, d->target, NULL) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    assert(screen_pixel(d, 30, 30) == 0);
    assert(gfx_blit(d->target, NULL, save, NULL) == 0);
    draw_quad(0, 0, 4, 4, 0xffffffff);
    gfx_end_scene();
    assert(screen_pixel(d, 30, 30) == 0x203040);
    assert(screen_pixel(d, 2, 2) == 0xffffff);
    gfx_release(save);
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

/* The screen's picture is read, a rectangle of it or the whole, with what was drawn before; a
   rectangle outside it, an empty one and a pitch narrower than the rows read nothing, and a target
   locks nothing: its pixels are the GPU's. */
static void check_read_frame(struct C3dDevice *d)
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
    assert(gfx_lock(d->target, NULL, 0, &lk) < 0);
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
        struct TexDrawVertex v[4];
        quad(v, 0, 0, 32, 32, 0xffff0000);
        gfx_draw_vertices(v, 4, 1, 0);
        gfx_clear(GFX_CLEAR_ZBUFFER, 0, 1.0f, &r);
        quad(v, 0, 0, 32, 32, 0xff00ff00);
        v[0].z = v[1].z = v[2].z = v[3].z = 0.75f;
        gfx_draw_vertices(v, 4, 1, 0);
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

/* A texture of distinct texels, w by h, made layered or not: texel (x, y) gets colour hash of
   salt, x and y, opaque or, where `alpha`, half of them see-through. */
static gfx_surface *pattern_texture(int layered, int w, int h, unsigned salt, int alpha)
{
    gfx_surface *t = layered ? gfx_create_layer_texture(w, h, GFX_FMT_8888) : gfx_create_texture(w, h, GFX_FMT_8888);
    struct gfx_lock lk;
    int x, y;
    assert(t && gfx_lock(t, NULL, 0, &lk) == 0);
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
    gfx_draw_vertices(v, 4, 1, 0);
}

/* Layered textures (gfx_create_layer_texture) read as the same texels in plain ones do, pixel for
   pixel, by the nearest texel and filtered, enlarged and shrunk, with mipmaps too; quads of several
   layers of one array are one draw; a full array leaves the next texture to a second one; a layer
   given up is taken by the next texture, which reads its own texels; large textures share an array
   with fewer others, the largest with none. */
static void check_layers(struct C3dDevice *d)
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

/* A list of triangles draws each of them, flat or not, and nothing of the quad they would close. */
static void check_triangles(struct C3dDevice *d)
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
        gfx_draw_vertices(v, 6, 0, 0);
        gfx_end_scene();
        /* the first above the diagonal of its square, the second below the other one */
        assert(screen_pixel(d, 2, 2) == 0xff0000 && screen_pixel(d, 13, 13) == 0);
        assert(screen_pixel(d, 33, 13) == 0x00ff00 && screen_pixel(d, 22, 2) == 0);
    }
}

/* World vertices and a mesh land on the pixels the same corners pre-transformed cover. */
static void check_world(struct C3dDevice *d)
{
    struct GfxWorldView view;
    struct TexDrawVertex v[4];
    struct GfxMeshVertex mv[4];
    static const WORD indices[6] = { 0, 1, 2, 2, 1, 3 };
    struct GfxMeshDraw draw;
    gfx_mesh *mesh;
    int i;
    test_view(&view);
    gfx_set_world_camera(&view);
    gfx_set_world_view(&view);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_mode(COLOUR);
    /* world x = -8..0, y = -8..0 at depth 100 cover the pixels 24..31, 16..23 */
    quad(v, 0, 0, 1, 1, 0xff00ff00);
    for (i = 0; i < 4; i++) {
        v[i].x = (i & 1) ? 0.0f - 0.5f : -8.0f - 0.5f;
        v[i].y = (i & 2) ? 0.0f - 0.5f : -8.0f - 0.5f;
        v[i].z = 100.0f;
    }
    gfx_draw_vertices(v, 4, 1, 1);
    gfx_end_scene();
    assert(screen_pixel(d, 24, 16) == 0x00ff00 && screen_pixel(d, 31, 23) == 0x00ff00);
    assert(screen_pixel(d, 23, 16) == 0 && screen_pixel(d, 32, 23) == 0 && screen_pixel(d, 24, 15) == 0
           && screen_pixel(d, 24, 24) == 0);

    memset(mv, 0, sizeof mv);
    for (i = 0; i < 4; i++) {
        mv[i].x = (i & 1) ? 8.0f - 0.5f : 0.0f - 0.5f;
        mv[i].y = (i & 2) ? 8.0f - 0.5f : 0.0f - 0.5f;
        mv[i].z = 100.0f;
        mv[i].diffuse = 0xff4080c0;
    }
    mesh = gfx_mesh_create(mv, 4, indices, 6);
    assert(mesh);
    memset(&draw, 0, sizeof draw);
    draw.kind = GFX_MESH_MODEL;
    draw.count = 6;
    draw.colour[0] = draw.colour[1] = draw.colour[2] = draw.colour[3] = 1.0f;
    draw.uv[0] = draw.uv[3] = 1.0f;
    draw.cull = GFX_CULL_NONE;
    gfx_set_texture(0, NULL);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    gfx_draw_mesh(mesh, &draw);
    gfx_end_scene();
    assert(screen_pixel(d, 32, 24) == 0x4080c0 && screen_pixel(d, 39, 31) == 0x4080c0);
    assert(screen_pixel(d, 31, 24) == 0 && screen_pixel(d, 40, 31) == 0 && screen_pixel(d, 32, 32) == 0);
    /* a draw from an odd index: the second triangle alone, below the diagonal from top right */
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    draw.first = 3;
    draw.count = 3;
    gfx_draw_mesh(mesh, &draw);
    gfx_end_scene();
    gfx_mesh_destroy(mesh);
    assert(screen_pixel(d, 39, 31) == 0x4080c0 && screen_pixel(d, 32, 24) == 0);
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

static void check_resize(struct C3dDevice *d)
{
    static DWORD frame[80 * 40];
    struct WobbleRow rows[40];
    RECT beyond = { 0, 0, 81, 40 };
    int w = 0, h = 0;
    assert(gfx_resize(80, 40) == 0);
    d->width = 80;
    d->height = 40;
    assert(gfx_get_size(d->target, &w, &h) == 0 && w == 80 && h == 40);
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
static void fog_quad(float x, float y, float size, float rhw, int world)
{
    struct TexDrawVertex v[4];
    int i;
    quad(v, x, y, x + size, y + size, 0xffff0000);
    for (i = 0; i < 4; i++) {
        v[i].rhw = rhw;
        if (world) {
            /* at depth 100 a unit is a pixel, the view's centre at (32, 24) */
            v[i].x = (i & 1 ? x + size : x) - 32.0f - 0.5f;
            v[i].y = (i & 2 ? y + size : y) - 24.0f - 0.5f;
            v[i].z = 100.0f;
        }
    }
    gfx_draw_vertices(v, 4, 1, world);
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
    static const int kinds[2] = { 0, 1 };
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
                            kinds[k] ? "world" : "screen", rhw, got, want);
            }
        }
    assert(!wrong);
}

static void check_fog(struct C3dDevice *d)
{
    static const WORD indices[6] = { 0, 1, 2, 2, 1, 3 };
    struct GfxFog fog;
    struct GfxWorldView view;
    struct GfxMeshVertex mv[4];
    struct GfxMeshDraw draw;
    gfx_mesh *mesh;
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
    fog_quad(0, 0, 8, 1.0f / 50, 0);   /* half way: 127 of 255 kept */
    fog_quad(8, 0, 8, 1.0f / 150, 0);  /* beyond the end: all fog */
    fog_quad(16, 0, 8, 1.0f, 0);       /* 0.01 of the way: 252 kept */
    /* a mode without fog */
    gfx_set_mode(DRAW_OVERLAY);
    fog_quad(24, 0, 8, 1.0f / 150, 0);
    /* the end where the start is: the depth's share alone */
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    fog.start = fog.end = 0.25f;
    gfx_set_fog(&fog);
    fog_quad(32, 0, 8, 1.0f / 50, 0);
    /* a mesh at depth 50 between 40 and 60: half fog (the pixels 8 to 15 by 12 to 19) */
    memset(mv, 0, sizeof mv);
    for (i = 0; i < 4; i++) {
        mv[i].x = (i & 1) ? -16.0f : -24.0f;
        mv[i].y = (i & 2) ? -4.0f : -12.0f;
        mv[i].z = 50.0f;
        mv[i].diffuse = 0xffff0000;
    }
    mesh = gfx_mesh_create(mv, 4, indices, 6);
    assert(mesh);
    memset(&view, 0, sizeof view);
    view.view[0] = view.view[4] = view.view[8] = 1.0f;
    view.proj[0] = view.proj[1] = 50.0f;
    view.centre[0] = W / 2.0f;
    view.centre[1] = H / 2.0f;
    view.nearZ = 10.0f;
    view.depthScale = 1.0f;
    gfx_set_world_view(&view);
    fog.start = 0.4f;
    fog.end = 0.6f;
    gfx_set_fog(&fog);
    model_draw(&draw, 0, 6, GFX_CULL_NONE);
    gfx_draw_mesh(mesh, &draw);
    /* the fog off */
    gfx_set_fog(NULL);
    fog_quad(40, 0, 8, 1.0f / 150, 0);
    gfx_end_scene();
    gfx_mesh_destroy(mesh);
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
    struct TexDrawVertex v[4];
    int i;
    gfx_clear(GFX_CLEAR_TARGET, 0xff102030, 1.0f, NULL);
    gfx_set_mode(DRAW_EFFECT_OVERLAY);
    gfx_set_texture(0, t);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ONE, 3);
    quad(v, 0, 0, 16, 16, 0xff200000);
    gfx_draw_vertices(v, 4, 1, 0);
    gfx_set_effect(DRAW_BLEND_SRCALPHA, DRAW_BLEND_DESTALPHA, 0);
    quad(v, 16, 0, 32, 16, 0x80ffffff);
    gfx_draw_vertices(v, 4, 1, 0);
    gfx_set_texture(1, u);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ZERO, 5);
    quad(v, 0, 16, 16, 32, 0xffffffff);
    for (i = 0; i < 4; i++) {
        v[i].u *= 2.0f;
        v[i].v *= 2.0f;
    }
    gfx_draw_vertices(v, 4, 1, 0);
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
        gfx_draw_vertices(v, 4, 1, 0);
        gfx_set_effect(DRAW_BLEND_ZERO, DRAW_BLEND_SRCALPHA, combine);
        quad(v, (float)(preset * 4 + 1), 5, (float)(preset * 4 + 3), 7, 0xe0b05080);
        gfx_draw_vertices(v, 4, 1, 0);
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
static void check_two_textures(struct C3dDevice *d)
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
    q.world.depth = GFX_DEPTH_ZERO;
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

/* The ground's meshes (GFX_MESH_GROUND): with the lightmap mode, texture times colour times the
   lightmap's alpha plus its colour, the lightmap opaque black outside its coordinates; without it,
   texture times colour. */
static void check_ground(struct C3dDevice *d)
{
    static const DWORD grey[4] = { 0xff808080, 0xff808080, 0xff808080, 0xff808080 };
    static const DWORD light[4] = { 0x80402010, 0x80402010, 0x80402010, 0x80402010 };
    static const WORD corners[6] = { 0, 1, 2, 2, 1, 3 };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, grey), *lm = texture_2x2(GFX_FMT_8888, light);
    struct GfxWorldView view;
    struct GfxMeshVertex mv[12];
    struct GfxMeshDraw draw;
    WORD indices[18];
    gfx_mesh *mesh;
    DWORD p;
    int q, i;
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
    mesh = gfx_mesh_create(mv, 12, indices, 18);
    assert(mesh);
    test_view(&view);
    gfx_set_world_view(&view);
    memset(&draw, 0, sizeof draw);
    draw.kind = GFX_MESH_GROUND;
    draw.cull = GFX_CULL_NONE;
    draw.colour[0] = draw.colour[1] = draw.colour[2] = draw.colour[3] = 1.0f;
    draw.uv[0] = draw.uv[3] = 1.0f;
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_texture(0, t);
    gfx_set_texture(1, lm);
    gfx_set_mode(DRAW_GROUND_LIGHTMAP);
    draw.count = 12;
    gfx_draw_mesh(mesh, &draw);
    gfx_set_mode(DRAW_GROUND);
    draw.first = 12;
    draw.count = 6;
    gfx_draw_mesh(mesh, &draw);
    gfx_end_scene();
    gfx_mesh_destroy(mesh);
    /* 0x40 + 0x80 / 255 * 0x80, 0x20 + the same, 0x10 + the same */
    p = screen_pixel(d, 4, 4);
    assert(near_value(p >> 16 & 0xff, 0x80) && near_value(p >> 8 & 0xff, 0x60) && near_value(p & 0xff, 0x50));
    assert(screen_pixel(d, 12, 4) == 0x808080 && screen_pixel(d, 20, 4) == 0x808080);
    gfx_set_texture(0, NULL);
    gfx_set_texture(1, NULL);
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
    struct TexDrawVertex v[4];
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_texture(0, NULL);
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    quad(v, 0, 0, 16, 32, 0xffff0000);
    v[0].z = v[1].z = v[2].z = v[3].z = 0.3f;
    gfx_draw_vertices(v, 4, 1, 0);
    gfx_set_mode(DRAW_EFFECT_DEPTH);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ZERO, DRAW_COMBINE_MODULATE);
    quad(v, 0, 0, 32, 16, 0xff00ff00);
    gfx_draw_vertices(v, 4, 1, 0);
    /* drawn after it, behind the effect quad: had that written depth, this would be hidden */
    gfx_set_mode(DRAW_OPAQUE_CLAMP);
    quad(v, 16, 0, 32, 8, 0xff0000ff);
    v[0].z = v[1].z = v[2].z = v[3].z = 0.75f;
    gfx_draw_vertices(v, 4, 1, 0);
    gfx_set_mode(DRAW_EFFECT_OVERLAY);
    gfx_set_effect(DRAW_BLEND_ONE, DRAW_BLEND_ZERO, DRAW_COMBINE_MODULATE);
    quad(v, 0, 16, 32, 32, 0xffffff00);
    v[0].z = v[1].z = v[2].z = v[3].z = 0.9f;
    gfx_draw_vertices(v, 4, 1, 0);
    gfx_end_scene();
    assert(screen_pixel(d, 4, 4) == 0xff0000 && screen_pixel(d, 20, 12) == 0x00ff00);
    assert(screen_pixel(d, 20, 4) == 0x0000ff);
    assert(screen_pixel(d, 4, 20) == 0xffff00 && screen_pixel(d, 20, 20) == 0xffff00);
}

/* The interface composited into a window's texture (DRAW_COMPOSITE): blended by the source's alpha
   into the target's colour and alpha, texels of alpha 0 dropped. The window then draws the target as
   a texture: copied as it is, its colour shows; drawn in the interface's mode over blue, its alpha
   blends it. */
static void check_composite(struct C3dDevice *d)
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
    assert(gfx_set_render_target(d->target) == 0);
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
static void check_clear_inside(struct C3dDevice *d)
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
    gfx_draw_vertices(v, 4, 1, 0);
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
static void check_wobble_frame(struct C3dDevice *d, gfx_surface *sys, gfx_surface *other)
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
    assert(gfx_blit(d->target, NULL, sys, NULL) == 0);
    if (other) {
        assert(gfx_set_render_target(other) == 0);
        gfx_clear(GFX_CLEAR_TARGET, 0, 0.0f, NULL);
        assert(gfx_set_render_target(d->target) == 0);
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
   (render/gfx_gpu.c, MARGIN). */
static void check_wobble(struct C3dDevice *d)
{
    gfx_surface *sys = gfx_create_surface(W, H), *other = gfx_create_render_target(4, 4);
    struct gfx_lock lk;
    int x, y, round;
    assert(sys && other && gfx_lock(sys, NULL, 0, &lk) == 0);
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++) {
            DWORD c = wobble_pattern(x, y);
            memcpy((BYTE *)lk.bits + y * lk.pitch + x * 4, &c, 4);
        }
    gfx_unlock(sys);
    for (round = 0; round < 2; round++)
        check_wobble_frame(d, sys, round ? other : NULL);
    gfx_release(other);
    gfx_release(sys);
}

/* A composited window's target drawn as the window's quad draws it (ui_window_make_quad_base,
   texture_draw_surface): at the window's place and size, its texture coordinates from the first
   texel's centre to a texel past the last one's (render/render_api.h, TEXEL_CENTRE). Every pixel
   shows its own texel exactly, for a target of odd size, and nothing around the window changes: by
   the nearest texel, and filtered linearly in the interface's mode, as the client draws it. Mesa's
   lavapipe filters anisotropically even where a pixel covers one texel when the coordinates are a
   target's, which the device scales to the target's part of its texture (render/gfx_gpu.c, MARGIN),
   and mixes the neighbours in (docs/custom-review.md, "No read-backs"): there the linear read is
   not compared. Answers whether it was. */
static int check_window_target(struct C3dDevice *d)
{
    enum { TW = 15, TH = 9, X = 21, Y = 13 };
    static DWORD all[W * H];
    gfx_surface *rt = gfx_create_render_target(TW, TH), *sys = gfx_create_surface(TW, TH);
    const char *gpu = SDL_GetStringProperty(SDL_GetGPUDeviceProperties(d->dev), SDL_PROP_GPU_DEVICE_NAME_STRING, "");
    int linear = !strstr(gpu, "llvmpipe");
    struct gfx_lock lk;
    struct TexDrawVertex v[4];
    int x, y, i, pass;
    assert(rt && sys && gfx_lock(sys, NULL, 0, &lk) == 0);
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
        assert(gfx_set_render_target(d->target) == 0);
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
        gfx_draw_vertices(v, 4, 1, 0);
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
static int check_present(struct C3dDevice *d)
{
    SDL_GPUDevice *dev = d->dev;
    const char *driver = SDL_GetCurrentVideoDriver();
    int i;
    gfx_clear(GFX_CLEAR_TARGET, 0xff334455, 1.0f, NULL);
    assert(gfx_present(0) == 0 && gfx_present(1) == 0);
    assert(SDL_GetGPUSwapchainTextureFormat(dev, d->hwnd) == SDL_GPU_TEXTUREFORMAT_INVALID);
    assert(screen_pixel(d, 5, 5) == 0x334455);
    if (!driver || strcmp(driver, "offscreen"))
        return 0;
    assert(SDL_ShowWindow(d->hwnd));
    assert(gfx_present(1) == 0);
    if (SDL_GetGPUSwapchainTextureFormat(dev, d->hwnd) == SDL_GPU_TEXTUREFORMAT_INVALID) {
        assert(!SDL_ClaimWindowForGPUDevice(dev, d->hwnd));
        SDL_HideWindow(d->hwnd);
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
    assert(SDL_HideWindow(d->hwnd));
    assert(gfx_present(1) == 0 && gfx_present(0) == 0);
    assert(SDL_GetGPUSwapchainTextureFormat(dev, d->hwnd) != SDL_GPU_TEXTUREFORMAT_INVALID);
    return 1;
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

int main(int argc, char **argv)
{
    struct C3dDevice *d = &g_3dDevice;
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
    d->hwnd = SDL_CreateWindow("gfx test", W, H, SDL_WINDOW_HIDDEN);
    if (!d->hwnd) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->width = W;
    d->height = H;
    assert(gfx_open(d) == 0);
    if (gfx_create_screen(d) != 0) {
        fprintf(stderr, "skipped: no GPU device\n");
        return 77;
    }
    assert(gfx_get_caps(&g_gfxCaps) == 0);
    assert(gfx_create_zbuffer(d) == 0);

    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff102030, 1.0f, NULL);
    assert(screen_pixel(d, 0, 0) == 0x102030 && screen_pixel(d, W - 1, H - 1) == 0x102030);

    gfx_set_mode(FILTERED);
    gfx_set_texture(0, NULL);
    quad(v, 8, 8, 24, 20, 0xffff0000);
    gfx_draw_vertices(v, 4, 1, 0);
    gfx_end_scene();
    assert(screen_pixel(d, 8, 8) == 0xff0000 && screen_pixel(d, 23, 19) == 0xff0000);
    assert(screen_pixel(d, 7, 8) == 0x102030 && screen_pixel(d, 24, 19) == 0x102030 && screen_pixel(d, 8, 20) == 0x102030);

    /* edges through pixel centres: the top-left rule takes the top and left ones, multisampled
       too, as Direct3D 9 drew flat quads without multisampling */
    gfx_clear(GFX_CLEAR_TARGET, 0xff102030, 1.0f, NULL);
    quad(v, 8.5f, 8.5f, 24.5f, 20.5f, 0xff00ff00);
    gfx_draw_vertices(v, 4, 1, 0);
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
        gfx_draw_vertices(v, 4, 1, 0);
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
        gfx_draw_vertices(v, 4, 1, 0);
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
    gfx_draw_vertices(v, 4, 1, 0);
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
    gfx_draw_vertices(v, 4, 1, 0);
    gfx_set_mode(COLOUR);
    quad(v, 0, 0, 16, 8, 0xff0000ff);
    gfx_draw_vertices(v, 4, 1, 0);
    gfx_end_scene();
    assert(gfx_set_render_target(d->target) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gfx_set_mode(TEXTURE);
    gfx_set_texture(0, rt);
    quad(v, 0, 0, 16, 16, 0xffffffff);
    gfx_draw_vertices(v, 4, 1, 0);
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
        assert(gfx_blit(d->target, &dst, sys, NULL) == 0);
    }
    assert(screen_pixel(d, 12, 11) == 0xffff00 && screen_pixel(d, 12, 16) == 0xff00ff && screen_pixel(d, 9, 9) == 0);
    gfx_release(sys);

    check_mipmaps(d);
    check_layers(d);
    check_triangles(d);
    check_target_copies(d);
    check_read_frame(d);
    check_target_edges(d);
    linear = check_window_target(d);
    check_wobble(d);
    check_clears(d);
    check_world(d);

    check_resize(d);
    presented = check_present(d);
    gfx_close(d);
    SDL_DestroyWindow(d->hwnd);
    SDL_Quit();
    printf("gfx gpu (RAGEXE_MSAA=%s): clears, edges and flat quads, 8888/565/1555/4444 textures, texel centres, "
           "culling, textures changed and released in a frame, alpha test, fog (the quads' at every step), blending, "
           "the effect modes' blending, the 16 presets against Direct3D 9's stages, two textures, the ground's meshes, "
           "two-pass alpha, effects behind depth, composites, clears inside a pass, the overlays batched, mipmaps, "
           "layered textures read as plain ones (filtered and mipmapped, one draw of several layers, a full "
           "array and a second, a layer given up and taken, arrays of 16 MB), lists of triangles, "
           "render targets and their edges, a window's target read 1:1 (%s), the wobble in a frame, modes kept from targets "
           "they do not draw into, modes counted, system surface blits, copies between targets and out of "
           "bounds, frames read whole and in part, world vertices and meshes, resizing, %s passed\n",
           argc > 1 ? argv[1] : "4", linear ? "by the nearest texel and filtered" : "by the nearest texel only",
           presented ? "the window claimed when shown and presented into"
                     : "the window unclaimed while hidden (no swapchain to present into on this video driver)");
    return 0;
}
