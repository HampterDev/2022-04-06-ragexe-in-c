#include "session/state.h"
#include "core/object_type_tables.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include "contents/client_res.h"
#include "contents/game_session.h"
#include "iteminfo/item_api.h"
#include "std/vec.h"
#include "contents/item_reform.h"
#include "contents/item_reform_wnd.h"
#include "std/std.h"
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
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/ui_items.h"

const struct UIWindowOps item_reform_list_ops = {
    .destroy = item_reform_list_delete,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = item_reform_list_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = item_reform_list_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = item_reform_list_on_draw,    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = item_reform_list_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = item_reform_list_on_mouse_move,
    .on_mouse_hover = item_reform_list_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = item_reform_list_press,
    .on_r_btn_down = item_reform_list_on_r_btn_down,
    .on_r_btn_up = item_reform_list_show_info,    .on_wheel = item_reform_list_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = item_reform_list_send_msg,
    .invalidate = ui_window_invalidate_base,    .render = ui_window_render_base,
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

#define ROW_BMP_PRESS "유저인터페이스\\ItemReform\\btn_reform_item_press.bmp"
#define ROW_BMP_OVER  "유저인터페이스\\ItemReform\\btn_reform_item_over.bmp"
#define ROW_BMP       "유저인터페이스\\ItemReform\\btn_reform_item.bmp"

#define ITEM_COUNT(list) ((unsigned)VEC_COUNT(&(list)->items, struct ItemInfo))
#define ITEM_AT(list, i) ((struct ItemInfo *)(list)->items.first + (i))

static unsigned row_at(const struct UIItemReformWnd_List *self, int y)
{
    unsigned row = (unsigned)y / self->row_height + self->top;
    unsigned last = ITEM_COUNT(self) - 1;

    return row < last ? row : last;
}

static void refresh(struct UIItemReformWnd_List *self)
{
    unsigned count = ITEM_COUNT(self);

    if (count > self->visible_rows) {
        unsigned top, limit;

        ui_window_set_show_base(&self->scroll_bar->base, 1);
        ui_scroll_bar_set_range(self->scroll_bar, count - self->visible_rows, self->visible_rows, count);
        top = self->top < 0 ? 0 : (unsigned)self->top;
        limit = count - self->visible_rows;
        if (top > limit)
            top = limit;
        self->top = (int)top;
        ui_scroll_bar_set_pos(self->scroll_bar, (int)top);
    } else {
        self->top = 0;
        ui_window_set_show_base(&self->scroll_bar->base, 0);
    }
    ui_window_invalidate_base(&self->frame.base);
    if (self->frame.base.parent)
        ui_window_invalidate(self->frame.base.parent);
}

static char has_card_past_slots(struct ItemInfo *item)
{
    unsigned i;

    if (item_info_get_slot_count(item) >= 4)
        return 0;
    for (i = (unsigned)item_info_get_slot_count(item); i < 4; i++)
        if (item->slot[i] > 0)
            return 1;
    return 0;
}

static void add_candidates(struct UIItemReformWnd_List *self, const struct ReformInfo *info, int base_item)
{
    struct Vec indices;
    int *index;

    session_inventory_indices(&g_session1, &indices, base_item, 1);
    for (index = (int *)indices.first; index != (int *)indices.last; index++) {
        struct ItemInfo item;
        unsigned long long refine;

        if (session_is_in_switch_sets(&g_session1, *index))
            continue;
        session_get_inventory_item(&g_session1, &item, *index);
        refine = (unsigned long long)(long long)item.refine;
        if ((unsigned long long)info->need_refine_min > refine
            || (unsigned long long)info->need_refine_max < refine
            || (unsigned long long)info->need_option_num_min
                   > (unsigned long long)(long long)item.option_count) {
            item_info_destruct(&item);
            continue;
        }
        if (info->is_empty_socket == 1 && has_card_past_slots(&item) == 1) {
            item_info_destruct(&item);
            continue;
        }
        vec_push(&self->items, &item, sizeof item);
    }
    vec_free(&indices);
}

static int fill(struct UIItemReformWnd_List *self, int reform_item)
{
    struct Vec bases;
    int *base;

    item_info_vec_clear(&self->items);
    item_reform_base_items(item_reform_mgr(), &bases, reform_item);
    for (base = (int *)bases.first; base != (int *)bases.last; base++) {
        const struct ReformInfo *info = item_reform_find(item_reform_mgr(), reform_item, *base);

        if (info)
            add_candidates(self, info, *base);
    }
    refresh(self);
    ui_window_invalidate_base(&self->frame.base);
    vec_free(&bases);
    return self->items.first == self->items.last ? -1 : 0;
}

struct UIItemReformWnd_List *item_reform_list_construct(struct UIItemReformWnd_List *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &item_reform_list_ops;
    self->row_height = 0x20;
    self->pressed = (unsigned)-1;
    self->hover = 0;
    self->top = 0;
    self->scroll_bar = NULL;
    self->items.first = self->items.last = self->items.end = NULL;
    return self;
}

struct UIItemReformWnd_List *item_reform_list_new(void)
{
    struct UIItemReformWnd_List *self = (struct UIItemReformWnd_List *)mem_alloc(sizeof *self);

    if (self)
        item_reform_list_construct(self);
    return self;
}

void item_reform_list_destruct(struct UIItemReformWnd_List *self)
{
    self->frame.base.ops = &item_reform_list_ops;
    item_info_vec_free(&self->items);
    ui_frame_wnd_destruct(&self->frame);
}

void item_reform_list_delete(struct UIWindow *window)
{
    struct UIItemReformWnd_List *self = (struct UIItemReformWnd_List *)window;
    item_reform_list_destruct(self);
    mem_free(self);
}

void item_reform_list_move(struct UIWindow *window, int x, int y)
{
    struct UIItemReformWnd_List *self = (struct UIItemReformWnd_List *)window;
    ui_window_move_base(&self->frame.base, x, y);
}

void item_reform_list_on_create(struct UIWindow *window, int w, int h)
{
    struct UIItemReformWnd_List *self = (struct UIItemReformWnd_List *)window;
    struct UIScrollBar *bar = mem_alloc(sizeof *bar);

    if (bar)
        bar = ui_scroll_bar_construct(bar);
    self->scroll_bar = bar;
    ui_scroll_bar_create(bar, w - 0xd, 0, 1, h);
    ui_window_set_show_base(&self->scroll_bar->base, 0);
    ui_window_add_child(&self->frame.base, &self->scroll_bar->base);
    self->visible_rows = (unsigned)h / self->row_height;
    refresh(self);
}

void item_reform_list_on_draw(struct UIWindow *window)
{
    struct UIItemReformWnd_List *self = (struct UIItemReformWnd_List *)window;
    unsigned row;

    ui_window_clear_canvas(&self->frame.base, 0xffffffff);
    for (row = 0; row < self->visible_rows; row++) {
        unsigned i = self->top + row;
        int y = self->row_height * row;
        struct String shown = {0};
        struct String name = {0};
        struct ItemInfo *item;
        const char *bmp;

        if (i >= ITEM_COUNT(self))
            continue;
        bmp = i == self->pressed ? ROW_BMP_PRESS : i == self->hover ? ROW_BMP_OVER : ROW_BMP;
        ui_window_draw_bitmap(&self->frame.base, 0, y, res_mgr_get(g_resMgr(), bmp), 1);
        item = ITEM_AT(self, self->top + row);
        ui_window_draw_item_icon(&self->frame.base, 0xf, self->row_height * row + 4, item, 1);
        item_info_get_display_name(item, &name);
        ui_window_fit_text(&self->frame.base, &shown, 0xa0, string_cstr(&name), 0, 0xc, 0);
        ui_window_text_out(&self->frame.base, 0x2a, self->row_height * row + 0xa, string_cstr(&shown),
                   0, 0, 0xc, 0, 0, 0);
        string_free(&shown);
        string_free(&name);
    }
}

void item_reform_list_on_l_btn_down(struct UIWindow *self, int x, int y) { (void)self; (void)x; (void)y; }
void item_reform_list_on_mouse_move(struct UIWindow *self, int x, int y) { (void)self; (void)x; (void)y; }
void item_reform_list_on_r_btn_down(struct UIWindow *self, int x, int y) { (void)self; (void)x; (void)y; }

void item_reform_list_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemReformWnd_List *self = (struct UIItemReformWnd_List *)window;
    unsigned row = row_at(self, y);

    (void)x;
    if (row < ITEM_COUNT(self) && self->hover != row) {
        self->hover = row;
        ui_window_invalidate_base(&self->frame.base);
    }
}

void item_reform_list_press(struct UIWindow *window, int x, int y)
{
    struct UIItemReformWnd_List *self = (struct UIItemReformWnd_List *)window;
    unsigned row = row_at(self, y);

    (void)x;
    if (row >= ITEM_COUNT(self) || self->pressed == row)
        return;
    self->pressed = row;
    if (self->frame.base.parent)
        ui_window_send(self->frame.base.parent, NULL, UI_MSG_BUTTON, REFORM_LIST_PRESSED, (int)ITEM_AT(self, row), 0, 0);
    ui_window_invalidate_base(&self->frame.base);
}

void item_reform_list_show_info(struct UIWindow *window, int x, int y)
{
    struct UIItemReformWnd_List *self = (struct UIItemReformWnd_List *)window;
    unsigned row = row_at(self, y);
    struct UIWindow *desc;

    (void)x;
    if (row >= ITEM_COUNT(self))
        return;
    desc = ui_window_open(ITEM_DESC_WND_ID);
    if (desc)
        ui_window_send(desc, &self->frame.base, UI_MSG_SHOW_ITEM, (int)ITEM_AT(self, row), 0, 0, 0);
}

void item_reform_list_wheel(struct UIWindow *window, int delta)
{
    struct UIItemReformWnd_List *self = (struct UIItemReformWnd_List *)window;
    ui_window_send(&self->frame.base, &self->scroll_bar->base, delta < 0 ? UI_MSG_SCROLL_DOWN : UI_MSG_SCROLL_UP, 0, 0, 0, 0);
}

int item_reform_list_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                         int a, int b, int c, int d)
{
    struct UIItemReformWnd_List *self = (struct UIItemReformWnd_List *)window;
    switch (msg) {
    case UI_MSG_SCROLL_BY:
    case UI_MSG_SCROLL_UP:
    case UI_MSG_SCROLL_DOWN: {
        int old = self->top;

        self->top = msg == UI_MSG_SCROLL_UP ? old - 1 : msg == UI_MSG_SCROLL_DOWN ? old + 1 : a + old;
        if (self->top != old)
            refresh(self);
        return self->top - old;
    }
    case UI_MSG_REFORM_ITEM:
        if (fill(self, a) == -1)
            return -1;
        break;
    }
    return ui_window_send_msg_base(&self->frame.base, sender, msg, a, b, c, d);
}
