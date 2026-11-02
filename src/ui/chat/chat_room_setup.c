#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <string.h>
#include "ui/chat/chat.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_jobs.h"
#include "session/session_party.h"

struct Vec;
struct UIChatRoomMakeWnd;
struct UIWindow;
void ui_chat_room_make_wnd_destroy(struct UIWindow *window);
void ui_chat_room_make_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_chat_room_make_wnd_on_draw(struct UIWindow *self);
intptr_t ui_chat_room_make_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_chat_room_make_wnd_ops = {
    .destroy = ui_chat_room_make_wnd_destroy,
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
    .on_create = ui_chat_room_make_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_chat_room_make_wnd_on_draw,
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
    .send_msg = ui_chat_room_make_wnd_send_msg,
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
void ui_chat_room_change_wnd_destroy(struct UIWindow *window);
intptr_t ui_chat_room_change_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_chat_room_change_wnd_ops = {
    .destroy = ui_chat_room_change_wnd_destroy,
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
    .on_create = ui_chat_room_make_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_chat_room_make_wnd_on_draw,
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
    .send_msg = ui_chat_room_change_wnd_send_msg,
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

#define MSG_CHAT_ROOM_MAKE_TITLE 0x7d
#define MSG_CHAT_ROOM_CHANGE_TITLE 0x7e
#define MSG_CHAT_ROOM_KIND 0x82
#define MSG_CHAT_ROOM_PEOPLE 0x83
#define MSG_CHAT_ROOM_PRIVATE 0xc8
#define MSG_CHAT_ROOM_PUBLIC 0xc9
#define MSG_CHAT_ROOM_NO_TITLE 0xd
#define MSG_CHAT_ROOM_NO_PASSWORD 0xf
#define MSG_CHAT_ROOM_SHORT_PASSWORD 0x10
#define MSG_CHAT_ROOM_NOT_ASCII 0xbe
#define MSG_CHAT_ROOM_REFUSED 0x295

#define CHAT_ROOM_ID_PEOPLE_LIST 0xdc
#define CHAT_ROOM_ID_KIND_LIST 0xdd

#define REQ_CHAT_ROOM_MAKE 0x2b
#define REQ_CHAT_ROOM_CHANGE 0x2c
#define CHAT_ROOM_MSG_SETUP 0x22

struct UIChatRoomMakeWnd {
    struct UIFrameWnd frame;
    struct String title;
    struct UIWindow *title_edit;
    struct UIWindow *password_edit;
    struct UIWindow *radio[2];
    struct UIWindow *people_label;
    struct UIWindow *kind_label;
    struct UIWindow *close_button;
    int is_private;
    int list_open;
    int limit;
    int kind;
    struct Vec people_names;
    struct Vec kind_names;
    struct Vec people_values;
    struct Vec kind_values;
};

static void chat_room_construct(struct UIChatRoomMakeWnd *self)
{
    const char *title;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_chat_room_make_wnd_ops;
    string_init(&self->title);
    memset(&self->people_names, 0, 4 * sizeof(struct Vec));
    title = msg_str(MSG_CHAT_ROOM_MAKE_TITLE);
    string_assign_cstr(&self->title, title);
}

struct UIChatRoomMakeWnd *ui_chat_room_make_wnd_construct(struct UIChatRoomMakeWnd *self)
{
    chat_room_construct(self);
    return self;
}

struct UIWindow *ui_chat_room_make_wnd_new(void)
{
    return (struct UIWindow *)ui_chat_room_make_wnd_construct(mem_alloc(sizeof(struct UIChatRoomMakeWnd)));
}

struct UIChatRoomMakeWnd *ui_chat_room_change_wnd_construct(struct UIChatRoomMakeWnd *self)
{
    const char *title;

    chat_room_construct(self);
    self->frame.base.ops = &ui_chat_room_change_wnd_ops;
    title = msg_str(MSG_CHAT_ROOM_CHANGE_TITLE);
    string_assign_cstr(&self->title, title);
    return self;
}

struct UIWindow *ui_chat_room_change_wnd_new(void)
{
    return (struct UIWindow *)ui_chat_room_change_wnd_construct(mem_alloc(sizeof(struct UIChatRoomMakeWnd)));
}

void ui_chat_room_make_wnd_destruct(struct UIChatRoomMakeWnd *self)
{
    self->frame.base.ops = &ui_chat_room_make_wnd_ops;
    vec_free(&self->kind_values);
    vec_free(&self->people_values);
    string_vec_free(&self->kind_names);
    string_vec_free(&self->people_names);
    string_free(&self->title);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_chat_room_make_wnd_destroy(struct UIWindow *window)
{
    struct UIChatRoomMakeWnd *self = (struct UIChatRoomMakeWnd *)window;
    ui_chat_room_make_wnd_destruct(self);
    mem_free(self);
}

void ui_chat_room_change_wnd_destroy(struct UIWindow *window)
{
    struct UIChatRoomMakeWnd *self = (struct UIChatRoomMakeWnd *)window;
    self->frame.base.ops = &ui_chat_room_change_wnd_ops;
    ui_chat_room_make_wnd_destruct(self);
    mem_free(self);
}

static void add_choice(struct Vec *names, struct Vec *values, const char *prefix, int msg_id,
                       int value)
{
    struct String line;
    const char *text = msg_str(msg_id);

    string_init(&line);
    string_assign_cstr(&line, prefix);
    string_append_cstr(&line, text);
    string_vec_push_move(names, &line);
    string_free(&line);
    vec_push(values, &value, sizeof value);
}

static void add_buttons(struct UIWindow *wnd, const char *const bitmaps[2][3], const int xy[2][2],
                        const int ids[2])
{
    int i, state;

    for (i = 0; i < 2; i++) {
        struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

        if (button)
            button = ui_bitmap_button_construct(button);
        for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++)
            ui_bitmap_button_set_bitmap(button, bitmaps[i][state], state, 0);
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, xy[i][0], xy[i][1]);
        button->button.base.id = ids[i];
        ui_window_add_child(wnd, &button->button.base);
    }
}

static struct UIWindow *add_edit(struct UIWindow *wnd, int w, int h, int x, int y, int max_chars, int mask)
{
    struct UIWindow *edit = mem_alloc(sizeof(struct UIEditCtrl));

    if (edit)
        edit = ui_edit_ctrl_construct(edit);
    ui_window_create(edit, w, h);
    ui_window_move(edit, x, y);
    UI_EDIT_MAX_CHARS(edit) = max_chars;
    if (mask)
        UI_EDIT_MASK_CHAR(edit) = mask;
    ui_edit_ctrl_set_back_colour(edit, 0xf2, 0xf2, 0xf2);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, "");
    ui_window_add_child(wnd, edit);
    return edit;
}

static struct UIWindow *add_label(struct UIWindow *wnd, const struct String *text, int x, int y)
{
    struct UIWindow *label = mem_alloc(sizeof(struct UIStaticText));

    if (label)
        label = ui_static_text_construct(label);
    ui_window_create(label, 0x44, 0xe);
    ui_static_text_set_text(label, string_cstr(text), 0);
    ui_window_move(label, x, y);
    ((struct UIStaticText *)(label))->background = 0xfff2f2f2;
    ui_window_invalidate(label);
    ui_window_add_child(wnd, label);
    return label;
}

void ui_chat_room_make_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIChatRoomMakeWnd *self = (struct UIChatRoomMakeWnd *)window;
    static const char *const ok_cancel[2][3] = {
        { UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp" },
        { UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp", UI_SKIN "btn_cancel_b.bmp" },
    };
    static const int ok_cancel_xy[2][2] = { { 0xbd, 0x60 }, { 0xea, 0x60 } };
    static const int ok_cancel_ids[2] = { UI_ID_OK, UI_ID_CANCEL };
#define TXTBOX_BTN(c) UI_SKIN "basic_interface\\txtbox_btn_" c ".bmp"
    static const char *const list_buttons[2][3] = {
        { TXTBOX_BTN("a"), TXTBOX_BTN("b"), TXTBOX_BTN("c") },
        { TXTBOX_BTN("a"), TXTBOX_BTN("b"), TXTBOX_BTN("c") },
    };
#undef TXTBOX_BTN
    static const int list_buttons_xy[2][2] = { { 0x77, 0x2e }, { 0x100, 0x2e } };
    static const int list_button_ids[2] = { CHAT_ROOM_ID_PEOPLE_LIST, CHAT_ROOM_ID_KIND_LIST };
    static const char *const people[] = { "2 ", "3 ", "5 ", "10 ", "15 ", "20 " };
    static const int people_values[] = { 2, 3, 5, 10, 15, 20 };
    struct UIWindow *wnd = &self->frame.base;
    const char *radio_labels[2];
    int radio_states[2] = { 1, 0 };
    int i, count;

    if (session_has_trick_dead(&g_session1)) {
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)msg_str(MSG_CHAT_ROOM_REFUSED), 0xff, 0,
                            0);
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_CHAT_ROOM_MAKE);
    }
    self->limit = 20;
    self->kind = 0;
    add_choice(&self->kind_names, &self->kind_values, "", MSG_CHAT_ROOM_KIND, 0);
    for (i = 0; i < 6; i++)
        add_choice(&self->people_names, &self->people_values, people[i], MSG_CHAT_ROOM_PEOPLE, people_values[i]);

    self->close_button = ui_window_add_close_button(wnd, "Alt+C", 0);
    add_buttons(wnd, ok_cancel, ok_cancel_xy, ok_cancel_ids);
    UI_FRAME_DEFAULT_ID(self) = UI_ID_OK;
    add_buttons(wnd, list_buttons, list_buttons_xy, list_button_ids);

    self->title_edit = add_edit(wnd, 0xe4, 0x10, 0x2d, 0x18, 0x24, 0);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->title_edit);
    self->password_edit = add_edit(wnd, 0x5a, 0x10, 0xb6, 0x46, 8, '*');

    self->is_private = 0;
    radio_labels[0] = msg_str(MSG_CHAT_ROOM_PUBLIC);
    radio_labels[1] = msg_str(MSG_CHAT_ROOM_PRIVATE);
    for (i = 0; i < 2; i++) {
        struct UIWindow *radio = mem_alloc(sizeof(struct UIRadioBtn));

        if (radio)
            radio = (struct UIWindow *)ui_radio_btn_construct((struct UIRadioBtn *)radio);
        self->radio[i] = radio;
        ui_window_create(radio, 0x28 + 0xc * i, 0xc);
        ui_window_move_base(radio, 0x2c + 0x34 * i, 0x46);
        ui_radio_btn_set_label(radio, radio_labels[i]);
        ui_radio_btn_send_msg(radio, wnd, UI_MSG_SET_STATE, radio_states[i], 0, 0, 0);
        ui_window_add_child(wnd, radio);
    }

    count = (int)VEC_COUNT(&self->people_names, struct String);
    self->people_label = add_label(wnd, &VEC_AT(&self->people_names, struct String, count - 1), 0x30,
                                   0x31);
    self->kind_label = add_label(wnd, &VEC_AT(&self->kind_names, struct String, 0), 0xbb, 0x31);
}

void ui_chat_room_make_wnd_on_draw(struct UIWindow *window)
{
    struct UIChatRoomMakeWnd *self = (struct UIChatRoomMakeWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\titlebar_fix.bmp"), 0);
    ui_window_draw_bitmap(wnd, 0, 0x11, ui_skin_bitmap(UI_SKIN "basic_interface\\chatwin0_bg.bmp"), 0);
    ui_window_text_out(wnd, 9, 4, string_cstr(&self->title), 0, 0, 0xc, RGB(0xff, 0xff, 0xff), 0, 0);
    ui_window_text_out(wnd, 8, 3, string_cstr(&self->title), 0, 0, 0xc, RGB(0, 0, 0), 0, 0);
}

static void chat_room_ok(struct UIChatRoomMakeWnd *self, int request, int wnd_id)
{
    struct ChatRoomInfo setup;
    int msg = 0;

    if (ui_edit_ctrl_get_text_size(self->title_edit) == 0)
        msg = MSG_CHAT_ROOM_NO_TITLE;
    else if (g_serviceType == 1 && !session_is_ascii_name(&g_session1, ui_edit_ctrl_get_text(self->title_edit)))
        msg = MSG_CHAT_ROOM_NOT_ASCII;
    else if (ui_edit_ctrl_get_text_size(self->password_edit) < 4 && self->is_private == 1)
        msg = ui_edit_ctrl_get_text_size(self->password_edit) == 0 ? MSG_CHAT_ROOM_NO_PASSWORD
                                                                      : MSG_CHAT_ROOM_SHORT_PASSWORD;
    if (msg) {
        ui_message_box(msg_str(msg));
        return;
    }

    string_init(&setup.title);
    string_init(&setup.password);
    setup.max_users = self->limit;
    setup.users = 0;
    setup.type = self->is_private != 1;
    setup.room_id = -1;
    {
        const char *text = ui_edit_ctrl_get_text(self->title_edit);
        string_assign_cstr(&setup.title, text);
        text = ui_edit_ctrl_get_text(self->password_edit);
        string_assign_cstr(&setup.password, text);
    }
    mode_send_message(g_modeMgr.curMode, request, (intptr_t)&setup, 0, 0, 0);
    ui_window_mgr_post_quit1(&g_windowMgr, wnd_id);
    string_free(&setup.password);
    string_free(&setup.title);
}

static void chat_room_open_list(struct UIChatRoomMakeWnd *self, int id, int wnd_id)
{
    struct UIWindow *combo;
    struct Vec *names;
    int count, i, x, y;

    if (g_windowMgr.combo_box_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_COMBO_BOX);
    combo = ui_window_open(WND_ID_COMBO_BOX);
    self->list_open = id == CHAT_ROOM_ID_KIND_LIST;
    names = self->list_open ? &self->kind_names : &self->people_names;
    count = (int)VEC_COUNT(names, struct String);
    for (i = 0; i < count; i++)
        ui_window_send_msg(combo, NULL, COMBO_MSG_ADD_LINE,
                 (intptr_t)string_cstr(&VEC_AT(names, struct String, i)), 0, 0, 0);
    ui_window_send_msg(combo, NULL, COMBO_MSG_SELECT, 0, 0, 0, 0);
    ui_window_resize(combo, 0x4a, count * 16 < 0x30 ? count * 16 : 0x30);
    ui_window_send_msg(combo, NULL, COMBO_MSG_OPENER_ID, wnd_id, 0, 0, 0);
    x = self->list_open * 0x89 + 0x2c;
    y = 0x40;
    ui_window_to_screen(&self->frame.base, &x, &y);
    ui_window_move(combo, x + 1, y);
}

intptr_t ui_chat_room_change_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIChatRoomMakeWnd *self = (struct UIChatRoomMakeWnd *)window;
    if (msg == CHAT_ROOM_MSG_SETUP) {
        const struct ChatRoomInfo *setup = (const struct ChatRoomInfo *)val1;
        int count = (int)VEC_COUNT(&self->people_names, struct String);
        int i;

        self->limit = setup->max_users;
        ui_static_text_set_text(self->people_label,
                             string_cstr(&VEC_AT(&self->people_names, struct String, count - 1)), 0);
        ui_window_invalidate(self->people_label);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->password_edit, string_cstr(&setup->password));
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->title_edit, string_cstr(&setup->title));
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->title_edit);
        ui_edit_ctrl_select_all(self->title_edit);
        self->is_private = setup->type == CHAT_ROOM_PRIVATE;
        for (i = 0; i < 2; i++) {
            ui_window_invalidate_base(self->radio[i]);
            ui_radio_btn_send_msg(self->radio[i], &self->frame.base, UI_MSG_SET_STATE, i == self->is_private, 0, 0, 0);
        }
        ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)self->close_button, "");
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    }
    if (msg == UI_MSG_COMMAND) {
        switch (val1) {
        case UI_ID_OK:
            chat_room_ok(self, REQ_CHAT_ROOM_CHANGE, WND_ID_CHAT_ROOM_CHANGE);
            return 0;
        case UI_ID_CANCEL:
        case UI_ID_CLOSE:
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_CHAT_ROOM_CHANGE);
            return 0;
        case CHAT_ROOM_ID_PEOPLE_LIST:
        case CHAT_ROOM_ID_KIND_LIST:
            chat_room_open_list(self, val1, WND_ID_CHAT_ROOM_CHANGE);
            return 0;
        }
    }
    return ui_chat_room_make_wnd_send_msg((struct UIWindow *)(self), sender, msg, val1, val2, val3, val4);
}

#define MSG_NO_TITLE 0xd
#define MSG_NO_PASSWORD 0xf
#define MSG_SHORT_PASSWORD 0x10
#define MSG_NOT_ASCII 0xbe
#define ID_RADIO 0xcf
#define ID_PEOPLE_LIST 0xdc
#define ID_KIND_LIST 0xdd
#define REQ_CHAT_ROOM_MAKE 0x2b

static void make_ok(struct UIChatRoomMakeWnd *self)
{
    struct ChatRoomInfo setup;
    const char *text;
    int msg = 0;

    if (!ui_edit_ctrl_get_text_size(self->title_edit))
        msg = MSG_NO_TITLE;
    else if (g_serviceType == 1 && !session_is_ascii_name(&g_session1, ui_edit_ctrl_get_text(self->title_edit)))
        msg = MSG_NOT_ASCII;
    else if (ui_edit_ctrl_get_text_size(self->password_edit) < 4 && self->is_private == 1)
        msg = ui_edit_ctrl_get_text_size(self->password_edit) ? MSG_SHORT_PASSWORD : MSG_NO_PASSWORD;
    if (msg) {
        ui_message_box(msg_str(msg));
        return;
    }
    string_init(&setup.title);
    string_init(&setup.password);
    setup.max_users = self->limit;
    setup.users = 0;
    setup.type = self->is_private != 1;
    setup.room_id = -1;
    text = ui_edit_ctrl_get_text(self->title_edit);
    string_assign_cstr(&setup.title, text);
    text = ui_edit_ctrl_get_text(self->password_edit);
    string_assign_cstr(&setup.password, text);
    mode_send_message(client_current_mode(), REQ_CHAT_ROOM_MAKE, (intptr_t)&setup, 0, 0, 0);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_CHAT_ROOM_MAKE);
    string_free(&setup.password);
    string_free(&setup.title);
}

static void make_open_list(struct UIChatRoomMakeWnd *self, int id)
{
    struct UIWindow *combo;
    struct Vec *names;
    int count, i, x, y;

    if (g_windowMgr.combo_box_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_COMBO_BOX);
    combo = ui_window_open(WND_ID_COMBO_BOX);
    self->list_open = id == ID_KIND_LIST;
    names = self->list_open ? &self->kind_names : &self->people_names;
    count = (int)VEC_COUNT(names, struct String);
    for (i = 0; i < count; i++)
        ui_window_send_msg(combo, NULL, COMBO_MSG_ADD_LINE,
                 (intptr_t)string_cstr(&VEC_AT(names, struct String, i)), 0, 0, 0);
    ui_window_send_msg(combo, NULL, COMBO_MSG_SELECT, 0, 0, 0, 0);
    ui_window_resize(combo, 0x4a, count * 16 < 0x30 ? count * 16 : 0x30);
    ui_window_send_msg(combo, NULL, COMBO_MSG_OPENER_ID, WND_ID_CHAT_ROOM_MAKE, 0, 0, 0);
    x = self->list_open * 0x89 + 0x2c;
    y = 0x40;
    ui_window_to_screen(&self->frame.base, &x, &y);
    ui_window_move(combo, x + 1, y);
}

intptr_t ui_chat_room_make_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender,
                                         int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIChatRoomMakeWnd *self = (struct UIChatRoomMakeWnd *)window;
    int i;

    if (msg == UI_MSG_COMMAND) {
        switch (val1) {
        case UI_ID_OK:
            make_ok(self);
            break;
        case UI_ID_CANCEL:
        case UI_ID_CLOSE:
            ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_CHAT_ROOM_MAKE);
            break;
        case ID_RADIO:
            for (i = 0; i < 2; i++) {
                ui_window_invalidate_base(self->radio[i]);
                if (sender == self->radio[i]) {
                    self->is_private = i;
                    ui_radio_btn_send_msg(self->radio[i], &self->frame.base, UI_MSG_SET_STATE, 1, 0, 0, 0);
                } else {
                    ui_radio_btn_send_msg(self->radio[i], &self->frame.base, UI_MSG_SET_STATE, 0, 0, 0, 0);
                }
            }
            break;
        case ID_PEOPLE_LIST:
        case ID_KIND_LIST:
            make_open_list(self, val1);
            break;
        }
        return 0;
    }
    if (msg == COMBO_MSG_CHOSEN) {
        if (self->list_open == 0) {
            ui_static_text_set_text(self->people_label,
                                 string_cstr(&VEC_AT(&self->people_names, struct String, val1)), 0);
            self->limit = VEC_AT(&self->people_values, int, val1);
        } else if (self->list_open == 1) {
            ui_static_text_set_text(self->kind_label,
                                 string_cstr(&VEC_AT(&self->kind_names, struct String, val1)), 0);
            self->kind = VEC_AT(&self->kind_values, int, val1);
        }
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
