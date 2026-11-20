#ifndef RENDER_FONTS_H
#define RENDER_FONTS_H

#include "platform/win_types.h"
#include <stddef.h>
#include <stdint.h>
#include "render/text_runs.h"

#define FONT_FACE_EOT_FIRST 0x14
#define FONT_FACE_SCDREAM   0x1d
#define FONT_FACE_FALLBACK  (-2)

/* The fonts the client's text is drawn in, with FreeType (render/fonts_ft.c): for each request the
   face GDI chose among the installed fonts (render/font_match.h), at GDI's size, hinted by its own
   instructions; on other platforms the free fonts that stand in for the Windows ones. Text DCs
   (render/textdc.h) draw with them. */
typedef struct Font *FontHandle;

struct FontCache {
    struct Font **fonts;
    size_t count, capacity;
};

struct FontTarget {
    BYTE *bits;
    int width, height, pitch;
};

extern struct FontCache g_fontCache;

void fonts_init(void);
void fonts_shutdown(void);
/* Face's font (an index into render/device.h's face tables) in a charset, at size: GDI's CreateFont
   height, positive for the cell and negative for the em. The same font for the same request until
   fonts_shutdown; NULL when no installed font answers. */
FontHandle font_cache_get_font(struct FontCache *self, int face, int size, int charset,
                       int bold, int italic);
/* The first installed font for script, a text_runs.h script other than TEXT_SCRIPT_DEFAULT, in
   the script's fallback order (Gulim, then Malgun Gothic for Hangul; render/fonts_ft.c lists the
   Windows fonts followed by the free fonts that stand in for them), or NULL when none is
   installed. */
FontHandle font_cache_get_script_font(struct FontCache *self, int script, int size, int bold, int italic);
/* A fixed-pitch font: family when it is installed, otherwise the fixed-pitch font GDI's font mapper
   gives such a request (Courier New) or a free font that stands in for it. */
FontHandle font_cache_get_fixed(struct FontCache *self, const char *family, int size);
/* Whether the font's own face has script's sample character, and its ascent in pixels. */
int font_covers(FontHandle font, int script);
int font_ascent(FontHandle font);
void font_text_extent(FontHandle font, const uint16_t *text, int length, int black_box, SIZE *out);
void font_text_out(FontHandle font, const struct FontTarget *target, int x, int y, const uint16_t *text, int length,
                   COLORREF colour, const COLORREF *background);

#endif
