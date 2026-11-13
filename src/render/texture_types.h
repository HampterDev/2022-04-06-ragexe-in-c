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

struct TextureUpload {
    gfx_surface *surface;
    unsigned version;
    DWORD hash[4];
    int x, y, w, h, pitch;
    BYTE colorKeyed;
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
    struct TextureUpload lastUpload;
};

#endif
