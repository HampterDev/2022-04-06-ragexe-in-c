#include "world/tooltip.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contents/client_res.h"
#include "contents/enchant_ui.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "ui/controls/buttons.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/scroll.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "base.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"
#include "contents/ui_window.h"
#include "contents/ui_window_mgr.h"
#include "core/msg_str.h"
#include "diag/perf.h"

#define ENCHANT_MSG_COMMAND         6
#define ENCHANT_MSG_TAB             0x16
#define ENCHANT_MSG_SHOW_ITEM       0x18
#define ENCHANT_MSG_TAB_DESELECTED  0x75
#define ENCHANT_MSG_TAB_SELECTED    0x76

#define ENCHANT_CMD_ITEM_PRESSED    0xc4

#define WID_ITEM_INFO               0xc

#define INK         0x965532
#define TAB_GREY    0xa0a0a0
#define LABEL_WHITE 0xf0f0f0
#define LABEL_GREY  0x646464

void enchant_tab_delete(struct UIWindow *window);
void enchant_tab_draw(struct UIWindow *self);
void enchant_tab_pressed(struct UIWindow *self, int x, int y);
int enchant_tab_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int wparam, int lparam, int extra1, int extra2);

const struct UIWindowOps enchant_tab_ops = {
    .destroy = enchant_tab_delete,
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
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = enchant_tab_draw,    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = enchant_tab_pressed,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = enchant_tab_send_msg,
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
void enchant_group_delete(struct UIWindow *window);
void enchant_group_draw(struct UIWindow *self);
int enchant_group_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int wparam, int lparam, int extra1, int extra2);

const struct UIWindowOps enchant_group_ops = {
    .destroy = enchant_group_delete,
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
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = enchant_group_draw,    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = enchant_group_send_msg,
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
void enchant_item_delete(struct UIWindow *window);
void enchant_item_draw(struct UIWindow *self);
void enchant_item_hover(struct UIWindow *self, int x, int y);
void enchant_item_pressed(struct UIWindow *self, int x, int y);
void enchant_item_show_info(struct UIWindow *self, int x, int y);

const struct UIWindowOps enchant_item_ops = {
    .destroy = enchant_item_delete,
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
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = enchant_item_draw,    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = enchant_item_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = enchant_item_pressed,
    .on_r_btn_down = enchant_item_show_info,
    .on_r_btn_up = ui_window_on_r_btn_up_base,    .on_wheel = ui_window_on_wheel_base,
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

void enchant_tab_destruct(struct UIEnchantTab *self)
{
    self->base.ops = &enchant_tab_ops;
    string_free(&self->label);
    ui_window_destruct(&self->base);
}

void enchant_tab_delete(struct UIWindow *window)
{
    struct UIEnchantTab *self = (struct UIEnchantTab *)window;
    enchant_tab_destruct(self);
    mem_free(self);
}

void enchant_tab_draw(struct UIWindow *window)
{
    struct UIEnchantTab *self = (struct UIEnchantTab *)window;

    ui_window_clear_canvas(&self->base, ENCHANT_COLORKEY);
    if (self->selected == 1)
        ui_window_text_out_centred(&self->base, 0, 5, string_cstr(&self->label), self->base.width, 0, 0xc, INK, 1);
    else
        ui_window_text_out_centred(&self->base, 0, 6, string_cstr(&self->label), self->base.width, 0, 0xc, TAB_GREY, 1);
}

void enchant_tab_pressed(struct UIWindow *window, int x, int y)
{
    struct UIEnchantTab *self = (struct UIEnchantTab *)window;
     (void)x; (void)y;
    if (self->base.parent)
        ui_window_send(self->base.parent, &self->base, ENCHANT_MSG_TAB, self->base.id, 0, 0, 0);
}

int enchant_tab_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                    int wparam, int lparam, int extra1, int extra2)
{
    struct UIEnchantTab *self = (struct UIEnchantTab *)window;
    switch (msg) {
    case ENCHANT_MSG_TAB_DESELECTED:
        self->selected = 0;
        ui_window_invalidate_base(&self->base);
        return 0;
    case ENCHANT_MSG_TAB_SELECTED:
        self->selected = 1;
        ui_window_invalidate_base(&self->base);
        return 0;
    }
    return ui_window_send_msg_base(&self->base, sender, msg, wparam, lparam, extra1, extra2);
}

void enchant_group_destruct(struct UIEnchantGroup *self)
{
    self->base.ops = &enchant_group_ops;
    ui_window_destruct(&self->base);
}

void enchant_group_delete(struct UIWindow *window)
{
    struct UIEnchantGroup *self = (struct UIEnchantGroup *)window;
    enchant_group_destruct(self);
    mem_free(self);
}

void enchant_group_draw(struct UIWindow *window)
{
    struct UIEnchantGroup *self = (struct UIEnchantGroup *)window;
    char path[0x100];

    if ((unsigned)self->page > 3) {
        ui_window_clear_canvas(&self->base, ENCHANT_COLORKEY);
        return;
    }
    sprintf(path, ENCHANT_BMP("bg_enc_menu%d.bmp"), self->page);
    ui_window_draw_bitmap(&self->base, 0, 0, bitmap(path), 0);
}

int enchant_group_send_msg(struct UIWindow *base_window, struct UIWindow *sender, int msg,
                                      int wparam, int lparam, int extra1, int extra2)
{
    struct UIEnchantGroup *self = (struct UIEnchantGroup *)base_window;
    struct List children;
    struct ListNode *node;

    if (msg != ENCHANT_MSG_TAB || self->locked == 1)
        return ui_window_send_msg_base(&self->base, sender, msg, wparam, lparam, extra1, extra2);

    ui_window_list_copy(&children, &self->base.children);
    for (node = children.head->next; node != children.head; node = node->next) {
        struct UIWindow *tab = *LIST_VALUE(node, struct UIWindow *);
        if (tab == sender) {
            self->page = tab->id;
            ui_window_send(tab, &self->base, ENCHANT_MSG_TAB_SELECTED, 0, 0, 0, 0);
        } else {
            ui_window_send(tab, &self->base, ENCHANT_MSG_TAB_DESELECTED, 0, 0, 0, 0);
        }
    }
    if (self->base.parent)
        ui_window_send(self->base.parent, sender, ENCHANT_MSG_TAB, self->page, 0, 0, 0);
    ui_window_invalidate_base(&self->base);
    list_free(&children, NULL);
    return 0;
}

void enchant_group_select(struct UIEnchantGroup *self, int page)
{
    struct List children;
    struct ListNode *node;

    ui_window_list_copy(&children, &self->base.children);
    for (node = children.head->next; node != children.head; node = node->next) {
        struct UIWindow *tab = *LIST_VALUE(node, struct UIWindow *);

        if ((int)tab->id == page) {
            ui_window_send(&self->base, tab, ENCHANT_MSG_TAB, 0, 0, 0, 0);
            break;
        }
    }
    list_free(&children, NULL);
}

void enchant_button_delete(struct UIWindow *window)
{
    struct UIEnchantButton *self = (struct UIEnchantButton *)window;

    ui_button_destruct(&self->button);
    mem_free(self);
}

void enchant_button_draw(struct UIWindow *window)
{
    struct UIEnchantButton *self = (struct UIEnchantButton *)window;
    struct String label;
    const char *text;
    int y;

    string_init(&label);
    if ((unsigned)self->button.cmd_value <= 3) {
        const char *s = msg_str(0xefe + self->button.cmd_value);
        string_assign_cstr(&label, s);
    }
    text = string_cstr(&label);
    y = (int)((float)(self->button.base.height - ui_window_text_height(&self->button.base, text, 0, 0, 0x14, 1, 0)) * 0.5f);

    if (self->button.disabled == 1) {
        ui_window_draw_bitmap(&self->button.base, 0, 0, bitmap(ENCHANT_BMP("btn_enc_dis.bmp")), 0);
        ui_window_text_out_centred_outlined(&self->button.base, 0, y, string_cstr(&label), self->button.base.width, 0,
                             LABEL_GREY, INK, 0, 0x14, 1);
    } else {
        switch (self->button.base.state) {
        case UI_BTN_UP:
            ui_window_draw_bitmap(&self->button.base, 0, 0, bitmap(ENCHANT_BMP("btn_enc.bmp")), 0);
            ui_window_text_out_centred_outlined(&self->button.base, 0, y, string_cstr(&label), self->button.base.width, 0,
                                 LABEL_WHITE, INK, 0, 0x14, 1);
            break;
        case UI_BTN_HOVER:
            ui_window_draw_bitmap(&self->button.base, 0, 0, bitmap(ENCHANT_BMP("btn_enc_over.bmp")), 0);
            ui_window_text_out_centred_outlined(&self->button.base, 0, y, string_cstr(&label), self->button.base.width, 0,
                                 LABEL_WHITE, INK, 0, 0x14, 1);
            break;
        case UI_BTN_DOWN:
            ui_window_draw_bitmap(&self->button.base, 0, 0, bitmap(ENCHANT_BMP("btn_enc_press.bmp")), 0);
            ui_window_text_out_centred_outlined(&self->button.base, 1, y + 1, string_cstr(&label), self->button.base.width, 0,
                                 LABEL_WHITE, INK, 0, 0x14, 1);
            break;
        }
    }
    string_free(&label);
}

void enchant_item_destruct(struct UIEnchantItem *self)
{
    self->base.ops = &enchant_item_ops;
    string_free(&self->item.name);
    string_free(&self->item.id_text);
    ui_window_destruct(&self->base);
}

void enchant_item_delete(struct UIWindow *window)
{
    struct UIEnchantItem *self = (struct UIEnchantItem *)window;
    enchant_item_destruct(self);
    mem_free(self);
}

static int item_id(const struct UIEnchantItem *self)
{
    return atoi(string_cstr(&self->item.id_text));
}

int enchant_item_cell_id(struct UIEnchantItem *self)
{
    return item_id(self);
}

void enchant_item_draw(struct UIWindow *window)
{
    struct UIEnchantItem *self = (struct UIEnchantItem *)window;
    char path[0x100];

    ui_window_clear_canvas(&self->base, ENCHANT_COLORKEY);
    if (item_id(self) <= 0)
        return;
    if (self->large == 1)
        sprintf(path, ENCHANT_UI_DIR "collection\\%s.bmp", item_info_get_res_name(&self->item));
    else
        sprintf(path, ENCHANT_UI_DIR "item\\%s.bmp", item_info_get_res_name(&self->item));
    ui_window_draw_bitmap(&self->base, 0, 0, bitmap(path), 1);
}

void enchant_item_pressed(struct UIWindow *window, int x, int y)
{
    struct UIEnchantItem *self = (struct UIEnchantItem *)window;
     (void)x; (void)y;
    if (self->locked != 1 && self->base.parent)
        ui_window_send(self->base.parent, &self->base, ENCHANT_MSG_COMMAND, ENCHANT_CMD_ITEM_PRESSED, 0, 0, 0);
}

void enchant_item_hover(struct UIWindow *window, int x, int y)
{
    struct UIEnchantItem *self = (struct UIEnchantItem *)window;
    struct String name = {0};

    if (item_id(self) <= 0)
        return;
    ui_window_to_screen(&self->base, &x, &y);
    item_info_get_display_name(&self->item, &name);
    mode_show_tooltip(client_current_mode(), string_cstr(&name), x + 5, y - 5, -1, 0);
    string_free(&name);
}

void enchant_item_show_info(struct UIWindow *window, int x, int y)
{
    struct UIEnchantItem *self = (struct UIEnchantItem *)window;
    struct UIWindow *info;

     (void)x; (void)y;
    if (item_id(self) <= 0)
        return;
    info = ui_window_open(WID_ITEM_INFO);
    if (info)
        ui_window_send(info, NULL, ENCHANT_MSG_SHOW_ITEM, (int)&self->item, 0, 0, 0);
}

void enchant_scrollbar_delete(struct UIWindow *window)
{
    struct UIEnchantScrollBar *self = (struct UIEnchantScrollBar *)window;

    ui_scroll_bar_destruct(&self->bar);
    mem_free(self);
}

void enchant_scrollbar_draw(struct UIWindow *window)
{
    struct UIEnchantScrollBar *self = (struct UIEnchantScrollBar *)window;
    struct BitmapRes *up, *down, *track, *thumb;
    int height = self->bar.base.height, range;
    __int64 i;

    ui_window_clear_canvas(&self->bar.base, ENCHANT_COLORKEY);
    if (!self->bar.max_pos)
        return;
    if (self->bar.orientation == 1) {
        up = bitmap(ENCHANT_BMP("scroll_arrow_up.bmp"));
        down = bitmap(ENCHANT_BMP("scroll_arrow_dowm.bmp"));
        track = bitmap(ENCHANT_BMP("scroll_bg.bmp"));
        if (up && down && track) {
            ui_window_draw_bitmap(&self->bar.base, 0, 0, up, 1);
            for (i = 0; i < (height - up->height - down->height) / track->height; i++)
                ui_window_draw_bitmap(&self->bar.base, 0, (int)i * track->height + up->height, track, 1);
            ui_window_draw_bitmap(&self->bar.base, 0, height - track->height - down->height, track, 1);
            ui_window_draw_bitmap(&self->bar.base, 0, height - down->height, down, 1);

            thumb = bitmap(ENCHANT_BMP("scroll_btn.bmp"));
            range = self->bar.total - self->bar.page;
            if (range < 0)
                range = 0;
            ui_window_draw_bitmap(&self->bar.base, 0,
                       (int)((double)(height - thumb->height - down->height - up->height) / (double)range
                             * (double)self->bar.pos + (double)up->height),
                       thumb, 1);
        }
    }
    self->bar.min_thumb = 0;
    ui_window_invalidate(self->bar.base.parent);
}
