#ifndef UI_CONTROLS_RESIZER_H
#define UI_CONTROLS_RESIZER_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"

struct UIWindow;

enum UIResizeEdge {
    UI_EDGE_LEFT = 0,
    UI_EDGE_RIGHT = 1,
    UI_EDGE_TOP = 2,
    UI_EDGE_BOTTOM = 3,
    UI_EDGE_TOP_LEFT = 4,
    UI_EDGE_TOP_RIGHT = 5,
    UI_EDGE_BOTTOM_LEFT = 6,
    UI_EDGE_BOTTOM_RIGHT = 7,
};
#define UI_MSG_RESIZE 0xe
struct UIResizer {
    struct UIWindow base;
    struct String bitmap;
    int press_x, press_y;
    int edge;
    int parent_width, parent_height;
    DWORD line_colour;
    DWORD fill_colour;
};
struct UIResizer *ui_resizer_construct(struct UIResizer *self);
void ui_resizer_set_bitmap(struct UIResizer *self, const char *name);
void ui_resizer_set_edge(struct UIResizer *self, int edge);
void ui_resizer_set_colours(struct UIResizer *self, DWORD line, DWORD fill);

void ui_resizer_destroy(struct UIWindow *window);
void ui_resizer_on_draw(struct UIWindow *self);
void ui_resizer_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_resizer_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_resizer_on_mouse_hover(struct UIWindow *self, int x_unused, int y_unused);
void ui_resizer_on_mouse_shape(struct UIWindow *self, int x, int y);
void ui_resizer_on_l_btn_up(struct UIWindow *self, int x_unused, int y_unused);

#endif
