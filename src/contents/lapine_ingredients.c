#include "session/item_predicates.h"
#include "ui/controls/scroll.h"
#include "game/drag_info.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include <string.h>
#include "contents/client_res.h"
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "contents/lapine_box.h"
#include "std/std.h"
#include "ui/controls/buttons.h"
#include "ui/controls/combo_box.h"
#include "ui/controls/line_info.h"
#include "ui/quests/tag_text_viewer.h"
#include "base.h"
#include "ui/window_types.h"
#include "ui/controls/edit_types.h"
#include "contents/ui_window.h"
#include "contents/ui_window_mgr.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/ui_items.h"

const struct UIWindowOps lapine_scroll_bar_ops = {
    .destroy = lapine_scroll_bar_delete,
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
    .on_draw = lapine_scroll_bar_on_draw,    .on_focus = ui_window_on_focus_base,
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
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

const struct UIWindowOps lapine_ingredient_ops = {
    .destroy = lapine_row_delete,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = lapine_row_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = lapine_row_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_window_on_create_base,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = lapine_row_on_draw,    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = lapine_row_press,
    .on_l_btn_dbl_clk = lapine_row_double_click,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = lapine_row_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = lapine_row_release,
    .on_r_btn_down = lapine_row_show_info,
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

const struct UIWindowOps lapine_ingredient_list_ops = {
    .destroy = lapine_list_delete,
    .resize = ui_scroll_wnd_resize,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = lapine_list_drop,
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
    .send_msg = lapine_list_send_msg,
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

#define MSG_TOO_FEW             0xb52
#define MSG_REFINE_TOO_LOW      0xb53
#define MSG_REFINE_TOO_HIGH     0xe3c
#define MSG_REFINE_ABOVE_MIN    0xe3d
#define MSG_HAS_CARDS           0xe3e
#define MSG_NAME_WITH_OPTIONS   0xa28

#define ROW_W           0x88
#define ROW_H           0x1e
#define ROW_TOP         7
#define COLOR_ROW_BG    0xfff7f7
#define COLOR_KEY       0xffff00ff

static struct ItemInfo *find_resource(struct LapineIngredientList *self, const struct ItemInfo *item)
{
    struct ItemInfo *r = (struct ItemInfo *)self->resources.first;
    struct ItemInfo *last = (struct ItemInfo *)self->resources.last;

    for (; r != last; r++)
        if (atoi(string_cstr(&item->id_text)) == atoi(string_cstr(&r->id_text)))
            break;
    return r;
}

static struct LapineIngredient **find_row(struct LapineIngredientList *self, int index)
{
    struct LapineIngredient **it = (struct LapineIngredient **)self->rows.first;
    struct LapineIngredient **last = (struct LapineIngredient **)self->rows.last;

    while (it != last && index != (*it)->item.index)
        it++;
    return it;
}

static void notice(struct LapineIngredientList *self, const char *text)
{
    ui_window_send(self->scroll.base.parent, &self->scroll.base, LAPINE_MSG_NOTICE, (intptr_t)text, 0, 0, 0);
}

static void notice_too_few(struct LapineIngredientList *self, int count)
{
    struct String text;

    string_init(&text);
    string_format(&text, msg_str(MSG_TOO_FEW), count);
    notice(self, string_cstr(&text));
    string_free(&text);
}

static char explain_row(struct LapineIngredientList *self, int index)
{
    struct LapineIngredient **it = find_row(self, index);
    struct ItemInfo *item, *r;
    int i;

    if (it == (struct LapineIngredient **)self->rows.last)
        return 0;
    item = &(*it)->item;
    r = find_resource(self, item);
    if (r == (struct ItemInfo *)self->resources.last)
        return 0;
    if (r->count > item->count) {
        notice_too_few(self, r->count);
        return 0;
    }
    if (self->min_refine > item->refine) {
        notice(self, msg_str(MSG_REFINE_TOO_LOW));
        return 0;
    }
    if (self->max_refine < item->refine) {
        notice(self, msg_str(MSG_REFINE_TOO_HIGH));
        return 0;
    }
    if (self->min_refine != 0 && self->min_refine < item->refine)
        notice(self, msg_str(MSG_REFINE_ABOVE_MIN));
    if (item_info_is_equipment_type(item) == 1 && !item_info_is_pet_range_id(item) && !item_info_has_maker_slot(item)
        && item_info_get_slot_count(item) > 0) {
        i = 0;
        do {
            if (i < 4 && item->slot[i] > 0) {
                notice(self, msg_str(MSG_HAS_CARDS));
                break;
            }
            i++;
        } while (i < item_info_get_slot_count(item));
    }
    return 1;
}

struct LapineIngredient *lapine_row_construct(struct LapineIngredient *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &lapine_ingredient_ops;
    item_info_construct(&self->item);
    self->selected = 0;
    self->unusable = 0;
    string_init(&self->name);
    self->count = 0;
    return self;
}

static struct LapineIngredient *new_row(void)
{
    struct LapineIngredient *row = (struct LapineIngredient *)mem_alloc(sizeof *row);

    if (row)
        lapine_row_construct(row);
    return row;
}

struct LapineIngredientList *lapine_list_construct(struct LapineIngredientList *self, int w, int h)
{
    ui_scroll_wnd_construct(&self->scroll, w, h, 0);
    self->scroll.base.ops = &lapine_ingredient_list_ops;
    self->resources.first = self->resources.last = self->resources.end = NULL;
    self->rows.first = self->rows.last = self->rows.end = NULL;
    self->min_refine = 0;
    self->max_refine = 20;
    ui_window_resize_base(&self->scroll.base, w, h);
    lapine_list_on_create(&self->scroll.base, w, h);
    return self;
}

struct LapineIngredientList *lapine_list_new(void)
{
    struct LapineIngredientList *self = (struct LapineIngredientList *)mem_alloc(sizeof *self);

    if (!self)
        return NULL;
    return lapine_list_construct(self, 0x96, 0xcc);
}

void lapine_list_destruct(struct LapineIngredientList *self)
{
    self->scroll.base.ops = &lapine_ingredient_list_ops;
    vec_free(&self->rows);
    item_info_vec_free(&self->resources);
    ui_scroll_wnd_destruct(&self->scroll);
}

static char list_add(struct LapineIngredientList *self, int index, const struct ItemInfo *item)
{
    struct ItemInfo from_inventory, *r;
    struct LapineIngredient **it, *row;
    struct ItemNameRuns runs = {0};

    if (session_is_pet_egg_id(&g_session1, atoi(string_cstr(&item->id_text))) == 1 && item->damaged == 1)
        return 0;
    if (session_is_in_switch_sets(&g_session1, item->index) == 1)
        return 0;
    r = find_resource(self, item);
    if (r == (struct ItemInfo *)self->resources.last)
        return 0;

    it = find_row(self, index);
    if (it != (struct LapineIngredient **)self->rows.last) {
        row = *it;
        row->item.count += r->count;
        row->unusable = 0;
        ui_window_set_show_base(&row->frame.base, 1);
        ui_window_invalidate_base(&row->frame.base);
        ui_window_invalidate_base(&self->scroll.base);
        return 1;
    }

    row = new_row();
    ui_window_create(&row->frame.base, ROW_W, ROW_H);
    session_get_inventory_item(&g_session1, &from_inventory, index);
    item_info_assign(&row->item, &from_inventory);
    item_info_destruct(&from_inventory);

    item_info_name_runs(&row->frame.base, &row->item, &runs, 1, 0);
    if (row->item.identified == 1 && row->item.option_count)
        string_format(&runs.text, msg_str(MSG_NAME_WITH_OPTIONS), string_cstr(&runs.text), row->item.option_count);
    item_info_set_name(&row->item, string_cstr(&runs.text));
    string_assign_cstr(&row->name, string_cstr(&runs.text));
    string_free(&runs.text);
    ui_window_invalidate_base(&row->frame.base);
    row->count = r->count;
    ui_scroll_wnd_add_child(&self->scroll, &row->frame.base);
    if (item->count < r->count)
        row->unusable = 1;
    if (self->min_refine > item->refine || self->max_refine < item->refine)
        row->unusable = 1;
    ui_window_invalidate_base(&row->frame.base);
    vec_push(&self->rows, &row, sizeof row);
    ui_window_send(&self->scroll.base, &self->scroll.base, LAPINE_MSG_LAYOUT, 0, 0, 0, 0);
    return 1;
}

char lapine_list_add(struct LapineIngredientList *self, int index)
{
    struct ItemInfo item;
    char added = 0;

    session_get_inventory_item(&g_session1, &item, index);
    if ((unsigned)item.index != 0)
        added = list_add(self, index, &item);
    item_info_destruct(&item);
    return added;
}

static void fill(struct LapineIngredientList *self)
{
    struct ItemInfo *r;
    struct LapineIngredient **it;

    for (r = (struct ItemInfo *)self->resources.first; r != (struct ItemInfo *)self->resources.last; r++) {
        struct Vec indices;
        int *p;

        session_inventory_indices(&g_session1, &indices, atoi(string_cstr(&r->id_text)), 1);
        for (p = (int *)indices.first; p != (int *)indices.last; p++)
            lapine_list_add(self, *p);
        vec_free(&indices);
    }
    for (it = (struct LapineIngredient **)self->rows.first; it != (struct LapineIngredient **)self->rows.last; it++) {
        struct ItemInfo item;

        session_get_inventory_item(&g_session1, &item, (*it)->item.index);
        if ((unsigned)item.index == 0)
            ui_window_set_show_base(&(*it)->frame.base, 0);
        item_info_destruct(&item);
    }
    ui_window_send(&self->scroll.base, &self->scroll.base, LAPINE_MSG_LAYOUT, 0, 0, 0, 0);
}

void lapine_list_layout(struct UIScrollWnd *scroll, struct Vec *rows)
{
    struct UIWindow **it = (struct UIWindow **)rows->first;
    int y = ROW_TOP;

    while (it != (struct UIWindow **)rows->last) {
        if ((*it)->visible == 0) {
            ui_window_invalidate(*it);
            memmove(it, it + 1, (char *)rows->last - (char *)(it + 1));
            rows->last = (struct UIWindow **)rows->last - 1;
        } else {
            ui_scroll_wnd_place_child(scroll, *it, 0, y);
            ui_window_invalidate(*it);
            it++;
            y += ROW_H;
        }
    }
    ui_scroll_wnd_update_bars(scroll);
    ui_window_invalidate_base(&scroll->base);
}

static int take(struct LapineIngredientList *self, int index)
{
    struct LapineIngredient **it = find_row(self, index), *row;
    struct ItemInfo *r;

    if (it == (struct LapineIngredient **)self->rows.last)
        return 0;
    row = *it;
    r = find_resource(self, &row->item);
    if (r == (struct ItemInfo *)self->resources.last)
        return 0;
    if (r->count > row->item.count) {
        notice_too_few(self, r->count);
        return -1;
    }
    if (r->refine > row->item.refine) {
        notice(self, msg_str(MSG_REFINE_TOO_LOW));
        return -1;
    }
    row->item.count -= r->count;
    if (row->item.count <= 0) {
        ui_window_set_show_base(&row->frame.base, 0);
    } else {
        row->unusable = 1;
        ui_window_invalidate_base(&row->frame.base);
    }
    ui_window_send(&self->scroll.base, &self->scroll.base, LAPINE_MSG_LAYOUT, 0, 0, 0, 0);
    return 0;
}

void lapine_list_delete(struct UIWindow *window)
{
    struct LapineIngredientList *self = (struct LapineIngredientList *)window;
    lapine_list_destruct(self);
    mem_free(self);
}

void lapine_list_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drop)
{
    struct LapineIngredientList *self = (struct LapineIngredientList *)window;

    if (drop->source == LAPINE_DRAG_CHOSEN && lapine_list_add(self, drop->index) == 1)
        ui_window_send(self->scroll.base.parent, &self->scroll.base, LAPINE_MSG_REMOVE, drop->index, 0, 0, 0);
    game_send_message(MODE_MSG_DROPPED, 0, 0, 0, 0);
}

void lapine_list_on_create(struct UIWindow *window, int w, int h)
{
    struct LapineIngredientList *self = (struct LapineIngredientList *)window;
    struct LapineScrollBar *bar;

    self->scroll.vbar_width = 9;
    if (self->scroll.vbar) {
        ui_scroll_wnd_remove_child(&self->scroll, &self->scroll.vbar->base, 0);
        if (self->scroll.vbar)
            ui_window_destroy(&self->scroll.vbar->base);
        self->scroll.vbar = NULL;
    }
    bar = (struct LapineScrollBar *)mem_alloc(sizeof *bar);
    if (bar)
        lapine_scroll_bar_construct(bar);
    self->scroll.vbar = &bar->bar;
    ui_scroll_bar_create(self->scroll.vbar, w - self->scroll.vbar_width, 0, 1, h - self->scroll.hbar_height);
    ui_window_add_child(&self->scroll.base, &self->scroll.vbar->base);
}

void lapine_list_on_draw(struct UIWindow *window)
{
    struct LapineIngredientList *self = (struct LapineIngredientList *)window;
    ui_window_clear_canvas(&self->scroll.base, COLOR_ROW_BG);
}

void lapine_list_wheel(struct UIWindow *window, int delta)
{
    struct LapineIngredientList *self = (struct LapineIngredientList *)window;
    if (self->scroll.vscroll_on == 1)
        ui_window_send(&self->scroll.base, &self->scroll.vbar->base, UI_MSG_SCROLL_BY, delta < 0 ? 15 : -15, 0, 0, 0);
}

intptr_t lapine_list_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                    intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    struct LapineIngredientList *self = (struct LapineIngredientList *)window;
    struct LapineIngredient **it;

    switch (msg) {
    case UI_MSG_BUTTON:
        for (it = (struct LapineIngredient **)self->rows.first; it != (struct LapineIngredient **)self->rows.last;
             it++) {
            if (&(*it)->frame.base != sender) {
                (*it)->selected = 0;
                ui_window_invalidate_base(&(*it)->frame.base);
            }
        }
        ui_window_invalidate_base(&self->scroll.base);
        break;
    case LAPINE_MSG_FILL:
        fill(self);
        break;
    case LAPINE_MSG_LAYOUT:
        lapine_list_layout(&self->scroll, &self->rows);
        break;
    case LAPINE_MSG_REMOVE:
        if (take(self, a) < 0)
            return -1;
        break;
    }
    return ui_scroll_wnd_send_msg((struct UIWindow *)(&self->scroll), sender, msg, a, b, c, d);
}

void lapine_row_destruct(struct LapineIngredient *self)
{
    self->frame.base.ops = &lapine_ingredient_ops;
    string_free(&self->name);
    item_info_destruct(&self->item);
    ui_frame_wnd_destruct(&self->frame);
}

void lapine_row_delete(struct UIWindow *window)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    lapine_row_destruct(self);
    mem_free(self);
}

void lapine_row_move(struct UIWindow *window, int x, int y)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    ui_window_move_base(&self->frame.base, x, y);
}

void lapine_row_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drop)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    struct UIWindow *list = self->frame.base.parent;

    ui_window_drag_and_drop(list, x, y, drop);
}

void lapine_row_on_draw(struct UIWindow *window)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    struct String cut = {0};

    ui_window_clear_canvas(&self->frame.base, COLOR_ROW_BG);
    if (self->selected == 1)
        ui_window_draw_bitmap(&self->frame.base, 0x20, 0, res_mgr_get(g_resMgr(), LAPINE_BMP("list_selected_item.bmp")), 0);
    ui_window_fit_text(&self->frame.base, &cut, 100, string_cstr(&self->item.name), 0, 0xc, 0);
    if (!self->unusable)
        ui_window_draw_item_icon_count(&self->frame.base, 4, 3, &self->item, 0, 0);
    else
        ui_window_draw_item_icon_count_grey(&self->frame.base, 1, 4, 3, &self->item, self->item.identified, 0);
    ui_window_draw_shadow_text(&self->frame.base, 0x24, 9, string_cstr(&cut), 0, self->unusable ? 0x969696 : 0, 0xe0e0e0,
               0, 0xc, 0);
    string_free(&cut);
}

void lapine_row_press(struct UIWindow *window, int x, int y)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    if (self->unusable != 1 && (unsigned)self->item.index > 0)
        game_mode_drag_item(game_current_mode(), LAPINE_DRAG_CANDIDATE, &self->item,
                            self->count, -1);
}

void lapine_row_double_click(struct UIWindow *window, int x, int y)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    struct LapineIngredientList *list = (struct LapineIngredientList *)self->frame.base.parent;
    struct LapineBox *box;
    int index, count;

    if (self->unusable == 1) {
        explain_row(list, self->item.index);
        return;
    }
    index = self->item.index;
    if (!index)
        return;
    count = self->count;
    box = (struct LapineBox *)list->scroll.base.parent;
    if (VEC_COUNT(&box->chosen, struct ItemInfo) < (unsigned)box->need_count
        && ui_window_send(&list->scroll.base, &list->scroll.base, LAPINE_MSG_REMOVE, index, 0, 0, 0) >= 0)
        lapine_box_add(box, index, count);
}

void lapine_row_hover(struct UIWindow *window, int x, int y)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    ui_frame_wnd_on_mouse_move(&self->frame.base, x, y);
    if (self->name.size > 0) {
        int bx = 0, by = -20;

        ui_window_to_screen(&self->frame.base, &bx, &by);
        mode_show_tooltip(client_current_mode(), string_cstr(&self->name), bx, by, -1, 0);
    }
}

void lapine_row_release(struct UIWindow *window, int x, int y)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    if (self->unusable == 1)
        return;
    self->selected = 1;
    ui_window_invalidate_base(&self->frame.base);
    if (self->frame.base.parent)
        ui_window_send(self->frame.base.parent, &self->frame.base, UI_MSG_BUTTON, 0, 0, 0, 0);
}

void lapine_row_show_info(struct UIWindow *window, int x, int y)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    if ((unsigned)self->item.index > 0) {
        struct UIWindow *desc = ui_window_open(ITEM_DESC_WND_ID);

        ui_window_send(desc, &self->frame.base, UI_MSG_SHOW_ITEM, (intptr_t)&self->item, 0, 0, 0);
    }
}

void lapine_row_wheel(struct UIWindow *window, int delta)
{
    struct LapineIngredient *self = (struct LapineIngredient *)window;
    struct UIWindow *list = self->frame.base.parent;

    if (list)
        ui_window_on_wheel(list, delta);
}

struct LapineScrollBar *lapine_scroll_bar_construct(struct LapineScrollBar *self)
{
    ui_scroll_bar_construct(&self->bar);
    self->bar.base.ops = &lapine_scroll_bar_ops;
    self->bar.button_size = 0;
    return self;
}

void lapine_scroll_bar_delete(struct UIWindow *window)
{
    struct LapineScrollBar *self = (struct LapineScrollBar *)window;
    ui_scroll_bar_destruct(&self->bar);
    mem_free(self);
}

static int pieces(int length, const struct BitmapRes *mid)
{
    return length / mid->height + (length % mid->height != 0);
}

static struct BitmapRes *bitmap(const char *name)
{
    return (struct BitmapRes *)res_mgr_get(g_resMgr(), name);
}

void lapine_scroll_bar_on_draw(struct UIWindow *window)
{
    struct LapineScrollBar *self = (struct LapineScrollBar *)window;
    static const char *const thumb_bitmaps[3][3] = {
        { LAPINE_BMP("scrollbar_thumb_out_top.bmp"), LAPINE_BMP("scrollbar_thumb_out_btm.bmp"),
          LAPINE_BMP("scrollbar_thumb_out_mid.bmp") },
        { LAPINE_BMP("scrollbar_thumb_over_top.bmp"), LAPINE_BMP("scrollbar_thumb_over_btm.bmp"),
          LAPINE_BMP("scrollbar_thumb_over_mid.bmp") },
        { LAPINE_BMP("scrollbar_thumb_press_top.bmp"), LAPINE_BMP("scrollbar_thumb_press_btm.bmp"),
          LAPINE_BMP("scrollbar_thumb_press_mid.bmp") },
    };

    ui_window_clear_canvas(&self->bar.base, COLOR_KEY);
    if (!self->bar.max_pos)
        return;
    if (self->bar.orientation == 1) {
        struct BitmapRes *top = bitmap(LAPINE_BMP("scrollbar_bg_top.bmp"));
        struct BitmapRes *btm = bitmap(LAPINE_BMP("scrollbar_bg_btm.bmp"));
        struct BitmapRes *mid = bitmap(LAPINE_BMP("scrollbar_bg_mid.bmp"));
        int h = self->bar.base.height, track, length, pos, n, i, which;

        ui_window_draw_bitmap(&self->bar.base, 0, 0, top, 1);
        n = pieces(h - btm->height - top->height, mid);
        for (i = 0; i < n; i++)
            ui_window_draw_bitmap(&self->bar.base, 0, top->height + mid->height * i, mid, 1);
        ui_window_draw_bitmap(&self->bar.base, 0, h - btm->height, btm, 1);

        track = self->bar.base.height - btm->height - top->height;
        length = self->bar.page * track / self->bar.total;
        pos = self->bar.pos * track / self->bar.total;
        switch (self->bar.hot) {
        case 0: which = 0; break;
        case 3: which = 1; break;
        case 6: which = 2; break;
        default: which = -1; break;
        }
        if (which >= 0) {
            struct BitmapRes *t_top = bitmap(thumb_bitmaps[which][0]);
            struct BitmapRes *t_btm = bitmap(thumb_bitmaps[which][1]);
            struct BitmapRes *t_mid = bitmap(thumb_bitmaps[which][2]);

            n = pieces(length - t_btm->height - t_top->height, t_mid);
            for (i = 0; i < n; i++)
                ui_window_draw_bitmap(&self->bar.base, 0, t_mid->height * i + pos + t_top->height + top->height, t_mid, 1);
            ui_window_draw_bitmap(&self->bar.base, 0, top->height + pos, t_top, 1);
            ui_window_draw_bitmap(&self->bar.base, 0, pos - t_btm->height + length + top->height, t_btm, 1);
        }
    }
    self->bar.min_thumb = 0;
    ui_window_invalidate(self->bar.base.parent);
}
