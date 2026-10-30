#ifndef RESOURCES_SPRITE_H
#define RESOURCES_SPRITE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "base.h"
#include "fileio/resource_types.h"

struct Texture;

struct SprImg {
    short width, height;
    short widthHalved, heightHalved;
    struct Texture *texture;
    BYTE *pixels;
};
OFFSET_ASSERT(SprImg, texture, 0x08);
SIZE_ASSERT(SprImg, 0x10);

#define SPR_PALETTE_IMAGES 0

struct SpriteImageList {
    struct SprImg *items;
    size_t count;
};

struct SprRes {
    struct Res res;
    DWORD palette[256];
    struct SpriteImageList images[2];
};
OFFSET_ASSERT(SprRes, palette, 0x110);
OFFSET_ASSERT(SprRes, images, 0x510);
SIZE_ASSERT(SprRes, 0x520);

static inline struct SprImg *sprite_resource_image(const struct SprRes *sprite, int kind, int index)
{
    struct SprImg *image;
    if (!sprite || (unsigned)kind >= 2 || index < 0 || (size_t)index >= sprite->images[kind].count
            || !sprite->images[kind].items)
        return NULL;
    image = &sprite->images[kind].items[index];
    return image->width > 0 && image->height > 0 ? image : NULL;
}

void sprite_resource_reset(struct SprRes *sprite);

#endif
