#include "ui/controls/listbox.h"
#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include "render/rp_pool.h"
#include "contents/sound.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "contents/mouse.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include <string.h>

#include "ui/character/character.h"
#include "ui/character/select_server.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_talk.h"

struct Vec;
struct UISelectServerWnd;
struct UIWindow;
void ui_select_server_wnd_destroy(struct UIWindow *window);
void ui_select_server_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_select_server_wnd_on_destroy(struct UIWindow *self);
void ui_select_server_wnd_on_draw(struct UIWindow *self);
int ui_select_server_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_select_server_wnd_ops = {
    .destroy = ui_select_server_wnd_destroy,
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
    .on_create = ui_select_server_wnd_on_create,
    .on_destroy = ui_select_server_wnd_on_destroy,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_select_server_wnd_on_draw,
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
    .send_msg = ui_select_server_wnd_send_msg,
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
struct Vec;
struct UIServerListBox;
struct UIWindow;
void ui_server_list_box_destroy(struct UIWindow *window);
void ui_server_list_box_on_mouse_hover(struct UIWindow *self, int x, int y);
const struct UIListBoxOps ui_server_list_box_ops = {
    .base = {
        .destroy = ui_server_list_box_destroy,
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
        .on_l_btn_down = ui_list_box_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_server_list_box_on_mouse_hover,
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

struct Vec;
void ui_color_trans_balloon_text_destroy(struct UIWindow *window);
const struct UIWindowOps ui_color_trans_balloon_text_ops = {
    .destroy = ui_color_trans_balloon_text_destroy,
    .resize = ui_trans_balloon_text_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_trans_balloon_text_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_trans_balloon_text_get_backdrop_box,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_balloon_text_is_hit_testable,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_trans_balloon_text_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_balloon_text_on_process,
    .on_draw = ui_color_trans_balloon_text_on_draw,
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

struct UIServerListBox {
    struct UIListBox list;
    struct Vec balloon_texts;
    struct UIWindow *balloon;
};
OFFSET_ASSERT(UIServerListBox, balloon, 0xdc);
SIZE_ASSERT(UIServerListBox, 0xe0);

struct UIServerListBox *ui_server_list_box_construct(struct UIServerListBox *self)
{
    ui_list_box_construct(&self->list);
    self->list.base.ops = &ui_server_list_box_ops.base;
    self->balloon_texts.first = NULL;
    self->balloon_texts.last = NULL;
    self->balloon_texts.end = NULL;
    self->balloon = NULL;
    return self;
}

void ui_server_list_box_destroy(struct UIWindow *window)
{
    struct UIServerListBox *self = (struct UIServerListBox *)window;
    self->list.base.ops = &ui_server_list_box_ops.base;
    string_vec_free(&self->balloon_texts);
    self->list.base.ops = &ui_list_box_ops.base;
    string_vec_free(&self->list.items);
    ui_window_destruct(&self->list.base);
    mem_free(self);
}

static void drop_balloon(struct UIServerListBox *self)
{
    ui_window_mgr_post_quit2(&g_windowMgr, self->balloon);
    self->balloon = NULL;
}

void ui_server_list_box_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIServerListBox *self = (struct UIServerListBox *)window;
    int index = ui_list_box_hit_line(&self->list, x, y);
    int mouse_y = g_mouse.y;
    struct String *texts = self->balloon_texts.first;
    struct UIWindow *balloon;
    int screen_x, screen_y;

    if (self->balloon_texts.first == self->balloon_texts.last)
        return;
    if (index == -1 || texts[index].size == 0) {
        drop_balloon(self);
        return;
    }
    screen_x = 0;
    screen_y = 0;
    if (self->balloon)
        drop_balloon(self);
    balloon = mem_alloc(sizeof(struct UITransBalloonText));
    if (balloon) {
        ui_trans_balloon_text_construct(balloon);
        balloon->ops = &ui_color_trans_balloon_text_ops;
    }
    self->balloon = balloon;
    ui_window_create(balloon, 8, 8);
    ui_window_to_screen(&self->list.base, &screen_x, &screen_y);
    ui_trans_balloon_text_move(self->balloon, screen_x + 0x50, mouse_y - 6);
    ui_window_mgr_add_window(&g_windowMgr, self->balloon);
    ui_balloon_text_set_text((struct UIBalloonText *)self->balloon, string_cstr(&texts[index]), 0x32);
}

#define SELECT_SERVER_ID_LIST 0xb0
#define SELECT_SERVER_ID_OK 0xb0
#define SELECT_SERVER_ID_CANCEL 0xb1
#define SELECT_SERVER_ID_EXIT 0xd5
#define MSG_SELECT_SERVER_TITLE 0x822
#define MSG_SELECT_ESCAPE_ZONE_TITLE 0x823

struct UISelectServerWnd {
    struct UIFrameWnd frame;
    struct UIServerListBox *list;
    struct UIWindow *exit_button;
    struct String server_value;
    int server;
    int mode_request;
};
OFFSET_ASSERT(UISelectServerWnd, server_value, 0x9c);

struct UISelectServerWnd *ui_select_server_wnd_construct(struct UISelectServerWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_select_server_wnd_ops;
    string_init(&self->server_value);
    self->list = NULL;
    self->exit_button = NULL;
    return self;
}

struct UIWindow *ui_select_server_wnd_new(void)
{
    return (struct UIWindow *)ui_select_server_wnd_construct(mem_alloc(sizeof(struct UISelectServerWnd)));
}

void ui_select_server_wnd_destroy(struct UIWindow *window)
{
    struct UISelectServerWnd *self = (struct UISelectServerWnd *)window;
    self->frame.base.ops = &ui_select_server_wnd_ops;
    string_free(&self->server_value);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

static const struct {
    int id;
    const char *name[3];
} select_server_buttons[3] = {
    { SELECT_SERVER_ID_OK, { "btn_ok", "btn_ok_a", "btn_ok_b" } },
    { SELECT_SERVER_ID_CANCEL, { "btn_cancel", "btn_cancel_a", "btn_cancel_b" } },
    { SELECT_SERVER_ID_EXIT, { "login_interface\\btn_exit", "login_interface\\btn_exit_a",
                               "login_interface\\btn_exit_b" } },
};

void ui_select_server_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UISelectServerWnd *self = (struct UISelectServerWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIServerListBox *list = mem_alloc(sizeof *list);
    int x[3] = { 0xbd, 0xea, 0x1b2 };
    int y[3] = { height - 0x18, height - 0x18, height - 0x18 };
    char path[3][0x80];
    int i, state;

    if (list)
        list = ui_server_list_box_construct(list);
    self->list = list;
    ui_list_box_create(&list->list, 0xc, 0x16, width - 0x18, height - 0x37, 0);
    self->list->list.r = 0xf0;
    self->list->list.g = 0xf0;
    self->list->list.b = 0xf0;
    self->list->list.base.id = SELECT_SERVER_ID_LIST;
    ui_window_add_child(wnd, &self->list->list.base);

    for (i = 0; i < 3; i++) {
        struct UIBitmapButton *button;

        for (state = 0; state < 3; state++)
            sprintf(path[state], UI_SKIN "%s.bmp", select_server_buttons[i].name[state]);
        button = ui_bitmap_button(path[0], path[1], path[2]);
        ui_window_move_base(&button->button.base, x[i], y[i]);
        button->button.base.id = select_server_buttons[i].id;
        ui_window_add_child(wnd, &button->button.base);
        if (select_server_buttons[i].id == SELECT_SERVER_ID_EXIT)
            self->exit_button = &button->button.base;
    }
    UI_FRAME_DEFAULT_ID(self) = SELECT_SERVER_ID_OK;
}

void ui_select_server_wnd_on_destroy(struct UIWindow *self)
{
}

void ui_select_server_wnd_on_draw(struct UIWindow *window)
{
    struct UISelectServerWnd *self = (struct UISelectServerWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    const char *title = msg_str(MSG_SELECT_SERVER_TITLE);

    if (mode_mgr_get_login_mode(&g_modeMgr)->sub_mode == 0x1e)
        title = msg_str(MSG_SELECT_ESCAPE_ZONE_TITLE);
    ui_window_draw_title_bar(wnd, 1, title, 0);
    ui_window_draw_body(wnd, 0x11, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
}

#define SELECT_SERVER_LIST_PREV 0x12
#define SELECT_SERVER_LIST_NEXT 0x13
#define SELECT_SERVER_LIST_SELECT 0x28
#define SELECT_SERVER_MSG_SETUP 0x50
#define SELECT_SERVER_MSG_PLACE_EXIT 0x59
#define SELECT_SERVER_EXIT_X 0xea
#define SELECT_SERVER_REQ_QUIT 2
#define SELECT_SERVER_REQ_CANCEL 0x2716
#define MSG_SELECT_SERVER_QUIT_ASK 0x11

int ui_select_server_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UISelectServerWnd *self = (struct UISelectServerWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *list = &self->list->list.base;
    const char *value;
    HKEY probe, key;
    DWORD type, size, disposition;

    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case SELECT_SERVER_ID_OK:
            if (self->list->list.selected == -1)
                return 0;
            ui_window_mgr_post_quit2(&g_windowMgr, wnd);
            mode_send_message(client_current_mode(), self->mode_request, self->list->list.selected, 0, 0, 0);
            sound_play_wave("버튼소리.wav"  , 0.0f, 0.0f,
                            0.0f, 0xfa, 0x28, 1.0f);
            value = string_cstr(&self->server_value);
            self->server = self->list->list.selected;
            disposition = 0;
            if (RegCreateKeyExA(HKEY_CURRENT_USER, g_registryKeyPath, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key,
                                &disposition) != ERROR_SUCCESS)
                return 0;
            RegSetValueExA(key, value, 0, REG_DWORD, (const BYTE *)&self->server, 4);
            RegCloseKey(key);
            return 0;
        case SELECT_SERVER_ID_CANCEL:
            ui_window_mgr_post_quit2(&g_windowMgr, wnd);
            mode_send_message(client_current_mode(), SELECT_SERVER_REQ_CANCEL, 0, 0, 0, 0);
            return 0;
        case SELECT_SERVER_ID_EXIT:
            if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_SELECT_SERVER_QUIT_ASK), UI_MSGBOX_QUESTION, 1,
                                       0, 0, "메시지", 0x118, 0x78, 0) != UI_MSGBOX_ANSWER_YES)
                return 0;
            mode_send_message(client_current_mode(), SELECT_SERVER_REQ_QUIT, 0, 0, 0, 0);
            ui_window_mgr_post_quit2(&g_windowMgr, wnd);
            return 0;
        }
        return 0;
    case SELECT_SERVER_LIST_PREV:
    case SELECT_SERVER_LIST_NEXT:
        ui_list_box_send_msg(list, NULL, msg, 0, 0, 0, 0);
        return 0;
    case SELECT_SERVER_LIST_SELECT:
        ui_list_box_send_msg(list, NULL, SELECT_SERVER_LIST_SELECT, self->server, 0, 0, 0);
        return 0;
    case SELECT_SERVER_MSG_SETUP:
        self->mode_request = val1;
        string_assign_cstr(&self->server_value, (const char *)val2);
        value = string_cstr(&self->server_value);
        self->server = CLIENT_LANGTYPE;
        if (RegOpenKeyExA(HKEY_CURRENT_USER, g_registryKeyPath, 0, KEY_QUERY_VALUE, &probe) != ERROR_SUCCESS)
            return 0;
        RegCloseKey(probe);
        if (RegOpenKeyExA(HKEY_CURRENT_USER, g_registryKeyPath, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
            return 0;
        size = 4;
        type = 4;
        RegQueryValueExA(key, value, NULL, &type, (BYTE *)&self->server, &size);
        RegCloseKey(key);
        return 0;
    case SELECT_SERVER_MSG_PLACE_EXIT:
        ui_window_move_base(self->exit_button, SELECT_SERVER_EXIT_X, self->exit_button->y);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

void ui_color_trans_balloon_text_destroy(struct UIWindow *self)
{
    ui_trans_balloon_text_destruct((struct UITransBalloonText *)self);
    mem_free(self);
}

struct UIListBox *ui_select_server_list(struct UIWindow *wnd)
{
    return &((struct UISelectServerWnd *)wnd)->list->list;
}

struct Vec *ui_server_list_balloons(struct UIListBox *list)
{
    return &((struct UIServerListBox *)list)->balloon_texts;
}
