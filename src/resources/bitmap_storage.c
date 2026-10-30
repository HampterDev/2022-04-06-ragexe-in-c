#include "resources/bitmap_internal.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

size_t bitmap_pixel_count(int width, int height)
{
    if (width <= 0 || height <= 0 ||
        (size_t)width > BITMAP_MAX_BYTES / sizeof(DWORD) / (size_t)height)
        return 0;
    return (size_t)width * height;
}

DWORD *bitmap_allocate_pixels(int width, int height)
{
    size_t count = bitmap_pixel_count(width, height);
    return count ? malloc(count * sizeof(DWORD)) : NULL;
}

void bitmap_res_free_gray_copy(struct BitmapRes *self)
{
    free(self->grayPixels);
    self->grayPixels = NULL;
}

void bitmap_resource_clear(struct BitmapRes *self)
{
    free(self->pixels);
    bitmap_res_free_gray_copy(self);
    self->pixels = NULL;
    self->width = self->height = self->hasAlpha = 0;
}

void bitmap_replace_pixels(struct BitmapRes *self, DWORD *pixels,
                           int width, int height, int has_alpha)
{
    bitmap_resource_clear(self);
    self->pixels = pixels;
    self->width = width;
    self->height = height;
    self->hasAlpha = has_alpha;
}

void gif_frames_destroy(struct GifFrames *frames)
{
    size_t i;
    for (i = 0; i < frames->count; ++i)
        free(frames->data[i].pixels);
    free(frames->data);
    memset(frames, 0, sizeof *frames);
}

BYTE gif_frames_append(struct GifFrames *frames, struct GifFrame frame)
{
    if (frames->count == USHRT_MAX)
        return 0;
    if (frames->count == frames->capacity) {
        size_t capacity = frames->capacity ? frames->capacity * 2 : 4;
        struct GifFrame *data;
        if (capacity > USHRT_MAX)
            capacity = USHRT_MAX;
        data = realloc(frames->data, capacity * sizeof *data);
        if (!data)
            return 0;
        frames->data = data;
        frames->capacity = capacity;
    }
    frames->data[frames->count++] = frame;
    return 1;
}

void gif_resource_clear(struct GifRes *self)
{
    size_t i;
    for (i = 0; i < self->frames.count; ++i)
        if (self->bmp.pixels == self->frames.data[i].pixels)
            self->bmp.pixels = NULL;
    gif_frames_destroy(&self->frames);
    bitmap_resource_clear(&self->bmp);
    self->loopCount = self->curFrame = 0;
    self->lastTime = 0;
    self->animated = self->changed = 0;
}

BYTE gif_res_add_blank_frame(struct GifRes *self, int width, int height)
{
    struct GifFrames frames = {0};
    struct GifFrame frame = {0};
    frame.pixels = bitmap_allocate_pixels(width, height);
    if (!frame.pixels)
        return 0;
    if (!gif_frames_append(&frames, frame)) {
        free(frame.pixels);
        return 0;
    }
    memset(frame.pixels, 0xff, bitmap_pixel_count(width, height) * sizeof *frame.pixels);
    gif_resource_clear(self);
    self->frames = frames;
    self->bmp.pixels = frame.pixels;
    self->bmp.width = width;
    self->bmp.height = height;
    return 1;
}

BYTE bitmap_res_make_gray_copy(struct BitmapRes *self, int mode)
{
    size_t i, count = bitmap_pixel_count(self->width, self->height);
    if (!self->pixels || self->grayPixels || !count)
        return 0;
    self->grayPixels = bitmap_allocate_pixels(self->width, self->height);
    if (!self->grayPixels)
        return 0;
    for (i = 0; i < count; ++i) {
        DWORD p = self->pixels[i];
        unsigned r = (p >> 16) & 255, g = (p >> 8) & 255, b = p & 255;
        if ((p & 0xffffff) != 0xff00ff) {
            unsigned floor = mode ? 100 : 120, v = (r + g + b) / 3;
            r = g = b = v < floor ? floor : v;
            if (mode == 1) r = 200;
            else if (mode == 2) g = 200;
            else if (mode == 3) b = 200;
        }
        self->grayPixels[i] = (p & 0xff000000) | r << 16 | g << 8 | b;
    }
    return 1;
}

DWORD bitmap_res_get_pixel(const struct BitmapRes *self, int x, int y)
{
    if (!self || !self->pixels || x < 0 || y < 0 || x >= self->width || y >= self->height)
        return 0xff0000;
    return self->pixels[(size_t)y * self->width + x];
}
