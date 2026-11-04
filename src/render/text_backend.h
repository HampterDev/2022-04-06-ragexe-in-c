#ifndef RENDER_TEXT_BACKEND_H
#define RENDER_TEXT_BACKEND_H

/* Text, and the one filled shape the client draws itself, on its 32-bit bitmaps (render/cdc.h) and
   surfaces (render/textdc.h). Two backends implement what follows: GDI on Windows
   (render/text_gdi.c, fonts from render/fonts.c), FreeType everywhere else and on Windows built with
   RAGE_TEXT_FREETYPE (render/text_freetype.c, fonts from render/fonts_ft.c); render/fonts.h says
   which one the build has (TEXT_BACKEND_GDI). render/cdc.c, render/textdc.c and render/ctextdc.c
   are the part both share: the bitmaps' pixels and textures, the client's font choice, the UTF-8
   text split into runs by script (render/text_runs.c) with each run's fallback font, and the extent
   cache. */

#include "platform/win_types.h"
#include "render/fonts.h"
#include <stdint.h>

struct DcBitmap;
struct TextDC;

#ifdef TEXT_BACKEND_GDI
/* GDI draws into a DIB section selected into a memory DC. */
struct TextBitmapState {
    HDC hdc;
    HBITMAP dib;
    HGDIOBJ oldBitmap;
};

struct TextDcState {
    HDC hdc;
    HGDIOBJ oldFont;
};
#else
struct TextBitmapState {
    char none;
};

/* Text goes straight into the bitmap's pixels or into the locked surface. */
struct TextDcState {
    struct DcBitmap *bitmap;
    FontHandle font;
    struct FontTarget target;
    COLORREF textColour, bkColour;
    int bkMode, locked;
};
#endif

/* Bitmaps. open and close bracket the bitmap's life; resize gives new pixels of w x h for the
   bitmap (cleared or not, as the backend's storage comes) and lets go of the old ones, or returns
   NULL and keeps them. dc_bitmap_fill_triangle (render/cdc.h) is the backend's too. */
int text_bitmap_open(struct DcBitmap *bitmap);
DWORD *text_bitmap_resize(struct DcBitmap *bitmap, int w, int h);
void text_bitmap_close(struct DcBitmap *bitmap);

/* Text DCs. text_dc_construct, text_dc_from_surface, text_dc_destruct, the colour and mode setters
   and text_dc_text_out_plain (render/textdc.h) are the backend's; so are these, for the shared
   part: use_font makes a font the one measure_run and draw_run use. An italic run measures without
   the overhang of each character's right side; a surrogate pair is one character. */
int text_dc_use_font(struct TextDC *self, FontHandle font);
int text_dc_measure_run(struct TextDC *self, const uint16_t *text, int length, SIZE *out);
void text_dc_draw_run(struct TextDC *self, int x, int y, const uint16_t *text, int length);

/* For the script fallback of render/textdc.c: whether font, which the font cache made, has a glyph
   for script (a text_runs.h script other than TEXT_SCRIPT_DEFAULT), and its ascent in pixels (0
   when unknown). GDI asks the font in this DC once and keeps the answer in the font cache. */
int text_dc_font_covers(struct TextDC *self, FontHandle font, int script);
int text_dc_font_ascent(struct TextDC *self, FontHandle font);

/* render/textdc.c, for a backend's text_dc_text_out_plain: one run of UTF-8 text in the selected
   face, without the script fallback. */
void text_dc_draw_plain(struct TextDC *self, int x, int y, const char *text, int length);

#endif
