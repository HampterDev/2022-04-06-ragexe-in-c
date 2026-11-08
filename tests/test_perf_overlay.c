#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "diag/perf.h"
#include "render/device.h"
#include "render/gfx.h"
#include "render/renderer.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"


static const char *environment;
static SHORT scroll_lock;
static LONGLONG clock_now;
static int clock_reads, draws;

static DWORD WINAPI test_environment(LPCSTR name, LPSTR buffer, DWORD size)
{
    size_t length;

    assert(strcmp(name, "RAGEXE_OVERLAY") == 0);
    if (!environment)
        return 0;
    length = strlen(environment);
    assert(length < size);
    memcpy(buffer, environment, length + 1);
    return (DWORD)length;
}

static SHORT WINAPI test_key_state(int key)
{
    assert(key == VK_SCROLL);
    return scroll_lock;
}

static BOOL WINAPI test_counter(LARGE_INTEGER *t)
{
    ++clock_reads;
    t->QuadPart = clock_now;
    return TRUE;
}

static BOOL WINAPI test_frequency(LARGE_INTEGER *f)
{
    f->QuadPart = 1000;
    return TRUE;
}

struct C3dDevice g_3dDevice;
static DWORD pixels[1024 * 128];
static char texture;
int gfx_texture_format_ok(int fmt) { (void)fmt; return 1; }
gfx_surface *gfx_create_texture(int w, int h, int fmt) { (void)w; (void)h; (void)fmt; return (gfx_surface *)&texture; }
void gfx_release(gfx_surface *s) { (void)s; }
int gfx_lock(gfx_surface *s, const RECT *r, int flags, struct gfx_lock *out)
{
    (void)s; (void)r; (void)flags;
    out->bits = pixels;
    out->pitch = 1024 * 4;
    out->width = 1024;
    out->height = 128;
    out->bpp = 32;
    return 0;
}
int gfx_unlock(gfx_surface *s) { (void)s; return 0; }
int gfx_get_render_state(int state, DWORD *out) { (void)state; *out = 0; return 0; }
void gfx_set_render_state(int state, DWORD value) { (void)state; (void)value; }
void gfx_set_stage_state(int stage, int state, DWORD value) { (void)stage; (void)state; (void)value; }
void gfx_set_texture(int stage, gfx_surface *s) { (void)stage; (void)s; }
void gfx_draw(int prim, int vertexKind, const void *verts, int nverts, const WORD *indices, int nindices)
{
    (void)prim; (void)vertexKind; (void)verts; (void)indices; (void)nindices;
    assert(nverts == 4);
    ++draws;
}
void renderer_set_blend_stages(struct Renderer *self, int mode) { (void)self; (void)mode; }

#define GetEnvironmentVariableA test_environment
#define GetKeyState test_key_state
#define QueryPerformanceCounter test_counter
#define QueryPerformanceFrequency test_frequency
#include "perf_overlay.inc"
#undef GetEnvironmentVariableA
#undef GetKeyState
#undef QueryPerformanceCounter
#undef QueryPerformanceFrequency

static struct Renderer renderer;
static int cases;

static void start(const char *value)
{
    memset(&P, 0, sizeof P);
    environment = value;
    scroll_lock = 0;
    clock_now = 1000;
    clock_reads = draws = 0;
    g_3dDevice.width = 800;
    g_3dDevice.height = 600;
}

static int frame(void)
{
    int before = draws;

    clock_now += 100;
    perf_begin(PERF_SCENE);
    perf_end(PERF_SCENE);
    perf_frame_end(&renderer);
    perf_overlay_draw(&renderer);
    return draws - before;
}

static void check_off(const char *value)
{
    int i, reads;

    start(value);
    frame();
    reads = clock_reads;
    for (i = 0; i < 40; ++i) {
        if (i % 5 == 0)
            scroll_lock ^= 1;
        assert(frame() == 0);
    }
    assert(draws == 0 && clock_reads == reads);
    ++cases;
}

static void check_on(void)
{
    int i;

    start("1");
    for (i = 0; i < 6; ++i)
        frame();
    assert(frame() == 1 && frame() == 1);
    scroll_lock ^= 1;
    assert(frame() == 0 && frame() == 0);
    scroll_lock ^= 1;
    assert(frame() == 1);
    assert(clock_reads > 0);
    ++cases;
}

#include "screenshot_keys.inc"
static struct Mode *const current_mode = (struct Mode *)0x6d6f;
static int screenshots;
struct Mode *client_current_mode(void) { return current_mode; }
int mode_send_message(struct Mode *mode, int message, int a, int b, int c, int d)
{
    assert(mode == current_mode && message == MODE_MSG_SCREENSHOT && !a && !b && !c && !d);
    ++screenshots;
    return 0;
}

static void handle_key(int vk, int first_press)
{
    switch (vk) {
#include "screenshot_case.inc"
    }
}

static void check_screenshot_keys(void)
{
    static const char *const values[] = { NULL, "0", "1" };
    size_t i;

    for (i = 0; i < sizeof values / sizeof *values; ++i) {
        start(values[i]);
        screenshots = 0;
        handle_key(VK_SCROLL, 1);
        handle_key(VK_SCROLL, 0);
        handle_key(VK_SNAPSHOT, 1);
        handle_key(VK_SNAPSHOT, 0);
        assert(screenshots == 2);
        ++cases;
    }
}

int main(void)
{
    check_off(NULL);
    check_off("0");
    check_on();
    check_screenshot_keys();
    printf("Frame-time overlay: %d cases; off without RAGEXE_OVERLAY, Scroll Lock still takes screenshots.\n",
           cases);
    return 0;
}
