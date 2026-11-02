#ifndef RENDER_FONTS_H
#define RENDER_FONTS_H

#include "platform/win_types.h"
#include <stddef.h>
#include <stdint.h>

#define FONT_FACE_EOT_FIRST 0x14
#define FONT_FACE_SCDREAM   0x1d

struct FontEntry;

struct FontCache {
    struct FontEntry *entries;
    size_t count, capacity;
    uintptr_t token;
};

extern struct FontCache g_fontCache;

struct FontCache *font_cache_construct(struct FontCache *self);
void font_cache_destruct(struct FontCache *self);
void gdiplus_init(void);
void gdiplus_destroy(void);
HFONT font_cache_get_font(struct FontCache *self, int face, int size, int charset,
                       int bold, int italic);

#endif
