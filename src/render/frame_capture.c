#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "render/frame_capture.h"
#include "render/render_api.h"
#include "resources/jpeg_codec.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* The screen's pixels as 24-bit rows of the JPEG image: B8G8R8A8 in, its alpha dropped. */
static void copy_pixels(struct JpegImage *image, const unsigned char *frame, int width, int height)
{
    int x, y;
    for (y = 0; y < height; ++y) {
        const unsigned char *src = frame + (size_t)y * (size_t)width * 4;
        unsigned char *dst = image->pixels + (size_t)y * image->stride;
        for (x = 0; x < width; ++x, src += 4, dst += 3) {
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
        }
    }
}

/* custom: the frame just presented, the whole screen downloaded from the GPU (gfx_read_frame),
   where the original locked the back buffer. */
char renderer_screenshot(struct Renderer *self, struct String filename)
{
    struct JpegImage image = {0};
    unsigned char *frame = NULL;
    int width = 0, height = 0;
    char ok = 0;
    (void)self;

    if (g_3dDevice.target && gfx_get_size(g_3dDevice.target, &width, &height) >= 0 && width > 0 && height > 0
        && (size_t)width <= SIZE_MAX / 4 / (size_t)height
        && (frame = malloc((size_t)width * (size_t)height * 4)) != NULL) {
        RECT all;
        all.left = 0;
        all.top = 0;
        all.right = width;
        all.bottom = height;
        if (gfx_read_frame(&all, frame, (size_t)width * 4) >= 0 && jpeg_image_create(&image, width, height)) {
            copy_pixels(&image, frame, width, height);
            ok = 1;
        }
    }
    free(frame);
    if (ok) {
        FILE *file = fopen(string_cstr(&filename), "wb");
        ok = file && jpeg_image_write(&image, file);
        if (file && fclose(file))
            ok = 0;
    }
    jpeg_image_destroy(&image);
    string_free(&filename);
    return ok;
}
