#ifndef RESOURCES_PALETTE_H
#define RESOURCES_PALETTE_H

#include "fileio/resource_types.h"

struct PaletteRes {
    struct Res res;
    BYTE raw[256 * 4];
    DWORD palette[256];
};
OFFSET_ASSERT(PaletteRes, raw, 0x110);
OFFSET_ASSERT(PaletteRes, palette, 0x510);
SIZE_ASSERT(PaletteRes, 0x910);

static inline DWORD *palette_resource_colors(struct PaletteRes *palette, DWORD *fallback)
{
    return palette ? palette->palette : fallback;
}

#endif
