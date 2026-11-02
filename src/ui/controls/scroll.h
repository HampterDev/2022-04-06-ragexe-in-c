#include "render/rp_pool.h"
#ifndef UI_CONTROLS_SCROLL_H
#define UI_CONTROLS_SCROLL_H

#include "ui/window.h"
#include "std/container_types.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/controls/buttons.h"

struct BitmapRes;
struct TexDrawPrim;
struct UIWindow;
struct UIWindowOps;

enum UIScrollOrientation {
    UI_SCROLL_HORZ = 0,
    UI_SCROLL_VERT = 1,
};
enum UIScrollPart {
    UI_SCROLL_OUTSIDE = -1,
    UI_SCROLL_ARROW_BACK = 0,
    UI_SCROLL_TRACK_BACK = 1,
    UI_SCROLL_THUMB = 2,
    UI_SCROLL_TRACK_FWD = 3,
    UI_SCROLL_ARROW_FWD = 4,
};
enum UIScrollHot {
    UI_SCROLL_HOT_NONE = 0,
    UI_SCROLL_HOT_BACK = 1,
    UI_SCROLL_HOT_FWD = 2,
    UI_SCROLL_HOT_THUMB = 3,
    UI_SCROLL_HOT_DRAG = 6,
};
#define UI_MSG_SCROLL_V 7
#define UI_MSG_SCROLL_H 8
#define UI_MSG_PAGE_UP 9
#define UI_MSG_PAGE_DOWN 0xa
#define UI_MSG_PAGE_LEFT 0xb
#define UI_MSG_PAGE_RIGHT 0xc
#define UI_MSG_BAR_PAGE_UP 0x12
#define UI_MSG_BAR_PAGE_DOWN 0x13
#define UI_SCROLL_MIN_THUMB 8

struct UIScrollBar {
    struct UIWindow base;
    int hot;
    int max_pos;
    int pos;
    int orientation;
    int press_x, press_y;
    int drag_steps;
    int total;
    int page;
    int button_size;
    int thickness;
    int min_thumb;
};
struct UIScrollBar *ui_scroll_bar_construct(struct UIScrollBar *self);
void ui_scroll_bar_destruct(struct UIScrollBar *self);
void ui_scroll_bar_create(struct UIScrollBar *self, int x, int y, int orientation, int length);
int ui_scroll_bar_thumb_pos(struct UIScrollBar *self);
void ui_scroll_bar_set_range(struct UIScrollBar *self, int max_pos,
                                     int page, int total);
void ui_scroll_bar_set_pos(struct UIScrollBar *self, int pos);

struct ChildPosition {
    struct UIWindow *child;
    POINT cell;
};

struct UIScrollWnd {
    struct UIWindow base;
    int view_width, view_height;
    int content_width, content_height;
    struct UIScrollBar *hbar;
    struct UIScrollBar *vbar;
    int vbar_width;
    int hbar_height;
    int hpos, vpos;
    BYTE hscroll_on;
    BYTE vscroll_on;
    struct Tree child_positions;
};
struct UIScrollWnd *ui_scroll_wnd_construct(struct UIScrollWnd *self, int width,
                                                     int height, int composites);
void ui_scroll_wnd_destruct(struct UIScrollWnd *self);
void ui_scroll_wnd_add_child(struct UIScrollWnd *self,
                                     struct UIWindow *child);
void ui_scroll_wnd_remove_all(struct UIScrollWnd *self);
void ui_scroll_wnd_update_bars(struct UIScrollWnd *self);

void ui_scroll_bar_destroy(struct UIWindow *window);
void ui_scroll_bar_resize(struct UIWindow *self, int width, int height);
BYTE ui_scroll_bar_is_click_through(struct UIWindow *self);
int ui_scroll_bar_get_thumb_size(struct UIScrollBar *self);
int ui_scroll_bar_thumb_travel(struct UIScrollBar *self);
int ui_scroll_bar_hit_part(struct UIScrollBar *self, int x, int y);
void ui_scroll_bar_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_scroll_bar_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_scroll_bar_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_scroll_bar_on_mouse_shape(struct UIWindow *self, int x, int y);
void ui_scroll_bar_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_scroll_bar_on_r_btn_up(struct UIWindow *self, int x, int y);
void ui_scroll_bar_on_wheel(struct UIWindow *self, int delta);
void ui_scroll_bar_on_draw(struct UIWindow *self);
struct UIScrollBar *ui_scroll_bar3_construct(struct UIScrollBar *self);
void ui_scroll_bar3_destroy(struct UIWindow *window);
void ui_scroll_bar3_on_draw(struct UIWindow *self);
struct UIScrollBar *ui_scroll_bar_navi_construct(struct UIScrollBar *self);
void ui_scroll_bar_navi_destroy(struct UIWindow *window);
void ui_scroll_bar_navi_on_draw(struct UIWindow *self);

void ui_scroll_bar2_destroy(struct UIWindow *window);
void ui_scroll_bar2_on_draw(struct UIWindow *self);

void ui_scroll_wnd_destroy(struct UIWindow *window);
void ui_scroll_wnd_remove_child(struct UIScrollWnd *self,
                                        struct UIWindow *child, int destroy);
void ui_scroll_wnd_resize(struct UIWindow *self, int width, int height);
void ui_scroll_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_scroll_wnd_on_wheel(struct UIWindow *self, int delta);
int ui_scroll_wnd_send_msg(struct UIWindow *self,
                                   struct UIWindow *sender, int msg, int val1, int val2, int val3,
                                   int val4);
BYTE ui_scroll_wnd_render(struct UIWindow *self, int force);
struct TexDrawPrim *ui_scroll_wnd_make_quad(struct UIWindow *window);

#endif
