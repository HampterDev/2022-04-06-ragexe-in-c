#include "ui/controls/buttons.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include <stdio.h>
#include <stdlib.h>
#include "contents/client_res.h"
#include "contents/enchant_mgr.h"
#include "contents/enchant_ui.h"
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "std/std.h"
#include "std/tree.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/scroll.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "base.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"
#include "contents/ui_window.h"
#include "diag/perf.h"
#include "session/session_items.h"

#define ENCHANT_MSG_COMMAND         6
#define INFO_CMD_ENCHANT            0xb0
#define ENCHANT_CMD_ITEM_PRESSED    0xc4
#define LIST_CMD_SELECTED           0x148
#define LIST_MSG_ADD                0x1f
#define LIST_MSG_CLEAR              0x4b

#define INK         0x965532
#define LABEL_WHITE 0xf0f0f0
#define SHORT_RED   0x5a3ce6

#define SOCKETS     4
#define MATERIALS   8

void enchant_info_delete(struct UIWindow *window);
void enchant_info_create(struct UIWindow *self, int width, int height);
void enchant_info_draw(struct UIWindow *self);
int enchant_info_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int wparam, int lparam, int extra1, int extra2);

const struct UIWindowOps enchant_info_ops = {
    .destroy = enchant_info_delete,
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
    .on_create = enchant_info_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = enchant_info_draw,    .on_focus = ui_window_on_focus_base,
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
    .send_msg = enchant_info_send_msg,
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

const struct UIButtonOps enchant_button_ops = {
    .base = {
        .destroy = enchant_button_delete,
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
        .on_draw = enchant_button_draw,        .on_focus = ui_window_on_focus_base,
        .on_focus_lost = ui_window_on_focus_lost_base,
        .on_l_btn_down = ui_button_on_l_btn_down,
        .on_l_btn_dbl_clk = ui_button_on_l_btn_dbl_clk,
        .on_mouse_move = ui_button_on_mouse_move,
        .on_mouse_hover = ui_button_on_mouse_hover,
        .track_mouse = ui_button_track_mouse,
        .on_mouse_shape = ui_button_on_mouse_shape,
        .on_l_btn_up = ui_button_on_l_btn_up,
        .on_r_btn_down = ui_window_on_r_btn_down_base,
        .on_r_btn_up = ui_window_on_r_btn_up_base,        .on_wheel = ui_window_on_wheel_base,
        .snap = ui_window_snap_base,
        .send_msg = ui_window_send_msg_base,
        .invalidate = ui_window_invalidate_base,        .render = ui_window_render_base,
        .has_custom_backdrop = 0,
        .draw_backdrop = ui_window_draw_backdrop_base,
        .get_records = ui_window_get_records_base,
        .type = UI_TYPE_WINDOW,
        .on_hover_enter = ui_window_on_hover_enter_base,
        .on_hover_leave = ui_window_on_hover_leave_base,
        .make_quad = ui_window_make_quad_base,
        .hit_test = ui_window_hit_test_base,
        .render_overlay = ui_window_render_overlay_base,
    },
    .set_show = ui_button_set_show,
};

static const int socket_pos[SOCKETS][2]   = { { 0x4b, 0x37 }, { 0x62, 0x74 }, { 0xb1, 0x74 }, { 0xc8, 0x37 } };
static const int label_pos[SOCKETS][2]    = { { 6, 6 }, { 0x1d, 0x95 }, { 0xae, 0x95 }, { 0xc5, 6 } };
static const int label_align[SOCKETS][2]  = { { 1, 5 }, { 1, 3 }, { 0, 3 }, { 0, 5 } };
static const int socket_bmp_pos[SOCKETS][2] = { { 6, 0xa }, { 0x1d, 0x69 }, { 0xa4, 0x69 }, { 0xbb, 0xa } };

static void *bitmap(const char *name)
{
    return res_mgr_get(g_resMgr(), name);
}

static struct UIEnchantItem *material(const struct UIEnchantInfo *self, unsigned i)
{
    return ((struct UIEnchantItem **)self->materials.first)[i];
}

static struct UIEnchantItem **socket_at(struct UIEnchantInfo *self, unsigned slot)
{
    struct EnchantSocket *socket = tree_get(&self->sockets, &slot, sizeof slot, sizeof *socket, tree_less_uint);

    return &socket->item;
}

static struct UITagTextViewer **label_at(struct UIEnchantInfo *self, unsigned slot)
{
    struct EnchantSocketLabel *label = tree_get(&self->socket_labels, &slot, sizeof slot, sizeof *label,
                                                tree_less_uint);

    return &label->label;
}

static void tag_text_clear(struct UITagTextViewer *label)
{
    ui_tag_text_viewer_remove_all(label);
}

void enchant_info_destruct(struct UIEnchantInfo *self)
{
    self->base.ops = &enchant_info_ops;
    vec_free(&self->materials);
    tree_free(&self->socket_labels, NULL);
    tree_free(&self->sockets, NULL);
    ui_window_destruct(&self->base);
}

void enchant_info_delete(struct UIWindow *window)
{
    struct UIEnchantInfo *self = (struct UIEnchantInfo *)window;
    enchant_info_destruct(self);
    mem_free(self);
}

static struct UIEnchantItem *new_item(char large, char locked)
{
    struct UIEnchantItem *item = mem_alloc(sizeof *item);

    if (item) {
        ui_window_construct(&item->base, 0);
        item->base.ops = &enchant_item_ops;
        item->large = large;
        item->locked = locked;
        item_info_construct(&item->item);
    }
    return item;
}

void enchant_info_create(struct UIWindow *window, int width, int height)
{
    struct UIEnchantInfo *self = (struct UIEnchantInfo *)window;
    struct UIEnchantItem *cell;
    struct UITagTextViewer *label;
    struct UIEnchantButton *button;
    struct UIEnchantList *list;
    unsigned slot, i;

     (void)width; (void)height;
    self->target = new_item(1, 1);
    ui_window_create(&self->target->base, 0x4b, 0x64);
    ui_window_move_base(&self->target->base, 0x70, 5);
    ui_window_add_child(&self->base, &self->target->base);

    for (slot = 0; slot < SOCKETS; slot++) {
        cell = new_item(0, 1);
        ui_window_create(&cell->base, 0x18, 0x18);
        ui_window_move_base(&cell->base, socket_pos[slot][0], socket_pos[slot][1]);
        ui_window_add_child(&self->base, &cell->base);
        *socket_at(self, slot) = cell;

        label = ui_tag_text_viewer_create(0x60, 0x28);
        ui_window_move_base(&label->base, label_pos[slot][0], label_pos[slot][1]);
        label->style.colour = INK;
        label->style.font_height = 0xc;
        label->style.bold = 1;
        label->bg_argb = ENCHANT_COLORKEY;
        ui_window_invalidate_base(&label->base);
        label->wrap_width = label->base.width;
        label->line_spacing = 1;
        label->margin_x = 0;
        label->margin_y = 0;
        ui_tag_text_viewer_set_align(label, (unsigned char)label_align[slot][0]);
        ui_tag_text_viewer_set_align(label, (unsigned char)label_align[slot][1]);
        ui_window_add_child(&self->base, &label->base);
        *label_at(self, slot) = label;
    }

    for (i = 0; i < MATERIALS; i++) {
        cell = new_item(0, 1);
        ui_window_create(&cell->base, 0x18, 0x18);
        ui_window_move_base(&cell->base, (i & 3) * 0x4a + 0xa, (i >> 2) * 0x34 + 0xd2);
        ui_window_add_child(&self->base, &cell->base);
        vec_push(&self->materials, &cell, sizeof(struct UIEnchantItem *));
    }

    button = mem_alloc(sizeof *button);
    if (button) {
        ui_button_construct(&button->button);
        button->button.base.ops = &enchant_button_ops.base;
    }
    self->button = button;
    ui_window_create(&self->button->button.base, 0x8c, 0x28);
    ui_window_move_base(&self->button->button.base, 0x50, 0x14a);
    self->button->button.base.id = INFO_CMD_ENCHANT;
    self->button->button.cmd_value = self->base.id;
    self->button->button.disabled = 1;
    ui_window_invalidate_base(&self->button->button.base);
    ui_window_add_child(&self->base, &self->button->button.base);

    list = mem_alloc(sizeof *list);
    if (list)
        list = enchant_listbox_construct(list);
    self->results = list;
    ui_window_create(&self->results->base, 0xa9, 0x179);
    ui_window_move_base(&self->results->base, 0x12c, 0);
    ui_window_add_child(&self->base, &self->results->base);
}

void enchant_info_draw(struct UIWindow *window)
{
    struct UIEnchantInfo *self = (struct UIEnchantInfo *)window;
    char text[0x100];
    unsigned slot, i;

    ui_window_clear_canvas(&self->base, ENCHANT_COLORKEY);

    for (slot = 0; slot < SOCKETS; slot++) {
        if (atoi(string_cstr(&(*socket_at(self, slot))->item.id_text)) <= 0)
            continue;
        sprintf(text, ENCHANT_BMP("btn_socket%u.bmp"), slot);
        ui_window_draw_bitmap(&self->base, socket_bmp_pos[slot][0], socket_bmp_pos[slot][1], bitmap(text), 1);
    }

    for (i = 0; i < MATERIALS; i++) {
        struct UIEnchantItem *cell = material(self, i);
        unsigned have, need;
        int x, y;

        if (atoi(string_cstr(&cell->item.id_text)) <= 0)
            continue;
        have = session_inventory_count(&g_session1, atoi(string_cstr(&cell->item.id_text)));
        need = cell->item.count;
        x = (i & 3) * 0x4a;
        y = (i >> 2) * 0x34;
        if (have >= need) {
            ui_window_draw_bitmap(&self->base, x + 4, y + 0xc8, bitmap(ENCHANT_BMP("bg_enc_on.bmp")), 0);
            sprintf(text, "%u", have);
            ui_window_text_out_centred_outlined(&self->base, x + 0x22, y + 0xda, text, 0x28, 0,
                                 LABEL_WHITE, INK, 0, 0xc, 1);
            sprintf(text, "/%u", need);
            ui_window_text_out_centred_outlined(&self->base, x + 0x22, y + 0xe8, text, 0x28, 0,
                                 LABEL_WHITE, INK, 0, 0xc, 1);
        } else {
            ui_window_draw_bitmap(&self->base, x + 4, y + 0xc8, bitmap(ENCHANT_BMP("bg_enc_off.bmp")), 0);
            sprintf(text, "%u", have);
            ui_window_text_out_centred(&self->base, x + 0x22, y + 0xda, text, 0x28, 0, 0xc, SHORT_RED, 1);
            sprintf(text, "/%u", need);
            ui_window_text_out_centred(&self->base, x + 0x22, y + 0xe8, text, 0x28, 0, 0xc, SHORT_RED, 1);
        }
    }

    sprintf(text, "%llu", self->zeny);
    ui_window_text_out_right(&self->base, 0xfa, 0x136, text, 0, 0, 0xe, 0, 1);
}

int enchant_info_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                     int wparam, int lparam, int extra1, int extra2)
{
    struct UIEnchantInfo *self = (struct UIEnchantInfo *)window;
    struct TreeNode *node;
    struct EnchantSocket *socket;

    if (msg != ENCHANT_MSG_COMMAND)
        return ui_window_send_msg_base(&self->base, sender, msg, wparam, lparam, extra1, extra2);

    switch (wparam) {
    case INFO_CMD_ENCHANT:
        if (self->base.parent)
            ui_window_send(self->base.parent, &self->base, ENCHANT_MSG_COMMAND, INFO_CMD_ENCHANT, 0, 0, 0);
        break;
    case ENCHANT_CMD_ITEM_PRESSED:
        for (node = tree_first(&self->sockets); node != self->sockets.head; node = tree_next(node))
            if ((struct UIWindow *)TREE_VALUE(node, struct EnchantSocket)->item == sender)
                break;
        if (node != self->sockets.head && self->base.parent) {
            socket = TREE_VALUE(node, struct EnchantSocket);
            ui_window_send(self->base.parent, &self->base, ENCHANT_MSG_COMMAND, ENCHANT_CMD_ITEM_PRESSED, socket->slot, enchant_item_cell_id(socket->item), 0);
        }
        break;
    case LIST_CMD_SELECTED:
        if (self->base.parent)
            ui_window_send(self->base.parent, &self->base, ENCHANT_MSG_COMMAND, LIST_CMD_SELECTED, lparam, 0, 0);
        break;
    }
    return 0;
}

void enchant_info_set_target(struct UIEnchantInfo *self, const struct ItemInfo *item)
{
    struct ItemInfo socket_item;
    struct String name = {0};
    struct UIEnchantItem *socket;
    unsigned slot;

    if (!item)
        return;
    item_info_assign(&self->target->item, item);
    ui_window_invalidate_base(&self->target->base);
    for (slot = 0; slot < self->sockets.size; slot++) {
        const char *text;

        item_info_construct(&socket_item);
        item_info_set_item_id(&socket_item, (int)slot < SOCKETS ? item->slot[slot] : 0);
        socket_item.identified = 1;
        socket = *socket_at(self, slot);
        item_info_assign(&socket->item, &socket_item);
        ui_window_invalidate_base(&socket->base);
        tag_text_clear(*label_at(self, slot));
        if (atoi(string_cstr(&socket_item.id_text)) > 0) {
            item_info_get_display_name(&socket_item, &name);
            text = string_cstr(&name);
        } else {
            string_clear(&name);
            text = "";
        }
        ui_tag_text_viewer_append_text(*label_at(self, slot), text);
        string_free(&name);
        item_info_destruct(&socket_item);
    }
    enchant_info_clear_materials(self);
    ui_window_send(&self->results->base, &self->base, LIST_MSG_CLEAR, 0, 0, 0, 0);
    self->enchant_slot = -1;
    ui_window_invalidate_base(&self->base);
    ui_window_invalidate_base(&self->base);
}

void enchant_info_set_materials(struct UIEnchantInfo *self, const struct Tree *needs,
                                           unsigned __int64 zeny)
{
    struct ItemInfo need;
    struct TreeNode *node;
    unsigned i = 0;

    enchant_info_clear_materials(self);
    for (node = tree_first(needs); node != needs->head; node = tree_next(node), i++) {
        const struct ItemCount *count = TREE_VALUE(node, const struct ItemCount);
        struct UIEnchantItem *cell;

        if (i >= VEC_COUNT(&self->materials, struct UIEnchantItem *))
            break;
        item_info_construct(&need);
        item_info_set_item_id(&need, count->item_id);
        need.identified = 1;
        need.count = count->count;
        cell = material(self, i);
        item_info_assign(&cell->item, &need);
        ui_window_invalidate_base(&cell->base);
        item_info_destruct(&need);
    }
    self->zeny = zeny;
    ui_window_invalidate_base(&self->base);
}

void enchant_info_set_results(struct UIEnchantInfo *self, const struct Vec *results,
                                         char lock)
{
    const struct EnchantResult *r;
    struct ItemInfo row;

    ui_window_send(&self->results->base, &self->base, LIST_MSG_CLEAR, 0, 0, 0, 0);
    for (r = results->first; r != results->last; r++) {
        item_info_construct(&row);
        item_info_set_item_id(&row, r->item_id);
        row.identified = 1;
        ui_window_send(&self->results->base, &self->base, LIST_MSG_ADD, (int)&row, (unsigned char)r->available, 0, 0);
        item_info_destruct(&row);
    }
    self->results->locked = lock;
    ui_window_invalidate_base(&self->results->base);
    ui_window_invalidate_base(&self->base);
}

void enchant_info_clear_materials(struct UIEnchantInfo *self)
{
    struct UIEnchantItem **cell;
    struct ItemInfo empty;

    for (cell = self->materials.first; cell != self->materials.last; cell++) {
        item_info_construct(&empty);
        item_info_assign(&(*cell)->item, &empty);
        ui_window_invalidate_base(&(*cell)->base);
        item_info_destruct(&empty);
    }
    self->zeny = 0;
    self->button->button.disabled = 1;
    ui_window_invalidate_base(&self->button->button.base);
}

void enchant_info_update_button(struct UIEnchantInfo *self)
{
    struct UIEnchantItem **cell;
    char disabled = 0;

    for (cell = self->materials.first; cell != self->materials.last; cell++) {
        int id = atoi(string_cstr(&(*cell)->item.id_text));
        unsigned need = (*cell)->item.count;
        if ((unsigned)session_inventory_count(&g_session1, id) < need) {
            disabled = 1;
            break;
        }
    }
    if (!disabled && (unsigned __int64)(__int64)g_session1.zeny < self->zeny)
        disabled = 1;
    self->button->button.disabled = disabled;
    ui_window_invalidate_base(&self->button->button.base);
}

void enchant_info_lock(struct UIEnchantInfo *self)
{
    struct TreeNode *node;

    for (node = tree_first(&self->sockets); node != self->sockets.head; node = tree_next(node)) {
        struct UIEnchantItem *socket = TREE_VALUE(node, struct EnchantSocket)->item;
        socket->locked = 1;
        ui_window_invalidate_base(&socket->base);
    }
    self->results->locked = 1;
    ui_window_invalidate_base(&self->results->base);
    self->button->button.disabled = 1;
    ui_window_invalidate_base(&self->button->button.base);
    ui_window_invalidate_base(&self->base);
}

void enchant_info_lock_sockets(struct UIEnchantInfo *self, char locked)
{
    struct TreeNode *node;

    for (node = tree_first(&self->sockets); node != self->sockets.head; node = tree_next(node)) {
        struct UIEnchantItem *socket = TREE_VALUE(node, struct EnchantSocket)->item;
        socket->locked = locked;
        ui_window_invalidate_base(&socket->base);
    }
}

int enchant_info_enchant_slot_item_id(struct UIEnchantInfo *self)
{
    unsigned slot = (unsigned)self->enchant_slot;
    struct EnchantSocket *socket = tree_find(&self->sockets, &slot, tree_less_uint);

    if (!socket)
        return 0;
    return atoi(string_cstr(&socket->item->item.id_text));
}
