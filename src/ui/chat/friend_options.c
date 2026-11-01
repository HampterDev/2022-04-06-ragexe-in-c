#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/chat/chat.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define FRIEND_OPTION_WND_ID 0x44

#define MSG_FRIEND_OPTIONS_TITLE 0x163
#define MSG_WHISPER_WINDOW_FRIENDS 0x167
#define MSG_WHISPER_WINDOW_STRANGERS 0x169
#define MSG_WHISPER_ALARM 0x16a

struct UIFriendOptionWnd;
struct UIWindow;
void ui_friend_option_wnd_destroy(struct UIWindow *window);
void ui_friend_option_wnd_store_info(struct UIWindow *self);
void ui_friend_option_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_friend_option_wnd_on_draw(struct UIWindow *self);
int ui_friend_option_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_friend_option_wnd_ops = {
    .destroy = ui_friend_option_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_friend_option_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_friend_option_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_friend_option_wnd_on_draw,
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
    .send_msg = ui_friend_option_wnd_send_msg,
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

struct UIFriendOptionWnd {
    struct UIFrameWnd frame;
    struct WindowInfo *info;
    struct UIWindow *alarm_box;
    struct UIWindow *friends_box;
    struct UIWindow *strangers_box;
    struct UIWindow *close_button;
};
OFFSET_ASSERT(UIFriendOptionWnd, info, 0x94);
OFFSET_ASSERT(UIFriendOptionWnd, close_button, 0xa4);
SIZE_ASSERT(UIFriendOptionWnd, 0xa8);

#define FRIEND_OPTION_WND_X 200
#define FRIEND_OPTION_WND_Y 120
#define FRIEND_OPTION_WND_W 0x118
#define FRIEND_OPTION_WND_H 0x78

static int friend_option_info_usable(struct WindowInfo *info)
{
    if (info->width != FRIEND_OPTION_WND_W || info->height != FRIEND_OPTION_WND_H)
        return 0;
    if (info->x < 0 || info->x > g_renderer->width - FRIEND_OPTION_WND_W)
        return 0;
    if (info->y < 0 || info->y > g_renderer->height - FRIEND_OPTION_WND_H)
        return 0;
    return 1;
}

static void friend_option_info_reset(struct WindowInfo *info)
{
    info->x = FRIEND_OPTION_WND_X;
    info->y = FRIEND_OPTION_WND_Y;
    info->width = FRIEND_OPTION_WND_W;
    info->height = FRIEND_OPTION_WND_H;
}

struct UIFriendOptionWnd *ui_friend_option_wnd_construct(struct UIFriendOptionWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_friend_option_wnd_ops;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_friend_option_wnd_new(void)
{
    return (struct UIWindow *)ui_friend_option_wnd_construct(mem_alloc(sizeof(struct UIFriendOptionWnd)));
}

static void ui_friend_option_wnd_destruct(struct UIFriendOptionWnd *self)
{
    self->frame.base.ops = &ui_friend_option_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_friend_option_wnd_destroy(struct UIWindow *window)
{
    struct UIFriendOptionWnd *self = (struct UIFriendOptionWnd *)window;
    ui_friend_option_wnd_destruct(self);
    mem_free(self);
}

static struct UIWindow *add_option_box(struct UIFriendOptionWnd *self, int label_id, int y, int checked)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *box;
    int width = ui_window_text_width(wnd, msg_str(label_id), 0, 0, 12, 0, 0) + 12;

    if (width >= wnd->width - 15)
        width = wnd->width - 15;
    box = mem_alloc(sizeof(struct UICheckBox));
    if (box)
        box = (struct UIWindow *)ui_check_box_construct(box);
    ui_window_create(box, width, 12);
    ui_check_box_set_bitmaps(box, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp", msg_str(label_id));
    ui_window_move_base(box, 6, y);
    ui_check_box_set_check(box, checked);
    ui_window_add_child(wnd, box);
    return box;
}

void ui_friend_option_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIFriendOptionWnd *self = (struct UIFriendOptionWnd *)window;
    self->strangers_box = add_option_box(self, MSG_WHISPER_WINDOW_STRANGERS, 0x15, g_session1.showWhisperWnd);
    self->friends_box = add_option_box(self, MSG_WHISPER_WINDOW_FRIENDS, 0x25, g_session1.showWhisperWndFriend);
    self->alarm_box = add_option_box(self, MSG_WHISPER_ALARM, 0x35, g_session1.playWhisperOpenSound);
    self->close_button = ui_window_add_close_button(&self->frame.base, "Alt+I", 0);
}

void ui_friend_option_wnd_on_draw(struct UIWindow *window)
{
    struct UIFriendOptionWnd *self = (struct UIFriendOptionWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    int title_height = ui_window_draw_title_bar(wnd, 1, msg_str(MSG_FRIEND_OPTIONS_TITLE), 0);
    int bar_height;

    ui_window_draw_body(wnd, title_height, 0, "");
    bar_height = ui_window_draw_button_bar(wnd, 0, 0, 1);
    ui_window_fill_rect(wnd, 0, wnd->height - bar_height, wnd->width, 1, RGB(0xc0, 0xc0, 0xc0));
}

int ui_friend_option_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                         int val1, int val2, int val3, int val4)
{
    struct UIFriendOptionWnd *self = (struct UIFriendOptionWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        if (val1 == UI_ID_CLOSE) {
            ui_window_mgr_post_quit1(&g_windowMgr, FRIEND_OPTION_WND_ID);
        } else if (val1 == UI_ID_CHECKBOX) {
            if (sender == self->strangers_box)
                g_session1.showWhisperWnd = val2;
            else if (sender == self->friends_box)
                g_session1.showWhisperWndFriend = val2;
            else if (sender == self->alarm_box)
                g_session1.playWhisperOpenSound = val2;
        }
        return 0;
    }
    if (msg == UI_MSG_SET_INFO) {
        self->info = (struct WindowInfo *)val1;
        if (!friend_option_info_usable(self->info))
            friend_option_info_reset(self->info);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_friend_option_wnd_store_info(struct UIWindow *window)
{
    struct UIFriendOptionWnd *self = (struct UIFriendOptionWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
}
