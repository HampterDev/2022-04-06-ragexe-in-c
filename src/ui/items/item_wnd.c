#include "ui/controls/buttons.h"
#include "ui/items/items.h"
#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "render/texture_cache.h"
#include "render/texture_types.h"
#include "ui/misc/snap_mgr.h"
#include "session/state.h"
#include "ui/window_info.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include "ui/shops/shops.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/runtime.h"
#include "ui/render/render.h"
#include <mmsystem.h>
#include "ui/chat/chat_window_parts.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_lua.h"
#include "session/session_state.h"
#include "ui/items/refining_internal.h"
#include "ui/quests/npc_input.h"
#include "ui/shops/merchant.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/controls/popups.h"

#define GRID_LEFT 0x28
#define GRID_TOP 0x11
#define GRID_CELL 32
#define GRID_MARGIN 0x26
#define STRIP_HEIGHT 8
#define ITEM_WND_MIN_WIDTH 0xe8
#define ITEM_WND_MAX_WIDTH 0x140
#define ITEM_WND_MIN_HEIGHT 0x66
#define ITEM_WND_MAX_HEIGHT 0xf0
#define ITEM_WND_WIDTH_ORIGIN 0x118

enum ItemWndTab {
    ITEM_TAB_USABLE = 0,
    ITEM_TAB_EQUIP = 1,
    ITEM_TAB_ETC = 2,
    ITEM_TAB_FAVOURITE = 3,
};
#define USABLE_FILTER_ALL 3
#define EQUIP_FILTER_ALL 3
#define ETC_FILTER_ALL 4

#define ID_DROP_LOCK UI_ID_CHECKBOX
#define ID_SORT 0x164
#define ID_COMPARE 0x1c0
#define ID_DEAL_LOCK 0x1f3
#define ID_ITEM_NUMBER 0x219

#define ITEM_WND_MSG_REFRESH 0x17
#define ITEM_WND_MSG_ADD_DRAGGED UI_MSG_ADD_DRAGGED
#define UI_MSG_SET_TAB UI_MSG_TAB_CHANGED
#define SWAP_MSG_SET_ITEM 0x17

#define MODE_REQ_EQUIP 0x13
#define MODE_REQ_TAKEOFF_EQUIP 0x14
#define MODE_REQ_USE_ITEM 0x1b
#define MODE_REQ_ADD_EXCHANGE_ITEM 0x33
#define MODE_REQ_TO_STORAGE 0x37
#define MODE_REQ_FROM_STORAGE 0x38
#define MODE_REQ_ITEM_TO_CART 0x4c
#define MODE_REQ_ITEM_FROM_CART 0x4d
#define MODE_REQ_WEAR_EQUIP_DIRECT 0x57
#define MODE_REQ_ITEM_COMPOSITION_LIST 0x7b
#define MODE_REQ_SWAP_EQUIP 0x12e
#define MODE_REQ_REMOVE_SWAP_EQUIP 0x12f
#define MODE_REQ_ITEM_NUMBER 0x136
#define RODEX_MSG_ADD_ITEM 0x68

#define WND_ID_INVENTORY 8
#define WND_ID_REFINING 0x127
#define WND_ID_GRADE_ENCHANT 0x157

#define CZ_INVENTORY_TAB 0x907
#define CZ_ONECLICK_IDENTIFY 0xa35
#define ITEM_ID_MAGNIFIER 0x263
#define ITEM_ID_MAGNIFIER_2 0x3025
#define ITEM_ID_CONFIRM_USE 0x3202

#define MSG_INVENTORY_TITLE 0x6a
#define MSG_ITEM_TOOLTIP 0xb7
#define MSG_CONFIRM_OPEN_CASH_ITEM 0x4ca
#define MSG_TAB_USABLE 0x5b9
#define MSG_TAB_EQUIP 0x802
#define MSG_TAB_ETC 0x5bf
#define MSG_TAB_FAVOURITE 0x803
#define MSG_CONFIRM_OPEN_BOUND_BOX 0x945
#define MSG_ITEM_TOOLTIP_OPTIONS 0x9f7
#define MSG_NO_MAGNIFIER 0xaaf
#define MSG_COMPARE_TIP 0xb1a
#define MSG_DROP_LOCK_TIP 0xb1b
#define MSG_DEAL_LOCK_TIP 0xb88
#define MSG_SORT_TIP 0xbbf
#define MSG_ITEM_NUMBER_TIP 0xde8

SIZE_ASSERT(ItemWndInfo, 0x28);

OFFSET_ASSERT(UIItemWnd, info, 0xd0);
OFFSET_ASSERT(UIItemWnd, tab, 0xec);
OFFSET_ASSERT(UIItemWnd, sort, 0x10c);
SIZE_ASSERT(UIItemWnd, 0x110);
struct DragInfo;
void ui_item_wnd_destroy(struct UIWindow *window);
void ui_item_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_item_wnd_on_process(struct UIWindow *self);
void ui_item_wnd_on_draw(struct UIWindow *self);
void ui_item_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_item_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_item_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_item_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_item_wnd_ops = {
    .destroy = ui_item_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_item_wnd_drag_and_drop,
    .store_info = ui_item_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_wnd_on_create,
    .on_destroy = ui_slot_no_op,
    .on_process = ui_item_wnd_on_process,
    .on_draw = ui_item_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_item_wnd_send_msg,
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

#define DEAL_LOCKED(self) ((self)->tab == ITEM_TAB_FAVOURITE && g_session1.blockPrivateTab == 1)

struct UIItemWnd *ui_item_wnd_construct(struct UIItemWnd *self)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_item_wnd_ops;
    self->deal_lock = NULL;
    self->deal_lock_notice = NULL;
    self->sort = NULL;
    self->base.resizer = NULL;
    self->base.scroll_bar = NULL;
    self->info = NULL;
    self->usable_filter = 0;
    self->equip_filter = 0;
    self->etc_filter = 0;
    self->tab = 0;
    self->base.scroll = 0;
    self->base.scrollable = 0;
    return self;
}

struct UIWindow *ui_item_wnd_new(void)
{
    return (struct UIWindow *)ui_item_wnd_construct(mem_alloc(sizeof(struct UIItemWnd)));
}

void ui_item_wnd_destruct(struct UIItemWnd *self)
{
    self->base.frame.base.ops = &ui_item_wnd_ops;
    ui_item_base_wnd_destruct(&self->base);
}

void ui_item_wnd_destroy(struct UIWindow *window)
{
    struct UIItemWnd *self = (struct UIItemWnd *)window;
    ui_item_wnd_destruct(self);
    mem_free(self);
}

void ui_slot_no_op(struct UIWindow *self)
{
}

void item_wnd_info_set_default(struct ItemWndInfo *self)
{
    self->rect.x = 0;
    self->rect.y = 0x78;
    self->rect.width = 0x118;
    self->rect.height = 0x78;
    self->org_height = 0x78;
    self->cur_tab = ITEM_TAB_USABLE;
    self->cur_radio[0] = USABLE_FILTER_ALL;
    self->cur_radio[1] = EQUIP_FILTER_ALL;
    self->cur_radio[2] = ETC_FILTER_ALL;
    self->show = 0;
}

int item_wnd_info_is_valid(struct ItemWndInfo *self)
{
    if (self->rect.x < 0 || self->rect.x > g_renderer->width - self->rect.width)
        return 0;
    if (self->rect.y < 0 || self->rect.y > g_renderer->height - self->rect.height)
        return 0;
    if ((unsigned)self->cur_tab > ITEM_TAB_FAVOURITE || (unsigned)self->cur_radio[0] > USABLE_FILTER_ALL ||
        (unsigned)self->cur_radio[1] > EQUIP_FILTER_ALL || (unsigned)self->cur_radio[2] > ETC_FILTER_ALL)
        return 0;
    return 1;
}

void ui_item_wnd_store_info(struct UIWindow *window)
{
    struct UIItemWnd *self = (struct UIItemWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    if (!self->info)
        return;
    self->info->rect.width = wnd->width;
    self->info->rect.height = wnd->height;
    self->info->rect.x = wnd->x;
    self->info->rect.y = wnd->y;
    self->info->org_height = self->full_height;
    self->info->cur_tab = self->tab;
    self->info->cur_radio[0] = self->usable_filter;
    self->info->cur_radio[1] = self->equip_filter;
    self->info->cur_radio[2] = self->etc_filter;
}

static struct UIBitmapButton *item_wnd_button(struct UIBitmapButton *button, const char *up, const char *over,
                                              const char *press)
{
    ui_bitmap_button_set_bitmap(button, up, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, over, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, press, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    return button;
}

void ui_item_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemWnd *self = (struct UIItemWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIResizer *resizer;
    struct UIScrollBar *scroll;
    struct UITabControl *tabs;
    struct UIWindow *box;
    struct UIBitmapButton *button;
    const char *tab_names[4];
    int i;

    resizer = mem_alloc(sizeof *resizer);
    resizer = resizer ? ui_resizer_construct(resizer) : NULL;
    self->base.resizer = resizer;
    ui_window_create(&resizer->base, 0x10, 0x10);
    ui_resizer_set_edge(resizer, UI_EDGE_BOTTOM_RIGHT);
    ui_resizer_set_bitmap(resizer, UI_SKIN "btn_resize.bmp");
    ui_window_move_base(&resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours(resizer, UI_TRANSPARENT, UI_TRANSPARENT);
    ui_window_add_child(wnd, &resizer->base);

    scroll = mem_alloc(sizeof *scroll);
    scroll = scroll ? ui_scroll_bar_construct(scroll) : NULL;
    self->base.scroll_bar = scroll;
    ui_scroll_bar_create(scroll, wnd->width - 0xf, GRID_TOP, UI_SCROLL_VERT, wnd->height - GRID_MARGIN);
    ui_window_add_child(wnd, &scroll->base);

    self->sys_button = UI_ADD_SYS_BUTTON(wnd);
    self->mini_button = ui_window_add_mini_button(wnd, "Ctrl+E", 0);
    self->close_button = ui_window_add_close_button(wnd, "Alt+E", 0);
    self->full_height = wnd->height;
    self->base.columns = (width - GRID_LEFT) / GRID_CELL;
    self->base.rows = (height - GRID_MARGIN) / GRID_CELL;
    ui_window_send_msg(wnd, NULL, ITEM_WND_MSG_REFRESH, 0, 0, 0, 0);

    tabs = mem_try_alloc(sizeof *tabs);
    tabs = tabs ? (struct UITabControl *)ui_tab_control_construct(&tabs->base) : NULL;
    self->tabs = tabs;
    ui_tab_control_setup(&tabs->base, UI_TABS_LEFT, 0x11, 0xa, 1, 0, 1, 0, 0, 0, 0, 0, 0);
    ui_window_move_base(&tabs->base, 3, 0x12);
    ui_window_add_child(wnd, &tabs->base);
    tab_names[ITEM_TAB_USABLE] = msg_str(MSG_TAB_USABLE);
    tab_names[ITEM_TAB_EQUIP] = msg_str(MSG_TAB_EQUIP);
    tab_names[ITEM_TAB_ETC] = msg_str(MSG_TAB_ETC);
    tab_names[ITEM_TAB_FAVOURITE] = msg_str(MSG_TAB_FAVOURITE);
    for (i = 0; i < 4; i++)
        ui_tab_control_add_tab(&self->tabs->base, tab_names[i]);
    ((DWORD *)self->tabs->colours.first)[ITEM_TAB_FAVOURITE] = 0xffdece;
    ui_window_invalidate_base(&self->tabs->base);

    box = mem_try_alloc(sizeof(struct UICheckBox));
    box = box ? (struct UIWindow *)ui_check_box_construct(box) : NULL;
    self->drop_lock = box;
    ui_window_create(box, 0xe, 0xb);
    ui_check_box_set_bitmaps_fit((struct UICheckBox *)self->drop_lock,
                             UI_SKIN "inventory\\item_drop_lock_on.bmp", UI_SKIN "inventory\\item_drop_lock_off.bmp");
    ui_window_move(self->drop_lock, 0x4f, height - 0x10);
    ui_check_box_set_check(self->drop_lock, g_session1.itemDropLock);
    ui_check_box_set_tool_tip(self->drop_lock, msg_str(MSG_DROP_LOCK_TIP));
    ui_window_add_child(wnd, self->drop_lock);

    box = mem_try_alloc(sizeof(struct UICheckBox));
    box = box ? (struct UIWindow *)ui_check_box_construct(box) : NULL;
    self->compare = box;
    ui_window_create(box, 0xc, 0xc);
    ui_check_box_set_bitmaps_fit((struct UICheckBox *)self->compare, UI_SKIN "inventory\\item_compare_on.bmp",
                             UI_SKIN "inventory\\item_compare_off.bmp");
    self->compare->id = ID_COMPARE;
    ui_window_move(self->compare, 0x67, height - 0x11);
    ui_check_box_set_check(self->compare, g_session1.itemCompare);
    ui_check_box_set_tool_tip(self->compare, msg_str(MSG_COMPARE_TIP));
    ui_window_add_child(wnd, self->compare);

    button = mem_try_alloc(sizeof *button);
    button = button ? ui_bitmap_button_construct(button) : NULL;
    self->item_number = button;
    item_wnd_button(button, UI_SKIN "inventory\\icon_num.bmp", UI_SKIN "inventory\\icon_num.bmp",
                    UI_SKIN "inventory\\icon_num.bmp");
    ui_window_move_base(&self->item_number->button.base, 0x17, height - 0x11);
    self->item_number->button.base.id = ID_ITEM_NUMBER;
    ui_bitmap_button_set_tool_tip(self->item_number, msg_str(MSG_ITEM_NUMBER_TIP));
    ui_window_add_child(wnd, &self->item_number->button.base);

    button = mem_alloc(sizeof *button);
    button = button ? ui_bitmap_button_construct(button) : NULL;
    self->deal_lock = button;
    item_wnd_button(button,
                    g_session1.blockPrivateTab == 1 ? UI_SKIN "inventory\\bt_itemdeal_lock_on.bmp"
                                      : UI_SKIN "inventory\\bt_itemDeal_lock_off.bmp",
                    UI_SKIN "inventory\\bt_itemDeal_lock_over.bmp", UI_SKIN "inventory\\bt_itemDeal_lock_press.bmp");
    ui_window_move_base(&self->deal_lock->button.base, 0x7c, wnd->height - 0x13);
    ui_bitmap_button_set_tool_tip(self->deal_lock, msg_str(MSG_DEAL_LOCK_TIP));
    self->deal_lock->button.base.id = ID_DEAL_LOCK;
    ui_window_add_child(wnd, &self->deal_lock->button.base);

    button = mem_alloc(sizeof *button);
    button = button ? ui_bitmap_button_construct(button) : NULL;
    self->sort = button;
    item_wnd_button(button, UI_SKIN "inventory\\bt_sort.bmp", UI_SKIN "inventory\\bt_sort_over.bmp",
                    UI_SKIN "inventory\\bt_sort_press.bmp");
    ui_window_move_base(&self->sort->button.base, 0x9d, wnd->height - 0x13);
    ui_bitmap_button_set_tool_tip(self->sort, msg_str(MSG_SORT_TIP));
    self->sort->button.base.id = ID_SORT;
    ui_window_add_child(wnd, &self->sort->button.base);
}

void ui_item_wnd_on_process(struct UIWindow *window)
{
    struct UIItemWnd *self = (struct UIItemWnd *)window;
    if (self->deal_lock_notice)
        ui_notice_message_box_lock_sell_item_on_process(self->deal_lock_notice);
}

static void item_wnd_draw_texture(struct UIWindow *wnd, int x, int y, int w, int h, struct Texture *tex,
                                  DWORD argb)
{
    dc_bitmap_draw_texels_scaled(wnd->canvas, x, y, w, h, tex, ((unsigned)tex->reqWidth), ((unsigned)tex->reqHeight), 1.0f, 1.0f, argb, 0,
                                  0, 0, 0, 0);
}

void ui_item_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemWnd *self = (struct UIItemWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct ListNode *node;
    struct String text;
    COLORREF colour;
    int body, strips, i, x, y, row, column, rows, columns, index, cell, total, width;

    ui_window_draw_title_bar(wnd, 1, msg_str(MSG_INVENTORY_TITLE), 0);
    if (wnd->height == GRID_TOP)
        return;
    if (wnd->height > 0x78) {
        body = wnd->height - GRID_MARGIN;
        strips = body / STRIP_HEIGHT + (body % STRIP_HEIGHT != 0);
        for (i = 0, y = GRID_TOP; i < strips; i++, y += STRIP_HEIGHT)
            ui_window_draw_bitmap(wnd, 0, y,
                                res_mgr_get(g_resMgr(),
                                             ui_skin_path(UI_SKIN "basic_interface\\itemwin_left.bmp")),
                                0);
    }
    body = wnd->height - GRID_MARGIN;
    strips = body / STRIP_HEIGHT + (body % STRIP_HEIGHT != 0);
    for (i = 0, y = GRID_TOP; i < strips; i++, y += STRIP_HEIGHT) {
        struct BitmapRes *right = res_mgr_get(g_resMgr(),
                                                ui_skin_path(UI_SKIN "basic_interface\\itemwin_right.bmp"));

        ui_window_draw_bitmap(wnd, wnd->width - right->width, y, right, 0);
    }
    if (DEAL_LOCKED(self)) {
        colour = ui_skin_colour(0, 8);
        ui_window_fill_rect(wnd, 0x14, GRID_TOP, wnd->width - 0x15, wnd->height - GRID_MARGIN, colour);
    } else {
        colour = ui_skin_colour(2, 2);
        body = wnd->height - GRID_MARGIN;
        ui_window_fill_rect(wnd, 0x14, GRID_TOP, (wnd->width - 0x3c) % GRID_CELL, body, colour);
        ui_window_fill_rect(wnd, 0x14, body / GRID_CELL * GRID_CELL + GRID_TOP, wnd->width - 0x18,
                          body % GRID_CELL, colour);
    }
    if (self->deal_lock_notice)
        ui_window_set_show_base(self->deal_lock_notice, DEAL_LOCKED(self) ? 1 : 0);
    if (self->deal_lock)
        ui_button_show((struct UIButton *)(self->deal_lock), self->tab == ITEM_TAB_FAVOURITE ? 1 : 0);
    if (self->sort)
        ui_button_show((struct UIButton *)(self->sort), self->tab == ITEM_TAB_FAVOURITE ? 1 : 0);
    ui_window_draw_button_bar(wnd, 0, 0, 1);
    ui_window_fill_rect(wnd, 0x14, GRID_TOP, 1, wnd->height - GRID_MARGIN, 0xc0c0c0);
    ui_window_fill_rect(wnd, 0x12, wnd->height - 0x15, 2, 1, 0xffffff);

    columns = (wnd->width - GRID_LEFT) / GRID_CELL;
    rows = (wnd->height - GRID_MARGIN) / GRID_CELL;
    for (row = 0, y = GRID_TOP; row < rows; row++, y += GRID_CELL)
        for (column = 0, x = GRID_LEFT; column < columns; column++, x += GRID_CELL) {
            if (DEAL_LOCKED(self))
                ui_window_draw_bitmap(wnd, x, y,
                                    res_mgr_get(g_resMgr(),
                                                 ui_skin_path(UI_SKIN "inventory\\itemwin_mid_lock.bmp")),
                                    0);
            else
                ui_window_draw_item_win_mid(wnd, x, y);
        }

    cell = 0;
    for (node = self->base.items.head->next, index = 0; node != self->base.items.head; node = node->next, index++) {
        if (index < self->base.scroll * self->base.columns || item_grid_hidden(LIST_VALUE(node, struct ItemInfo)))
            continue;
        x = cell % self->base.columns * GRID_CELL + GRID_LEFT;
        y = cell / self->base.columns * GRID_CELL + GRID_TOP;
        if (LIST_VALUE(node, struct ItemInfo)->is_new) {
            if (DEAL_LOCKED(self))
                ui_window_draw_bitmap(wnd, x, y,
                                    res_mgr_get(g_resMgr(),
                                                 ui_skin_path(UI_SKIN "inventory\\itemwin_new_lock.bmp")),
                                    0);
            else
                ui_window_draw_new_item_mark(wnd, x, y);
        }
        if (++cell == self->base.rows * self->base.columns)
            break;
    }
    cell = 0;
    for (node = self->base.items.head->next, index = 0; node != self->base.items.head; node = node->next, index++) {
        struct ItemInfo *info = LIST_VALUE(node, struct ItemInfo);

        if (index < self->base.scroll * self->base.columns || item_grid_hidden(info))
            continue;
        x = cell % self->base.columns * GRID_CELL + 0x2c;
        y = cell / self->base.columns * GRID_CELL;
        ui_window_draw_item_icon_count(wnd, x, y + 0x15, info, 0, 0);
        if (info->grade > 0) {
            struct String path;

            string_init(&path);
            string_format(&path, UI_SKIN "grade_enchant\\grade_icon%d.bmp", (int)info->grade);
            ui_window_draw_bitmap(wnd, x, y + 0x21, res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
            string_free(&path);
        }
        if (session_is_in_switch_sets(&g_session1, info->index)) {
            struct Texture *tex;

            tex = texture_cache_get(&g_textureCache, UI_SKIN "swap_equipment\\bg_change.bmp", 0, 0);
            if (tex)
                item_wnd_draw_texture(wnd, x, y + 0x15, 0x18, 0x18, tex, 0x22ffffff);
            tex = texture_cache_get(&g_textureCache, UI_SKIN "swap_equipment\\ico_change.bmp", 0, 0);
            if (tex)
                item_wnd_draw_texture(wnd, x, y + 0x15, ((unsigned)tex->reqWidth), ((unsigned)tex->reqHeight), tex, 0xffffffff);
        }
        if (++cell == self->base.rows * self->base.columns)
            break;
    }

    total = session_get_inventory_count(&g_session1) + session_count_worn_equips(&g_session1) +
            session_count_worn_costumes(&g_session1);
    string_init(&text);
    string_format(&text, "%d", total);
    ui_window_text_out(wnd, 0x23, wnd->height - 0xf, string_cstr(&text), 0, 0, 0xb,
                     total >= g_session1.inventoryExtension + 100 ? 0xff : 0x232323, 0, 0);
    string_free(&text);
    string_format(&text, "%d", total);
    width = ui_window_text_width(wnd, string_cstr(&text), 0, 0, 0xb, 0, 0);
    string_free(&text);
    string_format(&text, "/%d", g_session1.inventoryExtension + 100);
    ui_window_text_out(wnd, width + 0x24, wnd->height - 0xf, string_cstr(&text), 0, 0, 0xb, 0, 0, 0);
    string_free(&text);
}

struct ItemInfo *ui_item_wnd_item_at(struct UIItemWnd *self, struct ItemInfo *out, int x, int y)
{
    struct UIWindow *wnd = &self->base.frame.base;
    struct ListNode *node;
    int column, row, index, cell = 0;

    if (x >= GRID_LEFT && x < wnd->width - 0x14 && y >= GRID_TOP && y < self->base.rows * GRID_CELL + GRID_TOP) {
        column = (x - GRID_LEFT) / GRID_CELL;
        row = (y - GRID_TOP) / GRID_CELL;
        for (node = self->base.items.head->next, index = 0; node != self->base.items.head;
             node = node->next, index++) {
            if (index < self->base.columns * self->base.scroll || item_grid_hidden(LIST_VALUE(node, struct ItemInfo)))
                continue;
            if (cell == self->base.columns * row + column)
                return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
            cell++;
        }
    }
    return item_info_construct_with_arg(out);
}

static int less_by_type_name(const void *a, const void *b)
{
    const struct ItemInfo *item_a = a, *item_b = b;
    int rank_a = item_type_rank(item_a->type), rank_b = item_type_rank(item_b->type);
    struct String name_a = {0};
    struct String name_b = {0};
    int less;

    if (rank_a != rank_b)
        return rank_a < rank_b;
    item_info_get_display_name(item_b, &name_b);
    item_info_get_display_name(item_a, &name_a);
    less = string_compare(&name_a, &name_b) < 0;
    string_free(&name_a);
    string_free(&name_b);
    return less;
}

void ui_item_wnd_sort_favourites(struct UIItemWnd *self)
{
    struct List *items = &self->base.items;
    struct ItemInfo info;
    int i;

    list_clear(items, item_info_destroy);
    for (i = 0; i < session_get_inventory_count(&g_session1); i++) {
        session_get_inventory_item_at(&g_session1, &info, i);
        if (info.favorite == 1)
            item_info_copy(list_push_back(items, NULL, sizeof(struct ItemInfo)), &info);
        item_info_destruct(&info);
    }
    list_sort(items, less_by_type_name);
    ui_window_invalidate_base(&self->base.frame.base);
    ui_item_base_wnd_update_scroll(&self->base);
}

static void item_wnd_take_drag(void)
{
    struct Mode *game = game_current_base_mode();
    const struct DragInfo *drag = game_current_drag();

    switch (drag->source) {
    case 2:
        mode_send_message(game, MODE_REQ_TAKEOFF_EQUIP, drag->count, 0, 0, 0);
        break;
    case 0x18:
        mode_send_message(game, MODE_REQ_TAKEOFF_EQUIP, drag->count, 1, 0, 0);
        break;
    case 6:
        mode_send_message(game, MODE_REQ_FROM_STORAGE, drag->index, drag->drop_count, 0, 0);
        break;
    case 9:
        mode_send_message(game, MODE_REQ_ITEM_FROM_CART, drag->index, drag->drop_count, 0, 0);
        break;
    case 0xf:
        if (g_windowMgr.rodex_compose_wnd)
            ui_window_send_msg(g_windowMgr.rodex_compose_wnd, NULL, RODEX_MSG_ADD_ITEM, drag->index,
                     drag->drop_count, 0, 0);
        break;
    case 0x11:
        mode_send_message(game, MODE_REQ_REMOVE_SWAP_EQUIP, drag->count, 0, 0, 0);
        break;
    case 0x12:
        mode_send_message(game, MODE_REQ_REMOVE_SWAP_EQUIP, drag->count, 1, 0, 0);
        break;
    }
    mode_send_message(game, MODE_REQ_DRAG_END, 0, 0, 0, 0);
}

static void item_wnd_refresh(struct UIItemWnd *self)
{
    struct ItemInfo info;
    int count, i, take;

    list_clear(&self->base.items, item_info_destroy);
    count = session_get_inventory_count(&g_session1);
    for (i = 0; i < count; i++) {
        session_get_inventory_item_at(&g_session1, &info, i);
        take = 0;
        if (info.index > 0) {
            int type = info.type;

            if (info.favorite) {
                take = self->tab == ITEM_TAB_FAVOURITE;
            } else if (self->tab == ITEM_TAB_USABLE) {
                if (type == 0 || type == 1 || type == 2 || type == 0x12) {
                    switch (self->usable_filter) {
                    case 0:
                        take = type == 0;
                        break;
                    case 2:
                        take = type == 1 || type == 2;
                        break;
                    case USABLE_FILTER_ALL:
                        take = 1;
                        break;
                    }
                }
            } else if (self->tab == ITEM_TAB_EQUIP) {
                if (type >= 4 && type <= 0xf && type != 6 && type != 7 && type != 0xa) {
                    switch (self->equip_filter) {
                    case 0:
                        take = type == 5 || type == 9 || type == 8;
                        break;
                    case 1:
                        take = type == 4 || (type > 0xa && type <= 0xe);
                        break;
                    case EQUIP_FILTER_ALL:
                        take = 1;
                        break;
                    }
                }
            } else if (self->tab == ITEM_TAB_ETC) {
                if (type == 6 || type == 3 || type == 7 || type == 0xa || type == 0x10 || type == 0x11 ||
                    type == 0x13)
                    take = self->etc_filter == ETC_FILTER_ALL;
            }
            if (take)
                item_info_copy(list_push_back(&self->base.items, NULL, sizeof(struct ItemInfo)), &info);
        }
        item_info_destruct(&info);
    }
    ui_window_invalidate_base(&self->base.frame.base);
    ui_item_base_wnd_update_scroll(&self->base);
}

static void item_wnd_toggle_deal_lock(struct UIItemWnd *self)
{
    struct UIWindow *wnd = &self->base.frame.base;

    if (g_session1.blockPrivateTab == 1) {
        if (self->deal_lock_notice)
            ui_window_send_msg(wnd, self->deal_lock_notice, UI_MSG_COMMAND, UI_ID_CLOSE, 0, 0, 0);
        g_session1.blockPrivateTab = 0;
        ui_bitmap_button_set_bitmap(self->deal_lock, UI_SKIN "inventory\\bt_itemDeal_lock_off.bmp", UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(self->deal_lock, UI_SKIN "inventory\\bt_itemDeal_lock_over.bmp",
                                 UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(self->deal_lock, UI_SKIN "inventory\\bt_itemDeal_lock_press.bmp",
                                 UI_BTN_DOWN, 0);
    } else {
        if (!self->deal_lock_notice) {
            self->deal_lock_notice = (struct UIWindow *)ui_notice_message_box_lock_sell_item_new();
            ui_window_create(self->deal_lock_notice, 0xf2, 0x1a);
            ui_frame_wnd_move(self->deal_lock_notice, 0x1e, wnd->height - 0x32);
            ui_window_add_child(wnd, self->deal_lock_notice);
        }
        g_session1.blockPrivateTab = 1;
        ui_bitmap_button_set_bitmap(self->deal_lock, UI_SKIN "inventory\\bt_itemDeal_lock_on.bmp", UI_BTN_UP, 0);
        ui_bitmap_button_set_bitmap(self->deal_lock, UI_SKIN "inventory\\bt_itemDeal_lock_on_over.bmp",
                                 UI_BTN_HOVER, 0);
        ui_bitmap_button_set_bitmap(self->deal_lock, UI_SKIN "inventory\\bt_itemDeal_lock_on_press.bmp",
                                 UI_BTN_DOWN, 0);
    }
    ui_window_invalidate_base(&self->deal_lock->button.base);
    ui_window_invalidate_base(wnd);
}

static void item_wnd_place_children(struct UIItemWnd *self)
{
    struct UIWindow *wnd = &self->base.frame.base;

    ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
    ui_window_move(self->drop_lock, 0x4f, wnd->height - 0x10);
    ui_window_move(self->compare, 0x67, wnd->height - 0x11);
    ui_window_move_base(&self->item_number->button.base, 0x17, wnd->height - 0x11);
    ui_window_move_base(&self->deal_lock->button.base, 0x7c, wnd->height - 0x13);
    if (self->deal_lock_notice)
        ui_frame_wnd_move(self->deal_lock_notice, 0x1e, wnd->height - 0x32);
    ui_window_move_base(&self->sort->button.base, 0x9d, wnd->height - 0x13);
}

int ui_item_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                 int val3, int val4)
{
    struct UIItemWnd *self = (struct UIItemWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    switch (msg) {
    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_CLOSE:
            if (self->deal_lock_notice == sender) {
                ui_window_remove_child(wnd, self->deal_lock_notice, 0);
                self->deal_lock_notice = NULL;
                ui_window_invalidate_base(wnd);
            } else {
                ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_INVENTORY);
            }
            return 0;
        case UI_ID_HELP: {
            int old = wnd->height;

            if (old == GRID_TOP) {
                ui_window_resize_base(wnd, wnd->width, self->full_height);
            } else {
                self->full_height = old;
                ui_window_resize_base(wnd, wnd->width, GRID_TOP);
            }
            ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
            ui_window_mgr_raise_window(&g_windowMgr, wnd);
            if (wnd->height - old)
                snap_mgr_move_chain(&g_snapMgr, wnd, 0, wnd->height - old, 3);
            return 0;
        }
        case ID_DROP_LOCK:
            g_session1.itemDropLock = g_session1.itemDropLock == 0;
            return 0;
        case ID_SORT:
            if (self->tab != ITEM_TAB_FAVOURITE)
                return 0;
            g_session1.favouritesSorted = 1;
            ui_item_wnd_sort_favourites(self);
            return 0;
        case ID_COMPARE:
            g_session1.itemCompare = self->compare->state > 0;
            return 0;
        case ID_DEAL_LOCK:
            item_wnd_toggle_deal_lock(self);
            return 0;
        case ID_ITEM_NUMBER: {
            struct Mode *game = game_current_base_mode();

            if (game)
                mode_send_message(game, MODE_REQ_ITEM_NUMBER, 0, 0, 0, 0);
            return 0;
        }
        }
        return 0;
    case UI_MSG_RESIZE: {
        int width, height, tabs_height, body;

        if (val1 != UI_EDGE_BOTTOM_RIGHT)
            return 0;
        width = val2 > ITEM_WND_MAX_WIDTH ? ITEM_WND_MAX_WIDTH : val2;
        height = val3 > ITEM_WND_MAX_HEIGHT ? ITEM_WND_MAX_HEIGHT : val3;
        if (width < ITEM_WND_MIN_WIDTH)
            width = ITEM_WND_MIN_WIDTH;
        if (height < ITEM_WND_MIN_HEIGHT)
            height = ITEM_WND_MIN_HEIGHT;
        tabs_height = self->tabs->base.height;
        if (height < tabs_height + GRID_MARGIN)
            height = tabs_height + GRID_MARGIN;
        body = (height - tabs_height - GRID_MARGIN) / GRID_CELL * GRID_CELL + tabs_height;
        height = body + GRID_MARGIN;
        width = (width - ITEM_WND_WIDTH_ORIGIN) / GRID_CELL * GRID_CELL + ITEM_WND_WIDTH_ORIGIN;
        if (width != wnd->width || height != wnd->height) {
            int x[3];

            ui_window_resize_base(wnd, width, height);
            self->base.columns = (width - GRID_LEFT) / GRID_CELL;
            self->base.rows = body / GRID_CELL;
            ui_window_move_base(&self->base.resizer->base, wnd->width - 0xf, wnd->height - 0xf);
            ui_window_move_base(&self->base.scroll_bar->base, wnd->width - 0xf, GRID_TOP);
            ui_scroll_bar_resize(&self->base.scroll_bar->base, self->base.scroll_bar->base.width, wnd->height - GRID_MARGIN);
            ui_item_base_wnd_update_scroll(&self->base);
            ui_window_invalidate_base(wnd);
            x[0] = 3;
            x[1] = wnd->width - 0x1c;
            x[2] = wnd->width - 0xe;
            ui_window_move(self->sys_button, x[0], 3);
            ui_window_move_base(self->mini_button, x[1], 3);
            ui_window_move_base(self->close_button, x[2], 3);
        }
        self->full_height = wnd->height;
        item_wnd_place_children(self);
        return 0;
    }
    case UI_MSG_SET_TAB:
        self->tab = val1;
        ui_window_send_msg(wnd, NULL, ITEM_WND_MSG_REFRESH, 0, 0, 0, 0);
        return 0;
    case ITEM_WND_MSG_REFRESH:
        if (self->tab == ITEM_TAB_FAVOURITE && g_session1.favouritesSorted == 1)
            ui_item_wnd_sort_favourites(self);
        else
            item_wnd_refresh(self);
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct ItemWndInfo *)val1;
        if (!item_wnd_info_is_valid(self->info))
            item_wnd_info_set_default(self->info);
        ui_window_send_msg(wnd, NULL, UI_MSG_RESIZE, UI_EDGE_BOTTOM_RIGHT, self->info->rect.width, self->info->org_height, 0);
        self->tab = self->info->cur_tab;
        self->usable_filter = USABLE_FILTER_ALL;
        self->equip_filter = EQUIP_FILTER_ALL;
        self->etc_filter = ETC_FILTER_ALL;
        self->tabs->selected = self->info->cur_tab;
        ui_window_invalidate_base(wnd);
        ui_window_send_msg(wnd, NULL, ITEM_WND_MSG_REFRESH, 0, 0, 0, 0);
        if (self->info->rect.height != self->info->org_height)
            ui_window_send_msg(wnd, NULL, UI_MSG_COMMAND, UI_ID_HELP, 0, 0, 0);
        ui_frame_wnd_move(wnd, self->info->rect.x, self->info->rect.y);
        return 0;
    case ITEM_WND_MSG_ADD_DRAGGED:
        item_wnd_take_drag();
        return 0;
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}

void ui_item_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemWnd *self = (struct UIItemWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(wnd, x, y, drag);
        return;
    }
    switch (drag->source) {
    case 1:
        if (drag->drop_window == &self->tabs->base) {
#pragma pack(push, 1)
            struct {
                WORD id;
                WORD index;
                BYTE flag;
            } packet;
#pragma pack(pop)

            packet.id = CZ_INVENTORY_TAB;
            packet.index = (WORD)drag->index;
            packet.flag = ui_tab_control_tab_at(self->tabs, x, y) != ITEM_TAB_FAVOURITE;
            send_packet(&packet, rag_connection_get_packet_length(rag_connection_instance(), (short)packet.id));
        }
        break;
    case 6:
    case 9:
    case 0xf:
        if (drag->drop_count != 1) {
            ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
            ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, 6, 0, 0, 0);
            return;
        }
    case 2:
    case 0x11:
    case 0x12:
    case 0x18:
        ui_window_send_msg(wnd, NULL, ITEM_WND_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    case 0xe:
        if (g_windowMgr.make_target_process_wnd) {
            ui_window_send_msg(g_windowMgr.make_target_process_wnd, NULL, 0x4b, (int)string_cstr(&drag->name),
                     0, 0, 0);
        }
        break;
    case 0x1c: {
        struct UIWindow *target = ui_window_find(WND_ID_REFINING);

        if (target)
            ui_refining_wnd_reset((struct UIRefiningWnd *)target);
        break;
    }
    case 0x21: {
        struct UIWindow *target = ui_window_find(WND_ID_GRADE_ENCHANT);

        if (target)
            ui_grade_enchant_wnd_reset((struct UIGradeEnchantWnd *)target);
        break;
    }
    }
    game_send_message(MODE_REQ_DRAG_END, 0, 0, 0, 0);
}

void ui_item_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemWnd *self = (struct UIItemWnd *)window;
    struct ItemInfo item, current;
    struct UIWindow *edit;
    struct GameMode *game;

    if (y < GRID_TOP)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->base.frame), x, y);
    if (g_replayPlaying)
        return;
    ui_item_wnd_item_at(self, &item, x, y);
    if (item.index > 0) {
        game = game_current_mode();
        if (!item.move_pending) {
            session_get_inventory_item(&g_session1, &current, item.index);
            item_info_assign(&item, &current);
            item_info_destruct(&current);
            game_mode_drag_item(game, 1, &item, item.count, -1);
            if ((BYTE)(GetAsyncKeyState(VK_SHIFT) >> 8) && ui_window_mgr_get_focus_edit(&g_windowMgr) &&
                item.identified) {
                edit = ui_window_mgr_get_focus_edit(&g_windowMgr);
                if (edit->id == ITEM_LINK_EDIT_MEGAPHONE) {
                    megaphone_wnd_add_item_link(g_windowMgr.npc_text_edit_dialog_wnd, &item);
                } else if (ui_window_mgr_get_focus_edit(&g_windowMgr)->id == ITEM_LINK_EDIT_WHISPER) {
                    ui_chat_edit_ctrl_add_item_link(ui_window_mgr_get_focus_edit(&g_windowMgr), &item);
                } else if (ui_window_mgr_get_focus_edit(&g_windowMgr)->id == ITEM_LINK_EDIT_CHAT) {
                    ui_chat_edit_ctrl_add_item_link(ui_chat_window_edit(g_windowMgr.new_chat_wnd), &item);
                } else if (ui_window_mgr_get_focus_edit(&g_windowMgr)->id == ITEM_LINK_EDIT_RODEX) {
                    ui_rodex_compose_wnd_add_item_link(g_windowMgr.rodex_compose_wnd, &item);
                } else if (ui_window_mgr_get_focus_edit(&g_windowMgr)->id == ITEM_LINK_EDIT_CHAT_ROOM) {
                    ui_chat_edit_ctrl_add_item_link(ui_window_mgr_get_focus_edit(&g_windowMgr), &item);
                }
            }
        }
    }
    item_info_destruct(&item);
}

void ui_item_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIItemWnd *self = (struct UIItemWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct ItemInfo item;
    struct Mode *game;

    if (y < GRID_TOP) {
        ui_window_send_msg(wnd, wnd, UI_MSG_COMMAND, UI_ID_HELP, 0, 0, 0);
        return;
    }
    if (GetAsyncKeyState(VK_SHIFT))
        return;
    ui_item_wnd_item_at(self, &item, x, y);
    if (item.index > 0) {
        game = game_current_base_mode();
        switch (item.type) {
        case 0:
        case 1:
        case 2:
            if (item_info_get_item_id(&item) == ITEM_ID_CONFIRM_USE &&
                ui_window_mgr_message_box(&g_windowMgr, msg_str(MSG_CONFIRM_OPEN_BOUND_BOX), 2, 0, 0, 0, "Message",
                                       0x118, 0x78, 0) == UI_MSGBOX_ANSWER_NO)
                break;
            mode_send_message(game, MODE_REQ_USE_ITEM, item.index, 0, 0, 0);
            break;
        case 0x12: {
            struct String text;
            struct String name = {0};
            int answer;

            if (g_session1.manner < 0)
                break;
            item_info_get_display_name(&item, &name);
            string_init(&text);
            string_format(&text, msg_str(MSG_CONFIRM_OPEN_CASH_ITEM), string_cstr(&name));
            answer = ui_window_mgr_message_box(&g_windowMgr, string_cstr(&text), 2, 0, 0, 0, "Message",
                                            0x118, 0x78, 0);
            string_free(&text);
            string_free(&name);
            if (answer == UI_MSGBOX_ANSWER_YES)
                mode_send_message(game, MODE_REQ_USE_ITEM, item.index, 0, 0, 0);
            break;
        }
        case 4:
        case 5:
        case 8:
        case 9:
        case 0xb:
        case 0xc:
        case 0xd:
        case 0xe:
        case 0xf:
            if (!item.identified)
                break;
            if (!(item.index > 0 && item.equip_location && !item.worn_location) &&
                !session_is_pet_accessory(&g_session1, item_info_get_item_id(&item)))
                break;
            if (GetAsyncKeyState(VK_CONTROL) && g_session1.equipSwapOpen) {
                mode_send_message(game, MODE_REQ_SWAP_EQUIP, item.index, item.equip_location, 0, 0);
            } else if (ui_window_find(WND_ID_REFINING)) {
                ui_window_send(ui_window_find(WND_ID_REFINING), wnd, SWAP_MSG_SET_ITEM, item.index, 0, 0, 0);
            } else if (ui_window_find(WND_ID_GRADE_ENCHANT)) {
                ui_window_send(ui_window_find(WND_ID_GRADE_ENCHANT), wnd, SWAP_MSG_SET_ITEM, item.index, 0, 0, 0);
            } else {
                mode_send_message(game, MODE_REQ_EQUIP, item.index, item.equip_location, 0, 0);
            }
            break;
        case 6:
            mode_send_message(game, MODE_REQ_ITEM_COMPOSITION_LIST, item.index, 0, 0, 0);
            break;
        case 0xa:
        case 0x10:
        case 0x11:
        case 0x13:
            if (item.index > 0 && !item.worn_location) {
                if (GetAsyncKeyState(VK_CONTROL) && g_session1.equipSwapOpen)
                    mode_send_message(game, MODE_REQ_SWAP_EQUIP, item.index, item.equip_location, 0, 0);
                else
                    mode_send_message(game, MODE_REQ_WEAR_EQUIP_DIRECT, item.index, item.equip_location, 0, 0);
            }
            break;
        }
    }
    item_info_destruct(&item);
}

void ui_item_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIItemWnd *self = (struct UIItemWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct ItemInfo item;
    struct String text;
    struct String name = {0};
    struct Mode *mode;
    int tip_x, tip_y;

    ui_item_wnd_item_at(self, &item, x, y);
    if (item.index > 0) {
        tip_x = (x - GRID_LEFT) / GRID_CELL * GRID_CELL + GRID_LEFT;
        tip_y = (y - GRID_TOP) / GRID_CELL * GRID_CELL + 1;
        ui_window_to_screen(wnd, &tip_x, &tip_y);
        mode = client_current_mode();
        if (item.identified == 1 && item.option_count) {
            item_info_get_display_name(&item, &name);
            string_init(&text);
            string_format(&text, msg_str(MSG_ITEM_TOOLTIP_OPTIONS), string_cstr(&name), item.option_count,
                          item.count);
        } else {
            item_info_get_display_name(&item, &name);
            string_init(&text);
            string_format(&text, msg_str(MSG_ITEM_TOOLTIP), string_cstr(&name), item.count);
        }
        mode_show_tooltip(mode, string_cstr(&text), tip_x, tip_y,
                          session_get_item_dis_color(&item), 0);
        string_free(&text);
        string_free(&name);
    }
    item_info_destruct(&item);
}

void ui_item_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIItemWnd *self = (struct UIItemWnd *)window;
    struct ItemInfo item;
    struct GameMode *game;

    ui_item_wnd_item_at(self, &item, x, y);
    if (item.index > 0) {
        game = game_current_mode();
        if ((BYTE)(GetAsyncKeyState(VK_MENU) >> 8)) {
            if (!g_windowMgr.say_dialog_wnd && !g_windowMgr.choose_wnd) {
                struct ItemInfo current;

                session_get_inventory_item(&g_session1, &current, item.index);
                item_info_assign(&item, &current);
                item_info_destruct(&current);
                game_mode_drag_item(game, 1, &item, item.count, -1);
                if (!game_mode_refuse_held_item_drag(game)) {
                    if (g_windowMgr.item_store_wnd) {
                        if (game_take_item_move())
                            mode_send_message((struct Mode *)game, MODE_REQ_TO_STORAGE, item.index, item.count, 0, 0);
                    } else if (g_windowMgr.exchange_wnd) {
                        if (game_take_item_move())
                            mode_send_message((struct Mode *)game, MODE_REQ_ADD_EXCHANGE_ITEM, item.index, item.count, 0, 0);
                    } else if (g_windowMgr.merchant_item_wnd && game_take_item_move()) {
                        mode_send_message((struct Mode *)game, MODE_REQ_ITEM_TO_CART, item.index, item.count, 0, 0);
                    }
                }
            }
        } else if ((BYTE)(GetAsyncKeyState(VK_CONTROL) >> 8)) {
            if (item.index > 0 && !session_is_pet_egg_id(&g_session1, atoi(string_cstr(&item.id_text)))) {
                struct ItemInfo magnifier, magnifier2;
                int count;

                session_get_inventory_item_by_id(&g_session1, &magnifier, ITEM_ID_MAGNIFIER, 0);
                session_get_inventory_item_by_id(&g_session1, &magnifier2, ITEM_ID_MAGNIFIER_2, 0);
                count = magnifier2.count + magnifier.count;
                item_info_destruct(&magnifier2);
                item_info_destruct(&magnifier);
                if (count <= 0)
                    ui_chat_line(msg_str(MSG_NO_MAGNIFIER), 0xff);
                if (self->tab == ITEM_TAB_EQUIP && !item.identified) {
                    struct {
                        WORD id;
                        WORD index;
                    } packet;

                    packet.id = CZ_ONECLICK_IDENTIFY;
                    packet.index = (WORD)item.index;
                    send_packet(&packet, rag_connection_get_packet_length(rag_connection_instance(),
                                                                        CZ_ONECLICK_IDENTIFY));
                }
            }
        } else {
            ui_window_send(ui_window_open(ITEM_WND_ID), NULL, UI_MSG_SHOW_ITEM, (int)&item, 0, 0, 0);
        }
    }
    item_info_destruct(&item);
}

#define WND_ID_MENU_ICON 0x133

void ui_item_wnd_clear_new_marks(struct UIItemWnd *self, BYTE keep)
{
    struct ListNode *node;
    struct UIWindow *wnd;

    if (keep)
        return;
    for (node = self->base.items.head->next; node != self->base.items.head; node = node->next) {
        session_clear_inventory_new_flags(&g_session1);
        LIST_VALUE(node, struct ItemInfo)->is_new = 0;
    }
    wnd = ui_window_find(WND_ID_MENU_ICON);
    if (wnd)
        ui_window_send_msg(wnd, NULL, UI_MSG_COMMAND, 0x116, 0xba, 0, 0);
}
