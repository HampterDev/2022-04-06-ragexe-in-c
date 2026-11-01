#include "jpeg_fixture.h"
#include "resources/bitmap.h"
static void *jpeg_test_malloc(size_t bytes) { return jpeg_test_calloc(1, bytes); }
#define malloc jpeg_test_malloc
#define free jpeg_test_free
#include "../src/resources/bitmap_storage.c"
#include "../src/resources/bitmap_jpeg.c"
#undef malloc
#undef free

static void check_bitmap(void)
{
    struct BitmapRes bitmap, previous;
    int failure, x, y;
    for (failure = 0; failure < 4; ++failure) {
        jpeg_fixture_reset();
        memset(&bitmap, 0, sizeof bitmap);
        bitmap.width = bitmap.height = bitmap.hasAlpha = 1;
        bitmap.pixels = jpeg_test_malloc(4);
        bitmap.grayPixels = jpeg_test_malloc(4);
        bitmap.pixels[0] = 0x12345678;
        previous = bitmap;
        if (failure == 1) jpeg_fail_read = 1;
        if (failure == 2) jpeg_fail_allocation = jpeg_allocations + 1;
        if (failure == 3) jpeg_fail_allocation = jpeg_allocations + 2;
        assert(bitmap_res_load_jpg_data(&bitmap, jpeg_input, sizeof jpeg_input) == (failure == 0));
        if (failure) {
            assert(!memcmp(&bitmap, &previous, sizeof bitmap));
            assert(bitmap.pixels[0] == 0x12345678 && jpeg_live == 2);
        } else {
            assert(bitmap.width == 3 && bitmap.height == 2 && !bitmap.hasAlpha && !bitmap.grayPixels);
            assert(jpeg_live == 1 && bitmap.pixels[0] == 0x000a0a0a);
            for (y = 0; y < 2; ++y) for (x = 0; x < 3; ++x) {
                DWORD expected = 0xff000000u | (DWORD)(30 + x + y) << 16 | (DWORD)(20 + y) << 8 | (10 + x);
                if (x || y) assert(bitmap.pixels[y * 3 + x] == expected);
            }
        }
        jpeg_test_free(bitmap.pixels);
        jpeg_test_free(bitmap.grayPixels);
    }
    jpeg_fixture_reset();
    assert(!bitmap_res_load_jpg_data(NULL, jpeg_input, sizeof jpeg_input));
    assert(!bitmap_res_load_jpg_data(&bitmap, jpeg_input, -1));
}

int main(void)
{
    check_bitmap();
    jpeg_fixture_reset();
    puts("JPEG bitmap: padded BGR, color key, allocation failures and atomic replacement passed");
    return 0;
}
