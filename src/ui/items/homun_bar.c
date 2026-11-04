#include "render/rp_pool.h"
#include "ui/character/gauge_internal.h"
#include "core/object_type_tables.h"
#include "ui/items/items.h"
#include "core/fatal.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include <inttypes.h>

void ui_bar_graph_homun_destroy(struct UIWindow *window);
void ui_bar_graph_homun_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_bar_graph_homun_ops = {
    .destroy = ui_bar_graph_homun_destroy,
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
    .on_draw = ui_bar_graph_homun_on_draw,
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

#define GAUGE_LOW 0.25
#define GAUGE_INSET 4

struct UIValueGauge *ui_bar_graph_homun_construct(struct UIValueGauge *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_bar_graph_homun_ops;
    self->current = (struct GaugeValue){0};
    self->maximum = (struct GaugeValue){0};
    return self;
}

void ui_bar_graph_homun_destruct(struct UIValueGauge *self)
{
    self->base.ops = &ui_bar_graph_homun_ops;
    ui_window_destruct(&self->base);
}

void ui_bar_graph_homun_destroy(struct UIWindow *window)
{
    struct UIValueGauge *self = (struct UIValueGauge *)window;
    ui_bar_graph_homun_destruct(self);
    mem_free(self);
}

static struct BitmapRes *skin_bitmap(const char *path)
{
    return res_mgr_get(g_resMgr(), ui_skin_path(path));
}

void ui_bar_graph_homun_on_draw(struct UIWindow *window)
{
    struct UIValueGauge *self = (struct UIValueGauge *)window;
    struct BitmapRes *left, *mid, *right;
    int64_t value, maximum;
    const char *colour;
    char path[MAX_PATH];
    struct String text;
    double ratio;
    int fill, x;

    if (!self->current.read || !self->maximum.read)
        return;
    value = gauge_value_read(&self->current);
    maximum = gauge_value_read(&self->maximum);
    if (maximum < value)
        return;

    ui_window_clear_canvas(&self->base, 0xffff00ff);
    left = skin_bitmap(UI_SKIN "basic_interface\\gze_bg_left.bmp");
    mid = skin_bitmap(UI_SKIN "basic_interface\\gze_bg_mid.bmp");
    right = skin_bitmap(UI_SKIN "basic_interface\\gze_bg_right.bmp");
    if (left && mid && right) {
        ui_window_draw_bitmap(&self->base, 0, 0, left, 0);
        if (self->base.width - left->width - right->width > 0) {
            int64_t i;

            for (i = 0; i < (int64_t)(self->base.width - left->width - right->width); i++)
                ui_window_draw_bitmap(&self->base, left->width + (int)i, 0, mid, 0);
        }
        ui_window_draw_bitmap(&self->base, self->base.width - right->width, 0, right, 0);
    }

    if (maximum == 0)
        return;
    ratio = (double)value / (double)maximum;
    if (ratio == 0.0)
        return;
    colour = ratio < GAUGE_LOW ? "red" : "blue";
    fill = (int)((double)(self->base.width - 8) * ratio);

    sprintf(path, UI_SKIN "basic_interface\\gze%s_left.bmp", colour);
    ui_window_draw_bitmap(&self->base, 0, 0, skin_bitmap(path), 1);
    sprintf(path, UI_SKIN "basic_interface\\gze%s_mid.bmp", colour);
    mid = skin_bitmap(path);
    for (x = 0; x < fill; x++)
        ui_window_draw_bitmap(&self->base, x + GAUGE_INSET, 0, mid, 1);
    sprintf(path, UI_SKIN "basic_interface\\gze%s_right.bmp", colour);
    ui_window_draw_bitmap(&self->base, fill + GAUGE_INSET, 0, skin_bitmap(path), 1);

    string_init(&text);
    string_format(&text, "%3" PRId64 "  /  %3" PRId64, value, maximum);
    ui_window_text_out_centred(&self->base, 0, -2, string_cstr(&text), self->base.width, 1, 0xd, 0, 0);
    string_free(&text);
}
