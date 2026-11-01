#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "ui/quests/quests.h"
#include "diag/perf.h"

struct UIRPImageWnd;
struct UIWindow;
void uirp_image_wnd_destroy(struct UIWindow *window);
void uirp_image_wnd_on_draw(struct UIWindow *self);
const struct UIWindowOps uirp_image_wnd_ops = {
    .destroy = uirp_image_wnd_destroy,
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
    .on_draw = uirp_image_wnd_on_draw,
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
struct UIRPImageWnd2;
void uirp_image_wnd2_destroy(struct UIWindow *window);
void uirp_image_wnd2_on_draw(struct UIWindow *self);
const struct UIWindowOps uirp_image_wnd2_ops = {
    .destroy = uirp_image_wnd2_destroy,
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
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = uirp_image_wnd2_on_draw,
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
    .send_msg = ui_frame_wnd_send_msg,
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

struct UIRPImageWnd {
    struct UIWindow base;
    struct String image;
    int draw_count;
};

struct UIRPImageWnd2 {
    struct UIFrameWnd frame;
    struct String image;
    int draw_count;
    BYTE ready;
};

static void queue_image(struct UIWindow *wnd, const struct String *image, int draw_count)
{
    int i;

    ui_window_reset_draws(wnd, wnd->width, wnd->height, 0);
    for (i = 0; i < draw_count; i++) {
        struct String copy;
        string_copy(&copy, image);
        ui_window_queue_bitmap(wnd, 0, 0, copy, 0xffffffff, 0, 0);
    }
}

static struct UIRPImageWnd *uirp_image_wnd_construct(struct UIRPImageWnd *self, struct String image,
                                                     int draw_count)
{
    ui_window_construct(&self->base, 1);
    self->base.ops = &uirp_image_wnd_ops;
    string_copy(&self->image, &image);
    self->draw_count = draw_count;
    string_free(&image);
    return self;
}

struct UIRPImageWnd *uirp_image_wnd_new(const char *image, int draw_count)
{
    struct UIRPImageWnd *self = mem_alloc(sizeof *self);
    struct String path;

    if (!self)
        return NULL;
    string_init(&path);
    string_assign_cstr(&path, image);
    return uirp_image_wnd_construct(self, path, draw_count);
}

static void uirp_image_wnd_destruct(struct UIRPImageWnd *self)
{
    self->base.ops = &uirp_image_wnd_ops;
    string_free(&self->image);
    ui_window_destruct(&self->base);
}

void uirp_image_wnd_destroy(struct UIWindow *window)
{
    struct UIRPImageWnd *self = (struct UIRPImageWnd *)window;
    uirp_image_wnd_destruct(self);
    mem_free(self);
}

void uirp_image_wnd_on_draw(struct UIWindow *window)
{
    struct UIRPImageWnd *self = (struct UIRPImageWnd *)window;
    queue_image(&self->base, &self->image, self->draw_count);
}

struct UIRPImageWnd2 *uirp_image_wnd2_construct(struct UIRPImageWnd2 *self)
{
    ui_frame_wnd_construct(&self->frame, 1);
    self->frame.base.ops = &uirp_image_wnd2_ops;
    string_init(&self->image);
    self->ready = 0;
    return self;
}

struct UIWindow *uirp_image_wnd2_new(void)
{
    return (struct UIWindow *)uirp_image_wnd2_construct(mem_alloc(sizeof(struct UIRPImageWnd2)));
}

static void uirp_image_wnd2_destruct(struct UIRPImageWnd2 *self)
{
    self->frame.base.ops = &uirp_image_wnd2_ops;
    string_free(&self->image);
    ui_frame_wnd_destruct(&self->frame);
}

void uirp_image_wnd2_destroy(struct UIWindow *window)
{
    struct UIRPImageWnd2 *self = (struct UIRPImageWnd2 *)window;
    uirp_image_wnd2_destruct(self);
    mem_free(self);
}

void uirp_image_wnd2_set_image(struct UIRPImageWnd2 *self, struct String path, int draw_count)
{
    string_assign(&self->image, string_cstr(&path), path.size);
    self->draw_count = draw_count;
    self->ready = 1;
    string_free(&path);
}

void uirp_image_wnd2_on_draw(struct UIWindow *window)
{
    struct UIRPImageWnd2 *self = (struct UIRPImageWnd2 *)window;
    if (self->ready != 1)
        return;
    queue_image(&self->frame.base, &self->image, self->draw_count);
}
