#ifndef RENDER_FONTS_H
#define RENDER_FONTS_H

#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include "render/text_runs.h"

#define FONT_FACE_EOT_FIRST 0x14
#define FONT_FACE_SCDREAM   0x1d
#define FONT_FACE_FALLBACK  (-2)

struct FontEntry;

struct FontCache {
    struct FontEntry *entries;
    size_t count, capacity;
    uintptr_t token;
    signed char fallback[TEXT_SCRIPT_COUNT];
};

extern struct FontCache g_fontCache;

struct FontCache *font_cache_construct(struct FontCache *self);
void font_cache_destruct(struct FontCache *self);
void gdiplus_init(void);
void gdiplus_destroy(void);
HFONT font_cache_get_font(struct FontCache *self, int face, int size, int charset,
                       int bold, int italic);
/* The first installed Windows font for script (Gulim, then Malgun Gothic for Hangul), or
   NULL when none is installed. */
HFONT font_cache_get_script_font(struct FontCache *self, int script, int size, int bold, int italic);
/* Whether font, which this cache created, has a glyph for script; selects it into dc once
   to find out. Fonts the cache does not know count as covering. */
int font_cache_covers(struct FontCache *self, HDC dc, HFONT font, int script);
/* The ascent of font, which this cache created, in pixels; 0 when unknown. */
int font_cache_ascent(struct FontCache *self, HDC dc, HFONT font);

#endif
