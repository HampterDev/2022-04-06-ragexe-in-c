#ifndef UI_CONTROLS_CHECK_RADIO_H
#define UI_CONTROLS_CHECK_RADIO_H

#include "ui/window.h"
#include "std/string.h"
#include "ui/draw.h"
#include "ui/skin.h"

struct UIWindow;

struct UICheckBox {
    struct UIWindow base;
    int disabled;
    struct String checked_bitmap;
    struct String unchecked_bitmap;
    struct String label;
    struct String tooltip;
    BYTE no_draw;
    COLORREF label_colour;
    int label_height;
    int notice_value;
    int send_notice;
    struct String unchecked_tooltip;
};
#define UI_MSG_CHECK_NOTICE 2
#define UI_ID_CHECKBOX 0xcd

struct UICheckBox *ui_check_box_construct(struct UIWindow *self);
void ui_check_box_set_bitmaps(struct UIWindow *self, const char *checked,
                                      const char *unchecked, const char *label);
void ui_check_box_set_check(struct UIWindow *self, int checked);
void ui_check_box_set_hidden(struct UIWindow *self, BYTE hidden);
void ui_check_box_set_tool_tip(struct UIWindow *self, const char *text);
void ui_check_box_destruct(struct UICheckBox *self);
void ui_check_box_fit_to_label(struct UICheckBox *self);
void ui_check_box_set_bitmaps_fit(struct UICheckBox *self, const char *checked,
                                         const char *unchecked);
void ui_check_box_set_check_quiet(struct UICheckBox *self, int checked);

struct UIRadioBtn {
    struct UIWindow base;
    struct String label;
    int enabled;
    int alt_on_bitmap;
    int value;
    struct String tooltip;
    struct String on_bitmap;
    struct String off_bitmap;
    int label_height;
    COLORREF label_colour;
};
#define UI_ID_RADIO 0xcf

struct UIRadioBtn *ui_radio_btn_construct(struct UIRadioBtn *self);
void ui_radio_btn_destruct(struct UIRadioBtn *self);
void ui_radio_btn_set_label(struct UIWindow *self, const char *label);

void ui_check_box_destroy(struct UIWindow *window);
void ui_check_box_on_draw(struct UIWindow *self);
void ui_check_box_on_l_btn_down(struct UIWindow *self, int x_unused, int y_unused);
void ui_check_box_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_check_box_on_mouse_shape(struct UIWindow *self, int x_unused, int y_unused);
void ui_radio_btn_destroy(struct UIWindow *window);
void ui_radio_btn_on_draw(struct UIWindow *self);
void ui_radio_btn_on_l_btn_down(struct UIWindow *self, int x_unused, int y_unused);
void ui_radio_btn_on_mouse_hover(struct UIWindow *self, int x, int y);
int ui_radio_btn_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1,
                                  int val2, int val3, int val4);
void ui_window_set_state(struct UIWindow *self, int state);

struct UIBmpRadioBtn;
void ui_bmp_radio_btn_destroy(struct UIWindow *window);
void ui_bmp_radio_btn_on_draw(struct UIWindow *self);
void ui_bmp_radio_btn_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_bmp_radio_btn_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_bmp_radio_btn_hover_on(struct UIWindow *self);
void ui_bmp_radio_btn_hover_off(struct UIWindow *self);
int ui_bmp_radio_btn_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                     int val1, int val2, int val3, int val4);
BYTE ui_bmp_radio_btn_set_bitmap(struct UIWindow *wnd, const char *path,
                                        int look);
void ui_bmp_radio_btn_set_text_style(struct UIWindow *wnd, int look,
                                           int height, COLORREF colour, BYTE bold, BYTE shadow,
                                           int x, int y);
struct UIWindow *ui_bmp_radio_btn_create(int value);
void ui_bmp_radio_btn_set_caption(struct UIWindow *wnd, struct TextView text);
void ui_bmp_radio_btn_set_tool_tip(struct UIWindow *wnd, const char *text);

#endif
