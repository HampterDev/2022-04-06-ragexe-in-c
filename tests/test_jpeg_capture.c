#include "jpeg_fixture.h"
static FILE *capture_open(const char *name, const char *mode);
static int capture_close(FILE *file);
#define fopen capture_open
#define fclose capture_close
#define malloc(size) jpeg_test_calloc(1, (size))
#define free jpeg_test_free
#include "../src/render/frame_capture.c"
#undef malloc
#undef free
#undef fopen
#undef fclose

/* render/frame_capture.c's screenshot over a frame the device would download (gfx_read_frame): its
   pixels become the JPEG's, and every failure on the way (the frame's size, memory, the download,
   the encoder, the file) writes nothing, or reports the write failed, and leaves nothing allocated. */
struct C3dDevice g_3dDevice;
static struct Renderer renderer;
static int surface_token;
static int frame_width, frame_height, sizes, reads, fail_size, fail_read, destroyed_strings;
static int fail_open, fail_close, opens, closes, output_token;

static FILE *capture_open(const char *name, const char *mode)
{
    assert(!strcmp(name, "capture.jpg") && !strcmp(mode, "wb"));
    ++opens;
    return fail_open ? NULL : (FILE *)(void *)&output_token;
}

static int capture_close(FILE *file)
{
    assert(file == (FILE *)(void *)&output_token);
    ++closes;
    return fail_close ? EOF : 0;
}

int gfx_get_size(gfx_surface *handle, int *w, int *h)
{
    assert(handle == (gfx_surface *)&surface_token);
    ++sizes;
    *w = frame_width;
    *h = frame_height;
    return fail_size ? -1 : 0;
}

/* B8G8R8A8 rows: blue 10 + x, green 20 + y, red 30 + x + y. */
int gfx_read_frame(const RECT *r, void *out, size_t pitch)
{
    int x, y;
    assert(r && r->left == 0 && r->top == 0 && r->right == frame_width && r->bottom == frame_height);
    assert(pitch == (size_t)frame_width * 4);
    ++reads;
    if (fail_read)
        return -1;
    for (y = 0; y < frame_height; ++y)
        for (x = 0; x < frame_width; ++x) {
            unsigned char *p = (unsigned char *)out + y * pitch + x * 4;
            p[0] = (unsigned char)(10 + x);
            p[1] = (unsigned char)(20 + y);
            p[2] = (unsigned char)(30 + x + y);
            p[3] = 0xff;
        }
    return 0;
}

struct ResMgr *g_resMgr(void) { abort(); }
void *res_mgr_get(struct ResMgr *manager, const char *name)
{
    /* Capturing the framebuffer must not depend on logo resources. */
    (void)manager;
    (void)name;
    abort();
}
void string_free(struct String *name)
{
    assert(!strcmp(string_cstr(name), "capture.jpg"));
    ++destroyed_strings;
    string_init(name);
}

static void reset_capture(void)
{
    jpeg_fixture_reset();
    memset(&renderer, 0, sizeof renderer);
    memset(&g_3dDevice, 0, sizeof g_3dDevice);
    sizes = reads = fail_size = fail_read = destroyed_strings = 0;
    fail_open = fail_close = opens = closes = 0;
    frame_width = 5;
    frame_height = 4;
    renderer.width = 5;
    renderer.height = 4;
    g_3dDevice.target = (gfx_surface *)&surface_token;
}

static int save(void)
{
    char file[] = "capture.jpg";
    struct String name;
    name.data = file;
    name.size = name.capacity = sizeof file - 1;
    return renderer_screenshot(&renderer, name);
}

static void check_pixels(void)
{
    int x, y;
    reset_capture();
    assert(save());
    assert(jpeg_written_width == 5 && jpeg_written_height == 4);
    for (y = 0; y < 4; ++y) {
        const unsigned char *row = jpeg_written + (size_t)y * jpeg_written_stride;
        size_t pad;
        for (x = 0; x < 5; ++x) {
            const unsigned char *p = row + 3 * x;
            assert(p[0] == 10 + x && p[1] == 20 + y && p[2] == 30 + x + y);
        }
        for (pad = 5 * 3; pad < jpeg_written_stride; ++pad)
            assert(row[pad] == 0);
    }
    assert(sizes == 1 && reads == 1 && !jpeg_live && opens == 1 && closes == 1);
    assert(destroyed_strings == 1);
}

static void check_failures(void)
{
    int failure;
    for (failure = 0; failure < 8; ++failure) {
        reset_capture();
        if (failure == 0) fail_size = 1;
        if (failure == 1) frame_width = 0;
        if (failure == 2) jpeg_fail_allocation = 1;
        if (failure == 3) fail_read = 1;
        if (failure == 4) jpeg_fail_allocation = 2;
        if (failure == 5) jpeg_fail_write = 1;
        if (failure == 6) fail_open = 1;
        if (failure == 7) fail_close = 1;
        assert(!save());
        assert(reads == (failure >= 3));
        assert(opens == (failure >= 5) && closes == (failure == 5 || failure == 7));
        assert(!jpeg_live && destroyed_strings == 1);
    }
}

int main(void)
{
    check_pixels();
    check_failures();
    jpeg_fixture_reset();
    puts("Frame capture: JPEG screenshots of the downloaded frame, no resource lookups, failure cleanup passed");
    return 0;
}
