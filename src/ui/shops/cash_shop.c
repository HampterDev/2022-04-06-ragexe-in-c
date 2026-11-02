#include "ui/controls/edit.h"
#include "ui/controls/buttons.h"
#include "render/rp_pool.h"
#include "platform/client_config.h"
#include "platform/settings.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include <string.h>
#include "ui/shops/cash_shop.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/controls/controls.h"
#include "ui/shops/shops_std.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/quests/quests.h"
#include "ui/wndmgr/wndmgr.h"

#define CASH_PAGE_ROWS 9
#define CASH_PAGE_COLUMNS 3

#define MSG_CASH_TITLE 0xcd8
#define MSG_CASH_HEADING 0xcd9
#define MSG_CASH_SEARCH_HINT 0xce6
#define MSG_CASH_SUM_1 0xce7
#define MSG_CASH_SUM_2 0xce8
#define MSG_CASH_SUM_3 0xce9

struct Vec;
struct UICashShopWnd;
struct DragInfo;
struct UIWindow;
void ui_cash_shop_wnd_destroy(struct UIWindow *window);
void ui_cash_shop_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_cash_shop_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_cash_shop_wnd_on_process(struct UIWindow *self);
void ui_cash_shop_wnd_on_draw(struct UIWindow *self);
intptr_t ui_cash_shop_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_cash_shop_wnd_ops = {
    .destroy = ui_cash_shop_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_cash_shop_wnd_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_cash_shop_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_cash_shop_wnd_on_process,
    .on_draw = ui_cash_shop_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_cash_shop_wnd_send_msg,
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

struct UICashShopWnd {
    struct UIFrameWnd frame;
    struct List goods;
    struct Tree tabs;
    int tab;
    int page_rows;
    int page_columns;
    int page;
    int page_count;
    struct UIWindow *search;
    struct UIWindow *refresh;
    int cash;
    int free_cash;
    int cart_total;
    struct UIWindow *count;
    struct Vec tiles;
    struct UIWindow *cart;
};

struct CashTab {
    int id;
    struct UIWindow *button;
};

struct UICashShopWnd *ui_cash_shop_wnd_construct(struct UICashShopWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_cash_shop_wnd_ops;
    list_init(&self->goods);
    tree_init(&self->tabs);
    self->tab = 0;
    self->page_rows = CASH_PAGE_ROWS;
    self->page_columns = CASH_PAGE_COLUMNS;
    self->page = 0;
    self->page_count = 0;
    self->search = NULL;
    self->cash = 0;
    self->free_cash = 0;
    self->cart_total = 0;
    self->count = NULL;
    self->tiles.first = self->tiles.last = self->tiles.end = NULL;
    self->cart = NULL;
    return self;
}

struct UIWindow *ui_cash_shop_wnd_new(void)
{
    return (struct UIWindow *)ui_cash_shop_wnd_construct(mem_alloc(sizeof(struct UICashShopWnd)));
}

void ui_cash_shop_wnd_destruct(struct UICashShopWnd *self)
{
    self->frame.base.ops = &ui_cash_shop_wnd_ops;
    vec_free(&self->tiles);
    tree_free(&self->tabs, NULL);
    list_free(&self->goods, item_info_destroy);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_cash_shop_wnd_destroy(struct UIWindow *window)
{
    struct UICashShopWnd *self = (struct UICashShopWnd *)window;
    ui_cash_shop_wnd_destruct(self);
    mem_free(self);
}

void ui_cash_shop_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UICashShopWnd *self = (struct UICashShopWnd *)window;
    struct Mode *mode;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->frame.base, x, y, drag);
        return;
    }
    mode = game_current_base_mode();
    if (!mode)
        return;
    if (drag->source != DRAG_CASH_CART) {
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (drag->drop_count <= 1) {
        ui_cash_shop_wnd_send_msg(&self->frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    if (g_windowMgr.item_drop_cnt_wnd)
        ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, COUNT_FOR_CASH_SHOP, drag->drop_count, 0, 0);
}

void ui_cash_shop_wnd_on_process(struct UIWindow *window)
{
    struct UICashShopWnd *self = (struct UICashShopWnd *)window;
    struct String search, count;
    const char *text, *hint;

    text = ui_edit_ctrl_get_text(self->search);
    string_init(&search);
    string_assign_cstr(&search, text);
    hint = msg_str(MSG_CASH_SEARCH_HINT);
    if (!string_equal_cstr(&search, hint)) {
        if (!search.size) {
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->search, msg_str(MSG_CASH_SEARCH_HINT));
            ui_edit_ctrl_set_text_colour(self->search, 0xb1, 0xb1, 0xb1, -1);
        } else {
            ui_edit_ctrl_set_text_colour(self->search, 0, 0, 0, -1);
        }
    }

    text = ui_edit_ctrl_get_text(self->count);
    string_init(&count);
    string_assign_cstr(&count, text);
    if (!count.size) {
        struct String zero;

        string_init(&zero);
        string_format(&zero, "%d", 0);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->count, string_cstr(&zero));
        string_free(&zero);
    }
    string_free(&count);
    string_free(&search);
}

static void draw_cash_right(struct UICashShopWnd *self, int x, int y, int value)
{
    struct String text;

    string_init(&text);
    string_format(&text, "%d C", value);
    ui_window_text_out_right(&self->frame.base, x, y, string_cstr(&text), 0, 0, 12, 0, 0);
    string_free(&text);
}

void ui_cash_shop_wnd_on_draw(struct UIWindow *window)
{
    struct UICashShopWnd *self = (struct UICashShopWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct String text;
    const char *backdrop = self->tab == CASH_TAB_SALE ? UI_SKIN "cashshop\\img_shop_bg2.bmp"
                                                      : UI_SKIN "cashshop\\img_shop_bg.bmp";

    ui_window_draw_bitmap(wnd, 0, 0, res_mgr_get(g_resMgr(), backdrop), 0);
    ui_window_draw_title_bar(wnd, 0, msg_str(MSG_CASH_TITLE), 0);
    if (!self->page_count) {
        string_init(&text);
        string_assign_cstr(&text, "0 / 0");
    } else {
        string_init(&text);
        string_format(&text, "^b%d^/b / %d", self->page + 1, self->page_count);
    }
    ui_window_text_out_wrapped(wnd, 0xf7, 0x1ee, &text, 0x2b, 0, 6, 0, 12, 0x6b2921, 0);
    string_free(&text);

    ui_window_text_out_centred(wnd, 0x21a, 0x1b, msg_str(MSG_CASH_HEADING), 0xb9, 0, 14, 0, 1);
    ui_window_text_out_centred(wnd, 0x219, 0x1a, msg_str(MSG_CASH_HEADING), 0xb9, 0, 14, 0xffffff, 1);
    string_format(&text, "%d", self->cash);
    ui_window_text_out_right(wnd, 0x273, 0x33, string_cstr(&text), 0, 0, 12, 0xffffff, 1);
    string_free(&text);

    ui_window_text_out(wnd, 0x226, 0x1b2, msg_str(MSG_CASH_SUM_1), 0, 0, 12, 0, 0, 0);
    draw_cash_right(self, 0x2c4, 0x1b2, self->free_cash);
    ui_window_text_out(wnd, 0x226, 0x1c2, msg_str(MSG_CASH_SUM_2), 0, 0, 12, 0, 0, 0);
    ui_window_text_out_right(wnd, 0x2c4, 0x1c2, "C", 0, 0, 12, 0, 0);
    ui_window_text_out(wnd, 0x226, 0x1d2, msg_str(MSG_CASH_SUM_3), 0, 0, 12, 0, 0, 0);
    draw_cash_right(self, 0x2c4, 0x1d2, self->cash);

    string_format(&text, "%d C", self->cart_total);
    ui_window_text_out_centred(wnd, 0x228, 0x1ec, string_cstr(&text), 0x9a, 0, 14, 0xffffff, 1);
    string_free(&text);
}

#define UI_ID_CASH_SHOP 0xb8

void u_in_cash_call_wnd_destroy(struct UIWindow *window);
void u_in_cash_call_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void u_in_cash_call_wnd_on_draw(struct UIWindow *self);
intptr_t u_in_cash_call_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps u_in_cash_call_wnd_ops = {
    .destroy = u_in_cash_call_wnd_destroy,
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
    .on_create = u_in_cash_call_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = u_in_cash_call_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_frame_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_frame_wnd_snap,
    .send_msg = u_in_cash_call_wnd_send_msg,
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

struct UIWindow *u_in_cash_call_wnd_new(void)
{
    struct UIFrameWnd *self = mem_alloc(sizeof *self);

    ui_frame_wnd_construct(self, 0);
    self->base.ops = &u_in_cash_call_wnd_ops;
    return &self->base;
}

void u_in_cash_call_wnd_destruct(struct UIFrameWnd *self)
{
    self->base.ops = &u_in_cash_call_wnd_ops;
    ui_frame_wnd_destruct(self);
}

void u_in_cash_call_wnd_destroy(struct UIWindow *window)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    u_in_cash_call_wnd_destruct(self);
    mem_free(self);
}

void u_in_cash_call_wnd_on_create(struct UIWindow *window, int width_unused,
                                         int height_unused)
{
    struct UIFrameWnd *self = (struct UIFrameWnd *)window;
    static const char bitmap[] = UI_SKIN "basic_interface\\NC_CashShop.bmp";
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, bitmap, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, bitmap, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, bitmap, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, 0, 0);
    button->button.base.id = UI_ID_CASH_SHOP;
    ui_window_add_child(&self->base, &button->button.base);
}

void u_in_cash_call_wnd_on_draw(struct UIWindow *self)
{
}

#define CASH_SALE_PAGE_ROWS 6
struct UICashShopWnd_Item;
struct UICashShopWnd_Cart;

#define CASH_ID_PREV_PAGE 0x134
#define CASH_ID_NEXT_PAGE 0x135
#define CASH_ID_SEARCH 0x139
#define CASH_ID_CLEAR_CART 0x17a
#define CASH_ID_FIRST_PAGE 0x17b
#define CASH_ID_LAST_PAGE 0x17c
#define CASH_ID_REFRESH 0x197
#define CASH_WND_ID 0x13e

#define CASH_MSG_RELIST 0x17
#define CASH_MSG_DROP 0x26
#define CASH_MSG_POINTS 0x78
#define CASH_MSG_POINTS_B 0x7a
#define CASH_MSG_SELECT_TAB 0xa4
#define CASH_MSG_CLEARED 7
#define MODE_REQ_CASH_SHOP 0x140

#define MSG_CASH_BUY 0xce4
#define MSG_CASH_REFRESH 0xcea
#define MSG_CASH_SOLD_OUT 0xe50
#define MSG_CASH_NOT_FOUND 0x42d
#define MSG_CASH_POINTS_OVER 0x744
#define MSG_CASH_CONFIRM 0xebb
#define MSG_CASH_CONFIRM_POINTS 0xebc
#define MSG_CASH_NOT_ENOUGH 0x4e6

static const int cash_tab_names[CASH_TAB_COUNT] = {
    0xcda, 0xcdb, 0xcdc, 0xcdd, 0xcde, 0xcdf, 0xce0, 0xce1, 0xce2, 0xe48, 0xece,
};
#define CASH_TAB_NO_BUTTON(tab) ((tab) == 8 || (tab) == CASH_TAB_SEARCH)

#define PACKET_CASH_BUY 0x848
#define CASH_BUY_MAX_LEN 0x800
#pragma pack(push, 1)
struct CashBuyHeader {
    WORD id;
    WORD len;
    WORD count;
    int points;
};
struct CashBuyLine {
    int item_id;
    int count;
    WORD tab;
};
#pragma pack(pop)
SIZE_ASSERT(CashBuyHeader, 10);
SIZE_ASSERT(CashBuyLine, 10);

static struct UIBitmapButton *make_bitmap_button(struct UIBitmapButton *button, const char *up, const char *hover,
                                                 const char *down)
{
    ui_bitmap_button_set_bitmap(button, up, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, hover, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, down, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    return button;
}

static struct UIBitmapButton *new_bitmap_button(void)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    return button ? ui_bitmap_button_construct(button) : NULL;
}

static struct UIBmpTextButton *new_text_button(void)
{
    struct UIBmpTextButton *button = mem_alloc(sizeof *button);

    return button ? ui_bmp_text_button_construct(button) : NULL;
}

static void add_at(struct UICashShopWnd *self, struct UIWindow *child, int x, int y, int id)
{
    ui_window_move(child, x, y);
    child->id = id;
    ui_window_add_child(&self->frame.base, child);
}

static void add_arrow(struct UICashShopWnd *self, const char *off, const char *on, int x, int id)
{
    struct UIBitmapButton *button = make_bitmap_button(new_bitmap_button(), off, off, on);

    add_at(self, &button->button.base, x, 0x1ed, id);
}

static void set_caption(struct UIBmpTextButton *button, int msg)
{
    const char *text = msg_str(msg);

    ui_bmp_text_button_set_caption(button, text_view_cstr(text));
}

static void set_tab_bitmap(struct UIWindow *radio, const char *format, int tab, int state)
{
    struct String path;

    string_init(&path);
    string_format(&path, format, tab);
    ui_bmp_radio_btn_set_bitmap(radio, string_cstr(&path), state);
    string_free(&path);
}

void ui_cash_shop_wnd_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UICashShopWnd *self = (struct UICashShopWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIBitmapButton *button;
    struct UIBmpTextButton *text_button;
    struct CashTab *current;
    struct String zero;
    int tab, x, i;

    ui_window_add_sys_button(wnd);
    ui_window_add_close_button(wnd, "", 0);

    for (tab = 0, x = 8; tab < CASH_TAB_COUNT; tab++) {
        struct UIWindow *radio = NULL;
        struct CashTab entry;

        if (!CASH_TAB_NO_BUTTON(tab)) {
            const char *caption;

            radio = ui_bmp_radio_btn_create(0);
            set_tab_bitmap(radio, UI_SKIN "cashshop\\img_shop_tap%d_off.bmp", tab, 0);
            set_tab_bitmap(radio, UI_SKIN "cashshop\\img_shop_tap%d_off.bmp", tab, 1);
            set_tab_bitmap(radio, UI_SKIN "cashshop\\img_shop_tap%d_on.bmp", tab, 2);
            set_tab_bitmap(radio, UI_SKIN "cashshop\\img_shop_tap%d_disable.bmp", tab, 3);
            ui_window_move(radio, x, 0x48);
            caption = msg_str(cash_tab_names[tab]);
            ui_bmp_radio_btn_set_tool_tip(radio, caption);
            ui_window_send_msg(radio, wnd, UI_MSG_SET_STATE, tab == self->tab, 0, 0, 0);
            x += 0x3a;
            ui_window_add_child(wnd, radio);
        }
        entry.id = tab;
        entry.button = radio;
        tree_add(&self->tabs, &entry, sizeof entry, tree_less_int, NULL);
    }

    add_arrow(self, UI_SKIN "cashshop\\bt_arrowL_off.bmp", UI_SKIN "cashshop\\bt_arrowL_on.bmp", 0xec,
              CASH_ID_PREV_PAGE);
    add_arrow(self, UI_SKIN "cashshop\\bt_arrowR_off.bmp", UI_SKIN "cashshop\\bt_arrowR_on.bmp", 0x122,
              CASH_ID_NEXT_PAGE);
    add_arrow(self, UI_SKIN "cashshop\\bt_arrowL2_off.bmp", UI_SKIN "cashshop\\bt_arrowL2_on.bmp", 0xd9,
              CASH_ID_FIRST_PAGE);
    add_arrow(self, UI_SKIN "cashshop\\bt_arrowR2_off.bmp", UI_SKIN "cashshop\\bt_arrowR2_on.bmp", 0x130,
              CASH_ID_LAST_PAGE);

    text_button = new_text_button();
    self->refresh = &text_button->bmp.button.base;
    make_bitmap_button(&text_button->bmp, UI_SKIN "cashshop\\btn_refresh_normal.bmp",
                       UI_SKIN "cashshop\\btn_refresh_press.bmp", UI_SKIN "cashshop\\btn_refresh_press.bmp");
    set_caption(text_button, MSG_CASH_REFRESH);
    add_at(self, self->refresh, 0x1c4, 0x204, CASH_ID_REFRESH);

    self->search = mem_alloc(sizeof(struct UIEditCtrl));
    if (self->search)
        self->search = ui_edit_ctrl_construct(self->search);
    ui_window_create(self->search, 0x123, 0xe);
    ui_window_move(self->search, 0x5e, 0x206);
    ((struct UIEditCtrl *)self->search)->keep_ime_on_enter = 1;
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->search, msg_str(MSG_CASH_SEARCH_HINT));
    ui_edit_ctrl_set_text_colour(self->search, 0xb1, 0xb1, 0xb1, -1);
    ui_window_add_child(wnd, self->search);

    button = make_bitmap_button(new_bitmap_button(), UI_SKIN "cashshop\\btn_searchbar_normal.bmp",
                                UI_SKIN "cashshop\\btn_searchbar_over.bmp",
                                UI_SKIN "cashshop\\btn_searchbar_press.bmp");
    add_at(self, &button->button.base, 0x188, 0x204, CASH_ID_SEARCH);

    text_button = new_text_button();
    make_bitmap_button(&text_button->bmp, UI_SKIN "cashshop\\btn_buy_normal.bmp",
                       UI_SKIN "cashshop\\btn_buy_press.bmp", UI_SKIN "cashshop\\btn_buy_press.bmp");
    set_caption(text_button, MSG_CASH_BUY);
    add_at(self, &text_button->bmp.button.base, 0x228, 0x1fc, CASH_ID_ADD);

    self->count = mem_alloc(sizeof(struct UIEditCtrlCustom));
    if (self->count)
        self->count = ui_edit_ctrl_custom_construct(self->count);
    ui_window_create(self->count, 0x3e, 0xe);
    ui_window_move(self->count, 0x27c, 0x1c0);
    ((struct UIEditCtrlCustom *)self->count)->align = UI_EDIT_ALIGN_RIGHT;
    ui_edit_ctrl_set_back_colour(self->count, 0xce, 0xd6, 0xeb);
    ((struct UIEditCtrl *)self->count)->max_chars = 10;
    string_init(&zero);
    string_format(&zero, "%d", 0);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->count, string_cstr(&zero));
    string_free(&zero);
    ui_window_add_child(wnd, self->count);

    for (i = 0; i < self->page_rows; i++) {
        struct UIWindow *tile = (struct UIWindow *)ui_cash_shop_wnd_item_new();

        ui_window_create(tile, 0xac, 0x7e);
        ui_window_move(tile, i % self->page_columns * 0xaf + 8, i / self->page_columns * 0x7f + 0x6b);
        ui_window_set_show(tile, 0);
        ui_window_add_child(wnd, tile);
        vec_push(&self->tiles, &tile, sizeof(struct UIWindow *));
    }

    self->cart = (struct UIWindow *)ui_cash_shop_wnd_cart_new();
    ui_window_create(self->cart, 0xb9, 0x15e);
    ui_window_move_base(self->cart, 0x219, 0x48);
    ui_window_add_child(wnd, self->cart);

    current = tree_get(&self->tabs, &self->tab, sizeof self->tab, sizeof *current, tree_less_int);
    ui_cash_shop_wnd_send_msg(&self->frame.base, current->button, UI_MSG_COMMAND, UI_ID_RADIO, 0, 0, 0);
    ui_cash_shop_wnd_send_msg(&self->frame.base, NULL, CASH_MSG_RELIST, 0, 0, 0, 0);
    game_send_message(MODE_REQ_CASH_SHOP, 0, 0, 0, 0);
}

static void ui_cash_shop_wnd_layout_page(struct UICashShopWnd *self)
{
    struct ListNode *node;
    unsigned tile_count = (unsigned)VEC_COUNT(&self->tiles, struct UIWindow *);
    unsigned index, i;
    int n;

    for (i = 0; i < VEC_COUNT(&self->tiles, struct UIWindow *); i++)
        ui_window_set_show((VEC_AT(&self->tiles, struct UIWindow *, i)), 0);
    node = self->goods.head->next;
    index = 0;
    n = self->page * self->page_rows;
    if (n > 0) {
        index = n;
        for (; n; n--)
            node = node->next;
    }
    for (i = 0; (int)i < self->page_rows && index < self->goods.size && i < tile_count; i++, index++) {
        struct UIWindow *tile = VEC_AT(&self->tiles, struct UIWindow *, i);
        int y;

        if (self->tab == CASH_TAB_SALE) {
            ui_window_resize(tile, 0xac, 0xbd);
            y = (int)i / self->page_columns * 0xc0;
        } else {
            ui_window_resize(tile, 0xac, 0x7e);
            y = (int)i / self->page_columns * 0x7f;
        }
        ui_window_move(tile, (int)i % self->page_columns * 0xaf + 8, y + 0x6b);
        ui_window_send_msg(tile, &self->frame.base, CASH_MSG_SET_GOODS, (intptr_t)LIST_VALUE(node, struct ItemInfo), self->tab, 0, 0);
        node = node->next;
        tile_count = (unsigned)VEC_COUNT(&self->tiles, struct UIWindow *);
    }
    ui_window_invalidate_base(&self->frame.base);
}

static void ui_cash_shop_wnd_cart_clear(struct UIWindow *cart)
{
    struct Vec *lines = cash_shop_cart_lines(cart);

    item_info_vec_clear(lines);
    ui_window_send_msg(cart, cart, CASH_MSG_CLEARED, 0, 0, 0, 0);
    ui_window_invalidate(cart);
}

static struct ItemInfo *find_goods(struct List *list, const struct String *id_text)
{
    struct ListNode *node;

    for (node = list->head->next; node != list->head; node = node->next) {
        struct ItemInfo *goods = LIST_VALUE(node, struct ItemInfo);

        if (atoi(string_cstr(&goods->id_text)) == atoi(string_cstr(id_text)))
            return goods;
    }
    return NULL;
}

static struct ItemInfo *find_cart_line(struct Vec *lines, const struct String *id_text)
{
    struct ItemInfo *line;

    for (line = lines->first; line != lines->last; line++)
        if (atoi(string_cstr(&line->id_text)) == atoi(string_cstr(id_text)))
            break;
    return line;
}

static void drop_on_shop(struct UICashShopWnd *self)
{
    struct Mode *mode = game_current_base_mode();
    struct DragInfo *drag;

    if (!mode)
        return;
    drag = game_current_drag();
    if (!drag)
        return;
    if (drag->source == DRAG_CASH_CART) {
        struct Vec *lines = cash_shop_cart_lines(self->cart);
        struct ItemInfo *line = find_cart_line(lines, &drag->name);

        if (line != lines->last) {
            if (line->count > drag->drop_count)
                ui_cash_shop_wnd_cart_send_msg(self->cart, &self->frame.base, CASH_MSG_ADD, (intptr_t)line, -drag->drop_count, 0, 0);
            else
                ui_cash_shop_wnd_cart_send_msg(self->cart, &self->frame.base, CASH_MSG_REMOVE, item_info_get_item_id(line),
                                               0, 0, 0);
        }
    } else if (drag->source == DRAG_CASH_GOODS) {
        struct ItemInfo *goods;

        if (self->tab == CASH_TAB_SALE) {
            goods = find_goods(&self->goods, &drag->name);
            if (goods) {
                struct Vec *lines;
                struct ItemInfo *line;

                if (CASH_GOODS_STOCK(goods) <= 0) {
                    ui_message_box(msg_str(MSG_CASH_SOLD_OUT));
                    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
                    return;
                }
                lines = cash_shop_cart_lines(self->cart);
                line = find_cart_line(lines, &drag->name);
                if (line != lines->last && CASH_GOODS_STOCK(goods) < drag->drop_count + line->count)
                    drag->drop_count = CASH_GOODS_STOCK(goods) - line->count;
            }
        }
        goods = find_goods(&self->goods, &drag->name);
        if (goods)
            ui_cash_shop_wnd_cart_send_msg(self->cart, &self->frame.base, CASH_MSG_ADD, (intptr_t)goods, drag->drop_count, 0, 0);
    }
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
}

static int text_contains(const char *hay, size_t hay_len, const char *needle, size_t needle_len)
{
    const char *p, *end;

    if (needle_len > hay_len)
        return 0;
    end = hay + (hay_len - needle_len + 1);
    for (p = hay; (p = memchr(p, needle[0], end - p)) != NULL; p++)
        if (!memcmp(p, needle, needle_len))
            return 1;
    return 0;
}

static void search_goods(struct UICashShopWnd *self)
{
    struct String text;
    int tab, i, n;

    string_init(&text);
    string_assign_cstr(&text, ui_edit_ctrl_get_text(self->search));
    if (!text.size || string_equal_cstr(&text, msg_str(MSG_CASH_SEARCH_HINT))) {
        string_free(&text);
        return;
    }
    session_clear_item_list(&g_session1, CASH_TAB_SEARCH);
    for (tab = 0; tab < CASH_TAB_COUNT; tab++) {
        if (tab == CASH_TAB_SEARCH || tab == 8 || tab == CASH_TAB_SALE)
            continue;
        for (i = 0; i < session_cash_goods_count(&g_session1, tab); i++) {
            struct ItemInfo info;
            struct String name;

            session_get_cash_goods(&g_session1, &info, tab, i);
            string_copy(&name, &info.name);
            if (text_contains(string_cstr(&name), name.size, string_cstr(&text), text.size)
                && !session_has_cash_search_result(&g_session1, item_info_get_item_id(&info)))
                item_info_copy(list_push_back(&g_session1.cashShopGoods[10], NULL, sizeof(struct ItemInfo)), &info);
            string_free(&name);
            item_info_destruct(&info);
        }
    }

    if (session_cash_goods_count(&g_session1, CASH_TAB_SEARCH) <= 0) {
        ui_message_box(msg_str(MSG_CASH_NOT_FOUND));
        string_free(&text);
        return;
    }
    list_clear(&self->goods, item_info_destroy);
    n = session_cash_goods_count(&g_session1, CASH_TAB_SEARCH);
    for (i = 0; i < n; i++) {
        struct ItemInfo info;

        session_get_cash_goods(&g_session1, &info, CASH_TAB_SEARCH, i);
        item_info_copy(list_push_back(&self->goods, NULL, sizeof(struct ItemInfo)), &info);
        item_info_destruct(&info);
        n = session_cash_goods_count(&g_session1, CASH_TAB_SEARCH);
    }
    self->tab = CASH_TAB_SEARCH;
    self->page = 0;
    self->page_count = self->goods.size / (unsigned)self->page_rows
                       + (self->goods.size % (unsigned)self->page_rows != 0);
    {
        struct TreeNode *node;

        for (node = tree_first(&self->tabs); node != self->tabs.head; node = tree_next(node))
            if (TREE_VALUE(node, struct CashTab)->button)
                ui_window_send_msg(TREE_VALUE(node, struct CashTab)->button, &self->frame.base, UI_MSG_SET_STATE, 0,
                                   0, 0, 0);
    }
    ui_cash_shop_wnd_layout_page(self);
    string_free(&text);
}

static void buy_cart(struct UICashShopWnd *self)
{
    struct Vec *lines = cash_shop_cart_lines(self->cart);
    struct ItemInfo *line;
    struct CashBuyHeader header;
    char text[0x1030];
    char packet[CASH_BUY_MAX_LEN];
    int count = (int)VEC_COUNT(lines, struct ItemInfo);
    int len, i;

    if (lines->first == lines->last)
        return;
    header.id = PACKET_CASH_BUY;
    header.points = atoi(ui_edit_ctrl_get_text(self->count));
    if (self->cart_total < header.points) {
        ui_message_box(msg_str(MSG_CASH_POINTS_OVER));
        return;
    }
    if (header.points > 0)
        snprintf(text, 0x800, msg_str(MSG_CASH_CONFIRM_POINTS), self->cart_total - header.points,
                    header.points);
    else
        snprintf(text, 0x800, msg_str(MSG_CASH_CONFIRM), self->cart_total);
    if (self->cart_total > self->cash + header.points) {
        ui_message_box(msg_str(MSG_CASH_NOT_ENOUGH));
        return;
    }
    if (ui_window_mgr_message_box(&g_windowMgr, text, UI_MSGBOX_QUESTION, 1, 1, 0, "메시지", 0x118, 0x78,
                               0) != UI_MSGBOX_ANSWER_YES)
        return;
    len = (count + 1) * 10;
    if ((unsigned)len > CASH_BUY_MAX_LEN)
        return;
    header.len = (WORD)len;
    header.count = (WORD)count;
    memcpy(packet, &header, sizeof header);
    for (i = 0, line = lines->first; i < count; i++, line++) {
        struct CashBuyLine entry;

        entry.count = line->count;
        entry.item_id = item_info_get_item_id(line);
        entry.tab = line->bind_on_equip_type;
        memcpy(packet + sizeof header + i * sizeof entry, &entry, sizeof entry);
    }
    rag_connection_send_packet(rag_connection_instance(), (short)len, packet);
}

static void show_tab(struct UICashShopWnd *self, int tab)
{
    struct TreeNode *node;

    for (node = tree_first(&self->tabs); node != self->tabs.head; node = tree_next(node)) {
        struct CashTab *entry = TREE_VALUE(node, struct CashTab);

        if (CASH_TAB_NO_BUTTON(entry->id) || !entry->button)
            continue;
        ui_window_send_msg(entry->button, &self->frame.base, UI_MSG_SET_STATE, tab == entry->id, 0, 0, 0);
    }
}

static void tab_clicked(struct UICashShopWnd *self, struct UIWindow *sender)
{
    struct TreeNode *node;
    int tab = self->tab;
    struct PlatformSettingsKey *key = NULL;

    for (node = tree_first(&self->tabs); node != self->tabs.head; node = tree_next(node))
        if (!CASH_TAB_NO_BUTTON(TREE_VALUE(node, struct CashTab)->id)
            && sender == TREE_VALUE(node, struct CashTab)->button)
            tab = TREE_VALUE(node, struct CashTab)->id;
    if (tab != self->tab) {
        show_tab(self, tab);
        self->tab = tab;
        ui_cash_shop_wnd_send_msg(&self->frame.base, NULL, CASH_MSG_RELIST, 0, 0, 0, 0);
    }
    ui_button_set_show((struct UIButton *)(self->refresh), self->tab == CASH_TAB_SALE);
    if (platform_settings_create(g_registryKeyPath, PLATFORM_SETTINGS_ALL, &key))
        platform_settings_write(key, "NeoCash", PLATFORM_SETTING_DWORD, &self->tab, 4);
    platform_settings_close(key);
}

intptr_t ui_cash_shop_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                     intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UICashShopWnd *self = (struct UICashShopWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    switch (msg) {
    case UI_MSG_DEFAULT_BUTTON:
        if (sender == self->search)
            ui_cash_shop_wnd_send_msg(&self->frame.base, NULL, UI_MSG_COMMAND, CASH_ID_SEARCH, 0, 0, 0);
        return 0;

    case UI_MSG_COMMAND:
        switch (val1) {
        case UI_ID_CLOSE:
            ui_window_mgr_post_quit1(&g_windowMgr, CASH_WND_ID);
            return 0;
        case UI_ID_RADIO:
            tab_clicked(self, sender);
            return 0;
        case CASH_ID_ADD:
            buy_cart(self);
            return 0;
        case CASH_ID_PREV_PAGE:
            self->page = self->page - 1 >= 0 ? self->page - 1 : 0;
            ui_cash_shop_wnd_layout_page(self);
            return 0;
        case CASH_ID_NEXT_PAGE:
            self->page = self->page_count > self->page + 1 ? self->page + 1 : self->page_count - 1;
            ui_cash_shop_wnd_layout_page(self);
            return 0;
        case CASH_ID_SEARCH:
            search_goods(self);
            return 0;
        case CASH_ID_CLEAR_CART:
            self->cart_total = 0;
            ui_cash_shop_wnd_cart_clear(self->cart);
            ui_window_invalidate_base(&self->frame.base);
            return 0;
        case CASH_ID_FIRST_PAGE:
            self->page = 0;
            ui_cash_shop_wnd_layout_page(self);
            return 0;
        case CASH_ID_LAST_PAGE:
            self->page = self->page_count - 1;
            ui_cash_shop_wnd_layout_page(self);
            return 0;
        case CASH_ID_REFRESH:
            game_send_message(MODE_REQ_CASH_SHOP, 0, 0, 0, 0);
            return 0;
        }
        return 0;

    case CASH_MSG_RELIST: {
        int i;

        list_clear(&self->goods, item_info_destroy);
        for (i = 0; i < session_cash_goods_count(&g_session1, self->tab); i++) {
            struct ItemInfo info;

            session_get_cash_goods(&g_session1, &info, self->tab, i);
            if (info.index)
                item_info_copy(list_push_back(&self->goods, NULL, sizeof(struct ItemInfo)), &info);
            item_info_destruct(&info);
        }
        self->page_rows = self->tab == CASH_TAB_SALE ? CASH_SALE_PAGE_ROWS : CASH_PAGE_ROWS;
        self->page = 0;
        self->page_count = self->goods.size / (unsigned)self->page_rows
                           + (self->goods.size % (unsigned)self->page_rows != 0);
        ui_cash_shop_wnd_layout_page(self);
        return 0;
    }

    case CASH_MSG_ADD:
        ui_cash_shop_wnd_cart_send_msg(self->cart, sender, CASH_MSG_ADD, val1, 1, 0, 0);
        return 0;

    case CASH_MSG_DROP:
        drop_on_shop(self);
        return 0;

    case CASH_MSG_POINTS:
    case CASH_MSG_POINTS_B: {
        struct PlatformSettingsKey *key = NULL;
        uint32_t type, size;
        struct String zero;

        if (platform_settings_open(g_registryKeyPath, PLATFORM_SETTINGS_QUERY, &key)) {
            type = PLATFORM_SETTING_DWORD;
            size = 4;
            platform_settings_read(key, "CASH_CATEGORY", &type, &self->tab, &size);
        }
        platform_settings_close(key);
        self->cash = val1;
        self->free_cash = val2;
        string_init(&zero);
        string_format(&zero, "%d", 0);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->count, string_cstr(&zero));
        string_free(&zero);
        ui_cash_shop_wnd_send_msg(&self->frame.base, wnd, UI_MSG_COMMAND, CASH_ID_CLEAR_CART, 0, 0, 0);
        return 0;
    }

    case CASH_MSG_CART_TOTAL:
        self->cart_total = val1;
        ui_window_invalidate_base(&self->frame.base);
        return 0;

    case CASH_MSG_SELECT_TAB:
        if (val1 && val1 != self->tab) {
            show_tab(self, val1);
            self->tab = val1;
            ui_cash_shop_wnd_send_msg(&self->frame.base, NULL, CASH_MSG_RELIST, 0, 0, 0, 0);
        }
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
}

#define UI_MSG_BUTTON 6
intptr_t u_in_cash_call_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg,
                                       intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    if (msg != UI_MSG_BUTTON)
        return ui_window_send_msg_base(self, sender, msg, val1, val2, val3, val4);
    if (val1 == 0xb8) {
        game_send_message(0x143, 0, 0, 0, 0);
    }
    return 0;
}
