#ifndef RESOURCES_BITMAP_INTERNAL_H
#define RESOURCES_BITMAP_INTERNAL_H
#include "resources/bitmap.h"

#define BITMAP_MAX_BYTES (256u * 1024u * 1024u)
size_t bitmap_pixel_count(int width, int height);
DWORD *bitmap_allocate_pixels(int width, int height);
void bitmap_replace_pixels(struct BitmapRes *self, DWORD *pixels,
                           int width, int height, int has_alpha);
void gif_frames_destroy(struct GifFrames *frames);
BYTE gif_frames_append(struct GifFrames *frames, struct GifFrame frame);
#endif
