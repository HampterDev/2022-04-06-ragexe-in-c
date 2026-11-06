#include "ui/controls/listbox.h"
#include "ui/controls/buttons.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "platform/service.h"
#include "contents/sound.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include <string.h>

#include "replay/replay.h"

#include "ui/character/character.h"
#include "ui/character/replay_play.h"
#include "core/msg_str.h"
#include "diag/perf.h"

struct UIReplayButton;
struct UIWindow;
void ui_replay_button_destroy(struct UIWindow *window);
void ui_replay_button_on_l_btn_up(struct UIWindow *self, int x, int y);
const struct UIButtonOps ui_replay_button_ops = {
    .base = {
        .destroy = ui_replay_button_destroy,
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
        .on_draw = ui_bitmap_button_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_replay_button_on_l_btn_up,
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
struct UIReplayListBox;
void ui_replay_list_box_destroy(struct UIWindow *window);
void ui_replay_list_box_on_l_btn_down(struct UIWindow *self, int x, int y);
const struct UIListBoxOps ui_replay_list_box_ops = {
    .base = {
        .destroy = ui_replay_list_box_destroy,
        .resize = ui_list_box_resize,
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
        .on_create = ui_list_box_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_list_box_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_replay_list_box_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_list_box_send_msg,
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
    .create = ui_list_box_create,
    .add_item = ui_list_box_add_item,
    .clear_items = ui_list_box_clear_items,
    .update_scroll_bar = ui_list_box_update_scroll_bar,
};
struct UIReplayControlWnd;
void ui_replay_control_wnd_destroy(struct UIWindow *window);
void ui_replay_control_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_replay_control_wnd_on_draw(struct UIWindow *self);
int ui_replay_control_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_replay_control_wnd_ops = {
    .destroy = ui_replay_control_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
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
    .on_create = ui_replay_control_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_replay_control_wnd_on_draw,
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
    .send_msg = ui_replay_control_wnd_send_msg,
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
struct UISelectReplayDataWnd;
void ui_select_replay_data_wnd_destroy(struct UIWindow *window);
void ui_select_replay_data_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_select_replay_data_wnd_on_destroy(struct UIWindow *self);
void ui_select_replay_data_wnd_on_draw(struct UIWindow *self);
int ui_select_replay_data_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_select_replay_data_wnd_ops = {
    .destroy = ui_select_replay_data_wnd_destroy,
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
    .on_create = ui_select_replay_data_wnd_on_create,
    .on_destroy = ui_select_replay_data_wnd_on_destroy,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_select_replay_data_wnd_on_draw,
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
    .send_msg = ui_select_replay_data_wnd_send_msg,
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

struct UIReplayButton *ui_replay_button_construct(struct UIReplayButton *self)
{
    ui_bitmap_button_construct(&self->bmp);
    self->bmp.button.base.ops = &ui_replay_button_ops.base;
    return self;
}

void ui_replay_button_destroy(struct UIWindow *window)
{
    struct UIReplayButton *self = (struct UIReplayButton *)window;
    self->bmp.button.base.ops = &ui_replay_button_ops.base;
    ui_bitmap_button_destruct(&self->bmp);
    mem_free(self);
}

void ui_replay_button_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIReplayButton *self = (struct UIReplayButton *)window;
    struct UIWindow *wnd = &self->bmp.button.base;

    if (self->bmp.button.disabled)
        return;
    if (ui_window_mgr_get_capture(&g_windowMgr) != wnd)
        return;
    if (x >= 0 && x < wnd->width && y >= 0 && y < wnd->height) {
        wnd->state = UI_BTN_HOVER;
        if (wnd->parent) {
            ui_window_send_msg(wnd->parent, wnd, UI_MSG_COMMAND, wnd->id, self->bmp.button.cmd_value, 0, 0);
            ui_window_send_msg(wnd->parent, NULL, UI_MSG_REPLAY_BUTTON, 0, 0, 0, 0);
        } else {
            mode_send_message(client_current_mode(), MODE_MSG_UI_COMMAND, wnd->id, self->bmp.button.cmd_value, 0, 0);
        }
    } else {
        wnd->state = UI_BTN_UP;
    }
    ui_window_invalidate_base(wnd);
    ui_window_mgr_release_capture(&g_windowMgr);
}

void ui_replay_button_bitmaps(struct UIBitmapButton *button, const char *name)
{
    char up[0x100], down[0x100];

    sprintf(up, REPLAY_SKIN "%sa.bmp", name);
    sprintf(down, REPLAY_SKIN "%sb.bmp", name);
    ui_bitmap_button_set_bitmap(button, up, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, up, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, down, UI_BTN_DOWN, 0);
}

static struct UIBitmapButton *add_replay_button(struct UIWindow *parent, int replay, const char *name, int x,
                                                int y, int id)
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
    ui_window_move(&button->button.base, x, y);
    button->button.base.id = id;
    ui_window_add_child(parent, &button->button.base);
    return button;
}

static struct UIWindow *add_info_text(struct UIWindow *parent, int width, int height, int x, int y)
{
    struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

    if (text)
        text = ui_static_text_construct(text);
    ui_window_create(text, width, height);
    ui_static_text_set_text(text, "", 0);
    ui_window_move(text, x, y);
    ui_static_text_set_text_colour(text, 100, 100, 200);
    ui_window_add_child(parent, text);
    return text;
}

#define REPLAY_MSG_SELECTED 0x50

struct UIReplayListBox {
    struct UIListBox list;
};
SIZE_ASSERT(UIReplayListBox, 0xd0);

struct UIReplayListBox *ui_replay_list_box_construct(struct UIReplayListBox *self)
{
    ui_list_box_construct(&self->list);
    self->list.base.ops = &ui_replay_list_box_ops.base;
    return self;
}

void ui_replay_list_box_destroy(struct UIWindow *window)
{
    struct UIReplayListBox *self = (struct UIReplayListBox *)window;
    self->list.base.ops = &ui_replay_list_box_ops.base;
    ui_list_box_destruct(&self->list);
    mem_free(self);
}

void ui_replay_list_box_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIReplayListBox *self = (struct UIReplayListBox *)window;
    self->list.selected = ui_list_box_hit_line(&self->list, x, y);
    ui_window_invalidate_base(&self->list.base);
    ui_window_send_msg(self->list.base.parent, NULL, REPLAY_MSG_SELECTED, 0, 0, 0, 0);
}

#define SELECT_REPLAY_ID_LIST 0xb0
#define SELECT_REPLAY_ID_OK 0xb0
#define SELECT_REPLAY_ID_CANCEL 0xb1
#define SELECT_REPLAY_ID_EXIT 0xd5
#define SELECT_REPLAY_ID_SERVER 0x188
#define SELECT_REPLAY_INFO_LINES 5

#define MSG_SELECT_REPLAY_TITLE 0x824
#define MSG_SELECT_REPLAY_INFO 0x825
#define MSG_SELECT_REPLAY_FILES 0x826
#define MSG_REPLAY_SERVER_FMT 0x7ac
#define MSG_REPLAY_NAME_FMT 0x7ad
#define MSG_REPLAY_MAP_FMT 0x7ae
#define MSG_REPLAY_DATE_FMT 0x7af
#define MSG_REPLAY_LENGTH_FMT 0x7b0
#define MSG_REPLAY_CANNOT_READ 0x7b1

struct UISelectReplayDataWnd {
    struct UIFrameWnd frame;
    struct UIReplayListBox *list;
    struct UIWindow *exit_button;
    struct UIWindow *info[SELECT_REPLAY_INFO_LINES];
};
OFFSET_ASSERT(UISelectReplayDataWnd, info, 0x9c);
SIZE_ASSERT(UISelectReplayDataWnd, 0xb0);

struct UISelectReplayDataWnd *ui_select_replay_data_wnd_construct(struct UISelectReplayDataWnd *self)
{
    int i;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_select_replay_data_wnd_ops;
    self->list = NULL;
    self->exit_button = NULL;
    for (i = 0; i < SELECT_REPLAY_INFO_LINES; i++)
        self->info[i] = NULL;
    return self;
}

struct UIWindow *ui_select_replay_data_wnd_new(void)
{
    return (struct UIWindow *)ui_select_replay_data_wnd_construct(mem_alloc(sizeof(struct UISelectReplayDataWnd)));
}

void ui_select_replay_data_wnd_destroy(struct UIWindow *window)
{
    struct UISelectReplayDataWnd *self = (struct UISelectReplayDataWnd *)window;
    self->frame.base.ops = &ui_select_replay_data_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

static const struct {
    int id;
    const char *name[3];
} select_replay_buttons[4] = {
    { SELECT_REPLAY_ID_OK, { "btn_ok", "btn_ok_a", "btn_ok_b" } },
    { SELECT_REPLAY_ID_CANCEL, { "btn_cancel", "btn_cancel_a", "btn_cancel_b" } },
    { SELECT_REPLAY_ID_EXIT, { "login_interface\\btn_exit", "login_interface\\btn_exit_a",
                               "login_interface\\btn_exit_b" } },
    { SELECT_REPLAY_ID_SERVER, { "replay_interface\\btn_selectserver", "replay_interface\\btn_selectserver_a",
                                 "replay_interface\\btn_selectserver_b" } },
};

void ui_select_replay_data_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UISelectReplayDataWnd *self = (struct UISelectReplayDataWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIReplayListBox *list = mem_alloc(sizeof *list);
    int x[4] = { 0xbd, UI_POS_HIDDEN, 0xea, UI_POS_HIDDEN };
    int y[4] = { height - 0x19, UI_POS_HIDDEN, height - 0x19, UI_POS_HIDDEN };
    char path[3][0x80];
    int i, state, line_y;

    if (list)
        list = ui_replay_list_box_construct(list);
    self->list = list;
    ui_list_box_create(&list->list, 6, 0x7f, width - 12, height - 0xaa, 0);
    self->list->list.r = 0xf0;
    self->list->list.g = 0xf0;
    self->list->list.b = 0xf0;
    self->list->list.base.id = SELECT_REPLAY_ID_LIST;
    ui_window_add_child(wnd, &self->list->list.base);

    for (i = 0; i < 4; i++) {
        struct UIBitmapButton *button;

        for (state = 0; state < 3; state++)
            sprintf(path[state], UI_SKIN "%s.bmp", select_replay_buttons[i].name[state]);
        button = ui_bitmap_button(path[0], path[1], path[2]);
        ui_window_move_base(&button->button.base, x[i], y[i]);
        button->button.base.id = select_replay_buttons[i].id;
        ui_window_add_child(wnd, &button->button.base);
        if (select_replay_buttons[i].id == SELECT_REPLAY_ID_EXIT)
            self->exit_button = &button->button.base;
    }
    UI_FRAME_DEFAULT_ID(self) = SELECT_REPLAY_ID_OK;

    for (i = 0, line_y = 0x23; line_y < 0x6e; i++, line_y += 0xf)
        self->info[i] = add_info_text(wnd, width - 12, 0xf, 0xc, line_y);
}

void ui_select_replay_data_wnd_on_destroy(struct UIWindow *self)
{
}

void ui_select_replay_data_wnd_on_draw(struct UIWindow *window)
{
    struct UISelectReplayDataWnd *self = (struct UISelectReplayDataWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_SELECT_REPLAY_TITLE), 0);
    ui_window_draw_body(wnd, 0x11, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    ui_window_fill_rect(wnd, 6, 0x20, 0x10c, 0x4d, 0xffe7e7e7);
    ui_window_text_out(wnd, 6, 0x13, msg_str(MSG_SELECT_REPLAY_INFO), 0, 0, 12, 0, 0, 0);
    ui_window_text_out(wnd, 6, 0x70, msg_str(MSG_SELECT_REPLAY_FILES), 0, 0, 12, 0, 0, 0);
}

static const char *const replay_server_types[6] = {
    "Primary", "Sakray", "Local", "InstantEvent", "PVP", "PK",
};

BYTE ui_select_replay_data_wnd_show_selected(struct UISelectReplayDataWnd *self)
{
    struct ReplayDuration length;
    DWORD date, time;
    char buf[100];
    char name[64];
    int type;

    if (!replay_file_list_select(replay_file_list_instance(), self->list->list.selected)) {
        ui_static_text_set_text_ellipsis(self->info[0], "");
        ui_static_text_set_text_ellipsis(self->info[1], "");
        ui_static_text_set_text_ellipsis(self->info[2], "");
        ui_static_text_set_text_ellipsis(self->info[3], "");
        ui_static_text_set_text_ellipsis(self->info[4], "");
        return 1;
    }
    if (replay_file_list_open_selected(replay_file_list_instance(), REPLAY_OPEN_READ) != 1) {
        ui_static_text_set_text_ellipsis(self->info[0], "");
        ui_static_text_set_text_ellipsis(self->info[1], msg_str(MSG_REPLAY_CANNOT_READ));
        ui_static_text_set_text_ellipsis(self->info[2], "");
        ui_static_text_set_text_ellipsis(self->info[3], "");
        ui_static_text_set_text_ellipsis(self->info[4], "");
        return 1;
    }
    replay_recorder_close(replay_recorder_instance());
    memset(buf, 0, sizeof buf);
    type = (int)replay_recorder_instance()->info.serverType;
    if ((unsigned)type > 5) {
        ui_static_text_set_text_ellipsis(self->info[0], "");
        ui_static_text_set_text_ellipsis(self->info[1], msg_str(MSG_REPLAY_CANNOT_READ));
        ui_static_text_set_text_ellipsis(self->info[2], "");
        ui_static_text_set_text_ellipsis(self->info[3], "");
        ui_static_text_set_text_ellipsis(self->info[4], "");
        return 0;
    }
    sprintf(buf, msg_str(MSG_REPLAY_SERVER_FMT), replay_server_types[type]);
    ui_static_text_set_text_ellipsis(self->info[0], buf);

    memset(buf, 0, sizeof buf);
    memcpy(name, replay_recorder_instance()->info.name, sizeof name);
    sprintf(buf, msg_str(MSG_REPLAY_NAME_FMT), name);
    ui_static_text_set_text_ellipsis(self->info[1], buf);

    sprintf(buf, msg_str(MSG_REPLAY_MAP_FMT), (const char *)replay_recorder_instance()->info.mapData);
    ui_static_text_set_text_ellipsis(self->info[2], buf);

    date = replay_recorder_instance()->info.cipherKeyA;
    time = replay_recorder_instance()->info.cipherKeyB;
    sprintf(buf, msg_str(MSG_REPLAY_DATE_FMT), (int)(WORD)date, (int)(signed char)(date >> 16),
                (int)(signed char)(date >> 24), (int)(signed char)(time >> 8), (int)(signed char)(time >> 16),
                (int)(signed char)(time >> 24));
    ui_static_text_set_text_ellipsis(self->info[3], buf);

    length = replay_recorder_duration(replay_recorder_instance());
    sprintf(buf, msg_str(MSG_REPLAY_LENGTH_FMT), (int)length.hours, (int)(signed char)length.minutes,
                (int)(signed char)length.seconds);
    ui_static_text_set_text_ellipsis(self->info[4], buf);
    return 1;
}

#define SELECT_REPLAY_LIST_PREV 0x12
#define SELECT_REPLAY_LIST_NEXT 0x13
#define SELECT_REPLAY_LIST_SELECT 0x28
#define SELECT_REPLAY_MSG_PLACE_EXIT 0x59
#define SELECT_REPLAY_EXIT_X 0xea
#define SELECT_REPLAY_REQ_QUIT 2
#define SELECT_REPLAY_REQ_BACK 0x2729
#define SELECT_REPLAY_REQ_PLAY 0x272b
#define REPLAY_MODE_BACK 1
#define MSG_REPLAY_WRONG_SERVER 0x7b2
#define MSG_SELECT_REPLAY_QUIT_ASK 0x11

static void select_replay_box(int msg)
{
    ui_window_mgr_message_box(&g_windowMgr, msg_str(msg), UI_MSGBOX_OK, 1, 1, 0, "Message", 0x118, 0x78, 0);
}

int ui_select_replay_data_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             int val1, int val2, int val3, int val4)
{
    struct UISelectReplayDataWnd *self = (struct UISelectReplayDataWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case SELECT_REPLAY_ID_OK:
            if (self->list->list.selected == -1)
                return 0;
            ui_select_replay_data_wnd_show_selected(self);
            ui_window_invalidate_base(wnd);
            if (replay_file_list_open_selected(replay_file_list_instance(), REPLAY_OPEN_READ) != 1) {
                select_replay_box(MSG_REPLAY_CANNOT_READ);
                return 0;
            }
            if (!replay_recorder_server_type_matches(replay_recorder_instance(), g_serverType)) {
                replay_recorder_close(replay_recorder_instance());
                select_replay_box(MSG_REPLAY_WRONG_SERVER);
                return 0;
            }
            mode_send_message(client_current_mode(), SELECT_REPLAY_REQ_PLAY, 0, 0, 0, 0);
            ui_window_mgr_post_quit2(&g_windowMgr, wnd);
            sound_play_wave("버튼소리.wav"  , 0.0f, 0.0f,
                            0.0f, 0xfa, 0x28, 1.0f);
            return 0;
        case SELECT_REPLAY_ID_CANCEL:
        case SELECT_REPLAY_ID_SERVER:
            replay_recorder_set_mode(replay_recorder_instance(), REPLAY_MODE_BACK);
            ui_window_mgr_post_quit2(&g_windowMgr, wnd);
            mode_send_message(client_current_mode(), SELECT_REPLAY_REQ_BACK, 0, 0, 0, 0);
            return 0;
        case SELECT_REPLAY_ID_EXIT:
            if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_SELECT_REPLAY_QUIT_ASK), UI_MSGBOX_QUESTION, 1,
                                       0, 0, "Message", 0x118, 0x78, 0) != UI_MSGBOX_ANSWER_YES)
                return 0;
            mode_send_message(client_current_mode(), SELECT_REPLAY_REQ_QUIT, 0, 0, 0, 0);
            ui_window_mgr_post_quit2(&g_windowMgr, wnd);
            return 0;
        }
        return 0;
    case SELECT_REPLAY_LIST_PREV:
    case SELECT_REPLAY_LIST_NEXT:
        ui_list_box_send_msg(&self->list->list.base, NULL, msg, 0, 0, 0, 0);
        break;
    case SELECT_REPLAY_LIST_SELECT:
        ui_list_box_send_msg(&self->list->list.base, NULL, SELECT_REPLAY_LIST_SELECT, val1, 0, 0, 0);
        break;
    case REPLAY_MSG_SELECTED:
        break;
    case SELECT_REPLAY_MSG_PLACE_EXIT:
        ui_window_move_base(self->exit_button, SELECT_REPLAY_EXIT_X, self->exit_button->y);
        return 0;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }
    ui_select_replay_data_wnd_show_selected(self);
    ui_window_invalidate_base(wnd);
    return 0;
}

enum {
    REPLAY_ID_PLAY = 0x189,
    REPLAY_ID_PAUSE = 0x18a,
    REPLAY_ID_STOP = 0x18b,
    REPLAY_ID_END = 0x18c,
    REPLAY_ID_FASTER = 0x18d,
    REPLAY_ID_SLOWER = 0x18e,
};
enum {
    REPLAY_REQ_PLAY = 0x114,
    REPLAY_REQ_PAUSE = 0x115,
    REPLAY_REQ_STOP = 0x116,
    REPLAY_REQ_FASTER = 0x117,
    REPLAY_REQ_SLOWER = 0x118,
    REPLAY_REQ_END = 0x119,
};
#define REPLAY_MSG_REFRESH 0x3c
#define REPLAY_MSG_F3 0xf3

#define HOTKEY_CATEGORY_REPLAY 1
#define HOTKEY_REPLAY_PLAY 0x27
#define HOTKEY_REPLAY_FASTER 0x28
#define HOTKEY_REPLAY_SLOWER 0x29

#define MSG_REPLAY_CLOCK_FMT 0x7a3
#define MSG_REPLAY_SPEED_1 0x7a4
#define MSG_REPLAY_SPEED_OTHER 0x7ab

#define REPLAY_PLAY_X 5
#define REPLAY_PLAY_Y 0x59

struct UIReplayControlWnd {
    struct UIFrameWnd frame;
    struct UIReplayButton *play_button;
    struct UIReplayButton *pause_button;
    BYTE playing;
    struct UIBitmapButton *buttons[4];
    struct UIBarGraph4 *progress;
    struct UIWindow *clock_text;
    struct UIWindow *speed_text;
    struct UIWindow *location_text;
};
OFFSET_ASSERT(UIReplayControlWnd, buttons, 0xa0);

struct UIReplayControlWnd *ui_replay_control_wnd_construct(struct UIReplayControlWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_replay_control_wnd_ops;
    self->playing = 0;
    return self;
}

struct UIWindow *ui_replay_control_wnd_new(void)
{
    return (struct UIWindow *)ui_replay_control_wnd_construct(mem_alloc(sizeof(struct UIReplayControlWnd)));
}

void ui_replay_control_wnd_destroy(struct UIWindow *window)
{
    struct UIReplayControlWnd *self = (struct UIReplayControlWnd *)window;
    self->frame.base.ops = &ui_replay_control_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

static void set_hotkey_tooltip(struct UIBitmapButton *button, int index)
{
    struct HotKey key;

    session_get_hot_key(&g_session1, &key, HOTKEY_CATEGORY_REPLAY, index);
    ui_bitmap_button_set_tool_tip(button, string_cstr(&key.label));
    string_free(&key.action);
    string_free(&key.label);
}

BYTE ui_replay_control_wnd_refresh(struct UIReplayControlWnd *self)
{
    struct ReplayDuration length;
    int seconds, minutes, hours, index;
    char buf[100];

    memset(buf, 0, sizeof buf);
    seconds = replay_recorder_instance()->elapsed / 1000;
    minutes = seconds / 60;
    hours = minutes / 60;
    length = replay_recorder_duration(replay_recorder_instance());
    memset(buf, 0, sizeof buf);
    sprintf(buf, msg_str(MSG_REPLAY_CLOCK_FMT), hours, minutes - hours * 60, seconds - minutes * 60,
                (int)length.hours, (int)(signed char)length.minutes, (int)(signed char)length.seconds);
    ui_static_text_set_text_ellipsis(self->clock_text, buf);

    memset(buf, 0, sizeof buf);
    index = replay_recorder_speed_index(replay_recorder_instance());
    sprintf(buf, msg_str(index >= 1 && index <= 7 ? MSG_REPLAY_SPEED_1 + index - 1 : MSG_REPLAY_SPEED_OTHER));
    ui_static_text_set_text_ellipsis(self->speed_text, buf);

    if (client_current_mode()) {
        struct String location;

        game_mode_location_text((struct GameMode *)client_current_mode(), &location);
        ui_static_text_set_text_ellipsis(self->location_text, string_cstr(&location));
        string_free(&location);
    } else {
        ui_static_text_set_text_ellipsis(self->location_text, "");
    }
    ui_window_invalidate_base(&self->frame.base);
    ui_window_invalidate_children(&self->frame.base);
    return 1;
}

void ui_replay_control_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIReplayControlWnd *self = (struct UIReplayControlWnd *)window;
    static const char *const names[4] = { "replay_stop_", "replay_end_", "replay_speedadd_", "replay_speedmin_" };
    static const POINT button_pos[4] = { { 0x65, 0x59 }, { 0x95, 0x59 }, { 0x35, 0x59 }, { 0x35, 0x6f } };
    static const int ids[4] = { REPLAY_ID_STOP, REPLAY_ID_END, REPLAY_ID_FASTER, REPLAY_ID_SLOWER };
    static const POINT text_pos[4] = { { 8, 0x1a }, { 8, 0x28 }, { UI_POS_HIDDEN, UI_POS_HIDDEN }, { 8, 0x36 } };
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow **texts = &self->clock_text;
    struct UIBarGraph4 *bar;
    struct ReplayRecorder *rec;
    int i;

    for (i = 0; i < 4; i++)
        self->buttons[i] = add_replay_button(wnd, 0, names[i], button_pos[i].x, button_pos[i].y, ids[i]);
    self->play_button = (struct UIReplayButton *)add_replay_button(wnd, 1, "replay_start_", REPLAY_PLAY_X,
                                                                   REPLAY_PLAY_Y, REPLAY_ID_PLAY);
    self->pause_button = (struct UIReplayButton *)add_replay_button(wnd, 1, "replay_pause_", UI_POS_HIDDEN,
                                                                    UI_POS_HIDDEN, REPLAY_ID_PAUSE);

    {
        struct HotKey key;

        session_get_hot_key(&g_session1, &key, HOTKEY_CATEGORY_REPLAY, HOTKEY_REPLAY_PLAY);
        ui_bitmap_button_set_tool_tip(&self->play_button->bmp, string_cstr(&key.label));
        ui_bitmap_button_set_tool_tip(&self->pause_button->bmp, string_cstr(&key.label));
        string_free(&key.action);
        string_free(&key.label);
    }
    set_hotkey_tooltip(self->buttons[2], HOTKEY_REPLAY_FASTER);
    set_hotkey_tooltip(self->buttons[3], HOTKEY_REPLAY_SLOWER);

    bar = mem_alloc(sizeof *bar);
    if (bar) {
        ui_bar_graph3_construct(&bar->graph);
        bar->graph.base.ops = &ui_bar_graph4_ops;
    }
    self->progress = bar;
    bar->no_warning = 1;
    ui_window_create(&self->progress->graph.base, 0xbc, 0xb);
    ui_window_move_base(&self->progress->graph.base, 5, 0x48);
    rec = replay_recorder_instance();
    ui_bar_graph3_set_values(&self->progress->graph, &replay_recorder_instance()->elapsed,
                          (int *)&rec->info.totalTime);
    ui_window_add_child(wnd, &self->progress->graph.base);

    for (i = 0; i < 4; i++) {
        struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

        if (text)
            text = ui_static_text_construct(text);
        texts[i] = text;
        ui_window_create(text, 0xbc, 0xe);
        ui_static_text_set_text(texts[i], "", 0);
        ui_window_move(texts[i], text_pos[i].x, text_pos[i].y);
        ui_static_text_set_text_colour(texts[i], 100, 100, 200);
        ui_window_add_child(wnd, texts[i]);
    }
    ui_replay_control_wnd_refresh(self);
}

void ui_replay_control_wnd_on_draw(struct UIWindow *window)
{
    struct UIReplayControlWnd *self = (struct UIReplayControlWnd *)window;
    ui_window_draw_bitmap(&self->frame.base, 0, 0, ui_skin_bitmap(REPLAY_SKIN "replay_base.bmp"), 0);
}

static void replay_request(int request)
{
    mode_send_message(client_current_mode(), request, 0, 0, 0, 0);
}

int ui_replay_control_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          int val1, int val2, int val3, int val4)
{
    struct UIReplayControlWnd *self = (struct UIReplayControlWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case REPLAY_ID_PLAY:
            replay_request(REPLAY_REQ_PLAY);
            break;
        case REPLAY_ID_PAUSE:
            replay_request(REPLAY_REQ_PAUSE);
            break;
        case REPLAY_ID_STOP:
            replay_request(REPLAY_REQ_STOP);
            replay_recorder_instance()->speed = 1.0f;
            break;
        case REPLAY_ID_END:
            ui_window_mgr_post_quit2(&g_windowMgr, wnd);
            replay_request(REPLAY_REQ_END);
            replay_recorder_instance()->speed = 1.0f;
            break;
        case REPLAY_ID_FASTER:
            replay_request(REPLAY_REQ_FASTER);
            ui_replay_control_wnd_send_msg(wnd, NULL, REPLAY_MSG_REFRESH, 0, 0, 0, 0);
            break;
        case REPLAY_ID_SLOWER:
            replay_request(REPLAY_REQ_SLOWER);
            ui_replay_control_wnd_send_msg(wnd, NULL, REPLAY_MSG_REFRESH, 0, 0, 0, 0);
            break;
        }
        return 0;
    case UI_MSG_REPLAY_BUTTON:
        if (!self->playing) {
            ui_window_move_base(&self->play_button->bmp.button.base, UI_POS_HIDDEN, UI_POS_HIDDEN);
            ui_window_move_base(&self->pause_button->bmp.button.base, REPLAY_PLAY_X, REPLAY_PLAY_Y);
            self->playing = 1;
        } else {
            ui_window_move_base(&self->play_button->bmp.button.base, REPLAY_PLAY_X, REPLAY_PLAY_Y);
            ui_window_move_base(&self->pause_button->bmp.button.base, UI_POS_HIDDEN, UI_POS_HIDDEN);
            self->playing = 0;
        }
        ui_window_invalidate_base(wnd);
        ui_window_invalidate_children(wnd);
        return 0;
    case REPLAY_MSG_REFRESH:
        ui_replay_control_wnd_refresh(self);
        return 0;
    case REPLAY_MSG_F3:
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UIListBox *ui_select_replay_list(struct UIWindow *wnd)
{
    return &((struct UISelectReplayDataWnd *)wnd)->list->list;
}
