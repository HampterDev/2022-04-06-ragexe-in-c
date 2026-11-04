#include "resources/jpeg_codec.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void check_pixel(const struct JpegImage *image, int x, int y, int r, int g, int b, int tolerance)
{
    const unsigned char *p = image->pixels + (size_t)y * image->stride + x * 3;
    assert(abs((int)p[0] - b) <= tolerance);
    assert(abs((int)p[1] - g) <= tolerance);
    assert(abs((int)p[2] - r) <= tolerance);
}

static void check_quadrants(const struct JpegImage *image, int tolerance)
{
    assert(image->width == 17 && image->height == 19 && image->stride == 52);
    check_pixel(image, 1, 1, 240, 10, 20, tolerance);
    check_pixel(image, 15, 1, 10, 220, 30, tolerance);
    check_pixel(image, 1, 17, 15, 25, 230, tolerance);
    check_pixel(image, 15, 17, 245, 240, 230, tolerance);
    for (int y = 0; y < image->height; ++y)
        assert(image->pixels[(size_t)y * image->stride + 51] == 0);
}

static unsigned char *read_stream(FILE *file, size_t *size)
{
    long length;
    unsigned char *bytes;
    assert(!fseek(file, 0, SEEK_END));
    length = ftell(file);
    assert(length > 0 && length < 1024 * 1024);
    assert(!fseek(file, 0, SEEK_SET));
    *size = (size_t)length;
    bytes = malloc(*size);
    assert(bytes && fread(bytes, 1, *size, file) == *size);
    return bytes;
}

int main(int argc, char **argv)
{
    struct JpegImage image = {0}, original, decoded = {0};
    unsigned char *bytes, *encoded;
    size_t size, encoded_size;
    FILE *file;
    int width;
    assert(argc == 2);
    for (width = 1; width <= 17; ++width) {
        assert(jpeg_image_create(&image, width, 3));
        assert(image.stride == ((size_t)width * 3 + 3) / 4 * 4);
        for (size_t i = 0; i < image.stride * 3; ++i) assert(!image.pixels[i]);
    }
    original = image;
    assert(!jpeg_image_create(&image, INT_MAX, 1));
    assert(!jpeg_image_create(&image, 1, INT_MAX));
    assert(!jpeg_image_create(&image, 65536, 1));
    assert(!jpeg_image_create(&image, 1, 65536));
    assert(!jpeg_image_create(&image, 0, 1));
    assert(!jpeg_image_create(NULL, 1, 1));
    assert(!memcmp(&image, &original, sizeof image));
    assert(!jpeg_image_write(&image, NULL));
    assert(!jpeg_image_read(&image, NULL, 1));
    assert(!jpeg_image_read(&image, "broken", 6));
    assert(!jpeg_image_read(&image, "broken", SIZE_MAX));
    assert(!memcmp(&image, &original, sizeof image));

    file = fopen(argv[1], "rb");
    assert(file);
    bytes = read_stream(file, &size);
    assert(jpeg_image_read(&image, bytes, size));
    check_quadrants(&image, 3);
    original = image;
    for (size_t i = 0; i < 128; ++i) {
        assert(!jpeg_image_read(&image, bytes, i));
        assert(!memcmp(&image, &original, sizeof image));
    }
    for (size_t i = 128; i < size; ++i) {
        struct JpegImage partial = {0};
        jpeg_image_read(&partial, bytes, i);
        jpeg_image_destroy(&partial);
    }
    assert(!jpeg_image_write(&image, file));
    assert(!fclose(file));
    free(bytes);

    file = tmpfile();
    assert(file);
    {
        struct JpegImage invalid = image;
        invalid.width = 65536;
        invalid.stride = 196608;
        assert(!jpeg_image_write(&invalid, file) && ftell(file) == 0);
        invalid = image;
        invalid.height = 65536;
        assert(!jpeg_image_write(&invalid, file) && ftell(file) == 0);
        invalid = image;
        ++invalid.stride;
        assert(!jpeg_image_write(&invalid, file) && ftell(file) == 0);
    }
    assert(jpeg_image_write(&image, file));
    assert(!fflush(file));
    encoded = read_stream(file, &encoded_size);
    assert(jpeg_image_read(&decoded, encoded, encoded_size));
    check_quadrants(&decoded, 25);
    free(encoded);
    assert(!fclose(file));
    jpeg_image_destroy(&decoded);
    jpeg_image_destroy(&image);
    jpeg_image_destroy(&image);
    jpeg_image_destroy(NULL);
    puts("JPEG: independent fixture, padded BGR, orientation, round-trip, truncated input and write failures passed");
    return 0;
}
