#ifndef CONTENTS_MOUSE_H
#define CONTENTS_MOUSE_H

#include "platform/win_types.h"

enum {
    MOUSE_BUTTON_UP = 0,
    MOUSE_BUTTON_PRESSED = 1,
    MOUSE_BUTTON_HELD = 2,
    MOUSE_BUTTON_RELEASED = 3,
    MOUSE_BUTTON_DOUBLE = 4
};
enum { MOUSE_LEFT = 0, MOUSE_RIGHT = 1 };

struct Mouse {
    int x, y;
    int dx, dy;
    int button[2];
    int wheel;
    int cur_x, cur_y;
    int wheel_pending;
    char was_down[2];
    char down[2];
    DWORD press_time[2];
    DWORD double_click_ms;
    char ready;
};

extern struct Mouse g_mouse;

void mouse_read_state(struct Mouse *self);
void mouse_reset_buttons(struct Mouse *self);
void mouse_center(struct Mouse *self);
void mouse_set_x(struct Mouse *self, int x);
void mouse_set_y(struct Mouse *self, int y);
void mouse_on_move(struct Mouse *self);
void mouse_on_button(struct Mouse *self, HWND wnd, int button, int down);
void mouse_on_wheel(struct Mouse *self, int delta);

#endif
