#include "jpeg_fixture.h"
#include "../src/render/frame_capture.c"

struct C3dDevice g_3dDevice;
static struct Renderer renderer;
static struct BitmapRes logo;
static DWORD logo_pixels[4];
static unsigned char frame[1024];
static struct gfx_lock frame_lock;
static int surface_token, resource_token;
static int locks, unlocks, held, fail_lock, destroyed_strings;

int gfx_lock(gfx_surface *handle, const RECT *rect, int flags, struct gfx_lock *out)
{
    assert(handle == (gfx_surface *)&surface_token && !rect && flags == GFX_LOCK_WAIT);
    ++locks;
    if (fail_lock) return -1;
    assert(!held);
    held = 1;
    *out = frame_lock;
    return 0;
}
int gfx_unlock(gfx_surface *handle)
{
    assert(handle == (gfx_surface *)&surface_token && held);
    ++unlocks;
    held = 0;
    return 0;
}
struct ResMgr *g_resMgr(void) { return (struct ResMgr *)&resource_token; }
void *res_mgr_get(struct ResMgr *manager, const char *name)
{
    assert(manager == g_resMgr() && !strcmp(name, "scr_logo.bmp"));
    return &logo;
}
void string_free(struct String *name)
{
    assert(!strcmp(string_cstr(name), "capture.jpg"));
    ++destroyed_strings;
    string_init(name);
}

static void reset_capture(int bits)
{
    int x, y;
    jpeg_fixture_reset();
    memset(&renderer, 0, sizeof renderer);
    memset(&logo, 0, sizeof logo);
    memset(&g_3dDevice, 0, sizeof g_3dDevice);
    memset(frame, 0xef, sizeof frame);
    locks = unlocks = held = fail_lock = destroyed_strings = 0;
    renderer.width = 5;
    renderer.height = 4;
    g_3dDevice.target = (gfx_surface *)&surface_token;
    g_3dDevice.bMask = 0x1f; g_3dDevice.bShift = 0; g_3dDevice.bBits = 3;
    g_3dDevice.gMask = 0x7e0; g_3dDevice.gShift = 5; g_3dDevice.gBits = 2;
    g_3dDevice.rMask = 0xf800; g_3dDevice.rShift = 11; g_3dDevice.rBits = 3;
    frame_lock.bits = frame;
    frame_lock.pitch = 32;
    frame_lock.width = 5;
    frame_lock.height = 4;
    frame_lock.bpp = bits;
    for (y = 0; y < 4; ++y) for (x = 0; x < 5; ++x) {
        unsigned char *p = frame + y * 32 + x * (bits / 8);
        if (bits == 32) {
            p[0] = (unsigned char)(10+x); p[1] = (unsigned char)(20+y);
            p[2] = (unsigned char)(30+x+y); p[3] = 0xff;
        } else {
            WORD pixel = (WORD)((x + 1) | (y + 2) << 5 | (x + y + 3) << 11);
            memcpy(p, &pixel, sizeof pixel);
        }
    }
}

static int save(int mode, const RECT *region)
{
    char file[] = "capture.jpg";
    struct String name;
    name.data = file;
    name.size = name.capacity = sizeof file - 1;
    switch (mode) {
    case 1: return renderer_save_jpeg_region(&renderer, "capture.jpg", region);
    default: return renderer_screenshot(&renderer, name);
    }
}

static void expected_pixel(unsigned char out[3], int x, int y, int bits)
{
    if (bits == 32) {
        out[0] = (unsigned char)(10+x); out[1] = (unsigned char)(20+y);
        out[2] = (unsigned char)(30+x+y);
    } else {
        out[0] = (unsigned char)((x+1)*8); out[1] = (unsigned char)((y+2)*4);
        out[2] = (unsigned char)((x+y+3)*8);
    }
}

static void check_pixels(void)
{
    const RECT region = {1, 1, 4, 3};
    int mode, bits, x, y;
    for (mode = 1; mode <= 2; ++mode) for (bits = 16; bits <= 32; bits += 16) {
        int width = mode == 1 ? 3 : 5, height = mode == 1 ? 2 : 4;
        reset_capture(bits);
        assert(save(mode, &region));
        assert(jpeg_written_width == width && jpeg_written_height == height);
        for (y = 0; y < height; ++y) {
            const unsigned char *row = jpeg_written + (size_t)y * jpeg_written_stride;
            size_t pad;
            for (x = 0; x < width; ++x) {
                unsigned char expected[3];
                expected_pixel(expected, x + (mode == 1), y + (mode == 1), bits);
                assert(!memcmp(row + 3*x, expected, 3));
            }
            for (pad = (size_t)width * 3; pad < jpeg_written_stride; ++pad) assert(row[pad] == 0);
        }
        assert(locks == 1 && unlocks == 1 && !held && !jpeg_live && jpeg_frees == 1);
        assert(destroyed_strings == (mode >= 2));
    }
}

static void check_failures(void)
{
    const RECT region = {1, 1, 4, 3};
    int mode, failure;
    for (mode = 1; mode <= 2; ++mode) for (failure = 0; failure < 6; ++failure) {
        reset_capture(32);
        if (failure == 0) fail_lock = 1;
        if (failure == 1) jpeg_fail_allocation = 1;
        if (failure == 2) jpeg_fail_init = 1;
        if (failure == 3) jpeg_fail_write = 1;
        if (failure == 4) frame_lock.pitch = 1;
        if (failure == 5) frame_lock.bpp = 24;
        assert(!save(mode, &region));
        assert(locks == 1 && unlocks == (failure != 0) && !held && !jpeg_live && !jpeg_active);
        assert(jpeg_frees == (failure == 3));
        assert(destroyed_strings == (mode >= 2));
    }
    reset_capture(32);
    {
        const RECT invalid[] = {{-1,0,2,2},{0,-1,2,2},{0,0,6,2},{0,0,2,5},{2,0,2,1},{0,2,1,1},{INT_MIN,0,INT_MAX,1}};
        size_t i;
        for (i = 0; i < sizeof invalid/sizeof invalid[0]; ++i) assert(!save(1, &invalid[i]));
        assert(!renderer_save_jpeg_region(&renderer, "capture.jpg", NULL));
        assert(!locks && !jpeg_allocations);
    }
}

static void check_logo(void)
{
    unsigned char expected[3];
    const RECT region = {1, 1, 4, 3};
    reset_capture(32);
    logo.width = logo.height = 2;
    logo.pixels = logo_pixels;
    logo_pixels[0] = 0xff00ff; logo_pixels[1] = 0x112233;
    logo_pixels[2] = 0x445566; logo_pixels[3] = 0xff00ff;
    assert(save(2, &region));
    expected_pixel(expected, 3, 2, 32);
    assert(!memcmp(jpeg_written + 2*jpeg_written_stride + 3*3, expected, 3));
    assert(!memcmp(jpeg_written + 2*jpeg_written_stride + 4*3, "\x33\x22\x11", 3));
    assert(!memcmp(jpeg_written + 3*jpeg_written_stride + 3*3, "\x66\x55\x44", 3));
}

int main(void)
{
    check_pixels();
    check_failures();
    check_logo();
    jpeg_fixture_reset();
    puts("Frame capture: two JPEG writers, 16/32-bit pixels, independent pitches, crop edges, logos, failure cleanup and unlocks passed");
    return 0;
}
