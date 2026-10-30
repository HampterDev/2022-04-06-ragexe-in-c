#include "ui/controls/buttons.h"
#include "core/object_type.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <stdint.h>
#include <stdlib.h>
#include "core/skin_path.h"

#define BMP_TEXT_STATE_DISABLED 3
#define BMP_TEXT_DEFAULT_HEIGHT 0xc

static struct BitmapRes *bmp_text_skin_bitmap(const struct String *path)
{
    return object_as_type(res_mgr_get(g_resMgr(), ui_skin_path(string_cstr(path))), OBJECT_BITMAP_RESOURCE);
}

struct BmpTextLook {
    int bitmap;
    COLORREF shadow;
    BYTE text_shifted;
    COLORREF plain_colour;
};
static const struct BmpTextLook bmp_text_looks[4] = {
    { UI_BTN_UP, 0xd7d7d7, 0, 0x000000 },
    { UI_BTN_HOVER, 0xc7a796, 0, 0x000000 },
    { UI_BTN_DOWN, 0xffffff, 1, 0x000000 },
    { UI_BMP_DISABLED, 0xe2e2e2, 1, 0x6f6f6f },
};

void ui_bmp_text_button_on_draw(struct UIWindow *window)
{
    struct UIBmpTextButton *self = (struct UIBmpTextButton *)window;
    struct UIWindow *wnd = &self->bmp.button.base;
    const char *caption = string_cstr(&self->caption);
    const struct BmpTextStyle *style = NULL, *s;
    const struct BmpTextLook *look;
    size_t i;
    BYTE disabled = self->bmp.button.disabled;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    for (i = 0; i < self->style_count; ++i) {
        s = &self->styles[i];
        if (disabled == 0 ? wnd->state == s->state : disabled == 1 && s->state == BMP_TEXT_STATE_DISABLED) {
            style = s;
            break;
        }
    }
    if (disabled == 1)
        look = &bmp_text_looks[3];
    else if (wnd->state >= UI_BTN_UP && wnd->state <= UI_BTN_DOWN)
        look = &bmp_text_looks[wnd->state];
    else
        look = NULL;

    if (look) {
        ui_window_draw_bitmap(wnd, 0, 0, bmp_text_skin_bitmap(&self->bmp.bitmaps[look->bitmap]), 0);
        if (self->caption.size != 0) {
            int shadow_d = look->text_shifted ? 0 : 1, text_d = look->text_shifted ? 1 : 0;

            if (style) {
                if (style->shadow == 1)
                    ui_window_text_out(wnd, style->x + shadow_d, style->y + shadow_d, caption, 0, 0,
                                     style->font_height, look->shadow, disabled == 1 ? 0 : style->font_arg3, 0);
                ui_window_text_out(wnd, style->x + text_d, style->y + text_d, caption, 0, 0,
                                 style->font_height, style->colour, style->font_arg3, 0);
            } else {
                int x = (int)((wnd->width - ui_window_text_width(wnd, caption, 0, 0, BMP_TEXT_DEFAULT_HEIGHT,
                                                               0, 0)) * 0.5);
                int y = (int)((wnd->height - ui_window_text_height(wnd, caption, 0, 0,
                                                                 BMP_TEXT_DEFAULT_HEIGHT, 0, 0)) * 0.5);

                ui_window_text_out(wnd, x + shadow_d, y + shadow_d, caption, 0, 0, BMP_TEXT_DEFAULT_HEIGHT,
                                 look->shadow, 0, 0);
                ui_window_text_out(wnd, x + text_d, y + text_d, caption, 0, 0, BMP_TEXT_DEFAULT_HEIGHT,
                                 look->plain_colour, 0, 0);
            }
        }
    }
    if (self->bmp.redraw_parent == 1 && wnd->parent)
        ui_window_invalidate(wnd->parent);
}

void ui_bmp_text_button_set_text_style(struct UIBmpTextButton *self, int state,
                                             int font_height, COLORREF colour, char arg4, char arg5, int x, int y)
{
    struct UIWindow *wnd = &self->bmp.button.base;
    struct BmpTextStyle style, *s;
    size_t i;

    if (x == UI_TEXT_CENTRED)
        x = (int)((wnd->width - ui_window_text_width(wnd, string_cstr(&self->caption), self->caption.size,
                                                    0, font_height, (BYTE)arg4, 0)) * 0.5);
    if (y == UI_TEXT_CENTRED)
        y = (int)((wnd->height - ui_window_text_height(wnd, string_cstr(&self->caption),
                                                      self->caption.size, 0, font_height, (BYTE)arg4, 0)) * 0.5);
    for (i = 0; i < self->style_count; ++i) {
        s = &self->styles[i];
        if (s->state == state)
            return;
    }
    style.state = state;
    style.x = x;
    style.y = y;
    style.font_height = font_height;
    style.font_arg3 = arg4;
    style.colour = colour;
    style.shadow = arg5;
    if (self->style_count == self->style_capacity) {
        size_t limit = SIZE_MAX / sizeof *self->styles;
        size_t capacity;
        struct BmpTextStyle *grown;
        if (self->style_count == limit) abort();
        capacity = self->style_capacity > limit / 2 ? limit : self->style_capacity * 2;
        if (capacity < 4) capacity = 4;
        grown = realloc(self->styles, capacity * sizeof *self->styles);
        if (!grown) abort();
        self->styles = grown;
        self->style_capacity = capacity;
    }
    self->styles[self->style_count++] = style;
}
