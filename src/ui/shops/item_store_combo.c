#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include <string.h>
#include "ui/shops/shops.h"
#include "ui/controls/listbox.h"
#include "ui/shops/item_store_combo.h"
#include "diag/perf.h"

struct UIItemStoreComboBox {
    struct UIComboBoxEx combo;
    BYTE pink;
};

void ui_item_store_combo_box_destroy(struct UIWindow *window);
void ui_item_store_combo_box_on_create(struct UIWindow *self, int width, int height);
void ui_item_store_combo_box_on_draw(struct UIWindow *self);
void ui_item_store_combo_box_on_mouse_move(struct UIWindow *self, int x, int y);
void ui_item_store_combo_box_on_l_btn_up(struct UIWindow *self, int x, int y);
const struct UIListBoxExOps ui_item_store_combo_box_ops = {
    .base = {
        .destroy = ui_item_store_combo_box_destroy,
        .resize = ui_combo_box_ex_resize,
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
        .on_create = ui_item_store_combo_box_on_create,
        .on_destroy = ui_window_on_destroy_base,
        .on_process = ui_window_on_process_base,
        .on_draw = ui_item_store_combo_box_on_draw,
        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_combo_box_ignore_click,
        .on_l_btn_dbl_clk = ui_combo_box_ignore_click,
        .on_mouse_move = ui_item_store_combo_box_on_mouse_move,
        .on_mouse_hover = ui_window_on_mouse_hover_base,
        .track_mouse = ui_window_track_mouse_base,
        .on_mouse_shape = ui_window_on_mouse_shape_base,
        .on_l_btn_up = ui_item_store_combo_box_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,
        .on_wheel = ui_list_box_ex_on_wheel,
        .snap = ui_window_snap_base,
        .send_msg = ui_combo_box_ex_send_msg,
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
    .clear_items = ui_list_box_ex_clear_items,
    .update_scroll_bar = ui_combo_box_ex_update_scroll_bar,
};
void ui_item_store_combo_box_wnd_destroy(struct UIWindow *window);
void ui_item_store_combo_box_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_item_store_combo_box_wnd_on_draw(struct UIWindow *self);
int ui_item_store_combo_box_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_item_store_combo_box_wnd_ops = {
    .destroy = ui_item_store_combo_box_wnd_destroy,
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
    .on_create = ui_item_store_combo_box_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_store_combo_box_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_combo_box_def_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_item_store_combo_box_wnd_send_msg,
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
};

#define UI_ID_COMBO_BUTTON 0x109
#define UI_MSG_ROW_PICKED 0x27
#define COMBO_WND_X 0xd6

struct UIItemStoreComboBox *ui_item_store_combo_box_construct(struct UIItemStoreComboBox *self)
{
    ui_combo_box_ex_construct(&self->combo);
    self->combo.list.base.ops = &ui_item_store_combo_box_ops.base;
    self->pink = 0;
    return self;
}

void ui_item_store_combo_box_destruct(struct UIItemStoreComboBox *self)
{
    self->combo.list.base.ops = &ui_item_store_combo_box_ops.base;
    self->combo.list.base.ops = &ui_combo_box_ex_ops.base;
    ui_list_box_ex_destruct(&self->combo.list);
}

void ui_item_store_combo_box_destroy(struct UIWindow *window)
{
    struct UIItemStoreComboBox *self = (struct UIItemStoreComboBox *)window;
    ui_item_store_combo_box_destruct(self);
    mem_free(self);
}

void ui_item_store_combo_box_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemStoreComboBox *self = (struct UIItemStoreComboBox *)window;
    ui_list_box_ex_on_create(window, width, height);
    self->combo.list.line_height = 15;
}

static int row_at(const struct UIListBoxEx *list, int x, int y)
{
    int row;

    if (x < 0 || y < 0 || x >= list->base.width || y >= list->base.height)
        return -1;
    row = y / list->line_height + list->top;
    if (row < 0 || (unsigned)row >= list->rows.size)
        return -1;
    return row;
}

void ui_item_store_combo_box_on_draw(struct UIWindow *window)
{
    struct UIItemStoreComboBox *self = (struct UIItemStoreComboBox *)window;
    struct UIListBoxEx *list = &self->combo.list;
    struct UIWindow *wnd = &list->base;
    int top = list->top, end, i;
    unsigned shown = wnd->height / list->line_height - list->extra_lines + top;

    if (shown >= list->rows.size)
        shown = list->rows.size;
    end = (int)shown;

    if (list->selected >= top && list->selected < end) {
        char path[MAX_PATH];

        if (self->pink == 0)
            sprintf(path, UI_SKIN "basic_interface\\combobox_blue%d.bmp", list->selected);
        else if (self->pink == 1)
            sprintf(path, UI_SKIN "basic_interface\\combobox_pink%d.bmp", list->selected);
        ui_window_draw_bitmap(wnd, 0, 0, ui_skin_bitmap(path), 0);
    }

    for (i = top; i < end; i++) {
        struct UIListBoxExRow *entry = list_box_ex_entry_at(&list->rows, &i);

        ui_window_text_out(wnd, 3 - list->scroll_x, (i - top) * list->line_height + 2,
                         string_cstr(&entry->text), 0, 0, 12, entry->colour, 0, 0);
    }
    ui_window_invalidate_base(wnd);
}

void ui_item_store_combo_box_on_mouse_move(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreComboBox *self = (struct UIItemStoreComboBox *)window;
    struct UIListBoxEx *list = &self->combo.list;
    int old = list->selected;

    list->selected = row_at(list, x, y);
    if (list->selected != old)
        ui_window_invalidate_base(&list->base);
}

void ui_item_store_combo_box_on_l_btn_up(struct UIWindow *window, int x, int y)
{
    struct UIItemStoreComboBox *self = (struct UIItemStoreComboBox *)window;
    struct UIListBoxEx *list = &self->combo.list;

    list->selected = row_at(list, x, y);
    if (list->base.parent)
        ui_window_send_msg(list->base.parent, NULL, UI_MSG_ROW_PICKED, list->selected, 0, 0, 0);
}

struct UIItemStoreComboBoxWnd *ui_item_store_combo_box_wnd_construct(struct UIItemStoreComboBoxWnd *self)
{
    ui_combo_box_def_wnd_construct(&self->def.base);
    self->def.base.ops = &ui_item_store_combo_box_wnd_ops;
    self->pink = 0;
    return self;
}

void ui_item_store_combo_box_wnd_destruct(struct UIItemStoreComboBoxWnd *self)
{
    self->def.base.ops = &ui_item_store_combo_box_wnd_ops;
    self->def.base.ops = &ui_combo_box_def_wnd_ops;
    ui_window_destruct(&self->def.base);
}

void ui_item_store_combo_box_wnd_destroy(struct UIWindow *window)
{
    struct UIItemStoreComboBoxWnd *self = (struct UIItemStoreComboBoxWnd *)window;
    ui_item_store_combo_box_wnd_destruct(self);
    mem_free(self);
}

void ui_item_store_combo_box_wnd_on_create(struct UIWindow *window, int width_unused,
                                                int height_unused)
{
    struct UIItemStoreComboBoxWnd *self = (struct UIItemStoreComboBoxWnd *)window;
    struct UIWindow *wnd = &self->def.base;
    struct UIBitmapButton *button;
    struct UIItemStoreComboBox *list;
    struct UIWindow *label;
    char text[12] = {0};

    button = mem_alloc(sizeof *button);
    if (button)
        button = ui_bitmap_button_construct(button);
    self->def.button = button;
    ui_bitmap_button_set_bitmap(button, UI_SKIN "basic_interface\\combo_btn_a.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button,
                             self->pink == 1 ? UI_SKIN "basic_interface\\combo_btn_pink_b.bmp"
                                             : UI_SKIN "basic_interface\\combo_btn_b.bmp",
                             UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button,
                             self->pink == 1 ? UI_SKIN "basic_interface\\combo_btn_pink_c.bmp"
                                             : UI_SKIN "basic_interface\\combo_btn_c.bmp",
                             UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, wnd->width - button->bitmap_width, wnd->height - button->bitmap_height);
    button->button.base.id = UI_ID_COMBO_BUTTON;
    ui_window_add_child(wnd, &button->button.base);

    list = mem_alloc(sizeof *list);
    if (list)
        list = ui_item_store_combo_box_construct(list);
    self->def.list = &list->combo.list.base;
    list->pink = self->pink != 0;
    ui_window_create(&list->combo.list.base, 0x3e, 0x2d);
    ui_window_move_base(&list->combo.list.base, 0, 0);
    ui_window_set_show_base(&list->combo.list.base, 0);
    ui_window_add_child(wnd, &list->combo.list.base);

    sprintf(text, "(NO SELECT)");
    label = mem_alloc(sizeof(struct UIStaticText));
    if (label)
        label = ui_static_text_construct(label);
    self->def.label = label;
    ui_window_create(label, wnd->width - self->def.button->bitmap_width, self->def.button->bitmap_height);
    ui_static_text_set_text(label, text, 0);
    ui_window_move(label, 2, 3);
    ((struct UIStaticText *)label)->white_shadow = 1;
    ui_window_add_child(wnd, label);
}

void ui_item_store_combo_box_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemStoreComboBoxWnd *self = (struct UIItemStoreComboBoxWnd *)window;
    struct UIWindow *wnd = &self->def.base;
    struct BitmapRes *bitmap = ui_skin_bitmap(UI_SKIN "basic_interface\\bg_setting.bmp");

    ui_window_draw_bitmap(wnd, 0, wnd->height - bitmap->height, bitmap, 0);
}

static void toggle_list(struct UIItemStoreComboBoxWnd *self)
{
    struct UIWindow *wnd = &self->def.base;
    struct UIWindow *list = self->def.list;
    struct UIBitmapButton *button = self->def.button;
    BYTE was_open = self->def.open;

    self->def.open = !was_open;
    ui_window_set_show(list, !was_open);
    if (!was_open) {
        ui_window_resize_base(wnd, wnd->width, list->height + wnd->height);
        ui_window_move_base(wnd, COMBO_WND_X, wnd->y - list->height);
        ui_window_move_base(&button->button.base, wnd->width - button->bitmap_width, wnd->height - button->bitmap_height);
        ui_window_move(self->def.label, 2, list->height + 3);
    } else {
        ui_window_resize_base(wnd, wnd->width, wnd->height - list->height);
        ui_window_move_base(wnd, COMBO_WND_X, list->height + wnd->y);
        ui_window_move_base(&button->button.base, wnd->width - button->bitmap_width, 0);
        ui_window_move(self->def.label, 2, 3);
    }
    ui_window_invalidate(wnd->parent);
    ui_window_invalidate(list);
    ui_window_invalidate_base(wnd);
}

int ui_item_store_combo_box_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                              int val1, int val2, int val3, int val4)
{
    struct UIItemStoreComboBoxWnd *self = (struct UIItemStoreComboBoxWnd *)window;
    struct UIWindow *wnd = &self->def.base;

    if (msg == UI_MSG_COMMAND) {
        if (val1 == UI_ID_COMBO_BUTTON)
            toggle_list(self);
        return 0;
    }
    if (msg != UI_MSG_ROW_PICKED)
        return ui_window_send_msg_base(wnd, sender, msg, val1, val2, val3, val4);
    if (val1 >= 0) {
        int row = val1;
        struct UIListBoxExRow *entry = list_box_ex_entry_at(&((struct UIListBoxEx *)self->def.list)->rows, &row);

        ui_static_text_set_text(self->def.label, string_cstr(&entry->text), 0);
        ui_window_send_msg(wnd, NULL, UI_MSG_COMMAND, UI_ID_COMBO_BUTTON, 0, 0, 0);
        ui_window_send_msg(wnd->parent, wnd, UI_MSG_ROW_PICKED, val1, 0, 0, 0);
    }
    return 0;
}
