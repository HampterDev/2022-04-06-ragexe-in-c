#include "ui/controls/popups.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "ui/window.h"
#include "ui/draw.h"
#include "diag/perf.h"

struct Vec;
struct UIBackdropBox;
struct UITransMenuWnd;
struct UIWindow;
const struct UIWindowOps ui_trans_menu_wnd_ops = {
    .destroy = ui_trans_menu_wnd_destroy,
    .resize = ui_trans_menu_wnd_resize,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_trans_menu_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_trans_menu_wnd_get_backdrop_box,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_trans_menu_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_trans_menu_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_trans_menu_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_trans_menu_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_trans_menu_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define TRANS_MENU_MSG_CHOSEN 0x30
#define TRANS_MENU_PANEL_ARGB 0x66000000
#define TRANS_MENU_TEXT_ENABLED 0xffffff
#define TRANS_MENU_TEXT_DISABLED 0x646464
#define TRANS_MENU_TEXT_HOVER 0x282828
#define TRANS_MENU_HOVER_FILL 0xc8c8c8

struct TransMenuItem {
    char text[0x18];
    int value;
    BYTE enabled;
};

struct UITransMenuWnd {
    struct UIFrameWnd frame;
    struct Vec items;
    struct UIBackdropBox panel;
    int font_height;
    int margin;
    int hover;
};

#define ITEM_COUNT(self) VEC_COUNT(&(self)->items, struct TransMenuItem)
#define ITEM_AT(self, i) (((struct TransMenuItem *)(self)->items.first)[i])

struct UITransMenuWnd *ui_trans_menu_wnd_construct(struct UITransMenuWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_trans_menu_wnd_ops;
    self->items.first = self->items.last = self->items.end = NULL;
    self->margin = 5;
    self->font_height = 0xc;
    self->hover = -1;
    vec_reserve(&self->items, 8, sizeof(struct TransMenuItem));
    return self;
}

struct UIWindow *ui_trans_menu_wnd_new(void)
{
    return (struct UIWindow *)ui_trans_menu_wnd_construct(mem_alloc(sizeof(struct UITransMenuWnd)));
}

static void ui_trans_menu_wnd_destruct(struct UITransMenuWnd *self)
{
    self->frame.base.ops = &ui_trans_menu_wnd_ops;
    vec_free(&self->items);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_trans_menu_wnd_destroy(struct UIWindow *window)
{
    struct UITransMenuWnd *self = (struct UITransMenuWnd *)window;
    ui_trans_menu_wnd_destruct(self);
    mem_free(self);
}

void ui_trans_menu_wnd_resize(struct UIWindow *window, int width, int height)
{
    struct UITransMenuWnd *self = (struct UITransMenuWnd *)window;
    ui_window_resize_base(&self->frame.base, width, height);
    self->panel.width = self->frame.base.width;
    self->panel.height = self->frame.base.height;
}

void ui_trans_menu_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UITransMenuWnd *self = (struct UITransMenuWnd *)window;
    ui_window_move_base(&self->frame.base, x, y);
    self->panel.x = self->frame.base.x;
    self->panel.y = self->frame.base.y;
}

BYTE ui_trans_menu_wnd_get_backdrop_box(struct UIWindow *window, struct UIBackdropBox *out)
{
    struct UITransMenuWnd *self = (struct UITransMenuWnd *)window;

    *out = self->panel;
    return 1;
}

void ui_trans_menu_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UITransMenuWnd *self = (struct UITransMenuWnd *)window;
    self->panel.x = self->frame.base.x;
    self->panel.y = self->frame.base.y;
    self->panel.width = self->frame.base.width;
    self->panel.draw_edge = 0;
    self->panel.height = self->frame.base.height;
    self->panel.argb = TRANS_MENU_PANEL_ARGB;
    self->panel.edge_argb = 0;
}

void ui_trans_menu_wnd_on_draw(struct UIWindow *window)
{
    struct UITransMenuWnd *self = (struct UITransMenuWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    unsigned count, i;
    int row_height, top = 0;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    count = ITEM_COUNT(self);
    if (!count)
        return;
    row_height = (unsigned)(wnd->height - self->margin * 2) / count;
    for (i = 0; i < ITEM_COUNT(self); i++) {
        COLORREF colour = ITEM_AT(self, i).enabled ? TRANS_MENU_TEXT_ENABLED : TRANS_MENU_TEXT_DISABLED;

        if ((int)i == self->hover) {
            int half = self->margin / 2;

            colour = TRANS_MENU_TEXT_HOVER;
            ui_window_fill_rect(wnd, 0, ((int)i - 1) * half + top + self->margin, wnd->width, row_height + half,
                              TRANS_MENU_HOVER_FILL);
        }
        ui_window_text_out(wnd, self->margin, (self->margin / 2 + row_height) * (int)i + self->margin,
                         ITEM_AT(self, i).text, 0, 0, self->font_height, colour, 0, 0);
        top += row_height;
    }
}

void ui_trans_menu_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UITransMenuWnd *self = (struct UITransMenuWnd *)window;
    struct TransMenuItem *item;

    if (self->hover == -1)
        return;
    item = &ITEM_AT(self, self->hover);
    if (item->enabled)
        ui_trans_menu_wnd_send_msg(&self->frame.base, NULL, TRANS_MENU_MSG_CHOSEN, item->value, 0, 0, 0);
}

void ui_trans_menu_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UITransMenuWnd *self = (struct UITransMenuWnd *)window;
    int margin = self->margin;
    int old = self->hover;
    int row = -1;

    if (margin <= y) {
        unsigned count = ITEM_COUNT(self);

        if ((unsigned)((margin / 2 + self->font_height) * (int)count + margin) >= (unsigned)y) {
            int row_height = (unsigned)(self->frame.base.height - margin * 2) / count;

            row = (y - margin) / row_height;
        }
    }
    self->hover = row;
    if (old != row && row != -1)
        ui_window_invalidate_base(&self->frame.base);
}

intptr_t ui_trans_menu_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                      intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UITransMenuWnd *self = (struct UITransMenuWnd *)window;
    if (msg != TRANS_MENU_MSG_CHOSEN)
        ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
    return 0;
}
