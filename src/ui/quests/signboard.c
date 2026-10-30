#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include <stdlib.h>
#include <string.h>
#include "ui/quests/quests.h"
#include "diag/perf.h"

#define SIGNBOARD_BG_BMP UI_SKIN "signboard\\bg_signboard.bmp"

struct SignBoardWnd;
struct UIWindow;
void sign_board_wnd_destroy(struct UIWindow *window);
void sign_board_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void sign_board_wnd_on_draw(struct UIWindow *self);
const struct UIWindowOps sign_board_wnd_ops = {
    .destroy = sign_board_wnd_destroy,
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
    .on_create = sign_board_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = sign_board_wnd_on_draw,
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
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct SignBoardWnd {
    struct UIWindow base;
    struct UIWindow *label;
    struct String text;
    struct String icon;
    struct String colour;
};

static void sign_board_wnd_destruct(struct SignBoardWnd *self)
{
    self->base.ops = &sign_board_wnd_ops;
    string_free(&self->colour);
    string_free(&self->icon);
    string_free(&self->text);
    ui_window_destruct(&self->base);
}

void sign_board_wnd_destroy(struct UIWindow *window)
{
    struct SignBoardWnd *self = (struct SignBoardWnd *)window;
    sign_board_wnd_destruct(self);
    mem_free(self);
}

void sign_board_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct SignBoardWnd *self = (struct SignBoardWnd *)window;
    struct BitmapRes *bg = ui_skin_bitmap(SIGNBOARD_BG_BMP);
    struct UIWindow *label;

    ui_window_resize(&self->base, bg->width, bg->height);
    label = mem_try_alloc(sizeof(struct UIStaticText));
    if (label)
        label = ui_static_text_construct(label);
    self->label = label;
    ui_window_create(label, 0x7e, 0xe);
    ui_window_move(self->label, 0x1e, self->base.height / 2 - 7);
    ui_static_text_set_text_ellipsis(self->label, "");
    ui_static_text_set_font(self->label, 0, 12, 0, 0);
    ui_static_text_set_text_colour(self->label, 0x9e, 0xbd, 0xff);
    ui_window_add_child(&self->base, self->label);
}

void sign_board_wnd_on_draw(struct UIWindow *window)
{
    struct SignBoardWnd *self = (struct SignBoardWnd *)window;
    ui_window_clear_canvas(&self->base, 0xffff00ff);
    ui_window_draw_bitmap(&self->base, 0, 0, ui_skin_bitmap(SIGNBOARD_BG_BMP), 0);
    ui_window_draw_bitmap(&self->base, 5, 5, ui_skin_bitmap(string_cstr(&self->icon)), 1);
}

void sign_board_wnd_apply_text_colour(struct SignBoardWnd *self)
{
    int rgb[3] = { 0, 0, 0 };
    int r = 0, g = 0, b = 0;

    if (self->colour.size != 0) {
        const char *spec = string_cstr(&self->colour);
        const char *hash;

        if (strchr(spec, ',')) {
            const char *part = spec;
            int i = 0;

            for (;;) {
                const char *comma = strchr(part, ',');

                if (i < 3)
                    rgb[i] = atoi(part);
                i++;
                if (!comma)
                    break;
                part = comma + 1;
            }
            b = rgb[2];
        }
        hash = strchr(spec, '#');
        if (hash) {
            unsigned long value = strtoul(hash + 1, NULL, 16);

            r = (value >> 16) & 0xff;
            g = (value >> 8) & 0xff;
            b = value & 0xff;
        } else {
            r = rgb[0];
            g = rgb[1];
        }
    }
    ui_static_text_set_text_colour(self->label, r, g, b);
}
