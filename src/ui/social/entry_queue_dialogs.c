#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "ui/social/entry_queue_standby_internal.h"
#include "ui/social/entry_queue_windows.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include <string.h>
#include "ui/social/social.h"
#include "ui/social/entry_queue_state.h"
#include "core/msg_str.h"
#include "diag/perf.h"

static __inline void ui_text_add_line(struct UIWindow *text, const char *line)
{
    ui_window_send_msg(text, NULL, UI_MSG_ADD_TEXT, (int)line, 0x26, 0, 0);
}

static void fixed_window_setup(struct UIWindow *wnd, int width, int height, int close_button, int x, int y)
{
    ui_window_resize(wnd, width, height);
    ui_window_add_sys_button(wnd);
    if (close_button)
        ui_window_add_close_button(wnd, "", 0);
    ui_window_create(wnd, width, height);
    ui_window_on_create(wnd, width, height);
    ui_window_move(wnd, (g_renderer->width - 640) / 2 + x, (g_renderer->height - 480) / 2 + y);
}

static void draw_titled(struct UIWindow *wnd, int title)
{
    ui_window_draw_title_bar(wnd, 1, msg_str(title), 0);
    ui_window_draw_body(wnd, 0x11, 0, "");
}

#define MSG_ENTRY_HELP_FIRST 0x844
#define MSG_ENTRY_HELP_LAST 0x84c
#define MSG_ENTRY_HELP_TITLE 0x84d

struct UIEntryQueueHelpWnd {
    struct UIFrameWnd frame;
    struct UIWindow *text;
    struct UIBitmapButton *close;
};
SIZE_ASSERT(UIEntryQueueHelpWnd, 0x9c);
void ui_entry_queue_help_wnd_destroy(struct UIWindow *window);
void ui_entry_queue_help_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_entry_queue_help_wnd_on_destroy(struct UIWindow *self);
void ui_entry_queue_help_wnd_on_process(struct UIWindow *self);
void ui_entry_queue_help_wnd_on_draw(struct UIWindow *self);
int ui_entry_queue_help_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_entry_queue_help_wnd_ops = {
    .destroy = ui_entry_queue_help_wnd_destroy,
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
    .on_create = ui_entry_queue_help_wnd_on_create,
    .on_destroy = ui_entry_queue_help_wnd_on_destroy,
    .on_process = ui_entry_queue_help_wnd_on_process,
    .on_draw = ui_entry_queue_help_wnd_on_draw,
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
    .send_msg = ui_entry_queue_help_wnd_send_msg,
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

struct UIEntryQueueHelpWnd *ui_entry_queue_help_wnd_construct(struct UIEntryQueueHelpWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_entry_queue_help_wnd_ops;
    self->text = NULL;
    self->close = NULL;
    return self;
}

struct UIWindow *ui_entry_queue_help_wnd_new(void)
{
    return (struct UIWindow *)ui_entry_queue_help_wnd_construct(mem_alloc(sizeof(struct UIEntryQueueHelpWnd)));
}

static void ui_entry_queue_help_wnd_destruct(struct UIEntryQueueHelpWnd *self)
{
    self->frame.base.ops = &ui_entry_queue_help_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_entry_queue_help_wnd_destroy(struct UIWindow *window)
{
    struct UIEntryQueueHelpWnd *self = (struct UIEntryQueueHelpWnd *)window;
    ui_entry_queue_help_wnd_destruct(self);
    mem_free(self);
}

void ui_entry_queue_help_wnd_on_create(struct UIWindow *window, int width_unused,
                                             int height_unused)
{
    struct UIEntryQueueHelpWnd *self = (struct UIEntryQueueHelpWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    char line[0x104];
    int id;

    ui_window_resize_base(wnd, 0x118, 0x118);
    self->text = ui_add_text_viewer(wnd, 0xa, 0x16, 0x10e, 0xe1);
    memset(line, 0, sizeof line);
    for (id = MSG_ENTRY_HELP_FIRST; id <= MSG_ENTRY_HELP_LAST; id++) {
        strcpy_s(line, sizeof line, msg_str(id));
        ui_text_add_line(self->text, line);
    }
    self->close = ui_add_bitmap_button(wnd, UI_SKIN "btn_close.bmp", UI_SKIN "btn_close_a.bmp", UI_SKIN "btn_close_b.bmp",
                             0xea, 0x100, 0);
}

void ui_entry_queue_help_wnd_on_destroy(struct UIWindow *self)
{
}
void ui_entry_queue_help_wnd_on_process(struct UIWindow *self)
{
}

void ui_entry_queue_help_wnd_on_draw(struct UIWindow *window)
{
    struct UIEntryQueueHelpWnd *self = (struct UIEntryQueueHelpWnd *)window;
    draw_titled(&self->frame.base, MSG_ENTRY_HELP_TITLE);
}

int ui_entry_queue_help_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                           int val1, int val2, int val3, int val4)
{
    if (msg == UI_MSG_COMMAND && val1 == 0)
        ui_window_mgr_post_quit1(&g_windowMgr, ENTRY_QUEUE_HELP_WND_ID);
    return 0;
}

#define MSG_ENTRY_REQUEST_QUEUE 0x84e
#define MSG_ENTRY_REQUEST_LINE1 0x84f
#define MSG_ENTRY_REQUEST_LINE2 0x850
#define MSG_ENTRY_REQUEST_LINE5 0x853
#define MSG_ENTRY_REQUEST_ENTERED 0x854
#define MSG_ENTRY_REQUEST_TITLE 0x855
#define MSG_ENTRY_REQUEST_COUNTDOWN 0x856
#define ENTRY_REQUEST_SECONDS 20

#define ENTRY_REQUEST_ID_OK 0
#define ENTRY_REQUEST_ID_CANCEL 1

struct UIEntryQueueRequestWnd {
    struct UIFrameWnd frame;
    struct UIWindow *text;
    struct UIBitmapButton *ok;
    struct UIBitmapButton *cancel;
    unsigned interval;
    unsigned elapsed;
    unsigned since_step;
    DWORD last_step;
    struct UIWindow *countdown;
};
OFFSET_ASSERT(UIEntryQueueRequestWnd, interval, 0xa0);
OFFSET_ASSERT(UIEntryQueueRequestWnd, countdown, 0xb0);
SIZE_ASSERT(UIEntryQueueRequestWnd, 0xb4);
void ui_entry_queue_request_wnd_destroy(struct UIWindow *window);
void ui_entry_queue_request_wnd_on_destroy(struct UIWindow *self);
void ui_entry_queue_request_wnd_on_process(struct UIWindow *self);
void ui_entry_queue_request_wnd_on_draw(struct UIWindow *self);
int ui_entry_queue_request_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_entry_queue_request_wnd_ops = {
    .destroy = ui_entry_queue_request_wnd_destroy,
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
    .on_destroy = ui_entry_queue_request_wnd_on_destroy,
    .on_process = ui_entry_queue_request_wnd_on_process,
    .on_draw = ui_entry_queue_request_wnd_on_draw,
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
    .send_msg = ui_entry_queue_request_wnd_send_msg,
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

struct UIEntryQueueRequestWnd *ui_entry_queue_request_wnd_construct(struct UIEntryQueueRequestWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_entry_queue_request_wnd_ops;
    self->interval = 1000;
    self->elapsed = 0;
    self->since_step = 0;
    self->last_step = 0;
    return self;
}

struct UIWindow *ui_entry_queue_request_wnd_new(void)
{
    return (struct UIWindow *)ui_entry_queue_request_wnd_construct(mem_alloc(sizeof(struct UIEntryQueueRequestWnd)));
}

static void ui_entry_queue_request_wnd_destruct(struct UIEntryQueueRequestWnd *self)
{
    self->frame.base.ops = &ui_entry_queue_request_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_entry_queue_request_wnd_destroy(struct UIWindow *window)
{
    struct UIEntryQueueRequestWnd *self = (struct UIEntryQueueRequestWnd *)window;
    ui_entry_queue_request_wnd_destruct(self);
    mem_free(self);
}

BYTE ui_entry_queue_request_wnd_init(struct UIEntryQueueRequestWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *countdown;
    char line[0x104];
    int id;

    fixed_window_setup(wnd, 0x12c, 0xfa, 1, 0xb9, 0xb4);
    self->text = ui_add_text_viewer(wnd, 0xa, 0x16, 0x118, 0x131);
    memset(line, 0, sizeof line);
    if (g_entryQueueMgr && g_entryQueueMgr->current)
        sprintf_s(line, sizeof line, msg_str(MSG_ENTRY_REQUEST_QUEUE), g_entryQueueMgr->current->title);
    ui_list_box_clear((struct UIListBox *)self->text);
    ui_text_add_line(self->text, line);
    ui_text_add_line(self->text, msg_str(MSG_ENTRY_REQUEST_LINE1));
    ui_text_add_line(self->text, "");
    for (id = MSG_ENTRY_REQUEST_LINE2; id <= MSG_ENTRY_REQUEST_LINE5; id++)
        ui_text_add_line(self->text, msg_str(id));

    self->ok = ui_add_bitmap_button(wnd, UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp", 0xd1, 0xe2,
                          ENTRY_REQUEST_ID_OK);
    self->cancel = ui_add_bitmap_button(wnd, UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp",
                              UI_SKIN "btn_cancel_b.bmp", 0xfe, 0xe2, ENTRY_REQUEST_ID_CANCEL);

    countdown = mem_alloc(sizeof(struct UIStaticText));
    if (countdown)
        countdown = ui_static_text_construct(countdown);
    self->countdown = countdown;
    ui_window_create(countdown, 0x96, 0x14);
    ui_window_move(countdown, 0xa, wnd->height - 0x14);
    ui_window_add_child(wnd, countdown);
    return 1;
}

void ui_entry_queue_request_wnd_on_destroy(struct UIWindow *self)
{
}

void ui_entry_queue_request_wnd_on_process(struct UIWindow *window)
{
    struct UIEntryQueueRequestWnd *self = (struct UIEntryQueueRequestWnd *)window;
    DWORD now = GetTickCount();

    self->since_step = now - self->last_step;
    if (self->since_step > self->interval) {
        self->elapsed += self->interval;
        self->last_step = now;
        ui_window_invalidate_base(&self->frame.base);
    }
}

void ui_entry_queue_request_wnd_on_draw(struct UIWindow *window)
{
    struct UIEntryQueueRequestWnd *self = (struct UIEntryQueueRequestWnd *)window;
    char text[0x64];
    int left;

    draw_titled(&self->frame.base, MSG_ENTRY_REQUEST_TITLE);
    memset(text, 0, sizeof text);
    left = ENTRY_REQUEST_SECONDS - (int)(self->elapsed / 1000 % 60);
    if (left <= 0)
        ui_entry_queue_request_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, ENTRY_REQUEST_ID_CANCEL, 0, 0, 0);
    sprintf(text, msg_str(MSG_ENTRY_REQUEST_COUNTDOWN), left);
    ui_static_text_set_text(self->countdown, text, 0);
}

int ui_entry_queue_request_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                              int msg, int val1, int val2, int val3, int val4)
{
    struct UIEntryQueueRequestWnd *self = (struct UIEntryQueueRequestWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return 0;
    ui_window_record_command(&self->frame.base, val1, val2, val3, val4);
    if (val1 == ENTRY_REQUEST_ID_OK) {
        entry_queue_mgr_answer(g_entryQueueMgr, ENTRY_ANSWER_ENTER);
        ui_window_mgr_post_quit1(&g_windowMgr, ENTRY_QUEUE_STANDBY_WND_ID);
        ui_window_mgr_post_quit1(&g_windowMgr, ENTRY_QUEUE_REQUEST_WND_ID);
        ui_chat_line(msg_str(MSG_ENTRY_REQUEST_ENTERED), 0x64ffff);
    } else if (val1 == ENTRY_REQUEST_ID_CANCEL) {
        entry_queue_mgr_answer(g_entryQueueMgr, ENTRY_ANSWER_DECLINE);
        ui_window_mgr_post_quit1(&g_windowMgr, ENTRY_QUEUE_STANDBY_WND_ID);
        ui_window_mgr_post_quit1(&g_windowMgr, ENTRY_QUEUE_REQUEST_WND_ID);
    }
    return 0;
}

#define MSG_ENTRY_STANDBY_TITLE 0x857

#define ENTRY_STANDBY_ID_LEAVE 0
#define ENTRY_STANDBY_ID_CLOSE 1

void ui_entry_queue_stand_by_wnd_destroy(struct UIWindow *window);
void ui_entry_queue_stand_by_wnd_on_destroy(struct UIWindow *self);
void ui_entry_queue_stand_by_wnd_on_process(struct UIWindow *self);
void ui_entry_queue_stand_by_wnd_on_draw(struct UIWindow *self);
int ui_entry_queue_stand_by_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_entry_queue_stand_by_wnd_ops = {
    .destroy = ui_entry_queue_stand_by_wnd_destroy,
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
    .on_destroy = ui_entry_queue_stand_by_wnd_on_destroy,
    .on_process = ui_entry_queue_stand_by_wnd_on_process,
    .on_draw = ui_entry_queue_stand_by_wnd_on_draw,
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
    .send_msg = ui_entry_queue_stand_by_wnd_send_msg,
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

struct UIEntryQueueStandByWnd *ui_entry_queue_stand_by_wnd_construct(struct UIEntryQueueStandByWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_entry_queue_stand_by_wnd_ops;
    string_init(&self->queue);
    self->arg1 = 0;
    self->arg2 = 0;
    self->text = NULL;
    memset(self->lines, 0, sizeof self->lines);
    return self;
}

struct UIWindow *ui_entry_queue_stand_by_wnd_new(void)
{
    return (struct UIWindow *)ui_entry_queue_stand_by_wnd_construct(mem_alloc(sizeof(struct UIEntryQueueStandByWnd)));
}

static void ui_entry_queue_stand_by_wnd_destruct(struct UIEntryQueueStandByWnd *self)
{
    self->frame.base.ops = &ui_entry_queue_stand_by_wnd_ops;
    string_free(&self->queue);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_entry_queue_stand_by_wnd_destroy(struct UIWindow *window)
{
    struct UIEntryQueueStandByWnd *self = (struct UIEntryQueueStandByWnd *)window;
    ui_entry_queue_stand_by_wnd_destruct(self);
    mem_free(self);
}

BYTE ui_entry_queue_stand_by_wnd_init(struct UIEntryQueueStandByWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;

    fixed_window_setup(wnd, 0x118, 0x78, 0, 0xb9, 0xb4);
    self->text = ui_add_text_viewer(wnd, 0xa, 0x16, 0x104, 0x41);
    self->leave = ui_add_bitmap_button(wnd, UI_SKIN "cancel_waiting.bmp", UI_SKIN "cancel_waiting_a.bmp",
                             UI_SKIN "cancel_waiting_b.bmp", 0x9f, 0x60, ENTRY_STANDBY_ID_LEAVE);
    self->close = ui_add_bitmap_button(wnd, UI_SKIN "btn_close.bmp", UI_SKIN "btn_close_a.bmp", UI_SKIN "btn_close_b.bmp",
                             0xea, 0x60, ENTRY_STANDBY_ID_CLOSE);
    return 1;
}

void ui_entry_queue_stand_by_wnd_on_destroy(struct UIWindow *self)
{
}
void ui_entry_queue_stand_by_wnd_on_process(struct UIWindow *self)
{
}

void ui_entry_queue_stand_by_wnd_on_draw(struct UIWindow *window)
{
    struct UIEntryQueueStandByWnd *self = (struct UIEntryQueueStandByWnd *)window;
    draw_titled(&self->frame.base, MSG_ENTRY_STANDBY_TITLE);
}

int ui_entry_queue_stand_by_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                              int msg, int val1, int val2, int val3, int val4)
{
    struct UIEntryQueueStandByWnd *self = (struct UIEntryQueueStandByWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return 0;
    ui_window_record_command(&self->frame.base, val1, val2, val3, val4);
    if (val1 == ENTRY_STANDBY_ID_LEAVE)
        entry_queue_mgr_cancel(g_entryQueueMgr);
    else if (val1 == ENTRY_STANDBY_ID_CLOSE)
        ui_window_mgr_post_quit1(&g_windowMgr, ENTRY_QUEUE_STANDBY_WND_ID);
    return 0;
}
