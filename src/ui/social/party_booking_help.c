#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "ui/social/social.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define PARTY_BOOKING_HELP_WND_ID 0xad
#define MSG_PARTY_BOOKING_HELP_TITLE 0x6b4

struct UIPartyBookingHelpWnd {
    struct UIFrameWnd frame;
    struct UIWindow *text;
};
void ui_party_booking_help_wnd_destroy(struct UIWindow *window);
void ui_party_booking_help_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_party_booking_help_wnd_on_draw(struct UIWindow *self);
int ui_party_booking_help_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_party_booking_help_wnd_ops = {
    .destroy = ui_party_booking_help_wnd_destroy,
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
    .on_create = ui_party_booking_help_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_party_booking_help_wnd_on_draw,
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
    .send_msg = ui_party_booking_help_wnd_send_msg,
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

struct UIPartyBookingHelpWnd *ui_party_booking_help_wnd_construct(struct UIPartyBookingHelpWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_party_booking_help_wnd_ops;
    return self;
}

struct UIWindow *ui_party_booking_help_wnd_new(void)
{
    return (struct UIWindow *)ui_party_booking_help_wnd_construct(mem_alloc(sizeof(struct UIPartyBookingHelpWnd)));
}

static void ui_party_booking_help_wnd_destruct(struct UIPartyBookingHelpWnd *self)
{
    self->frame.base.ops = &ui_party_booking_help_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_party_booking_help_wnd_destroy(struct UIWindow *window)
{
    struct UIPartyBookingHelpWnd *self = (struct UIPartyBookingHelpWnd *)window;
    ui_party_booking_help_wnd_destruct(self);
    mem_free(self);
}

void ui_party_booking_help_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIPartyBookingHelpWnd *self = (struct UIPartyBookingHelpWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *text;
    struct String help;
    const char *found = NULL;

    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "", 0);

    text = mem_alloc(sizeof(struct UITextViewer));
    if (text)
        text = ui_text_viewer_construct(text);
    self->text = text;
    ui_list_box_create_window((struct UIListBox *)text, 0xa, 0x19, width - 0x14, height - 0x23, 1);
    ui_window_add_child(wnd, text);

    string_init(&help);

    if (client_lua_call(g_session1.luaState, "GetPartyBookingHelp", "d>s", -1, &found)) {
        string_assign_cstr(&help, found);
        if (help.size) {
            char *line = mem_alloc(help.size + 4);

            sprintf(line, "%s%s", language_charset_tag(g_language, 1), string_cstr(&help));
            ui_window_send_msg(self->text, NULL, UI_MSG_ADD_TEXT, (int)line, 0, 1, 1);
        }
    }
    string_free(&help);
}

void ui_party_booking_help_wnd_on_draw(struct UIWindow *window)
{
    struct UIPartyBookingHelpWnd *self = (struct UIPartyBookingHelpWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_PARTY_BOOKING_HELP_TITLE), 0);
    ui_window_draw_body(wnd, 0x11, 0, "");
}

int ui_party_booking_help_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             int val1, int val2, int val3, int val4)
{
    struct UIPartyBookingHelpWnd *self = (struct UIPartyBookingHelpWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    if (val1 == UI_ID_CLOSE)
        ui_window_mgr_post_quit1(&g_windowMgr, PARTY_BOOKING_HELP_WND_ID);
    return 0;
}
