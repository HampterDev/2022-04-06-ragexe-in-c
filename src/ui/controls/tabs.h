#ifndef UI_CONTROLS_TABS_H
#define UI_CONTROLS_TABS_H

#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/buttons.h"
#include "ui/controls/listbox.h"

struct Vec;
struct UIWindow;

enum UITabKind {
    UI_TAB_ITM = 0,
    UI_TAB_CMD = 1,
    UI_TAB_QUE = 2,
    UI_TAB_ITEM = 3,
};
#define UI_MSG_TAB_CHANGED 0x16
struct UITab {
    struct UIWindow base;
    int count;
    int kind;
};
struct UITab *ui_tab_construct(struct UITab *self);

enum UITabSide {
    UI_TABS_LEFT = 0,
    UI_TABS_TOP = 1,
    UI_TABS_RIGHT = 2,
    UI_TABS_BOTTOM = 3,
};
#define UI_MSG_TAB_RENAMED 0x74
#define UI_MSG_TAB_DRAGGED 0x75
struct UITabControl {
    struct UIWindow base;
    int selected;
    int side;
    int thickness;
    int label_margin;
    BYTE equal_lengths;
    DWORD fill_selected, fill;
    DWORD frame_selected, frame;
    int text_colour_selected, text_colour;
    BYTE renamable;
    BYTE no_fill;
    int drag_x, drag_y;
    struct Vec names;
    struct Vec colours;
    struct Vec lengths;
    struct UIWindow *edit;
    int font_height;
    struct StdBitVector new_marks;
};
struct UIWindow *ui_tab_control_construct(struct UIWindow *self);
void ui_tab_control_setup(struct UIWindow *self, int side, int thickness, int padding,
                                   BYTE equal_lengths, BYTE no_fill, BYTE renamable, DWORD fill_selected,
                                   DWORD fill, DWORD frame_selected, DWORD frame, int text_colour_selected,
                                   int text_colour);
void ui_tab_control_add_tab(struct UIWindow *self, const char *name);
void ui_tab_control_fit_size(struct UITabControl *self);
void ui_tab_control_relayout(struct UITabControl *self);

void ui_tab_control_destroy(struct UIWindow *window);
void ui_tab_control_on_draw(struct UIWindow *self);
void ui_tab_control_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_tab_control_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_tab_control_on_l_btn_dbl_clk(struct UIWindow *self, int x_unused, int y_unused);
void ui_tab_control_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_tab_control_on_l_btn_up(struct UIWindow *self, int x_unused, int y_unused);
intptr_t ui_tab_control_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                    intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
void ui_tab_control_select_tab(struct UIWindow *wnd, int tab);
int ui_tab_control_tab_count(struct UITabControl *self);
int ui_tab_control_tab_at(struct UITabControl *self, int x, int y);
void ui_tab_control_set_new_mark(struct UITabControl *self, int tab, BYTE on);
void ui_tab_control_remove_tab(struct UIWindow *wnd, int index);

void ui_tab_destroy(struct UIWindow *window);
void ui_tab_on_draw(struct UIWindow *self);
void ui_tab_on_l_btn_down(struct UIWindow *self, int x_unused, int y);
intptr_t ui_tab_send_msg(struct UIWindow *self, struct UIWindow *sender_unused, int msg, intptr_t val1,
                             intptr_t val2_unused, intptr_t val3_unused, intptr_t val4_unused);

#endif
