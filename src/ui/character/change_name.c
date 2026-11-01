#include "ui/controls/edit.h"
#include "ui/window_access.h"
#include "ui/chat/markup_links.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "contents/connection_api.h"
#include <stdio.h>
#include <string.h>
#include "ui/character/character.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/skin.h"
#include "ui/draw.h"
#include "ui/ui_std.h"
#include "ui/character/char_info.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_party.h"
#include "ui/character/character_std.h"

#define UI_MSG_CHANGE_NAME_CHAR 0x22
#define MODE_MSG_GET_NEW_CHAR_NAME 0x271e

#define PACKET_CHANGE_NAME 0x8fc
#define CHAR_NAME_LEN 24

#define MSG_NAME_TOO_SHORT 0x15
#define MSG_NAME_BLANK_EDGE 0xc7
#define MSG_NAME_TOO_LONG 0x213
#define MSG_NAME_ASK 0x4bd
#define MSG_NAME_SERVICE_TEST 0x7e2
#define MSG_NAME_NAVI_LINK 0xafc
#define MSG_NAME_LINE_BREAK 0xd06

#pragma pack(push, 1)
struct PacketChangeName {
    WORD id;
    int char_id;
    char name[CHAR_NAME_LEN];
};
#pragma pack(pop)
SIZE_ASSERT(PacketChangeName, 0x1e);

struct UIChangeNameWnd {
    struct UIFrameWnd frame;
    struct UIBitmapButton *ok;
    struct UIBitmapButton *cancel;
    struct UIBitmapEditCtrl *edit;
    const struct CharInfo *char_info;
    struct CharInfo char_copy;
};
OFFSET_ASSERT(UIChangeNameWnd, edit, 0x9c);
OFFSET_ASSERT(UIChangeNameWnd, char_copy, 0xa4);
SIZE_ASSERT(UIChangeNameWnd, 0x154);

struct UIWindow;
void ui_change_name_wnd_destroy(struct UIWindow *window);
void ui_change_name_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_change_name_wnd_on_draw(struct UIWindow *self);
int ui_change_name_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_change_name_wnd_ops = {
    .destroy = ui_change_name_wnd_destroy,
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
    .on_create = ui_change_name_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_change_name_wnd_on_draw,
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
    .send_msg = ui_change_name_wnd_send_msg,
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

void ui_change_name_wnd_destruct(struct UIChangeNameWnd *self)
{
    self->frame.base.ops = &ui_change_name_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_change_name_wnd_destroy(struct UIWindow *window)
{
    struct UIChangeNameWnd *self = (struct UIChangeNameWnd *)window;
    ui_change_name_wnd_destruct(self);
    mem_free(self);
}

static struct UIBitmapButton *change_name_button(struct UIChangeNameWnd *self, const char *const names[3], int x,
                                                  int y, int id)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);
    struct String skin;
    int state;

    string_init(&skin);
    string_assign(&skin, UI_SKIN, sizeof UI_SKIN - 1);
    if (button)
        button = ui_bitmap_button_construct(button);
    for (state = 0; state < 3; state++) {
        struct String path;

        string_copy(&path, &skin);
        string_append_cstr(&path, names[state]);
        string_append_cstr(&path, ".bmp");
        ui_bitmap_button_set_bitmap(button, string_cstr(&path), state, 0);
        string_free(&path);
    }
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    ui_window_add_child(&self->frame.base, &button->button.base);
    string_free(&skin);
    return button;
}

void ui_change_name_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIChangeNameWnd *self = (struct UIChangeNameWnd *)window;
    static const char *const ok_names[3] = { "btn_ok", "btn_ok_a", "btn_ok_b" };
    static const char *const cancel_names[3] = { "btn_cancel", "btn_cancel_a", "btn_cancel_b" };
    struct UIWindow *wnd = &self->frame.base;
    struct UIBitmapEditCtrl *edit;

    self->ok = change_name_button(self, ok_names, wnd->width - 0x5d, wnd->height - 0x18, UI_ID_OK);
    self->cancel = change_name_button(self, cancel_names, wnd->width - 0x2e, wnd->height - 0x18, UI_ID_CANCEL);
    mode_send_message(client_current_mode(), MODE_MSG_GET_NEW_CHAR_NAME, 0, 0, 0, 0);

    edit = mem_alloc(sizeof *edit);
    if (edit) {
        ui_edit_ctrl_construct(&edit->edit.base);
        edit->edit.base.ops = &ui_bitmap_edit_ctrl_ops.base;
        string_init(&edit->bitmap);
    }
    self->edit = edit;
    ui_bitmap_edit_ctrl_set_bitmap(edit, UI_SKIN "login_interface\\name-edit.bmp");
    ui_window_create(&self->edit->edit.base, self->edit->edit.base.width, self->edit->edit.base.height);
    ui_window_move_base(&self->edit->edit.base, 0x3c, 0x2c);
    self->edit->edit.max_chars = CHAR_NAME_LEN;
    self->edit->edit.base.id = UI_ID_OK;
    ui_window_add_child(wnd, &self->edit->edit.base);
    ui_window_mgr_set_focus_edit(&g_windowMgr, &self->edit->edit.base);
    self->frame.default_id = UI_ID_OK;
}

void ui_change_name_wnd_on_draw(struct UIWindow *window)
{
    struct UIChangeNameWnd *self = (struct UIChangeNameWnd *)window;
    ui_window_draw_bitmap(&self->frame.base, 0, 0, ui_skin_bitmap(UI_SKIN "login_interface\\change_name.bmp"),
                        0);
}

static void change_name_refuse(struct UIChangeNameWnd *self, int msg_id)
{
    ui_message_box(msg_str(msg_id));
    ui_edit_ctrl_set_text_default(&self->edit->edit, "");
}

static int change_name_has_gm_tag(const char *name)
{
    if (name[0] == '[') {
        if (name[1] == 'g' && name[2] == 'm')
            return 1;
    } else if (name[1] == '[' && name[2] == 'g') {
        return name[3] == 'm';
    }
    return name[2] == '[' && name[3] == 'g' && name[4] == 'm';
}

static void change_name_submit(struct UIChangeNameWnd *self)
{
    struct UIWindow *edit = &self->edit->edit.base;
    struct String text, arg;
    char lower[CHAR_NAME_LEN] = {0};
    char question[0x12c];
    BYTE blank_edge;

    if (navi_links_has_link(text_view_cstr(ui_edit_ctrl_get_text(edit)))) {
        change_name_refuse(self, MSG_NAME_NAVI_LINK);
        return;
    }
    string_init(&text);
    string_assign_cstr(&text, ui_edit_ctrl_get_text(edit));
    if (string_find(&text, "\n", 0) != STRING_NPOS) {
        change_name_refuse(self, MSG_NAME_LINE_BREAK);
        goto done;
    }
    if (g_serviceType == 1 && !session_is_ascii_name(&g_session1, ui_edit_ctrl_get_text(edit))) {
        change_name_refuse(self, MSG_NAME_SERVICE_TEST);
        goto done;
    }
    if (ui_edit_ctrl_get_text_size(edit) < 4) {
        ui_message_box(msg_str(MSG_NAME_TOO_SHORT));
        goto done;
    }
    blank_edge = np_dect(ui_edit_ctrl_get_text(edit));
    if (blank_edge) {
        ui_message_box(msg_str(MSG_NAME_BLANK_EDGE));
        goto done;
    }
    if (ui_edit_ctrl_get_text_size(edit) >= CHAR_NAME_LEN) {
        change_name_refuse(self, MSG_NAME_TOO_LONG);
        goto done;
    }
    string_init(&arg);
    string_assign_cstr(&arg, ui_edit_ctrl_get_text(edit));
    if (!text_within_columns(self, arg, CHAR_NAME_LEN)) {
        change_name_refuse(self, MSG_NAME_TOO_LONG);
        goto done;
    }

    strcpy(lower, ui_edit_ctrl_get_text(edit));
    self->char_copy = *self->char_info;
    memcpy(self->char_copy.name, lower, CHAR_NAME_LEN);
    if (strstr(self->char_copy.name, "그라비티"))
        goto done;
    _strlwr(lower);
    if (strstr(lower, "gravity") || change_name_has_gm_tag(lower))
        goto done;

    sprintf(question, msg_str(MSG_NAME_ASK), self->char_copy.name);
    if (ui_window_mgr_message_box(&g_windowMgr, question, UI_MSGBOX_QUESTION, 0, 0, 0, "메시지", 0x118,
                               0x78, 0) == UI_MSGBOX_ANSWER_YES) {
        struct PacketChangeName packet;
        int len;

        packet.id = PACKET_CHANGE_NAME;
        packet.char_id = (int)self->char_info->gid;
        g_session1.renameCharId = packet.char_id;
        memcpy(packet.name, self->char_copy.name, CHAR_NAME_LEN);
        memcpy(g_session1.renameNewName, self->char_copy.name, CHAR_NAME_LEN);
        len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_CHANGE_NAME);
        rag_connection_send_packet(rag_connection_instance(), len, &packet);
        ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
    }
done:
    string_free(&text);
}

int ui_change_name_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UIChangeNameWnd *self = (struct UIChangeNameWnd *)window;
    switch (msg) {
    case UI_MSG_DEFAULT_BUTTON:
        ui_change_name_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, self->frame.default_id, 0, 0, 0);
        break;
    case UI_MSG_COMMAND:
        if (val1 == UI_ID_OK)
            change_name_submit(self);
        else if (val1 == UI_ID_CANCEL)
            ui_window_mgr_post_quit2(&g_windowMgr, &self->frame.base);
        break;
    case UI_MSG_CHANGE_NAME_CHAR:
        self->char_info = (const struct CharInfo *)val1;
        ui_edit_ctrl_set_text_default(&self->edit->edit, ((const struct CharInfo *)val1)->name);
        break;
    }
    return 0;
}

struct UIChangeNameWnd *ui_change_name_wnd_construct(struct UIChangeNameWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_change_name_wnd_ops;
    return self;
}

struct UIWindow *ui_change_name_wnd_new(void)
{
    return (struct UIWindow *)ui_change_name_wnd_construct(mem_alloc(sizeof(struct UIChangeNameWnd)));
}
