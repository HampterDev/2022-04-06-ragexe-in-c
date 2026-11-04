#ifndef RENDER_FONTS_H
#define RENDER_FONTS_H

#include "platform/win_types.h"
#include <stddef.h>
#include <stdint.h>
#include "render/text_runs.h"

#define FONT_FACE_EOT_FIRST 0x14
#define FONT_FACE_SCDREAM   0x1d
#define FONT_FACE_FALLBACK  (-2)

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
    signed char fallback[TEXT_SCRIPT_COUNT];
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
/* The first installed font for script, a text_runs.h script other than TEXT_SCRIPT_DEFAULT, in
   the script's fallback order (Gulim, then Malgun Gothic for Hangul; render/fonts.c lists the
   Windows fonts, render/fonts_ft.c the same followed by the free fonts that stand in for them), or
   NULL when none is installed. */
FontHandle font_cache_get_script_font(struct FontCache *self, int script, int size, int bold, int italic);
#ifdef TEXT_BACKEND_GDI
/* Whether font, which this cache created, has a glyph for script; selects it into dc once
   to find out. Fonts the cache does not know count as covering. */
int font_cache_covers(struct FontCache *self, HDC dc, HFONT font, int script);
/* The ascent of font, which this cache created, in pixels; 0 when unknown. */
int font_cache_ascent(struct FontCache *self, HDC dc, HFONT font);
#else
FontHandle font_cache_get_family(struct FontCache *self, const char *family, int size);
/* Whether the font's own face has script's sample character, and its ascent in pixels. */
int font_covers(FontHandle font, int script);
int font_ascent(FontHandle font);
void font_text_extent(FontHandle font, const uint16_t *text, int length, int black_box, SIZE *out);
void font_text_out(FontHandle font, const struct FontTarget *target, int x, int y, const uint16_t *text, int length,
                   COLORREF colour, const COLORREF *background);
#endif

#endif
