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

/* custom: what a texture's last upload put into its surface (texture_upload_pixels), so that one of
   the same pixels into the same surface, unchanged since, is skipped; surface NULL before any */
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
    unsigned serial;
    struct TextureUpload lastUpload;
};

#endif
