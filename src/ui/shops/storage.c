#include "core/thread_text.h"
#include "ui/controls/edit.h"
#include "ui/controls/buttons.h"
#include "ui/shops/shops.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <string.h>
#include "ui/shops/storage_internal.h"
#include "ui/shops/storage.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/runtime.h"
#include "ui/shops/item_store_combo.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/items/items.h"

struct UIItemStoreWnd;
void ui_item_store_wnd_destroy(struct UIWindow *window);
void ui_item_store_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_item_store_wnd_store_info(struct UIWindow *self);
void ui_item_store_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_item_store_wnd_on_draw(struct UIWindow *self);
void ui_item_store_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_item_store_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_item_store_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
intptr_t ui_item_store_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

const struct UIWindowOps ui_item_store_wnd_ops = {
    .destroy = ui_item_store_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_store_wnd_drag_and_drop,
    .store_info = ui_item_store_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_store_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_store_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_store_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_store_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_store_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_store_wnd_send_msg,
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
struct UIBitmapEditCtrl;
const struct UIEditCtrlOps ui_bitmap_edit_ctrl_ops = {
    .base = {
        .destroy = ui_bitmap_edit_ctrl_destroy,
        .resize = ui_window_resize_base,
        .is_frame = 0,
        .refreshes_children = 0,
        .move = ui_window_move_base,
        .is_focusable = ui_edit_ctrl_is_focusable,
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
        .on_draw = ui_bitmap_edit_ctrl_on_draw,
        .on_focus = ui_edit_ctrl_on_focus,
        .on_focus_lost = ui_edit_ctrl_on_focus_lost,
        .on_l_btn_down = ui_edit_ctrl_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
        .on_mouse_move = ui_window_on_mouse_move_base,
        .on_mouse_hover = ui_edit_ctrl_on_mouse_hover,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_window_on_l_btn_up_base,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_edit_ctrl_send_msg,
        .invalidate = ui_window_invalidate_base,
        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .type = UI_TYPE_EDIT,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_text = ui_edit_ctrl_set_text_default,
    .draw_text = ui_edit_ctrl_draw_text_default,
    .truncate = ui_edit_ctrl_truncate_default,
};

static const char *const s_tabNames[STORE_TABS] = {
    "소비",
    "캐시",
    "방어구",
    "무기",
    "의상",
    "투사체",
    "카드",
    "기타",
};
static const int s_tabIds[STORE_TABS] = { 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, STORE_TAB_WND_LAST_ID };
static const char *const s_closeButton[3] = { "btn_close", "btn_close_a", "btn_close_b" };
static const char *const s_searchButton[3] = { "bt_search_normal", "bt_search_over", "bt_search_press" };

struct UIItemStoreWnd *ui_item_store_wnd_construct(struct UIItemStoreWnd *self)
{
    int i;

    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_item_store_wnd_ops;
    string_init(&self->count_text);
    self->margins[0] = self->margins[1] = self->margins[2] = 0;
    self->category = 0;
    self->base.show_counts = 1;
    self->base.button_bar = 1;
    self->info = NULL;
    self->margins[3] = 0;
    self->spacing = 0;
    self->edit = NULL;
    for (i = 0; i < STORE_TABS; i++) {
        self->tab_flags[i] = 0;
        self->boxes[i] = NULL;
        self->tab_wnds[i] = NULL;
    }
    self->find_wnd = NULL;
    self->count = 0;
    self->capacity = 0;
    self->list_received = 0;
    return self;
}

struct UIWindow *ui_item_store_wnd_new(void)
{
    return (struct UIWindow *)ui_item_store_wnd_construct(mem_alloc(sizeof(struct UIItemStoreWnd)));
}

void ui_item_store_wnd_destruct(struct UIItemStoreWnd *self)
{
    self->base.frame.base.ops = &ui_item_store_wnd_ops;
    g_session1.guildStorageOpen = 0;
    string_free(&self->count_text);
    ui_item_base_wnd_destruct(&self->base);
}

void ui_item_store_wnd_destroy(struct UIWindow *window)
{
    struct UIItemStoreWnd *self = (struct UIItemStoreWnd *)window;
    ui_item_store_wnd_destruct(self);
    mem_free(self);
}

void ui_item_store_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemStoreWnd *self = (struct UIItemStoreWnd *)window;
    if (g_session1.guildStorageOpen) {
        char buf[0xfa];

        if (g_session1.guildStorageInUse == 1) {
            memset(buf, 0, sizeof buf);
            sprintf(buf, msg_str(0x9c2));
            ui_chat_line(buf, 0xff);
            return;
        }
        if (!g_session1.guildStorageCount && !g_session1.guildStorageInUse) {
            memset(buf, 0, sizeof buf);
            sprintf(buf, msg_str(0x9c5));
            ui_chat_line(buf, 0xff);
            return;
        }
    }
    storage_drop(&self->base, x, y, drag);
}

void ui_item_store_wnd_store_info(struct UIWindow *window)
{
    struct UIItemStoreWnd *self = (struct UIItemStoreWnd *)window;
    storage_store_info(&self->base.frame.base, &self->info->frame);
}

void ui_item_store_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemStoreWnd *self = (struct UIItemStoreWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIBitmapEditCtrl *edit;
    struct UIItemStoreComboBoxWnd *combo;
    int i;

    (void)width;
    self->title = shop_add_title(wnd, 0x46, g_session1.guildStorageOpen ? 0x9c4 : 0xa9);
    shop_add_resizer(&self->base, UI_EDGE_BOTTOM);
    self->tabs = mem_alloc(sizeof *self->tabs);
    if (self->tabs)
        self->tabs = ui_tab_construct(self->tabs);
    ui_window_create(&self->tabs->base, 0x14, 0xeb);
    ui_window_move_base(&self->tabs->base, 0, 0x12);
    self->tabs->count = STORE_TABS;
    self->tabs->kind = UI_TAB_ITEM;
    ui_tab_send_msg(&self->tabs->base, wnd, UI_MSG_SET_STATE, 0, 0, 0, 0);
    ui_window_add_child(wnd, &self->tabs->base);
    shop_add_scroll_bar(&self->base, wnd->height - 0x45);

    edit = mem_alloc(sizeof *edit);
    if (edit) {
        ui_edit_ctrl_construct(&edit->edit.base);
        edit->edit.base.ops = &ui_bitmap_edit_ctrl_ops.base;
        string_init(&edit->bitmap);
    }
    self->edit = edit;
    ui_window_create(&edit->edit.base, 0x7a, 0x12);
    ui_bitmap_edit_ctrl_set_bitmap(self->edit, UI_SKIN "bg_search.bmp");
    ui_window_move_base(&self->edit->edit.base, 0x41, wnd->height - 0x17);
    self->edit->edit.max_chars = 0x14;
    ui_window_add_child(wnd, &self->edit->edit.base);

    for (i = 0; i < 2; i++) {
        struct UIBitmapButton *button = mem_alloc(sizeof *button);

        if (button)
            button = ui_bitmap_button_construct(button);
        if (i == 0)
            self->base.close = button;
        else
            self->base.extra_buttons[0] = button;
        shop_place_button(wnd, button, i == 0 ? s_closeButton : s_searchButton, i == 0 ? 0xdc : 0xb9,
                          wnd->height - 0x17, i == 0 ? UI_ID_CLOSE : STORE_ID_SEARCH);
    }

    for (i = 0; i < STORE_TABS; i++) {
        struct UIWindow *box = mem_alloc(sizeof(struct UICheckBox));
        char checked[0x80], unchecked[0x80];

        if (box)
            box = (struct UIWindow *)ui_check_box_construct(box);
        self->boxes[i] = box;
        sprintf(unchecked, UI_SKIN "basic_interface\\%s-2.bmp", s_tabNames[i]);
        sprintf(checked, UI_SKIN "basic_interface\\%s-1.bmp", s_tabNames[i]);
        ui_check_box_set_bitmaps_fit((struct UICheckBox *)self->boxes[i], checked, unchecked);
        ui_window_create(self->boxes[i], 0x17, 0x17);
        ui_window_move_base(self->boxes[i], 5 + 0x1a * i, wnd->height - 0x34);
        ui_check_box_set_check(self->boxes[i], 0);
        self->boxes[i]->id = s_tabIds[i];
        if (i < 4)
            ui_check_box_set_tool_tip(self->boxes[i], msg_str(0x5b9 + i));
        else if (i == 4)
            ui_check_box_set_tool_tip(self->boxes[4], msg_str(0xc4f));
        else
            ui_check_box_set_tool_tip(self->boxes[i], msg_str(0x5b8 + i));
        ui_window_add_child(wnd, self->boxes[i]);
    }

    UI_FRAME_DEFAULT_ID(wnd) = STORE_ID_SEARCH;
    string_clear(&self->count_text);
    self->base.columns = 1;
    self->base.rows = (height - 0x4a) / ITEM_GRID_CELL;
    ui_window_add_close_button(wnd, "", 0);

    combo = mem_alloc(sizeof *combo);
    if (combo)
        combo = ui_item_store_combo_box_wnd_construct(combo);
    self->combo = &combo->def.base;
    combo->pink = 0;
    ui_window_create(self->combo, 0x3e, 0x12);
    ui_window_move_base(self->combo, 0xd6, wnd->height - 0x34);
    ui_window_add_child(wnd, self->combo);
    ui_combo_add(self->combo, msg_str(0xccc));
    ui_combo_add(self->combo, msg_str(0xcca));
    ui_combo_add(self->combo, msg_str(0xccb));
    for (i = 0; i < 3; i++) {
        if (g_session1.itemSort[i] == 1) {
            ui_combo_box_def_wnd_select(self->combo, i);
            ui_item_store_wnd_send_msg(wnd, self->combo, STORE_MSG_SORT_CHOSEN, i, 0, 0, 0);
            return;
        }
        if (i == 2) {
            ui_combo_box_def_wnd_select(self->combo, 0);
            ui_item_store_wnd_send_msg(wnd, self->combo, STORE_MSG_SORT_CHOSEN, 0, 0, 0, 0);
        }
    }
}

void ui_item_store_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemStoreWnd *self = (struct UIItemStoreWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    int columns, rows, i, width;
    struct String text;

    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (wnd->height == ITEM_GRID_Y)
        return;
    shop_draw_frame(&self->base, wnd->height - 0x26, self->base.button_bar ? 3 : 0);
    columns = (wnd->width - 0x28) / ITEM_GRID_CELL;
    rows = (wnd->height - 0x4a) / ITEM_GRID_CELL;
    if (!self->list_received) {
        ui_window_set_show_base(&self->base.scroll_bar->base, 0);
        ui_window_fill_rect(wnd, 0x28, ITEM_GRID_Y, columns * ITEM_GRID_CELL, rows * ITEM_GRID_CELL,
                          ui_skin_colour(2, 2));
        ui_window_draw_round_frame(wnd, 0x37, 0x83, 0xbb, 0x8b, 0xce9484);
        ui_window_fill_round_rect(wnd, 0x38, 0x84, 0xb9, 0x89, 0xffe7de);
        ui_window_fill_round_rect(wnd, 0x3d, 0x89, 0xaf, 0x7f, 0xffffff);
        ui_window_draw_bitmap(wnd, 0x74, 0x97,
                            res_mgr_get(g_resMgr(), UI_SKIN "basic_interface\\notice_poring.bmp"), 1);
        ui_window_text_out_centred(wnd, 0x14, 0xd2, msg_str(0xdd4), 0x101, 0, 0xc, 0, 0);
        ui_window_text_out_centred(wnd, 0x14, 0xec, msg_str(0xc77), 0x101, 0, 0xf, 0xc89482, 1);
        ui_window_text_out_centred(wnd, 0x14, 0x110, msg_str(0xe13), 0x101, 0, 0xc, 0x6464dc, 1);
        return;
    }
    ui_window_set_show_base(&self->base.scroll_bar->base, 1);
    for (i = 0; i < rows; i++)
        ui_window_draw_item_win_mid(wnd, ITEM_GRID_X, ITEM_GRID_Y + i * ITEM_GRID_CELL);
    ui_window_fill_rect(wnd, 0x48, ITEM_GRID_Y, columns * ITEM_GRID_CELL - 0x20, rows * ITEM_GRID_CELL,
                      ui_skin_colour(2, 2));
    storage_draw_items(&self->base, ITEM_GRID_X, 0xb4);
    ui_window_draw_bitmap(wnd, 6, wnd->height - 0x15,
                        res_mgr_get(g_resMgr(), ui_skin_path(UI_SKIN "inventory\\icon_num.bmp")), 1);
    string_init(&text);
    string_format(&text, "%d", self->count);
    ui_window_text_out(wnd, 0x16, wnd->height - 0x13, string_cstr(&text), 0, 0, 0xb,
                     self->count >= self->capacity ? 0xff : 0x232323, 0, 0);
    string_free(&text);
    string_format(&text, "%d", self->count);
    width = ui_window_text_width(wnd, string_cstr(&text), 0, 0, 0xb, 0, 0);
    string_free(&text);
    string_format(&text, "/%d", self->capacity);
    ui_window_text_out(wnd, width + 0x17, wnd->height - 0x13, string_cstr(&text), 0, 0, 0xb, 0, 0, 0);
    string_free(&text);
}

void ui_item_store_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreWnd *self = (struct UIItemStoreWnd *)window;
    storage_on_lbtn_down(&self->base, x, y, ITEM_GRID_X);
}

void ui_item_store_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreWnd *self = (struct UIItemStoreWnd *)window;
    storage_on_mouse_hover(&self->base, x, y, ITEM_GRID_X);
}

void ui_item_store_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreWnd *self = (struct UIItemStoreWnd *)window;
    storage_on_r_btn_down(&self->base, x, y, ITEM_GRID_X);
}

static void store_tell_lists(int msg)
{
    struct UIWindow *wnd;
    int i;

    for (i = 0; i < 7; i++)
        if ((wnd = ui_window_find(STORE_TAB_WND_FIRST + i)) != NULL)
            ui_window_send_msg(wnd, NULL, msg, 0, 0, 0, 0);
    if ((wnd = ui_window_find(STORE_TAB_WND_LAST_ID)) != NULL)
        ui_window_send_msg(wnd, NULL, msg, 0, 0, 0, 0);
    if ((wnd = ui_window_find(STORE_FIND_WND_ID)) != NULL)
        ui_window_send_msg(wnd, NULL, msg, 0, 0, 0, 0);
}

static void store_search_text(struct UIItemStoreWnd *self, struct String *out)
{
    const char *text = ui_edit_ctrl_get_text(&self->edit->edit.base);

    string_init(out);
    string_assign_cstr(out, text);
}

static void store_toggle_tab(struct UIItemStoreWnd *self, int i)
{
    int id = s_tabIds[i];

    if (!self->boxes[i])
        return;
    if (self->boxes[i]->state) {
        self->tab_wnds[i] = NULL;
        self->tab_wnds[i] = ui_window_open(id);
        if (self->tab_wnds[i])
            ui_window_send_msg(self->tab_wnds[i], NULL, STORE_MSG_REFILL, 0, 0, 0, 0);
        self->info->open_tabs |= 1u << i;
    } else {
        ui_window_mgr_post_quit1(&g_windowMgr, id);
        if (self->tab_wnds[i]) {
            self->info->open_tabs ^= 1u << i;
            self->tab_wnds[i] = NULL;
        }
    }
}

static void store_open_find(struct UIItemStoreWnd *self, const struct String *text)
{
    struct String copy;

    string_copy(&copy, text);
    storage_find_set_search((struct UIItemStoreFindWnd *)self->find_wnd, copy);
}

static int store_command(struct UIItemStoreWnd *self, int id)
{
    struct UIWindow *wnd = &self->base.frame.base;
    struct String text;
    int i;

    switch (id) {
    case STORE_ID_FIND:
        if (self->find_wnd)
            return 0;
        self->find_wnd = ui_window_open(STORE_FIND_WND_ID);
        store_search_text(self, &text);
        if (text.size && storage_filter(&self->base, &text, 0)) {
            store_open_find(self, &text);
            ui_window_send_msg(self->find_wnd, NULL, STORE_MSG_SEARCH, 0, 0, 0, 0);
        }
        string_free(&text);
        return 0;
    case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: case 0x98:
        store_toggle_tab(self, id - STORE_TAB_WND_FIRST);
        return 0;
    case STORE_TAB_WND_LAST_ID:
        store_toggle_tab(self, 7);
        return 0;
    case UI_ID_CLOSE: {
        unsigned open_tabs = self->info->open_tabs;

        for (i = 0; i < 7; i++)
            ui_window_mgr_post_quit1(&g_windowMgr, STORE_TAB_WND_FIRST + i);
        ui_window_mgr_post_quit1(&g_windowMgr, STORE_TAB_WND_LAST_ID);
        self->info->open_tabs = open_tabs;
        ui_window_mgr_post_quit1(&g_windowMgr, STORE_FIND_WND_ID);
        self->find_wnd = NULL;
        if (client_current_mode())
            mode_send_message(client_current_mode(), MODE_REQ_CLOSE_STORAGE, 0, 0, 0, 0);
        return 0;
    }
    case STORE_ID_UNSORT:
        self->sorted = 0;
        ui_button_show((struct UIButton *)self->base.extra_buttons[0], 1);
        ui_button_show((struct UIButton *)self->base.extra_buttons[2], 0);
        ui_window_move((struct UIWindow *)self->base.extra_buttons[0], wnd->width - 0x69, wnd->height - 0x18);
        ui_item_store_wnd_send_msg(wnd, NULL, STORE_MSG_REFILL, 0, 0, 0, 0);
        for (i = 0; i < 7; i++)
            if (self->tab_wnds[i])
                ui_window_send_msg(self->tab_wnds[i], NULL, STORE_MSG_UNSORT, 0, 0, 0, 0);
        ui_window_move((struct UIWindow *)self->base.extra_buttons[0], 0xdc, wnd->height - 0x32);
        return 0;
    case STORE_MSG_SORT: {
        struct UIWindow *list;

        storage_sort(&self->base, 0);
        ui_window_invalidate_base(wnd);
        ui_item_base_wnd_update_scroll(&self->base);
        ui_button_show((struct UIButton *)self->base.extra_buttons[0], 0);
        ui_button_show((struct UIButton *)self->base.extra_buttons[2], 1);
        ui_window_move((struct UIWindow *)self->base.extra_buttons[2], wnd->width - 0x30, wnd->height - 0x34);
        for (i = 0; i < 7; i++)
            if (ui_window_find(STORE_TAB_WND_FIRST + i)) {
                list = ui_window_find(STORE_TAB_WND_FIRST + i);
                ui_window_send(list, NULL, STORE_MSG_SORT, 0, 0, 0, 0);
            }
        if (ui_window_find(STORE_TAB_WND_LAST_ID)) {
            list = ui_window_find(STORE_TAB_WND_LAST_ID);
            ui_window_send(list, NULL, STORE_MSG_SORT, 0, 0, 0, 0);
        }
        if (self->find_wnd)
            ui_window_send_msg(self->find_wnd, NULL, STORE_MSG_SORT, 0, 0, 0, 0);
        return 0;
    }
    case STORE_ID_SEARCH:
        store_search_text(self, &text);
        if (text.size) {
            self->find_wnd = ui_window_open(STORE_FIND_WND_ID);
            store_open_find(self, &text);
            ui_window_send_msg(self->find_wnd, NULL, STORE_MSG_REFILL, STORE_MSG_SEARCH, 0, 0, 0);
            ui_window_send_msg(self->find_wnd, NULL, STORE_MSG_SEARCH, 0, 0, 0, 0);
            ui_edit_ctrl_set_text_default(&self->edit->edit, "");
        }
        string_free(&text);
        return 0;
    }
    return 0;
}

intptr_t ui_item_store_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                      intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIItemStoreWnd *self = (struct UIItemStoreWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    int i;

    switch (msg) {
    case UI_MSG_COMMAND:
        return store_command(self, val1);
    case UI_MSG_RESIZE: {
        int most = g_windowMgr.screen_height * 7 / 10, height = val3;

        if (val1 != UI_EDGE_BOTTOM)
            return 0;
        if (height > most)
            height = most;
        if (height < 0x1ca)
            height = 0x1ca;
        height = (height - 0x1ca) / ITEM_GRID_CELL * ITEM_GRID_CELL + 0x1ca;
        if (height != wnd->height) {
            ui_window_resize_base(wnd, wnd->width, height);
            self->base.columns = 1;
            self->base.rows = (height - 0x4a) / ITEM_GRID_CELL;
            ui_window_move_base(&self->base.close->button.base, 0xdc, wnd->height - 0x17);
            ui_window_move((struct UIWindow *)self->base.extra_buttons[0], 0xb9, wnd->height - 0x17);
            for (i = 0; i < STORE_TABS; i++)
                ui_window_move_base(self->boxes[i], 5 + 0x1a * i, wnd->height - 0x34);
            ui_window_move_base(self->combo, 0xd6, wnd->height - 0x34);
            ui_window_move_base(&self->edit->edit.base, 0x4d, wnd->height - 0x17);
            ui_window_move_base(&self->base.resizer->base, wnd->width - 0xf, wnd->height - 0xf);
            ui_window_move_base(&self->base.scroll_bar->base, wnd->width - 0xf, 0xe);
            ui_scroll_bar_resize(&self->base.scroll_bar->base, self->base.scroll_bar->base.width, wnd->height - 0x4a);
            ui_item_base_wnd_update_scroll(&self->base);
        }
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;
    }
    case UI_MSG_TAB_CHANGED:
        self->category = val1 < 0 ? 0 : val1 > 7 ? 7 : val1;
        ui_window_invalidate_base(wnd);
        ui_item_store_wnd_send_msg(wnd, NULL, STORE_MSG_REFILL, 0, 0, 0, 0);
        return 0;
    case STORE_MSG_REFILL: {
        struct String text;

        storage_fill(&self->base, self->category);
        storage_sort_chosen(&self->base);
        store_search_text(self, &text);
        if (text.size && val1 == STORE_MSG_SEARCH) {
            if (storage_filter(&self->base, &text, 0)) {
                ui_window_invalidate_base(wnd);
                ui_item_base_wnd_update_scroll(&self->base);
                store_tell_lists(STORE_MSG_REFILL);
            }
        } else {
            store_tell_lists(STORE_MSG_REFILL);
            ui_window_invalidate_base(wnd);
            ui_item_base_wnd_update_scroll(&self->base);
        }
        string_free(&text);
        return 0;
    }
    case UI_MSG_SET_INFO:
        self->info = (struct StoreWndInfo *)val1;
        if (!shop_info_usable(&self->info->frame)) {
            self->info->frame.x = 0x168;
            self->info->frame.y = 0xc8;
            self->info->frame.width = 0x118;
            self->info->frame.height = 0x1ca;
            self->info->open_tabs = 0;
        }
        ui_frame_wnd_move(wnd, self->info->frame.x, self->info->frame.y);
        ui_item_store_wnd_send_msg(wnd, NULL, UI_MSG_RESIZE, UI_EDGE_BOTTOM, self->info->frame.width,
                 self->info->frame.height, 0);
        self->margins[0] = self->margins[1] = self->margins[2] = self->margins[3] = 3;
        self->spacing = 4;
        for (i = 0; i < STORE_TABS; i++)
            if ((self->info->open_tabs & (1u << i)) && self->boxes[i])
                ui_check_box_set_check(self->boxes[i], 1);
        ui_window_invalidate_base(wnd);
        return 0;
    case UI_MSG_ADD_DRAGGED:
        storage_add_dragged();
        return 0;
    case STORE_MSG_SORT_CHOSEN:
        if (sender != self->combo)
            return 0;
        for (i = 0; i < 3; i++)
            g_session1.itemSort[i] = i == val1;
        if (val1 == 0) {
            ui_item_store_wnd_send_msg(wnd, NULL, STORE_MSG_REFILL, 0, 0, 0, 0);
            return 0;
        }
        if (val1 == 1)
            storage_sort(&self->base, 0);
        else if (val1 == 2)
            storage_sort(&self->base, 1);
        for (i = 0; i < STORE_TABS; i++)
            if (self->tab_wnds[i])
                ui_window_send_msg(self->tab_wnds[i], NULL, STORE_MSG_SORT, 0, 0, 0, 0);
        if (self->find_wnd)
            ui_window_send_msg(self->find_wnd, NULL, STORE_MSG_SORT, 0, 0, 0, 0);
        ui_window_invalidate_base(wnd);
        ui_item_base_wnd_update_scroll(&self->base);
        return 0;
    case STORE_MSG_COUNT: {
        const char *text;

        self->count = val1;
        self->capacity = val2;
        text = thread_text_format("%d/%d", (int)val1, (int)val2);
        string_assign_cstr(&self->count_text, text);
        ui_window_invalidate_base(wnd);
        return 0;
    }
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}

void ui_item_store_wnd_set_page_check(struct UIItemStoreWnd *self, int page, BYTE checked)
{
    ui_check_box_set_check(self->boxes[page], checked);
}

void ui_storage_set_title(struct UIWindow *wnd, const char *name)
{
    ui_static_text_set_text_ellipsis(((struct UIItemStoreWnd *)wnd)->title, name);
}

void ui_storage_set_list_received(struct UIWindow *wnd)
{
    ((struct UIItemStoreWnd *)wnd)->list_received = 1;
}
