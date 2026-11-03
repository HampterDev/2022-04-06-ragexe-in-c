#include "contents/adventurer_agency.h"
#include "contents/character_settings.h"
#include "game/mode_misc.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/character/character.h"
#include "game/context.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_jobs.h"
#include "ui/ui_std.h"

#define WND_ESC_MENU 0x9b

#define ESC_ID_CHARACTER_SELECT 0x16b
#define ESC_ID_GRAPHIC_SETTING 0x1c2
#define ESC_ID_HOTKEY_SETTINGS 0x16a
#define ESC_ID_QUIT_GAME 0xf3
#define ESC_ID_CANCEL UI_ID_CANCEL
#define ESC_ID_RISE_HERE 0x159
#define ESC_ID_SAVE_POINT 0x16c

#define MODE_MSG_RESTART 0x19
#define MSG_ESC_MENU_TITLE 0x5cb
#define MSG_ASK_SAVE_POINT 0x60c

#define ESC_BUTTON_COUNT 7
#define ESC_BUTTON_X 0x1d
#define ESC_BUTTON_STEP 0x17
#define ESC_BUTTON_PARK (-100)

enum EscLayout {
    ESC_LAYOUT_FULL = 0,
    ESC_LAYOUT_SHORT = 1,
    ESC_LAYOUT_MIDDLE = 2,
};

struct UIEscOptionWnd {
    struct UIFrameWnd frame;
    struct UIBitmapButton *buttons[ESC_BUTTON_COUNT];
    int layout;
    int title_height;
};
OFFSET_ASSERT(UIEscOptionWnd, buttons, 0x94);
OFFSET_ASSERT(UIEscOptionWnd, layout, 0xb0);
SIZE_ASSERT(UIEscOptionWnd, 0xb8);

struct UIWindow;
void ui_esc_option_wnd_destroy(struct UIWindow *window);
void ui_esc_option_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_esc_option_wnd_on_draw(struct UIWindow *self);
int ui_esc_option_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_esc_option_wnd_ops = {
    .destroy = ui_esc_option_wnd_destroy,
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
    .on_create = ui_esc_option_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_esc_option_wnd_on_draw,
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
    .send_msg = ui_esc_option_wnd_send_msg,
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

static const char *const esc_button_names[ESC_BUTTON_COUNT] = {
    "esc_01", "esc_001", "esc_08", "esc_09", "esc_02", "esc_05", "esc_10",
};
static const int esc_button_ids[ESC_BUTTON_COUNT] = {
    ESC_ID_CHARACTER_SELECT, ESC_ID_GRAPHIC_SETTING, ESC_ID_HOTKEY_SETTINGS, ESC_ID_QUIT_GAME, ESC_ID_CANCEL, ESC_ID_RISE_HERE, ESC_ID_SAVE_POINT,
};

void ui_esc_option_wnd_destruct(struct UIEscOptionWnd *self)
{
    self->frame.base.ops = &ui_esc_option_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_esc_option_wnd_destroy(struct UIWindow *window)
{
    struct UIEscOptionWnd *self = (struct UIEscOptionWnd *)window;
    ui_esc_option_wnd_destruct(self);
    mem_free(self);
}

static void esc_set_bitmap(struct UIBitmapButton *button, const struct String *skin, const char *name,
                           const char *suffix, int state)
{
    struct String path;

    string_copy(&path, skin);
    string_append_cstr(&path, name);
    string_append(&path, suffix, 5);
    ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
    string_free(&path);
}

static void esc_place(struct UIBitmapButton *button, int y)
{
    ui_window_move_base(&button->button.base, ESC_BUTTON_X, y);
}

void ui_esc_option_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIEscOptionWnd *self = (struct UIEscOptionWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String skin;
    int i, top;

    string_init(&skin);
    string_assign(&skin, UI_SKIN, sizeof UI_SKIN - 1);
    for (i = 0; i < ESC_BUTTON_COUNT; i++) {
        struct UIBitmapButton *button = mem_alloc(sizeof *button);

        if (button)
            button = ui_bitmap_button_construct(button);
        self->buttons[i] = button;
        esc_set_bitmap(button, &skin, esc_button_names[i], "a.bmp", 0);
        esc_set_bitmap(button, &skin, esc_button_names[i], "b.bmp", 1);
        esc_set_bitmap(button, &skin, esc_button_names[i], "c.bmp", 2);
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, ESC_BUTTON_PARK, ESC_BUTTON_PARK);
        button->button.base.id = esc_button_ids[i];
        ui_window_add_child(wnd, &button->button.base);
    }
    for (i = 0; i < ESC_BUTTON_COUNT; i++)
        ui_window_move_base(&self->buttons[i]->button.base, ESC_BUTTON_PARK, ESC_BUTTON_PARK);

    top = self->title_height;
    if (self->layout == ESC_LAYOUT_FULL) {
        ui_window_resize_base(wnd, wnd->width, top + 0x7a);
        for (i = 0; i < 5; i++)
            esc_place(self->buttons[i], self->title_height + 5 + i * ESC_BUTTON_STEP);
    } else if (self->layout == ESC_LAYOUT_SHORT) {
        ui_window_resize_base(wnd, wnd->width, top + 0x35);
        esc_place(self->buttons[6], self->title_height + 5);
        esc_place(self->buttons[4], self->title_height + 0x1c);
    } else if (self->layout == ESC_LAYOUT_MIDDLE) {
        ui_window_resize_base(wnd, wnd->width, top + 0x4c);
        esc_place(self->buttons[5], self->title_height + 5);
        esc_place(self->buttons[6], self->title_height + 0x1c);
        esc_place(self->buttons[4], self->title_height + 0x33);
    }
    string_free(&skin);
}

void ui_esc_option_wnd_on_draw(struct UIWindow *window)
{
    struct UIEscOptionWnd *self = (struct UIEscOptionWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String id_text, title;

    session_scramble_number(&g_session1, &id_text, g_session1.ownGid);
    string_init(&title);
    string_format(&title, "%s (%s)", msg_str(MSG_ESC_MENU_TITLE), string_cstr(&id_text));
    self->title_height = ui_window_draw_title_bar(wnd, 1, string_cstr(&title), 0);
    string_free(&title);
    string_free(&id_text);
    ui_window_draw_body(wnd, self->title_height, 0, "");
}

static void esc_toggle_window(int window_id)
{
    if (!ui_close_window(window_id))
        ui_make_window(window_id);
}

int ui_esc_option_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                      int val2, int val3, int val4)
{
    struct UIEscOptionWnd *self = (struct UIEscOptionWnd *)window;
    struct Mode *mode = game_current_base_mode();

    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);

    switch (val1) {
    case ESC_ID_CANCEL:
        ui_close_window(WND_ESC_MENU);
        break;
    case ESC_ID_QUIT_GAME:
        ui_close_window(WND_ESC_MENU);
        mode_send_message(mode, 0x58, 0, 0, 0, 0);
        break;
    case ESC_ID_RISE_HERE:
        mode_send_message(mode, 0xfa, 0, 0, 0, 0);
        ui_close_window(WND_ESC_MENU);
        break;
    case ESC_ID_HOTKEY_SETTINGS:
        esc_toggle_window(0x9c);
        break;
    case ESC_ID_CHARACTER_SELECT:
        char_user_data_save(g_charUserDataMgr);
        adventurer_agency_cancel_own_booking(g_adventurerAgency);
        mode_send_message(mode, MODE_MSG_RESTART, 1, 0, 0, 0);
        ui_close_window(WND_ESC_MENU);
        ui_close_window(0xa4);
        break;
    case ESC_ID_SAVE_POINT:
        if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_ASK_SAVE_POINT), UI_MSGBOX_QUESTION, 0, 0, 0,
                                   "메시지", 0x118, 0x78, 0) == UI_MSGBOX_ANSWER_YES) {
            mode_send_message(mode, MODE_MSG_RESTART, 0, 0, 0, 0);
            ui_close_window(WND_ESC_MENU);
        }
        break;
    case ESC_ID_GRAPHIC_SETTING:
        esc_toggle_window(0x131);
        break;
    }
    return 0;
}

struct UIEscOptionWnd *ui_esc_option_wnd_construct(struct UIEscOptionWnd *self)
{
    struct GameMode *mode;

    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_esc_option_wnd_ops;
    self->title_height = 0x11;
    mode = game_current_mode();
    if (!game_player_is_dead())
        self->layout = ESC_LAYOUT_FULL;
    else if ((mode && game_current_world() && game_mode_has_resurrection_token(mode)) ||
             session_has_status_effect(&g_session1, 0x244))
        self->layout = ESC_LAYOUT_MIDDLE;
    else
        self->layout = ESC_LAYOUT_SHORT;
    return self;
}

struct UIWindow *ui_esc_option_wnd_new(void)
{
    return (struct UIWindow *)ui_esc_option_wnd_construct(mem_alloc(sizeof(struct UIEscOptionWnd)));
}
