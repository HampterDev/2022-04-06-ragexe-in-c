#ifndef UI_CHARACTER_CHARACTER_DRAW_H
#define UI_CHARACTER_CHARACTER_DRAW_H

#include <stdint.h>

struct UIWindow;

struct CharacterDrawOptions {
    int x, y;
    unsigned char sex;
    short job;
    short body, head;
    short accessory, accessory2, accessory3;
    int robe;
    unsigned char riding;
    int scaled;
    int body_palette, head_palette;
    int action, motion;
    uint32_t start_time;
};

void ui_draw_character(struct UIWindow *window, const struct CharacterDrawOptions *options,
                       unsigned char flag);

#endif
