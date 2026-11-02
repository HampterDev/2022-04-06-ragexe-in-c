#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_dispatch.h"
#include "world/mode_manager.h"
#include "ui/controls/controls.h"
#include "render/texture.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "diag/perf.h"
#include "platform/clock.h"

struct UIWindow;
const struct UIWindowOps ui_window_ops = {
    .destroy = ui_window_destroy_base,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_window_on_draw_base,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

void ui_window_destruct(struct UIWindow *self)
{
    struct UIWindow *w = self;
    struct ListNode *node;

    self->ops = &ui_window_ops;
    ui_window_on_destroy(self);
    if (w->canvas) {
        dc_bitmap_destroy(w->canvas);
        w->canvas = NULL;
    }
    for (node = w->children.head->next; node != w->children.head; node = node->next) {
        struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);

        if (child)
            ui_window_destroy(child);
    }
    list_clear(&w->children, NULL);

    ui_window_mgr_remove_from_name_waiting_list(&g_windowMgr, self);
    ui_window_mgr_forget_window(&g_windowMgr, self);
    vec_free(&w->sprite_quads);
    vec_free(&w->quads);
    list_free(&w->children, NULL);
    texture_destroy(self->texture);
    self->texture = NULL;
    list_free(&self->draws, NULL);
}

void ui_window_destroy_base(struct UIWindow *self)
{
    ui_window_destruct(self);
    mem_free(self);
}

void ui_window_resize_base(struct UIWindow *self, int width, int height)
{
    struct UIWindow *w = self;

    if (w->canvas)
        dc_bitmap_destroy(w->canvas);
    w->canvas = dc_bitmap_create(width, height);
    w->width = width;
    w->height = height;
    ui_window_invalidate(self);
}

void ui_window_move_base(struct UIWindow *self, int x, int y)
{
    self->x = x;
    self->y = y;
}

void ui_window_drag_and_drop_base(struct UIWindow *self, int x_unused, int y_unused, const struct DragInfo *drag_unused)
{
    mode_send_message(client_current_mode(), MODE_MSG_DRAG_END, 0, 0, 0, 0);
}

void ui_window_on_mouse_shape_base(struct UIWindow *self, int x_unused, int y_unused)
{
    mode_set_cursor_action(client_current_mode(), CURSOR_ARROW);
}

int ui_window_send_msg_base(struct UIWindow *self, struct UIWindow *sender_unused, int msg, int val1_unused,
                                int val2_unused, int val3_unused, int val4_unused)
{
    if ((msg == UI_MSG_DEFAULT_BUTTON || msg == UI_MSG_CANCEL_BUTTON) && self->parent)
        ui_window_send_msg(self->parent, self, msg, 0, 0, 0, 0);
    return 0;
}

DWORD ui_window_get_color(struct UIWindow *self)
{
    struct UIWindow *w = self;
    unsigned alpha = w->alpha;
    unsigned target = w->alpha_target;

    if (alpha < target) {
        if (target >= w->alpha + ((platform_ticks_ms() - w->alpha_time) >> 4))
            target = w->alpha + ((platform_ticks_ms() - w->alpha_time) >> 4);
        w->alpha = target;
        alpha = target;
    } else if (alpha > target) {
        if (target <= w->alpha - ((platform_ticks_ms() - w->alpha_time) >> 4))
            target = w->alpha - ((platform_ticks_ms() - w->alpha_time) >> 4);
        w->alpha = target;
        alpha = target;
    }
    if (alpha > 0xff) {
        alpha = w->alpha_target;
        w->alpha = alpha;
    }
    return (alpha << 24) + 0xffffff;
}

struct UIWindow *ui_window_hit_test_base(struct UIWindow *self, int x, int y)
{
    struct UIWindow *w = self;
    struct ListNode *node;
    struct UIWindow *hit;

    if (!ui_window_is_hit_testable(self) || !self->visible)
        return NULL;
    if (x < self->x || x >= self->x + self->width || y < self->y || y >= self->y + self->height)
        return NULL;
    node = w->children.head;
    if (node != node->next) {
        do {
            node = node->prev;
            hit = ui_window_hit_test(*LIST_VALUE(node, struct UIWindow *), x - self->x, y - self->y);
            if (hit)
                return hit;
        } while (node != w->children.head->next);
    }
    return self;
}

void ui_window_render_overlay_base(struct UIWindow *self)
{
    struct UIWindow *w = self;
    struct ListNode *node;

    for (node = w->children.head->next; node != w->children.head; node = node->next) {
        struct UIWindow *child = *LIST_VALUE(node, struct UIWindow *);

        if (child)
            ui_window_render_overlay(child);
    }
}

struct UIWindow *ui_window_get_ancestor(struct UIWindow *self)
{
    while (self->parent)
        self = self->parent;
    return self;
}

void ui_window_to_screen(struct UIWindow *self, int *x, int *y)
{
    struct UIWindow *p;

    *x += self->x;
    *y += self->y;
    for (p = self->parent; p; p = p->parent) {
        *x += p->x;
        *y += p->y;
    }
}

void ui_window_clear_canvas(struct UIWindow *self, DWORD colour)
{
    struct DcBitmap *canvas = self->canvas;

    if (canvas)
        dc_bitmap_fill(canvas, NULL, colour);
}

void ui_window_fill_canvas(struct UIWindow *self, DWORD colour, const RECT *rect)
{
    struct DcBitmap *canvas = self->canvas;

    if (canvas)
        dc_bitmap_fill(canvas, rect, colour);
}
