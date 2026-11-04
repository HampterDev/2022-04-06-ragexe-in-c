#ifndef RESOURCES_JPEG_CODEC_H
#define RESOURCES_JPEG_CODEC_H

#include <stddef.h>
#include <stdio.h>

struct JpegImage {
    unsigned char *pixels;
    size_t stride;
    int width, height;
};

int jpeg_image_create(struct JpegImage *image, int width, int height);
int jpeg_image_read(struct JpegImage *image, const void *data, size_t size);
int jpeg_image_write(const struct JpegImage *image, FILE *file);
void jpeg_image_destroy(struct JpegImage *image);

#endif
