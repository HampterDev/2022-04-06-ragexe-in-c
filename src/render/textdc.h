#ifndef RENDER_TEXTDC_H
#define RENDER_TEXTDC_H

#include "platform/win_types.h"
#include "render/fonts.h"
struct DcBitmap;

enum { TEXT_DC_TRANSPARENT = 1, TEXT_DC_OPAQUE = 2 };

/* Text drawn into a bitmap's pixels (render/cdc.h) in the fonts of render/fonts_ft.c: the selected
   face, and for a run of a script it lacks (render/text_runs.c) the script's fallback font, on the
   face's baseline. Text is UTF-8. */
struct TextDC {
    struct DcBitmap *bitmap;
    COLORREF textColour, bkColour;
    int bkMode;
    int face, size, charset;
    BYTE bold, italic;
};

struct TextDC *text_dc_construct(struct TextDC *self, struct DcBitmap *bitmap);
void text_dc_destruct(struct TextDC *self);
void text_dc_select_font(struct TextDC *self, int face, int size, int bold, int italic);
COLORREF text_dc_set_bk_color(struct TextDC *self, COLORREF colour);
int text_dc_set_bk_mode(struct TextDC *self, int mode);
COLORREF text_dc_set_text_color(struct TextDC *self, COLORREF colour);

void text_dc_text_extent(struct TextDC *self, const char *text, int len, SIZE *out);
void text_dc_text_out(struct TextDC *self, int x, int y, const char *text, int len);
/* Text in the selected face alone, without the script fallback. The original drew this text
   (the login screen's serial number) in a new DC's default font, GDI's System font. */
void text_dc_text_out_plain(struct TextDC *self, int x, int y, const char *text, int len);
void text_dc_text_extent_plain(struct TextDC *self, const char *text, int len, SIZE *out);
/* Measures UTF-8 text in font, caching the extent (render/ctextdc.c). An italic DC measures each
   character to the end of its black box, as GDI measured italic text. */
SIZE *text_dc_measure_font(struct TextDC *self, SIZE *out, const char *text, int len, FontHandle font);

#endif
