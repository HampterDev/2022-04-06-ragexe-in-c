#include "ui/controls/scroll.h"
#include "world/tooltip.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <stdio.h>
#include <stdlib.h>
#include "contents/client_res.h"
#include "contents/enchant_ui.h"
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "ui/controls/buttons.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "base.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"
#include "contents/ui_window.h"
#include "contents/ui_window_mgr.h"
#include "diag/perf.h"
#include "session/session_items.h"

#define ENCHANT_MSG_COMMAND     6
#define ENCHANT_MSG_SHOW_ITEM   0x18
#define LIST_MSG_SCROLL_BY      7
#define LIST_MSG_SCROLL_UP      9
#define LIST_MSG_SCROLL_DOWN    0xa
#define LIST_MSG_REFRESH        0x17
#define LIST_MSG_ADD            0x1f
#define LIST_MSG_CLEAR          0x4b
#define LIST_CMD_SELECTED       0x148
#define WID_ITEM_INFO           0xc

#define NAME_GREEN      0xc89682
#define NAME_GREY       0xaaaaaa
#define NAME_WHITE      0xffffff
#define NAME_SHADOW     0x959595

void enchant_listbox_delete(struct UIWindow *window);
void enchant_listbox_create(struct UIWindow *self, int width, int height);
void enchant_listbox_draw(struct UIWindow *self);
void enchant_listbox_hover(struct UIWindow *self, int x, int y);
void enchant_listbox_pressed(struct UIWindow *self, int x, int y);
void enchant_listbox_show_info(struct UIWindow *self, int x, int y);
void enchant_listbox_wheel(struct UIWindow *self, int delta);
int enchant_listbox_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int wparam, int lparam, int extra1, int extra2);
void enchant_listbox_unhover(struct UIWindow *self);

const struct UIWindowOps enchant_listbox_ops = {
    .destroy = enchant_listbox_delete,
    .resize = ui_window_resize_base,
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
    .on_create = enchant_listbox_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = enchant_listbox_draw,    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = enchant_listbox_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = enchant_listbox_pressed,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = enchant_listbox_show_info,    .on_wheel = enchant_listbox_wheel,
    .snap = ui_window_snap_base,
    .send_msg = enchant_listbox_send_msg,
    .invalidate = ui_window_invalidate_base,    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = enchant_listbox_unhover,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};
void enchant_target_list_delete(struct UIWindow *window);
void enchant_target_list_draw(struct UIWindow *self);

const struct UIWindowOps enchant_target_list_ops = {
    .destroy = enchant_target_list_delete,
    .resize = ui_window_resize_base,
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
    .on_create = enchant_listbox_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = enchant_target_list_draw,    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = enchant_listbox_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = enchant_listbox_pressed,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = enchant_listbox_show_info,    .on_wheel = enchant_listbox_wheel,
    .snap = ui_window_snap_base,
    .send_msg = enchant_listbox_send_msg,
    .invalidate = ui_window_invalidate_base,    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = enchant_listbox_unhover,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

const struct UIWindowOps enchant_scrollbar_ops = {
    .destroy = enchant_scrollbar_delete,
    .resize = ui_scroll_bar_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_scroll_bar_is_click_through,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = enchant_scrollbar_draw,    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_scroll_bar_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_scroll_bar_on_l_btn_dbl_clk,
    .on_mouse_move = ui_scroll_bar_on_mouse_move,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_scroll_bar_on_mouse_shape,
    .on_l_btn_up = ui_scroll_bar_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_scroll_bar_on_r_btn_up,    .on_wheel = ui_scroll_bar_on_wheel,
    .snap = ui_window_snap_base,
    .send_msg = ui_window_send_msg_base,
    .invalidate = ui_window_invalidate_base,    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .get_records = ui_window_get_records_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

static void *bitmap(const char *name)
{
    return res_mgr_get(g_resMgr(), name);
}

static void rows_destroy_range(struct EnchantRow *first, struct EnchantRow *last)
{
    for (; first != last; first++) {
        string_free(&first->item.name);
        string_free(&first->item.id_text);
    }
}

static void rows_free(struct Vec *rows)
{
    rows_destroy_range(rows->first, rows->last);
    vec_free(rows);
}

int enchant_listbox_first_row_y(struct UIEnchantList *self)
{
    int count = (int)ENCHANT_ROW_COUNT(self);

    if (count != self->full_rows && self->top == count - self->full_rows)
        return self->base.height - self->visible_rows * self->row_height;
    return 0;
}

struct RowHit {
    char hit;
    int row;
};

struct RowHit *enchant_listbox_row_at(struct UIEnchantList *self, struct RowHit *out, int y)
{
    int count = (int)ENCHANT_ROW_COUNT(self);
    int offset = y + enchant_listbox_first_row_y(self);
    int row = offset / self->row_height + self->top;

    if (offset < 0)
        row--;
    out->hit = row >= 0 && (unsigned)row < (unsigned)count;
    out->row = row;
    return out;
}

void enchant_listbox_update_scroll(struct UIEnchantList *self)
{
    int count = (int)ENCHANT_ROW_COUNT(self);
    struct UIScrollBar *bar = &self->scrollbar->bar;

    if (count > self->full_rows) {
        int top = self->top < 0 ? 0 : self->top;
        ui_window_set_show_base(&bar->base, 1);
        ui_scroll_bar_set_range(bar, count - self->full_rows, self->full_rows, count);
        if (top > count - self->full_rows)
            top = count - self->full_rows;
        self->top = top;
        ui_scroll_bar_set_pos(bar, top);
    } else {
        self->top = 0;
        ui_window_set_show_base(&bar->base, 0);
    }
    ui_window_invalidate_base(&self->base);
    if (self->base.parent)
        ui_window_invalidate(self->base.parent);
}

void enchant_listbox_select(struct UIEnchantList *self, unsigned row)
{
    if (row >= ENCHANT_ROW_COUNT(self))
        return;
    self->selected = row;
    if (self->base.parent)
        ui_window_send(self->base.parent, &self->base, ENCHANT_MSG_COMMAND, LIST_CMD_SELECTED, (int)ENCHANT_ROW(self, row), 0, 0);
    ui_window_invalidate_base(&self->base);
}

int enchant_listbox_selected_item_id(struct UIEnchantList *self)
{
    if (self->selected < 0 || ENCHANT_ROW_COUNT(self) <= (unsigned)self->selected)
        return 0;
    return atoi(string_cstr(&ENCHANT_ROW(self, self->selected)->item.id_text));
}

void enchant_listbox_destruct(struct UIEnchantList *self)
{
    self->base.ops = &enchant_listbox_ops;
    rows_free(&self->rows);
    ui_window_destruct(&self->base);
}

void enchant_target_list_destruct(struct UIEnchantList *self)
{
    self->base.ops = &enchant_target_list_ops;
    enchant_listbox_destruct(self);
}

void enchant_listbox_delete(struct UIWindow *window)
{
    struct UIEnchantList *self = (struct UIEnchantList *)window;
    enchant_listbox_destruct(self);
    mem_free(self);
}

void enchant_target_list_delete(struct UIWindow *window)
{
    struct UIEnchantList *self = (struct UIEnchantList *)window;
    enchant_target_list_destruct(self);
    mem_free(self);
}

void enchant_listbox_create(struct UIWindow *window, int width, int height)
{
    struct UIEnchantList *self = (struct UIEnchantList *)window;
    struct UIEnchantScrollBar *bar = mem_alloc(sizeof *bar);

    if (bar) {
        ui_scroll_bar_construct(&bar->bar);
        bar->bar.base.ops = &enchant_scrollbar_ops;
        bar->bar.thickness = 0x10;
    }
    self->scrollbar = bar;
    ui_scroll_bar_create(&bar->bar, width - self->scrollbar_width, 0, 1, height);
    ui_window_set_show_base(&self->scrollbar->bar.base, 0);
    ui_window_add_child(&self->base, &self->scrollbar->bar.base);
    self->full_rows = height / self->row_height;
    self->visible_rows = self->full_rows + (self->base.height % self->row_height != 0);
    enchant_listbox_update_scroll(self);
}

void enchant_listbox_pressed(struct UIWindow *window, int x, int y)
{
    struct UIEnchantList *self = (struct UIEnchantList *)window;
    struct RowHit hit;

    (void)x;
    if (self->locked == 1)
        return;
    enchant_listbox_row_at(self, &hit, y);
    if (hit.hit)
        enchant_listbox_select(self, hit.row);
}

void enchant_listbox_hover(struct UIWindow *window, int x, int y)
{
    struct UIEnchantList *self = (struct UIEnchantList *)window;
    struct RowHit hit;
    struct String name = {0};
    int sx = x, sy = y;

    enchant_listbox_row_at(self, &hit, y);
    if (!hit.hit)
        return;
    ui_window_to_screen(&self->base, &sx, &sy);
    item_info_get_display_name(&ENCHANT_ROW(self, hit.row)->item, &name);
    mode_show_tooltip(client_current_mode(), string_cstr(&name), sx + 5, sy - 5, -1, 0);
    string_free(&name);
    if (self->hover != hit.row) {
        self->hover = hit.row;
        ui_window_invalidate_base(&self->base);
    }
}

void enchant_listbox_unhover(struct UIWindow *window)
{
    struct UIEnchantList *self = (struct UIEnchantList *)window;

    self->hover = -1;
    ui_window_invalidate_base(&self->base);
}

void enchant_listbox_show_info(struct UIWindow *window, int x, int y)
{
    struct UIEnchantList *self = (struct UIEnchantList *)window;
    struct RowHit hit;
    struct UIWindow *info;

    (void)x;
    enchant_listbox_row_at(self, &hit, y);
    if (!hit.hit)
        return;
    info = ui_window_open(WID_ITEM_INFO);
    if (info)
        ui_window_send(info, &self->base, ENCHANT_MSG_SHOW_ITEM, (int)&ENCHANT_ROW(self, hit.row)->item, 0, 0, 0);
}

void enchant_listbox_wheel(struct UIWindow *window, int delta)
{
    struct UIEnchantList *self = (struct UIEnchantList *)window;

    ui_window_send(&self->base, &self->scrollbar->bar.base, delta < 0 ? LIST_MSG_SCROLL_DOWN : LIST_MSG_SCROLL_UP, 0, 0, 0, 0);
}

int enchant_listbox_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                     int wparam, int lparam, int extra1, int extra2)
{
    struct UIEnchantList *self = (struct UIEnchantList *)window;
    struct EnchantRow copy;
    int old_top;

    switch (msg) {
    case LIST_MSG_SCROLL_BY:
    case LIST_MSG_SCROLL_UP:
    case LIST_MSG_SCROLL_DOWN:
        old_top = self->top;
        self->top = msg == LIST_MSG_SCROLL_UP ? old_top - 1
                  : msg == LIST_MSG_SCROLL_DOWN ? old_top + 1 : old_top + wparam;
        if (self->top != old_top)
            enchant_listbox_update_scroll(self);
        return self->top - old_top;

    case LIST_MSG_REFRESH: {
        struct Vec old = self->rows;
        struct EnchantRow *row;
        struct ItemInfo now;

        vec_init(&self->rows);
        for (row = old.first; row != old.last; row++) {
            session_get_inventory_item(&g_session1, &now, row->item.index);
            item_info_copy(&copy.item, &now);
            copy.available = row->available;
            vec_push(&self->rows, &copy, sizeof copy);
            string_free(&now.name);
            string_free(&now.id_text);
        }
        ui_window_invalidate_base(&self->base);
        rows_free(&old);
        break;
    }

    case LIST_MSG_ADD:
        if (!wparam)
            break;
        item_info_copy(&copy.item, (const struct ItemInfo *)wparam);
        copy.available = lparam != 0;
        vec_push(&self->rows, &copy, sizeof copy);
        enchant_listbox_update_scroll(self);
        break;

    case LIST_MSG_CLEAR:
        rows_destroy_range(self->rows.first, self->rows.last);
        vec_clear(&self->rows);
        self->selected = -1;
        enchant_listbox_update_scroll(self);
        break;
    }
    return ui_window_send_msg_base(&self->base, sender, msg, wparam, lparam, extra1, extra2);
}

static int visible_row(const struct UIEnchantList *self, int y0, int i)
{
    int row = self->top + (y0 < 0 ? -1 : 0) + i;

    return row < 0 || (unsigned)row >= ENCHANT_ROW_COUNT(self) ? -1 : row;
}
static void *row_background(const struct UIEnchantList *self, int row, const char *pressed,
                            const char *over, const char *normal)
{
    return bitmap(row == self->selected ? pressed : row == self->hover ? over : normal);
}

void enchant_listbox_draw(struct UIWindow *window)
{
    struct UIEnchantList *self = (struct UIEnchantList *)window;
    struct EnchantRow copy;
    struct String fitted = {0};
    struct String name = {0};
    char path[0x100];
    void *bg;
    int y0 = enchant_listbox_first_row_y(self), i, row, y;

    ui_window_clear_canvas(&self->base, ENCHANT_COLORKEY);
    for (i = 0; i < self->visible_rows; i++) {
        if ((row = visible_row(self, y0, i)) < 0)
            continue;
        y = self->row_height * i + y0;
        bg = row_background(self, row, ENCHANT_BMP("btn_enc_item_press.bmp"),
                            ENCHANT_BMP("btn_enc_item_over.bmp"), ENCHANT_BMP("btn_enc_item.bmp"));
        if (bg)
            ui_window_draw_bitmap(&self->base, 3, y, bg, 1);
        if (ENCHANT_ROW_COUNT(self) <= (unsigned)row)
            fatal_error("invalid vector<T> subscript");

        item_info_copy(&copy.item, &ENCHANT_ROW(self, row)->item);
        copy.available = ENCHANT_ROW(self, row)->available;
        sprintf(path, ENCHANT_UI_DIR "item\\%s.bmp", item_info_get_res_name(&copy.item));
        if (copy.available == 1)
            ui_window_draw_bitmap(&self->base, 5, y + 3, bitmap(path), 1);
        else
            ui_window_draw_bitmap_tinted(&self->base, 0, 5, y + 3, bitmap(path), 1, 0);
        item_info_get_display_name(&copy.item, &name);
        ui_window_fit_text(&self->base, &fitted, self->base.width - self->scrollbar_width - 0x34,
                    string_cstr(&name), 0, 0xc, 1);
        ui_window_text_out(&self->base, 0x1f, y + 9, string_cstr(&fitted), 0, 0, 0xc,
                   copy.available == 1 ? NAME_GREEN : NAME_GREY, 1, 0);
        string_free(&fitted);
        string_free(&name);
        rows_destroy_range(&copy, &copy + 1);
    }
}

static void target_name(struct UIEnchantList *self, const struct ItemInfo *item, int x, int y,
                        unsigned color)
{
    struct String fitted = {0};

    struct String name = {0};

    item_info_get_display_name(item, &name);
    ui_window_fit_text(&self->base, &fitted, self->base.width - self->scrollbar_width - 0x34,
                string_cstr(&name), 0, 0xc, 1);
    ui_window_text_out(&self->base, x, y, string_cstr(&fitted), 0, 0, 0xc, color, 1, 0);
    string_free(&fitted);
    string_free(&name);
}

void enchant_target_list_draw(struct UIWindow *window)
{
    struct UIEnchantList *self = (struct UIEnchantList *)window;
    const struct EnchantRow *entry;
    char path[0x100];
    void *bg;
    int y0 = enchant_listbox_first_row_y(self), i, row, y, selected;

    ui_window_clear_canvas(&self->base, ENCHANT_COLORKEY);
    for (i = 0; i < self->visible_rows; i++) {
        if ((row = visible_row(self, y0, i)) < 0)
            continue;
        y = self->row_height * i + y0;
        bg = row_background(self, row, ENCHANT_BMP("bg_enc_gear_press.bmp"),
                            ENCHANT_BMP("bg_enc_gear_over.bmp"), ENCHANT_BMP("bg_enc_gear.bmp"));
        if (bg)
            ui_window_draw_bitmap(&self->base, 4, y, bg, 1);
        if (ENCHANT_ROW_COUNT(self) <= (unsigned)row)
            fatal_error("invalid vector<T> subscript");

        entry = ENCHANT_ROW(self, row);
        selected = row == self->selected;
        sprintf(path, ENCHANT_UI_DIR "item\\%s.bmp", item_info_get_res_name(&entry->item));
        ui_window_draw_bitmap(&self->base, selected ? 0xb : 0xa, y + (selected ? 0xe : 0xd), bitmap(path), 1);
        if (entry->item.grade > 0) {
            sprintf(path, ENCHANT_UI_DIR "grade_enchant\\grade_icon%d.bmp", entry->item.grade);
            ui_window_draw_bitmap(&self->base, selected ? 0xab : 0xaa, y + (selected ? 8 : 7), bitmap(path), 1);
        }
        if (selected) {
            target_name(self, &entry->item, 0x2a, y + 0x17, NAME_WHITE);
            target_name(self, &entry->item, 0x2b, y + 0x18, NAME_SHADOW);
        } else {
            target_name(self, &entry->item, 0x2a, y + 0x17, NAME_SHADOW);
        }
    }
}

struct UIEnchantList *enchant_listbox_construct(struct UIEnchantList *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &enchant_listbox_ops;
    self->scrollbar_width = 0x10;
    self->row_height = 0x1e;
    self->selected = -1;
    self->hover = -1;
    self->top = 0;
    self->scrollbar = 0;
    self->locked = 0;
    self->rows.first = self->rows.last = self->rows.end = 0;
    return self;
}
