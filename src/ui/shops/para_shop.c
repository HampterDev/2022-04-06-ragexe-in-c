#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "iteminfo/item_api.h"
#include "session/character_name.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/object_type.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "game/context.h"
#include <string.h>
#include "ui/shops/shops.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/items/items.h"
#include "ui/shops/npc_shop_internal.h"
#include "ui/wndmgr/wndmgr.h"

#define PARA_SHOP_WND_ID 0xfe
#define PARA_CART_WND_ID 0xff

#define PARA_MSG_RESET 0x24
#define PARA_SHOP_RESIZE_EDGE 7
#define PARA_CART_RESIZE_EDGE 3
#define PARA_RESULT_RESIZE_EDGE 7
#define PARA_MAX_COUNT 30000
#define PARA_REFUSAL_COLOUR 0xff
#define UIWM_MSG_SHOW_CHAT 3
#define MODE_MSG_PARA_CLOSE 0x124
#define MODE_MSG_PARA_CLOSE2 0x28
#define MODE_MSG_PARA_BUY 0x106
#define MODE_MSG_PARA_BUY_INDEX 0x107
#define COUNT_FOR_PARA_CART 0x15
#define COUNT_FOR_PARA_SHOP 0x16
#define CART_LOAD_WND_ID 0x16

#define MSG_PARA_TITLE_COUNT 0xb9
#define MSG_PARA_TITLE 0xba
#define MSG_PARA_EQUIP_TWICE 0x77
#define MSG_PARA_MAX_COUNT 0x6a8
#define MSG_PARA_STOCK 0x9e3
#define MSG_PARA_ZENY 0x37
#define MSG_PARA_WEIGHT 0x9e8
#define MSG_PARA_RESULT_TITLE 0x9e2
#define MSG_POINTS_CASH 0x4ba
#define MSG_POINTS_HANCOIN 0x50a
#define MSG_POINTS_FREE 0x519
#define DRAG_SHOP_GOODS 3

struct ItemRecordA;

struct UIParaResultWnd {
    struct UIItemBaseWnd base;
    struct UIWindow *title;
    struct UIWindow *total;
    int hover;
    int bottom;
    int extra;
};

struct DragInfo;
struct UIItemShopWnd;
void ui_para_item_shop_wnd_destroy(struct UIWindow *window);
void ui_para_item_shop_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_para_item_shop_wnd_on_draw(struct UIWindow *self);
int ui_para_item_shop_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_para_item_shop_wnd_ops = {
    .destroy = ui_para_item_shop_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_para_item_shop_wnd_drag_and_drop,
    .store_info = ui_item_shop_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_shop_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_para_item_shop_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_shop_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_item_shop_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_shop_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_shop_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_para_item_shop_wnd_send_msg,
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
struct UIItemPurchaseWnd;
void ui_para_item_purchase_wnd_destroy(struct UIWindow *window);
void ui_para_item_purchase_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
int ui_para_item_purchase_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);

const struct UIWindowOps ui_para_item_purchase_wnd_ops = {
    .destroy = ui_para_item_purchase_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_para_item_purchase_wnd_drag_and_drop,
    .store_info = ui_item_purchase_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_item_purchase_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_purchase_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_item_purchase_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_item_purchase_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_item_purchase_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_para_item_purchase_wnd_send_msg,
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
void ui_para_result_wnd_destroy(struct UIWindow *window);
void ui_para_result_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_para_result_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_para_result_wnd_on_draw(struct UIWindow *self);
void ui_para_result_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_para_result_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_para_result_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_para_result_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_para_result_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_para_result_wnd_ops = {
    .destroy = ui_para_result_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_para_result_wnd_drag_and_drop,
    .store_info = ui_item_base_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_para_result_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_para_result_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_para_result_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_para_result_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_para_result_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_para_result_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_para_result_wnd_send_msg,
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

static void refusal(const char *text)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
    ui_chat_line(text, PARA_REFUSAL_COLOUR);
}

static void send_plain(struct UIWindow *wnd, int msg)
{
    ui_window_send(wnd, NULL, msg, 0, 0, 0, 0);
}

static void open_count_window(int count_for)
{
    struct UIWindow *wnd;

    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    wnd = object_as_type(ui_window_find(WND_ID_COUNT_INPUT), OBJECT_ITEM_COUNT_WINDOW);
    ui_window_send(wnd, NULL, COUNT_MSG_OPEN, count_for, 0, 0, 0);
}

static void refresh_para_window(int id, enum ObjectType type)
{
    send_plain(object_as_type(ui_window_find(id), type), SHOP_MSG_REFRESH);
}

static int is_equipment_type(int type)
{
    switch (type) {
    case 4: case 5: case 8: case 9: case 11: case 12: case 13: case 14: case 15:
        return 1;
    }
    return 0;
}

static void clear_items(struct UIItemBaseWnd *base)
{
    list_clear(&base->items, item_info_destroy);
}

static void set_total(struct UIWindow *total, int zeny)
{
    char line[0x50], money[0x40];

    memset(line, 0, sizeof line);
    memset(money, 0, sizeof money);
    sprintf_s(line, sizeof line, "Total : %s Zeny", format_money(zeny, money));
    ui_static_text_set_text_fit(total, line);
}

static void clamp_size(int *width, int *height)
{
    int w = *width > 0x140 ? 0x140 : *width, h = *height > 0x150 ? 0x150 : *height;

    if (w < 0xe8)
        w = 0xe8;
    if (h < 0x78)
        h = 0x78;
    *width = (w - 0x118) / ITEM_GRID_CELL * ITEM_GRID_CELL + 0x118;
    *height = (h - 0x78) / ITEM_GRID_CELL * ITEM_GRID_CELL + 0x78;
}

static void take_info(struct UIWindow *wnd, struct WindowInfo *info, int edge,
                      int(*usable)(struct WindowInfo *),
                      void(*make_default)(struct WindowInfo *))
{
    if (!usable(info))
        make_default(info);
    ui_window_send_msg(wnd, NULL, UI_MSG_RESIZE, edge, info->width, info->height, 0);
    ui_window_move(wnd, info->x, info->y);
}

static int price_text(char *text, const struct ItemInfo *info, const char *unit)
{
    char money[0x40];

    memset(money, 0, sizeof money);
    if (info->price != info->charged_price)
        return sprintf(text, " %d -> %s %s", info->price, format_money(info->charged_price, money), unit);
    return sprintf(text, " %s %s", format_money(info->price, money), unit);
}

static void draw_grade(struct UIWindow *wnd, const struct ItemInfo *info, int x, int y)
{
    struct String path;

    if (info->grade <= 0)
        return;
    string_init(&path);
    string_format(&path, UI_SKIN "grade_enchant\\grade_icon%d.bmp", (int)info->grade);
    ui_window_draw_bitmap(wnd, x, y, res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
    string_free(&path);
}

static void draw_strips(struct UIWindow *wnd, const char *path, int right, int top, int body, int bar)
{
    struct BitmapRes *strip = ui_skin_bitmap(path);
    int i, x;

    if (!strip)
        return;
    x = right ? wnd->width - strip->width : 0;
    for (i = 0; i < body / strip->height; i++)
        ui_window_draw_bitmap(wnd, x, top + strip->height * i, strip, 0);
    if (body % strip->height > 0)
        ui_window_draw_bitmap(wnd, x, wnd->height - strip->height - bar, strip, 0);
}

struct UIItemShopWnd *ui_para_item_shop_wnd_construct(struct UIItemShopWnd *self)
{
    ui_item_shop_wnd_construct(self);
    self->shop.base.frame.base.ops = &ui_para_item_shop_wnd_ops;
    return self;
}

struct UIWindow *ui_para_item_shop_wnd_new(void)
{
    return (struct UIWindow *)ui_para_item_shop_wnd_construct(mem_alloc(sizeof(struct UIItemShopWnd)));
}

void ui_para_item_shop_wnd_destroy(struct UIWindow *window)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    self->shop.base.frame.base.ops = &ui_para_item_shop_wnd_ops;
    ui_item_shop_wnd_destruct(self);
    mem_free(self);
}

void ui_para_item_shop_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->shop.base.frame.base, x, y, drag);
        return;
    }
    if (drag->source != DRAG_CASH_CART) {
        game_send_message(MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (drag->drop_count == 1)
        send_plain(&self->shop.base.frame.base, UI_MSG_ADD_DRAGGED);
    else
        open_count_window(COUNT_FOR_PARA_SHOP);
}

void ui_para_item_shop_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    struct UIItemBaseWnd *base = &self->shop.base;
    struct UIWindow *wnd = &base->frame.base;
    struct ListNode *node;
    const char *unit;
    int top, bar, body, rows, i, y, drawn;

    top = ui_window_draw_title_bar(wnd, 0, "", 0);
    if (top == wnd->height)
        return;
    bar = ui_window_draw_button_bar(wnd, base->button_bar == 1 ? 2 : 0, 0, 1);
    body = wnd->height - bar - top;
    ui_window_fill_rect(wnd, 0, top, wnd->width, body, ui_skin_colour(2, 2));
    draw_strips(wnd, UI_SKIN "basic_interface\\itemwin_left.bmp", 0, top, body, bar);
    draw_strips(wnd, UI_SKIN "basic_interface\\itemwin_right.bmp", 1, top, body, bar);
    rows = (body - (self->cash == 1 ? self->shop.extra : 0)) / ITEM_GRID_CELL;
    for (i = 0, y = top; i < rows; i++, y += ITEM_GRID_CELL)
        ui_window_draw_item_win_mid(wnd, 0x1e, y);

    y = top + 4;
    drawn = 1;
    for (node = base->items.head->next, i = 0; node != base->items.head; node = node->next, i++) {
        struct ItemInfo *info = LIST_VALUE(node, struct ItemInfo);
        char price[0x40];
        int len;

        if (i < base->scroll)
            continue;
        if (session_is_pet_egg_id(&g_session1, atoi(string_cstr(&info->id_text))) == 1 && info->damaged == 1)
            continue;
        if (self->shop.hover == i)
            ui_window_fill_rect(wnd, 0x48, y - 2, 0xbf, 0x1c, ui_skin_colour(6, 2));
        if (self->shop.with_count != 1 && info->count == 0)
            ui_window_draw_item_icon_count_grey(wnd, 0, 0x22, y, info, 0, 0);
        else
            ui_window_draw_item_icon_count(wnd, 0x22, y, info, 0, 0);
        ui_window_draw_item_name_short(wnd, 0x46, y + 8, 0x78, info, 0xc);
        draw_grade(wnd, info, 0x22, y + 0xc);
        if (!self->cash)
            unit = "Z";
        else if (g_serviceType == 6)
            unit = "K";
        else if (g_serviceType != 0)
            unit = "P";
        else if (g_session1.channelCopId == 1)
            unit = "H";
        else
            unit = "C";
        len = price_text(price, info, unit);
        ui_window_text_out_right(wnd, wnd->width - 0x12, y + 8, price, len, 0, 0xc, 0, 0);
        if (drawn == base->rows)
            break;
        y += ITEM_GRID_CELL;
        drawn++;
    }

    if (self->cash == 1) {
        struct String line;
        struct String shortened = {0};
        const char *name = ui_window_shorten_text(wnd, &shortened, session_character_name(&g_session1),
                                               100, 0, 12, 0);

        string_init(&line);
        string_format(&line,
                      msg_str(g_serviceType == 0 && g_session1.channelCopId == 1 ? MSG_POINTS_HANCOIN : MSG_POINTS_CASH),
                      name,
                      g_session1.cashPoints);
        ui_window_text_out(wnd, 0xc, wnd->height - 0x15, string_cstr(&line), line.size, 0, 0xc, 0, 0, 0);
        string_free(&line);
        string_format(&line, msg_str(MSG_POINTS_FREE),
                      name,
                      g_session1.freePoints);
        ui_window_text_out(wnd, 0xc, wnd->height - bar - 0x15, string_cstr(&line), line.size, 0, 0xc, 0, 0,
                         0);
        string_free(&line);
        string_free(&shortened);
    }
}

int ui_para_item_shop_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                         int val2, int val3, int val4)
{
    struct UIItemShopWnd *self = (struct UIItemShopWnd *)window;
    struct UIItemShopWnd2 *shop = &self->shop;
    struct UIWindow *wnd = &shop->base.frame.base;
    struct ItemInfo info;
    struct Mode *mode;
    int i, count;

    switch (msg) {
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_CHECKBOX) {
            g_windowMgr.drag_all = val2;
        }
        return 0;
    case UI_MSG_RESIZE:
        if (val1 == PARA_SHOP_RESIZE_EDGE)
            shop_resize(shop, val2, val3, 1);
        return 0;
    case SHOP_MSG_REFRESH:
        clear_items(&shop->base);
        count = session_sellable_count(&g_session1);
        for (i = 0; i < count; i++) {
            session_sellable_at(&g_session1, &info, i);
            if (info.index)
                item_info_copy(list_push_back(&shop->base.items, NULL, sizeof(struct ItemInfo)), &info);
            item_info_destruct(&info);
        }
        ui_window_invalidate_base(wnd);
        ui_item_base_wnd_update_scroll(&shop->base);
        return 0;
    case UI_MSG_SET_INFO:
        shop->info = (struct WindowInfo *)val1;
        take_info(wnd, shop->info, PARA_SHOP_RESIZE_EDGE, shop_info_usable, shop_info_default);
        return 0;
    case PARA_MSG_RESET:
        if (val1 == 1) {
            shop->with_count = 1;
            shop->base.show_counts = 1;
            ui_static_text_set_text_fit(shop->title, msg_str(MSG_PARA_TITLE_COUNT));
            ((struct UICheckBox *)shop->check)->disabled = 0;
        } else {
            shop->with_count = 0;
            shop->base.show_counts = 0;
            ui_static_text_set_text_fit(shop->title, msg_str(MSG_PARA_TITLE));
            ((struct UICheckBox *)shop->check)->disabled = 1;
        }
        return 0;
    case UI_MSG_ADD_DRAGGED:
        if (val1 != 0)
            return 0;
        mode = game_current_base_mode();
        item_info_construct(&info);
        item_info_assign_from_drag(&info, game_current_drag());
        if (info.by_index == 1) {
            session_remove_shop_buy_item_keyed(&g_session1, &info);
            session_sellable_add(&g_session1, &info);
        } else {
            session_purchase_list_remove(&g_session1, &info);
            session_add_shop_goods_by_name(&g_session1, &info);
        }
        refresh_para_window(PARA_CART_WND_ID, OBJECT_PARA_PURCHASE_WINDOW);
        send_plain(wnd, SHOP_MSG_REFRESH);
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        item_info_destruct(&info);
        return 0;
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&shop->base), sender, msg, val1, val2, val3, val4);
}

struct UIItemPurchaseWnd *ui_para_item_purchase_wnd_construct(struct UIItemPurchaseWnd *self)
{
    ui_item_purchase_wnd_construct(self);
    self->base.frame.base.ops = &ui_para_item_purchase_wnd_ops;
    return self;
}

struct UIWindow *ui_para_item_purchase_wnd_new(void)
{
    return (struct UIWindow *)ui_para_item_purchase_wnd_construct(mem_alloc(sizeof(struct UIItemPurchaseWnd)));
}

void ui_para_item_purchase_wnd_destroy(struct UIWindow *window)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    self->base.frame.base.ops = &ui_para_item_purchase_wnd_ops;
    ui_item_purchase_wnd_destruct(self);
    mem_free(self);
}

void ui_para_item_purchase_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    struct Mode *mode;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->base.frame.base, x, y, drag);
        return;
    }
    mode = game_current_base_mode();
    if (drag->source != DRAG_SHOP_GOODS) {
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (is_equipment_type(drag->type))
        send_plain(&self->base.frame.base, UI_MSG_ADD_DRAGGED);
    else
        open_count_window(COUNT_FOR_PARA_CART);
}

static void cart_add_dragged(struct UIItemPurchaseWnd *self, int wanted)
{
    struct UIWindow *wnd = &self->base.frame.base;
    struct Mode *mode = game_current_base_mode();
    const struct DragInfo *drag = game_current_drag();
    struct ListNode *node;
    struct ItemInfo info;
    char text[0xc8];
    int in_cart = 0, stock, by_index, afford;
    WORD weight;

    if (is_equipment_type(drag->type)) {
        int held = drag->by_index == 1
                       ? session_get_shop_buy_count(&g_session1, drag->index)
                       : session_get_shop_buy_count_by_name(&g_session1, string_cstr(&drag->name));

        if (held > 0) {
            mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
            refusal(msg_str(MSG_PARA_EQUIP_TWICE));
            return;
        }
    }
    item_info_construct(&info);
    item_info_assign_from_drag(&info, drag);
    afford = (long long)g_session1.zeny >= (long long)drag->charged_price * drag->drop_count
                                      + session_get_shop_buy_total(&g_session1);
    for (node = self->base.items.head->next; node != self->base.items.head; node = node->next) {
        const struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);

        if (item->id_text.size == info.id_text.size
            && memcmp(string_cstr(&info.id_text), string_cstr(&item->id_text), info.id_text.size) == 0) {
            in_cart = item->count;
            break;
        }
    }
    if (in_cart + info.count > PARA_MAX_COUNT) {
        info.count = PARA_MAX_COUNT - in_cart;
        memset(text, 0, sizeof text);
        sprintf_s(text, sizeof text, msg_str(MSG_PARA_MAX_COUNT), PARA_MAX_COUNT);
        refusal(text);
    }
    by_index = info.by_index == 1;
    stock = by_index ? session_get_shop_goods_count_by_index(&g_session1, &info)
                     : session_get_shop_goods_count_by_id(&g_session1, &info);
    if (stock != -1 && stock < wanted) {
        info.count = stock;
        refusal(msg_str(MSG_PARA_STOCK));
    }
    if (info.count > 0) {
        if (by_index)
            weight = (WORD)(session_get_shop_goods_weight_by_index(&g_session1, &info) * (WORD)info.count)
                     + session_shop_buy_weight_by_index(&g_session1);
        else
            weight = (WORD)(session_get_shop_goods_weight_by_id(&g_session1, &info) * (WORD)info.count)
                     + session_shop_buy_weight_by_id(&g_session1);
        if (!afford) {
            refusal(msg_str(MSG_PARA_ZENY));
        } else if (weight + g_session1.weight > g_session1.maxWeight) {
            refusal(msg_str(MSG_PARA_WEIGHT));
        } else {
            if (by_index) {
                session_remove_counted_shop_goods(&g_session1, &info);
                session_add_shop_buy_item_keyed(&g_session1, &info);
            } else {
                session_remove_shop_goods_by_id(&g_session1, &info);
                session_add_shop_buy_item(&g_session1, &info);
            }
            send_plain(wnd, SHOP_MSG_REFRESH);
            refresh_para_window(PARA_SHOP_WND_ID, OBJECT_PARA_SHOP_WINDOW);
        }
    }
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
    item_info_destruct(&info);
}

int ui_para_item_purchase_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                             int val1, int val2, int val3, int val4)
{
    struct UIItemPurchaseWnd *self = (struct UIItemPurchaseWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct ItemInfo info;
    int i, count, total, width, height;

    switch (msg) {
    case PARA_MSG_RESET:
        ui_static_text_set_text_fit(self->total, "Total : 0 Zeny");
        return 0;
    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        take_info(wnd, self->info, PARA_CART_RESIZE_EDGE, purchase_info_usable, purchase_info_default);
        return 0;
    case UI_MSG_ADD_DRAGGED:
        cart_add_dragged(self, val1);
        return 0;
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_CANCEL) {
            mode_send_message(client_current_mode(), MODE_MSG_PARA_CLOSE, 0, 0, 0, 0);
            mode_send_message(client_current_mode(), MODE_MSG_PARA_CLOSE2, 0, 0, 0, 0);
            session_clear_lists(&g_session1);
        } else if (val1 == UI_ID_OK) {
            session_get_shop_buy_item_at(&g_session1, &info, 0);
            mode_send_message(client_current_mode(), info.by_index == 1 ? MODE_MSG_PARA_BUY_INDEX : MODE_MSG_PARA_BUY, 0, 0,
                          0, 0);
            session_clear_lists(&g_session1);
            item_info_destruct(&info);
        }
        return 0;
    case UI_MSG_RESIZE: {
        struct UIWindow **buttons = (struct UIWindow **)&self->base.close;

        if (val1 != PARA_CART_RESIZE_EDGE)
            return 0;
        width = val2;
        height = val3;
        clamp_size(&width, &height);
        if (width != wnd->width || height != wnd->height) {
            ui_window_resize_base(wnd, width, height);
            self->base.columns = 1;
            self->base.rows = (wnd->height - 0x26) / ITEM_GRID_CELL;
            ui_window_move_base(&self->base.resizer->base, wnd->width - 0xf, wnd->height - 0xf);
            ui_window_move_base(&self->base.scroll_bar->base, wnd->width - 0xf, 0xe);
            ui_scroll_bar_resize(&self->base.scroll_bar->base, self->base.scroll_bar->base.width, wnd->height - 0x26);
            ui_item_base_wnd_update_scroll(&self->base);
        }
        ui_window_move(self->total, 0xc, wnd->height - 0x14);
        ui_window_move(buttons[0], wnd->width - 0x6a, wnd->height - 0x18);
        ui_window_move(buttons[1], wnd->width - 0x3d, wnd->height - 0x18);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;
    }
    case SHOP_MSG_REFRESH:
        clear_items(&self->base);
        count = session_get_shop_buy_list_size(&g_session1);
        total = 0;
        for (i = 0; i < count; i++) {
            session_get_shop_buy_item_at(&g_session1, &info, i);
            if (info.index) {
                total += info.count * info.charged_price;
                item_info_copy(list_push_back(&self->base.items, NULL, sizeof(struct ItemInfo)), &info);
            }
            item_info_destruct(&info);
        }
        set_total(self->total, total);
        ui_window_invalidate_base(wnd);
        ui_item_base_wnd_update_scroll(&self->base);
        return 0;
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}

struct UIParaResultWnd *ui_para_result_wnd_construct(struct UIParaResultWnd *self)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_para_result_wnd_ops;
    self->title = NULL;
    self->total = NULL;
    self->hover = -1;
    self->base.show_counts = 0;
    self->base.button_bar = 1;
    self->bottom = SHOP_BOTTOM;
    self->extra = 0;
    return self;
}

struct UIWindow *ui_para_result_wnd_new(void)
{
    return (struct UIWindow *)ui_para_result_wnd_construct(mem_alloc(sizeof(struct UIParaResultWnd)));
}

void ui_para_result_wnd_destroy(struct UIWindow *window)
{
    struct UIParaResultWnd *self = (struct UIParaResultWnd *)window;
    self->base.frame.base.ops = &ui_para_result_wnd_ops;
    session_clear_lists(&g_session1);
    ui_item_base_wnd_destruct(&self->base);
    mem_free(self);
}

void ui_para_result_wnd_add_bought_record(struct UIParaResultWnd *self, const struct ItemRecordA *record,
                           int count)
{
    struct ItemInfo info;

    item_info_construct(&info);
    item_info_assign_from_record_a(&info, record);
    info.count = count;
    info.refine = 0;
    info.damaged = 0;
    item_info_set_item_id(&info, atoi(string_cstr(&info.id_text)));
    item_info_copy(list_push_back(&self->base.items, NULL, sizeof(struct ItemInfo)), &info);
    item_info_destruct(&info);
}

void ui_para_result_wnd_add_item(struct UIParaResultWnd *self, const struct ItemInfo *item)
{
    item_info_copy(list_push_back(&self->base.items, NULL, sizeof(struct ItemInfo)), item);
}

void ui_para_result_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
}

void ui_para_result_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIParaResultWnd *self = (struct UIParaResultWnd *)window;
    static const char *const ok_button[3] = { "btn_ok", "btn_ok_a", "btn_ok_b" };
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIWindow *total = mem_alloc(sizeof(struct UIStaticText));

    if (total)
        total = ui_static_text_construct(total);
    self->total = total;
    ui_window_create(total, 0xa0, 0xf);
    ui_window_move(self->total, 0xc, wnd->height - 0x15);
    ui_window_add_child(wnd, self->total);
    self->title = shop_add_title(wnd, 0x46, MSG_PARA_RESULT_TITLE);
    shop_add_resizer(&self->base, PARA_RESULT_RESIZE_EDGE);
    shop_add_scroll_bar(&self->base, wnd->height - self->extra - self->bottom - ITEM_GRID_Y);
    self->base.close = shop_add_button(wnd, ok_button, wnd->width - 0x3d, wnd->height - 0x18, UI_ID_OK);
    self->base.columns = 1;
    self->base.rows = (height - self->extra - self->bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
    send_plain(wnd, SHOP_MSG_REFRESH);
}

void ui_para_result_wnd_on_draw(struct UIWindow *window)
{
    struct UIParaResultWnd *self = (struct UIParaResultWnd *)window;
    struct UIItemBaseWnd *base = &self->base;
    struct UIWindow *wnd = &base->frame.base;
    struct ListNode *node;
    int body, rows, i, cell = 0;

    ui_window_draw_title_bar(wnd, 0, "", 0);
    if (wnd->height == ITEM_GRID_Y)
        return;
    body = wnd->height - self->bottom - ITEM_GRID_Y;
    shop_draw_frame(base, body, base->button_bar == 1 ? 2 : 0);
    rows = (wnd->height - self->extra - self->bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
    shop_draw_cells(wnd, rows);
    shop_fill_names(wnd, rows);
    for (node = base->items.head->next, i = 0; node != base->items.head; node = node->next, i++) {
        struct ItemInfo *info = LIST_VALUE(node, struct ItemInfo);
        char price[0x40];
        int row_y, col_x, len;

        if (i < base->scroll * base->columns || item_grid_hidden(info))
            continue;
        row_y = cell / base->columns * ITEM_GRID_CELL;
        col_x = cell % base->columns * ITEM_GRID_CELL;
        if (self->hover == i)
            ui_window_fill_rect(wnd, col_x + 0x48, row_y + 0x13, 0xbf, 0x1c, ui_skin_colour(6, 2));
        ui_window_draw_item_icon_count(wnd, col_x + 0x2c, row_y + 0x15, info, 0, 0);
        ui_window_draw_item_name_short(wnd, col_x + 0x4c, row_y + 0x1d, 0x6e, info, 0xc);
        draw_grade(wnd, info, col_x + 0x2c, row_y + 0x21);
        memset(price, 0, sizeof price);
        len = price_text(price, info, "Z");
        ui_window_text_out_right(wnd, wnd->width - 0x12, row_y + 0x1d, price, len, 0, 0xc, 0, 0);
        if (++cell == base->rows * base->columns)
            break;
    }
}

void ui_para_result_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIParaResultWnd *self = (struct UIParaResultWnd *)window;
    int old = self->hover;

    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->base.frame), x, y);
    self->hover = ui_item_base_wnd_index_at(&self->base, x, y);
    if (old != self->hover)
        ui_window_invalidate_base(&self->base.frame.base);
}

void ui_para_result_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y)
{
}

void ui_para_result_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIParaResultWnd *self = (struct UIParaResultWnd *)window;
    struct ItemInfo info;
    struct String text;
    struct String name = {0};
    char money[0x40];
    int tip_x, tip_y;

    ui_item_base_wnd_item_at(&self->base, &info, x, y);
    if (info.index) {
        shop_tooltip_cell(&self->base.frame.base, x, y, &tip_x, &tip_y);
        memset(money, 0, sizeof money);
        item_info_get_display_name(&info, &name);
        string_init(&text);
        if (info.price != info.charged_price)
            string_format(&text, "%s %d -> %s Zeny", string_cstr(&name), info.price,
                          format_money(info.charged_price, money));
        else
            string_format(&text, "%s %s Zeny", string_cstr(&name), format_money(info.charged_price, money));
        string_free(&name);
        mode_show_tooltip(client_current_mode(), string_cstr(&text), tip_x, tip_y,
                          session_get_item_dis_color(&info), 0);
        string_free(&text);
    }
    item_info_destruct(&info);
}

void ui_para_result_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIParaResultWnd *self = (struct UIParaResultWnd *)window;
    struct ItemInfo info;
    int old = self->hover;

    self->hover = ui_item_base_wnd_index_at(&self->base, x, y);
    if (old != self->hover)
        ui_window_invalidate_base(&self->base.frame.base);
    ui_item_base_wnd_get_item(&self->base, &info, self->hover);
    if (info.index) {
        game_current_base_mode();
        ui_window_send(ui_window_open(ITEM_WND_ID), NULL, UI_MSG_SHOW_ITEM, (int)&info, 0, 0, 0);
    }
    item_info_destruct(&info);
}

int ui_para_result_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1,
                                       int val2, int val3, int val4)
{
    struct UIParaResultWnd *self = (struct UIParaResultWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct ListNode *node;
    int width, height, total;

    switch (msg) {
    case UI_MSG_SET_INFO:
        ui_static_text_set_text_fit(self->total, "Total : 0 Zeny");
        return 0;
    case UI_MSG_COMMAND:

        if (val1 == UI_ID_OK) {
            mode_send_message(client_current_mode(), MODE_MSG_PARA_CLOSE, 0, 0, 0, 0);
            mode_send_message(client_current_mode(), MODE_MSG_PARA_CLOSE2, 0, 0, 0, 0);
            session_clear_lists(&g_session1);
        }
        return 0;
    case PARA_MSG_RESET:
        self->base.show_counts = 0;
        ui_static_text_set_text_fit(self->title, msg_str(MSG_PARA_RESULT_TITLE));
        ui_para_result_wnd_send_msg(wnd, NULL, UI_MSG_RESIZE, PARA_RESULT_RESIZE_EDGE, wnd->width, val1, 0);
        return 0;
    case UI_MSG_RESIZE:
        if (val1 != PARA_RESULT_RESIZE_EDGE)
            return 0;
        width = val2;
        height = val3;
        clamp_size(&width, &height);
        if (width != wnd->width || height != wnd->height) {
            ui_window_resize_base(wnd, width, height);
            self->base.columns = 1;
            self->base.rows = (height - self->extra - self->bottom - ITEM_GRID_Y) / ITEM_GRID_CELL;
            ui_window_move_base(&self->base.resizer->base, wnd->width - 0xf, wnd->height - 0xf);
            ui_window_move_base(&self->base.scroll_bar->base, wnd->width - 0xf, 0xe);
            ui_scroll_bar_resize(&self->base.scroll_bar->base, self->base.scroll_bar->base.width,
                     wnd->height - self->extra - self->bottom - ITEM_GRID_Y);
            ui_item_base_wnd_update_scroll(&self->base);
        }
        ui_window_move(self->total, 0xc, wnd->height - 0x14);
        ui_window_move_base(&self->base.close->button.base, wnd->width - 0x3d, wnd->height - 0x18);
        ui_frame_wnd_move(wnd, g_renderer->width / 2 - wnd->width / 2,
                 g_renderer->height / 2 - wnd->height / 2);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        return 0;
    case SHOP_MSG_REFRESH:
        total = 0;
        for (node = self->base.items.head->next; node != self->base.items.head; node = node->next) {
            const struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);

            total += item->charged_price * item->count;
        }
        set_total(self->total, total);
        ui_window_invalidate_base(wnd);
        ui_item_base_wnd_update_scroll(&self->base);
        return 0;
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}
