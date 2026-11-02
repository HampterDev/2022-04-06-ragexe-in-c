#ifndef UI_CONTROLS_UI_STRING_H
#define UI_CONTROLS_UI_STRING_H

#include "ui/window.h"
#include "core/text_view.h"

struct UIString {
    struct UIWindow base;
    int joined;
    struct String link;
    int r, g, b;
    int size;
    struct String image;
    struct String font;
    struct String text;
};

struct UIString *ui_string_construct(struct UIString *self);
void ui_string_destruct(struct UIString *self);
struct UIString *ui_string_create(int width, int height, struct TextView text);
void ui_string_parse(struct UIString *self, struct TextView text);
void ui_string_set_text(struct UIString *self, int width, int height, struct TextView text);

void ui_string_destroy(struct UIWindow *window);
void ui_string_on_draw(struct UIWindow *window);
void ui_string_on_mouse_shape(struct UIWindow *window, int x, int y);
void ui_string_on_l_btn_up(struct UIWindow *window, int x, int y);
intptr_t ui_string_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                     intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

#endif
