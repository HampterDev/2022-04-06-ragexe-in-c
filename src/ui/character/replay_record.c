#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "core/markup_tags.h"
#include <string.h>
#include <mbstring.h>

#include "replay/replay.h"

#include "ui/character/character.h"
#include <mmsystem.h>
#include "core/msg_str.h"
#include "diag/perf.h"

#define REC_WND_ID 0xc6

struct UIReplayRECControlWnd;
struct UIWindow;
void ui_replay_rec_control_wnd_destroy(struct UIWindow *window);
void ui_replay_rec_control_wnd_store_info(struct UIWindow *self);
void ui_replay_rec_control_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_replay_rec_control_wnd_on_draw(struct UIWindow *self);
int ui_replay_rec_control_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_replay_rec_control_wnd_ops = {
    .destroy = ui_replay_rec_control_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_replay_rec_control_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_replay_rec_control_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_replay_rec_control_wnd_on_draw,
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
    .send_msg = ui_replay_rec_control_wnd_send_msg,
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

#define REPLAY_SKIN UI_SKIN "replay_interface\\"

enum RecState {
    REC_STATE_IDLE = 0,
    REC_STATE_RECORDING = 1,
    REC_STATE_OPTIONS = 2,
    REC_STATE_NAMING = 3,
};

enum {
    REC_ID_START = 0x19c,
    REC_ID_STOP = 0x19d,
    REC_ID_START_NAMED = 0x19e,
    REC_ID_OPTIONS = 0x19f,
    REC_ID_OPTIONS_BACK = 0x1a0,
    REC_ID_CLOSE = 0x1a1,
};
#define REC_REQ_START 0x112
#define REC_REQ_STOP 0x113

#define REC_MSG_REFRESH 0x3c
#define REC_MSG_SAVE_POS 0x57
#define REC_MSG_UPDATE 0x74
#define REC_MSG_F3 0xf3

#define MSG_REC_OPTIONS_TITLE 0x7b3
#define MSG_REC_NAME_HINT 0x7b4
#define MSG_REC_SET_SAVE_DATA 0x7b5
#define MSG_REC_SET_OPTION 0x7b6
#define MSG_REC_NAME_HAS_LINK 0xafc

#define REC_FRAMES 5
#define REC_OPTIONS 10
#define REC_OPTION_BASIC 0
#define REC_OPTION_ALL 9

struct UIReplayRECControlWnd {
    struct UIFrameWnd frame;
    int state;
    struct UIReplayButton *start_button;
    struct UIReplayButton *stop_button;
    struct UIBitmapButton *name_start_button;
    struct UIBitmapButton *options_button;
    struct UIBitmapButton *options_back_button;
    struct UIBitmapButton *close_button;
    struct UIStaticBitmap *frames[REC_FRAMES];
    struct UIWindow *time_text;
    struct UIWindow *options_title;
    DWORD clock_tick;
    struct UIWindow *boxes[REC_OPTIONS];
    BYTE options[REC_OPTIONS];
    struct UIWindow *custom_name_box;
    struct UIWindow *check_name_box;
    BYTE use_custom_name;
    BYTE check_name;
    struct UIWindow *name_edit;
    struct WindowInfo *info;
};

#define REC_WND_W 0xc2
#define REC_WND_H 0x22
#define REC_WND_OPTIONS_H 0xc8
#define REC_WND_X 0x103
#define REC_WND_Y 0xe6

struct UIReplayRECControlWnd *ui_replay_rec_control_wnd_construct(struct UIReplayRECControlWnd *self, int state)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->state = state;
    self->frame.base.ops = &ui_replay_rec_control_wnd_ops;
    self->clock_tick = 0;
    for (i = 0; i < REC_OPTIONS; i++) {
        self->boxes[i] = NULL;
        self->options[i] = 0;
    }
    return self;
}

struct UIWindow *ui_replay_rec_control_wnd_new(int arg)
{
    return (struct UIWindow *)ui_replay_rec_control_wnd_construct(mem_alloc(sizeof(struct UIReplayRECControlWnd)), arg);
}

void ui_replay_rec_control_wnd_destroy(struct UIWindow *window)
{
    struct UIReplayRECControlWnd *self = (struct UIReplayRECControlWnd *)window;
    self->frame.base.ops = &ui_replay_rec_control_wnd_ops;
    self->clock_tick = 0;
    self->state = REC_STATE_IDLE;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

void ui_replay_rec_control_wnd_store_info(struct UIWindow *window)
{
    struct UIReplayRECControlWnd *self = (struct UIReplayRECControlWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
}

static void hide(void *child)
{
    ui_window_move(child, UI_POS_HIDDEN, UI_POS_HIDDEN);
}

static struct UIBitmapButton *add_rec_button(struct UIReplayRECControlWnd *self, int replay, const char *name,
                                             int id, const char *tooltip)
{
    struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

    if (button) {
        if (replay)
            button = &ui_replay_button_construct((struct UIReplayButton *)button)->bmp;
        else
            button = ui_bitmap_button_construct(button);
    }
    ui_replay_button_bitmaps(button, name);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    hide(button);
    button->button.base.id = id;
    ui_bitmap_button_set_tool_tip(button, tooltip);
    ui_window_add_child(&self->frame.base, &button->button.base);
    return button;
}

static struct UIWindow *add_rec_box(struct UIReplayRECControlWnd *self, int width, const char *label, int checked)
{
    struct UIWindow *box = mem_alloc(sizeof(struct UICheckBox));

    if (box)
        box = (struct UIWindow *)ui_check_box_construct(box);
    ui_window_create(box, width, 0xc);
    ui_check_box_set_bitmaps(box, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp", label);
    hide(box);
    ui_check_box_set_check(box, checked);
    ui_window_add_child(&self->frame.base, box);
    return box;
}

static void rec_layout(struct UIReplayRECControlWnd *self)
{
    static const POINT box_pos[REC_OPTIONS] = {
        { 0x11, 0x3f }, { 0x11, 0x51 }, { 0x11, 0x63 }, { 0x11, 0x75 }, { UI_POS_HIDDEN, UI_POS_HIDDEN },
        { 0x68, 0x51 }, { 0x68, 0x63 }, { 0x68, 0x75 }, { 0, 0 }  ,   { 0x68, 0x3f },
    };
    struct UIWindow *wnd = &self->frame.base;
    int i;

    ui_window_resize_base(wnd, REC_WND_W, REC_WND_H);
    switch (self->state) {
    case REC_STATE_IDLE:
        hide(self->stop_button);
        for (i = 0; i < REC_FRAMES; i++)
            hide(self->frames[i]);
        hide(self->name_start_button);
        hide(self->name_edit);
        hide(self->close_button);
        hide(self->options_title);
        for (i = 0; i < REC_OPTIONS; i++)
            hide(self->boxes[i]);
        hide(self->custom_name_box);
        hide(self->check_name_box);
        hide(self->name_edit);
        ui_window_move_base(&self->start_button->bmp.button.base, 0x6e, 5);
        ui_window_move(&self->options_button->button.base, 0x88, 5);
        self->options_button->button.disabled = 0;
        ui_window_invalidate(&self->options_button->button.base);
        ui_window_move(&self->close_button->button.base, 0xa2, 5);
        self->close_button->button.disabled = 0;
        ui_window_invalidate(&self->close_button->button.base);
        ui_window_move(self->time_text, 0x20, 0xc);
        ui_window_move_base(&self->frames[0]->base, 6, 5);
        break;
    case REC_STATE_RECORDING:
        self->clock_tick = timeGetTime();
        hide(self->start_button);
        for (i = 0; i < REC_FRAMES - 1; i++)
            hide(self->frames[i]);
        hide(self->frames[REC_FRAMES - 1]);
        hide(self->options_back_button);
        hide(self->boxes[7]);
        for (i = 0; i < REC_OPTIONS; i++)
            hide(self->boxes[i]);
        hide(self->custom_name_box);
        hide(self->check_name_box);
        hide(self->name_start_button);
        hide(self->name_edit);
        ui_window_move_base(&self->stop_button->bmp.button.base, 0x6e, 5);
        ui_window_move(&self->options_button->button.base, 0x88, 5);
        self->options_button->button.disabled = 1;
        ui_window_invalidate(&self->options_button->button.base);
        ui_window_move(&self->close_button->button.base, 0xa2, 5);
        self->close_button->button.disabled = 1;
        ui_window_invalidate(&self->close_button->button.base);
        ui_window_move(self->time_text, 0x20, 0xc);
        ui_window_move_base(&self->frames[0]->base, 6, 5);
        break;
    case REC_STATE_OPTIONS:
        ui_window_resize_base(wnd, REC_WND_W, REC_WND_OPTIONS_H);
        hide(self->start_button);
        hide(self->stop_button);
        hide(self->options_button);
        hide(self->close_button);
        hide(self->time_text);
        for (i = 0; i < REC_FRAMES - 1; i++)
            hide(self->frames[i]);
        hide(self->name_start_button);
        hide(self->name_edit);
        ui_window_move_base(&(self->frames[REC_FRAMES - 1])->base, 6, 5);
        ui_window_move(&self->options_back_button->button.base, 0xa2, 5);
        ui_window_move(self->options_title, 0x2a, 0xc);
        for (i = 0; i < REC_OPTIONS; i++) {
            if (i != 8)
                ui_window_move_base(self->boxes[i], box_pos[i].x, box_pos[i].y);
        }
        ui_window_move_base(self->custom_name_box, 0x11, 0xa0);
        ui_window_move_base(self->check_name_box, 0x11, 0xb2);
        break;
    case REC_STATE_NAMING:
        hide(self->start_button);
        hide(self->stop_button);
        hide(self->options_button);
        hide(self->close_button);
        hide(self->time_text);
        for (i = 0; i < REC_FRAMES - 1; i++)
            hide(self->frames[i]);
        hide(self->frames[REC_FRAMES - 1]);
        hide(self->options_back_button);
        hide(self->boxes[7]);
        for (i = 0; i < REC_OPTIONS; i++)
            hide(self->boxes[i]);
        hide(self->custom_name_box);
        hide(self->check_name_box);
        ui_window_move(&self->name_start_button->button.base, 0xa2, 5);
        ui_window_move(self->name_edit, 7, 8);
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->name_edit);
        break;
    }
    ui_replay_rec_control_wnd_send_msg(wnd, NULL, REC_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_replay_rec_control_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIReplayRECControlWnd *self = (struct UIReplayRECControlWnd *)window;
    static const int msg_ids[16] = { 0x7b9, 0x7ba, 0x7bb, 0x7bc, 0x7bd, 0x7be, 0x7bf, 0x64,
                                     0x68,  0x6a,  0x11b, 0x7c0, 0x7c1, 0x7c2, 0x7c3, 0x7c4 };
    static const char *const box_labels[REC_OPTIONS] = {
        "Basicinfo", "Equip", "Item", "Skill", "Guild", "Party", "Chatting", "Shortcut", "Status", "ALL",
    };
    static const POINT text_size[2] = { { 0x32, 0xc }, { 0x50, 0xc } };
    struct UIWindow *wnd = &self->frame.base;
    struct ReplayRecorder *rec;
    struct ReplayFileList *files;
    const char *msgs[16];
    const char *texts[2];
    struct UIWindow **text_fields[2] = { &self->time_text, &self->options_title };
    const char *hint;
    char path[0x100];
    BYTE checked;
    int i;

    for (i = 0; i < 16; i++)
        msgs[i] = msg_str(msg_ids[i]);

    self->start_button = (struct UIReplayButton *)add_rec_button(self, 1, "btn_start_", REC_ID_START, msgs[0]);
    self->stop_button = (struct UIReplayButton *)add_rec_button(self, 1, "btn_stop_", REC_ID_STOP, msgs[1]);
    self->name_start_button = add_rec_button(self, 0, "btn_start_", REC_ID_START_NAMED, msgs[2]);
    self->options_button = add_rec_button(self, 0, "btn_save_", REC_ID_OPTIONS, msgs[3]);
    self->options_back_button = add_rec_button(self, 0, "btn_setup_", REC_ID_OPTIONS_BACK, msgs[4]);
    self->close_button = add_rec_button(self, 0, "btn_close_", REC_ID_CLOSE, msgs[5]);

    for (i = 0; i < REC_FRAMES; i++) {
        struct UIStaticBitmap *frame = mem_alloc(sizeof *frame);

        if (frame)
            frame = ui_static_bitmap_construct(frame);
        self->frames[i] = frame;
        sprintf(path, REPLAY_SKIN "replay_recrun_%d.bmp", i + 1);
        ui_static_bitmap_set_bitmap(frame, path);
        ui_window_create(&frame->base, frame->base.width, frame->base.height);
        hide(frame);
        frame->base.id = (int)msgs[i];
        ui_window_add_child(wnd, &frame->base);
    }

    texts[0] = "000:00:00";
    texts[1] = msg_str(MSG_REC_OPTIONS_TITLE);
    for (i = 0; i < 2; i++) {
        struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

        if (text)
            text = ui_static_text_construct(text);
        *text_fields[i] = text;
        ui_window_create(text, text_size[i].x, text_size[i].y);
        ui_static_text_set_text(text, texts[i], 0);
        hide(text);
        ui_static_text_set_text_colour(text, 0xff, 0xff, 0xff);
        ui_window_add_child(wnd, text);
    }

    rec = replay_recorder_instance();
    memcpy(self->options, rec->info.options, REC_OPTIONS);
    for (i = 0; i < REC_OPTIONS; i++)
        self->boxes[i] = add_rec_box(self, 0x46, box_labels[i], replay_recorder_instance()->info.options[i]);

    self->custom_name_box = add_rec_box(self, 0x96, "User Defined File Name", 0);
    self->check_name_box = add_rec_box(self, 0x96, "Repeated File Check", 0);
    files = replay_file_list_instance();
    checked = files->useCustomName;
    ui_check_box_set_check(self->custom_name_box, checked);
    self->use_custom_name = checked;
    checked = replay_file_list_instance()->customNameSet;
    ui_check_box_set_check(self->check_name_box, checked);
    self->check_name = checked;

    self->name_edit = mem_alloc(sizeof(struct UIEditCtrl));
    if (self->name_edit)
        self->name_edit = ui_edit_ctrl_construct(self->name_edit);
    ui_window_create(self->name_edit, 0x97, 0x12);
    hide(self->name_edit);
    UI_EDIT_MAX_CHARS(self->name_edit) = 0x50;
    ui_edit_ctrl_set_back_colour(self->name_edit, 0xe8, 0xe8, 0xe8);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->name_edit, "");
    hint = msg_str(MSG_REC_NAME_HINT);
    if (hint)
        string_assign_cstr(&((struct UIEditCtrl *)self->name_edit)->tooltip, hint);
    ui_window_add_child(wnd, self->name_edit);
    ui_edit_ctrl_select_all(self->name_edit);
    rec_layout(self);
    ui_window_invalidate_base(wnd);
}

void ui_replay_rec_control_wnd_on_draw(struct UIWindow *window)
{
    struct UIReplayRECControlWnd *self = (struct UIReplayRECControlWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    ui_window_fill_round_rect(wnd, 0, 0, wnd->width, 0x22, 0xf5b899);
    ui_window_draw_round_frame(wnd, 0, 0, wnd->width, 0x22, 0x945a63);
    ui_window_draw_round_frame(wnd, 1, 1, wnd->width - 2, 0x20, 0xf7c6ad);
    ui_window_draw_round_frame(wnd, 2, 2, wnd->width - 4, 0x1e, 0xffffff);
    ui_window_draw_round_frame(wnd, 3, 3, wnd->width - 6, 0x1c, 0xad7b84);
    if (self->state != REC_STATE_OPTIONS)
        return;
    ui_window_fill_round_rect(wnd, 0, 0x22, wnd->width, wnd->height - 0x22, 0xdedede);
    ui_window_fill_round_rect(wnd, 4, 0x26, wnd->width - 8, 0x5f, 0xffffff);
    ui_window_fill_round_rect(wnd, 4, 0x8a, wnd->width - 8, 0x39, 0xffffff);
    ui_window_text_out_outlined(wnd, 10, 0x2a, msg_str(MSG_REC_SET_SAVE_DATA), 0, 0x7f3c00, 0xffffff, 0, 12, 1, 0);
    ui_window_text_out_outlined(wnd, 10, 0x8e, msg_str(MSG_REC_SET_OPTION), 0, 0x7f3c00, 0xffffff, 0, 12, 1, 0);
}

static void rec_update(struct UIReplayRECControlWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    int i, elapsed, seconds, minutes, hours;
    char buf[100];

    for (i = 0; i < REC_FRAMES - 1; i++)
        hide(self->frames[i]);
    ui_window_move_base(&(self->frames[timeGetTime() % 1000 / 250])->base, 6, 5);
    if (timeGetTime() - self->clock_tick <= 200)
        return;
    elapsed = (int)(timeGetTime() - replay_recorder_instance()->wallBase);
    seconds = elapsed / 1000;
    minutes = seconds / 60;
    hours = minutes / 60;
    if (hours == 100)
        ui_window_move(self->time_text, 0x7c, 0x2f);
    else if (hours == 1000)
        ui_window_move(self->time_text, 0x74, 0x2f);
    memset(buf, 0, sizeof buf);
    sprintf(buf, "%03d:%02d:%02d", hours, minutes - hours * 60, seconds - minutes * 60);
    ui_static_text_set_text_ellipsis(self->time_text, buf);
    ui_window_invalidate_base(wnd);
    ui_window_invalidate_children(wnd);
    self->clock_tick = timeGetTime();
}

static int rec_info_usable(const struct WindowInfo *info)
{
    return ui_info_on_screen(info);
}

static void rec_info_reset(struct WindowInfo *info)
{
    info->x = REC_WND_X;
    info->y = REC_WND_Y;
    info->width = REC_WND_W;
    info->height = REC_WND_H;
}

static void rec_start_named(struct UIReplayRECControlWnd *self)
{
    const char *name;

    if (ui_edit_ctrl_get_text_size(self->name_edit) <= 0) {
        ui_message_box(msg_str(MSG_REC_NAME_HINT));
        return;
    }
    if (_mbsstr((const unsigned char *)ui_edit_ctrl_get_text(self->name_edit),
                (const unsigned char *)string_cstr(&text_tag_ITEML)) ||
        _mbsstr((const unsigned char *)ui_edit_ctrl_get_text(self->name_edit),
                (const unsigned char *)string_cstr(&text_tag_ITEML_END))) {
        ui_message_box(msg_str(MSG_REC_NAME_HAS_LINK));
        return;
    }
    name = ui_edit_ctrl_get_text(self->name_edit);
    replay_file_list_set_name(replay_file_list_instance(), name);
    ui_window_mgr_set_focus_edit(&g_windowMgr, NULL);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->name_edit, "");
    mode_send_message(client_current_mode(), REC_REQ_START, 0, 0, 0, 0);
    if (replay_recorder_instance()->state == 1)
        return;
    self->state = REC_STATE_RECORDING;
    rec_layout(self);
}

static void rec_option_ticked(struct UIReplayRECControlWnd *self, struct UIWindow *sender, int checked)
{
    int i;

    if (sender == self->boxes[REC_OPTION_ALL]) {
        if (checked == 1) {
            for (i = 0; i < REC_OPTIONS; i++) {
                if (i == REC_OPTION_ALL)
                    continue;
                self->options[i] = 1;
                ui_check_box_set_check(self->boxes[i], 1);
            }
        }
        self->options[REC_OPTION_ALL] = checked != 0;
    } else if (sender == self->boxes[REC_OPTION_BASIC]) {
        if (checked == 0) {
            for (i = 1; i < REC_OPTIONS; i++) {
                self->options[i] = 0;
                ui_check_box_set_check(self->boxes[i], 0);
            }
        }
        self->options[REC_OPTION_BASIC] = checked != 0;
    } else {
        if (checked == 1) {
            if (self->boxes[REC_OPTION_BASIC]->state == 0) {
                ui_check_box_set_check(self->boxes[REC_OPTION_BASIC], checked);
                self->options[REC_OPTION_BASIC] = (BYTE)checked;
            }
        } else if (self->boxes[REC_OPTION_ALL]->state == 1) {
            ui_check_box_set_check(self->boxes[REC_OPTION_ALL], 0);
            self->options[REC_OPTION_ALL] = 0;
        }
        for (i = 1; i < REC_OPTIONS - 1; i++) {
            if (sender == self->boxes[i])
                self->options[i] = checked != 0;
        }
    }
    replay_recorder_set_options(replay_recorder_instance(), self->options, REC_OPTIONS);
}

int ui_replay_rec_control_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             int val1, int val2, int val3, int val4)
{
    struct UIReplayRECControlWnd *self = (struct UIReplayRECControlWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct ReplayFileList *files;

    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_CHECKBOX:
            if (sender == self->custom_name_box) {
                replay_file_list_instance()->useCustomName = val2 != 0;
                self->use_custom_name = replay_file_list_instance()->useCustomName;
            } else if (sender == self->check_name_box) {
                files = replay_file_list_instance();
                files->customNameSet = val2 != 0;
                self->check_name = replay_file_list_instance()->customNameSet;
            } else {
                rec_option_ticked(self, sender, val2);
            }
            break;
        case REC_ID_START:
            if (!self->use_custom_name) {
                self->state = REC_STATE_RECORDING;
                mode_send_message(client_current_mode(), REC_REQ_START, 0, 0, 0, 0);
            } else {
                self->state = REC_STATE_NAMING;
            }
            rec_layout(self);
            break;
        case REC_ID_STOP:
            self->state = REC_STATE_IDLE;
            rec_layout(self);
            mode_send_message(client_current_mode(), REC_REQ_STOP, 0, 0, 0, 0);
            break;
        case REC_ID_START_NAMED:
            rec_start_named(self);
            break;
        case REC_ID_OPTIONS:
            self->state = REC_STATE_OPTIONS;
            rec_layout(self);
            break;
        case REC_ID_OPTIONS_BACK:
            self->state = REC_STATE_IDLE;
            rec_layout(self);
            break;
        case REC_ID_CLOSE:
            self->state = REC_STATE_IDLE;
            ui_close_window(REC_WND_ID);
            break;
        }
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!rec_info_usable(self->info))
            rec_info_reset(self->info);
        ui_frame_wnd_move(wnd, self->info->x, self->info->y);
        return 0;
    case REC_MSG_REFRESH:
        ui_window_invalidate_base(wnd);
        ui_window_invalidate_children(wnd);
        return 0;
    case REC_MSG_SAVE_POS:
        replay_recorder_instance()->windowX = wnd->x;
        replay_recorder_instance()->windowY = wnd->y;
        return 0;
    case REC_MSG_UPDATE:
        rec_update(self);
        return 0;
    case UI_MSG_LOAD_RECORDS:
    case REC_MSG_F3:
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
