#ifndef RENDER_TEXTDC_H
#define RENDER_TEXTDC_H

#include "platform/win_types.h"
#include "render/text_backend.h"
struct DcBitmap;
struct gfx_surface;

enum { TEXT_DC_TRANSPARENT = 1, TEXT_DC_OPAQUE = 2 };

struct TextDC {
    struct TextDcState state;
    struct gfx_surface *surface;
    int face, size, charset;
    BYTE bold, italic;
};

struct TextDC *text_dc_construct(struct TextDC *self, struct DcBitmap *bitmap);
struct TextDC *text_dc_from_surface(struct TextDC *self, struct gfx_surface *surface);
void text_dc_destruct(struct TextDC *self);
void text_dc_select_font(struct TextDC *self, int face, int size, int bold, int italic);
COLORREF text_dc_set_bk_color(struct TextDC *self, COLORREF colour);
int text_dc_set_bk_mode(struct TextDC *self, int mode);
COLORREF text_dc_set_text_color(struct TextDC *self, COLORREF colour);

void text_dc_text_extent(struct TextDC *self, const char *text, int len, SIZE *out);
void text_dc_text_out(struct TextDC *self, int x, int y, const char *text, int len);
void text_dc_text_out_plain(struct TextDC *self, int x, int y, const char *text, int len);
SIZE *text_dc_measure_in_charset(struct TextDC *self, SIZE *out, const char *text, int len, int charset);

#endif
