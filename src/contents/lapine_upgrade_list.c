#include "session/item_predicates.h"
#include "ui/controls/scroll.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "contents/client_res.h"
#include "contents/connection.h"
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "contents/lapine_box.h"
#include "contents/lapine_upgrade.h"
#include "std/std.h"
#include "ui/controls/buttons.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "base.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"
#include "contents/ui_window.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/ui_items.h"

const struct UIWindowOps lapine_upgrade_row_ops = {
    .destroy = lapine_upgrade_row_delete,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = lapine_row_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = lapine_upgrade_row_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = lapine_upgrade_row_on_draw,    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = lapine_upgrade_row_press,
    .on_l_btn_dbl_clk = lapine_upgrade_row_double_click,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = lapine_row_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = lapine_upgrade_row_release,
    .on_r_btn_down = lapine_upgrade_row_show_info,
    .on_r_btn_up = ui_window_on_r_btn_up_base,    .on_wheel = lapine_row_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_frame_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

const struct UIWindowOps lapine_upgrade_list_ops = {
    .destroy = lapine_upgrade_list_delete,
    .resize = ui_scroll_wnd_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = lapine_upgrade_list_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = lapine_list_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = lapine_list_on_draw,    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,    .on_wheel = lapine_list_wheel,
    .snap = ui_window_snap_base,
    .send_msg = lapine_upgrade_list_send_msg,
    .invalidate = ui_window_invalidate_base,    .render = ui_scroll_wnd_render,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_scroll_wnd_make_quad,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define MSG_REFINE_ABOVE_MIN    0xe3d
#define MSG_HAS_CARDS           0xe3e
#define MSG_NAME_WITH_OPTIONS   0xa28

#define ROW_W   0x88
#define ROW_H   0x1e

static struct ItemInfo *find_resource(struct LapineUpgradeList *self, const struct ItemInfo *item)
{
    struct ItemInfo *r = (struct ItemInfo *)self->resources.first;
    struct ItemInfo *last = (struct ItemInfo *)self->resources.last;

    for (; r != last; r++)
        if (atoi(string_cstr(&item->id_text)) == atoi(string_cstr(&r->id_text)))
            break;
    return r;
}

static struct LapineIngredient **find_row(struct LapineUpgradeList *self, int index)
{
    struct LapineIngredient **it = (struct LapineIngredient **)self->rows.first;
    struct LapineIngredient **last = (struct LapineIngredient **)self->rows.last;

    while (it != last && index != (*it)->item.index)
        it++;
    return it;
}

static void notice(struct LapineUpgradeList *self, int msg_id)
{
    ui_window_send(self->scroll.base.parent, &self->scroll.base, LAPINE_MSG_NOTICE, (intptr_t)msg_str(msg_id), 0, 0, 0);
}

static void item_text(struct UIWindow *wnd, struct ItemInfo *item, struct ItemNameRuns *runs)
{
    item_info_name_runs(wnd, item, runs, 1, 0);
    if (item->identified == 1 && item->option_count)
        string_format(&runs->text, msg_str(MSG_NAME_WITH_OPTIONS), string_cstr(&runs->text), item->option_count);
}

struct LapineIngredient *lapine_upgrade_row_construct(struct LapineIngredient *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &lapine_upgrade_row_ops;
    item_info_construct(&self->item);
    self->selected = 0;
    string_init(&self->name);
    self->count = 0;
    return self;
}

void lapine_upgrade_row_destruct(struct LapineIngredient *self)
{
    self->frame.base.ops = &lapine_upgrade_row_ops;
    string_free(&self->name);
    item_info_destruct(&self->item);
    ui_frame_wnd_destruct(&self->frame);
}

struct LapineUpgradeList *lapine_upgrade_list_construct(struct LapineUpgradeList *self, int w, int h)
{
    ui_scroll_wnd_construct(&self->scroll, w, h, 0);
    self->scroll.base.ops = &lapine_upgrade_list_ops;
    self->resources.first = self->resources.last = self->resources.end = NULL;
    self->rows.first = self->rows.last = self->rows.end = NULL;
    self->min_refine = 0;
    self->max_refine = 20;
    ui_window_resize_base(&self->scroll.base, w, h);
    lapine_list_on_create(&self->scroll.base, w, h);
    return self;
}

void lapine_upgrade_list_destruct(struct LapineUpgradeList *self)
{
    self->scroll.base.ops = &lapine_upgrade_list_ops;
    vec_free(&self->rows);
    item_info_vec_free(&self->resources);
    ui_scroll_wnd_destruct(&self->scroll);
}

void lapine_upgrade_destruct(struct LapineUpgradeBox *self)
{
    struct ItemInfo empty;
    unsigned short id = PACKET_LAPINE_UPGRADE_CLOSE;
    int len;

    self->frame.base.ops = &lapine_upgrade_box_ops;
    item_info_construct(&empty);
    item_info_assign(&self->target, &empty);
    item_info_destruct(&empty);
    len = rag_connection_get_packet_length(rag_connection_instance(), PACKET_LAPINE_UPGRADE_CLOSE);
    rag_connection_send_packet(rag_connection_instance(), len, &id);
    string_free(&self->notice);
    string_free(&self->need_source);
    item_info_destruct(&self->target);
    string_free(&self->title);
    ui_frame_wnd_destruct(&self->frame);
}

struct LapineIngredient *lapine_upgrade_list_selected(struct LapineUpgradeList *self)
{
    struct LapineIngredient **it = (struct LapineIngredient **)self->rows.first;
    struct LapineIngredient **last = (struct LapineIngredient **)self->rows.last;

    for (; it != last; it++)
        if ((*it)->selected == 1)
            return *it;
    return NULL;
}

static char list_add(struct LapineUpgradeList *self, int index, const struct ItemInfo *item)
{
    struct ItemInfo from_inventory;
    struct LapineIngredient *row;
    struct ItemNameRuns runs = {0};

    if (self->min_refine > item->refine || self->max_refine < item->refine)
        return 0;
    if (self->min_option_num > item->option_count)
        return 0;
    if (session_is_pet_egg_id(&g_session1, atoi(string_cstr(&item->id_text))) == 1 && item->damaged == 1)
        return 0;
    if (session_is_in_switch_sets(&g_session1, item->index) == 1)
        return 0;
    if (find_resource(self, item) == (struct ItemInfo *)self->resources.last)
        return 0;
    if (find_row(self, index) != (struct LapineIngredient **)self->rows.last) {
        return 1;
    }

    row = (struct LapineIngredient *)mem_alloc(sizeof *row);
    if (row)
        lapine_upgrade_row_construct(row);
    ui_window_create(&row->frame.base, ROW_W, ROW_H);
    session_get_inventory_item(&g_session1, &from_inventory, index);
    item_info_assign(&row->item, &from_inventory);
    item_info_destruct(&from_inventory);
    item_text(&row->frame.base, &row->item, &runs);
    item_info_set_name(&row->item, string_cstr(&runs.text));
    string_assign_cstr(&row->name, string_cstr(&runs.text));
    ui_window_invalidate_base(&row->frame.base);
    string_free(&runs.text);
    row->count = 1;
    ui_scroll_wnd_add_child(&self->scroll, &row->frame.base);
    ui_window_invalidate_base(&row->frame.base);
    vec_push(&self->rows, &row, sizeof row);
    ui_window_send(&self->scroll.base, &self->scroll.base, LAPINE_MSG_LAYOUT, 0, 0, 0, 0);
    return 1;
}

char lapine_upgrade_list_add(struct LapineUpgradeList *self, int index)
{
    struct ItemInfo item;
    char added = 0;

    session_get_inventory_item(&g_session1, &item, index);
    if ((unsigned)item.index != 0)
        added = list_add(self, index, &item);
    item_info_destruct(&item);
    return added;
}

char lapine_upgrade_list_take(struct LapineUpgradeList *self, int index)
{
    struct LapineIngredient **it = find_row(self, index);
    struct ItemInfo *item;
    int i;

    if (it == (struct LapineIngredient **)self->rows.last)
        return 0;
    item = &(*it)->item;
    if (find_resource(self, item) == (struct ItemInfo *)self->resources.last)
        return 0;
    if (self->min_refine > item->refine || self->max_refine < item->refine)
        return 0;
    if (self->min_refine != 0 && self->min_refine < item->refine)
        notice(self, MSG_REFINE_ABOVE_MIN);
    if (item_info_is_equipment_type(item) == 1 && !item_info_is_pet_range_id(item) && !item_info_has_maker_slot(item)
        && item_info_get_slot_count(item) > 0) {
        i = 0;
        do {
            if (i < 4 && item->slot[i] > 0) {
                notice(self, MSG_HAS_CARDS);
                break;
            }
            i++;
        } while (i < item_info_get_slot_count(item));
    }
    if (self->min_option_num > item->option_count)
        return 0;
    ui_window_set_show_base(&(*it)->frame.base, 0);
    ui_window_send(&self->scroll.base, &self->scroll.base, LAPINE_MSG_LAYOUT, 0, 0, 0, 0);
    return 1;
}

void lapine_upgrade_add_resource(struct LapineUpgradeBox *self, int item_id)
{
    struct Vec *resources = &self->list->resources;
    struct ItemInfo *r = (struct ItemInfo *)resources->first;

    for (; r != (struct ItemInfo *)resources->last; r++)
        if (item_id == atoi(string_cstr(&r->id_text)))
            break;
    if (r == (struct ItemInfo *)resources->last) {
        struct ItemInfo item;

        item_info_construct(&item);
        item_info_set_item_id(&item, item_id);
        vec_push(resources, &item, sizeof item);
    }
}

void lapine_upgrade_set_waiting(struct LapineUpgradeBox *self, char on)
{
    struct UIWindow *button = self->make_button;

    self->waiting = on;
    ((struct UIButton *)button)->disabled = on;
    ui_window_invalidate(button);
    ui_window_invalidate_base(&self->frame.base);
}
