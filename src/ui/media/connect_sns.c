#include "ui/controls/edit.h"
#include "ui/controls/scroll.h"
#include "render/rp_pool.h"
#include "contents/twitter.h"
#include "ui/browser_windows.h"
#include "session/state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <io.h>
#include <direct.h>
#include "ui/window_mgr.h"
#include "render/frame_capture.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/character/char_info.h"
#include "ui/runtime.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include "core/app.h"
#include "browser/browser.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/media/connect_sns.h"
#include "ui/wndmgr/make_window_internal.h"

#define SNS_DIR UI_SKIN "sns\\"

#define TWITTER_WINDOW 0x11a
#define RECT_WINDOW 0x11b

#define ID_CAPTURE 0xb5
#define ID_CAPTURE_PART 0xb6
#define ID_AUTH 0xcd
#define ID_SEND 0x151

#define MSG_TITLE 0xb1d
#define MSG_NO_IMAGE 0xb1c
#define MSG_EMPTY_TEXT 0xb55
#define MSG_CAPTURE_TIP 0xb8a
#define MSG_CAPTURE_PART_TIP 0xb8b
#define MSG_SEND_TIP 0xb8c
#define MSG_NO_CAPTURE 0xb91
#define MSG_HASHTAG 0xbde
#define MSG_CAUTION 0xbdf
#define MSG_NOT_AUTHORISED 0xca2
#define MSG_NO_SERVICE 0xca3

#define CURSOR_CROSS 9
#define SNAP 12

int g_snsCaptureCount;
enum { TWEET_MAX_CHARS = 140, TWEET_INDENT = 10 };
#include "render/device.h"

struct UITwitterScrollBar {
    struct UIScrollBar bar;
    DWORD colour;
};
SIZE_ASSERT(UITwitterScrollBar, 0xb0);

struct UIConnectSnsWnd {
    struct UIFrameWnd frame;
    RECT capture;
    struct String image;
};
OFFSET_ASSERT(UIConnectSnsWnd, image, 0xa4);

struct UIConnectTwitterWnd {
    struct UIConnectSnsWnd sns;
    struct UIEditCtrl3 *edit;
    struct UIBitmapButton *capture;
    struct UIBitmapButton *capture_part;
    int capture_kind;
    struct UIBitmapButton *send;
    struct UIWindow *close;
    struct UICheckBox *auth;
    int margin;
    int bottom_gap;
    int edit_inset;
    BYTE first_draw;
};

struct UIControlRectWnd {
    struct UIFrameWnd frame;
    BYTE dragging;
    float start_x, start_y;
    float cur_x, cur_y;
    int mode;
    int kind;
};
OFFSET_ASSERT(UIControlRectWnd, start_x, 0x98);
OFFSET_ASSERT(UIControlRectWnd, kind, 0xac);
SIZE_ASSERT(UIControlRectWnd, 0xb0);
#define RECT_KIND_PART 1
#define RECT_KIND_SCREEN 2

struct ConnectSnsMgr {
    BYTE valid;
};
struct ConnectSnsMgr *g_connectSnsMgr;

void ui_control_rect_wnd_destroy(struct UIWindow *window);
void ui_control_rect_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_control_rect_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_control_rect_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_control_rect_wnd_on_mouse_shape(struct UIWindow *self, int x, int y);
void ui_control_rect_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
const struct UIWindowOps ui_control_rect_wnd_ops = {
    .destroy = ui_control_rect_wnd_destroy,
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
    .on_create = ui_control_rect_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_window_on_draw_base,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_control_rect_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_control_rect_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_control_rect_wnd_on_mouse_shape,
    .on_l_btn_up = ui_control_rect_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_frame_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
void ui_connect_sns_wnd_destroy(struct UIWindow *window);
void ui_connect_sns_wnd_resize(struct UIWindow *self, int width, int height);

const struct UIWindowOps ui_connect_sns_wnd_ops = {
    .destroy = ui_connect_sns_wnd_destroy,
    .resize = ui_connect_sns_wnd_resize,
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
    .on_draw = ui_window_on_draw_base,
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
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
void ui_connect_twitter_wnd_destroy(struct UIWindow *window);
void ui_connect_twitter_wnd_resize(struct UIWindow *self, int width, int height);
void ui_connect_twitter_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_connect_twitter_wnd_on_draw(struct UIWindow *self);
int ui_connect_twitter_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
static void ui_connect_twitter_wnd_send(struct UIConnectTwitterWnd *self);
static void ui_connect_twitter_wnd_fit_image( int *width, int *height);
static void ui_connect_twitter_wnd_min_size( int *width, int *height);
const struct UIWindowOps ui_connect_twitter_wnd_ops = {
    .destroy = ui_connect_twitter_wnd_destroy,
    .resize = ui_connect_twitter_wnd_resize,
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
    .on_create = ui_connect_twitter_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_connect_twitter_wnd_on_draw,
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
    .send_msg = ui_connect_twitter_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_frame_wnd_get_records,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
void ui_twitter_edit_ctrl3_destroy(struct UIWindow *window);
void ui_twitter_edit_ctrl3_on_create(struct UIWindow *self, int width, int height);
void ui_twitter_edit_ctrl3_on_draw(struct UIWindow *self);
static void ui_twitter_edit_ctrl3_update_bar(struct UIEditCtrl3 *self);

const struct UIEditCtrl3Ops ui_twitter_edit_ctrl3_ops = {
    .edit = {
        .base = {
            .destroy = ui_twitter_edit_ctrl3_destroy,
            .resize = ui_edit_ctrl3_resize,
            .is_frame = 0,
            .refreshes_children = 0,
            .move = ui_window_move_base,
            .is_focusable = ui_edit_ctrl_is_focusable,
            .get_backdrop_box = ui_window_get_backdrop_box_base,
            .is_click_through = ui_window_is_click_through_base,
            .is_hit_testable = ui_window_is_hit_testable_base,
            .drag_and_drop = ui_window_drag_and_drop_base,
            .store_info = ui_window_store_info_base,
            .save_pos = ui_window_save_pos_base,
            .move_delta = ui_window_move_delta_base,
            .set_show = ui_window_set_show_base,
            .on_create = ui_twitter_edit_ctrl3_on_create,
            .on_destroy = ui_window_on_destroy_base,
            .on_process = ui_window_on_process_base,
            .on_draw = ui_twitter_edit_ctrl3_on_draw,
            .on_focus = ui_edit_ctrl_on_focus,
            .on_focus_lost = ui_edit_ctrl_on_focus_lost,
            .on_l_btn_down = ui_edit_ctrl_on_l_btn_down,
            .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
            .on_mouse_move = ui_window_on_mouse_move_base,
            .on_mouse_hover = ui_edit_ctrl_on_mouse_hover,
            .track_mouse = ui_window_track_mouse_base,
            .on_mouse_shape = ui_window_on_mouse_shape_base,
            .on_l_btn_up = ui_window_on_l_btn_up_base,
            .on_r_btn_down = ui_window_on_r_btn_down_base,
            .on_r_btn_up = ui_window_on_r_btn_up_base,
            .on_wheel = ui_edit_ctrl3_on_wheel,
            .snap = ui_window_snap_base,
            .send_msg = ui_edit_ctrl3_send_msg,
            .invalidate = ui_window_invalidate_base,
            .render = ui_window_render_base,
            .has_custom_backdrop = 0,
            .draw_backdrop = ui_window_draw_backdrop_base,
            .get_records = ui_window_get_records_base,
            .type = UI_TYPE_EDIT,
            .on_hover_enter = ui_window_on_hover_enter_base,
            .on_hover_leave = ui_window_on_hover_leave_base,
            .make_quad = ui_window_make_quad_base,
            .hit_test = ui_window_hit_test_base,
            .render_overlay = ui_window_render_overlay_base,
        },
        .set_text = ui_edit_ctrl3_set_text,
        .draw_text = ui_edit_ctrl3_draw_text,
        .truncate = ui_edit_ctrl_truncate_to_ime,
    },
    .update_bar = ui_twitter_edit_ctrl3_update_bar,
};
void ui_twitter_scroll_bar_destroy(struct UIWindow *window);
void ui_twitter_scroll_bar_on_draw(struct UIWindow *self);
const struct UIWindowOps ui_twitter_scroll_bar_ops = {
    .destroy = ui_twitter_scroll_bar_destroy,
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
    .on_draw = ui_twitter_scroll_bar_on_draw,
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
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

void ui_connect_sns_wnd_save_capture(struct UIConnectSnsWnd *self);

#define SNS(self) (&(self)->frame.base)
#define TW(self) (&(self)->sns.frame.base)
#define RW(self) (&(self)->frame.base)

static COLORREF edit_back_colour(const struct UIEditCtrl *edit)
{
    return (BYTE)edit->back_r | (BYTE)edit->back_g << 8 | (BYTE)edit->back_b << 16;
}

BYTE connect_sns_mgr_create_instance(void)
{
    struct ConnectSnsMgr *mgr;

    if (g_connectSnsMgr)
        return 0;
    mgr = mem_try_alloc(sizeof *mgr);
    if (mgr)
        mgr->valid = 1;
    g_connectSnsMgr = mgr;
    if (!mgr)
        return 0;
    if (!mgr->valid) {
        mem_free(mgr);
        g_connectSnsMgr = NULL;
        return 0;
    }
    return 1;
}

void connect_sns_mgr_destroy_instance(void)
{
    if (g_connectSnsMgr) {
        mem_free(g_connectSnsMgr);
        g_connectSnsMgr = NULL;
    }
}

int connect_sns_mgr_is_window_closed(void)
{
    return ui_window_find(TWITTER_WINDOW) == NULL;
}

struct UIControlRectWnd *ui_control_rect_wnd_construct(struct UIControlRectWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    RW(self)->ops = &ui_control_rect_wnd_ops;
    self->dragging = 0;
    return self;
}

struct UIWindow *ui_control_rect_wnd_new(void)
{
    return (struct UIWindow *)ui_control_rect_wnd_construct(mem_alloc(sizeof(struct UIControlRectWnd)));
}

void ui_control_rect_wnd_destruct(struct UIControlRectWnd *self)
{
    RW(self)->ops = &ui_control_rect_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_control_rect_wnd_destroy(struct UIWindow *window)
{
    struct UIControlRectWnd *self = (struct UIControlRectWnd *)window;
    ui_control_rect_wnd_destruct(self);
    mem_free(self);
}

void ui_control_rect_wnd_draw(struct UIControlRectWnd *self)
{
    struct UIConnectSnsWnd *wnd;
    float mx, my;
    int rem;
    RECT rc;

    ui_window_clear_canvas(RW(self), 0x80000000);
    if (self->kind == RECT_KIND_SCREEN) {
        GetClientRect(g_hMainWnd, &rc);
        if (self->mode == 0) {
            wnd = (struct UIConnectSnsWnd *)ui_window_find(TWITTER_WINDOW);
            if (wnd) {
                if (ui_window_find(RECT_WINDOW))
                    ui_window_mgr_post_quit1(&g_windowMgr, RECT_WINDOW);
                wnd->capture = rc;
                ui_connect_sns_wnd_save_capture(wnd);
            }
        }
    }
    if (self->dragging != 1)
        return;
    mx = (float)g_mouse.x;
    my = (float)g_mouse.y;
    rem = (int)(mx - self->start_x) % SNAP;
    rc.left = (int)(mx > self->start_x ? self->start_x : mx);
    rc.right = (int)((self->start_x > mx ? self->start_x : mx) - (float)rem);
    rc.top = (int)(self->start_y < my ? self->start_y : my);
    rc.bottom = (int)(self->start_y > my ? self->start_y : my);
    if (rc.left > 0 && rc.right > 0 && rc.top > 0 && rc.bottom != 0)
        ui_window_fill_canvas(RW(self), 0xffff00ff, &rc);
}

void ui_control_rect_wnd_start(struct UIControlRectWnd *self)
{
    if (self->kind != RECT_KIND_PART)
        return;
    RW(self)->alpha_target = 0x80;
    RW(self)->alpha_time = timeGetTime();
    ui_control_rect_wnd_draw(self);
}

void ui_control_rect_wnd_on_create(struct UIWindow *self, int width, int height)
{
    (void)self;
    (void)width;
    (void)height;
}

void ui_control_rect_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIControlRectWnd *self = (struct UIControlRectWnd *)window;
    if (!self->dragging) {
        self->dragging = 1;
        self->start_x = (float)g_mouse.x;
        self->start_y = (float)g_mouse.y;
    }
    ui_window_invalidate_base(&self->frame.base);
    (void)x;
    (void)y;
}

void ui_control_rect_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIControlRectWnd *self = (struct UIControlRectWnd *)window;
    struct UIConnectSnsWnd *wnd;
    float my;
    int mouse_x, rem;
    RECT rc;

    (void)x;
    (void)y;
    if (self->dragging != 1)
        return;
    mouse_x = g_mouse.x;
    rem = (int)((float)mouse_x - self->start_x) % SNAP;
    self->cur_x = (float)(mouse_x - rem);
    my = (float)g_mouse.y;
    self->cur_y = my;
    rc.left = (int)(self->start_x < self->cur_x ? self->start_x : self->cur_x);
    rc.right = (int)(self->start_x > self->cur_x ? self->start_x : self->cur_x);
    rc.top = (int)(self->start_y < my ? self->start_y : my);
    rc.bottom = (int)(self->start_y > my ? self->start_y : my);
    if (self->mode != 0)
        return;
    wnd = (struct UIConnectSnsWnd *)ui_window_find(TWITTER_WINDOW);
    if (!wnd)
        return;
    if (ui_window_find(RECT_WINDOW))
        ui_window_mgr_post_quit1(&g_windowMgr, RECT_WINDOW);
    wnd->capture = rc;
    ui_connect_sns_wnd_save_capture(wnd);
}

void ui_control_rect_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIControlRectWnd *self = (struct UIControlRectWnd *)window;
    ui_control_rect_wnd_draw(self);
    (void)x;
    (void)y;
}

void ui_control_rect_wnd_on_mouse_shape(struct UIWindow *self, int x, int y)
{
    mode_set_cursor_action(client_current_mode(), CURSOR_CROSS);
    (void)self;
    (void)x;
    (void)y;
}

void ui_connect_sns_wnd_destruct(struct UIConnectSnsWnd *self)
{
    SNS(self)->ops = &ui_connect_sns_wnd_ops;
    string_free(&self->image);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_connect_sns_wnd_destroy(struct UIWindow *window)
{
    struct UIConnectSnsWnd *self = (struct UIConnectSnsWnd *)window;
    ui_connect_sns_wnd_destruct(self);
    mem_free(self);
}

void ui_connect_sns_wnd_resize(struct UIWindow *self, int width, int height)
{
    (void)self;
    (void)width;
    (void)height;
}

void ui_connect_sns_wnd_save_capture(struct UIConnectSnsWnd *self)
{
    struct String dir, base, ext, num, tmp, path, next;
    struct BitmapRes *bitmap;
    RECT *rc = &self->capture;

    if (rc->right - rc->left < 1 || rc->bottom - rc->top < 1) {
        ui_window_set_show_base(&self->frame.base, 1);
        ui_window_invalidate_base(&self->frame.base);
        return;
    }
    string_init(&dir);
    string_format(&dir, "%s\\ScreenShot", g_clientDir);
    if (_access(string_cstr(&dir), 0) != 0 && _mkdir(string_cstr(&dir)) != 0) {
        string_free(&dir);
        return;
    }
    string_init(&base);
    string_assign_cstr(&base, "\\");
    string_insert(&base, 0, string_cstr(&dir), dir.size);
    string_append_cstr(&base, "ForSns");
    string_append_cstr(&base, "screen");
    string_init(&ext);
    string_assign_cstr(&ext, ".jpg");
    string_init(&num);
    string_format(&num, "%03d", g_snsCaptureCount++);
    string_append(&base, string_cstr(&num), num.size);
    string_move(&tmp, &base);
    string_append(&tmp, string_cstr(&ext), ext.size);
    string_move(&path, &tmp);
    string_free(&num);
    while (_access(string_cstr(&path), 0) == 0) {
        string_format(&num, "%03d", g_snsCaptureCount++);
        string_init(&next);
        string_append(&next, string_cstr(&base), base.size);
        string_append(&next, string_cstr(&num), num.size);
        string_append(&next, string_cstr(&ext), ext.size);
        string_free(&path);
        path = next;
        string_free(&num);
    }

    if (renderer_save_jpeg_region(g_renderer, string_cstr(&path), rc)
        == 1) {
        string_clear(&self->image);
        string_assign(&self->image, string_cstr(&path), path.size);
        string_append_cstr(&path, " is Saved.");
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)string_cstr(&path), 0xff00, 0, 0);
        bitmap = res_mgr_get(g_resMgr(), string_cstr(&self->image));
        if (bitmap)
            ui_window_resize(&self->frame.base, bitmap->width, bitmap->height);
    } else {
        string_append_cstr(&path, " failed to save");
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)string_cstr(&path), 0xff00, 0, 0);
        string_clear(&self->image);
        ui_window_resize(&self->frame.base, 0x12c, 0xe2);
    }
    ui_window_set_show_base(&self->frame.base, 1);
    ui_window_invalidate_base(&self->frame.base);
    string_free(&path);
    string_free(&ext);
    string_free(&base);
    string_free(&dir);
}

struct UITwitterScrollBar *ui_twitter_scroll_bar_construct(struct UITwitterScrollBar *self)
{
    ui_scroll_bar_construct(&self->bar);
    self->bar.base.ops = &ui_twitter_scroll_bar_ops;
    self->bar.button_size = 5;
    self->colour = 0xfff7cfbd;
    return self;
}

void ui_twitter_scroll_bar_destroy(struct UIWindow *window)
{
    struct UITwitterScrollBar *self = (struct UITwitterScrollBar *)window;
    ui_scroll_bar_destruct(&self->bar);
    mem_free(self);
}

void ui_twitter_scroll_bar_on_draw(struct UIWindow *window)
{
    struct UITwitterScrollBar *self = (struct UITwitterScrollBar *)window;
    int pos;

    ui_window_clear_canvas(&self->bar.base, self->colour);
    if (!self->bar.max_pos)
        return;
    if (self->bar.orientation == UI_SCROLL_VERT) {
        pos = ui_scroll_bar_thumb_pos(&self->bar);
        if (pos < 5)
            pos = 5;
        ui_scroll_bar_get_thumb_size(&self->bar);
        ui_window_draw_bitmap(&self->bar.base, 1, pos, ui_skin_bitmap(SNS_DIR "scrollBar.bmp"), 0);
    }
    self->bar.min_thumb = 0;
}

void ui_twitter_edit_ctrl3_destroy(struct UIWindow *window)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)window;
    ui_edit_ctrl3_destruct(&self->edit.base);
    mem_free(self);
}

void ui_twitter_edit_ctrl3_on_create(struct UIWindow *window, int width, int height)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)window;
    struct UITwitterScrollBar *mem = mem_alloc(sizeof(struct UITwitterScrollBar));
    struct UITwitterScrollBar *bar = mem ? ui_twitter_scroll_bar_construct(mem) : NULL;

    self->bar = &bar->bar;
    ui_scroll_bar_create(&bar->bar, width - 5, 0, UI_SCROLL_VERT, height);
    ui_window_add_child(&self->edit.base, &self->bar->base);
}

void ui_twitter_edit_ctrl3_on_draw(struct UIWindow *window)
{
    struct UIEditCtrl3 *self = (struct UIEditCtrl3 *)window;
    struct String text;
    int height, width;

    ui_edit_ctrl3_on_draw(&self->edit.base);
    string_init(&text);
    string_format(&text, "%d", TWEET_MAX_CHARS - ui_edit_ctrl_get_text_size(&self->edit.base));
    height = ui_window_text_height(&self->edit.base, string_cstr(&text), 0, 0, 0xc, 0, 0);
    width = ui_window_text_width(&self->edit.base, string_cstr(&text), 0, 0, 0xc, 0, 0);
    ui_window_text_out_right(&self->edit.base, self->edit.base.width - width - 3, self->edit.base.height - height,
                          string_cstr(&text), 0, 0, 0xa, 0xd6673a, 0);
    string_free(&text);
}

static void ui_twitter_edit_ctrl3_update_bar(struct UIEditCtrl3 *self)
{
    int visible, max_pos;

    if (!self->bar)
        return;
    visible = self->edit.base.height / self->line_height;
    max_pos = self->first_line != 0 ? self->line_count - visible : 0;
    if (max_pos < 0)
        max_pos = 0;
    self->scrolls = max_pos != 0;
    if (max_pos) {
        ui_scroll_bar_set_range(self->bar, max_pos, visible, self->line_count);
        ui_scroll_bar_set_pos(self->bar, self->first_line);
    } else {
        ui_scroll_bar_set_range(self->bar, 0, visible, self->line_count);
        self->first_line = 0;
    }
    ui_window_invalidate(&self->bar->base);
}

struct UIConnectTwitterWnd *ui_connect_twitter_wnd_construct(struct UIConnectTwitterWnd *self)
{
    ui_frame_wnd_construct(&self->sns.frame, 0);
    TW(self)->ops = &ui_connect_sns_wnd_ops;
    string_init(&self->sns.image);
    TW(self)->ops = &ui_connect_twitter_wnd_ops;
    self->edit = NULL;
    self->capture = NULL;
    self->capture_part = NULL;
    self->capture_kind = 0;
    self->send = NULL;
    self->close = NULL;
    self->auth = NULL;
    self->margin = 9;
    self->bottom_gap = 0x1d;
    self->edit_inset = 2;
    self->first_draw = 1;
    return self;
}

struct UIWindow *ui_connect_twitter_wnd_new(void)
{
    return (struct UIWindow *)ui_connect_twitter_wnd_construct(mem_alloc(sizeof(struct UIConnectTwitterWnd)));
}

void ui_connect_twitter_wnd_destruct(struct UIConnectTwitterWnd *self)
{
    TW(self)->ops = &ui_connect_twitter_wnd_ops;
    ui_connect_sns_wnd_destruct(&self->sns);
}

void ui_connect_twitter_wnd_destroy(struct UIWindow *window)
{
    struct UIConnectTwitterWnd *self = (struct UIConnectTwitterWnd *)window;
    ui_connect_twitter_wnd_destruct(self);
    mem_free(self);
}

int ui_connect_twitter_wnd_caution_height(struct UIConnectTwitterWnd *self, int width)
{
    struct TextArray lines = { 0 };
    int height;

    ui_window_break_lines(TW(self), msg_str(MSG_CAUTION), &lines,
                        width - 2 * self->margin - TWEET_INDENT);
    height = (int)lines.size * 0xd;
    text_array_clear(&lines);
    return height;
}

static void ui_connect_twitter_wnd_fit_image( int *width, int *height)
{
    float w = (float)*width, h = (float)*height;

    if (w > 582.0f) {
        h = h / w * 582.0f;
        w = 582.0f;
    }
    if (h > 436.0f) {
        w = w / h * 436.0f;
        h = 436.0f;
    }
    *width = (int)w;
    *height = (int)h;
}

static void ui_connect_twitter_wnd_min_size( int *width, int *height)
{
    if (*width < 0x12c)
        *width = 0x12c;
    if (*height < 0xe2)
        *height = 0xe2;
}

struct WebBrowser *ui_connect_twitter_wnd_open_browser(void)
{
    struct WebBrowser *browser, *mem;
    int width = 0x26c, height = 0x352;

    if (g_renderer->width < width)
        width = g_renderer->width;
    if (g_renderer->height < height)
        height = g_renderer->height;
    if (g_3dDevice.fullscreen)
        return ui_window_mgr_get_login_browser(&g_windowMgr, width, height);
    browser = ui_window_mgr_find_web_browser(&g_windowMgr, g_hMainWnd);
    if (browser)
        return browser;
    mem = malloc(sizeof *mem);
    if (!mem) return NULL;
    browser = web_browser_construct(mem);
    if (!web_browser_create_child(browser, g_hMainWnd, g_hInstance, width, height)
        || !ui_window_mgr_add_web_browser(&g_windowMgr, browser->hwnd, browser)) {
        web_browser_destruct(browser);
        free(browser);
        return NULL;
    }
    return browser;
}

static struct UIBitmapButton *add_sns_button(struct UIConnectTwitterWnd *self, const char *normal, const char *over,
                                             const char *press, int tooltip, int id)
{
    struct UIBitmapButton *mem = mem_alloc(sizeof(struct UIBitmapButton));
    struct UIBitmapButton *btn = mem ? ui_bitmap_button_construct(mem) : NULL;

    ui_bitmap_button_set_bitmap(btn, normal, 0, 0);
    ui_bitmap_button_set_bitmap(btn, over, 1, 0);
    ui_bitmap_button_set_bitmap(btn, press, 2, 0);
    ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
    ui_bitmap_button_set_tool_tip(btn, msg_str(tooltip));
    btn->button.base.id = id;
    ui_window_add_child(TW(self), &btn->button.base);
    return btn;
}

void ui_connect_twitter_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIConnectTwitterWnd *self = (struct UIConnectTwitterWnd *)window;
    struct UIEditCtrl3 *mem;
    struct UICheckBox *box;
    int r, g, b = 0;

    (void)width;
    (void)height;
    self->close = ui_window_add_close_button(TW(self), "", 0);

    mem = mem_alloc(sizeof(struct UIEditCtrl3));
    if (mem) {
        ui_edit_ctrl3_construct(&mem->edit.base);
        mem->edit.base.ops = &ui_twitter_edit_ctrl3_ops.edit.base;
    }
    self->edit = mem;
    ui_window_create(&mem->edit.base, 0x12c - 2 * self->edit_inset, 0x58 - 2 * self->edit_inset);
    self->edit->edit.multiline = 1;
    self->edit->edit.max_chars = TWEET_MAX_CHARS;
    ui_skin_color_chip(&g_skinMgr, 6, 1, &r, &g, &b);
    ui_edit_ctrl_set_back_colour(&self->edit->edit.base, r + 0x4d, g + 0x30, b + 0xa);
    ui_edit_ctrl_set_text_colour(&self->edit->edit.base, 0x31, 0x31, 0x31, -1);
    if (self->edit->bar)
        ((struct UITwitterScrollBar *)self->edit->bar)->colour = edit_back_colour(&self->edit->edit);
    ui_window_add_child(TW(self), &self->edit->edit.base);
    self->edit->edit.disabled = 1;
    ui_window_invalidate_base(&self->edit->edit.base);
    ui_window_mgr_set_focus_edit(&g_windowMgr, &self->edit->edit.base);

    self->capture = add_sns_button(self, SNS_DIR "btn_capture_normal.bmp", SNS_DIR "btn_capture_over.bmp",
                                   SNS_DIR "btn_capture_press.bmp", MSG_CAPTURE_TIP, ID_CAPTURE);
    self->capture_part = add_sns_button(self, SNS_DIR "btn_capturePart_normal.bmp", SNS_DIR "btn_capturePart_over.bmp",
                                        SNS_DIR "btn_capturePart_press.bmp", MSG_CAPTURE_PART_TIP, ID_CAPTURE_PART);
    self->send = add_sns_button(self, SNS_DIR "btn_send_normal.bmp", SNS_DIR "btn_send_over.bmp",
                                SNS_DIR "btn_send_press.bmp", MSG_SEND_TIP, ID_SEND);

    box = mem_alloc(sizeof(struct UICheckBox));
    if (box)
        box = ui_check_box_construct(&box->base);
    self->auth = box;
    ui_window_create(&box->base, 0x5a, 0xc);
    ui_check_box_set_bitmaps(&self->auth->base, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp", " ConnectSns");
    ui_check_box_set_check(&self->auth->base, g_session1.snsAuthorised);
    ui_window_add_child(TW(self), &self->auth->base);
}

void ui_connect_twitter_wnd_on_draw(struct UIWindow *window)
{
    struct UIConnectTwitterWnd *self = (struct UIConnectTwitterWnd *)window;
    struct TextArray lines = { 0 };
    struct String tag;
    struct BitmapRes *bitmap;
    const char *text;
    int bar_height, w, h, edit_y, box_height, text_x, text_y, half_w, half_h, count, i, y;

    ui_window_clear_canvas(TW(self), 0xffffffff);
    ui_window_draw_title_bar(TW(self), 1, msg_str(MSG_TITLE), 0);
    bar_height = ui_window_draw_button_bar(TW(self), 2, TW(self)->width, 1);
    w = self->sns.capture.right - self->sns.capture.left;
    h = self->sns.capture.bottom - self->sns.capture.top;
    ui_connect_twitter_wnd_fit_image( &w, &h);
    if (w < 1 || h < 1)
        string_clear(&self->sns.image);
    edit_y = self->edit->edit.base.y;

    if (self->sns.image.size) {
        bitmap = res_mgr_get(g_resMgr(), string_cstr(&self->sns.image));
        if (bitmap)
            ui_window_draw_bitmap_scaled(TW(self), TW(self)->width / 2 - w / 2, (edit_y + 0x11) / 2 - h / 2 - 1, w,
                                      h, bitmap);
    } else {
        box_height = edit_y - 2 * self->margin - 0x11;
        ui_window_fill_rect(TW(self), self->margin, self->margin + 0x11, 0x12c, box_height, 0xe7e0df);
        half_h = ui_window_text_height(TW(self), msg_str(MSG_NO_CAPTURE), 0, 0, 0xe, 1, 0) / 2;
        half_w = ui_window_text_width(TW(self), msg_str(MSG_NO_CAPTURE), 0, 0, 0xe, 1, 0) / 2;
        ui_window_text_out(TW(self), TW(self)->width / 2 - half_w,
                         box_height / 2 - half_h + self->margin + 0x11, msg_str(MSG_NO_CAPTURE), 0, 0, 0xe, 0xb3adac,
                         1, 0);
    }

    ui_connect_twitter_wnd_caution_height(self, TW(self)->width);
    text_x = self->edit->edit.base.x + TWEET_INDENT;
    ui_window_break_lines(TW(self), msg_str(MSG_CAUTION), &lines,
                        TW(self)->width - 2 * self->margin - TWEET_INDENT);
    text_y = self->edit->edit.base.height + 2 * (self->edit_inset + 1) + edit_y;
    bitmap = ui_skin_bitmap(SNS_DIR "img_poring2.bmp");
    if (bitmap)
        ui_window_draw_bitmap(TW(self), self->edit->edit.base.x, text_y - bitmap->height + 0x10, bitmap, 0);
    bitmap = ui_skin_bitmap(SNS_DIR "img_poring1.bmp");
    if (bitmap)
        ui_window_draw_bitmap(TW(self), TW(self)->width - bitmap->width,
                            TW(self)->height - bitmap->height - bar_height, bitmap, 0);
    count = (int)lines.size;
    bitmap = ui_skin_bitmap(SNS_DIR "ico_caution.bmp");
    if (bitmap)
        ui_window_draw_bitmap(TW(self), self->edit->edit.base.x, count + text_y, bitmap, 0);
    for (i = 0, y = 0; i < count; i++, y += 0xd) {
        const char *line = text_array_at(&lines, i).data;

        ui_window_text_out(TW(self), text_x + 0xd, count + y + text_y, line, 0, 0, 0xb,
                         0x713c28, 0, 0);
    }

    ui_window_fill_rect(TW(self), self->margin, edit_y - self->edit_inset + 1, TW(self)->width - 2 * self->margin,
                      0x58, edit_back_colour(&self->edit->edit));
    if (self->first_draw == 1) {
        self->first_draw = 0;
        string_init(&tag);
        string_assign_cstr(&tag, "   ");
        string_append_cstr(&tag, "#");
        text = msg_str(MSG_HASHTAG);
        string_append_cstr(&tag, text);
        string_append_cstr(&tag, "#Ragnarok");
        ui_edit_ctrl_set_text(&self->edit->edit, string_cstr(&tag));
        language_set_selection(g_language, 0, 0);
        string_free(&tag);
    }
    if (strlen(ui_edit_ctrl_get_text(&self->edit->edit.base)) == 0)
        ui_edit_ctrl_set_text(&self->edit->edit, msg_str(MSG_EMPTY_TEXT));
    ui_window_text_out(TW(self), 5, TW(self)->height - 0x14, msg_str(MSG_TITLE), 0, 0, 0xc, 0, 0, 0);
    text_array_clear(&lines);
}

void ui_connect_twitter_wnd_resize(struct UIWindow *window, int width, int height)
{
    struct UIConnectTwitterWnd *self = (struct UIConnectTwitterWnd *)window;
    struct UIWindow *edit;
    struct UIBitmapButton *capture;
    int caution, total_w, total_h;

    ui_connect_twitter_wnd_fit_image( &width, &height);
    ui_connect_twitter_wnd_min_size( &width, &height);
    if (self->edit)
        ui_window_resize(&self->edit->edit.base, width - 2 * self->edit_inset, (0x2b - self->edit_inset) * 2);
    total_w = width + 0x12;
    caution = ui_connect_twitter_wnd_caution_height(self, total_w);
    edit = &self->edit->edit.base;
    total_h = height + 0x11 + 3 * self->margin + edit->height + self->bottom_gap + caution;
    ui_window_resize_base(TW(self), total_w, total_h);
    ui_window_move_base(self->close, total_w - 0xe, 3);
    ui_window_move_base(edit, total_w / 2 - edit->width / 2,
             total_h - edit->height - self->edit_inset - self->bottom_gap - self->margin - caution - 1);
    capture = self->capture;
    ui_window_move_base(&capture->button.base, total_w - 3 * capture->bitmap_width - 0xd, total_h - capture->bitmap_height - 4);
    ui_window_move_base(&self->capture_part->button.base, capture->button.base.x + capture->bitmap_width + 4,
             total_h - self->capture_part->bitmap_height - 4);
    ui_window_move_base(&self->send->button.base, capture->button.base.x + 2 * capture->bitmap_width + 8,
             total_h - self->send->bitmap_height - 4);
    ui_window_move_base(&self->auth->base, ui_window_text_width(TW(self), msg_str(MSG_TITLE), 0, 0, 0xc, 0, 0) + 0xb,
             total_h - 0x14);
    ui_frame_wnd_move(&self->sns.frame.base, g_renderer->width / 2 - TW(self)->width / 2,
             g_renderer->height / 2 - TW(self)->height / 2);
}

static void auth_page(char *buf, const char *url)
{
    memset(buf, 0, 0x400);
    _snprintf_s(buf, 0x400, (size_t)-1, "%s?AID=%d&AuthToken=%.*s", url, g_session1.ownAid, (int)sizeof g_session1.webAuthToken, g_session1.webAuthToken);
}

int ui_connect_twitter_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           int val1, int val2, int val3, int val4)
{
    struct UIConnectTwitterWnd *self = (struct UIConnectTwitterWnd *)window;
    struct UIControlRectWnd *picker;
    struct WebBrowser *browser;
    struct UIConnectTwitterWnd *wnd;
    const char *url;
    char buf[0x400];

    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->sns.frame), sender, msg, val1, val2, val3, val4);
    switch (val1) {
    case UI_ID_CLOSE:
        ui_window_mgr_post_quit1(&g_windowMgr, TWITTER_WINDOW);
        break;
    case ID_CAPTURE:
    case ID_CAPTURE_PART:
        ui_window_set_show_base(&self->sns.frame.base, 0);
        picker = (struct UIControlRectWnd *)ui_window_open(RECT_WINDOW);
        if (picker) {
            picker->mode = 0;
            picker->kind = val1 == ID_CAPTURE ? RECT_KIND_SCREEN : RECT_KIND_PART;
            ui_control_rect_wnd_start(picker);
        }
        break;
    case ID_SEND:
        ui_connect_twitter_wnd_send(self);
        break;
    case ID_AUTH:
        if (sender != &self->auth->base)
            break;
        g_session1.snsAuthorised = 0;
        if (self->auth->base.state) {
            ui_check_box_set_check_quiet(self->auth, 0);
            url = twitter_auth_url(g_twitterDataMgr);
            if (!*url) {
                ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(MSG_NO_SERVICE), 0xff, 0, 0);
            } else {
                auth_page(buf, url);
                browser = ui_connect_twitter_wnd_open_browser();
                web_browser_navigate(browser, buf);
            }
        } else {
            browser = ui_connect_twitter_wnd_open_browser();
            if (!browser)
                break;
            url = twitter_auth_url(g_twitterDataMgr);
            if (!*url) {
                ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(MSG_NO_SERVICE), 0xff, 0, 0);
            } else {
                auth_page(buf, url);
                web_browser_navigate(browser, buf);
                web_browser_show(browser, 0);
                ui_window_mgr_delete_web_browser(&g_windowMgr, browser->hwnd);
                wnd = (struct UIConnectTwitterWnd *)ui_window_find(TWITTER_WINDOW);
                if (wnd)
                    ui_check_box_set_check_quiet(wnd->auth, 0);
            }
        }
        break;
    }
    return 0;
}

static void ui_connect_twitter_wnd_send(struct UIConnectTwitterWnd *self)
{
    const char *text;

    if (!g_session1.snsAuthorised) {
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(MSG_NOT_AUTHORISED), 0xff, 0, 0);
        return;
    }
    if (!self->sns.image.size) {
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(MSG_NO_IMAGE), 0x64ffff, 0, 0);
        return;
    }
    if (!twitter_has_service(g_twitterDataMgr)) {
        ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(MSG_NO_SERVICE), 0xff, 0, 0);
        return;
    }
    text = ui_edit_ctrl_get_text(&self->edit->edit.base);
    if (!twitter_send(g_twitterDataMgr, text_view_cstr(text), string_view(&self->sns.image))) {
        twitter_on_sent(g_twitterDataMgr, 0);
        return;
    }
    ui_window_send_msg(&self->sns.frame.base, NULL, UI_MSG_COMMAND, UI_ID_CLOSE, 0, 0, 0);
}

void ui_connect_twitter_wnd_set_authorised(struct UIConnectTwitterWnd *self, BYTE authorised)
{
    ui_check_box_set_check_quiet(self->auth, authorised);
}
