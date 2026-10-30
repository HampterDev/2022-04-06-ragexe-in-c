#include "resources/jpeg_codec.h"
#include "platform/ijl_api.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int jpeg_image_layout(int width, int height, size_t *stride, size_t *bytes)
{
    if (width <= 0 || height <= 0 || width > (INT_MAX - 3) / 3)
        return 0;
    *stride = ((size_t)width * 3 + 3) & ~(size_t)3;
    if ((size_t)height > SIZE_MAX / *stride)
        return 0;
    *bytes = *stride * (size_t)height;
    return 1;
}

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

static void jpeg_set_pixels(struct IjlProps *props, const struct JpegImage *image)
{
    props->dibBytes = image->pixels;
    props->dibWidth = image->width;
    props->dibHeight = image->height;
    props->dibPadBytes = (int)(image->stride - (size_t)image->width * 3);
    props->dibChannels = 3;
    props->dibColor = IJL_BGR;
}

int jpeg_image_read(struct JpegImage *image, const void *data, size_t size)
{
    struct IjlProps props = {0};
    struct JpegImage replacement = {0};
    int ok = 0;

    if (!image || !data || !size || size > INT_MAX || ijlInit(&props) != 0)
        return 0;
    props.jpgBytes = data;
    props.jpgSizeBytes = (int)size;
    if (ijlRead(&props, IJL_JBUFF_READPARAMS) == 0 &&
        jpeg_image_create(&replacement, props.jpgWidth, props.jpgHeight)) {
        jpeg_set_pixels(&props, &replacement);
        ok = ijlRead(&props, IJL_JBUFF_READWHOLEIMAGE) == 0;
    }
    ijlFree(&props);
    if (ok) {
        jpeg_image_destroy(image);
        *image = replacement;
    } else {
        jpeg_image_destroy(&replacement);
    }
    return ok;
}

int jpeg_image_write(const struct JpegImage *image, const char *filename)
{
    struct IjlProps props = {0};
    size_t stride, bytes;
    int ok;

    if (!image || !image->pixels || !filename || !*filename ||
        !jpeg_image_layout(image->width, image->height, &stride, &bytes) ||
        image->stride != stride || ijlInit(&props) != 0)
        return 0;
    jpeg_set_pixels(&props, image);
    props.jpgFile = filename;
    props.jpgWidth = image->width;
    props.jpgHeight = image->height;
    ok = ijlWrite(&props, IJL_JFILE_WRITEWHOLEIMAGE) == 0;
    ijlFree(&props);
    return ok;
}
