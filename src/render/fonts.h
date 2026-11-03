#ifndef RENDER_FONTS_H
#define RENDER_FONTS_H

#include "platform/win_types.h"
#include <stddef.h>
#include <stdint.h>

#define FONT_FACE_EOT_FIRST 0x14
#define FONT_FACE_SCDREAM   0x1d

/* The text backend (render/text_backend.h): GDI on Windows unless the build asks for FreeType. */
#if defined(_WIN32) && !defined(RAGE_TEXT_FREETYPE)
#define TEXT_BACKEND_GDI 1
#endif

#ifdef TEXT_BACKEND_GDI
typedef HFONT FontHandle;

struct FontEntry;

struct FontCache {
    struct FontEntry *entries;
    size_t count, capacity;
    uintptr_t token;
};
#else
typedef struct Font *FontHandle;

struct FontCache {
    struct Font **fonts;
    size_t count, capacity;
};

struct FontTarget {
    BYTE *bits;
    int width, height, pitch;
};
#endif

extern struct FontCache g_fontCache;

struct FontCache *font_cache_construct(struct FontCache *self);
void font_cache_destruct(struct FontCache *self);
void fonts_init(void);
void fonts_shutdown(void);
FontHandle font_cache_get_font(struct FontCache *self, int face, int size, int charset,
                       int bold, int italic);
#ifndef TEXT_BACKEND_GDI
FontHandle font_cache_get_family(struct FontCache *self, const char *family, int size);
void font_text_extent(FontHandle font, const uint16_t *text, int length, int black_box, SIZE *out);
void font_text_out(FontHandle font, const struct FontTarget *target, int x, int y, const uint16_t *text, int length,
                   COLORREF colour, const COLORREF *background);
#endif

#endif
