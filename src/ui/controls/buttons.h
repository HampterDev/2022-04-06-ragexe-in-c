#include "resources/animation.h"
#ifndef UI_CONTROLS_BUTTONS_H
#define UI_CONTROLS_BUTTONS_H

#include "ui/window.h"
#include "std/string.h"
#include "core/text_array.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/ui_std.h"

struct Vec;
struct UICheckBox;
struct UIWindow;

enum UIButtonState {
    UI_BTN_UP = 0,
    UI_BTN_HOVER = 1,
    UI_BTN_DOWN = 2,
};

#define UI_ID_HOVER_NOTE 0x107
#define UI_WND_NOTE_TARGET 0xb6
#define UI_WND_NO_TOOLTIPS 0xcb

#define UI_MSG_SET_STATE 0xd
#define UI_GREY_TEXT 0xb5b5b5

struct UIButton {
    struct UIWindow base;
    struct String text;
    struct String tooltip;
    BYTE disabled;
    BYTE skinned;
    int cmd_value;
};

struct UIButtonOps {
    struct UIWindowOps base;
    void (*set_show)(struct UIButton *button, BYTE show);
};

static inline void ui_button_show(struct UIButton *button, BYTE show)
{
    const struct UIButtonOps *ops = (const struct UIButtonOps *)button->base.ops;
    ops->set_show(button, show);
}

static inline void ui_button_set_tool_tip(struct UIButton *button, const char *text)
{
    if (text)
        string_assign_cstr(&button->tooltip, text);
}

struct UIButton *ui_button_construct(struct UIButton *self);
void ui_button_destruct(struct UIButton *self);
void ui_button_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_button_set_show(struct UIButton *self, BYTE show);

struct UIBitmapButton {
    struct UIButton button;
    int bitmap_width, bitmap_height;
    struct String bitmaps[4];
    BYTE redraw_parent;
};
#define UI_BMP_DISABLED 3

struct UIBitmapButton *ui_bitmap_button_construct(struct UIBitmapButton *self);
BYTE ui_bitmap_button_set_bitmap(struct UIBitmapButton *self, const char *path,
                                         int state, int flag);
void ui_bitmap_button_set_tool_tip(struct UIBitmapButton *self,
                                          const char *text);
void ui_bitmap_button_destruct(struct UIBitmapButton *self);
void ui_bitmap_button_on_draw(struct UIWindow *self);

struct UIBasicButton {
    struct UIButton button;
    struct String label;
    BYTE redraw_parent;
};
#define UI_BASIC_BUTTON_HEIGHT 0x14
struct UIBasicButton *ui_basic_button_construct(struct UIBasicButton *self);
BYTE ui_basic_button_set_label(struct UIWindow *self, int width,
                                       const char *label);

struct UINoneVisibleButton {
    struct UIButton button;
};
struct UINoneVisibleButton *ui_none_visible_button_construct(struct UINoneVisibleButton *self);

struct UIBmpTextButton {
    struct UIBitmapButton bmp;
    struct String caption;
    struct BmpTextStyle *styles;
    size_t style_count, style_capacity;
};
struct UIBmpTextButton *ui_bmp_text_button_construct(struct UIBmpTextButton *self);
#define UI_TEXT_CENTRED (-1000)
void ui_bmp_text_button_set_text_style(struct UIBmpTextButton *self, int state,
                                             int font_height, COLORREF colour, char arg4, char arg5, int x,
                                             int y);

#define UI_MSG_TEXT_BUTTON 0x62
#define UI_MSG_TEXT_BUTTON_RCLICK 0x63
struct UITextButton {
    struct UIButton button;
    DWORD text_colour;
    DWORD normal_colour;
    DWORD hover_colour;
    DWORD outline_colour;
    BYTE outlined;
    BYTE underlined;
    int font_height;
    int font_face;
    int value;
    struct String label;
    struct TextArray lines;
};
struct UITextButton *ui_text_button_create(void);
struct UITextButton *ui_text_button_construct(struct UITextButton *self);
void ui_text_button_destruct(struct UITextButton *self);
void ui_text_button_set_text(struct UIWindow *self, const char *text, DWORD colour,
                                     int font_face, int font_height, BYTE outlined, BYTE underlined);
void ui_text_button_add_line(struct UITextButton *self, struct TextView text);
struct TextView ui_text_button_get_line(const struct UITextButton *self, size_t i);

struct UISpriteButton {
    struct UIButton button;
    int offset_x, offset_y;
    int action;
    int motion;
    int animating;
    int loop;
    struct String spr;
    struct String act;
    struct String palette_path;
};
struct UIWindow *ui_sprite_button_construct(struct UIWindow *self);
void ui_sprite_button_set_sprite(struct UIWindow *self, const char *path);
void ui_sprite_button_set_act(struct UIWindow *self, const char *path);

void ui_bmp_text_button_on_draw(struct UIWindow *self);

void ui_button_destroy(struct UIWindow *window);
void ui_button_on_draw(struct UIWindow *self);
void ui_button_on_l_btn_down(struct UIWindow *self, int x_unused, int y_unused);
void ui_button_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_button_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_button_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_button_track_mouse(struct UIWindow *self, int x, int y);
void ui_button_on_mouse_shape(struct UIWindow *self, int x, int y);
void ui_bitmap_button_destroy(struct UIWindow *window);
void ui_basic_button_destroy(struct UIWindow *window);
void ui_basic_button_on_draw(struct UIWindow *self);
void ui_none_visible_button_destroy(struct UIWindow *window);
void ui_none_visible_button_on_draw(struct UIWindow *self);
void ui_bmp_text_button_destroy(struct UIWindow *window);
void ui_text_button_destroy(struct UIWindow *window);
void ui_text_button_on_draw(struct UIWindow *self);
void ui_text_button_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_text_button_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_text_button_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_text_button_on_r_btn_down(struct UIWindow *self, int x_unused, int y_unused);
void ui_text_button_on_r_btn_up(struct UIWindow *self, int x, int y);
void ui_text_button_on_wheel(struct UIWindow *self, int delta);
void ui_bmp_text_button_set_caption(struct UIBmpTextButton *self, struct TextView text);

void ui_sprite_button_destruct(struct UISpriteButton *self);
void ui_sprite_button_destroy(struct UIWindow *window);
void ui_sprite_button_on_draw(struct UIWindow *self);
void ui_sprite_button_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_sprite_button_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_sprite_button_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_sprite_button_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_sprite_button_set_palette(struct UIWindow *wnd, const char *path);
struct UIWindow *ui_headspr_button_construct(struct UIWindow *wnd);
void ui_headspr_button_destruct(struct UISpriteButton *self);
void ui_headspr_button_destroy(struct UIWindow *window);
void ui_headspr_button_on_draw(struct UIWindow *self);

void ui_text_button_fit_width(struct UITextButton *self, int max_width,
                                      int height);
size_t ui_text_button_line_count(const struct UITextButton *self);
void ui_text_button_fit_to_caption(struct UITextButton *self);

static __inline void ui_button_bitmap(struct UIBitmapButton *button, const struct String *dir, const char *name,
                                      int state)
{
    struct String path;

    string_copy(&path, dir);
    string_append_cstr(&path, name);
    string_append_cstr(&path, ".bmp");
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
    string_free(&path);
}

static __inline struct UIBitmapButton *ui_add_named_skin_button(struct UIWindow *parent, const struct String *dir,
                                                                const char *const names[3], int x, int y, int id)
{
    void *mem = mem_alloc(sizeof(struct UIBitmapButton));
    struct UIBitmapButton *button = mem ? ui_bitmap_button_construct(mem) : NULL;

    ui_button_bitmap(button, dir, names[0], 0);
    ui_button_bitmap(button, dir, names[1], 1);
    ui_button_bitmap(button, dir, names[2], 2);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    ui_window_add_child(parent, &button->button.base);
    return button;
}

static __inline void ui_skin_dir(struct String *dir)
{
    string_init(dir);
    string_assign(dir, UI_SKIN, 15);
}

struct BmpTextStyle {
    int state;
    int x, y;
    int font_height;
    BYTE font_arg3;
    COLORREF colour;
    BYTE shadow;
};

struct ActRes;

#endif
