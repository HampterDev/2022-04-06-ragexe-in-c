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

struct GifFrame {
    unsigned short delay;
    DWORD *pixels;
};

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

#endif
