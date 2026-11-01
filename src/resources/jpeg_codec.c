#include "resources/jpeg_codec.h"
#include "resources/jpeg_internal.h"
#include "../../vendor/stb/stb_image.h"
#include "../../vendor/stb/stb_image_write.h"
#include <limits.h>
#include <stdlib.h>

static void copy_swapped_channels(unsigned char *out, size_t out_stride,
                                 const unsigned char *in, size_t in_stride,
                                 int width, int height)
{
    int x, y;
    for (y = 0; y < height; ++y) {
        const unsigned char *source = in + (size_t)y * in_stride;
        unsigned char *target = out + (size_t)y * out_stride;
        for (x = 0; x < width; ++x) {
            target[x * 3] = source[x * 3 + 2];
            target[x * 3 + 1] = source[x * 3 + 1];
            target[x * 3 + 2] = source[x * 3];
        }
    }
}

int jpeg_image_read(struct JpegImage *image, const void *data, size_t size)
{
    struct JpegImage replacement = {0};
    unsigned char *rgb;
    int width, height, channels;
    size_t stride, bytes;
    if (!image || !data || !size || size > INT_MAX ||
        !stbi_info_from_memory(data, (int)size, &width, &height, &channels) ||
        !jpeg_image_layout(width, height, &stride, &bytes))
        return 0;
    rgb = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 3);
    if (!rgb)
        return 0;
    if (!jpeg_image_create(&replacement, width, height)) {
        stbi_image_free(rgb);
        return 0;
    }
    copy_swapped_channels(replacement.pixels, replacement.stride, rgb, (size_t)width * 3, width, height);
    stbi_image_free(rgb);
    jpeg_image_destroy(image);
    *image = replacement;
    return 1;
}

struct JpegOutput {
    FILE *file;
    int failed;
};

static void jpeg_write_bytes(void *context, void *data, int size)
{
    struct JpegOutput *output = context;
    if (!output->failed && fwrite(data, 1, (size_t)size, output->file) != (size_t)size)
        output->failed = 1;
}

int jpeg_image_write(const struct JpegImage *image, FILE *file)
{
    struct JpegOutput output = {file, 0};
    size_t stride, bytes;
    unsigned char *rgb;
    int ok;
    if (!image || !image->pixels || !file ||
        !jpeg_image_layout(image->width, image->height, &stride, &bytes) ||
        image->stride != stride)
        return 0;
    rgb = malloc((size_t)image->width * image->height * 3);
    if (!rgb)
        return 0;
    copy_swapped_channels(rgb, (size_t)image->width * 3, image->pixels, image->stride,
                          image->width, image->height);
    ok = stbi_write_jpg_to_func(jpeg_write_bytes, &output, image->width, image->height, 3, rgb, 90);
    free(rgb);
    return ok && !output.failed && !ferror(file);
}
