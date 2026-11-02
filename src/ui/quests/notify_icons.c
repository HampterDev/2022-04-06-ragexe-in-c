#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include "ui/quests/quests.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define RODEX_INBOX_WND_ID 0x107
#define REFINING_WND_ID 0x127
#define GRADE_ENCHANT_WND_ID 0x157

#define NOTIFY_ICON_BUTTON_ID 0xb8
#define NOTIFY_ICON_BMP_DIR UI_SKIN "basic_interface\\"

#define MSG_MAIL_BLOCKED_BY_REFINING 0xbb0
#define MSG_MAIL_BLOCKED_BY_GRADE_ENCHANT 0xe83

#define MAIL_ICON_BMP UI_SKIN "basic_interface\\rodexsystem\\renewal\\icon_noti_gotmail.bmp"

struct UIOpenMailBoxWnd;
struct UIWindow;
void ui_open_mail_box_wnd_destroy(struct UIWindow *window);
void ui_open_mail_box_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_open_mail_box_wnd_on_draw(struct UIWindow *self);
int ui_open_mail_box_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_open_mail_box_wnd_ops = {
    .destroy = ui_open_mail_box_wnd_destroy,
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
    .on_create = ui_open_mail_box_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_open_mail_box_wnd_on_draw,
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
    .send_msg = ui_open_mail_box_wnd_send_msg,
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

struct UIOpenMailBoxWnd {
    struct UIFrameWnd frame;
};

struct UIOpenMailBoxWnd *ui_open_mail_box_wnd_construct(struct UIOpenMailBoxWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_open_mail_box_wnd_ops;
    return self;
}

struct UIWindow *ui_open_mail_box_wnd_new(void)
{
    return (struct UIWindow *)ui_open_mail_box_wnd_construct(mem_alloc(sizeof(struct UIOpenMailBoxWnd)));
}

static void ui_open_mail_box_wnd_destruct(struct UIOpenMailBoxWnd *self)
{
    self->frame.base.ops = &ui_open_mail_box_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_open_mail_box_wnd_destroy(struct UIWindow *window)
{
    struct UIOpenMailBoxWnd *self = (struct UIOpenMailBoxWnd *)window;
    ui_open_mail_box_wnd_destruct(self);
    mem_free(self);
}

void ui_open_mail_box_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIOpenMailBoxWnd *self = (struct UIOpenMailBoxWnd *)window;
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, MAIL_ICON_BMP, 0, 0);
    ui_bitmap_button_set_bitmap(button, MAIL_ICON_BMP, 1, 0);
    ui_bitmap_button_set_bitmap(button, MAIL_ICON_BMP, 2, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    button->button.base.id = NOTIFY_ICON_BUTTON_ID;
    ui_window_add_child(&self->frame.base, &button->button.base);
}

void ui_open_mail_box_wnd_on_draw(struct UIWindow *self)
{
}

int ui_open_mail_box_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                        int val1, int val2, int val3, int val4)
{
    struct UIOpenMailBoxWnd *self = (struct UIOpenMailBoxWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
    if (val1 != NOTIFY_ICON_BUTTON_ID)
        return 0;
    if (ui_get_window(REFINING_WND_ID))
        ui_chat_line(msg_str(MSG_MAIL_BLOCKED_BY_REFINING), RGB(0xff, 0, 0));
    else if (ui_get_window(GRADE_ENCHANT_WND_ID))
        ui_chat_line(msg_str(MSG_MAIL_BLOCKED_BY_GRADE_ENCHANT), RGB(0xff, 0, 0));
    else if (!ui_close_window(RODEX_INBOX_WND_ID))
        ui_make_window(RODEX_INBOX_WND_ID);
    return 0;
}

#define STATUS_WND_ID 0xb
#define UI_TITLE_BAR_ONLY_HEIGHT 0x11
#define UI_ID_RESTORE 0xc2

struct UINotifyLevelUpWnd;
void ui_notify_level_up_wnd_destroy(struct UIWindow *window);
void ui_notify_level_up_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_notify_level_up_wnd_on_draw(struct UIWindow *self);
int ui_notify_level_up_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_notify_level_up_wnd_ops = {
    .destroy = ui_notify_level_up_wnd_destroy,
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
    .on_create = ui_notify_level_up_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_notify_level_up_wnd_on_draw,
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
    .send_msg = ui_notify_level_up_wnd_send_msg,
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

struct UINotifyLevelUpWnd {
    struct UIFrameWnd frame;
};

static struct UINotifyLevelUpWnd *ui_notify_level_up_wnd_construct(struct UINotifyLevelUpWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_notify_level_up_wnd_ops;
    return self;
}

struct UINotifyLevelUpWnd *ui_notify_level_up_wnd_new(void)
{
    struct UINotifyLevelUpWnd *self = mem_alloc(sizeof *self);

    return self ? ui_notify_level_up_wnd_construct(self) : NULL;
}

void ui_notify_level_up_wnd_destruct(struct UINotifyLevelUpWnd *self)
{
    self->frame.base.ops = &ui_notify_level_up_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_notify_level_up_wnd_destroy(struct UIWindow *window)
{
    struct UINotifyLevelUpWnd *self = (struct UINotifyLevelUpWnd *)window;
    ui_notify_level_up_wnd_destruct(self);
    mem_free(self);
}

static void notify_icon_add_button(struct UIWindow *wnd, const char *name)
{
    char path[MAX_PATH];
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

    if (button)
        button = ui_bitmap_button_construct(button);
    sprintf(path, "%s%s_off.bmp", NOTIFY_ICON_BMP_DIR, name);
    ui_bitmap_button_set_bitmap(button, path, 0, 0);
    ui_bitmap_button_set_bitmap(button, path, 1, 0);
    sprintf(path, "%s%s_on.bmp", NOTIFY_ICON_BMP_DIR, name);
    ui_bitmap_button_set_bitmap(button, path, 2, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, 0, 0);
    button->button.base.id = NOTIFY_ICON_BUTTON_ID;
    ui_window_add_child(wnd, &button->button.base);
}

void ui_notify_level_up_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UINotifyLevelUpWnd *self = (struct UINotifyLevelUpWnd *)window;
    notify_icon_add_button(&self->frame.base, "lv_up");
}

void ui_notify_level_up_wnd_on_draw(struct UIWindow *self)
{
}

int ui_notify_level_up_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          int val1, int val2, int val3, int val4)
{
    struct UINotifyLevelUpWnd *self = (struct UINotifyLevelUpWnd *)window;
    struct UIWindow *status;

    if (msg != UI_MSG_COMMAND)
        return ui_window_send_msg_base(&self->frame.base, sender, msg, val1, val2, val3, val4);
    if (val1 != NOTIFY_ICON_BUTTON_ID)
        return 0;
    status = ui_make_window(STATUS_WND_ID);
    if (status->height == UI_TITLE_BAR_ONLY_HEIGHT)
        ui_window_send_msg(status, NULL, UI_MSG_COMMAND, UI_ID_RESTORE, 0, 0, 0);
    ui_window_mgr_raise_window(&g_windowMgr, status);
    return 0;
}

struct UINotifyQuestWnd;
void ui_notify_quest_wnd_destroy(struct UIWindow *window);
void ui_notify_quest_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_notify_quest_wnd_on_draw(struct UIWindow *self);
int ui_notify_quest_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_notify_quest_wnd_ops = {
    .destroy = ui_notify_quest_wnd_destroy,
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
    .on_create = ui_notify_quest_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_notify_quest_wnd_on_draw,
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
    .send_msg = ui_notify_quest_wnd_send_msg,
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

struct UINotifyQuestWnd {
    struct UIFrameWnd frame;
};

struct UIWindow *ui_notify_quest_wnd_new(void)
{
    struct UINotifyQuestWnd *self = mem_alloc(sizeof *self);

    if (!self)
        return NULL;
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_notify_quest_wnd_ops;
    return &self->frame.base;
}

static void ui_notify_quest_wnd_destruct(struct UINotifyQuestWnd *self)
{
    self->frame.base.ops = &ui_notify_quest_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_notify_quest_wnd_destroy(struct UIWindow *window)
{
    struct UINotifyQuestWnd *self = (struct UINotifyQuestWnd *)window;
    ui_notify_quest_wnd_destruct(self);
    mem_free(self);
}

void ui_notify_quest_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UINotifyQuestWnd *self = (struct UINotifyQuestWnd *)window;
    notify_icon_add_button(&self->frame.base, "quest");
}

void ui_notify_quest_wnd_on_draw(struct UIWindow *self)
{
}

struct UIFrameWnd;
void ui_notify_job_level_up_wnd_destroy(struct UIWindow *window);
int ui_notify_job_level_up_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_notify_job_level_up_wnd_ops = {
    .destroy = ui_notify_job_level_up_wnd_destroy,
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
    .on_create = ui_notify_level_up_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_notify_level_up_wnd_on_draw,
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
    .send_msg = ui_notify_job_level_up_wnd_send_msg,
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

struct UIWindow *ui_notify_job_level_up_wnd_new(void)
{
    struct UINotifyLevelUpWnd *self = ui_notify_level_up_wnd_new();

    if (!self)
        return NULL;
    self->frame.base.ops = &ui_notify_job_level_up_wnd_ops;
    return &self->frame.base;
}

void ui_notify_job_level_up_wnd_destruct(struct UIFrameWnd *self)
{
    self->base.ops = &ui_notify_job_level_up_wnd_ops;
    self->base.ops = &ui_notify_level_up_wnd_ops;
    ui_frame_wnd_destruct(self);
}

void ui_notify_job_level_up_wnd_destroy(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    ui_notify_job_level_up_wnd_destruct(self);
    mem_free(self);
}

#define UI_MSG_BUTTON 6
int ui_notify_job_level_up_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender,
                                             int msg, int val1, int val2, int val3, int val4)
{
    if (msg != UI_MSG_BUTTON)
        return ui_window_send_msg_base(self, sender, msg, val1, val2, val3, val4);
    if (val1 == 0xb8) {
        ui_window_raise(ui_window_open(0x25));
    }
    return 0;
}

int ui_notify_quest_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                        int val1, int val2, int val3, int val4)
{
    if (msg != UI_MSG_BUTTON)
        return ui_window_send_msg_base(self, sender, msg, val1, val2, val3, val4);
    if (val1 == 0xb8) {
        ui_window_raise(ui_window_open(0x141));
    }
    return 0;
}
