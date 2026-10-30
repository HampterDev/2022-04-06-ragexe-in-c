#ifndef RENDER_TEXTURE_TYPES_H
#define RENDER_TEXTURE_TYPES_H

#include "render/gfx.h"
#include "base.h"

struct SurfaceOps;
struct Surface {
    const struct SurfaceOps *ops;
    int width, height;
    gfx_surface *surface;
};

struct Texture {
    struct Surface base;
    int format;
    BYTE colorKeyed;
    int reqWidth, reqHeight;
    char name[0x100];
    int lockCount;
    DWORD lastUse;
    BYTE fullSize;
};

#endif
