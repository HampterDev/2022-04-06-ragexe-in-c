#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/items/item_collection_internal.h"
#include "diag/perf.h"
#include "ui/wndmgr/wndmgr.h"

struct UIWindow;
void ui_item_move_info_destroy(struct UIWindow *window);
void ui_item_move_info_on_draw(struct UIWindow *self);
void ui_item_move_info_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_item_move_info_on_hover_leave(struct UIWindow *self);
const struct UIButtonOps ui_item_move_info_ops = {
    .base = {
        .destroy = ui_item_move_info_destroy,
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
        .on_draw = ui_item_move_info_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_text_button_on_mouse_move,
        .on_mouse_hover = ui_item_move_info_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_text_button_on_l_btn_up,
        .on_r_btn_down = ui_text_button_on_r_btn_down,
        .on_r_btn_up = ui_text_button_on_r_btn_up,
        .on_wheel = ui_text_button_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_window_send_msg_base,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .get_records = ui_window_get_records_base,
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_item_move_info_on_hover_leave,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_show = ui_button_set_show,
};

struct UIItemMoveInfo *ui_item_move_info_construct(struct UIItemMoveInfo *self)
{
    ui_text_button_construct(&self->button);
    self->button.button.base.ops = &ui_item_move_info_ops.base;
    item_info_construct(&self->item);
    return self;
}

void ui_item_move_info_destroy(struct UIWindow *window)
{
    struct UIItemMoveInfo *self = (struct UIItemMoveInfo *)window;
    self->button.button.base.ops = &ui_item_move_info_ops.base;
    item_info_destruct(&self->item);
    ui_text_button_destruct(&self->button);
    mem_free(self);
}

void ui_item_move_info_on_draw(struct UIWindow *window)
{
    struct UIItemMoveInfo *self = (struct UIItemMoveInfo *)window;
    ui_text_button_on_draw((struct UIWindow *)(&self->button));
}

void ui_item_move_info_on_hover_leave(struct UIWindow *window)
{
    struct UIItemMoveInfo *self = (struct UIItemMoveInfo *)window;
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ITEM_MOVE_INFO, 0);
    ui_window_invalidate_base(&self->button.button.base);
}

#define WND_ITEM_MOVE_INFO 0x114
#define UI_MSG_MOVE_INFO_ITEM 0x93
void ui_item_move_info_on_mouse_hover(struct UIWindow *self, int x, int y)
{
    struct UIWindow *wnd = ui_window_open(WND_ITEM_MOVE_INFO);

    ui_window_send(wnd, NULL, UI_MSG_MOVE_INFO_ITEM, ((struct UIItemMoveInfo *)self)->item_id, 0, 0, 0);
}
