#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "ui/character/gauge_internal.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "ui/character/character.h"
#include "ui/character/character_std.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include <stdint.h>

struct UIBarGraph3;
struct UIWindow;
void ui_bar_graph3_destroy(struct UIWindow *window);
const struct UIWindowOps ui_bar_graph3_ops = {
    .destroy = ui_bar_graph3_destroy,
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
    .on_draw = ui_window_abstract_on_draw,
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

struct UIBarGraph4;
void ui_bar_graph4_destroy(struct UIWindow *window);
void ui_bar_graph4_on_draw(struct UIWindow *self);
void ui_bar_graph4_on_mouse_hover(struct UIWindow *self, int arg1_unused, int arg2_unused);
const struct UIWindowOps ui_bar_graph4_ops = {
    .destroy = ui_bar_graph4_destroy,
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
    .on_draw = ui_bar_graph4_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_bar_graph4_on_mouse_hover,
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

struct UIBarGraph;
struct UIWindow;
void ui_bar_graph_destroy(struct UIWindow *window);
void ui_bar_graph_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_bar_graph_ops = {
    .destroy = ui_bar_graph_destroy,
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
    .on_draw = ui_bar_graph_on_draw,
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
struct UIBarGraph2;
void ui_bar_graph2_destroy(struct UIWindow *window);
void ui_bar_graph2_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_bar_graph2_ops = {
    .destroy = ui_bar_graph2_destroy,
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
    .on_draw = ui_bar_graph2_on_draw,
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
struct UIINT64BarGraph;
void uiint64_bar_graph_destroy(struct UIWindow *window);
void uiint64_bar_graph_on_draw(struct UIWindow *self);
void uiint64_bar_graph_slot25(struct UIWindow *self, int arg1_unused, int arg2_unused);
const struct UIWindowOps uiint64_bar_graph_ops = {
    .destroy = uiint64_bar_graph_destroy,
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
    .on_draw = uiint64_bar_graph_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = uiint64_bar_graph_slot25,
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
struct UIWindow;
void ui_bar_graph_player_destroy(struct UIWindow *window);
void ui_bar_graph_player_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_bar_graph_player_ops = {
    .destroy = ui_bar_graph_player_destroy,
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
    .on_draw = ui_bar_graph_player_on_draw,
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
void ui_bar_graph_player_ap_destroy(struct UIWindow *window);
void ui_bar_graph_player_ap_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_bar_graph_player_ap_ops = {
    .destroy = ui_bar_graph_player_ap_destroy,
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
    .on_draw = ui_bar_graph_player_ap_on_draw,
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

#define GAUGE_LOW_PERCENT 25.0f
#define GAUGE_RED RGB(0xff, 0, 0)

static COLORREF chip_colour(int x, int y)
{
    int r, g, b;

    ui_skin_color_chip(&g_skinMgr, x, y, &r, &g, &b);
    return RGB((BYTE)r, (BYTE)g, (BYTE)b);
}

static void draw_chip_background(struct UIWindow *wnd)
{
    int r, g, b;

    ui_skin_color_chip(&g_skinMgr, 0x1a, 2, &r, &g, &b);
    ui_window_clear_canvas(wnd, RGB((BYTE)r, (BYTE)g, (BYTE)g));
    ui_window_fill_rect(wnd, 1, 1, wnd->width - 2, wnd->height - 2, chip_colour(0x1e, 2));
}

static void draw_gze_bar(struct UIWindow *wnd, const char *colour, int filled)
{
    char path[0x80];
    void *mid;
    int i;

    sprintf(path, UI_SKIN "basic_interface\\gze%s_left.bmp", colour);
    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(path), 1);
    sprintf(path, UI_SKIN "basic_interface\\gze%s_mid.bmp", colour);
    mid = ui_skin_bitmap(path);
    for (i = 0; i < filled; i++)
        ui_window_draw_bitmap(wnd, i + 4, 0, mid, 1);
    sprintf(path, UI_SKIN "basic_interface\\gze%s_right.bmp", colour);
    ui_window_draw_bitmap(wnd, filled + 4, 0, ui_skin_bitmap(path), 1);
}

static void show_percent_balloon(struct UIWindow *wnd, int per_mille)
{
    char text[0x104];
    int x = 0, y = -16;

    ui_window_to_screen(wnd, &x, &y);
    memset(text, 0, sizeof text);
    sprintf(text, "%.1f %%", (double)((float)per_mille * 0.1f));
    mode_show_tooltip(g_modeMgr.curMode, text, x, y, 0xffffffff, 0);
}

void ui_bar_graph_destruct(struct UIBarGraph *self)
{
    self->base.ops = &ui_bar_graph_ops;
    ui_window_destruct(&self->base);
}

void ui_bar_graph_destroy(struct UIWindow *window)
{
    struct UIBarGraph *self = (struct UIBarGraph *)window;
    ui_bar_graph_destruct(self);
    mem_free(self);
}

void ui_bar_graph_set_pos(struct UIWindow *self, float pos)
{
    ((struct UIBarGraph *)self)->pos = pos;
    ui_window_invalidate(self);
}

void ui_bar_graph_on_draw(struct UIWindow *window)
{
    struct UIBarGraph *self = (struct UIBarGraph *)window;
    struct UIWindow *wnd = &self->base;

    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\gze_bg.bmp"), 0);
    if (self->pos == 0.0f)
        return;
    draw_gze_bar(wnd, self->pos < GAUGE_LOW_PERCENT ? "red" : "blue", (wnd->width - 8) * (int)self->pos / 100);
}

struct UIWindow *ui_bar_graph2_construct(struct UIWindow *wnd)
{
    struct UIBarGraph2 *self = (struct UIBarGraph2 *)wnd;

    ui_window_construct(wnd, 0);
    wnd->ops = &ui_bar_graph_ops;
    self->graph.pos = 0.0f;
    wnd->ops = &ui_bar_graph2_ops;
    self->vertical = 0;
    self->back_red = 0x8e;
    self->back_green = 0x8e;
    self->back_blue = 0x8e;
    self->bar_red = 0x2a;
    self->bar_green = 0x4b;
    self->bar_blue = 0x8f;
    return wnd;
}

void ui_bar_graph2_destroy(struct UIWindow *window)
{
    struct UIBarGraph2 *self = (struct UIBarGraph2 *)window;
    self->graph.base.ops = &ui_bar_graph2_ops;
    ui_bar_graph_destruct(&self->graph);
    mem_free(self);
}

void ui_bar_graph2_on_draw(struct UIWindow *window)
{
    struct UIBarGraph2 *self = (struct UIBarGraph2 *)window;
    struct UIWindow *wnd = &self->graph.base;
    COLORREF back = RGB((BYTE)self->back_red, (BYTE)self->back_green, (BYTE)self->back_blue);
    COLORREF bar = RGB((BYTE)self->bar_red, (BYTE)self->bar_green, (BYTE)self->bar_blue);
    char text[12];

    if (self->vertical) {
        int height = wnd->height, filled;

        ui_window_clear_canvas(wnd, back);
        if (self->graph.pos == 0.0f)
            return;
        filled = (int)((float)height * self->graph.pos / 100.0f);
        ui_window_fill_rect(wnd, 0, height - filled, wnd->width, filled, bar);
        return;
    }
    ui_window_fill_rect(wnd, 0, 0, wnd->width, wnd->height, 0xffff00);
    ui_window_fill_rect(wnd, 1, 1, wnd->width - 2, wnd->height - 2, back);
    if (self->graph.pos != 0.0f)
        ui_window_fill_rect(wnd, 1, 1, (int)((float)(wnd->width - 2) * self->graph.pos / 100.0f), wnd->height - 2,
                          bar);
    sprintf(text, "%3ld%%", (long)self->graph.pos);
    ui_window_text_out(wnd, (wnd->width >> 1) - 0x14, (wnd->height >> 1) - 7, text, 0, 1, 16, 0xffff, 0, 0);
}

struct UIBarGraph3 *ui_bar_graph3_construct(struct UIBarGraph3 *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_bar_graph3_ops;
    self->current = NULL;
    self->maximum = NULL;
    return self;
}

void ui_bar_graph3_set_values(struct UIBarGraph3 *self, int *current, int *maximum)
{
    self->current = current;
    self->maximum = maximum;
}

void ui_bar_graph3_destruct(struct UIBarGraph3 *self)
{
    self->base.ops = &ui_bar_graph3_ops;
    ui_window_destruct(&self->base);
}

void ui_bar_graph3_destroy(struct UIWindow *window)
{
    struct UIBarGraph3 *self = (struct UIBarGraph3 *)window;
    ui_bar_graph3_destruct(self);
    mem_free(self);
}

void ui_bar_graph4_destroy(struct UIWindow *window)
{
    struct UIBarGraph4 *self = (struct UIBarGraph4 *)window;
    self->graph.base.ops = &ui_bar_graph4_ops;
    ui_bar_graph3_destruct(&self->graph);
    mem_free(self);
}

void ui_bar_graph4_on_draw(struct UIWindow *window)
{
    struct UIBarGraph4 *self = (struct UIBarGraph4 *)window;
    struct UIWindow *wnd = &self->graph.base;
    int *current = self->graph.current, *maximum = self->graph.maximum;
    float ratio;
    COLORREF colour;

    if (!current || !maximum || *maximum < *current)
        return;
    draw_chip_background(wnd);
    if (*self->graph.maximum == 0)
        return;
    ratio = (float)*self->graph.current / (float)*self->graph.maximum;
    if (ratio == 0.0f)
        return;
    colour = chip_colour(2, 6);
    if (!self->no_warning && GAUGE_LOW_PERCENT > ratio * 100.0f)
        colour = GAUGE_RED;
    ui_window_fill_rect(wnd, 1, 1, (int)((float)(wnd->width - 2) * ratio), wnd->height - 2, colour);
}

void ui_bar_graph4_on_mouse_hover(struct UIWindow *window, int arg1_unused, int arg2_unused)
{
    struct UIBarGraph4 *self = (struct UIBarGraph4 *)window;
    int *current = self->graph.current, *maximum = self->graph.maximum;

    if (!current || !maximum || *maximum < *current || *maximum == 0)
        return;
    show_percent_balloon(&self->graph.base,
                         (int)((float)*self->graph.current / (float)*self->graph.maximum * 1000.0f));
}

struct UIINT64BarGraph {
    struct UIWindow base;
    BYTE no_warning;
    int64_t current;
    int64_t maximum;
};

static struct UIINT64BarGraph *uiint64_bar_graph_construct(struct UIINT64BarGraph *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &uiint64_bar_graph_ops;
    self->current = 0;
    self->maximum = 0;
    return self;
}

struct UIINT64BarGraph *uiint64_bar_graph_new(void)
{
    struct UIINT64BarGraph *self = mem_alloc(sizeof *self);

    return self ? uiint64_bar_graph_construct(self) : NULL;
}

void uiint64_bar_graph_set_values(struct UIINT64BarGraph *self, int64_t current, int64_t maximum)
{
    self->current = current;
    self->maximum = maximum;
}

void uiint64_bar_graph_set_no_warning(struct UIINT64BarGraph *self, BYTE no_warning)
{
    self->no_warning = no_warning;
}

void uiint64_bar_graph_destroy(struct UIWindow *window)
{
    struct UIINT64BarGraph *self = (struct UIINT64BarGraph *)window;
    self->base.ops = &uiint64_bar_graph_ops;
    ui_window_destruct(&self->base);
    mem_free(self);
}

void uiint64_bar_graph_on_draw(struct UIWindow *window)
{
    struct UIINT64BarGraph *self = (struct UIINT64BarGraph *)window;
    struct UIWindow *wnd = &self->base;
    double ratio;
    COLORREF colour;

    if (self->maximum < self->current)
        return;
    draw_chip_background(wnd);
    if (self->maximum == 0)
        return;
    ratio = (double)self->current / (double)self->maximum;
    if (ratio == 0.0)
        return;
    colour = chip_colour(2, 6);
    if (!self->no_warning && 25.0 > ratio * 100.0)
        colour = GAUGE_RED;
    ui_window_fill_rect(wnd, 1, 1, (int)((double)(wnd->width - 2) * ratio), wnd->height - 2, colour);
}

void uiint64_bar_graph_slot25(struct UIWindow *window, int arg1_unused, int arg2_unused)
{
    struct UIINT64BarGraph *self = (struct UIINT64BarGraph *)window;
    if (self->maximum < self->current || self->maximum == 0)
        return;
    show_percent_balloon(&self->base, (int)((double)self->current / (double)self->maximum * 1000.0));
}

static struct UIValueGauge *ui_bar_graph_player_construct(struct UIValueGauge *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_bar_graph_player_ops;
    self->current = (struct GaugeValue){0};
    self->maximum = (struct GaugeValue){0};
    return self;
}

void ui_bar_graph_player_destruct(struct UIValueGauge *self)
{
    self->base.ops = &ui_bar_graph_player_ops;
    ui_window_destruct(&self->base);
}

void ui_bar_graph_player_destroy(struct UIWindow *window)
{
    struct UIValueGauge *self = (struct UIValueGauge *)window;
    ui_bar_graph_player_destruct(self);
    mem_free(self);
}

static void draw_player_numbers(struct UIWindow *wnd, int64_t current, int64_t maximum)
{
    struct String text;

    string_init(&text);
    string_format(&text, "%3I64d  /  %3I64d", current, maximum);
    ui_window_text_out_centred(wnd, 0, -2, string_cstr(&text), wnd->width, 1, 13, 0, 0);
    string_free(&text);
}

void ui_bar_graph_player_on_draw(struct UIWindow *window)
{
    struct UIValueGauge *self = (struct UIValueGauge *)window;
    struct UIWindow *wnd = &self->base;
    int64_t current, maximum;
    double ratio;

    if (!self->current.read || !self->maximum.read)
        return;
    current = gauge_value_read(&self->current);
    maximum = gauge_value_read(&self->maximum);
    if (maximum < current)
        return;
    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\gze_bg1.bmp"), 0);
    if (maximum == 0)
        return;
    ratio = (double)current / (double)maximum;
    draw_gze_bar(wnd, 0.25 > ratio ? "red" : "blue", (int)((double)(wnd->width - 8) * ratio));
    draw_player_numbers(wnd, current, maximum);
}

void ui_bar_graph_player_ap_destroy(struct UIWindow *window)
{
    struct UIValueGauge *self = (struct UIValueGauge *)window;
    self->base.ops = &ui_bar_graph_player_ap_ops;
    ui_bar_graph_player_destruct(self);
    mem_free(self);
}

void ui_bar_graph_player_ap_on_draw(struct UIWindow *window)
{
    struct UIValueGauge *self = (struct UIValueGauge *)window;
    struct UIWindow *wnd = &self->base;
    int64_t current, maximum;
    double ratio;

    if (!self->current.read || !self->maximum.read)
        return;
    current = gauge_value_read(&self->current);
    maximum = gauge_value_read(&self->maximum);
    if (maximum < current)
        return;
    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\gze_bg1.bmp"), 0);
    if (maximum > 0) {
        ratio = (double)current / (double)maximum;
        if (ratio > 0.0)
            draw_gze_bar(wnd, ratio == 1.0 ? "red" : "blue", (int)((double)(wnd->width - 8) * ratio));
    }
    draw_player_numbers(wnd, current, maximum);
}

struct UIWindow *ui_value_gauge_create(enum ValueGaugeStyle style)
{
    struct UIValueGauge *self = mem_alloc(sizeof(*self));
    if (!self)
        return NULL;
    if (style == VALUE_GAUGE_HOMUN) {
        ui_bar_graph_homun_construct(self);
    } else {
        ui_bar_graph_player_construct(self);
        if (style == VALUE_GAUGE_AP)
            self->base.ops = &ui_bar_graph_player_ap_ops;
    }
    return &self->base;
}

void ui_value_gauge_set(struct UIWindow *window,
    struct GaugeValue current, struct GaugeValue maximum)
{
    struct UIValueGauge *self = (struct UIValueGauge *)window;
    self->current = current;
    self->maximum = maximum;
}
