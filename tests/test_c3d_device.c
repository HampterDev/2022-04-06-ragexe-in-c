/* render/c3ddevice.c over a device that records what it is asked: the frame the fade keeps
   (c3d_device_save_frame, c3d_device_restore_frame) in a render target of the screen's size, made
   at the first save, used again while the size stays, made again after it changed and missing
   where the device makes none, a restore without a saved frame copying nothing; the device made at
   the window's client size, with the static world attached and the screen's channel masks, and
   closed again, the screen let go, when a step fails;
   the frame presented only with a screen, counted as shown either way. */
#include "../src/render/c3ddevice.c"
#include <assert.h>
#include <stdio.h>

struct gfx_surface {
    int width, height, live;
};

struct gfx_caps g_gfxCaps;
struct perf_counters g_perf;
int g_trilinearFilter;

static struct gfx_surface screen, made[8];
static int nmade, fail_target, fail_open, fail_screen, fail_zbuffer, blits, closes, presents, vsyncs, shown,
    attaches;
static gfx_surface *blit_dst, *blit_src;
static int64_t clock_ns;
static int window_token;

int gfx_get_size(gfx_surface *s, int *w, int *h)
{
    assert(s->live);
    *w = s->width;
    *h = s->height;
    return 0;
}

gfx_surface *gfx_create_render_target(int w, int h)
{
    struct gfx_surface *s;
    if (fail_target)
        return NULL;
    assert(nmade < 8);
    s = &made[nmade++];
    s->width = w;
    s->height = h;
    s->live = 1;
    return s;
}

void gfx_release(gfx_surface *s)
{
    assert(s->live);
    s->live = 0;
}

int gfx_blit(gfx_surface *dst, const RECT *dstRect, gfx_surface *src, const RECT *srcRect)
{
    assert(dst->live && src->live && !dstRect && !srcRect);
    blit_dst = dst;
    blit_src = src;
    ++blits;
    return 0;
}

int gfx_clear(int flags, DWORD colour, float z, const RECT *r)
{
    (void)flags; (void)colour; (void)z; (void)r;
    abort();
}

int gfx_world_attach(void)
{
    ++attaches;
    return 1;
}

int gfx_open(PlatformWindow window)
{
    assert(window == (PlatformWindow)(void *)&window_token && attaches);
    return fail_open ? GFX_E_DDRAW_CREATE : 0;
}

gfx_surface *gfx_create_screen(int w, int h)
{
    assert(w == 640 && h == 480);
    if (fail_screen)
        return NULL;
    screen.width = w;
    screen.height = h;
    screen.live = 1;
    return &screen;
}

int gfx_get_caps(struct gfx_caps *out)
{
    memset(out, 0, sizeof *out);
    return 0;
}

int gfx_create_zbuffer(void)
{
    assert(screen.live);
    return fail_zbuffer ? GFX_E_ZBUFFER : 0;
}

void gfx_close(void)
{
    ++closes;
}

void gfx_set_mipmaps(int on) { (void)on; }

int gfx_present(int vsync)
{
    ++presents;
    vsyncs += vsync;
    return 0;
}

void platform_client_rect(PlatformWindow window, RECT *out)
{
    assert(window == (PlatformWindow)(void *)&window_token);
    out->left = out->top = 0;
    out->right = 640;
    out->bottom = 480;
}

int64_t platform_stopwatch_ns(void) { return clock_ns += 7; }
void drive_state_frame_shown(void) { ++shown; }
void error_msg(const char *text, int flags) { (void)text; (void)flags; abort(); }
struct Renderer *renderer_construct(struct Renderer *self) { (void)self; abort(); }
void renderer_set_projection(struct Renderer *self, int width, int height) { (void)self; (void)width; (void)height; abort(); }
void renderer_destruct(struct Renderer *self) { (void)self; abort(); }

static void check_save_and_restore(void)
{
    struct C3dDevice d;
    gfx_surface *first;
    memset(&d, 0, sizeof d);
    d.width = 640;
    d.height = 480;
    c3d_device_save_frame(&d);
    c3d_device_restore_frame(&d);
    assert(!nmade && !blits && !d.save);

    d.target = &screen;
    screen.live = 1;
    c3d_device_restore_frame(&d);
    assert(!blits);
    c3d_device_save_frame(&d);
    assert(nmade == 1 && d.save == &made[0] && made[0].width == 640 && made[0].height == 480);
    assert(blits == 1 && blit_dst == d.save && blit_src == &screen);
    first = d.save;
    c3d_device_save_frame(&d);
    assert(nmade == 1 && d.save == first && first->live && blits == 2);
    c3d_device_restore_frame(&d);
    assert(blits == 3 && blit_dst == &screen && blit_src == first);

    /* the window resized: the old target goes, one of the new size comes */
    d.width = 800;
    d.height = 600;
    c3d_device_save_frame(&d);
    assert(!first->live && nmade == 2 && d.save == &made[1] && made[1].width == 800 && made[1].height == 600);
    assert(blits == 4 && blit_dst == &made[1]);

    /* resized again where the device makes no target: none kept, nothing copied or put back */
    d.width = 320;
    fail_target = 1;
    c3d_device_save_frame(&d);
    c3d_device_restore_frame(&d);
    fail_target = 0;
    assert(!made[1].live && !d.save && blits == 4);
}

static void check_start_and_show(void)
{
    struct C3dDevice d;
    int failure;
    memset(&d, 0, sizeof d);
    assert(c3d_device_init(&d, NULL) == GFX_E_INVALIDARG && !closes);
    assert(c3d_device_init(&d, (PlatformWindow)(void *)&window_token) == 0 && !closes);
    assert(d.width == 640 && d.height == 480 && d.target == &screen);
    assert(d.rShift == 16 && d.rBits == 0 && d.gShift == 8 && d.gBits == 0 && d.bShift == 0 && d.bBits == 0);
    assert(c3d_device_show_frame(&d, 2) == 0 && presents == 1 && vsyncs == 1 && shown == 1);
    assert(c3d_device_show_frame(&d, 0) == 0 && presents == 2 && vsyncs == 1 && shown == 2);
    assert(g_perf.presentNs == 14);
    for (failure = 0; failure < 3; ++failure) {
        memset(&d, 0, sizeof d);
        fail_open = failure == 0;
        fail_screen = failure == 1;
        fail_zbuffer = failure == 2;
        closes = 0;
        screen.live = 0;
        assert(c3d_device_init(&d, (PlatformWindow)(void *)&window_token) < 0 && closes == 1 && !d.target
               && !screen.live);
    }
    fail_open = fail_screen = fail_zbuffer = 0;
    assert(c3d_device_show_frame(&d, 1) == (int)GFX_E_NO_PRIMARY && presents == 2 && shown == 3);
}

int main(void)
{
    check_save_and_restore();
    check_start_and_show();
    puts("C3dDevice: the fade's frame kept in a target of the screen's size, made again after a resize, "
         "none without the device's, a restore without a frame; the start at the window's size and its "
         "failures; the frame presented only with a screen passed");
    return 0;
}
