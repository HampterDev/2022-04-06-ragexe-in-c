#include "ui/controls/listbox.h"
#include "ui/social/guild_windows.h"
#include "contents/player_names.h"
#include "render/rp_pool.h"
#include "render/sprite_atlas.h"
#include "resources/palette.h"
#include "resources/sprite.h"
#include "contents/client_lua_calls.h"
#include "ui/social/guild_manage.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "std/tree.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "core/markup_tags.h"
#include <string.h>
#include <time.h>
#include "ui/social/social.h"
#include "ui/social/guild_wnd.h"
#include "ui/character/character.h"
#include "ui/character/sel_char_userver.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_names.h"

static void destroy_string(void *value) { string_free(value); }

const struct UIWindowOps ui_agit_message_box_ops = {
    .destroy = ui_agit_message_box_destroy,
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
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_agit_message_box_on_draw,
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
    .send_msg = ui_agit_message_box_send_msg,
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

void ui_agit_message_box_destruct(struct UIAgitMessageBox *self)
{
    self->frame.base.ops = &ui_agit_message_box_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_agit_message_box_destroy(struct UIWindow *window)
{
    struct UIAgitMessageBox *self = (struct UIAgitMessageBox *)window;
    ui_agit_message_box_destruct(self);
    mem_free(self);
}

const struct UIGuildWndOps ui_guild_info_manage_wnd_ops = {
    .base = {
        .destroy = ui_guild_info_manage_wnd_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 1,
        .refreshes_children = 0,
        .move = ui_frame_wnd_move,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_guild_wnd_store_info,
        .save_pos = ui_frame_wnd_save_pos,
        .move_delta = ui_frame_wnd_move_delta,
        .set_show = ui_window_set_show_base,
        .on_create = ui_guild_info_manage_wnd_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_guild_info_manage_wnd_on_process,
        .on_draw = ui_guild_info_manage_wnd_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_guild_info_manage_wnd_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_frame_wnd_on_mouse_move,
        .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
        .on_r_btn_down = ui_guild_info_manage_wnd_on_r_btn_down,
        .on_r_btn_up = ui_guild_info_manage_wnd_on_r_btn_up,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_frame_wnd_snap,
        .send_msg = ui_guild_info_manage_wnd_send_msg,
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
    },
    .tab = GUILD_TAB_INFO,
};

#define WND(self) (&(self)->base.frame.base)

#define GUILD_INFO_ID_EDIT 0x10e
#define GUILD_INFO_ID_CASTLES 0x226
#define GUILD_INFO_ID_CASTLE_MENU 0x227
#define MSG_GUILD_CASTLES 0x14d
#define MSG_CASTLE_MENU_FIRST 0xe2d
#define GUILD_BUTTON_HIDDEN -200

struct UIGuildInfoManageWnd *ui_guild_info_manage_wnd_construct(struct UIGuildInfoManageWnd *self)
{
    ui_guild_wnd_construct(&self->base);
    WND(self)->ops = &ui_guild_info_manage_wnd_ops.base;
    list_init(&self->emblem_files);
    self->menu.first = self->menu.last = self->menu.end = NULL;
    self->emblem_data = NULL;
    self->emblem_size = 0;
    self->guild_id = g_guildInfo.guild_id;
    return self;
}

void ui_guild_info_manage_wnd_free_emblem_data(struct UIGuildInfoManageWnd *self)
{
    if (self->emblem_data) {
        mem_free(self->emblem_data);
        self->emblem_data = NULL;
        self->emblem_size = 0;
    }
}

void ui_guild_info_manage_wnd_destruct(struct UIGuildInfoManageWnd *self)
{
    WND(self)->ops = &ui_guild_info_manage_wnd_ops.base;
    ui_guild_info_manage_wnd_free_emblem_data(self);
    ui_menu_entry_vec_free(&self->menu);
    list_free(&self->emblem_files, destroy_string);
    ui_guild_wnd_destruct(&self->base);
}

void ui_guild_info_manage_wnd_destroy(struct UIWindow *window)
{
    struct UIGuildInfoManageWnd *self = (struct UIGuildInfoManageWnd *)window;
    ui_guild_info_manage_wnd_destruct(self);
    mem_free(self);
}

void ui_guild_info_manage_wnd_fill_castles(struct UIGuildInfoManageWnd *self)
{
    int count, i;

    ui_list_box_clear(self->agit_list);
    count = guild_info_castle_count(&g_guildInfo);
    if (count == 0)
        return;
    for (i = 0; i < count; i++) {
        int id = guild_info_castle_at(&g_guildInfo, i);
        const char *name = NULL;

        client_lua_call(g_session1.luaState, "get_agit_name", "d>s", id, &name);
        if (strnlen(name, 0x100))
            ui_list_box_append_text(self->agit_list, name);
    }
}

#define BUTTON_WND(button) ((struct UIWindow *)(button))

static struct UIBitmapButton *new_button(void)
{
    struct UIBitmapButton *button = (struct UIBitmapButton *)mem_alloc(sizeof *button);

    return button ? ui_bitmap_button_construct(button) : NULL;
}

void ui_guild_info_manage_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIGuildInfoManageWnd *self = (struct UIGuildInfoManageWnd *)window;
    static const char *const edit_names[3] = {"btn_edit", "btn_edit_a", "btn_edit_b"};
    static const char *const disband_paths[3] = {
        UI_SKIN "btn_disBand.bmp", UI_SKIN "btn_disBand_a.bmp", UI_SKIN "btn_disBand_b.bmp"};
    struct String skin;
    struct UIBitmapButton *button;
    struct UIListBox *box;
    struct UIListBoxAgitMenu *menu;
    int i, label_width;

    ui_guild_wnd_init_tab_text(&self->base);
    self->ally_selected = -1;
    self->enemy_selected = -1;
    if (strcmp(string_cstr(&g_guildInfo.master_name), "") == 0)
        mode_send_message(client_current_mode(), MODE_REQ_GUILD_INFO, 0, 0, 0, 0);
    g_guildInfo.page_asked[GUILD_PAGE_EXPELLED] = 1;
    mode_send_message(client_current_mode(), MODE_REQ_GUILD_INFO, 4, 0, 0, 0);
    g_guildInfo.page_asked[GUILD_PAGE_INFO] = 1;
    mode_send_message(client_current_mode(), MODE_REQ_GUILD_INFO, 0, 0, 0, 0);
    g_guildInfo.page_asked[GUILD_PAGE_MEMBERS] = 1;
    mode_send_message(client_current_mode(), MODE_REQ_GUILD_INFO, 1, 0, 0, 0);

    string_init(&skin);
    string_assign(&skin, UI_SKIN, sizeof UI_SKIN - 1);
    self->emblem_button = new_button();
    for (i = 0; i < 3; i++) {
        struct String path;

        string_copy(&path, &skin);
        string_append_cstr(&path, edit_names[i]);
        string_append_cstr(&path, ".bmp");
        ui_bitmap_button_set_bitmap(self->emblem_button, string_cstr(&path), i, 0);
        string_free(&path);
    }
    button = self->emblem_button;
    ui_window_create(BUTTON_WND(button), button->bitmap_width, button->bitmap_height);
    if (g_guildInfo.is_master)
        ui_window_move_base(&button->button.base, WND(self)->width - BUTTON_WND(button)->width - 8, 0x45);
    else
        ui_window_move_base(&button->button.base, GUILD_BUTTON_HIDDEN, GUILD_BUTTON_HIDDEN);
    BUTTON_WND(self->emblem_button)->id = GUILD_INFO_ID_EDIT;
    ui_window_add_child(WND(self), BUTTON_WND(self->emblem_button));

    self->disband_button = new_button();
    for (i = 0; i < 3; i++)
        ui_bitmap_button_set_bitmap(self->disband_button, disband_paths[i], i, 0);
    button = self->disband_button;
    ui_window_create(BUTTON_WND(button), button->bitmap_width, button->bitmap_height);
    if (g_guildInfo.is_master)
        ui_window_move_base(&button->button.base, WND(self)->width - BUTTON_WND(button)->width - 0xf,
                 WND(self)->height - BUTTON_WND(button)->height - 5);
    else
        ui_window_move_base(&button->button.base, GUILD_BUTTON_HIDDEN, GUILD_BUTTON_HIDDEN);
    BUTTON_WND(self->disband_button)->id = GUILD_INFO_ID_DISBAND;
    ui_window_add_child(WND(self), BUTTON_WND(self->disband_button));
    string_free(&skin);

    box = (struct UIListBox *)mem_alloc(sizeof *box);
    if (box) {
        ui_list_box_construct(box);
        box->base.ops = &ui_list_box_agit_ops.base;
    }
    self->agit_list = box;
    ui_window_create(&box->base, 0x5f, 0x9e);
    label_width = ui_window_text_width(WND(self), msg_str(MSG_GUILD_CASTLES), 0, 0, 0xc, 0, 0);
    ui_window_move_base(&self->agit_list->base, label_width + 0x13, 0x81);
    self->agit_list->line_height = 0xd;
    self->agit_list->base.id = GUILD_INFO_ID_CASTLES;
    ui_window_add_child(WND(self), &self->agit_list->base);

    menu = mem_alloc(sizeof *menu);
    if (menu) {
        ui_list_box_construct(&menu->list);
        menu->list.base.ops = &ui_list_box_agit_menu_ops.base;
    }
    self->agit_menu = menu;
    ui_window_create(&menu->list.base, 0x78, 0x23);
    self->agit_menu->list.base.id = GUILD_INFO_ID_CASTLE_MENU;
    ui_window_set_show_base(&self->agit_menu->list.base, 0);
    ui_list_box_add_item(&self->agit_menu->list, msg_str(MSG_CASTLE_MENU_FIRST));
    ui_list_box_add_item(&self->agit_menu->list, msg_str(MSG_CASTLE_MENU_FIRST + 1));
    ui_window_add_child(WND(self), &self->agit_menu->list.base);
    ui_guild_info_manage_wnd_fill_castles(self);
}

const struct UIGuildWndOps ui_guild_member_manage_wnd_ops = {
    .base = {
        .destroy = ui_guild_member_manage_wnd_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 1,
        .refreshes_children = 0,
        .move = ui_frame_wnd_move,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_guild_wnd_store_info,
        .save_pos = ui_frame_wnd_save_pos,
        .move_delta = ui_frame_wnd_move_delta,
        .set_show = ui_window_set_show_base,
        .on_create = ui_guild_member_manage_wnd_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_guild_member_manage_wnd_on_process,
        .on_draw = ui_guild_member_manage_wnd_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_guild_member_manage_wnd_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_frame_wnd_on_mouse_move,
        .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
        .on_r_btn_down = ui_guild_member_manage_wnd_on_r_btn_down,
        .on_r_btn_up = ui_guild_member_manage_wnd_on_r_btn_up,
        .on_wheel = ui_guild_member_manage_wnd_on_wheel,
        .snap = ui_frame_wnd_snap,
        .send_msg = ui_guild_member_manage_wnd_send_msg,
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
    },
    .tab = GUILD_TAB_MEMBERS,
};

static void destroy_member(void *value)
{
    guild_member_destruct(value);
}

static void destroy_position(void *value)
{
    guild_position_destruct(value);
}

struct UIGuildMemberManageWnd *ui_guild_member_manage_wnd_construct(struct UIGuildMemberManageWnd *self)
{
    int i;

    ui_guild_wnd_construct(&self->base);
    WND(self)->ops = &ui_guild_member_manage_wnd_ops.base;
    self->menu.first = self->menu.last = self->menu.end = NULL;
    self->top = 0;
    self->selected = 0xa;
    self->per_row = 1;
    self->rows = GUILD_MEMBER_ROWS;
    self->menu_row = self->scrollable = self->row_gap = 0;
    list_init(&self->members);
    tree_init(&self->changes);
    tree_init(&self->sort_dirs);
    self->ok_time = 0;
    self->bar = 0;
    for (i = 0; i < GUILD_MEMBER_ROWS; i++)
        self->pos_buttons[i] = NULL;
    self->sort_column = -1;
    return self;
}

void ui_guild_member_manage_wnd_destruct(struct UIGuildMemberManageWnd *self)
{
    WND(self)->ops = &ui_guild_member_manage_wnd_ops.base;
    tree_free(&self->sort_dirs, NULL);
    tree_free(&self->changes, NULL);
    list_free(&self->members, destroy_member);
    ui_menu_entry_vec_free(&self->menu);
    ui_guild_wnd_destruct(&self->base);
}

void ui_guild_member_manage_wnd_destroy(struct UIWindow *window)
{
    struct UIGuildMemberManageWnd *self = (struct UIGuildMemberManageWnd *)window;
    ui_guild_member_manage_wnd_destruct(self);
    mem_free(self);
}

const struct UIGuildWndOps ui_guild_position_manage_wnd_ops = {
    .base = {
        .destroy = ui_guild_position_manage_wnd_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 1,
        .refreshes_children = 0,
        .move = ui_frame_wnd_move,
        .is_focusable = ui_window_is_focusable_base,
        .get_backdrop_box = ui_window_get_backdrop_box_base,
        .is_click_through = ui_window_is_click_through_base,
        .is_hit_testable = ui_window_is_hit_testable_base,
        .drag_and_drop = ui_window_drag_and_drop_base,
        .store_info = ui_guild_wnd_store_info,
        .save_pos = ui_frame_wnd_save_pos,
        .move_delta = ui_frame_wnd_move_delta,
        .set_show = ui_window_set_show_base,
        .on_create = ui_guild_position_manage_wnd_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_guild_position_manage_wnd_on_process,
        .on_draw = ui_guild_position_manage_wnd_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_guild_position_manage_wnd_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_frame_wnd_on_mouse_move,
        .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
        .on_r_btn_down = ui_guild_position_manage_wnd_on_r_btn_down,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_guild_position_manage_wnd_on_wheel,
        .snap = ui_frame_wnd_snap,
        .send_msg = ui_guild_position_manage_wnd_send_msg,
        .invalidate = ui_guild_position_manage_wnd_invalidate,
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
    },
    .tab = GUILD_TAB_POSITIONS,
};

struct UIGuildPositionManageWnd *ui_guild_position_manage_wnd_construct(struct UIGuildPositionManageWnd *self)
{
    int row;

    ui_guild_wnd_construct(&self->base);
    WND(self)->ops = &ui_guild_position_manage_wnd_ops.base;
    list_init(&self->positions);
    tree_init(&self->changes);
    self->ok_time = 0;
    for (row = 0; row < GUILD_POSITION_ROWS; row++) {
        self->invite_checks[row] = NULL;
        self->expel_checks[row] = NULL;
        self->storage_checks[row] = NULL;
        self->tax_edits[row] = NULL;
        self->name_edits[row] = NULL;
        self->tax_texts[row] = NULL;
        self->name_texts[row] = NULL;
    }
    return self;
}

void ui_guild_position_manage_wnd_destruct(struct UIGuildPositionManageWnd *self)
{
    WND(self)->ops = &ui_guild_position_manage_wnd_ops.base;
    tree_free(&self->changes, guild_position_edit_destroy);
    list_free(&self->positions, destroy_position);
    ui_guild_wnd_destruct(&self->base);
}

void ui_guild_position_manage_wnd_destroy(struct UIWindow *window)
{
    struct UIGuildPositionManageWnd *self = (struct UIGuildPositionManageWnd *)window;
    ui_guild_position_manage_wnd_destruct(self);
    mem_free(self);
}

void ui_guild_position_manage_wnd_invalidate(struct UIWindow *window)
{
    struct UIGuildPositionManageWnd *self = (struct UIGuildPositionManageWnd *)window;
    ui_window_invalidate_base(WND(self));
    ui_window_mgr_set_focus_edit(&g_windowMgr, NULL);
}

#define AGIT_BOX_ID_OK 0xb3
#define AGIT_BOX_ID_CANCEL 0xb4

void ui_agit_message_box_create(struct UIAgitMessageBox *self, const char *text, int width, int height)
{
    static const char *const names[2][3] = {
        { "btn_ok", "btn_ok_a", "btn_ok_b" },
        { "btn_cancel", "btn_cancel_a", "btn_cancel_b" },
    };
    static const int ids[2] = { AGIT_BOX_ID_OK, AGIT_BOX_ID_CANCEL };
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *viewer;
    struct String dir;
    int i;

    ui_window_create(wnd, width, height);
    ui_window_on_create_base(wnd, width, height);
    viewer = mem_alloc(sizeof(struct UITextViewer));
    if (viewer)
        viewer = ui_text_viewer_construct(viewer);
    ui_list_box_create_window((struct UIListBox *)viewer, 0xa, 0x16, width - 0x14, height - 0x37, 0);
    UI_TEXT_VIEWER_BACKGROUND(viewer) = ui_skin_argb(2, 2);
    ui_window_invalidate(viewer);
    ui_window_add_child(wnd, viewer);
    ui_window_send_msg(viewer, NULL, UI_MSG_ADD_TEXT, (int)text, width / 6, 0, 0);

    ui_skin_dir(&dir);
    for (i = 0; i < 2; i++) {
        struct UIBitmapButton *button = mem_alloc(sizeof(struct UIBitmapButton));

        if (button)
            button = ui_bitmap_button_construct(button);
        ui_button_bitmap(button, &dir, names[i][0], 0);
        ui_button_bitmap(button, &dir, names[i][1], 1);
        ui_button_bitmap(button, &dir, names[i][2], 2);
        ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
        ui_window_move_base(&button->button.base, wnd->width - (button->bitmap_width + 5) * (2 - i), height - 0x18);
        button->button.base.id = ids[i];
        ui_window_add_child(wnd, &button->button.base);
    }
    string_free(&dir);
}

#define MSG_MEMBER_UNNAMED 0x245
#define MSG_LAST_LOGIN_TIME 0xbc3
#define MSG_LAST_LOGIN 0xbc4
#define MEMBER_ONLINE_COLOUR 0xe4ffe2
#define MEMBER_EXP_MAX 0x7fffffff
#define MEMBER_EXP_MAX_COLOUR 0xff

BYTE ui_guild_member_manage_wnd_get_member(struct UIGuildMemberManageWnd *self, int index,
                                                 struct GuildMember *out)
{
    struct ListNode *head = (struct ListNode *)self->members.head, *node;
    const struct GuildMember *m;

    for (node = head->next; node != head && index; node = node->next)
        index--;
    if (node == head)
        return 0;
    m = LIST_VALUE(node, struct GuildMember);
    out->aid = m->aid;
    out->gid = m->gid;
    string_assign(&out->name, string_cstr(&m->name), m->name.size);
    out->hair_style = m->hair_style;
    out->hair_color = m->hair_color;
    out->job = m->job;
    out->sex = m->sex;
    out->position = m->position;
    string_assign(&out->position_name, string_cstr(&m->position_name), m->position_name.size);
    out->level = m->level;
    string_assign(&out->memo, string_cstr(&m->memo), m->memo.size);
    out->exp_share = m->exp_share;
    out->exp = m->exp;
    out->online = m->online;
    out->last_login = m->last_login;
    return 1;
}

static int member_draw_head(struct UIGuildMemberManageWnd *self, struct GuildMember *m, int row)
{
    struct String act_path, spr_path;
    struct ActRes *act;
    struct SprRes *spr;
    struct SprClip *clip;
    struct SprImg *img;
    struct SpriteRegion place = {0};
    DWORD *palette;
    void *tex;
    const int h = self->row_gap;

    session_get_head_spr_name0(&act_path, m->job, m->sex, &m->hair_style);
    session_get_head_spr_name1(&spr_path, m->job, m->sex, &m->hair_style);
    act = res_mgr_get(g_resMgr(), string_cstr(&act_path));
    spr = res_mgr_get(g_resMgr(), string_cstr(&spr_path));
    clip = motion_get_clip(act_res_get_motion(act, 0, 0), 1);
    if (clip->sprIndex == -1) {
        string_free(&spr_path);
        string_free(&act_path);
        return 0;
    }
    img = spr_res_image(spr, clip);
    if (!img) {
        string_free(&spr_path);
        string_free(&act_path);
        return 0;
    }
    palette = spr->palette;
    if (m->hair_color) {
        struct String path;

        session_get_head_pal_name(&path, m->hair_style, m->job, m->sex, m->hair_color);
        palette = palette_resource_colors(res_mgr_get(g_resMgr(), string_cstr(&path)), spr->palette);
        string_free(&path);
    }
    memset(&place, 0, sizeof place);
    if (clip->clipType == 0) {
        tex = sprite_atlas_resolve(g_renderer, img, palette, &place);
    } else {
        tex = img->texture;
    }
    if (tex) ui_canvas_draw_texture(WND(self)->canvas, (h - img->width + 0x24) / 2 + 6, (h + 0x24) * row + 0x46, tex,
                         place.x, place.y, img->width, img->height, clip->flags,
                         img->widthHalved + 1, img->heightHalved + 1);
    string_free(&spr_path);
    string_free(&act_path);
    return tex != NULL;
}

static void member_draw_right(struct UIGuildMemberManageWnd *self, int x, int y, struct String *text,
                              COLORREF colour)
{
    ui_window_text_out_right(WND(self), x, y, string_cstr(text), 0, 0, 0xc, colour, 0);
    string_free(text);
}

BYTE ui_guild_member_manage_wnd_draw_row(struct UIGuildMemberManageWnd *self, struct GuildMember *m,
                                               int *counter, int row)
{
    struct UIWindow *wnd = WND(self);
    const int *cols = g_guildLayout.member_position_cols;
    const int h = self->row_gap;
    struct String cut, job, text;
    char time_text[0x80];
    const char *job_name;
    int y, x;

    if (*counter < self->per_row * self->top) {
        ++*counter;
        return 0;
    }
    if (row >= GUILD_MEMBER_ROWS - h * GUILD_MEMBER_ROWS / 0x24)
        return 0;
    if (game_current_base_mode()) {
        {
            struct TextView cached_name = player_name_lookup(m->gid);
            string_assign(&m->name, cached_name.data, cached_name.size);
        }
    }
    if (m->name.size == 0) {
        const char *word = msg_str(MSG_MEMBER_UNNAMED);
        struct String unnamed;

        string_init(&unnamed);
        string_reserve(&unnamed, strlen(word) + text_tag_NAMELESS.size);
        string_append_cstr(&unnamed, word);
        string_append(&unnamed, string_cstr(&text_tag_NAMELESS), text_tag_NAMELESS.size);
        string_free(&m->name);
        string_move(&m->name, &unnamed);
    }
    if (m->online == 1)
        ui_window_fill_rect(wnd, 6, (h + 0x24) * row + 0x43, 0x177, h + 0x23, MEMBER_ONLINE_COLOUR);
    if (self->selected == *counter)
        ui_window_fill_rect(wnd, 6, (h + 0x24) * row + 0x43, 0x177, h + 0x23, ui_skin_colour(6, 2));
    if (m->online == 1 && !member_draw_head(self, m, row))
        return 0;

    y = (h + 0x24) * row + 0x4f + h / 2;
    guild_name_cut_nameless(&cut, &m->name);
    ui_static_text_set_text_ellipsis_wide(self->col_name[row], string_cstr(&cut));
    ui_window_move(self->col_name[row], 0x2b, y - h / 2);
    ui_static_text_set_text_ellipsis_wide(self->col_2[row], string_cstr(&m->position_name));
    ui_window_move(self->col_2[row], cols[0] + 0xb, y);
    if (self->pos_buttons[row])
        ui_window_move_base(&self->pos_buttons[row]->button.base, 0x96, y - 4);
    x = cols[0] + 8 + cols[1];
    job_name = session_get_job_name(&g_session1, m->job, m->sex);
    string_init(&job);
    string_assign_cstr(&job, job_name);
    ui_static_text_set_text_ellipsis_wide(self->col_3[row], string_cstr(&job));
    ui_window_move(self->col_3[row], x + 4, y);
    x += cols[2] + 1;
    string_init(&text);
    string_format(&text, "%d", m->level);
    member_draw_right(self, cols[3] + x - 4, y, &text, 0);
    x += cols[3] + 1;
    string_format(&text, "%3.0f%%", (double)m->exp_share);
    member_draw_right(self, cols[4] + x - 4, y, &text, 0);
    x += cols[4] + 1;
    string_format(&text, "%d", m->exp);
    member_draw_right(self, cols[5] + x - 4, y, &text, m->exp >= MEMBER_EXP_MAX ? MEMBER_EXP_MAX_COLOUR : 0);
    strftime(time_text, sizeof time_text, msg_str(MSG_LAST_LOGIN_TIME), _localtime32((__time32_t *)&m->last_login));
    string_format(&text, msg_str(MSG_LAST_LOGIN), time_text);
    ui_window_text_out(wnd, 8, y + 0x11, string_cstr(&text), 0, 0, 0xb, 0x404040, 0, 0);
    string_free(&text);
    string_free(&job);
    string_free(&cut);
    return 1;
}

int ui_guild_position_manage_wnd_hit_test(struct UIGuildPositionManageWnd *self, int x, int y)
{
    const struct GuildListView view = {
        .top = self->top, .per_row = self->per_row, .rows = self->rows
    };
    return guild_list_view_hit_test(&view, WND(self), self->positions.head, x, y,
                                 g_guildLayout.position_row_height);
}

struct GuildPosition *ui_guild_position_manage_wnd_get_position(struct UIGuildPositionManageWnd *self,
                                                                        int index, int *found)
{
    static struct GuildPosition none;
    static int none_made;
    struct ListNode *head = (struct ListNode *)self->positions.head, *node;

    *found = 1;
    if (!none_made) {
        none_made = 1;
        string_init(&none.name);
    }
    for (node = head->next; node != head; node = node->next, index--)
        if (index == 0)
            return LIST_VALUE(node, struct GuildPosition);
    *found = 0;
    return &none;
}
