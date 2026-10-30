#ifndef RESOURCES_BITMAP_TYPES_H
#define RESOURCES_BITMAP_TYPES_H

#include "fileio/resource_types.h"

struct BitmapRes {
    struct Res res;
    int hasAlpha;
    int width, height;
    DWORD *pixels;
    DWORD *grayPixels;
};
OFFSET_ASSERT(BitmapRes, hasAlpha, 0x110);
OFFSET_ASSERT(BitmapRes, width, 0x114);
OFFSET_ASSERT(BitmapRes, pixels, 0x11c);
SIZE_ASSERT(BitmapRes, 0x124);

struct GifFrame {
    unsigned short delay;
    DWORD *pixels;
};
SIZE_ASSERT(GifFrame, 8);

struct GifFrames {
    struct GifFrame *data;
    size_t count;
    size_t capacity;
};

struct GifRes {
    struct BitmapRes bmp;
    struct GifFrames frames;
    unsigned short loopCount;
    unsigned short curFrame;
    DWORD lastTime;
    BYTE animated;
    BYTE changed;
};
OFFSET_ASSERT(GifRes, frames, 0x124);

#endif
