#include "render/rp_pool.h"
#include "ui/window_info.h"
#ifndef UI_WINDOW_TYPES_H
#define UI_WINDOW_TYPES_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "base.h"
#include "std/container_types.h"

struct UIWindow;
struct DragInfo;
struct ReplayBuffer;
struct UIWindowMgr;
struct DcBitmap;
struct Texture;
struct TexDrawPrim;
struct UIBitmapButton;

struct UIBackdropBox {
    int x, y, width, height;
    int draw_edge;
    DWORD argb;
    int edge_argb;
};
SIZE_ASSERT(UIBackdropBox, 0x1c);
OFFSET_ASSERT(UIBackdropBox, argb, 0x14);

#define UI_METHOD(ret, name, ...) ret (*name)(struct UIWindow *self, ##__VA_ARGS__)

enum UIWindowType {
    UI_TYPE_WINDOW = 0,
    UI_TYPE_FRAME = 1,
    UI_TYPE_EDIT = 2,
};

struct UIWindowOps {
    UI_METHOD(void, destroy);
    UI_METHOD(void, resize, int width, int height);
    BYTE is_frame;
    BYTE refreshes_children;
    UI_METHOD(void, move, int x, int y);
    UI_METHOD(BYTE, is_focusable);
    UI_METHOD(BYTE, get_backdrop_box, struct UIBackdropBox *out);
    UI_METHOD(BYTE, is_click_through);
    UI_METHOD(BYTE, is_hit_testable);
    UI_METHOD(void, drag_and_drop, int x, int y, const struct DragInfo *drag);
    UI_METHOD(void, store_info);
    UI_METHOD(void, save_pos);
    UI_METHOD(void, move_delta, int dx, int dy);
    UI_METHOD(void, set_show, int visible);
    UI_METHOD(void, on_create, int width, int height);
    UI_METHOD(void, on_destroy);
    UI_METHOD(void, on_process);
    UI_METHOD(void, on_draw);
    UI_METHOD(void, on_focus);
    UI_METHOD(void, on_focus_lost);
    UI_METHOD(void, on_l_btn_down, int x, int y);
    UI_METHOD(void, on_l_btn_dbl_clk, int x, int y);
    UI_METHOD(void, on_mouse_move, int x, int y);
    UI_METHOD(void, on_mouse_hover, int x, int y);
    UI_METHOD(void, track_mouse, int x, int y);
    UI_METHOD(void, on_mouse_shape, int x, int y);
    UI_METHOD(void, on_l_btn_up, int x, int y);
    UI_METHOD(void, on_r_btn_down, int x, int y);
    UI_METHOD(void, on_r_btn_up, int x, int y);
    UI_METHOD(void, on_wheel, int delta);
    UI_METHOD(void, snap);
    UI_METHOD(int, send_msg, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
    UI_METHOD(void, invalidate);
    UI_METHOD(BYTE, render, int force);
    BYTE has_custom_backdrop;
    UI_METHOD(void, draw_backdrop);
    UI_METHOD(int, get_records, struct ReplayBuffer *out);
    enum UIWindowType type;
    UI_METHOD(void, on_hover_enter);
    UI_METHOD(void, on_hover_leave);
    UI_METHOD(struct TexDrawPrim *, make_quad);
    UI_METHOD(struct UIWindow *, hit_test, int x, int y);
    UI_METHOD(void, render_overlay);
};

enum UIMsg {
    UI_MSG_DEFAULT_BUTTON = 0,
    UI_MSG_CANCEL_BUTTON = 1,
    UI_MSG_COMMAND = 6,
    UI_MSG_SET_INFO = 0x22,
    UI_MSG_LOAD_RECORDS = 0x7b,
};

enum UIControlId {
    UI_ID_OK = 0xb0,
    UI_ID_CANCEL = 0xb1,
    UI_ID_CLOSE = 0xc1,
    UI_ID_HELP = 0xc2,
    UI_ID_NONE = 0x239,
};

struct UIWindow {
    const struct UIWindowOps *ops;
    struct List draws;
    struct Texture *texture;
    struct UIWindow *parent;
    int width, height;
    int x, y;
    struct DcBitmap *canvas;
    int visible;
    int id;
    int state;
    int state_count;
    unsigned alpha_target;
    DWORD alpha_time;
    unsigned alpha;
    BYTE text_bk_on;
    int text_bk_colour;
    int list_rank;
    struct List children;
    int dirty;
    struct Vec quads;
    struct Vec sprite_quads;
    int serial;
    BYTE composites;
    BYTE redraws_tree;
};

#define UI_WINDOW(p) ((struct UIWindow *)(p))

static __inline void ui_window_destroy(struct UIWindow *self)
{
    self->ops->destroy(self);
}

static __inline void ui_window_resize(struct UIWindow *self, int width, int height)
{
    self->ops->resize(self, width, height);
}

static __inline void ui_window_move(struct UIWindow *self, int x, int y)
{
    self->ops->move(self, x, y);
}

static __inline BYTE ui_window_is_focusable(struct UIWindow *self)
{
    return self->ops->is_focusable(self);
}

static __inline BYTE ui_window_get_backdrop_box(struct UIWindow *self, struct UIBackdropBox *out)
{
    return self->ops->get_backdrop_box(self, out);
}

static __inline BYTE ui_window_is_click_through(struct UIWindow *self)
{
    return self->ops->is_click_through(self);
}

static __inline BYTE ui_window_is_hit_testable(struct UIWindow *self)
{
    return self->ops->is_hit_testable(self);
}

static __inline void ui_window_drag_and_drop(struct UIWindow *self, int x, int y, const struct DragInfo *drag)
{
    self->ops->drag_and_drop(self, x, y, drag);
}

static __inline void ui_window_store_info(struct UIWindow *self)
{
    self->ops->store_info(self);
}

static __inline void ui_window_set_show(struct UIWindow *self, int visible)
{
    self->ops->set_show(self, visible);
}

static __inline void ui_window_on_create(struct UIWindow *self, int width, int height)
{
    self->ops->on_create(self, width, height);
}

static __inline void ui_window_on_destroy(struct UIWindow *self)
{
    self->ops->on_destroy(self);
}

static __inline void ui_window_on_process(struct UIWindow *self)
{
    self->ops->on_process(self);
}

static __inline void ui_window_on_draw(struct UIWindow *self)
{
    self->ops->on_draw(self);
}

static __inline void ui_window_on_focus(struct UIWindow *self)
{
    self->ops->on_focus(self);
}

static __inline void ui_window_on_focus_lost(struct UIWindow *self)
{
    self->ops->on_focus_lost(self);
}

static __inline void ui_window_on_l_btn_down(struct UIWindow *self, int x, int y)
{
    self->ops->on_l_btn_down(self, x, y);
}

static __inline void ui_window_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y)
{
    self->ops->on_l_btn_dbl_clk(self, x, y);
}

static __inline void ui_window_on_mouse_move(struct UIWindow *self, int x, int y)
{
    self->ops->on_mouse_move(self, x, y);
}

static __inline void ui_window_on_mouse_hover(struct UIWindow *self, int x, int y)
{
    self->ops->on_mouse_hover(self, x, y);
}

static __inline void ui_window_track_mouse(struct UIWindow *self, int x, int y)
{
    self->ops->track_mouse(self, x, y);
}

static __inline void ui_window_on_mouse_shape(struct UIWindow *self, int x, int y)
{
    self->ops->on_mouse_shape(self, x, y);
}

static __inline void ui_window_on_l_btn_up(struct UIWindow *self, int x, int y)
{
    self->ops->on_l_btn_up(self, x, y);
}

static __inline void ui_window_on_r_btn_down(struct UIWindow *self, int x, int y)
{
    self->ops->on_r_btn_down(self, x, y);
}

static __inline void ui_window_on_r_btn_up(struct UIWindow *self, int x, int y)
{
    self->ops->on_r_btn_up(self, x, y);
}

static __inline void ui_window_on_wheel(struct UIWindow *self, int delta)
{
    self->ops->on_wheel(self, delta);
}

static __inline void ui_window_snap(struct UIWindow *self)
{
    self->ops->snap(self);
}

static __inline int ui_window_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4)
{
    return self->ops->send_msg(self, sender, msg, val1, val2, val3, val4);
}

static __inline void ui_window_invalidate(struct UIWindow *self)
{
    self->ops->invalidate(self);
}

static __inline BYTE ui_window_render(struct UIWindow *self, int force)
{
    return self->ops->render(self, force);
}

static __inline void ui_window_draw_backdrop(struct UIWindow *self)
{
    self->ops->draw_backdrop(self);
}

static __inline int ui_window_get_records(struct UIWindow *self, struct ReplayBuffer *out)
{
    return self->ops->get_records(self, out);
}

static __inline void ui_window_on_hover_enter(struct UIWindow *self)
{
    self->ops->on_hover_enter(self);
}

static __inline void ui_window_on_hover_leave(struct UIWindow *self)
{
    self->ops->on_hover_leave(self);
}

static __inline struct TexDrawPrim *ui_window_make_quad(struct UIWindow *self)
{
    return self->ops->make_quad(self);
}

static __inline struct UIWindow *ui_window_hit_test(struct UIWindow *self, int x, int y)
{
    return self->ops->hit_test(self, x, y);
}

static __inline void ui_window_render_overlay(struct UIWindow *self)
{
    self->ops->render_overlay(self);
}
OFFSET_ASSERT(UIWindow, parent, 0x10);
OFFSET_ASSERT(UIWindow, canvas, 0x24);
OFFSET_ASSERT(UIWindow, id, 0x2c);
OFFSET_ASSERT(UIWindow, alpha_target, 0x38);
OFFSET_ASSERT(UIWindow, children, 0x50);
OFFSET_ASSERT(UIWindow, quads, 0x5c);
OFFSET_ASSERT(UIWindow, serial, 0x74);
SIZE_ASSERT(UIWindow, 0x7c);

#endif
