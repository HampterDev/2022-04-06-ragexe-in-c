#include "ui/controls/buttons.h"
#include "ui/items/refine_controls.h"
#include "ui/items/refining_internal.h"
#include "core/object_type_tables.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "ui/wndmgr/wndmgr.h"

#define COLOUR_CAPTION 0x3e1b12
#define COLOUR_CAPTION_DISABLED 0x6f4333
#define COLOUR_CAPTION_HIGHLIGHT 0xe1c1f
#define COLOUR_REFINE_DISABLED 0x94644a
#define COLOUR_PRICE 0xffffff

#define MSG_REFINE_CAPTION 0xb96
#define MSG_RETURN_CAPTION 0xb9d
#define MSG_AGAIN_CAPTION 0xca9
#define MSG_AGAIN_COUNT 0xcd5

void ui_refine_button_set_show(struct UIButton *button, BYTE show)
{
    if (!show)
        ui_window_mgr_release_capture(&g_windowMgr);
    ui_button_set_show(button, show);
}

#define CAPTION_Y(btn, y) ((y) + ((btn)->button.base.state == UI_BTN_DOWN))

static void refine_button_centred(struct UIBitmapButton *btn, const char *text, int y, int width_height,
                                  int width_arg3, int height, COLORREF colour, int arg3)
{
    struct UIWindow *wnd = &btn->button.base;
    int w = ui_window_text_width(wnd, text, 0, 0, width_height, width_arg3, 0);

    ui_window_text_out(wnd, (int)((float)(wnd->width - w) * 0.5f), y, text, 0, 0, height, colour, arg3, 0);
}

void ui_refining_btn_refining_destroy(struct UIWindow *window)
{
    struct UIRefining_BtnRefining *self = (struct UIRefining_BtnRefining *)window;
    self->button.button.base.ops = &ui_refining_btn_refining_ops.base;
    ui_bitmap_button_destruct(&self->button);
    mem_free(self);
}

void ui_refining_btn_refining_on_draw(struct UIWindow *window)
{
    struct UIRefining_BtnRefining *self = (struct UIRefining_BtnRefining *)window;
    struct UIBitmapButton *btn = &self->button;
    struct String price;
    int w;

    ui_bitmap_button_on_draw((struct UIWindow *)(btn));
    w = ui_window_text_width(&btn->button.base, msg_str(MSG_REFINE_CAPTION), 0, 0, 0xd, 1, 0);
    ui_window_text_out(&btn->button.base, (int)((float)(btn->button.base.width - w) * 0.5f),
                     CAPTION_Y(btn, 0xb), msg_str(MSG_REFINE_CAPTION), 0, 0, 0xc,
                     btn->button.disabled == 1 ? COLOUR_REFINE_DISABLED : COLOUR_CAPTION, 1, 0);
    if (btn->button.disabled)
        return;
    string_init(&price);
    string_format(&price, "%d", self->price);
    w = ui_window_text_width(&btn->button.base, string_cstr(&price), 0, 0, 0xc, 1, 0);
    string_free(&price);
    string_format(&price, "%d", self->price);
    ui_window_text_out(&btn->button.base, 0x53 - w, CAPTION_Y(btn, 0x1c), string_cstr(&price), 0, 0, 0xc,
                     COLOUR_PRICE, 1, 0);
    string_free(&price);
}

void ui_refining_btn_again_destroy(struct UIWindow *window)
{
    struct UIRefining_BtnAgain *self = (struct UIRefining_BtnAgain *)window;
    self->button.button.base.ops = &ui_refining_btn_again_ops.base;
    ui_bitmap_button_destruct(&self->button);
    mem_free(self);
}

void ui_refining_btn_again_on_draw(struct UIWindow *window)
{
    struct UIRefining_BtnAgain *self = (struct UIRefining_BtnAgain *)window;
    struct UIBitmapButton *btn = &self->button;
    struct UIWindow *wnd = &btn->button.base;
    COLORREF colour;
    struct String line, price;
    int w;

    ui_bitmap_button_on_draw((struct UIWindow *)(btn));
    w = ui_window_text_width(wnd, msg_str(MSG_AGAIN_CAPTION), 0, 0, 0xc, 1, 0);
    if (self->highlight == 1)
        colour = COLOUR_CAPTION_HIGHLIGHT;
    else
        colour = btn->button.disabled == 1 ? COLOUR_CAPTION_DISABLED : COLOUR_CAPTION;
    if (btn->button.disabled == 1) {
        ui_window_text_out(wnd, (int)((float)(wnd->width - w) * 0.5f), CAPTION_Y(btn, 0xe),
                         msg_str(MSG_AGAIN_CAPTION), 0, 0, 0xc, colour, 1, 0);
        return;
    }
    ui_window_text_out(wnd, (int)((float)(wnd->width - w) * 0.5f), CAPTION_Y(btn, 9), msg_str(MSG_AGAIN_CAPTION),
                     0, 0, 0xc, colour, 1, 0);

    string_init(&line);
    string_format(&line, msg_str(MSG_AGAIN_COUNT), self->count);
    refine_button_centred(btn, string_cstr(&line), CAPTION_Y(btn, 0x16), 0xb, 0, 0xb, 0, 0);

    string_init(&price);
    string_format(&price, "%d", self->price);
    w = ui_window_text_width(wnd, string_cstr(&price), 0, 0, 0xb, 1, 0);
    string_free(&price);
    string_format(&price, "%d", self->price);
    ui_window_text_out(wnd, (int)((float)(wnd->width - w) * 0.5f), CAPTION_Y(btn, 0x26), string_cstr(&price),
                     0, 0, 0xb, COLOUR_PRICE, 1, 0);
    string_free(&price);
    string_free(&line);
}

void ui_refining_btn_return_destroy(struct UIWindow *window)
{
    struct UIRefining_BtnReturn *self = (struct UIRefining_BtnReturn *)window;
    self->button.button.base.ops = &ui_refining_btn_return_ops.base;
    ui_bitmap_button_destruct(&self->button);
    mem_free(self);
}

void ui_refining_btn_return_on_draw(struct UIWindow *window)
{
    struct UIRefining_BtnReturn *self = (struct UIRefining_BtnReturn *)window;
    struct UIBitmapButton *btn = &self->button;

    ui_bitmap_button_on_draw((struct UIWindow *)(btn));
    refine_button_centred(btn, msg_str(MSG_RETURN_CAPTION), CAPTION_Y(btn, 0xe), 0xc, 1, 0xc,
                          self->highlight == 1 ? COLOUR_CAPTION_HIGHLIGHT : COLOUR_CAPTION, 1);
}
