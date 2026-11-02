#include "ui/controls/buttons.h"
#include "navi/navi_manager.h"
#include "core/object_type_tables.h"
#include "ui/chat/navi_text.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/ui_std.h"
#include <stdlib.h>
#include <string.h>
#include "diag/perf.h"

void ui_navi_link_button_destroy(struct UIWindow *window);
void ui_navi_link_button_on_l_btn_down(struct UIWindow *self, int x, int y);
const struct UIButtonOps ui_navi_link_button_ops = {
    .base = {
        .destroy = ui_navi_link_button_destroy,
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
        .on_draw = ui_text_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_navi_link_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_text_button_on_mouse_move,
        .on_mouse_hover = ui_text_button_on_mouse_hover,
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
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_show = ui_button_set_show,
};
static void navi_link_button_destruct(struct UINaviLinkButton *self)
{
    self->text.button.base.ops = &ui_navi_link_button_ops.base;
    string_free(&self->link);
    ui_text_button_destruct(&self->text);
}

void ui_navi_link_button_destroy(struct UIWindow *window)
{
    struct UINaviLinkButton *self = (struct UINaviLinkButton *)window;
    navi_link_button_destruct(self);
    free(self);
}

void ui_navi_link_button_activate(struct UINaviLinkButton *self)
{
    struct TextView link = string_view(&self->link);
    struct TextView map;
    struct String name = {0};
    size_t close = text_view_find(link, text_view("</NAVIL>", 8), 0);
    int x, y;

    if (close == (size_t)-1 || close < 4) return;
    x = navi_links_base62(text_view(link.data + close - 4, 2));
    y = navi_links_base62(text_view(link.data + close - 2, 2));
    map = text_view(link.data, close - 4);
    if (text_view_find(map, text_view("<NAVIL>", 7), 0) != STRING_NPOS)
        map = text_view(map.data + 7, map.size - 7);
    if (map.size) {
        string_assign(&name, map.data, map.size);
        navi_show_map(&g_navi, string_cstr(&name), x, y);
    }
    string_free(&name);
}

void ui_navi_link_button_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UINaviLinkButton *self = (struct UINaviLinkButton *)window;
    ui_navi_link_button_activate(self);
}

struct UINaviLinkButton *ui_navi_link_button_construct(struct UINaviLinkButton *self)
{
    ui_text_button_construct(&self->text);
    self->text.button.base.ops = &ui_navi_link_button_ops.base;
    string_init(&self->link);
    return self;
}

struct UINaviLinkButton *ui_navi_link_button_create(void)
{
    struct UINaviLinkButton *self = malloc(sizeof *self);
    return self ? ui_navi_link_button_construct(self) : NULL;
}
