#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "contents/sound.h"
#include "platform/client_config.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <string.h>
#include "ui/character/character.h"
#include <imm.h>
#include <mmsystem.h>
#include "core/app.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/quests/quests.h"

struct Vec;
struct UILoginWnd;
struct UIWindow;
void ui_login_wnd_destroy(struct UIWindow *window);
void ui_login_wnd_store_info(struct UIWindow *self);
void ui_login_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_login_wnd_on_destroy(struct UIWindow *self);
void ui_login_wnd_on_process(struct UIWindow *self);
void ui_login_wnd_on_draw(struct UIWindow *self);
void ui_login_wnd_on_mouse_move(struct UIWindow *self, int arg1_unused, int arg2_unused);
int ui_login_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_login_wnd_ops = {
    .destroy = ui_login_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_login_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_login_wnd_on_create,
    .on_destroy = ui_login_wnd_on_destroy,
    .on_process = ui_login_wnd_on_process,
    .on_draw = ui_login_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_login_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_login_wnd_send_msg,
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

#define WND_ID_LOGIN_BACKDROP 0x145

#define LOGIN_BUTTON_TEXT_HEIGHT 12
#define LOGIN_BUTTON_TEXT_COLOUR 0x313131

static struct UIBmpTextButton *add_text_button(struct UIWindow *parent, const char *normal,
                                               const char *hover, const char *pressed, int extra_height,
                                               int x, int y, int id, int caption)
{
    struct UIBmpTextButton *button = mem_alloc(sizeof(struct UIBmpTextButton));
    const char *text;
    int state;

    if (button)
        button = ui_bmp_text_button_construct(button);
    ui_bitmap_button_set_bitmap(&button->bmp, normal, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(&button->bmp, hover, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(&button->bmp, pressed, UI_BTN_DOWN, 0);
    ui_window_create(&button->bmp.button.base, button->bmp.bitmap_width,
                     button->bmp.bitmap_height + extra_height);
    ui_window_move_base(&button->bmp.button.base, x, y);
    button->bmp.button.base.id = id;
    text = msg_str(caption);
    string_assign_cstr(&button->caption, text);
    ui_window_invalidate_base(&button->bmp.button.base);
    for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++)
        ui_bmp_text_button_set_text_style(button, state, LOGIN_BUTTON_TEXT_HEIGHT, LOGIN_BUTTON_TEXT_COLOUR,
                                     1, 1, UI_TEXT_CENTRED, UI_TEXT_CENTRED);
    ui_window_add_child(parent, &button->bmp.button.base);
    return button;
}

#define LOGIN_ID_START 0xb2
#define LOGIN_ID_CLOSE 0xd5
#define LOGIN_ID_CANCEL 0x115
#define LOGIN_MAX_CHARS 24
#define LOGIN_PLACEHOLDER_GREY 0x9c

#define MSG_LOGIN_SAVE_ID 0xd53
#define MSG_LOGIN_CLOSE 0xd67
#define MSG_ACCOUNT_CLASS_0 0xcb2
#define MSG_ACCOUNT_CLASS_1 0xcb3
#define LOGIN_MSG_COMBO_CHANGED 0x27

struct UILoginWnd {
    struct UIFrameWnd frame;
    struct UIWindow *id_edit;
    struct UIWindow *password_edit;
    struct UIWindow *backdrop;
    BYTE id_placeholder;
    BYTE password_placeholder;
    int save_password;
    int save_id;
    int show_notices;
    int notice_top, notice_selected;
    struct Vec notice_lines;
    int notice_list_open;
    struct UIWindow *account_combo;
    const char *account_class;
};
OFFSET_ASSERT(UILoginWnd, id_placeholder, 0xa0);
OFFSET_ASSERT(UILoginWnd, save_id, 0xa8);
OFFSET_ASSERT(UILoginWnd, notice_lines, 0xb8);
OFFSET_ASSERT(UILoginWnd, account_combo, 0xc8);
SIZE_ASSERT(UILoginWnd, 0xd0);

#define LOGIN_REG_ID_SIZE 0x33
#define LOGIN_REG_PASSWORD_SIZE 0x19

void ui_login_wnd_load_saved_id(struct UILoginWnd *self)
{
    char id[0x34];
    char password[0x1a];
    HKEY probe, key;
    DWORD type, size, check_save, bitwise;
    int colour;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, g_registryKeyPath, 0, KEY_QUERY_VALUE, &probe) != ERROR_SUCCESS) {
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->id_edit, "");
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->password_edit, "");
        return;
    }
    RegCloseKey(probe);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, g_registryKeyPath, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return;
    size = 4;
    type = 4;
    RegQueryValueExA(key, "CHECKSAVE", NULL, &type, (BYTE *)&check_save, &size);
    check_save = check_save == 1 ? 1 : 0;
    RegQueryValueExA(key, "BITWISE", NULL, &type, (BYTE *)&bitwise, &size);
    memset(id, 0, sizeof id);
    memset(password, 0, sizeof password);
    type = REG_SZ;
    size = LOGIN_REG_PASSWORD_SIZE;
    RegQueryValueExA(key, "PASSWORD", NULL, &type, (BYTE *)password, &size);
    password[0x18] = 0;
    type = REG_SZ;
    size = LOGIN_REG_ID_SIZE;
    RegQueryValueExA(key, "ID", NULL, &type, (BYTE *)id, &size);
    id[0x32] = 0;
    password[0x18] = 0;
    self->save_id = check_save;
    if (check_save) {
        if (strcmp("", id) != 0 && strcmp("ID", id) != 0) {
            self->id_placeholder = 0;
            colour = 0;
        } else {
            self->id_placeholder = 1;
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->id_edit, "ID");
            colour = LOGIN_PLACEHOLDER_GREY;
        }
        ui_edit_ctrl_set_text_colour(self->id_edit, colour, colour, colour, -1);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->id_edit, id);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->password_edit, "");
    }
    RegCloseKey(key);
}

void ui_login_wnd_store_saved_id(struct UILoginWnd *self)
{
    char id[0x34];
    HKEY key;
    DWORD disposition = 0;
    DWORD check_save, bitwise;

    if (RegCreateKeyExA(HKEY_CURRENT_USER, g_registryKeyPath, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, &disposition) !=
        ERROR_SUCCESS)
        return;
    if (self->save_id)
        strcpy(id, ui_edit_ctrl_get_text(self->id_edit));
    else
        memset(id, 0, sizeof id);
    id[0x32] = 0;
    check_save = self->save_id ? 1 : 0;
    RegSetValueExA(key, "ID", 0, REG_SZ, (const BYTE *)id, 0x33);
    RegSetValueExA(key, "CHECKSAVE", 0, REG_DWORD, (const BYTE *)&check_save, 4);
    bitwise = 1;
    RegSetValueExA(key, "BITWISE", 0, REG_DWORD, (const BYTE *)&bitwise, 4);
    RegCloseKey(key);
}

struct UILoginWnd *ui_login_wnd_construct(struct UILoginWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_login_wnd_ops;
    self->notice_lines.first = NULL;
    self->notice_lines.last = NULL;
    self->notice_lines.end = NULL;
    self->account_combo = NULL;
    self->account_class = "G000";
    self->id_edit = NULL;
    self->password_edit = NULL;
    self->save_password = 0;
    self->save_id = 0;
    self->show_notices = 1;
    self->notice_top = 0;
    self->notice_selected = 0;
    self->backdrop = NULL;
    return self;
}

struct UIWindow *ui_login_wnd_new(void)
{
    return (struct UIWindow *)ui_login_wnd_construct(mem_alloc(sizeof(struct UILoginWnd)));
}

void ui_login_wnd_destroy(struct UIWindow *window)
{
    struct UILoginWnd *self = (struct UILoginWnd *)window;
    self->frame.base.ops = &ui_login_wnd_ops;
    string_vec_free(&self->notice_lines);
    ui_frame_wnd_destruct(&self->frame);
    mem_free(self);
}

static struct UIWindow *add_login_edit(struct UILoginWnd *self, int y)
{
    struct UIWindow *edit = mem_alloc(sizeof(struct UIEditCtrlCustom));

    if (edit)
        edit = ui_edit_ctrl_custom_construct(edit);
    ui_window_create(edit, 0x99, 0x10);
    ui_window_move(edit, 0xf, y);
    ((struct UIEditCtrlCustom *)edit)->align = UI_EDIT_ALIGN_CENTRE;
    ui_edit_ctrl_set_back_colour(edit, 0xff, 0xff, 0xff);
    return edit;
}

void ui_login_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UILoginWnd *self = (struct UILoginWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *label, *check, *combo;
    struct UIBitmapButton *button;
    struct String image;
    const char *id;

    self->backdrop = ui_window_open(WND_ID_LOGIN_BACKDROP);
    string_init(&image);
    string_assign_cstr(&image, UI_SKIN "login_interface\\bg_login.tga");
    uirp_image_wnd2_set_image((struct UIRPImageWnd2 *)self->backdrop, image, 5);
    self->password_placeholder = 1;

    label = mem_alloc(sizeof(struct UIStaticText));
    if (label)
        label = ui_static_text_construct(label);
    ui_window_create(label, 0x48, 0x16);
    ui_static_text_set_text(label, msg_str(MSG_LOGIN_SAVE_ID), 0);
    ui_static_text_set_text_colour(label, 0x33, 0x33, 0x33);
    ui_window_move(label, 0x49, 0x57);
    ui_static_text_set_font(label, 0, 11, 0, 0);
    ui_window_add_child(wnd, label);

    add_text_button(wnd, UI_SKIN "login_interface\\bt_close_normal.bmp",
                    UI_SKIN "login_interface\\bt_close_over.bmp", UI_SKIN "login_interface\\bt_close_press.bmp",
                    2, 0xd, 0x68, LOGIN_ID_CANCEL, MSG_LOGIN_CLOSE);

    button = ui_bitmap_button(UI_SKIN "login_interface\\bt_start_normal.bmp",
                              UI_SKIN "login_interface\\bt_start_over.bmp",
                              UI_SKIN "login_interface\\bt_start_press.bmp");
    ui_window_move_base(&button->button.base, 0xc0, 0x20);
    button->button.base.id = LOGIN_ID_START;
    ui_window_add_child(wnd, &button->button.base);
    button = ui_bitmap_button(UI_SKIN "login_interface\\bt_close2_normal.bmp",
                              UI_SKIN "login_interface\\bt_close2_over.bmp",
                              UI_SKIN "login_interface\\bt_close2_press.bmp");
    ui_window_move_base(&button->button.base, 0x111, 0xf);
    button->button.base.id = LOGIN_ID_CLOSE;
    ui_window_add_child(wnd, &button->button.base);
    UI_FRAME_DEFAULT_ID(self) = LOGIN_ID_START;

    self->password_edit = add_login_edit(self, 0x40);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->password_edit, "PASSWORD");
    ui_edit_ctrl_set_text_colour(self->password_edit, LOGIN_PLACEHOLDER_GREY, LOGIN_PLACEHOLDER_GREY,
                             LOGIN_PLACEHOLDER_GREY, -1);
    UI_EDIT_MASK_CHAR(self->password_edit) = '*';
    ui_window_add_child(wnd, self->password_edit);

    self->id_edit = add_login_edit(self, 0x28);
    ui_edit_ctrl_set_text_colour(self->id_edit, LOGIN_PLACEHOLDER_GREY, LOGIN_PLACEHOLDER_GREY,
                             LOGIN_PLACEHOLDER_GREY, -1);
    ui_window_add_child(wnd, self->id_edit);
    ui_login_wnd_load_saved_id(self);

    check = mem_alloc(sizeof(struct UICheckBox));
    if (check)
        check = (struct UIWindow *)ui_check_box_construct(check);
    ui_check_box_set_bitmaps_fit((struct UICheckBox *)check, UI_SKIN "login_interface\\checkbox_on.bmp",
                             UI_SKIN "login_interface\\checkbox_off.bmp");
    ui_window_create(check, check->width, check->height);
    ui_window_move_base(check, 0x38, 0x55);
    ui_check_box_set_check(check, self->save_id);
    ui_window_add_child(wnd, check);

    combo = mem_alloc(sizeof(struct UIComboBoxDef2Wnd));
    if (combo)
        combo = ui_combo_box_def2_wnd_construct(combo);
    self->account_combo = combo;
    ((struct UIComboBoxDef2Wnd *)combo)->login_skin = 1;
    ((struct UIComboBoxDef2Wnd *)self->account_combo)->style = 1;
    ((struct UIComboBoxDef2Wnd *)self->account_combo)->def.list_height = 0x12;
    ((struct UIComboBoxDef2Wnd *)self->account_combo)->label_y = 3;
    ui_window_create(self->account_combo, 0xa0, 0x16);
    ui_window_move(self->account_combo, 0xd, 0xa);
    ui_window_add_child(wnd, self->account_combo);
    ui_combo_add(self->account_combo, msg_str(MSG_ACCOUNT_CLASS_0));
    ui_combo_add(self->account_combo, msg_str(MSG_ACCOUNT_CLASS_1));
    ui_combo_box_def_wnd_clear_list(self->account_combo);
    ui_combo_add(self->account_combo, msg_str(MSG_ACCOUNT_CLASS_0));
    ui_combo_box_def_wnd_select(self->account_combo, 0);
    ui_login_wnd_send_msg(wnd, self->account_combo, LOGIN_MSG_COMBO_CHANGED, 0, 0, 0, 0);

    UI_EDIT_MAX_CHARS(self->id_edit) = LOGIN_MAX_CHARS;
    UI_EDIT_MAX_CHARS(self->password_edit) = LOGIN_MAX_CHARS;
    id = ui_edit_ctrl_get_text(self->id_edit);
    if (strcmp(id, "") != 0 && strcmp(ui_edit_ctrl_get_text(self->id_edit), "ID") != 0)
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->password_edit);
    else
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->id_edit);
}

void ui_login_wnd_on_destroy(struct UIWindow *self)
{
}

void ui_login_wnd_on_process(struct UIWindow *window)
{
    struct UILoginWnd *self = (struct UILoginWnd *)window;
    struct UIWindow *focus = ui_window_mgr_get_focus_edit(&g_windowMgr);

    if (focus == self->id_edit) {
        ui_edit_ctrl_set_text_colour(self->id_edit, 0, 0, 0, -1);
        if (self->id_placeholder == 1 && strcmp("ID", ui_edit_ctrl_get_text(self->id_edit)) == 0)
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->id_edit, "");
        else
            self->id_placeholder = 0;
    } else {
        if (strcmp("", ui_edit_ctrl_get_text(self->id_edit)) == 0) {
            ui_edit_ctrl_set_text_colour(self->id_edit, LOGIN_PLACEHOLDER_GREY, LOGIN_PLACEHOLDER_GREY,
                                     LOGIN_PLACEHOLDER_GREY, -1);
            self->id_placeholder = 1;
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->id_edit, "ID");
        }
        if (self->id_placeholder == 0)
            ui_edit_ctrl_set_text_colour(self->id_edit, 0, 0, 0, -1);
    }

    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == self->password_edit) {
        HIMC imc = ImmGetContext(g_hMainWnd);
        DWORD conversion, sentence;

        ImmGetConversionStatus(imc, &conversion, &sentence);
        ImmSetConversionStatus(imc, 0, sentence);
        ImmReleaseContext(g_hMainWnd, imc);
        ui_edit_ctrl_set_text_colour(self->password_edit, 0, 0, 0, -1);
        if (self->password_placeholder == 1
            && strcmp("PASSWORD", ui_edit_ctrl_get_text(self->password_edit)) == 0) {
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->password_edit, "");
            UI_EDIT_MASK_CHAR(self->password_edit) = 0;
            return;
        }
        self->password_placeholder = 0;
    } else {
        if (strcmp("", ui_edit_ctrl_get_text(self->password_edit)) == 0) {
            self->password_placeholder = 1;
            ui_edit_ctrl_set_text_colour(self->password_edit, LOGIN_PLACEHOLDER_GREY, LOGIN_PLACEHOLDER_GREY,
                                     LOGIN_PLACEHOLDER_GREY, -1);
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->password_edit, "PASSWORD");
            UI_EDIT_MASK_CHAR(self->password_edit) = 0;
        }
        if (self->password_placeholder != 0)
            return;
        ui_edit_ctrl_set_text_colour(self->password_edit, 0, 0, 0, -1);
    }
    UI_EDIT_MASK_CHAR(self->password_edit) = '*';
}

void ui_login_wnd_on_draw(struct UIWindow *window)
{
    struct UILoginWnd *self = (struct UILoginWnd *)window;
    ui_window_clear_canvas(&self->frame.base, 0xffff00ff);
}

void ui_login_wnd_on_mouse_move(struct UIWindow *self, int arg1_unused, int arg2_unused)
{
}

void ui_login_wnd_store_info(struct UIWindow *window)
{
    struct UILoginWnd *self = (struct UILoginWnd *)window;
    ui_login_wnd_store_saved_id(self);
}

#define LOGIN_CMD_CANCEL 0xb1
#define LOGIN_ID_SAVE_ID 0xcd
#define LOGIN_ID_NOTICES 0xdd
#define LOGIN_ID_PORTAL_0 0x15f
#define LOGIN_ID_PORTAL_2 0x160
#define LOGIN_REQ_TO_SERVICE_SELECT 0x2722
#define LOGIN_REQ_PASSWORD 0x2717
#define LOGIN_REQ_USER_ID 0x2718
#define LOGIN_REQ_ACCOUNT_CLASS 0x2736
#define LOGIN_REQ_SET_SUBMODE 0x2735
#define LOGIN_SUBMODE_OTP 0x26
#define LOGIN_REQ_QUIT 2
#define LOGIN_ID_MIN_CHARS 4
#define LOGIN_PASSWORD_MIN_CHARS 6
#define MSG_LOGIN_ID_TOO_SHORT 0xbd5
#define MSG_LOGIN_PASSWORD_TOO_SHORT 0xbd6
#define MSG_LOGIN_QUIT_ASK 0x11

static void login_leave(struct UILoginWnd *self)
{
    mode_send_message(client_current_mode(), LOGIN_REQ_QUIT, 0, 0, 0, 0);
    ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
}

static void login_start(struct UILoginWnd *self)
{
    struct String text;

    sound_play_wave("버튼소리.wav"  , 0.0f, 0.0f, 0.0f, 0xfa,
                    0x28, 1.0f);
    if (ui_edit_ctrl_get_text_size(self->password_edit) >= LOGIN_PASSWORD_MIN_CHARS
        && ui_edit_ctrl_get_text_size(self->id_edit) >= LOGIN_ID_MIN_CHARS) {
        mode_send_message(client_current_mode(), LOGIN_REQ_SET_SUBMODE, LOGIN_SUBMODE_OTP, 0, 0, 0);
        mode_send_message(client_current_mode(), LOGIN_REQ_PASSWORD, (int)ui_edit_ctrl_get_text(self->password_edit), 0, 0, 0);
        mode_send_message(client_current_mode(), LOGIN_REQ_USER_ID, (int)ui_edit_ctrl_get_text(self->id_edit), 0, 0, 0);
        mode_send_message(client_current_mode(), LOGIN_REQ_ACCOUNT_CLASS, (int)self->account_class, 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, 3);
        ui_login_wnd_store_saved_id(self);
        return;
    }
    string_init(&text);
    if (ui_edit_ctrl_get_text_size(self->id_edit) < LOGIN_ID_MIN_CHARS)
        string_format(&text, msg_str(MSG_LOGIN_ID_TOO_SHORT), LOGIN_ID_MIN_CHARS);
    else
        string_format(&text, msg_str(MSG_LOGIN_PASSWORD_TOO_SHORT), LOGIN_PASSWORD_MIN_CHARS);
    ui_message_box(string_cstr(&text));
    string_free(&text);
}

static void login_show_notices(struct UILoginWnd *self)
{
    struct UIWindow *list;
    unsigned count, i;
    int x = 0x3c, y = 0x60;

    if (g_windowMgr.combo_box_wnd)
        ui_window_mgr_post_quit1(&g_windowMgr, UI_WND_COMBO_BOX);
    list = ui_window_open(UI_WND_COMBO_BOX);
    self->notice_list_open = 1;
    count = VEC_COUNT(&self->notice_lines, struct String);
    for (i = 0; i < VEC_COUNT(&self->notice_lines, struct String); i++)
        ui_window_send_msg(list, NULL, COMBO_MSG_ADD_LINE,
                 (int)string_cstr((struct String *)self->notice_lines.first + i), 0, 0, 0);
    ui_window_send_msg(list, NULL, COMBO_MSG_SELECT, 0, 0, 0, 0);
    ui_window_resize(list, 0x4a, (int)(count * 16) < 0x30 ? (int)(count * 16) : 0x30);
    ui_window_send_msg(list, NULL, COMBO_MSG_OPENER_ID, 3, 0, 0, 0);
    ui_window_to_screen(&self->frame.base, &x, &y);
    ui_window_move(list, x + 1, y);
}

static void login_close(struct UILoginWnd *self)
{
    ui_login_wnd_store_saved_id(self);
    if (ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_LOGIN_QUIT_ASK), UI_MSGBOX_QUESTION, 1, 0, 0,
                               UI_MSGBOX_TITLE, 0x118, 0x78, 0) == UI_MSGBOX_ANSWER_YES)
        login_leave(self);
}

int ui_login_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                  int val3, int val4)
{
    struct UILoginWnd *self = (struct UILoginWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case LOGIN_CMD_CANCEL:
            mode_send_message(client_current_mode(), LOGIN_REQ_TO_SERVICE_SELECT, 0, 0, 0, 0);
            break;
        case LOGIN_ID_START:
            login_start(self);
            break;
        case LOGIN_ID_SAVE_ID:
            self->save_id = val2;
            ui_login_wnd_store_saved_id(self);
            break;
        case LOGIN_ID_CLOSE:
            login_close(self);
            break;
        case LOGIN_ID_NOTICES:
            login_show_notices(self);
            break;
        case LOGIN_ID_PORTAL_0:
            g_session1.channelCopId = 0;
            ui_window_invalidate_base(&self->frame.base);
            break;
        case LOGIN_ID_PORTAL_2:
            g_session1.channelCopId = 2;
            ui_window_invalidate_base(&self->frame.base);
            break;
        }
        return 0;
    case LOGIN_MSG_COMBO_CHANGED:
        if (sender != self->account_combo)
            return 0;
        switch (val1) {
        case 0:
            self->account_class = "G000";
            break;
        case 1:
            self->account_class = "G007";
            break;
        default:
            ui_window_mgr_message_box(&g_windowMgr, "unknown account class found!", UI_MSGBOX_OK, 0, 0, 0,
                                   UI_MSGBOX_TITLE, 0x118, 0x78, 0);
            login_leave(self);
            break;
        }
        return 0;
    case 0x58:
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
