#ifndef UI_CHARACTER_CHARACTER_DRAW_H
#define UI_CHARACTER_CHARACTER_DRAW_H

#include <stdint.h>
#include "session/session_names.h"

struct UIWindow;

#define UI_ANIMATION_STEPS 64

struct UIAnimationStep {
    struct MotionStepping stepping;
    int frame;
};

struct UIAnimation {
    int count;
    struct UIAnimationStep steps[UI_ANIMATION_STEPS];
};

void ui_animation_reset(struct UIAnimation *self);
void ui_animation_add(struct UIAnimation *self, const struct MotionStepping *stepping, int frame);
int ui_animation_due(const struct UIAnimation *self);

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
    struct UIAnimation *animation;
};

void ui_draw_character(struct UIWindow *window, const struct CharacterDrawOptions *options,
                       unsigned char flag);

#endif
