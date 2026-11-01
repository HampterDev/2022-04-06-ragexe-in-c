#include "jpeg_fixture.h"
#include "../src/render/frame_capture.c"

struct C3dDevice g_3dDevice;
static struct Renderer renderer;
static unsigned char frame[1024];
static struct gfx_lock frame_lock;
static int surface_token;
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
struct ResMgr *g_resMgr(void) { abort(); }
void *res_mgr_get(struct ResMgr *manager, const char *name)
{
    /* Capturing the framebuffer must not depend on logo resources. */
    abort();
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

static int save(void)
{
    char file[] = "capture.jpg";
    struct String name;
    name.data = file;
    name.size = name.capacity = sizeof file - 1;
    return renderer_screenshot(&renderer, name);
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
    int bits, x, y;
    for (bits = 16; bits <= 32; bits += 16) {
        int width = 5, height = 4;
        reset_capture(bits);
        assert(save());
        assert(jpeg_written_width == width && jpeg_written_height == height);
        for (y = 0; y < height; ++y) {
            const unsigned char *row = jpeg_written + (size_t)y * jpeg_written_stride;
            size_t pad;
            for (x = 0; x < width; ++x) {
                unsigned char expected[3];
                expected_pixel(expected, x, y, bits);
                assert(!memcmp(row + 3*x, expected, 3));
            }
            for (pad = (size_t)width * 3; pad < jpeg_written_stride; ++pad) assert(row[pad] == 0);
        }
        assert(locks == 1 && unlocks == 1 && !held && !jpeg_live && jpeg_frees == 1);
        assert(destroyed_strings == 1);
    }
}

static void check_failures(void)
{
    int failure;
    for (failure = 0; failure < 6; ++failure) {
        reset_capture(32);
        if (failure == 0) fail_lock = 1;
        if (failure == 1) jpeg_fail_allocation = 1;
        if (failure == 2) jpeg_fail_init = 1;
        if (failure == 3) jpeg_fail_write = 1;
        if (failure == 4) frame_lock.pitch = 1;
        if (failure == 5) frame_lock.bpp = 24;
        assert(!save());
        assert(locks == 1 && unlocks == (failure != 0) && !held && !jpeg_live && !jpeg_active);
        assert(jpeg_frees == (failure == 3));
        assert(destroyed_strings == 1);
    }

}

int main(void)
{
    check_pixels();
    check_failures();
    jpeg_fixture_reset();
    puts("Frame capture: normal JPEG screenshots, 16/32-bit pixels, independent pitches, no resource lookups, failure cleanup and unlocks passed");
    return 0;
}
