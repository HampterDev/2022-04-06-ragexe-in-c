#include "resources/bitmap_internal.h"
#include "resources/jpeg_codec.h"
#include <stdint.h>

BYTE bitmap_res_load_jpg_data(struct BitmapRes *self, const BYTE *data, int size)
{
    struct JpegImage image = {0};
    DWORD *pixels;
    int x, y;

    if (!self || size <= 0 || !jpeg_image_read(&image, data, (size_t)size))
        return 0;
    if (!bitmap_pixel_count(image.width, image.height)) {
        jpeg_image_destroy(&image);
        return 0;
    }
    pixels = bitmap_allocate_pixels(image.width, image.height);
    if (!pixels) {
        jpeg_image_destroy(&image);
        return 0;
    }
    for (y = 0; y < image.height; ++y) {
        const BYTE *source = image.pixels + (size_t)y * image.stride;
        DWORD *target = pixels + (size_t)y * image.width;

        for (x = 0; x < image.width; ++x, source += 3) {
            unsigned b = source[0], g = source[1], r = source[2];
            target[x] = r >= 0xfe && g <= 3 && b >= 0xfe
                ? 0x000a0a0a : 0xff000000u | r << 16 | g << 8 | b;
        }
    }
    bitmap_replace_pixels(self, pixels, image.width, image.height, 0);
    jpeg_image_destroy(&image);
    return 1;
}
