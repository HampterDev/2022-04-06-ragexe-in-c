#include "ui/controls/listbox.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include <string.h>
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/controls/controls.h"
#include "ui/character/character.h"
#include "ui/ui_items.h"
#include "render/renderer.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "platform/clock.h"

#define MSGBOX_OK 0
#define MSGBOX_YES_NO 2
#define MSGBOX_CANCEL 4
#define MSGBOX_ID_OK 0xb0
#define MSGBOX_ID_CANCEL 0xb1
#define MSGBOX_ID_YES 0xb3
#define MSGBOX_ID_NO 0xb4
#define MSGBOX_MSG_SET_TEXT 0x39
#define MSGBOX_MSG_SET_FORMAT 0x43
#define MSGBOX_MSG_SET_MODAL 0x9c
#define MSGBOX_MSG_SET_FC 0x9d
#define MSGBOX_MSG_GET_FC 0x9e
#define MSGBOX_TIMEOUT_RESULT MSGBOX_ID_YES
#define MSGBOX_TEXT_CODE 0x26

struct UIMessageBox {
    struct UIFrameWnd frame;
    struct UIWindow *viewer;
    struct UIWindow *gauge;
    int counting;
    DWORD start;
    DWORD end;
    int total;
    int shown;
    struct String format;
    struct String text;
    struct String title;
    int modal;
    BYTE keep_text;
};

void ui_message_box_destroy(struct UIWindow *window);
void ui_message_box_on_destroy(struct UIWindow *self);
void ui_message_box_on_process(struct UIWindow *self);
void ui_message_box_on_draw(struct UIWindow *self);
intptr_t ui_message_box_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_message_box_ops = {
    .destroy = ui_message_box_destroy,
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
    .on_destroy = ui_message_box_on_destroy,
    .on_process = ui_message_box_on_process,
    .on_draw = ui_message_box_on_draw,
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
    .send_msg = ui_message_box_send_msg,
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
void ui_message_box_autoreturn_destroy(struct UIWindow *window);
void ui_message_box_autoreturn_on_process(struct UIWindow *self);
void ui_message_box_autoreturn_on_draw(struct UIWindow *self);
intptr_t ui_message_box_autoreturn_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_message_box_autoreturn_ops = {
    .destroy = ui_message_box_autoreturn_destroy,
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
    .on_process = ui_message_box_autoreturn_on_process,
    .on_draw = ui_message_box_autoreturn_on_draw,
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
    .send_msg = ui_message_box_autoreturn_send_msg,
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

static void msgbox_frame(struct UIMessageBox *self, const struct UIWindowOps *ops)
{
    ui_window_construct(&self->frame.base, 0);
    self->frame.base.ops = &ui_frame_wnd_ops;
    self->frame.press_x = 0;
    self->frame.press_y = 0;
    self->frame.default_id = UI_ID_NONE;
    self->frame.cancel_id = UI_ID_NONE;
    self->frame.base.ops = ops;
}

static int msgbox_seconds(DWORD start, DWORD end)
{
    DWORD left = end - start;

    return (int)(left / 1000 + (left % 1000 != 0));
}

static void msgbox_start(struct UIMessageBox *self, const char *text, DWORD duration)
{
    string_assign_cstr(&self->text, text);
    string_assign_cstr(&self->format, text);
    self->start = platform_ticks_ms();
    self->end = platform_ticks_ms() + duration;
    self->shown = msgbox_seconds(self->start, self->end);
}

static void msgbox_start_count(struct UIMessageBox *self, DWORD duration)
{
    char buf[0x200];

    if (duration) {
        self->counting = 1;
        sprintf(buf, string_cstr(&self->format), self->shown);
        string_assign_cstr(&self->text, buf);
    } else {
        self->counting = 0;
    }
}

static void msgbox_centre(struct UIMessageBox *self)
{
    ui_frame_wnd_move(&self->frame.base, (g_renderer->width - 640) / 2 + 0xb9, (g_renderer->height - 480) / 2 + 0xb4);
}

static void msgbox_viewer(struct UIMessageBox *self, int x, int y, int w, int h)
{
    struct UIWindow *viewer = mem_alloc(sizeof(struct UITextViewer));

    if (viewer)
        viewer = ui_text_viewer_construct(viewer);
    self->viewer = viewer;
    ui_list_box_create_window((struct UIListBox *)viewer, x, y, w, h, 0);
    UI_TEXT_VIEWER_BACKGROUND(self->viewer) = ui_skin_argb(2, 2);
    ui_window_invalidate(self->viewer);
    ui_window_add_child(&self->frame.base, self->viewer);
}

static int msgbox_tick(struct UIMessageBox *self)
{
    char buf[0x200];
    int last, secs, round_up;

    if (platform_ticks_ms() > self->end) {
        g_windowMgr.modal_result = MSGBOX_TIMEOUT_RESULT;
        self->counting = 0;
        return 0;
    }
    last = self->shown;
    round_up = (self->end - platform_ticks_ms()) % 1000 != 0;
    secs = (int)((self->end - platform_ticks_ms()) / 1000) + round_up;
    self->shown = secs;
    if (last != secs) {
        sprintf(buf, string_cstr(&self->format), secs);
        string_assign_cstr(&self->text, buf);
        ui_list_box_clear((struct UIListBox *)self->viewer);
        ui_window_send_msg(self->viewer, NULL, UI_MSG_ADD_TEXT, (intptr_t)string_cstr(&self->text), MSGBOX_TEXT_CODE,
                 0, 0);
    }
    return 1;
}

static struct UIMessageBox *ui_message_box_construct(struct UIMessageBox *self)
{
    msgbox_frame(self, &ui_message_box_ops);
    string_init(&self->format);
    string_init(&self->text);
    string_init(&self->title);
    self->counting = 0;
    self->keep_text = 0;
    return self;
}

struct UIMessageBox *ui_message_box_new(void)
{
    struct UIMessageBox *self = mem_alloc(sizeof *self);

    return self ? ui_message_box_construct(self) : NULL;
}

void ui_message_box_destruct(struct UIMessageBox *self)
{
    self->frame.base.ops = &ui_message_box_ops;
    string_free(&self->title);
    string_free(&self->text);
    string_free(&self->format);
    self->frame.base.ops = &ui_frame_wnd_ops;
    ui_window_destruct(&self->frame.base);
}

void ui_message_box_destroy(struct UIWindow *window)
{
    struct UIMessageBox *self = (struct UIMessageBox *)window;
    ui_message_box_destruct(self);
    mem_free(self);
}

static const char *const msgbox_ok[3] = { "btn_ok", "btn_ok_a", "btn_ok_b" };
static const char *const msgbox_cancel[3] = { "btn_cancel", "btn_cancel_a", "btn_cancel_b" };

void ui_message_box_create(struct UIMessageBox *self, const char *text, int type, int default_yes,
                                    DWORD duration, const char *title, int width, int height, int modal,
                                    const char *picture)
{
    struct UIWindow *wnd = &self->frame.base;
    struct String skin;
    int left = 0;

    string_assign_cstr(&self->title, title);
    msgbox_start(self, text, duration);
    msgbox_start_count(self, duration);
    self->modal = modal;
    ui_window_create(wnd, width, height);
    ui_window_on_create_base(wnd, width, height);
    msgbox_centre(self);

    if (picture) {
        struct UIStaticBitmap *pic = mem_alloc(sizeof *pic);

        if (pic)
            pic = ui_static_bitmap_construct(pic);
        ui_static_bitmap_set_bitmap(pic, picture);
        ui_window_create(&pic->base, pic->base.width, pic->base.height);
        ui_window_move_base(&pic->base, 0xa, 0x16);
        ui_window_add_child(wnd, &pic->base);
        left = pic->base.width + pic->base.x;
    }
    msgbox_viewer(self, left + 0xa, 0x16, width - left - 0x14, height - 0x37);
    ui_window_send_msg(self->viewer, NULL, UI_MSG_ADD_TEXT, (intptr_t)string_cstr(&self->text),
             (int)(width / 7.5), 0, 0);
    self->frame.default_id = type == MSGBOX_YES_NO ? MSGBOX_ID_NO : MSGBOX_ID_CANCEL;

    ui_skin_dir(&skin);
    switch (type) {
    case MSGBOX_OK:
        ui_add_named_skin_button(wnd, &skin, msgbox_ok, width - 0x2e, height - 0x18, MSGBOX_ID_OK);
        self->frame.default_id = MSGBOX_ID_OK;
        break;
    case MSGBOX_YES_NO:
        ui_add_named_skin_button(wnd, &skin, msgbox_ok, width - 0x5b, height - 0x18, MSGBOX_ID_YES);
        ui_add_named_skin_button(wnd, &skin, msgbox_cancel, width - 0x5b + 0x2d, height - 0x18, MSGBOX_ID_NO);
        self->frame.cancel_id = MSGBOX_ID_NO;
        self->frame.default_id = default_yes ? MSGBOX_ID_YES : MSGBOX_ID_NO;
        break;
    case MSGBOX_CANCEL:
        ui_add_named_skin_button(wnd, &skin, msgbox_cancel, width - 0x2e, height - 0x18, MSGBOX_ID_CANCEL);
        self->frame.default_id = MSGBOX_ID_CANCEL;
        break;
    }
    string_free(&skin);
}

void ui_message_box_on_destroy(struct UIWindow *self)
{
}

void ui_message_box_on_process(struct UIWindow *window)
{
    struct UIMessageBox *self = (struct UIMessageBox *)window;
    if (self->counting)
        msgbox_tick(self);
}

void ui_message_box_on_draw(struct UIWindow *window)
{
    struct UIMessageBox *self = (struct UIMessageBox *)window;
    ui_window_draw_title_bar(&self->frame.base, 1, string_cstr(&self->title), 0);
    ui_window_draw_body(&self->frame.base, 0x11, 0, "");
}

intptr_t ui_message_box_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2,
                                    intptr_t val3, intptr_t val4)
{
    struct UIMessageBox *self = (struct UIMessageBox *)window;
    switch (msg) {
    case UI_MSG_COMMAND:

        if (self->modal) {
            g_windowMgr.modal_result = val1;
            return 0;
        }
        if (val1 == MSGBOX_ID_OK || val1 == MSGBOX_ID_YES || val1 == MSGBOX_ID_NO)
            ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
        return 0;
    case MSGBOX_MSG_SET_TEXT:
        ui_list_box_clear((struct UIListBox *)self->viewer);
        ui_window_send_msg(self->viewer, NULL, UI_MSG_ADD_TEXT, val1, MSGBOX_TEXT_CODE, 0, 0);
        return 0;
    case MSGBOX_MSG_SET_FORMAT:
        string_assign_cstr(&self->format, (const char *)val1);
        return 0;
    case MSGBOX_MSG_SET_MODAL:
        self->modal = val1;
        return 0;
    case MSGBOX_MSG_SET_FC:
        self->keep_text = val1 > 0;
        return 0;
    case MSGBOX_MSG_GET_FC:
        return self->keep_text == 1;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_message_box_autoreturn_destruct(struct UIMessageBox *self)
{
    self->frame.base.ops = &ui_message_box_autoreturn_ops;
    string_free(&self->text);
    string_free(&self->format);
    self->frame.base.ops = &ui_frame_wnd_ops;
    ui_window_destruct(&self->frame.base);
}

void ui_message_box_autoreturn_destroy(struct UIWindow *window)
{
    struct UIMessageBox *self = (struct UIMessageBox *)window;
    ui_message_box_autoreturn_destruct(self);
    mem_free(self);
}

void ui_message_box_autoreturn_on_process(struct UIWindow *window)
{
    struct UIMessageBox *self = (struct UIMessageBox *)window;
    unsigned total_ms, gone;

    if (!self->counting || !msgbox_tick(self))
        return;
    total_ms = (unsigned)self->total * 1000;
    gone = platform_ticks_ms() + (total_ms - self->end);
    ui_bar_graph_set_pos(self->gauge, (float)(double)(gone * 100 / total_ms));
}

void ui_message_box_autoreturn_on_draw(struct UIWindow *window)
{
    struct UIMessageBox *self = (struct UIMessageBox *)window;
    ui_window_draw_bitmap(&self->frame.base, 0, 0, ui_skin_bitmap(UI_SKIN "win_msgbox.bmp"), 0);
}

intptr_t ui_message_box_autoreturn_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                              intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMessageBox *self = (struct UIMessageBox *)window;
    if (msg == UI_MSG_COMMAND) {
        g_windowMgr.modal_result = val1;
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
