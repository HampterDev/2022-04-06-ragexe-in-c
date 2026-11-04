#ifndef UI_CHARACTER_SELECTION_BUTTONS_H
#define UI_CHARACTER_SELECTION_BUTTONS_H

#include "ui/controls/buttons.h"

struct UIHairBitmap;
struct UIHairStyleButton {
    struct UIBitmapButton button;
    int style;
    int male;
    struct UIHairBitmap *picture;
    BYTE selected;
};

struct UISlotBmpButton {
    struct UIBitmapButton bmp;
    int occupied;
    BYTE selected;
    int style;
    int slot;
    struct UIWindow *job_icon;
    short anim_tick;
    short anim_divisor;
    unsigned long long drawn_key;
};

int ui_slot_bmp_button_changed(struct UISlotBmpButton *self);

struct UIHairStyleButton *ui_hair_style_button_construct(struct UIHairStyleButton *self, int style, int male);
intptr_t ui_hair_style_button_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                         intptr_t val2, intptr_t val3, intptr_t val4);
struct UIWindow *ui_descript_race_create(void);
intptr_t ui_descript_race_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                      intptr_t val2, intptr_t val3, intptr_t val4);

struct UINewMakeCharWnd *ui_new_make_char_wnd_construct(struct UINewMakeCharWnd *self);
struct UIWindow *ui_new_make_char_wnd_new(void);

#endif
