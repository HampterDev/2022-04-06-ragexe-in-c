#ifndef RESOURCES_JPEG_INTERNAL_H
#define RESOURCES_JPEG_INTERNAL_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

static int jpeg_image_layout(int width, int height, size_t *stride, size_t *bytes)
{
    if (width <= 0 || height <= 0 || width > UINT16_MAX || height > UINT16_MAX)
        return 0;
    *stride = ((size_t)width * 3 + 3) & ~(size_t)3;
    if ((size_t)height > INT_MAX / *stride)
        return 0;
    *bytes = *stride * (size_t)height;
    return 1;
}

#endif
