#include "resources/jpeg_codec.h"
#include "resources/jpeg_internal.h"
#include <stdlib.h>
#include <string.h>

void jpeg_image_destroy(struct JpegImage *image)
{
    if (image) {
        free(image->pixels);
        memset(image, 0, sizeof *image);
    }
}

int jpeg_image_create(struct JpegImage *image, int width, int height)
{
    struct JpegImage replacement = {0};
    size_t bytes;
    if (!image || !jpeg_image_layout(width, height, &replacement.stride, &bytes))
        return 0;
    replacement.pixels = calloc(1, bytes);
    if (!replacement.pixels)
        return 0;
    replacement.width = width;
    replacement.height = height;
    jpeg_image_destroy(image);
    *image = replacement;
    return 1;
}
