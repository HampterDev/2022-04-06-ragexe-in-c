#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/shops/shops.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "platform/clock.h"

#define MSG_ITEM_DEAL_LOCKED 0xb87
#define ITEM_DEAL_LOCK_LIFETIME 2000

struct UINoticeMessageBox_LockSellItem;
struct UIWindow;
void ui_notice_message_box_lock_sell_item_destroy(struct UIWindow *window);
void ui_notice_message_box_lock_sell_item_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_notice_message_box_lock_sell_item_on_process(struct UIWindow *self);
void ui_notice_message_box_lock_sell_item_on_draw(struct UIWindow *self);
intptr_t ui_notice_message_box_lock_sell_item_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_notice_message_box_lock_sell_item_ops = {
    .destroy = ui_notice_message_box_lock_sell_item_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_notice_message_box_lock_sell_item_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_notice_message_box_lock_sell_item_on_process,
    .on_draw = ui_notice_message_box_lock_sell_item_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_notice_message_box_lock_sell_item_send_msg,
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

struct UINoticeMessageBox_LockSellItem {
    struct UIFrameWnd frame;
    DWORD opened_at;
    DWORD lifetime;
};

static struct UINoticeMessageBox_LockSellItem *ui_notice_message_box_lock_sell_item_construct(struct UINoticeMessageBox_LockSellItem *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_notice_message_box_lock_sell_item_ops;
    self->opened_at = platform_ticks_ms();
    self->lifetime = ITEM_DEAL_LOCK_LIFETIME;
    return self;
}

struct UINoticeMessageBox_LockSellItem *ui_notice_message_box_lock_sell_item_new(void)
{
    struct UINoticeMessageBox_LockSellItem *self = mem_alloc(sizeof *self);

    return self ? ui_notice_message_box_lock_sell_item_construct(self) : NULL;
}

void ui_notice_message_box_lock_sell_item_destruct(struct UINoticeMessageBox_LockSellItem *self)
{
    self->frame.base.ops = &ui_notice_message_box_lock_sell_item_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_notice_message_box_lock_sell_item_destroy(struct UIWindow *window)
{
    struct UINoticeMessageBox_LockSellItem *self = (struct UINoticeMessageBox_LockSellItem *)window;
    ui_notice_message_box_lock_sell_item_destruct(self);
    mem_free(self);
}

void ui_notice_message_box_lock_sell_item_on_create(struct UIWindow *window,
                                                         int width_unused, int height_unused)
{
    struct UINoticeMessageBox_LockSellItem *self = (struct UINoticeMessageBox_LockSellItem *)window;
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, UI_SKIN "inventory\\bt_close.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, UI_SKIN "inventory\\bt_close.bmp", UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, UI_SKIN "inventory\\bt_close.bmp", UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, 0xe5, 4);
    button->button.base.id = UI_ID_CLOSE;
    ui_window_add_child(&self->frame.base, &button->button.base);
}

void ui_notice_message_box_lock_sell_item_on_process(struct UIWindow *window)
{
    struct UINoticeMessageBox_LockSellItem *self = (struct UINoticeMessageBox_LockSellItem *)window;
    if (self->opened_at + self->lifetime < platform_ticks_ms())
        ui_notice_message_box_lock_sell_item_send_msg(&self->frame.base, &self->frame.base, UI_MSG_COMMAND, UI_ID_CLOSE, 0, 0, 0);
}

void ui_notice_message_box_lock_sell_item_on_draw(struct UIWindow *window)
{
    struct UINoticeMessageBox_LockSellItem *self = (struct UINoticeMessageBox_LockSellItem *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_draw_bitmap(wnd, 0, 0,
                        res_mgr_get(g_resMgr(), UI_SKIN "inventory\\popup_itemDeal_lock.bmp"), 0);
    ui_window_text_out_centred(wnd, 0, 5, msg_str(MSG_ITEM_DEAL_LOCKED), wnd->width, 0, 12, 0xffffff, 0);
}

intptr_t ui_notice_message_box_lock_sell_item_send_msg(struct UIWindow *window,
                                                       struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                                       intptr_t val3, intptr_t val4)
{
    struct UINoticeMessageBox_LockSellItem *self = (struct UINoticeMessageBox_LockSellItem *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (msg != UI_MSG_COMMAND) {
        ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
        return 0;
    }
    if (val1 == UI_ID_CLOSE && wnd->parent)
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, UI_ID_CLOSE, 0, 0, 0);
    return 0;
}
