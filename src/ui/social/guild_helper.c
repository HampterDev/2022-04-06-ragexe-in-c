#include "render/rp_pool.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "core/markup_tags.h"
#include <string.h>
#include "ui/social/social.h"
#include <mmsystem.h>
#include "core/msg_str.h"
#include "diag/perf.h"

#define GUILD_HELPER_WND_ID 0xd4
#define CREATE_GUILD_WND_ID 0xd5
#define DISBAND_GUILD_WND_ID 0xd7

#define MSG_GUILD_HELPER_TITLE 0x81a
#define MSG_GUILD_HELPER_TEXT 0x81b
#define MSG_CREATE_GUILD_TITLE 0x81c
#define MSG_CREATE_GUILD_TEXT 0x81d
#define MSG_CREATE_GUILD_NO_NAME 0x820
#define MSG_DISBAND_GUILD_TITLE 0x828
#define MSG_DISBAND_GUILD_TEXT 0x829
#define MSG_CREATE_GUILD_ITEM_LINK 0xafc

#define GUILD_HELPER_ID_CREATE 0x1b8

#define REQ_CREATE_GUILD 0x70
#define REQ_DISBAND_GUILD 0x6a

#define UI_MSG_REBUILD 0x17

struct UIGuildHelperWnd {
    struct UIFrameWnd frame;
    struct UIBitmapButton *button;
    struct WindowInfo *info;
};
SIZE_ASSERT(UIGuildHelperWnd, 0x9c);
struct Vec;
void ui_guild_helper_wnd_destroy(struct UIWindow *window);
void ui_guild_helper_wnd_store_info(struct UIWindow *self);
void ui_guild_helper_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_guild_helper_wnd_on_draw(struct UIWindow *self);
void ui_guild_helper_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_guild_helper_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_guild_helper_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_guild_helper_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_guild_helper_wnd_ops = {
    .destroy = ui_guild_helper_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_guild_helper_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_guild_helper_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_guild_helper_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_guild_helper_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_guild_helper_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_guild_helper_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_guild_helper_wnd_send_msg,
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

struct UIGuildHelperWnd *ui_guild_helper_wnd_construct(struct UIGuildHelperWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_guild_helper_wnd_ops;
    return self;
}

struct UIWindow *ui_guild_helper_wnd_new(void)
{
    return (struct UIWindow *)ui_guild_helper_wnd_construct(mem_alloc(sizeof(struct UIGuildHelperWnd)));
}

void ui_guild_helper_wnd_destruct(struct UIGuildHelperWnd *self)
{
    self->frame.base.ops = &ui_guild_helper_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_guild_helper_wnd_destroy(struct UIWindow *window)
{
    struct UIGuildHelperWnd *self = (struct UIGuildHelperWnd *)window;
    ui_guild_helper_wnd_destruct(self);
    mem_free(self);
}

#define GUILD_HELPER UI_SKIN "guild_helper\\"

void ui_guild_helper_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIGuildHelperWnd *self = (struct UIGuildHelperWnd *)window;
    static const char *const bitmaps[2][3] = {
        { GUILD_HELPER "create_guild.bmp", GUILD_HELPER "create_guild_a.bmp", GUILD_HELPER "create_guild_b.bmp" },
        { GUILD_HELPER "btn_ok.bmp", GUILD_HELPER "btn_ok_a.bmp", GUILD_HELPER "btn_ok_b.bmp" },
    };
    static const int ids[2] = { GUILD_HELPER_ID_CREATE, UI_ID_OK };
    int i;

    for (i = 0; i < 2; i++)
        self->button = ui_add_bitmap_button(&self->frame.base, bitmaps[i][0], bitmaps[i][1], bitmaps[i][2],
                                            i == 0 ? 8 : width - 0x34, height - 0x18, ids[i]);
}

void ui_guild_helper_wnd_on_draw(struct UIWindow *window)
{
    struct UIGuildHelperWnd *self = (struct UIGuildHelperWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_GUILD_HELPER_TITLE), 0);
    ui_window_draw_body(wnd, 0x11, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    ui_window_text_out(wnd, 0xf, 0x20, msg_str(MSG_GUILD_HELPER_TEXT), 0, 0, 0xc, 0, 0, 0);
}

static void place_attached(struct UIGuildHelperWnd *self)
{
    int x = 0, y = 0;

    ui_window_to_screen(&self->frame.base, &x, &y);
    ui_window_move(g_windowMgr.create_guild_wnd, x + 2 + self->frame.base.width, y);
}

void ui_guild_helper_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIGuildHelperWnd *self = (struct UIGuildHelperWnd *)window;
    if (y >= 0x11)
        return;
    ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
    if (g_windowMgr.create_guild_wnd) {
        g_windowMgr.create_guild_wnd->alpha_target = 0x80;
        g_windowMgr.create_guild_wnd->alpha_time = timeGetTime();
    }
}

void ui_guild_helper_wnd_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIGuildHelperWnd *self = (struct UIGuildHelperWnd *)window;
    ui_frame_wnd_on_mouse_move((struct UIWindow *)(&self->frame), x, y);
    if (g_windowMgr.create_guild_wnd)
        place_attached(self);
}

void ui_guild_helper_wnd_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIGuildHelperWnd *self = (struct UIGuildHelperWnd *)window;
    ui_frame_wnd_on_l_btn_up((struct UIWindow *)(&self->frame), x, y);
    if (g_windowMgr.create_guild_wnd) {
        g_windowMgr.create_guild_wnd->alpha_target = 0xff;
        g_windowMgr.create_guild_wnd->alpha_time = timeGetTime();
    }
}

void ui_guild_helper_wnd_store_info(struct UIWindow *window)
{
    struct UIGuildHelperWnd *self = (struct UIGuildHelperWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    if (!self->info)
        return;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
}

static void guild_helper_info_reset(struct WindowInfo *info)
{
    info->x = 0xe6;
    info->y = 0;
    info->width = 0xc8;
    info->height = 0x96;
}

int ui_guild_helper_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                        int val2, int val3, int val4)
{
    struct UIGuildHelperWnd *self = (struct UIGuildHelperWnd *)window;
    if (msg == UI_MSG_COMMAND) {
        switch (val1) {
        case UI_ID_OK:
            ui_window_mgr_post_quit1(&g_windowMgr, CREATE_GUILD_WND_ID);
            ui_window_mgr_post_quit1(&g_windowMgr, GUILD_HELPER_WND_ID);
            break;
        case GUILD_HELPER_ID_CREATE:
            if (!ui_close_window(CREATE_GUILD_WND_ID)) {
                ui_make_window(CREATE_GUILD_WND_ID);
                if (g_windowMgr.create_guild_wnd)
                    place_attached(self);
            }
            break;
        }
        return 0;
    }
    if (msg == UI_MSG_SET_INFO) {
        self->info = (struct WindowInfo *)val1;
        if (!ui_info_on_screen(self->info))
            guild_helper_info_reset(self->info);
        ui_window_send_msg(&self->frame.base, NULL, UI_MSG_REBUILD, 0, 0, 0, 0);
        ui_frame_wnd_move(&self->frame.base, self->info->x, self->info->y);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

struct UICreateGuildWnd {
    struct UIGuildHelperWnd helper;
    struct UIWindow *name;
    int disband;
};
OFFSET_ASSERT(UICreateGuildWnd, name, 0x9c);
SIZE_ASSERT(UICreateGuildWnd, 0xa4);
void ui_create_guild_wnd_destroy(struct UIWindow *window);
void ui_create_guild_wnd_store_info(struct UIWindow *self);
void ui_create_guild_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_create_guild_wnd_on_draw(struct UIWindow *self);
void ui_create_guild_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_create_guild_wnd_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_create_guild_wnd_on_l_btn_up(struct UIWindow *self, int x, int y);
int ui_create_guild_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_create_guild_wnd_ops = {
    .destroy = ui_create_guild_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_create_guild_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_create_guild_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_create_guild_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_create_guild_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_create_guild_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_create_guild_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_create_guild_wnd_send_msg,
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

struct UICreateGuildWnd *ui_create_guild_wnd_construct(struct UICreateGuildWnd *self, int disband)
{
    ui_guild_helper_wnd_construct(&self->helper);
    self->helper.frame.base.ops = &ui_create_guild_wnd_ops;
    self->disband = disband;
    return self;
}

struct UIWindow *ui_create_guild_wnd_new(int arg)
{
    return (struct UIWindow *)ui_create_guild_wnd_construct(mem_alloc(sizeof(struct UICreateGuildWnd)), arg);
}

static void ui_create_guild_wnd_destruct(struct UICreateGuildWnd *self)
{
    self->helper.frame.base.ops = &ui_create_guild_wnd_ops;
    ui_guild_helper_wnd_destruct(&self->helper);
}

void ui_create_guild_wnd_destroy(struct UIWindow *window)
{
    struct UICreateGuildWnd *self = (struct UICreateGuildWnd *)window;
    ui_create_guild_wnd_destruct(self);
    mem_free(self);
}

void ui_create_guild_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UICreateGuildWnd *self = (struct UICreateGuildWnd *)window;
    struct UIWindow *wnd = &self->helper.frame.base;
    struct UIWindow *name = mem_alloc(sizeof(struct UIEditCtrl));

    if (name)
        name = ui_edit_ctrl_construct(name);
    self->name = name;
    ui_window_create(name, 0x7c, 0x14);
    ui_window_move(name, 0xd, 0x2a);
    UI_EDIT_MAX_CHARS(name) = 0x17;
    ui_edit_ctrl_set_back_colour(name, 0xe6, 0xe6, 0xe6);
    ui_window_add_child(wnd, name);

    ui_add_bitmap_button(wnd, UI_SKIN "btn_ok.bmp", UI_SKIN "btn_ok_a.bmp", UI_SKIN "btn_ok_b.bmp",
                         wnd->width - 0x5c, wnd->height - 0x18, UI_ID_OK);
    ui_add_bitmap_button(wnd, UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp", UI_SKIN "btn_cancel_b.bmp",
                         wnd->width - 0x2e, wnd->height - 0x18, UI_ID_CANCEL);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->name);
    UI_FRAME_DEFAULT_ID(&self->helper.frame) = UI_ID_OK;
}

void ui_create_guild_wnd_on_draw(struct UIWindow *window)
{
    struct UICreateGuildWnd *self = (struct UICreateGuildWnd *)window;
    struct UIWindow *wnd = &self->helper.frame.base;
    int text = self->disband ? MSG_DISBAND_GUILD_TEXT : MSG_CREATE_GUILD_TEXT;
    int title = self->disband ? MSG_DISBAND_GUILD_TITLE : MSG_CREATE_GUILD_TITLE;

    ui_window_draw_body(wnd, 0x11, 0, "");
    ui_window_draw_button_bar(wnd, 2, 0, 1);
    ui_window_draw_title_bar(wnd, 1, msg_str(title), 0);
    ui_window_text_out(wnd, 0x10, 0x17, msg_str(text), 0, 0, 0xc, 0, 0, 0);
}

void ui_create_guild_wnd_on_l_btn_down(struct UIWindow *self, int x, int y)
{
}

void ui_create_guild_wnd_on_mouse_move(struct UIWindow *self, int x, int y)
{
}

void ui_create_guild_wnd_on_l_btn_up(struct UIWindow *self, int x, int y)
{
}

void ui_create_guild_wnd_store_info(struct UIWindow *self)
{
}

int ui_create_guild_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                        int val2, int val3, int val4)
{
    struct UICreateGuildWnd *self = (struct UICreateGuildWnd *)window;
    struct Mode *game;
    struct String name;
    const char *typed;

    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->helper.frame), sender, msg, val1, val2, val3, val4);
    if (val1 == UI_ID_CANCEL) {
        ui_window_mgr_post_quit1(&g_windowMgr, self->disband ? DISBAND_GUILD_WND_ID : CREATE_GUILD_WND_ID);
        return 0;
    }
    if (val1 != UI_ID_OK)
        return 0;

    game = game_current_base_mode();
    string_init(&name);
    typed = ui_edit_ctrl_get_text(self->name);
    string_assign_cstr(&name, typed);
    ui_edit_ctrl_select_all(self->name);
    if (name.size == 0) {
        ui_chat_line(msg_str(MSG_CREATE_GUILD_NO_NAME), RGB(0xff, 0, 0));
    } else if (strstr(ui_edit_ctrl_get_text(self->name), string_cstr(&text_tag_ITEML)) ||
               strstr(ui_edit_ctrl_get_text(self->name), string_cstr(&text_tag_ITEML_END))) {
        ui_chat_line(msg_str(MSG_CREATE_GUILD_ITEM_LINK), RGB(0xff, 0, 0));
    } else if (!self->disband) {
        mode_send_message(game, REQ_CREATE_GUILD, (int)string_cstr(&name), 0, 0, 0);
    } else {
        mode_send_message(game, REQ_DISBAND_GUILD, (int)string_cstr(&name), 0, 0, 0);
        ui_window_mgr_post_quit1(&g_windowMgr, DISBAND_GUILD_WND_ID);
    }
    string_free(&name);
    return 0;
}
