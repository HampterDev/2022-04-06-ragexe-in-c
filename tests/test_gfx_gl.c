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

static gfx_surface *texture_2x2(int fmt, const DWORD argb[4])
{
    gfx_surface *t = gfx_create_texture(2, 2, fmt);
    struct gfx_lock lk;
    int i;
    assert(t && gfx_lock(t, NULL, 0, &lk) == 0);
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
    return t;
}

static void check_texture(struct C3dDevice *d, int fmt)
{
    static const DWORD texels[4] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff };
    struct Vertex v[4];
    gfx_surface *t = texture_2x2(fmt, texels);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0, 1.0f, NULL);
    stage(GFX_TOP_SELECTARG1, GFX_TA_TEXTURE, GFX_TOP_SELECTARG1, GFX_TA_TEXTURE);
    gfx_set_texture(0, t);
    quad(v, 0, 0, 32, 32, 0xffffffff, 0);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
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
   Direct3D does: the edge nudge that breaks coverage ties must not move the samples. */
static void check_texel_centres(struct C3dDevice *d)
{
    static const DWORD texels[4] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff };
    struct Vertex v[4];
    gfx_surface *t = texture_2x2(GFX_FMT_8888, texels);
    gfx_clear(GFX_CLEAR_TARGET | GFX_CLEAR_ZBUFFER, 0, 1.0f, NULL);
    stage(GFX_TOP_SELECTARG1, GFX_TA_TEXTURE, GFX_TOP_SELECTARG1, GFX_TA_TEXTURE);
    gfx_set_stage_state(0, GFX_TSS_MINFILTER, GFX_FILTER_LINEAR);
    gfx_set_stage_state(0, GFX_TSS_MAGFILTER, GFX_FILTER_LINEAR);
    gfx_set_texture(0, t);
    quad(v, 10, 10, 12, 12, 0xffffffff, 0);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_end_scene();
    assert(screen_pixel(d, 10, 10) == 0xff0000);
    assert(screen_pixel(d, 11, 10) == 0x00ff00);
    assert(screen_pixel(d, 10, 11) == 0x0000ff);
    assert(screen_pixel(d, 11, 11) == 0xffffff);
    gfx_set_texture(0, NULL);
    gfx_release(t);
}

int main(void)
{
    struct C3dDevice *d = &g_3dDevice;
    struct SurfaceDesc mode = {0};
    struct Vertex v[4];
    gfx_surface *rt, *sys;
    struct gfx_lock lk;
    int x, y;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    d->hwnd = (HWND)SDL_CreateWindow("gfx test", W, H, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!d->hwnd) {
        fprintf(stderr, "skipped: %s\n", SDL_GetError());
        return 77;
    }
    d->width = W;
    d->height = H;
    assert(gfx_open(d, NULL, 0) == 0);
    assert(gfx_create_screen(d, &mode, 2) == 0);
    assert(gfx_create_3d(d, NULL, 0) == 0);
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

    gfx_clear(GFX_CLEAR_TARGET, 0xff102030, 1.0f, NULL);
    quad(v, 8.5f, 8.5f, 24.5f, 20.5f, 0xff00ff00, 0);
    gfx_draw(GFX_PT_TRIANGLESTRIP, GFX_VTX_TEX1, v, 4, NULL, 0);
    gfx_end_scene();
    assert(screen_pixel(d, 8, 8) == 0x00ff00 && screen_pixel(d, 23, 19) == 0x00ff00);
    assert(screen_pixel(d, 7, 8) == 0x102030 && screen_pixel(d, 8, 7) == 0x102030);
    assert(screen_pixel(d, 24, 19) == 0x102030 && screen_pixel(d, 23, 20) == 0x102030);

    check_texture(d, GFX_FMT_8888);
    check_texture(d, GFX_FMT_565);
    check_texture(d, GFX_FMT_1555);
    check_texture(d, GFX_FMT_4444);
    check_texel_centres(d);

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

    assert(gfx_present(d, 0, 0) == 0);
    gfx_close(d);
    SDL_DestroyWindow((SDL_Window *)d->hwnd);
    SDL_Quit();
    puts("gfx gl: clear, untextured, 8888/565/1555/4444 textures, texel centres, alpha test, vertex fog, blending, render targets and system surface blits passed");
    return 0;
}
