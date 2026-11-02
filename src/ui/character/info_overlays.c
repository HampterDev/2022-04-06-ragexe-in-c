#include "platform/language.h"
#include "render/rp_pool.h"
#include "world/view_types.h"
#include "world/state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/character/character.h"
#include "platform/clientinfo.h"
#include "core/text_util.h"
#include "diag/perf.h"

struct UIKeyStrokeWnd;
struct UIWindow;
void ui_key_stroke_wnd_destroy(struct UIWindow *window);
void ui_key_stroke_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_key_stroke_wnd_on_draw(struct UIWindow *self);
int ui_key_stroke_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_key_stroke_wnd_ops = {
    .destroy = ui_key_stroke_wnd_destroy,
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
    .on_create = ui_key_stroke_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_key_stroke_wnd_on_draw,
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
    .send_msg = ui_key_stroke_wnd_send_msg,
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
struct UIViewCameraInfoWnd;
void ui_view_camera_info_wnd_destroy(struct UIWindow *window);
void ui_view_camera_info_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_view_camera_info_wnd_on_draw(struct UIWindow *self);
int ui_view_camera_info_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_view_camera_info_wnd_ops = {
    .destroy = ui_view_camera_info_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_query_true,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_view_camera_info_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_view_camera_info_wnd_on_draw,
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
    .send_msg = ui_view_camera_info_wnd_send_msg,
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

#define KEY_STROKE_LANGUAGE 4

struct UIKeyStrokeWnd {
    struct UIFrameWnd frame;
};

struct UIKeyStrokeWnd *ui_key_stroke_wnd_construct(struct UIKeyStrokeWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_key_stroke_wnd_ops;
    return self;
}

struct UIWindow *ui_key_stroke_wnd_new(void)
{
    return (struct UIWindow *)ui_key_stroke_wnd_construct(mem_alloc(sizeof(struct UIKeyStrokeWnd)));
}

void ui_key_stroke_wnd_destroy(struct UIWindow *window)
{
    struct UIKeyStrokeWnd *self = (struct UIKeyStrokeWnd *)window;
    self->frame.base.ops = &ui_key_stroke_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_key_stroke_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused)
{
}

static COLORREF key_stroke_chip(int x, int y)
{
    int red, green, blue;

    ui_skin_color_chip(&g_skinMgr, x, y, &red, &green, &blue);
    return RGB((BYTE)red, (BYTE)green, (BYTE)blue);
}

void ui_key_stroke_wnd_on_draw(struct UIWindow *window)
{
    struct UIKeyStrokeWnd *self = (struct UIKeyStrokeWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    COLORREF edge;
    char key[3];
    int i;

    if (g_languageIndex != KEY_STROKE_LANGUAGE)
        return;
    ui_window_fill_rect(wnd, 1, 1, wnd->width - 2, wnd->height - 2, key_stroke_chip(2, 2));
    edge = key_stroke_chip(0x16, 2);
    ui_window_fill_rect(wnd, 0, 0, wnd->width, 1, edge);
    ui_window_fill_rect(wnd, 0, 0, 1, wnd->height, edge);
    ui_window_fill_rect(wnd, 0, wnd->height - 1, wnd->width, 1, edge);
    ui_window_fill_rect(wnd, wnd->width - 1, 0, 1, wnd->height, edge);
    key[0] = key[1] = key[2] = 0;
    for (i = 0; i < 4; i++) {
        memcpy(key, language_reading(g_language) + i * 2, 2);
        ui_window_text_out(wnd, 2 + i * 14, 2, key, 2, 0, 12, 0, 0, 0);
    }
}

int ui_key_stroke_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                      int val2, int val3, int val4)
{
    struct UIKeyStrokeWnd *self = (struct UIKeyStrokeWnd *)window;
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UIViewCameraInfoWnd {
    struct UIFrameWnd frame;
};

struct UIViewCameraInfoWnd *ui_view_camera_info_wnd_construct(struct UIViewCameraInfoWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_view_camera_info_wnd_ops;
    return self;
}

struct UIWindow *ui_view_camera_info_wnd_new(void)
{
    return (struct UIWindow *)ui_view_camera_info_wnd_construct(mem_alloc(sizeof(struct UIViewCameraInfoWnd)));
}

void ui_view_camera_info_wnd_destroy(struct UIWindow *window)
{
    struct UIViewCameraInfoWnd *self = (struct UIViewCameraInfoWnd *)window;
    self->frame.base.ops = &ui_view_camera_info_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_view_camera_info_wnd_on_create(struct UIWindow *self, int width_unused,
                                             int height_unused)
{
}

static void camera_line(struct UIWindow *wnd, int y, const char *format, float value)
{
    struct String text;

    string_init(&text);
    string_format(&text, format, (double)value);
    ui_window_text_out(wnd, 10, y, string_cstr(&text), 0, 0, 12, 0xff, 1, 0);
    string_free(&text);
}

void ui_view_camera_info_wnd_on_draw(struct UIWindow *window)
{
    struct UIViewCameraInfoWnd *self = (struct UIViewCameraInfoWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct World *world;
    struct View *view;

    ui_window_clear_canvas(wnd, 0xffff00ff);
    world = game_current_world();
    view = game_current_view();
    if (!world || !world->player || !view)
        return;
    camera_line(wnd, 10, "range : %4.3f", view->distance);
    camera_line(wnd, 0x19, "rotation : %4.3f", view->longitude);
    camera_line(wnd, 0x28, "latitude : %4.3f", view->latitude);
}

int ui_view_camera_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           int val1, int val2, int val3, int val4)
{
    struct UIViewCameraInfoWnd *self = (struct UIViewCameraInfoWnd *)window;
    return ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
}
