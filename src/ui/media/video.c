#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "ui/media/video.h"
#include "ui/media/video_playback.h"
#include <stdlib.h>
#include "render/texture.h"
#include "contents/sound.h"
#include "core/object_type_tables.h"
#include "ui/render/render.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "world/world.h"

#define VIDEO_WND_ID 0x130
#define VIDEO_SKIP_BUTTON_ID 0x201

#define UI_MSG_VIDEO_ASK_SKIP 5
#define UI_MSG_VIDEO_OPEN 0x96
#define UI_MSG_VIDEO_PAUSE 0x97
#define UI_MSG_VIDEO_VOLUME 0x9f
#define VK_CODE_ESCAPE 0x1b
#define MSG_VIDEO_SKIP_QUESTION 0xc46

struct UISkipButton;
struct UIWindow;
void ui_skip_button_destroy(struct UIWindow *window);
void ui_skip_button_on_draw(struct UIWindow *self);
const struct UIButtonOps ui_skip_button_ops = {
    .base = {
        .destroy = ui_skip_button_destroy,
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
        .on_draw = ui_skip_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
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
    },
    .set_show = ui_button_set_show,
};

struct UISkipButton {
    struct UIButton button;
};

void ui_skip_button_destruct(struct UISkipButton *self)
{
    self->button.base.ops = &ui_skip_button_ops.base;
    ui_button_destruct(&self->button);
}

void ui_skip_button_destroy(struct UIWindow *window)
{
    struct UISkipButton *self = (struct UISkipButton *)window;
    ui_skip_button_destruct(self);
    mem_free(self);
}

void ui_skip_button_on_draw(struct UIWindow *window)
{
    struct UISkipButton *self = (struct UISkipButton *)window;
    struct UIWindow *wnd = &self->button.base;
    const char *text = "SKIP>";
    int text_w, text_h;

    ui_window_clear_canvas(wnd, 0xff000000);
    text_w = ui_window_text_width(wnd, text, 0, 0, 0x1e, 1, 0);
    text_h = ui_window_text_height(wnd, text, 0, 0, 0x1e, 1, 0);
    ui_window_text_out(wnd, (int)((float)(wnd->width - text_w) * 0.5f + 5.0f),
                     (int)((float)(wnd->height - text_h) * 0.5f), text, 0, 0, 0x1e,
                     wnd->state == UI_BTN_HOVER ? 0x4ec7ff : 0xffffff, 1, 0);
}

struct UIVideoWnd;
void ui_video_wnd_destroy(struct UIWindow *window);
void ui_video_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_video_wnd_on_process(struct UIWindow *self);
void ui_video_wnd_on_draw(struct UIWindow *self);
int ui_video_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_video_wnd_ops = {
    .destroy = ui_video_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
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
    .on_create = ui_video_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_video_wnd_on_process,
    .on_draw = ui_video_wnd_on_draw,
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
    .send_msg = ui_video_wnd_send_msg,
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

struct UIVideoWnd {
    struct UIWindow base;
    struct VideoPlayback *playback;
    int saved_volume;
};

struct UIWindow *video_window_create(void)
{
    struct UIVideoWnd *self = calloc(1, sizeof *self);
    if (!self) return NULL;
    self->playback = video_playback_create();
    if (!self->playback) {
        free(self);
        return NULL;
    }
    ui_window_construct(&self->base, 1);
    self->base.ops = &ui_video_wnd_ops;
    self->saved_volume = -1;
    return &self->base;
}

void ui_video_wnd_destruct(struct UIVideoWnd *self)
{
    self->base.ops = &ui_video_wnd_ops;
    video_playback_destroy(self->playback);
    self->playback = NULL;
    if (self->saved_volume != -1) {
        sound_set_bgm_volume(self->saved_volume);
        self->saved_volume = -1;
    }
    ui_window_destruct(&self->base);
}

void ui_video_wnd_destroy(struct UIWindow *window)
{
    struct UIVideoWnd *self = (struct UIVideoWnd *)window;
    ui_video_wnd_destruct(self);
    free(self);
}

void ui_video_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIVideoWnd *self = (struct UIVideoWnd *)window;
    struct UISkipButton *skip = mem_alloc(sizeof *skip);

    if (!skip) return;
    ui_button_construct(&skip->button);
    skip->button.base.ops = &ui_skip_button_ops.base;
    ui_window_create(&skip->button.base, 0x7f, 0x2b);
    ui_window_move_base(&skip->button.base, width - skip->button.base.width - 0x21, height - skip->button.base.height - 0x6e);
    skip->button.base.id = VIDEO_SKIP_BUTTON_ID;
    ui_window_add_child(&self->base, &skip->button.base);
}

void ui_video_wnd_on_process(struct UIWindow *window)
{
    struct UIVideoWnd *self = (struct UIVideoWnd *)window;
    enum VideoUpdate update = video_playback_update(self->playback);
    if (update == VIDEO_REDRAW) ui_window_invalidate_base(&self->base);
    if (update == VIDEO_FINISHED) ui_close_window(VIDEO_WND_ID);
}

void ui_video_wnd_on_draw(struct UIWindow *window)
{
    struct UIVideoWnd *self = (struct UIVideoWnd *)window;
    struct UIWindow *wnd = &self->base;
    struct Texture *texture = video_playback_texture(self->playback);

    ui_window_reset_draws(wnd, wnd->width, wnd->height, 0xff000000);
    if (texture) {
        float height = (float)texture->reqHeight / (float)texture->reqWidth * (float)wnd->width;
        int frame_h;

        if (height > (float)wnd->height)
            height = (float)wnd->height;
        frame_h = (int)height;
        ui_window_queue_texture_stretched(wnd, 0, (int)((double)(wnd->height - frame_h) * 0.5), texture,
                                       0xffffffff, wnd->width, frame_h);
    }
}

int ui_video_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                  int val3, int val4)
{
    struct UIVideoWnd *self = (struct UIVideoWnd *)window;
    switch (msg) {
    case 0:
        if (val1 == VK_CODE_ESCAPE)
            ui_video_wnd_send_msg(&self->base, NULL, UI_MSG_VIDEO_ASK_SKIP, 0, 0, 0, 0);
        break;
    case UI_MSG_VIDEO_ASK_SKIP:
        if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_VIDEO_SKIP_QUESTION), UI_MSGBOX_QUESTION, 1, 0, 0,
                                   "메시지", 0x118, 0x78, 0) == UI_MSGBOX_ANSWER_YES)
            ui_close_window(VIDEO_WND_ID);
        break;
    case UI_MSG_COMMAND:
        if (val1 == VIDEO_SKIP_BUTTON_ID)
            ui_close_window(VIDEO_WND_ID);
        break;
    case UI_MSG_VIDEO_OPEN:
        if (video_playback_open(self->playback, (const char *)val1)) {
            list_clear(&self->base.draws, NULL);
            ui_window_invalidate_base(&self->base);
        }
        break;
    case UI_MSG_VIDEO_PAUSE:
        video_playback_pause(self->playback);
        break;
    case UI_MSG_VIDEO_VOLUME:
        video_playback_volume(self->playback, val1);
        if (!val2) {
            if (self->saved_volume == -1) self->saved_volume = sound_bgm_volume();
            sound_set_bgm_volume(0);
        }
        break;
    }
    return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
}
