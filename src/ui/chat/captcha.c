#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "ui/chat/captcha.h"
#include "ui/chat/captcha_data.h"
#include "resources/bitmap.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "platform/win_types.h"
#include "ui/chat/chat.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "platform/clock.h"

struct UICaptchaImage;
struct UIWindow;
void ui_captcha_image_destroy(struct UIWindow *window);
void ui_captcha_image_on_draw(struct UIWindow *window);
void ui_captcha_image_on_mouse_hover(struct UIWindow *window, int x_unused, int y_unused);
const struct UIWindowOps ui_captcha_image_ops = {
    .destroy = ui_captcha_image_destroy,
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
    .on_draw = ui_captcha_image_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_captcha_image_on_mouse_hover,
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
struct UICaptchaPreviewWnd;
void ui_captcha_preview_wnd_destroy(struct UIWindow *window);
void ui_captcha_preview_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused);
void ui_captcha_preview_wnd_on_draw(struct UIWindow *window);
int ui_captcha_preview_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_captcha_preview_wnd_ops = {
    .destroy = ui_captcha_preview_wnd_destroy,
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
    .on_create = ui_captcha_preview_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_captcha_preview_wnd_on_draw,
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
    .send_msg = ui_captcha_preview_wnd_send_msg,
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
#define WND_ID_CAPTCHA_PREVIEW 0x120

struct UICaptchaImage {
    struct UIWindow base;
    struct BitmapRes image;
    struct String tooltip;
};

static struct UICaptchaImage *ui_captcha_image_construct(struct UICaptchaImage *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_captcha_image_ops;
    bitmap_resource_init(&self->image);
    self->image.width = self->image.height = 0;
    string_init(&self->tooltip);
    return self;
}

void ui_captcha_image_destroy(struct UIWindow *window)
{
    struct UICaptchaImage *self = (struct UICaptchaImage *)window;
    self->base.ops = &ui_captcha_image_ops;
    bitmap_resource_clear(&self->image);
    string_free(&self->tooltip);
    ui_window_destruct(&self->base);
    free(self);
}

void ui_captcha_image_on_draw(struct UIWindow *window)
{
    struct UICaptchaImage *self = (struct UICaptchaImage *)window;
    ui_window_draw_bitmap(&self->base, 0, 0, &self->image, 1);
}

void ui_captcha_image_on_mouse_hover(struct UIWindow *window, int x_unused, int y_unused)
{
    struct UICaptchaImage *self = (struct UICaptchaImage *)window;
    int x = 0, y = -20;

    if (self->tooltip.size == 0)
        return;
    ui_window_to_screen(&self->base, &x, &y);
    mode_show_tooltip(g_modeMgr.curMode, string_cstr(&self->tooltip), x, y, -1, 0);
}

struct UICaptchaPreviewWnd {
    struct UIFrameWnd frame;
    struct UICaptchaImage *image;
    unsigned char key[4];
    struct CaptchaStream stream;
};

static int captcha_bitmap_load(struct UICaptchaImage *image, const void *bytes, size_t size)
{
    struct BitmapRes bitmap;
    if (!image || !captcha_bitmap_valid(bytes, size)) return 0;
    bitmap_resource_init(&bitmap);
    if (!bitmap_res_load_bmp_data(&bitmap, bytes, (int)size, 0)) {
        bitmap_resource_clear(&bitmap);
        return 0;
    }
    bitmap_resource_clear(&image->image);
    image->image = bitmap;
    ui_window_invalidate_base(&image->base);
    return 1;
}

static int captcha_unpack(struct UICaptchaImage *image, const struct CaptchaData *packed)
{
    struct CaptchaData bitmap = {0};
    int loaded = captcha_data_unpack(&bitmap, packed)
        && captcha_bitmap_load(image, bitmap.bytes, bitmap.size);
    captcha_data_clear(&bitmap);
    return loaded;
}

static struct UICaptchaPreviewWnd *ui_captcha_preview_wnd_construct(struct UICaptchaPreviewWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_captcha_preview_wnd_ops;
    self->image = NULL;
    memset(&self->stream, 0, sizeof self->stream);
    memset(self->key, 0, sizeof self->key);
    return self;
}

void ui_captcha_preview_wnd_destroy(struct UIWindow *window)
{
    struct UICaptchaPreviewWnd *self = (struct UICaptchaPreviewWnd *)window;
    self->frame.base.ops = &ui_captcha_preview_wnd_ops;
    captcha_stream_clear(&self->stream);
    ui_frame_wnd_destruct(&self->frame);
    free(self);
}

int captcha_preview_begin(struct UIWindow *window, const void *key, size_t size)
{
    struct UICaptchaPreviewWnd *self = (struct UICaptchaPreviewWnd *)window;
    if (!self || !key || !captcha_stream_begin(&self->stream, size)) return 0;
    memcpy(self->key, key, sizeof self->key);
    ui_window_set_show(window, 0);
    return 1;
}

void captcha_preview_append(struct UIWindow *window, const void *key, const void *chunk, size_t size)
{
    struct UICaptchaPreviewWnd *self = (struct UICaptchaPreviewWnd *)window;
    if (!self || !key || memcmp(self->key, key, sizeof self->key)) return;
    if (captcha_stream_append(&self->stream, chunk, size) == CAPTCHA_COMPLETE
        && captcha_unpack(self->image, &self->stream.packed))
        ui_window_set_show(window, 1);
}

void ui_captcha_preview_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UICaptchaPreviewWnd *self = (struct UICaptchaPreviewWnd *)window;
    struct UICaptchaImage *image = malloc(sizeof *image);

    if (!image) return;
    image = ui_captcha_image_construct(image);
    self->image = image;
    ui_window_create(&image->base, CAPTCHA_WIDTH, CAPTCHA_HEIGHT);
    ui_window_move_base(&self->image->base, 0, 0);
    ui_window_add_child(&self->frame.base, &self->image->base);
    ui_window_add_close_button(&self->frame.base, "", 0);
}

void ui_captcha_preview_wnd_on_draw(struct UIWindow *window)
{
    struct UICaptchaPreviewWnd *self = (struct UICaptchaPreviewWnd *)window;
    ui_window_clear_canvas(&self->frame.base, 0xffffffff);
}

int ui_captcha_preview_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           int val1, int val2, int val3, int val4)
{
    struct UICaptchaPreviewWnd *self = (struct UICaptchaPreviewWnd *)window;
    if (msg == UI_MSG_COMMAND && val1 == UI_ID_CLOSE)
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_CAPTCHA_PREVIEW);
    return ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
}

#define CAPTCHA_ID_SEND 0x151
#define CAPTCHA_ANSWER_MAX 0x10

#define WND_ID_MACRO_DETECTOR_CLOSES1 0x9b
#define WND_ID_MACRO_DETECTOR_CLOSES2 0x27
#define MSG_DETECTOR_LINE1 0xb43
#define MSG_DETECTOR_LINE2 0xb44
#define MSG_DETECTOR_LINE3 0xb45
#define MSG_DETECTOR_COUNT 0xb46
#define MSG_DETECTOR_RETRY 0xb3b
#define PACKET_DETECTOR_ANSWER 0xa5c
#define PACKET_DETECTOR_IMAGE_DONE 0xa5a
#define DETECTOR_BAR_WIDTH 220

struct UIMacroDetectorWnd {
    struct UIFrameWnd frame;
    struct UICaptchaImage *image;
    struct UIWindow *answer_edit;
    struct UIBitmapButton *send_button;
    char key[4];
    struct CaptchaStream stream;
    BYTE count;
    DWORD time_total;
    DWORD time_limit;
    DWORD time_base;
    struct String last_answer;
    struct String message;
    DWORD prev_timer;
    DWORD message_ms;
};

void ui_macro_detector_wnd_destroy(struct UIWindow *window);
void ui_macro_detector_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused);
void ui_macro_detector_wnd_on_process(struct UIWindow *window);
void ui_macro_detector_wnd_on_draw(struct UIWindow *window);
void ui_macro_detector_wnd_on_l_btn_down(struct UIWindow *window, int x, int y);
void ui_macro_detector_wnd_on_mouse_move(struct UIWindow *window, int x, int y);
void ui_macro_detector_wnd_on_l_btn_up(struct UIWindow *window, int x, int y);
int ui_macro_detector_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_macro_detector_wnd_ops = {
    .destroy = ui_macro_detector_wnd_destroy,
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
    .on_create = ui_macro_detector_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_macro_detector_wnd_on_process,
    .on_draw = ui_macro_detector_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_macro_detector_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_macro_detector_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_macro_detector_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_macro_detector_wnd_send_msg,
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

static struct UIMacroDetectorWnd *ui_macro_detector_wnd_construct(struct UIMacroDetectorWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_macro_detector_wnd_ops;
    self->image = NULL;
    self->answer_edit = NULL;
    self->send_button = NULL;
    memset(&self->stream, 0, sizeof self->stream);
    self->count = 0;
    self->time_total = 0;
    self->time_limit = 0;
    self->time_base = 0;
    string_init(&self->last_answer);
    string_init(&self->message);
    self->prev_timer = 0;
    self->message_ms = 3000;
    memset(self->key, 0, sizeof self->key);
    return self;
}

void ui_macro_detector_wnd_destroy(struct UIWindow *window)
{
    struct UIMacroDetectorWnd *self = (struct UIMacroDetectorWnd *)window;
    self->frame.base.ops = &ui_macro_detector_wnd_ops;
    captcha_stream_clear(&self->stream);
    string_free(&self->message);
    string_free(&self->last_answer);
    ui_frame_wnd_destruct(&self->frame);
    free(self);
}

int captcha_detector_begin(struct UIWindow *window, const void *key, size_t size)
{
    struct UIMacroDetectorWnd *self = (struct UIMacroDetectorWnd *)window;
    if (!self || !key || !captcha_stream_begin(&self->stream, size)) return 0;
    memcpy(self->key, key, sizeof self->key);
    ui_window_set_show(window, 0);
    return 1;
}

void captcha_detector_append(struct UIWindow *window, const void *key, const void *chunk, size_t size)
{
    struct UIMacroDetectorWnd *self = (struct UIMacroDetectorWnd *)window;
    WORD id = PACKET_DETECTOR_IMAGE_DONE;
    if (!self || !key || memcmp(self->key, key, sizeof self->key)) return;
    if (captcha_stream_append(&self->stream, chunk, size) != CAPTCHA_COMPLETE
        || !captcha_unpack(self->image, &self->stream.packed)) return;
    ui_window_invalidate(window);
    rag_connection_send_packet(rag_connection_instance(), sizeof id, &id);
}

void captcha_detector_set_timer(struct UIWindow *window, BYTE count, DWORD limit)
{
    struct UIMacroDetectorWnd *self = (struct UIMacroDetectorWnd *)window;
    DWORD now = platform_ticks_ms();
    if (!self) return;

    if (self->prev_timer != 0) {
        struct String line;

        string_init(&line);
        string_format(&line, msg_str(MSG_DETECTOR_RETRY), count);
        string_assign(&self->message, string_cstr(&line), line.size);
        string_free(&line);
    }
    self->count = count;
    self->prev_timer = now;
    self->time_base = now;
    if (self->time_total == 0)
        self->time_total = limit;
    self->time_limit = limit;
    ui_window_invalidate_base(&self->frame.base);
}

void ui_macro_detector_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIMacroDetectorWnd *self = (struct UIMacroDetectorWnd *)window;
    static const char *const send_bitmaps[3] = {
        UI_SKIN "btn_get.bmp", UI_SKIN "btn_get_a.bmp", UI_SKIN "btn_get_b.bmp",
    };
    struct UIWindow *wnd = &self->frame.base;
    struct UICaptchaImage *image;
    struct UIWindow *edit;
    struct UIBitmapButton *button;
    int state;

    wnd->alpha = 0xbb;
    wnd->alpha_target = 0xbb;

    image = malloc(sizeof *image);
    if (!image) return;
    image = ui_captcha_image_construct(image);
    self->image = image;
    ui_window_create(&image->base, CAPTCHA_WIDTH, CAPTCHA_HEIGHT);
    ui_window_move_base(&self->image->base, 0x3c, 0x9f);
    ui_window_add_child(wnd, &self->image->base);

    edit = mem_alloc(sizeof(struct UIEditCtrl));
    if (!edit) return;
    edit = ui_edit_ctrl_construct(edit);
    self->answer_edit = edit;
    ui_window_create(edit, 0xc8, 0x10);
    ui_window_move(self->answer_edit, 0x2d, 0x106);
    ((struct UIEditCtrl *)self->answer_edit)->max_chars = CAPTCHA_ANSWER_MAX;
    ui_edit_ctrl_set_back_colour(self->answer_edit, 0xff, 0xff, 0xff);
    ((struct UIEditCtrl *)self->answer_edit)->no_border = 0;
    ui_edit_ctrl_set_border_colour(self->answer_edit, 0x7d, 0x7d, 0x7d);
    ui_edit_ctrl_set_text_colour(self->answer_edit, 0, 0, 0, -1);
    ((struct UIEditCtrl *)self->answer_edit)->keep_ime_on_enter = 1;
    ui_window_add_child(wnd, self->answer_edit);

    button = mem_alloc(sizeof(struct UIBitmapButton));
    if (!button) return;
    button = ui_bitmap_button_construct(button);
    self->send_button = button;
    for (state = 0; state < 3; state++)
        ui_bitmap_button_set_bitmap(self->send_button, send_bitmaps[state], state, 0);
    ui_window_create(&self->send_button->button.base, self->send_button->bitmap_width,
                     self->send_button->bitmap_height);
    ui_window_move_base(&self->send_button->button.base, 0xfd, 0x105);
    self->send_button->button.base.id = CAPTCHA_ID_SEND;
    ui_window_add_child(wnd, &self->send_button->button.base);
}

void ui_macro_detector_wnd_on_draw(struct UIWindow *window)
{
    struct UIMacroDetectorWnd *self = (struct UIMacroDetectorWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    DWORD left, elapsed;

    ui_window_draw_title_bar(wnd, 0, "", 0);
    ui_window_draw_button_bar(wnd, 0, 0, 1);
    ui_window_fill_rect(wnd, 0, 0x10, wnd->width, wnd->height - 0x24, 0xffffffff);
    ui_window_draw_outline(wnd, 0, 0x10, wnd->width, wnd->height - 0x24, 0xc6c6c6);
    ui_window_text_out(wnd, 0x19, 0x1c, msg_str(MSG_DETECTOR_LINE1), 0, 0, 0xc, 0, 1, 0);
    ui_window_text_out(wnd, 0x19, 0x2d, msg_str(MSG_DETECTOR_LINE2), 0, 0, 0xc, 0, 1, 0);
    ui_window_text_out(wnd, 0x19, 0x3e, msg_str(MSG_DETECTOR_LINE3), 0, 0, 0xc, 0, 1, 0);
    ui_window_text_out(wnd, 0x19, 0x52, string_cstr(&self->message), 0, 0, 0xc, 0x2323ff, 1, 0);
    ui_window_fill_rect(wnd, 0x3b, 0x9e, 0xde, 0x5c, 0xc6c6c6);
    ui_window_fill_rect(wnd, 0x3c, 0x61, 0xdc, 0x1e, 0xc6c6c6);

    elapsed = platform_ticks_ms() - self->time_base;
    left = elapsed < self->time_limit ? self->time_limit - elapsed : 0;
    if (left && self->time_total) {
        struct String text;

        ui_window_fill_rect(wnd, 0x3c, 0x61,
                          (int)((double)(left < self->time_total ? left : self->time_total) / self->time_total * DETECTOR_BAR_WIDTH), 0x1e, 0xff);
        string_init(&text);
        string_format(&text, "%2d : %2d", left / 1000 / 60, left / 1000);
        ui_window_text_out_right(wnd, 0x118, 0x8a, string_cstr(&text), 0, 0, 0xd, 0xff2323, 1);
        string_free(&text);
        string_format(&text, msg_str(MSG_DETECTOR_COUNT), self->count);
        ui_window_text_out(wnd, 0x3c, 0x8a, string_cstr(&text), 0, 0, 0xd, 0x2323ff, 1, 0);
        string_free(&text);
    }
}

void ui_macro_detector_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
}

void ui_macro_detector_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
}

void ui_macro_detector_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
}

void ui_macro_detector_wnd_on_process(struct UIWindow *window)
{
    struct UIMacroDetectorWnd *self = (struct UIMacroDetectorWnd *)window;
    if (platform_ticks_ms() - self->time_base > self->message_ms)
        string_clear(&self->message);
    ui_window_invalidate_base(&self->frame.base);
}

int ui_macro_detector_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          int val1, int val2, int val3, int val4)
{
    struct UIMacroDetectorWnd *self = (struct UIMacroDetectorWnd *)window;
    if (msg == 0) {
        ui_macro_detector_wnd_send_msg(&self->frame.base, &self->frame.base, UI_MSG_COMMAND, CAPTCHA_ID_SEND, 0, 0, 0);
    } else if (msg == UI_MSG_COMMAND) {
        if (val1 == CAPTCHA_ID_SEND && self->answer_edit && ui_edit_ctrl_get_text_size(self->answer_edit) != 0
                   && !string_equal_cstr(&self->last_answer, ui_edit_ctrl_get_text(self->answer_edit))) {
#pragma pack(push, 1)
            struct {
                WORD id;
                char answer[CAPTCHA_ANSWER_MAX];
            } packet;
#pragma pack(pop)
            size_t answer_size = strlen(ui_edit_ctrl_get_text(self->answer_edit));
            packet.id = PACKET_DETECTOR_ANSWER;
            memset(packet.answer, 0, sizeof packet.answer);
            if (answer_size > sizeof packet.answer) answer_size = sizeof packet.answer;
            memcpy(packet.answer, ui_edit_ctrl_get_text(self->answer_edit), answer_size);
            if (rag_connection_send_packet(rag_connection_instance(), sizeof packet, &packet)) {
                string_assign_cstr(&self->last_answer, ui_edit_ctrl_get_text(self->answer_edit));
                language_cancel_input(g_language);
            }
        }
    }
    return ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
}

struct UIWindow *captcha_preview_create(void)
{
    struct UICaptchaPreviewWnd *self = malloc(sizeof *self);
    if (!self) return NULL;
    ui_captcha_preview_wnd_construct(self);
    ui_window_create(&self->frame.base, CAPTCHA_WIDTH, CAPTCHA_HEIGHT);
    if (!self->image) {
        ui_captcha_preview_wnd_destroy(&self->frame.base);
        return NULL;
    }
    return &self->frame.base;
}

struct UIWindow *captcha_detector_create(void)
{
    struct UIMacroDetectorWnd *self = malloc(sizeof *self);
    if (!self) return NULL;
    ui_macro_detector_wnd_construct(self);
    ui_window_create(&self->frame.base, 340, 312);
    if (!self->image || !self->answer_edit || !self->send_button) {
        ui_macro_detector_wnd_destroy(&self->frame.base);
        return NULL;
    }
    ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_MACRO_DETECTOR_CLOSES1);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_MACRO_DETECTOR_CLOSES2);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->answer_edit);
    return &self->frame.base;
}

struct UIWindow *captcha_detector_edit(struct UIWindow *window)
{
    struct UIMacroDetectorWnd *self = (struct UIMacroDetectorWnd *)window;
    return self ? self->answer_edit : NULL;
}
