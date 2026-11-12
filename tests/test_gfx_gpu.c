/* render/gfx_gpu.c through render/gfx.h on a real GPU, in a hidden window: what each draw puts on
   the target, read back through gfx_lock. Skipped (77) where SDL finds no GPU device. The argument
   is RAGEXE_MSAA: the test runs once without multisampling and once with it. */
#include "render/gfx.h"
#include "render/device.h"
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
    DWORD diffuse, specular;
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

static void quad(struct Vertex *v, float x0, float y0, float x1, float y1, DWORD diffuse, DWORD specular)
{
    int i;
    for (i = 0; i < 4; i++) {
        v[i].z = 0.5f;
        v[i].rhw = 1.0f;
        v[i].diffuse = diffuse;
        v[i].specular = specular;
    }
    v[0].x = x0 - 0.5f; v[0].y = y0 - 0.5f; v[0].u = 0.0f; v[0].v = 0.0f;
    v[1].x = x1 - 0.5f; v[1].y = y0 - 0.5f; v[1].u = 1.0f; v[1].v = 0.0f;
    v[2].x = x0 - 0.5f; v[2].y = y1 - 0.5f; v[2].u = 0.0f; v[2].v = 1.0f;
    v[3].x = x1 - 0.5f; v[3].y = y1 - 0.5f; v[3].u = 1.0f; v[3].v = 1.0f;
}

static void stage(int op, int arg1, int aop, int aarg1)
{
    gfx_set_stage_state(0, GFX_TSS_COLOROP, op);
    gfx_set_stage_state(0, GFX_TSS_COLORARG1, arg1);
    gfx_set_stage_state(0, GFX_TSS_COLORARG2, GFX_TA_DIFFUSE);
    gfx_set_stage_state(0, GFX_TSS_ALPHAOP, aop);
    gfx_set_stage_state(0, GFX_TSS_ALPHAARG1, aarg1);
    gfx_set_stage_state(0, GFX_TSS_ALPHAARG2, GFX_TA_DIFFUSE);
    gfx_set_stage_state(1, GFX_TSS_COLOROP, GFX_TOP_DISABLE);
    gfx_set_stage_state(1, GFX_TSS_ALPHAOP, GFX_TOP_DISABLE);
    gfx_set_stage_state(0, GFX_TSS_MINFILTER, 1);
    gfx_set_stage_state(0, GFX_TSS_MAGFILTER, 1);
    gfx_set_stage_state(0, GFX_TSS_ADDRESS, GFX_ADDRESS_CLAMP);
}

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
    struct Vertex v[4];
    quad(v, x0, y0, x1, y1, diffuse, 0);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
}

static void check_texture(struct C3dDevice *d, int fmt)
{
    static const DWORD texels[4] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff };
    gfx_surface *t = texture_2x2(fmt, texels);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0, 1.0f, NULL);
    stage(GFX_TOP_SELECTARG1, GFX_TA_TEXTURE, GFX_TOP_SELECTARG1, GFX_TA_TEXTURE);
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

/* A texture drawn one texel per pixel with bilinear filtering reads each texel at its centre, as
   Direct3D 9 does with its pixel centres. Magnified: a linear minification is anisotropic, which
   Mesa's lavapipe does not bring to the bilinear result at one texel per pixel. */
static void check_texel_centres(struct C3dDevice *d)
{
    static const DWORD texels[4] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, texels);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0, 1.0f, NULL);
    stage(GFX_TOP_SELECTARG1, GFX_TA_TEXTURE, GFX_TOP_SELECTARG1, GFX_TA_TEXTURE);
    gfx_set_stage_state(0, GFX_TSS_MAGFILTER, GFX_FILTER_LINEAR);
    gfx_set_texture(0, t);
    draw_quad(10, 10, 12, 12, 0xffffffff);
    gfx_end_scene();
    assert(screen_pixel(d, 10, 10) == 0xff0000);
    assert(screen_pixel(d, 11, 10) == 0x00ff00);
    assert(screen_pixel(d, 10, 11) == 0x0000ff);
    assert(screen_pixel(d, 11, 11) == 0xffffff);
    gfx_set_texture(0, NULL);
    gfx_release(t);
}

/* Direct3D 9 culls by the winding on the screen: GFX_CULL_CW drops the triangles whose corners go
   clockwise there (y down), GFX_CULL_CCW the others. */
static void check_culling(struct C3dDevice *d)
{
    struct Vertex v[3];
    static const float cw[3][2] = { { 4, 4 }, { 20, 4 }, { 4, 20 } }, ccw[3][2] = { { 30, 4 }, { 30, 20 }, { 46, 4 } };
    int mode, i;
    stage(GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE, GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE);
    for (mode = GFX_CULL_NONE; mode <= GFX_CULL_CCW; mode++) {
        gfx_clear(GFX_CLEAR_TARGET, 0, 1.0f, NULL);
        gfx_set_render_state(GFX_RS_CULLMODE, (DWORD)mode);
        for (i = 0; i < 3; i++) {
            memset(&v[i], 0, sizeof v[i]);
            v[i].rhw = 1.0f;
            v[i].diffuse = 0xffffffff;
            v[i].x = cw[i][0];
            v[i].y = cw[i][1];
        }
        gfx_draw(GFX_PT_TRIANGLELIST, GFX_VTX_TEX1, v, 3, NULL, 0);
        for (i = 0; i < 3; i++) {
            v[i].x = ccw[i][0];
            v[i].y = ccw[i][1];
        }
        gfx_draw(GFX_PT_TRIANGLELIST, GFX_VTX_TEX1, v, 3, NULL, 0);
        gfx_end_scene();
        assert((screen_pixel(d, 7, 7) != 0) == (mode != GFX_CULL_CW));
        assert((screen_pixel(d, 33, 7) != 0) == (mode != GFX_CULL_CCW));
    }
    gfx_set_render_state(GFX_RS_CULLMODE, GFX_CULL_NONE);
}

/* A texture changed after a draw used it keeps its old texels for that draw and gives the new
   ones to the draws after the change, in one frame. */
static void check_texture_changed_in_frame(struct C3dDevice *d)
{
    static const DWORD red[4] = { 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000 };
    static const DWORD green[4] = { 0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00 };
    gfx_surface *t = texture_2x2(GFX_FMT_8888, red);
    gfx_clear(GFX_CLEAR_TARGET, 0, 1.0f, NULL);
    stage(GFX_TOP_SELECTARG1, GFX_TA_TEXTURE, GFX_TOP_SELECTARG1, GFX_TA_TEXTURE);
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
    stage(GFX_TOP_SELECTARG1, GFX_TA_TEXTURE, GFX_TOP_SELECTARG1, GFX_TA_TEXTURE);
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
    stage(GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE, GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE);
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
        stage(GFX_TOP_SELECTARG1, GFX_TA_TEXTURE, GFX_TOP_SELECTARG1, GFX_TA_TEXTURE);
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
    stage(GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE, GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE);
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
    stage(GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE, GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE);
    draw_quad(0, 0, 15, 9, 0xff00ff00);
    gfx_end_scene();
    assert(gfx_set_render_target(d->target) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    stage(GFX_TOP_SELECTARG1, GFX_TA_TEXTURE, GFX_TOP_SELECTARG1, GFX_TA_TEXTURE);
    gfx_set_stage_state(0, GFX_TSS_MINFILTER, GFX_FILTER_LINEAR);
    gfx_set_stage_state(0, GFX_TSS_MAGFILTER, GFX_FILTER_LINEAR);
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
    stage(GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE, GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE);
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
    stage(GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE, GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE);
    gfx_set_render_state(GFX_RS_ZENABLE, 0);
    draw_quad(0, 0, 32, 32, 0xffffffff);
    gfx_clear(GFX_CLEAR_TARGET, 0xff0000ff, 1.0f, &r);
    gfx_end_scene();
    assert(screen_pixel(d, 12, 12) == 0x0000ff && screen_pixel(d, 7, 12) == 0xffffff && screen_pixel(d, 16, 12) == 0xffffff);
    /* depth: a near quad, the depth cleared under part of it, then a far one */
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    gfx_set_render_state(GFX_RS_ZENABLE, 1);
    gfx_set_render_state(GFX_RS_ZWRITEENABLE, 1);
    gfx_set_render_state(GFX_RS_ZFUNC, GFX_CMP_LESSEQUAL);
    {
        struct Vertex v[4];
        quad(v, 0, 0, 32, 32, 0xffff0000, 0);
        gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
        gfx_clear(GFX_CLEAR_ZBUFFER, 0, 1.0f, &r);
        quad(v, 0, 0, 32, 32, 0xff00ff00, 0);
        v[0].z = v[1].z = v[2].z = v[3].z = 0.75f;
        gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    }
    gfx_end_scene();
    assert(screen_pixel(d, 12, 12) == 0x00ff00 && screen_pixel(d, 4, 4) == 0xff0000);
    gfx_set_render_state(GFX_RS_ZENABLE, 0);
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
    stage(GFX_TOP_SELECTARG1, GFX_TA_TEXTURE, GFX_TOP_SELECTARG1, GFX_TA_TEXTURE);
    gfx_set_stage_state(0, GFX_TSS_MIPFILTER, GFX_MIPFILTER_LINEAR);
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
        gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
        gfx_set_texture(0, t);
        draw_quad(0, 0, 4, 4, 0xffffffff);
        gfx_end_scene();
        p = screen_pixel(d, 1, 1);
        if (pass == 0)
            assert((p & 0xff) >= 0x7e && (p & 0xff) <= 0x81 && p == (p & 0xff) * 0x010101u);
        else
            assert((p & 0xff) == 0xff && (p >> 8 & 0xff) >= 0x7e && (p >> 8 & 0xff) <= 0x81);
    }
    gfx_set_stage_state(0, GFX_TSS_MIPFILTER, GFX_MIPFILTER_NONE);
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
    v->fogStart = 1000.0f;
    v->fogEnd = 2000.0f;
}

/* World vertices and a mesh land on the pixels the same corners pre-transformed cover. */
static void check_world(struct C3dDevice *d)
{
    struct GfxWorldView view;
    struct Vertex v[4];
    struct GfxMeshVertex mv[4];
    static const WORD indices[6] = { 0, 1, 2, 2, 1, 3 };
    struct GfxMeshDraw draw;
    gfx_mesh *mesh;
    int i;
    test_view(&view);
    gfx_set_world_camera(&view);
    gfx_set_world_view(&view);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff000000, 1.0f, NULL);
    stage(GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE, GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE);
    /* world x = -8..0, y = -8..0 at depth 100 cover the pixels 24..31, 16..23 */
    quad(v, 0, 0, 1, 1, 0xff00ff00, 0);
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
    gfx_set_texture(0, NULL);
    gfx_set_render_state(GFX_RS_ALPHATESTENABLE, 0);
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

/* A window resized: the screen and its copy take the new size, keep drawing and lock at it, and go
   back. */
static void check_resize(struct C3dDevice *d)
{
    struct gfx_lock lk;
    assert(gfx_resize(d, 80, 40) == 0);
    d->width = 80;
    d->height = 40;
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff405060, 1.0f, NULL);
    stage(GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE, GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE);
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
    gfx_default_states(d);

    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0xff102030, 1.0f, NULL);
    assert(screen_pixel(d, 0, 0) == 0x102030 && screen_pixel(d, W - 1, H - 1) == 0x102030);

    gfx_set_render_state(GFX_RS_ZENABLE, 0);
    gfx_set_render_state(GFX_RS_ALPHABLENDENABLE, 0);
    stage(GFX_TOP_MODULATE, GFX_TA_TEXTURE, GFX_TOP_MODULATE, GFX_TA_TEXTURE);
    gfx_set_texture(0, NULL);
    quad(v, 8, 8, 24, 20, 0xffff0000, 0);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_end_scene();
    assert(screen_pixel(d, 8, 8) == 0xff0000 && screen_pixel(d, 23, 19) == 0xff0000);
    assert(screen_pixel(d, 7, 8) == 0x102030 && screen_pixel(d, 24, 19) == 0x102030 && screen_pixel(d, 8, 20) == 0x102030);

    /* edges through pixel centres: the top-left rule takes the top and left ones, multisampled
       too, as Direct3D 9 drew flat quads without multisampling */
    gfx_clear(GFX_CLEAR_TARGET, 0xff102030, 1.0f, NULL);
    quad(v, 8.5f, 8.5f, 24.5f, 20.5f, 0xff00ff00, 0);
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
        stage(GFX_TOP_SELECTARG1, GFX_TA_TEXTURE, GFX_TOP_SELECTARG1, GFX_TA_TEXTURE);
        gfx_set_texture(0, t);
        quad(v, 10.25f, 10.25f, 30.25f, 30.25f, 0xffffffff, 0);
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
        gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
        stage(GFX_TOP_SELECTARG1, GFX_TA_TEXTURE, GFX_TOP_SELECTARG1, GFX_TA_TEXTURE);
        gfx_set_render_state(GFX_RS_ALPHATESTENABLE, 1);
        gfx_set_render_state(GFX_RS_ALPHAFUNC, GFX_CMP_GREATEREQUAL);
        gfx_set_render_state(GFX_RS_ALPHAREF, 0x80);
        gfx_set_texture(0, t);
        quad(v, 0, 0, 32, 32, 0xffffffff, 0);
        gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
        gfx_end_scene();
        assert(screen_pixel(d, 4, 4) == 0 && screen_pixel(d, 28, 4) == 0x00ff00 && screen_pixel(d, 28, 28) == 0);
        gfx_set_render_state(GFX_RS_ALPHATESTENABLE, 0);
        gfx_set_texture(0, NULL);
        gfx_release(t);
    }

    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    stage(GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE, GFX_TOP_SELECTARG1, GFX_TA_DIFFUSE);
    gfx_set_render_state(GFX_RS_FOGCOLOR, 0xff00ff00);
    gfx_set_render_state(GFX_RS_FOGENABLE, 1);
    quad(v, 0, 0, 16, 16, 0xffff0000, 0x00000000);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    quad(v, 16, 0, 32, 16, 0xffff0000, 0xff000000);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_end_scene();
    assert(screen_pixel(d, 4, 4) == 0x00ff00 && screen_pixel(d, 20, 4) == 0xff0000);
    gfx_set_render_state(GFX_RS_FOGENABLE, 0);

    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    gfx_set_render_state(GFX_RS_ALPHABLENDENABLE, 1);
    gfx_set_render_state(GFX_RS_SRCBLEND, GFX_BLEND_SRCALPHA);
    gfx_set_render_state(GFX_RS_DESTBLEND, GFX_BLEND_INVSRCALPHA);
    quad(v, 0, 0, 16, 16, 0x80ffffff, 0);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_end_scene();
    x = (int)(screen_pixel(d, 4, 4) & 0xff);
    assert(x >= 0x7e && x <= 0x82);
    gfx_set_render_state(GFX_RS_ALPHABLENDENABLE, 0);

    rt = gfx_create_render_target(16, 16);
    assert(rt);
    assert(gfx_set_render_target(rt) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    quad(v, 0, 0, 16, 8, 0xff0000ff, 0);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_end_scene();
    assert(gfx_set_render_target(d->target) == 0);
    gfx_clear(GFX_CLEAR_TARGET, 0xff000000, 1.0f, NULL);
    stage(GFX_TOP_SELECTARG1, GFX_TA_TEXTURE, GFX_TOP_SELECTARG1, GFX_TA_TEXTURE);
    gfx_set_texture(0, rt);
    quad(v, 0, 0, 16, 16, 0xffffffff, 0);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_end_scene();
    assert(screen_pixel(d, 4, 2) == 0x0000ff && screen_pixel(d, 4, 12) == 0);
    gfx_set_texture(0, NULL);
    gfx_release(rt);

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

    check_resize(d);
    assert(gfx_present(d, 0) == 0);
    gfx_close(d);
    SDL_DestroyWindow(d->hwnd);
    SDL_Quit();
    printf("gfx gpu (RAGEXE_MSAA=%s): clears, edges and flat quads, 8888/565/1555/4444 textures, texel centres, "
           "culling, textures changed and released in a frame, alpha test, vertex fog, blending, mipmaps, render "
           "targets and their edges, system surface blits, target copies, stretched and out of bounds, "
           "write-back, rectangle locks, world vertices and meshes, resizing passed\n", argc > 1 ? argv[1] : "4");
    return 0;
}
