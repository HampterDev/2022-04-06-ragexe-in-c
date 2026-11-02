#include "std/string.h"
#include "core/text_array.h"
#include "core/byte_array.h"
#include "resources/sprite.h"
#include "resources/animation.h"
#include "ui/window_paint.h"
#ifndef UI_DRAW_H
#define UI_DRAW_H

#include "render/math3d.h"

#include "ui/window.h"

struct BitmapRes;

#include "render/cdc.h"

#define UI_TRANSPARENT 0xffff00ff

void ui_window_draw_outline(struct UIWindow *self, int x, int y, int width, int height,
                           COLORREF colour);
void ui_window_fill_triangle(struct UIWindow *self, int x1, int y1, int x2, int y2,
                                      int x3, int y3, COLORREF colour);

void ui_window_draw_actor_sprite(struct UIWindow *self, void *spr, void *act, int x,
                                         int y, int width, int height, int period, char scale_down, void *backdrop);
static __inline void ui_canvas_draw_texture(struct DcBitmap *canvas, int x, int y, struct Texture *tex, int sx, int sy, int sw,
                                          int sh, int mirror, int cw, int ch)
{
    if (!canvas || !tex)
        return;
    dc_bitmap_draw_texels(canvas, x, y, tex, sx, sy, sw, sh, mirror, cw, ch);
}

void ui_window_reset_draws(struct UIWindow *self, int width, int height,
                                    DWORD argb);
void ui_window_queue_bitmap(struct UIWindow *self, int x, int y, struct String name,
                                     DWORD argb, int width, int height);

void ui_window_draw_body(struct UIWindow *self, int top, int width,
                                  const char *bitmap);
int ui_window_draw_button_bar(struct UIWindow *self, int variant,
                                      int width, int rows);
void ui_window_draw_sys_box(struct UIWindow *self, int x, int y,
                                    int width, int height);

void ui_window_text_out_line(struct UIWindow *self, int x, int y,
                                     const struct String *text, int width, int align, int face, int height,
                                     COLORREF colour, BYTE bold);

void ui_window_text_out_outlined(struct UIWindow *self, int x, int y, const char *text,
                                         int len, COLORREF colour, COLORREF outline, int font_face, int font_height,
                                         int font_arg3, int font_arg4);
void ui_window_text_out_runs(struct UIWindow *self, int x, int y, const char *text, int len,
                           COLORREF *colour, int font_face, int font_height, BYTE shadow);
void ui_window_draw_text_outlined_vertical(struct UIWindow *self, int x, int y, const char *text, int len,
                           COLORREF colour, COLORREF outline, int font_face, int font_height, int font_arg3);

int ui_window_vertical_text_length(struct UIWindow *self, const char *text, int len,
                                           int font_face, int font_height, int font_arg3);
struct String *ui_window_ellipsize_text(struct UIWindow *self, struct String *out,
                                                    int max_width, const char *text, int font_face, int font_height,
                                                    int font_arg);
struct String *ui_window_fit_plain_text(struct UIWindow *self, struct String *out,
                                        struct TextView text, int width);
void text_split_lines(const char *text, struct TextArray *lines, int width);

#include "core/money_text.h"
struct AmountColours {
    COLORREF colour;
    int double_colour;
};
struct AmountColours *ui_window_amount_colours(struct UIWindow *self,
                                                        struct AmountColours *out, long long amount);

struct BitmapRes;
struct SprClip;
struct SprImg;
struct String;
struct Vec;
struct UIBitmapButton;
struct UIWindow;
void ui_window_draw_spr_clip(struct UIWindow *self, int x, int y, struct SprImg *img,
                                     struct SprClip *clip, float scale, float angle, DWORD colour,
                                     DWORD *palette, float zoom2);
BYTE ui_window_fit_motion(void *motion, void *spr,
                                   int width, int height, int *ox, int *oy, float *scale);
void ui_window_text_out_down(struct UIWindow *self, int x, int y,
                                     const char *text, int len, int font_face, int font_height,
                                     COLORREF colour);
void ui_window_text_out_outlined_ptr(struct UIWindow *self, int x, int y,
                                            const char *text, int len, COLORREF *colour,
                                            COLORREF outline, int font_face, int font_height,
                                            int font_arg);
void ui_window_text_out_runs_ex(struct UIWindow *self, int x, int y,
                                       const char *text, int len, COLORREF *colour, int font_face,
                                       int font_height, int font_arg3, int font_arg4, BYTE shadow);
SIZE *ui_window_text_extent(struct UIWindow *self, SIZE *out,
                                     const char *text, int len, int font_face, int font_height,
                                     int font_arg3, int font_arg4);
int ui_window_markup_text_width(struct UIWindow *self,
                                        struct String text, int font_face, int font_height,
                                        BYTE bold);
int ui_window_text_out_wrapped(struct UIWindow *self, int x, int y,
                                       const struct String *text, int wrap_width, int line_gap,
                                       int align, int face, int height, int arg9, BYTE arg10);
void ui_window_text_out_price(struct UIWindow *self, int value,
                                      struct String text, int x, int y);
int ui_window_get_color_text_width(struct UIWindow *self, const char *text,
                                          int len, int font_face, int font_height, int font_arg3);
void ui_window_break_lines(struct UIWindow *self, const char *text,
                                    struct TextArray *lines, int width);
void ui_window_break_coloured_lines(struct UIWindow *self,
                                            const char *text, struct TextArray *lines,
                                            struct ByteArray *flags, int width);
const char *ui_window_shorten_text(struct UIWindow *self, struct String *out,
                                            const char *text, int max_width, int font_face,
                                            int font_height, int font_arg);
#include "ui/controls/item_id_tag.h"
void text_wrap_bytes(const char *text, struct TextArray *lines, int width, int skip_cr);
void ui_window_draw_box_bitmaps(struct UIWindow *self, const char *prefix,
                                        int x, int y, int width, int height, BYTE tile_bg);
struct UIWindow *ui_window_add_sys_button(struct UIWindow *self);
struct UIWindow *ui_window_add_close_button(struct UIWindow *self,
                                                    const char *tooltip, int width);
struct UIBitmapButton *ui_window_add_big_close_button(struct UIWindow *self, const char *tooltip,
                                                             int width);
struct UIWindow *ui_window_add_mini_button(struct UIWindow *self,
                                                   const char *tooltip, int width);
struct UIBitmapButton *ui_window_add_bitmap_button(struct UIWindow *self,
                                                           int id, int x, int y, const char *name,
                                                           const char *tooltip);
struct UIBitmapButton *ui_window_add_numbered_button(struct UIWindow *self, int id, int x, int y,
                                                             const char *name, const char *tooltip,
                                                             BYTE skin);
struct UIWindow *ui_window_add_check_box(struct UIWindow *self, int id,
                                                 int width, int height, const char *label, int x,
                                                 int y, BYTE checked, const char *tooltip,
                                                 int label_height, COLORREF label_colour,
                                                 const char *checked_bmp,
                                                 const char *unchecked_bmp);
struct UIWindow *ui_window_add_check_box_plain(struct UIWindow *self,
                                                      int id, int width, int height,
                                                      const char *label, int x, int y, int checked,
                                                      const char *tooltip);

void ui_window_draw_new_item_mark(struct UIWindow *self, int x, int y);

#endif
