#ifndef UI_CONTROLS_STATIC_H
#define UI_CONTROLS_STATIC_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/buttons.h"
#include "core/object_type_tables.h"

struct UIStaticText3;
struct UITextButton;
struct UIWindow;
struct UIWindowOps;

struct UIStaticText {
    struct UIWindow base;
    int r, g, b;
    int white_shadow;
    int outline;
    DWORD background;
    COLORREF outline_colour;
    int font_height;
    int font_face;
    BYTE font_arg3, font_arg4;
    int has_tooltip;
    struct String text;
    struct String tooltip;
};
OFFSET_ASSERT(UIStaticText, background, 0x90);
OFFSET_ASSERT(UIStaticText, has_tooltip, 0xa4);
struct UIWindow *ui_static_text_construct(struct UIWindow *self);
void ui_static_text_destruct(struct UIStaticText *self);
void ui_static_text_set_text(struct UIWindow *self, const char *text,
                                     int outline);
void ui_static_text_set_text_ellipsis(struct UIWindow *self, const char *text);
void ui_static_text_set_font(struct UIWindow *self, int face, int height, BYTE font_arg3,
                                     BYTE font_arg4);
void ui_static_text_set_text_colour(struct UIWindow *self, int red,
                                           int green, int blue);

struct UIStaticBitmap {
    struct UIWindow base;
    struct String bitmap;
    struct String tooltip;
    int tip_x, tip_y;
    int hover_to_parent;
};
struct UIStaticBitmap *ui_static_bitmap_construct(struct UIStaticBitmap *self);
void ui_static_bitmap_destruct(struct UIStaticBitmap *self);
void ui_static_bitmap_set_bitmap(struct UIStaticBitmap *self, const char *name);
void ui_static_bitmap_set_tool_tip(struct UIWindow *self, const char *text);

struct UIStaticTextLF {
    struct UIWindow base;
    struct String text;
    int line_gap;
    int align;
    int font_face;
    int line_height;
    int colour;
    BYTE bold;
};
void ui_static_text_lf_set_text(struct UIStaticTextLF *self, struct String text);
static __inline struct UIStaticTextLF *ui_static_text_lf_construct(struct UIStaticTextLF *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_static_text_lf_ops;
    string_init(&self->text);
    self->line_gap = 0;
    self->align = 0;
    self->font_face = 0;
    self->line_height = 12;
    self->colour = 0;
    self->bold = 0;
    return self;
}

#define UI_ID_NAVI_LINK 0x1ae
struct UINaviLinkButton {
    struct UITextButton text;
    struct String link;
    int line_x;
    int line;
};
struct UINaviLinkButton *ui_navi_link_button_create(void);
struct UINaviLinkButton *ui_navi_link_button_construct(struct UINaviLinkButton *self);

void ui_static_text_destroy(struct UIWindow *window);
void ui_static_text_on_draw(struct UIWindow *self);
void ui_static_text_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_static_text_on_mouse_hover(struct UIWindow *self, int x_unused, int y_unused);
void ui_static_text_on_wheel(struct UIWindow *self, int delta);
struct String *ui_static_text_get_text(struct UIWindow *self,
                                                  struct String *out);
void ui_static_bitmap_destroy(struct UIWindow *window);
void ui_static_bitmap_on_draw(struct UIWindow *self);
void ui_static_bitmap_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_static_bitmap_on_wheel(struct UIWindow *self, int delta);
void ui_static_text_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_static_text_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_static_text_on_mouse_shape(struct UIWindow *self, int x, int y);
void ui_static_text_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_static_text_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_static_text_on_r_btn_up(struct UIWindow *self, int x, int y);
void ui_static_bitmap_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_static_bitmap_on_mouse_shape(struct UIWindow *self, int x, int y);
void ui_static_bitmap_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_static_bitmap_on_r_btn_up(struct UIWindow *self, int x, int y);
void ui_static_text_lf_destruct(struct UIStaticTextLF *self);
void ui_static_text_lf_destroy(struct UIWindow *window);
void ui_static_text_lf_on_draw(struct UIWindow *self);
void ui_static_text_lf_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_static_text_lf_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_static_text_lf_on_wheel(struct UIWindow *self, int delta);

void ui_static_text_set_text_fit(struct UIWindow *wnd, const char *text);
void ui_static_text_set_text_ellipsis_wide(struct UIWindow *wnd,
                                                 const char *text);
void ui_static_text3_destruct(struct UIStaticText3 *self);
void ui_static_text3_destroy(struct UIWindow *window);
void ui_static_text3_on_draw(struct UIWindow *self);
void ui_static_text_set_text_or_shrink2(struct UIWindow *wnd,
                                              const char *text, int outline);

struct UISlotBitmap {
    struct UIStaticBitmap bitmap;
    BYTE has_add_button;
    struct UIBitmapButton *add_button;
    int slot;
};

struct UIStaticText3 {
    struct UIStaticText st;
    int x_offset;
};

#endif
