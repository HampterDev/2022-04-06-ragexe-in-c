#include "jpeg_fixture.h"
#include "resources/bitmap.h"
static void *jpeg_test_malloc(size_t bytes) { return jpeg_test_calloc(1, bytes); }
#define malloc jpeg_test_malloc
#define free jpeg_test_free
#include "../src/resources/bitmap_storage.c"
#include "../src/resources/bitmap_jpeg.c"
#undef malloc
#undef free

static void check_create(void)
{
    struct JpegImage image = {0}, previous;
    int width;
    for (width = 1; width <= 17; ++width) {
        size_t i;
        jpeg_fixture_reset();
        assert(jpeg_image_create(&image, width, 3));
        assert(image.width == width && image.height == 3);
        assert(image.stride == ((size_t)width * 3 + 3) / 4 * 4);
        for (i = 0; i < image.stride * 3; ++i) assert(image.pixels[i] == 0);
        previous = image;
        jpeg_fail_allocation = jpeg_allocations + 1;
        assert(!jpeg_image_create(&image, width + 1, 2));
        assert(!memcmp(&image, &previous, sizeof image));
        jpeg_image_destroy(&image);
        jpeg_image_destroy(&image);
    }
    jpeg_fixture_reset();
    assert(!jpeg_image_create(&image, 0, 1));
    assert(!jpeg_image_create(&image, -1, 1));
    assert(!jpeg_image_create(&image, 1, 0));
    assert(!jpeg_image_create(&image, INT_MAX, 1));
    assert(!jpeg_image_create(&image, 1, INT_MAX));
    assert(!jpeg_image_create(NULL, 1, 1));
    assert(jpeg_allocations == 0);
    jpeg_image_destroy(NULL);
}

static void check_read(void)
{
    struct JpegImage image = {0}, previous;
    int width, failure;
    for (width = 1; width <= 17; ++width) {
        jpeg_fixture_reset();
        jpeg_width = width;
        assert(jpeg_image_read(&image, jpeg_input, sizeof jpeg_input));
        assert(image.width == width && image.height == 2);
        assert(image.pixels[0] == 254 && image.pixels[1] == 3 && image.pixels[2] == 255);
        assert(image.pixels[image.stride + 1] == 21);
        assert(jpeg_inits == 1 && jpeg_frees == 1 && jpeg_live == 1);
        assert(jpeg_image_read(&image, jpeg_input, sizeof jpeg_input));
        assert(jpeg_live == 1 && jpeg_frees == 2);
        jpeg_image_destroy(&image);
    }
    for (failure = 0; failure < 6; ++failure) {
        jpeg_fixture_reset();
        assert(jpeg_image_create(&image, 2, 1));
        image.pixels[0] = 77;
        previous = image;
        if (failure == 0) jpeg_fail_init = 1;
        if (failure == 1) jpeg_fail_header = 1;
        if (failure == 2) jpeg_fail_read = 1;
        if (failure == 3) jpeg_fail_allocation = jpeg_allocations + 1;
        if (failure == 4) jpeg_width = 0;
        if (failure == 5) jpeg_height = INT_MAX;
        assert(!jpeg_image_read(&image, jpeg_input, sizeof jpeg_input));
        assert(!memcmp(&image, &previous, sizeof image) && image.pixels[0] == 77);
        assert(jpeg_live == 1 && jpeg_frees == (failure != 0));
        jpeg_image_destroy(&image);
    }
    jpeg_fixture_reset();
    assert(!jpeg_image_read(&image, NULL, 1));
    assert(!jpeg_image_read(&image, jpeg_input, 0));
    assert(!jpeg_image_read(&image, jpeg_input, (size_t)INT_MAX + 1));
    assert(!jpeg_image_read(NULL, jpeg_input, sizeof jpeg_input));
    assert(jpeg_inits == 0);
}

static void check_write(void)
{
    struct JpegImage image = {0}, previous;
    int failure;
    for (failure = 0; failure < 3; ++failure) {
        jpeg_fixture_reset();
        assert(jpeg_image_create(&image, 5, 3));
        memset(image.pixels, 0x36, image.stride * 3);
        previous = image;
        jpeg_fail_init = failure == 1;
        jpeg_fail_write = failure == 2;
        assert(jpeg_image_write(&image, "test.jpg") == (failure == 0));
        assert(!memcmp(&image, &previous, sizeof image));
        assert(jpeg_live == 1 && jpeg_frees == (failure != 1));
        if (failure != 1) {
            assert(jpeg_written_width == 5 && jpeg_written_height == 3 && jpeg_written_stride == 16);
            assert(!memcmp(jpeg_written, image.pixels, image.stride * 3));
        }
        jpeg_image_destroy(&image);
    }
    jpeg_fixture_reset();
    assert(!jpeg_image_write(&image, "test.jpg"));
    assert(jpeg_image_create(&image, 1, 1));
    assert(!jpeg_image_write(&image, ""));
    assert(!jpeg_image_write(&image, NULL));
    ++image.stride;
    assert(!jpeg_image_write(&image, "test.jpg"));
    assert(jpeg_inits == 0);
    jpeg_image_destroy(&image);
}

static void check_bitmap(void)
{
    struct BitmapRes bitmap, previous;
    int failure, x, y;
    for (failure = 0; failure < 6; ++failure) {
        jpeg_fixture_reset();
        memset(&bitmap, 0, sizeof bitmap);
        bitmap.width = bitmap.height = bitmap.hasAlpha = 1;
        bitmap.pixels = jpeg_test_malloc(4);
        bitmap.grayPixels = jpeg_test_malloc(4);
        bitmap.pixels[0] = 0x12345678;
        previous = bitmap;
        if (failure == 1) jpeg_fail_init = 1;
        if (failure == 2) jpeg_fail_header = 1;
        if (failure == 3) jpeg_fail_read = 1;
        if (failure == 4) jpeg_fail_allocation = jpeg_allocations + 1;
        if (failure == 5) jpeg_fail_allocation = jpeg_allocations + 2;
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
    check_create();
    check_read();
    check_write();
    check_bitmap();
    jpeg_fixture_reset();
    puts("JPEG ownership: padded widths, atomic replacement, DLL failures, allocation failures, bitmap color key and repeat cleanup passed");
    return 0;
}
