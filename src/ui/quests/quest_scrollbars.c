#include "ui/controls/scroll.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/quests/quests.h"
#include "diag/perf.h"

struct UIScrollBar;
struct UIWindow;
void ui_renew_quest_scroll_bar_destroy(struct UIWindow *window);
void ui_renew_quest_scroll_bar_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_renew_quest_scroll_bar_ops = {
    .destroy = ui_renew_quest_scroll_bar_destroy,
    .resize = ui_scroll_bar_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_scroll_bar_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_renew_quest_scroll_bar_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_scroll_bar_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_scroll_bar_on_l_btn_dbl_clk,
    .on_mouse_move = ui_scroll_bar_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_scroll_bar_on_mouse_shape,
    .on_l_btn_up = ui_scroll_bar_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_scroll_bar_on_r_btn_up,
    .on_wheel = ui_scroll_bar_on_wheel,
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

#define RENEW_BAR_DIR UI_SKIN "renew_questui\\"

struct UIScrollBar;
struct UIWindow;

static void bar_draw(struct UIScrollBar *bar, int x, int y, const char *bitmap)
{
    ui_window_draw_bitmap(&bar->base, x, y, ui_skin_bitmap(bitmap), 1);
}

static void bar_draw_thumb(struct UIScrollBar *bar, int top, const char *top_bmp, const char *mid_bmp,
                          const char *bottom_bmp)
{
    int bottom, y;

    bar_draw(bar, 0, top, top_bmp);
    bottom = top - 4 + ui_scroll_bar_get_thumb_size(bar);
    for (y = top + 4; y < bottom; y += 4)
        bar_draw(bar, 0, y, mid_bmp);
    bar_draw(bar, 0, bottom, bottom_bmp);
}

struct UIScrollBar *ui_renew_quest_scroll_bar_construct(struct UIScrollBar *self)
{
    ui_scroll_bar_construct(self);
    self->base.ops = &ui_renew_quest_scroll_bar_ops;
    self->button_size = 3;
    self->thickness = 7;
    return self;
}

static void ui_renew_quest_scroll_bar_destruct(struct UIScrollBar *self)
{
    self->base.ops = &ui_renew_quest_scroll_bar_ops;
    ui_scroll_bar_destruct(self);
}

void ui_renew_quest_scroll_bar_destroy(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    ui_renew_quest_scroll_bar_destruct(self);
    mem_free(self);
}

void ui_renew_quest_scroll_bar_on_draw(struct UIWindow *window)
{
    struct UIScrollBar *self = (struct UIScrollBar *)window;
    ui_window_clear_canvas(&self->base, 0xffff00ff);
    if (!self->max_pos)
        return;
    bar_draw(self, 1, 0, RENEW_BAR_DIR "bt_scrollbar_arrow_upper.bmp");
    bar_draw(self, 1, self->base.height - self->button_size - 1, RENEW_BAR_DIR "bt_scrollbar_arrow_down.bmp");
    bar_draw_thumb(self, ui_scroll_bar_thumb_pos(self) + self->button_size, RENEW_BAR_DIR "bt_scrollBar_up.bmp",
                  RENEW_BAR_DIR "bt_scrollBar_up.bmp", RENEW_BAR_DIR "bt_scrollBar_down.bmp");
}
