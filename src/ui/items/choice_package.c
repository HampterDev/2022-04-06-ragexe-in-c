#include "render/rp_pool.h"
#include "ui/item_draw.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/character/char_info.h"
#include "ui/runtime.h"
#include "ui/ui_std.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/wndmgr/make_window_internal.h"
#include "core/select_package.h"

#define PACKAGE_DIR UI_SKIN "selectpackage\\"

#define ROWS 7
#define ROW_HEIGHT 0x22
#define LIST_TOP 0x11
#define LIST_BOTTOM 0xff
#define SETS_LEFT 0xe
#define ITEMS_LEFT 0xad
#define NAME_WIDTH 0x91

#define ID_SELECT UI_ID_OK
#define ID_REFRESH 0x232
#define MSG_SET_LIST 0xab
#define MSG_ITEM_INFO 0x18
#define ITEM_INFO_WINDOW 0xc
#define MODE_REQ_CHOOSE_PACKAGE 0x145
#define MSG_CONFIRM 0xf87
#define MSG_CLOSE_TOOLTIP 0x513
#define ANSWER_NO 0xb4

struct ChoicePackageItemBoxWnd {
    struct UIFrameWnd frame;
    BYTE over_sets;
    int box_id;
    int item_id;
    struct Package *list;
    struct UIScrollBar *sets_bar;
    int selected;
    int first_set;
    int first_item;
    struct UIScrollBar *items_bar;
};
OFFSET_ASSERT(ChoicePackageItemBoxWnd, list, 0xa0);
SIZE_ASSERT(ChoicePackageItemBoxWnd, 0xb8);

void choice_package_item_box_wnd_destroy(struct UIWindow *window);
void choice_package_item_box_wnd_on_create(struct UIWindow *self, int width, int height);
void choice_package_item_box_wnd_on_process(struct UIWindow *self);
void choice_package_item_box_wnd_on_draw(struct UIWindow *self);
void choice_package_item_box_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void choice_package_item_box_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void choice_package_item_box_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void choice_package_item_box_wnd_on_wheel(struct UIWindow *self, int delta);
int choice_package_item_box_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps choice_package_item_box_wnd_ops = {
    .destroy = choice_package_item_box_wnd_destroy,
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
    .on_create = choice_package_item_box_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = choice_package_item_box_wnd_on_process,
    .on_draw = choice_package_item_box_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = choice_package_item_box_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = choice_package_item_box_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = choice_package_item_box_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = choice_package_item_box_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = choice_package_item_box_wnd_send_msg,
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
static const RECT g_setsRect = { SETS_LEFT, LIST_TOP, ITEMS_LEFT, LIST_BOTTOM };

#define WND(self) (&(self)->frame.base)

static unsigned set_count(const struct Package *list)
{
    return (unsigned)VEC_COUNT(&list->entries, struct PackageEntry *);
}

static unsigned item_count(const struct PackageEntry *set)
{
    return (unsigned)VEC_COUNT(&set->items, struct ItemInfo);
}

static int rect_contains(RECT rc, int x, int y)
{
    return rc.left < x && rc.right > x && rc.bottom > y && rc.top < y;
}

struct ChoicePackageItemBoxWnd *choice_package_item_box_wnd_construct(struct ChoicePackageItemBoxWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    WND(self)->ops = &choice_package_item_box_wnd_ops;
    self->over_sets = 1;
    self->box_id = 0;
    self->item_id = 0;
    self->list = NULL;
    self->sets_bar = NULL;
    self->selected = -1;
    self->first_set = 0;
    self->first_item = -1;
    self->items_bar = NULL;
    return self;
}

struct UIWindow *choice_package_item_box_wnd_new(void)
{
    return (struct UIWindow *)choice_package_item_box_wnd_construct(mem_alloc(sizeof(struct ChoicePackageItemBoxWnd)));
}

void choice_package_item_box_wnd_destruct(struct ChoicePackageItemBoxWnd *self)
{
    WND(self)->ops = &choice_package_item_box_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void choice_package_item_box_wnd_destroy(struct UIWindow *window)
{
    struct ChoicePackageItemBoxWnd *self = (struct ChoicePackageItemBoxWnd *)window;
    choice_package_item_box_wnd_destruct(self);
    mem_free(self);
}

struct ItemInfo *choice_package_item_box_wnd_item_at(struct ChoicePackageItemBoxWnd *self, int x, int y)
{
    struct PackageEntry *set;

    if (self->first_item == -1 || (unsigned)(y - (LIST_TOP + 1)) > 0xed || x < ITEMS_LEFT
        || WND(self)->width - 0xe < x)
        return NULL;
    set = package_entry_at(self->list, self->selected);
    if (!set)
        return NULL;
    return package_entry_item(set, (y - LIST_TOP) / ROW_HEIGHT + self->first_item);
}

struct String *package_set_get_name(struct PackageEntry *self, struct String *out)
{
    string_copy(out, &self->name);
    return out;
}

static struct UIBitmapButton *new_button(const char *name0, const char *name1, const char *name2)
{
    struct UIBitmapButton *mem = mem_alloc(sizeof(struct UIBitmapButton));
    struct UIBitmapButton *btn = mem ? ui_bitmap_button_construct(mem) : NULL;

    ui_bitmap_button_set_bitmap(btn, name0, 0, 0);
    ui_bitmap_button_set_bitmap(btn, name1, 1, 0);
    ui_bitmap_button_set_bitmap(btn, name2, 2, 0);
    ui_window_create(&btn->button.base, btn->bitmap_width, btn->bitmap_height);
    return btn;
}

static struct UIScrollBar *new_scroll_bar(void)
{
    struct UIScrollBar *mem = mem_alloc(sizeof(struct UIScrollBar));

    return mem ? ui_scroll_bar_construct(mem) : NULL;
}

void choice_package_item_box_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct ChoicePackageItemBoxWnd *self = (struct ChoicePackageItemBoxWnd *)window;
    struct UIBitmapButton *close, *select;
    struct UIScrollBar *bar;
    int length;

    close = new_button(PACKAGE_DIR "btn_close_0.bmp", PACKAGE_DIR "btn_close_1.bmp", PACKAGE_DIR "btn_close_2.bmp");
    ui_window_move_base(&close->button.base, WND(self)->width - close->bitmap_width - 0xe,
             WND(self)->height - close->bitmap_height - 4);
    close->button.base.id = UI_ID_CLOSE;
    ui_window_add_child(WND(self), &close->button.base);

    select = new_button(PACKAGE_DIR "btn_select_0.bmp", PACKAGE_DIR "btn_select_1.bmp",
                        PACKAGE_DIR "btn_select_2.bmp");
    ui_window_move_base(&select->button.base, close->button.base.x - select->bitmap_width - 0xa,
             WND(self)->height - select->bitmap_height - 4);
    select->button.base.id = ID_SELECT;
    ui_window_add_child(WND(self), &select->button.base);

    length = height - 0x2d;
    bar = new_scroll_bar();
    self->sets_bar = bar;
    ui_scroll_bar_create(bar, 1, LIST_TOP, UI_SCROLL_VERT, length);
    ui_window_set_show_base(&self->sets_bar->base, 0);
    ui_window_add_child(WND(self), &self->sets_bar->base);

    bar = new_scroll_bar();
    self->items_bar = bar;
    ui_scroll_bar_create(bar, WND(self)->width - 0xe, LIST_TOP, UI_SCROLL_VERT, length);
    ui_window_set_show_base(&self->items_bar->base, 0);
    ui_window_add_child(WND(self), &self->items_bar->base);
    (void)width;
}

void choice_package_item_box_wnd_on_draw(struct UIWindow *window)
{
    struct ChoicePackageItemBoxWnd *self = (struct ChoicePackageItemBoxWnd *)window;
    struct PackageEntry *set;
    struct ItemInfo *item;
    struct String name;
    struct String text = {0};
    const char *bitmap;
    int i, y;

    ui_window_clear_canvas(WND(self), 0xff000000);
    ui_window_draw_bitmap(WND(self), 0, 0, ui_skin_bitmap(PACKAGE_DIR "bg_select.bmp"), 0);
    ui_window_draw_title_bar(WND(self), 1, item_table_get(self->item_id)->name, 0);
    ui_window_add_close_button(WND(self), msg_str(MSG_CLOSE_TOOLTIP), 0);
    if (!self->list)
        return;

    for (i = 0, y = LIST_TOP; y < LIST_BOTTOM; i++, y += ROW_HEIGHT) {
        set = package_entry_at(self->list, self->first_set + i);
        if (set) {
            bitmap = self->first_set + i == self->selected ? PACKAGE_DIR "btn_set_press.bmp"
                                                           : PACKAGE_DIR "btn_set_list.bmp";
            ui_window_draw_bitmap(WND(self), SETS_LEFT, y, ui_skin_bitmap(bitmap), 0);
            string_copy(&name, &set->name);
            ui_window_fit_plain_text(WND(self), &text, string_view(&name), NAME_WIDTH);
            ui_window_text_out(WND(self), 0x18, y + 0xb, string_cstr(&text), 0, 0, 0xc, 0, 0, 0);
            string_free(&text);
            string_free(&name);
        } else {
            ui_window_draw_bitmap(WND(self), SETS_LEFT, y, ui_skin_bitmap(PACKAGE_DIR "btn_set_empty.bmp"), 0);
        }
    }

    set = package_entry_at(self->list, self->selected);
    if (!set)
        return;
    for (i = 0, y = LIST_TOP; y < LIST_BOTTOM; i++, y += ROW_HEIGHT) {
        char count_text[12];
        int digits;
        item = package_entry_item(set, self->first_item + i);
        if (!item)
            continue;
        if (item->expire_time)
            bitmap = item->option_count ? PACKAGE_DIR "item_list_timeran.bmp" : PACKAGE_DIR "item_list_time.bmp";
        else
            bitmap = item->option_count ? PACKAGE_DIR "item_list_ranop.bmp" : PACKAGE_DIR "item_list.bmp";
        ui_window_draw_bitmap(WND(self), ITEMS_LEFT, y, ui_skin_bitmap(bitmap), 0);
        ui_window_draw_item_icon(WND(self), 0xb2, y + 5, item, 0);
        digits = snprintf(count_text, sizeof count_text, "%d", item->count);
        ui_window_text_out(WND(self), 0xcb - digits * 5, y + 0x18, count_text, 0, 0, 0xa, 0,
                         0, 0);
        ui_window_draw_item_name_short(WND(self), 0xd9, y + 0xb, NAME_WIDTH, item, 0xc);
    }
}

void choice_package_item_box_wnd_reset_items(struct ChoicePackageItemBoxWnd *self)
{
    struct PackageEntry *set;
    int count, max_pos;

    self->first_item = 0;
    ui_window_set_show_base(&self->items_bar->base, 0);
    if (!self->list)
        return;
    set = package_entry_at(self->list, self->selected);
    if (!set)
        return;
    self->first_item = 0;
    count = (int)item_count(set);
    max_pos = count - ROWS;
    if (max_pos < 0)
        max_pos = 0;
    if (self->items_bar) {
        ui_scroll_bar_set_range(self->items_bar, max_pos, ROWS, count);
        ui_scroll_bar_set_pos(self->items_bar, self->first_item);
        ui_window_set_show_base(&self->items_bar->base, max_pos != 0);
    }
    ui_window_invalidate_base(&self->frame.base);
}

void choice_package_item_box_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct ChoicePackageItemBoxWnd *self = (struct ChoicePackageItemBoxWnd *)window;
    unsigned count;
    int idx;

    if (y < LIST_TOP) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }
    if (!self->list) {
        self->selected = -1;
        return;
    }
    if (!rect_contains(g_setsRect, x, y))
        return;
    idx = (self->first_set > 0 ? self->first_set : 0) + (y - LIST_TOP) / ROW_HEIGHT;
    self->selected = idx;
    count = set_count(self->list);
    if (count <= (unsigned)idx)
        idx = (int)count - 1;
    self->selected = idx;
    choice_package_item_box_wnd_reset_items(self);
    ui_window_invalidate_base(&self->frame.base);
}

void choice_package_item_box_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct ChoicePackageItemBoxWnd *self = (struct ChoicePackageItemBoxWnd *)window;
    self->over_sets = x <= ITEMS_LEFT;
    (void)y;
}

void choice_package_item_box_wnd_on_process(struct UIWindow *self)
{
    (void)self;
}

void choice_package_item_box_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct ChoicePackageItemBoxWnd *self = (struct ChoicePackageItemBoxWnd *)window;
    struct ItemInfo *item;
    struct UIWindow *wnd;
    RECT items;

    if (!self->list) {
        self->selected = -1;
        return;
    }
    items.left = ITEMS_LEFT;
    items.top = LIST_TOP;
    items.right = WND(self)->width - 0xe;
    items.bottom = LIST_BOTTOM;
    if (self->selected < 0 || !rect_contains(items, x, y))
        return;
    item = choice_package_item_box_wnd_item_at(self, x, y);
    if (!item)
        return;
    wnd = ui_window_open(ITEM_INFO_WINDOW);
    ui_window_send(wnd, NULL, MSG_ITEM_INFO, (int)item, 0, 0, 0);
    ui_window_invalidate_base(&self->frame.base);
}

void choice_package_item_box_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct ChoicePackageItemBoxWnd *self = (struct ChoicePackageItemBoxWnd *)window;
    struct UIScrollBar *bar = self->over_sets ? self->sets_bar : self->items_bar;

    if (!bar->base.visible)
        return;
    choice_package_item_box_wnd_send_msg(&self->frame.base, &bar->base, UI_MSG_SCROLL_V, delta < 0 ? 1 : -1, 0, 0, 0);
}

static int scroll_rows(int first, int lines, int count)
{
    first += lines;
    if ((unsigned)count < (unsigned)(first + ROWS))
        first = count - ROWS;
    if (first < 0)
        first = 0;
    return first;
}

int choice_package_item_box_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                                int val1, int val2, int val3, int val4)
{
    struct ChoicePackageItemBoxWnd *self = (struct ChoicePackageItemBoxWnd *)window;
    struct PackageEntry *set;
    struct String name, text;
    int answer, count, max_pos;

    switch (msg) {
    case UI_MSG_SCROLL_V:
    case UI_MSG_PAGE_UP:
    case UI_MSG_PAGE_DOWN:
        if (!self->list)
            return 0;
        if (sender == &self->sets_bar->base) {
            if (!sender->visible) {
                self->first_item = 0;
                return 0;
            }
            self->first_set = scroll_rows(self->first_set, val1, (int)set_count(self->list));
            ui_scroll_bar_set_pos(self->sets_bar, self->first_set);
            ui_window_invalidate_base(&self->frame.base);
            ui_window_invalidate_base(&self->sets_bar->base);
            return val1;
        }
        if (sender != &self->items_bar->base)
            return 0;
        if (!sender->visible) {
            self->first_item = 0;
            return 0;
        }
        set = package_entry_at(self->list, self->selected);
        if (!set)
            return -1;
        self->first_item = scroll_rows(self->first_item, val1, (int)item_count(set));
        ui_scroll_bar_set_pos(self->items_bar, self->first_item);
        ui_window_invalidate_base(&self->frame.base);
        ui_window_invalidate_base(&self->items_bar->base);
        return val1;

    case UI_MSG_COMMAND:
        switch (val1) {
        case ID_SELECT:
            if (self->selected < 0)
                return 0;
            set = package_entry_at(self->list, self->selected);
            if (!set || (unsigned)self->selected >= set_count(self->list))
                return 0;
            package_set_get_name(set, &name);
            string_init(&text);
            string_format(&text, msg_str(MSG_CONFIRM), string_cstr(&name));
            answer = ui_window_mgr_message_box(&g_windowMgr, string_cstr(&text), UI_MSGBOX_QUESTION, 1, 0, 0,
                                            "Message", 0x118, 0x78, 0);
            string_free(&text);
            string_free(&name);
            if (answer != ANSWER_NO)
                game_send_message(MODE_REQ_CHOOSE_PACKAGE, self->box_id,
                              self->item_id, set->id, 0);
            return 0;
        case UI_ID_CLOSE:
            ui_window_mgr_post_quit1(&g_windowMgr, WND(self)->id);
            return 0;
        case ID_REFRESH:
            choice_package_item_box_wnd_reset_items(self);
            return 0;
        }
        break;

    case MSG_SET_LIST:
        self->list = (struct Package *)val1;
        if (!self->list)
            return 0;
        self->item_id = val2;
        self->box_id = val3;
        self->selected = -1;
        self->first_set = 0;
        count = (int)set_count(self->list);
        max_pos = count - ROWS;
        if (max_pos < 0)
            max_pos = 0;
        if (self->sets_bar) {
            ui_scroll_bar_set_range(self->sets_bar, max_pos, ROWS, count);
            ui_scroll_bar_set_pos(self->sets_bar, self->first_set);
            ui_window_set_show_base(&self->sets_bar->base, max_pos != 0);
        }
        self->first_item = -1;
        ui_window_set_show_base(&self->items_bar->base, 0);
        ui_window_invalidate_base(&self->frame.base);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}
