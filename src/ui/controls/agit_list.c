#include "core/thread_text.h"
#include "ui/controls/listbox.h"
#include "render/rp_pool.h"
#include "contents/client_lua_calls.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/controls/controls.h"
#include "ui/skin.h"
#include <string.h>
#include "ui/social/guild_manage.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_jobs.h"

struct UIListBox;
struct UIWindow;
const struct UIListBoxOps ui_list_box_agit_menu_ops = {
    .base = {
        .destroy = ui_list_box_agit_menu_destroy,
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
        .on_draw = ui_list_box_agit_menu_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_list_box_agit_menu_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_list_box_agit_menu_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_list_box_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_list_box_agit_menu_send_msg,
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

const struct UIListBoxOps ui_list_box_agit_ops = {
    .base = {
        .destroy = ui_list_box_agit_destroy,
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
        .on_l_btn_down = ui_list_box_agit_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_list_box_on_l_btn_dbl_clk,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_list_box_agit_on_r_btn_down,
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

#define AGIT_MSG_CHOSEN 0x30
#define AGIT_MSG_ROW_MENU 0xa2
#define AGIT_MSG_PRESSED 0xa3

#define MENU_LINES 2

void ui_list_box_agit_destruct(struct UIListBox *self)
{
    self->base.ops = &ui_list_box_agit_ops.base;
    ui_list_box_destruct(self);
}

void ui_list_box_agit_destroy(struct UIWindow *window)
{
    struct UIListBox *self = (struct UIListBox *)window;
    ui_list_box_agit_destruct(self);
    mem_free(self);
}

void ui_list_box_agit_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIListBox *self = (struct UIListBox *)window;
    if (!self->base.parent)
        return;
    ui_list_box_on_l_btn_down((struct UIWindow *)(self), x, y);
    ui_window_send_msg(self->base.parent, NULL, AGIT_MSG_PRESSED, 0, 0, 0, 0);
}

void ui_list_box_agit_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIListBox *self = (struct UIListBox *)window;
    struct UIWindow *parent = self->base.parent;

    if (!parent)
        return;
    ui_list_box_on_l_btn_down((struct UIWindow *)(self), x, y);
    if (self->selected < 0)
        ui_window_send_msg(parent, NULL, AGIT_MSG_PRESSED, 0, 0, 0, 0);
    else
        ui_window_send_msg(parent, NULL, AGIT_MSG_ROW_MENU, self->base.x - 3 + self->base.width,
                 self->base.y - y % self->line_height + y + 2, 0, 0);
}

void ui_list_box_agit_menu_destruct(struct UIListBox *self)
{
    self->base.ops = &ui_list_box_agit_menu_ops.base;
    ui_list_box_destruct(self);
}

void ui_list_box_agit_menu_destroy(struct UIWindow *window)
{
    struct UIListBox *self = (struct UIListBox *)window;
    ui_list_box_agit_menu_destruct(self);
    mem_free(self);
}

void ui_list_box_agit_menu_on_draw(struct UIWindow *window)
{
    struct UIListBox *self = (struct UIListBox *)window;
    struct UIWindow *wnd = &self->base;
    struct BitmapRes *corner;
    COLORREF selection;
    int line;

    ui_window_clear_canvas(wnd, ui_skin_colour(6, 6));
    ui_window_fill_rect(wnd, 1, 1, wnd->width - 2, wnd->height - 2, ui_skin_colour(2, 2));
    ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_lu.bmp"), 0);
    corner = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_ld.bmp");
    ui_window_draw_bitmap(wnd, 0, wnd->height - corner->height, corner, 0);
    corner = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_ru.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - corner->width, 0, corner, 0);
    corner = ui_skin_bitmap(UI_SKIN "basic_interface\\sysboxs_rd.bmp");
    ui_window_draw_bitmap(wnd, wnd->width - corner->width, wnd->height - corner->height, corner, 0);
    selection = ui_skin_colour(6, 2);
    for (line = 0; line < MENU_LINES; line++) {
        if (line == self->selected)
            ui_window_fill_rect(wnd, 3, self->line_height * line + 1, wnd->width - 6, self->line_height - 2,
                              selection);
        ui_window_text_out(wnd, 3, self->line_height * line + 1,
                         string_cstr(&((struct String *)self->items.first)[line]), 0, 0, 12, 0, 0, 0);
    }
}

void ui_list_box_agit_menu_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIListBox *self = (struct UIListBox *)window;
    ui_list_box_on_l_btn_down((struct UIWindow *)(self), x, y);
    ui_window_send_msg(&self->base, NULL, AGIT_MSG_CHOSEN, self->selected, 0, 0, 0);
    if (self->base.parent)
        ui_window_send_msg(self->base.parent, NULL, AGIT_MSG_PRESSED, 0, 0, 0, 0);
}

void ui_list_box_agit_menu_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIListBox *self = (struct UIListBox *)window;
    int old = self->selected;
    int line;

    if (x < 0 || x >= self->base.width || y < 0 || y >= self->base.height) {
        line = -1;
    } else {
        line = y / self->line_height;
        if (line < 0)
            line = 0;
        if (line > MENU_LINES - 1)
            line = MENU_LINES - 1;
    }
    self->selected = line;
    if (old != line)
        ui_window_invalidate_base(&self->base);
}

#define MSG_AGIT_IN_SIEGE 0xe31
#define MSG_AGIT_BOX_TEXT 0xe2f

int ui_list_box_agit_menu_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                         int val3, int val4)
{
    struct UIListBox *self = (struct UIListBox *)window;
    int agit = ((struct UIListBoxAgitMenu *)self)->agit_index;

    if (msg != AGIT_MSG_CHOSEN)
        return ui_list_box_send_msg((struct UIWindow *)(self), sender, msg, val1, val2, val3, val4);
    if (val1 == 1) {
        struct Mode *mode = game_current_base_mode();

        mode_send_message(mode, MODE_REQ_AGIT_INVESTMENT, (WORD)guild_info_castle_at(&g_guildInfo, agit), 0, 0, 0);
    } else if (val1 == 0) {
        const char *name = NULL;
        int cost0 = 0, cost1 = 0;
        WORD id;

        if (session_is_siege_mode(&g_session1) == 1) {
            ui_chat_line(msg_str(MSG_AGIT_IN_SIEGE), 0xff);
            return 0;
        }
        id = (WORD)guild_info_castle_at(&g_guildInfo, agit);
        client_lua_call(g_session1.luaState, "get_agit_name", "d>s", (int)id, &name);
        if (!strnlen(name, 0x100))
            return 0;
        client_lua_call(g_session1.luaState, "get_agit_costzeny", "db>d", (int)id, 0, &cost0);
        client_lua_call(g_session1.luaState, "get_agit_costzeny", "db>d", (int)id, 1, &cost1);
        agit_message_box_open(id, thread_text_format(msg_str(MSG_AGIT_BOX_TEXT), name, cost0, cost1));
    }
    return 0;
}
