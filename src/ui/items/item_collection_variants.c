#include "ui/controls/buttons.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "ui/items/item_collection_internal.h"
#include "diag/perf.h"

void ui_item_collection_comparison_wnd_destroy(struct UIWindow *window);
void ui_item_collection_comparison_wnd_move(struct UIWindow *self, int x, int y);
void ui_item_collection_comparison_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_item_collection_comparison_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_item_collection_comparison_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
void ui_item_collection_comparison_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
intptr_t ui_item_collection_comparison_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_item_collection_comparison_wnd_ops = {
    .destroy = ui_item_collection_comparison_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_item_collection_comparison_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_item_collection_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_collection_comparison_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_collection_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_collection_comparison_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_collection_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_item_collection_comparison_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_collection_comparison_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_collection_comparison_wnd_send_msg,
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
void ui_combined_card_item_collection_wnd_destroy(struct UIWindow *window);
void ui_combined_card_item_collection_wnd_move(struct UIWindow *self, int x, int y);
void ui_combined_card_item_collection_wnd_store_info(struct UIWindow *self);
void ui_combined_card_item_collection_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_combined_card_item_collection_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
intptr_t ui_combined_card_item_collection_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_combined_card_item_collection_wnd_ops = {
    .destroy = ui_combined_card_item_collection_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_combined_card_item_collection_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_combined_card_item_collection_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_combined_card_item_collection_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_collection_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_collection_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_collection_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_combined_card_item_collection_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_collection_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_combined_card_item_collection_wnd_send_msg,
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
void ui_combined_card_item_collection_comparison_wnd_destroy(struct UIWindow *window);
void ui_combined_card_item_collection_comparison_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_combined_card_item_collection_comparison_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_combined_card_item_collection_comparison_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
intptr_t ui_combined_card_item_collection_comparison_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_combined_card_item_collection_comparison_wnd_ops = {
    .destroy = ui_combined_card_item_collection_comparison_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_item_collection_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_item_collection_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_combined_card_item_collection_comparison_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_collection_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_combined_card_item_collection_comparison_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_collection_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_combined_card_item_collection_comparison_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_collection_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_combined_card_item_collection_comparison_wnd_send_msg,
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

struct UIItemCollectionWnd *ui_item_collection_comparison_wnd_construct(struct UIItemCollectionWnd *self)
{
    ui_item_collection_wnd_construct(self);
    self->frame.base.ops = &ui_item_collection_comparison_wnd_ops;
    return self;
}

struct UIWindow *ui_item_collection_comparison_wnd_new(void)
{
    return (struct UIWindow *)ui_item_collection_comparison_wnd_construct(mem_alloc(sizeof(struct UIItemCollectionWnd)));
}

void ui_item_collection_comparison_wnd_destruct(struct UIItemCollectionWnd *self)
{
    self->frame.base.ops = &ui_item_collection_comparison_wnd_ops;
    ui_item_collection_wnd_destruct(self);
}

void ui_item_collection_comparison_wnd_destroy(struct UIWindow *window)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    ui_item_collection_comparison_wnd_destruct(self);
    mem_free(self);
}

void ui_item_collection_comparison_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    ui_item_collection_wnd_on_create((struct UIWindow *)(self), width, height);
    ui_button_show((struct UIButton *)(self->close_btn), 0);
    self->wnd_id = WND_ITEM_COLLECTION_CMP;
    UI_TEXT_VIEWER_BACKGROUND(self->text) = ui_skin_argb(0x1e, 6);
    ui_window_invalidate(self->text);
}

void ui_item_collection_comparison_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    struct ItemInfo card;
    int slot_x, slot_y, slot;

    slot = item_collection_card_at(self, x, y, &slot_x, &slot_y);
    if (slot == -1)
        return;
    item_collection_card_item(&card, self->item.slot[slot]);
    item_collection_show_card(WND_COMBINED_CARD_COLLECTION, &card,
                         slot >= item_info_get_slot_count(&self->item) ? 1 : 0);
    item_info_destruct(&card);
}

intptr_t ui_item_collection_comparison_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                                      int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    if (msg != UI_MSG_SET_INFO)
        return ui_item_collection_wnd_send_msg((struct UIWindow *)(self), sender, msg, val1, val2, val3, val4);
    item_collection_place_left_of(COLLECTION_WND(self), WND_ITEM_COLLECTION);
    return 0;
}

struct UICombinedCardItemCollectionWnd *
ui_combined_card_item_collection_wnd_construct(struct UICombinedCardItemCollectionWnd *self)
{
    ui_item_collection_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_combined_card_item_collection_wnd_ops;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_combined_card_item_collection_wnd_new(void)
{
    return (struct UIWindow *)ui_combined_card_item_collection_wnd_construct(mem_alloc(sizeof(struct UICombinedCardItemCollectionWnd)));
}

void ui_combined_card_item_collection_wnd_destruct(struct UICombinedCardItemCollectionWnd *self)
{
    self->base.frame.base.ops = &ui_combined_card_item_collection_wnd_ops;
    ui_window_mgr_post_quit1(&g_windowMgr, WND_COMBINED_CARD_COLLECTION_CMP);
    ui_item_collection_wnd_destruct(&self->base);
}

void ui_combined_card_item_collection_wnd_destroy(struct UIWindow *window)
{
    struct UICombinedCardItemCollectionWnd *self = (struct UICombinedCardItemCollectionWnd *)window;
    ui_combined_card_item_collection_wnd_destruct(self);
    mem_free(self);
}

void ui_combined_card_item_collection_wnd_move(struct UIWindow *window, int x, int y)
{
    struct UICombinedCardItemCollectionWnd *self = (struct UICombinedCardItemCollectionWnd *)window;
    ui_frame_wnd_move((struct UIWindow *)(&self->base.frame), x, y);
    if (g_windowMgr.combined_card_item_collection_comparison_wnd)
        item_collection_place_left_of(g_windowMgr.combined_card_item_collection_comparison_wnd, WND_COMBINED_CARD_COLLECTION);
}

void ui_combined_card_item_collection_wnd_store_info(struct UIWindow *window)
{
    struct UICombinedCardItemCollectionWnd *self = (struct UICombinedCardItemCollectionWnd *)window;
    if (!self->info)
        return;
    self->info->x = self->base.frame.base.x;
    self->info->y = self->base.frame.base.y;
    self->info->width = self->base.frame.base.width;
    self->info->height = self->base.frame.base.height;
}

void ui_combined_card_item_collection_wnd_on_create(struct UIWindow *window, int width,
                                                         int height)
{
    struct UICombinedCardItemCollectionWnd *self = (struct UICombinedCardItemCollectionWnd *)window;
    ui_item_collection_wnd_on_create((struct UIWindow *)(&self->base), width, height);
    self->base.wnd_id = WND_COMBINED_CARD_COLLECTION;
}

void ui_combined_card_item_collection_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UICombinedCardItemCollectionWnd *self = (struct UICombinedCardItemCollectionWnd *)window;
    ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->base.frame), x, y);
    if (g_windowMgr.combined_card_item_collection_comparison_wnd) {
        g_windowMgr.combined_card_item_collection_comparison_wnd->alpha_target = self->base.frame.base.alpha_target;
        g_windowMgr.combined_card_item_collection_comparison_wnd->alpha_time = self->base.frame.base.alpha_time;
    }
}

intptr_t ui_combined_card_item_collection_wnd_send_msg(struct UIWindow *window,
                                                       struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3,
                                                       intptr_t val4)
{
    struct UICombinedCardItemCollectionWnd *self = (struct UICombinedCardItemCollectionWnd *)window;
    if (msg != UI_MSG_SET_INFO)
        return ui_item_collection_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
    self->info = (struct WindowInfo *)val1;
    item_collection_take_info(&self->base.frame.base, self->info, 0x116);
    return 0;
}

struct UIItemCollectionWnd *
ui_combined_card_item_collection_comparison_wnd_construct(struct UIItemCollectionWnd *self)
{
    ui_item_collection_wnd_construct(self);
    self->frame.base.ops = &ui_combined_card_item_collection_comparison_wnd_ops;
    return self;
}

struct UIWindow *ui_combined_card_item_collection_comparison_wnd_new(void)
{
    return (struct UIWindow *)ui_combined_card_item_collection_comparison_wnd_construct(mem_alloc(sizeof(struct UIItemCollectionWnd)));
}

void ui_combined_card_item_collection_comparison_wnd_destruct(struct UIItemCollectionWnd *self)
{
    self->frame.base.ops = &ui_combined_card_item_collection_comparison_wnd_ops;
    ui_item_collection_wnd_destruct(self);
}

void ui_combined_card_item_collection_comparison_wnd_destroy(struct UIWindow *window)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    ui_combined_card_item_collection_comparison_wnd_destruct(self);
    mem_free(self);
}

void ui_combined_card_item_collection_comparison_wnd_on_create(struct UIWindow *window, int width,
                                                                    int height)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    ui_item_collection_wnd_on_create((struct UIWindow *)(self), width, height);
    ui_button_show((struct UIButton *)(self->close_btn), 0);
    self->wnd_id = WND_COMBINED_CARD_COLLECTION_CMP;
    UI_TEXT_VIEWER_BACKGROUND(self->text) = 0xfff7dfc6;
    ui_window_invalidate(self->text);
}

intptr_t ui_combined_card_item_collection_comparison_wnd_send_msg(struct UIWindow *window,
                                                                  struct UIWindow *sender, int msg, intptr_t val1,
                                                                  intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIItemCollectionWnd *self = (struct UIItemCollectionWnd *)window;
    if (msg != UI_MSG_SET_INFO)
        return ui_item_collection_wnd_send_msg((struct UIWindow *)(self), sender, msg, val1, val2, val3, val4);
    item_collection_place_left_of(COLLECTION_WND(self), WND_COMBINED_CARD_COLLECTION);
    return 0;
}

void ui_item_collection_comparison_wnd_move(struct UIWindow *self, int x, int y)
{
}

void ui_item_collection_comparison_wnd_on_l_btn_down(struct UIWindow *self, int x, int y)
{
}

void ui_item_collection_comparison_wnd_on_l_btn_up(struct UIWindow *self, int x, int y)
{
}

void ui_combined_card_item_collection_comparison_wnd_on_l_btn_down(struct UIWindow *self, int x, int y)
{
}

void ui_combined_card_item_collection_comparison_wnd_on_l_btn_up(struct UIWindow *self, int x, int y)
{
}
