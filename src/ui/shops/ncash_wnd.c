#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "platform/client_config.h"
#include "platform/settings.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include "contents/connection_api.h"
#include <string.h>
#include "ui/shops/ncash_wnd.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/items/items.h"
#include "ui/wndmgr/wndmgr.h"
#include "ui/draw_key.h"

#define NCASH_MSG_CART_WHEEL 7
#define NCASH_MSG_CART_UP 9
#define NCASH_MSG_CART_DOWN 0xa
#define NCASH_MSG_DROP 0x26
#define NCASH_MSG_LIST_GOODS 0x3c
#define NCASH_MSG_OPEN 0x78
#define NCASH_MSG_PAGES 0x79
#define NCASH_MSG_RESET 0x7a

#define NCASH_ID_CLOSE UI_ID_CLOSE
#define NCASH_ID_BUY 0xd8
#define NCASH_ID_PREV_PAGE 0x134
#define NCASH_ID_NEXT_PAGE 0x135
#define NCASH_ID_FIND 0x165
#define NCASH_ID_CANCEL 0x17a
#define NCASH_ID_FIRST_PAGE 0x17b
#define NCASH_ID_LAST_PAGE 0x17c
#define NCASH_ID_TAB0 0x17d
#define NCASH_ID_REFRESH 0x197
#define NCASH_ID_1D5 0x1d5

#define MSG_NCASH_TITLE 0x817
#define MSG_NCASH_TAB_TIP 0x736
#define MSG_NCASH_POINTS 0x741
#define MSG_NCASH_CASH 0x742
#define MSG_NCASH_TOTAL 0x51a
#define MSG_NCASH_TOO_MUCH 0x4e6
#define MSG_NCASH_POINTS_OVER 0x744
#define MSG_NCASH_CONFIRM 0xebb
#define MSG_NCASH_CONFIRM_POINTS 0xebc
#define MSG_NCASH_NOT_FOUND 0x42d
#define MSG_NCASH_SOLD_OUT 0x9db
#define MSG_NCASH_LIMIT 0x9dc
#define MSG_NCASH_GOODS_PRICE 0x73e
#define MSG_NCASH_CART_COUNT 0x73f
#define MSG_NCASH_CART_UNIT 0x740
#define NCASH_MEASURE_Y (-0x1e)

#define PACKET_NCASH_BUY 0x848
#define PACKET_NCASH_CLOSE 0x84a
#define PACKET_NCASH_REFRESH 0x9c3
#define NCASH_BUY_MAX_LEN 0x800

#define GAMEMODE_DRAG_END 0x12

#define NCASH_DIR UI_SKIN "basic_interface\\"
#define REG_SUBKEY "\\NeoCash"
#define REG_TAB_VALUE "CASH_CATEGORY"

#define BUTTON_DISABLED(wnd) (((struct UIButton *)(wnd))->disabled)

static void copy_cart(struct UInCashWnd *self)
{
    int i, n;

    list_clear(&self->cart, item_info_destroy);
    n = session_get_shop_buy_list_size(&g_session1);
    for (i = 0; i < n; i++) {
        struct ItemInfo info;

        session_get_shop_buy_item_at(&g_session1, &info, i);
        if (info.index)
            item_info_copy(list_push_back(&self->cart, NULL, sizeof(struct ItemInfo)), &info);
        item_info_destruct(&info);
    }
}

static void cart_shrunk(struct UInCashWnd *self)
{
    int n = session_get_shop_buy_list_size(&g_session1);

    if (n == 3) {
        self->cart_pos = 0;
        ui_scroll_bar_set_pos(self->scroll, 0);
        ui_scroll_bar_set_range(self->scroll, 4, 4, 4);
        ui_window_invalidate_base(&self->scroll->base);
        ui_window_set_show_base(&self->scroll->base, 0);
    } else if (n > 3 && n < 7) {
        ui_scroll_bar_set_range(self->scroll, 4, 4, session_get_shop_buy_list_size(&g_session1) + 1);
        if (self->cart_pos > session_get_shop_buy_list_size(&g_session1) - 3) {
            self->cart_pos = session_get_shop_buy_list_size(&g_session1) - 3;
            ui_scroll_bar_set_pos(self->scroll, self->cart_pos);
        }
        ui_window_invalidate_base(&self->scroll->base);
    }
}

static void cart_grew(struct UInCashWnd *self)
{
    int n = session_get_shop_buy_list_size(&g_session1);

    if (n < 4 || session_get_shop_buy_list_size(&g_session1) >= NCASH_CART_MAX)
        return;
    if (session_get_shop_buy_list_size(&g_session1) == 4) {
        self->cart_pos = 1;
        ui_window_set_show_base(&self->scroll->base, 1);
        ui_scroll_bar_set_range(self->scroll, 4, 4, 5);
    } else {
        self->cart_pos = session_get_shop_buy_list_size(&g_session1) - 3;
        ui_scroll_bar_set_range(self->scroll, 4, 4, session_get_shop_buy_list_size(&g_session1) + 1);
    }
    ui_scroll_bar_set_pos(self->scroll, self->cart_pos);
    ui_window_invalidate_base(&self->scroll->base);
}

static void take_off_cart(struct UInCashWnd *self, const struct ItemInfo *info)
{
    session_purchase_list_remove(&g_session1, info);
    self->cart_total -= info->count * info->charged_price;
}

static int row_at(struct UInCashWnd *self, int y)
{
    int row;

    for (row = 0; row < NCASH_ROWS; row++) {
        int lo = self->grid_top + row * self->row_height;

        if (y >= lo && y <= lo + self->row_height)
            return row;
    }
    return -1;
}

static struct ItemInfo *copy_entry(struct ItemInfo *out, struct List *list, int pos)
{
    struct ListNode *node = list->head->next;

    for (; pos > 0; pos--)
        node = node->next;
    if (node != list->head)
        return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
    return item_info_construct_with_arg(out);
}

static int goods_in_reach(struct UInCashWnd *self, int index)
{
    int count = session_cash_goods_count(&g_session1, self->tab);

    if (self->tab == NCASH_TAB_SALE)
        count = self->base.items.size;
    return count > index;
}

static struct ItemInfo *u_in_cash_wnd_hit_test(struct UInCashWnd *self, struct ItemInfo *out, int x, int y)
{
    int row, index;

    if ((unsigned)(x - 0x40) > 0x22c || y < 0x32 || y > self->top + 0x1b8)
        return item_info_construct_with_arg(out);
    if (x < 0x1bc) {
        int column = x < 0xfc ? 0 : 1;

        row = row_at(self, y);
        if (row < 0)
            return copy_entry(out, &self->base.items, 0);
        index = (self->page - 1) * NCASH_PAGE_GOODS + column * NCASH_ROWS + row;
        if (!goods_in_reach(self, index))
            return item_info_construct_with_arg(out);
        self->hit = column * NCASH_ROWS + row;
        return copy_entry(out, &self->base.items, index);
    }
    row = row_at(self, y);
    if (row < 0) {
        index = self->hit;
    } else {
        index = self->cart_pos + row;
        self->hit = index;
        if (session_get_shop_buy_list_size(&g_session1) <= index)
            return item_info_construct_with_arg(out);
    }
    self->hit = ~index;
    return copy_entry(out, &self->cart, index);
}

void u_in_cash_wnd_store_info(struct UIWindow *window)
{
    struct UInCashWnd *self = (struct UInCashWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    if (!self->info)
        return;
    self->info->width = wnd->width;
    self->info->height = wnd->height;
    self->info->x = wnd->x;
    self->info->y = wnd->y;
}

static void set_button_bitmap(struct UIWindow *button, const struct String *dir, const char *name, int state)
{
    struct String path;

    string_copy(&path, dir);
    string_append_cstr(&path, name);
    string_append_cstr(&path, ".bmp");
    ui_bitmap_button_set_bitmap((struct UIBitmapButton *)button, string_cstr(&path), state, 0);
    string_free(&path);
}

static const char *const button_bitmaps[NCASH_BUTTON_COUNT][3] = {
    { "신규" "b", "신규" "a", "신규" "a" },
    { "인기" "b", "인기" "a", "인기" "a" },
    { "한정" "b", "한정" "a", "한정" "a" },
    { "임대장비" "b", "임대장비" "a",
      "임대장비" "a" },
    { "영구장비" "b", "영구장비" "a",
      "영구장비" "a" },
    { "버프" "b", "버프" "a", "버프" "a" },
    { "회복" "b", "회복" "a", "회복" "a" },
    { "기타" "b", "기타" "a", "기타" "a" },
    { "특가" "b", "특가" "a", "특가" "a" },
    { "검색" "b", "검색" "a", "검색" "a" },
    { "NC_FirstPage", "NC_FirstPage", "NC_FirstPage" },
    { "NC_PrevPage", "NC_PrevPage", "NC_PrevPage" },
    { "NC_NextPage", "NC_NextPage", "NC_NextPage" },
    { "NC_LastPage", "NC_LastPage", "NC_LastPage" },
    { "btn_buy", "btn_buy_a", "btn_buy_b" },
    { "btn_cancel", "btn_cancel_a", "btn_cancel_b" },
    { "btn_close", "btn_close_a", "btn_close_b" },
    { "btn_find", "btn_find_a", "btn_find_b" },
    { "btn_refresh", "btn_refresh_a", "btn_refresh_b" },
};
static const int button_ids[NCASH_BUTTON_COUNT] = {
    NCASH_ID_TAB0, NCASH_ID_TAB0 + 1, NCASH_ID_TAB0 + 2, NCASH_ID_TAB0 + 3, NCASH_ID_TAB0 + 4,
    NCASH_ID_TAB0 + 5, NCASH_ID_TAB0 + 6, NCASH_ID_TAB0 + 7, NCASH_ID_TAB0 + 8, NCASH_ID_TAB0 + 9,
    NCASH_ID_FIRST_PAGE, NCASH_ID_PREV_PAGE, NCASH_ID_NEXT_PAGE, NCASH_ID_LAST_PAGE,
    NCASH_ID_BUY, NCASH_ID_CANCEL, NCASH_ID_CLOSE, NCASH_ID_FIND, NCASH_ID_REFRESH,
};
#define BUTTONS_IN_BASIC 15

static void button_pos(struct UInCashWnd *self, int i, int *x, int *y)
{
    struct UIWindow *wnd = &self->base.frame.base;

    if (i < NCASH_TAB_COUNT) {
        *x = 4;
        *y = self->top + 0x2a + i * 0x2d;
    } else if (i < 14) {
        static const int xs[4] = { 0xcb, 0xd8, 0x114, 0x122 };

        *x = xs[i - 10];
        *y = wnd->height - 0x37;
    } else if (i < 17) {
        *x = wnd->width - 0x96 + (i - 14) * 0x32;
        *y = wnd->height - 0x28;
    } else if (i == 17) {
        *x = self->search->width + self->search->x + 10;
        *y = 0x16;
    } else {
        *x = -100;
        *y = -100;
    }
}

static const char *button_tip(int i)
{
    if (i < 8)
        return msg_str(MSG_NCASH_TAB_TIP + i);
    switch (i) {
    case 8: return msg_str(0x9d8);
    case 10: case 18: return msg_str(0x447);
    case 11: return msg_str(0x734);
    case 12: return "<=";
    case 13: return "=>";
    case 14: return msg_str(0x735);
    case 15: return msg_str(0x732);
    case 16: return msg_str(0x733);
    case 17: return msg_str(0x513);
    }
    return NULL;
}

static struct UIWindow *add_edit(struct UInCashWnd *self, int width, int height, int x, int y, int max_chars)
{
    struct UIWindow *edit = mem_alloc(sizeof(struct UIEditCtrl));

    if (edit)
        edit = ui_edit_ctrl2_construct(edit);
    ui_window_create(edit, width, height);
    ((struct UIEditCtrl *)edit)->multiline = 1;
    ((struct UIEditCtrl *)edit)->no_border = 0;
    ui_window_move(edit, x, y);
    ((struct UIEditCtrl *)edit)->max_chars = max_chars;
    ui_edit_ctrl_set_back_colour(edit, 0xd7, 0xd7, 0xd7);
    ui_window_add_child(&self->base.frame.base, edit);
    return edit;
}

int u_in_cash_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
void u_in_cash_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UInCashWnd *self = (struct UInCashWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct String basic_dir, skin_dir;
    struct UIWindow *title;
    struct UIScrollBar *scroll;
    int title_width, i;

    session_clear_lists(&g_session1);
    list_clear(&self->cart, item_info_destroy);
    self->page_count = 1;
    self->page = 1;
    session_clear_item_list(&g_session1, CASH_TAB_SEARCH);
    u_in_cash_wnd_send_msg(wnd, wnd, NCASH_MSG_LIST_GOODS, 0, 0, 0, 0);
    ui_window_add_close_button(wnd, "Alt+X", 0);

    title_width = ui_window_text_width(wnd, msg_str(MSG_NCASH_TITLE), 0, 0, 0xc, 1, 0);
    title = mem_alloc(sizeof(struct UIStaticText));
    if (title)
        title = ui_static_text_construct(title);
    self->title = title;
    ui_window_create(title, title_width, 0xc);
    ui_static_text_set_text(self->title, msg_str(MSG_NCASH_TITLE), 1);
    ui_window_move(self->title, 0x14, 0x1a);
    ui_window_add_child(wnd, self->title);

    self->search = add_edit(self, 0x40, 0x12, self->title->x + title_width, 0x16, 0x14);

    string_init(&basic_dir);
    string_assign(&basic_dir, NCASH_DIR, sizeof NCASH_DIR - 1);
    string_init(&skin_dir);
    string_assign(&skin_dir, UI_SKIN, sizeof UI_SKIN - 1);
    for (i = 0; i < NCASH_BUTTON_COUNT; i++) {
        const struct String *dir = i < BUTTONS_IN_BASIC ? &basic_dir : &skin_dir;
        struct UIWindow *button = mem_alloc(sizeof(struct UIBitmapButton));
        const char *tip;
        int x, y;

        if (button)
            button = (struct UIWindow *)ui_bitmap_button_construct((struct UIBitmapButton *)button);
        self->buttons[i] = button;
        set_button_bitmap(button, dir, button_bitmaps[i][0], 0);
        set_button_bitmap(button, dir, button_bitmaps[i][1], 1);
        set_button_bitmap(button, dir, button_bitmaps[i][2], 2);
        set_button_bitmap(button, dir, button_bitmaps[i][1], 3);
        ui_window_create(button, ((struct UIBitmapButton *)button)->bitmap_width,
                         ((struct UIBitmapButton *)button)->bitmap_height);
        button_pos(self, i, &x, &y);
        ui_window_move_base(button, x, y);
        button->id = button_ids[i];
        tip = button_tip(i);
        if (tip)
            ui_bitmap_button_set_tool_tip((struct UIBitmapButton *)button, tip);
        ui_window_add_child(wnd, button);
    }
    string_free(&skin_dir);
    string_free(&basic_dir);

    self->count = add_edit(self, 0x40, 0xe, 0x166, wnd->height - 0x26, 0x40);

    scroll = mem_alloc(sizeof(struct UIScrollBar));
    if (scroll)
        scroll = ui_scroll_bar_construct(scroll);
    self->scroll = scroll;
    ui_scroll_bar_create(scroll, wnd->width - 0xf, 0x11, 1, self->top + 0x1af);
    ui_scroll_bar_set_range(self->scroll, 4, 4, 8);
    self->cart_pos = 0;
    ui_scroll_bar_set_pos(self->scroll, 0);
    ui_window_add_child(wnd, &self->scroll->base);
    ui_window_set_show_base(&self->scroll->base, 0);
}

static void draw_collection_bitmap(struct UIWindow *wnd, const struct ItemInfo *item, int x, int y)
{
    struct String name, moved, path;
    const char *res = item_info_get_res_name(item);

    string_init(&name);
    string_assign_cstr(&name, res);
    string_insert(&name, 0, UI_SKIN "collection\\", 0x1a);
    moved = name;
    string_init(&name);
    string_append_cstr(&moved, ".bmp");
    string_move(&path, &moved);
    string_free(&name);
    if (path.size)
        ui_window_draw_bitmap(wnd, x, y, res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
    string_free(&path);
}

static int draw_goods_name(struct UIWindow *wnd, const struct ItemInfo *item, int x, int cx, int y)
{
    int width = ui_window_draw_item_name(wnd, x, NCASH_MEASURE_Y, item, 0, 0);

    ui_window_draw_item_name_short(wnd, cx - width / 2, y + 4, width, item, 0xc);
    return width;
}

static void draw_goods_and_cart(struct UInCashWnd *self)
{
    struct UIWindow *wnd = &self->base.frame.base;
    struct ListNode *node;
    char text[0x40], money[0x40];
    int index = 0, drawn = 0, x, y, len;

    for (node = self->base.items.head->next; node != self->base.items.head; node = node->next, index++) {
        struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);

        if (index < self->page * NCASH_PAGE_GOODS - NCASH_PAGE_GOODS)
            continue;
        if (drawn < NCASH_ROWS) {
            y = self->row_height * drawn;
            x = 0x47;
        } else {
            y = (drawn - NCASH_ROWS) * self->row_height;
            x = 0x101;
        }
        y += self->grid_top;
        draw_collection_bitmap(wnd, item, x, y);
        draw_goods_name(wnd, item, x, x + 0x5a, y);
        len = sprintf(text, msg_str(MSG_NCASH_GOODS_PRICE));
        ui_window_text_out(wnd, x + 0x70, y + 0x40, text, len, 0, 0xc, 0, 0, 0);
        len = sprintf(text, " %s C", format_money(item->price, money));
        ui_window_text_out_right(wnd, x + 0x98, y + 0x54, text, len, 0, 0xc, 0, 0);
        if (++drawn > NCASH_PAGE_GOODS - 1)
            break;
    }
    index = 0;
    drawn = 0;
    for (node = self->cart.head->next; node != self->cart.head; node = node->next, index++) {
        struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);

        y = self->row_height * drawn + self->grid_top;
        if (index < self->cart_pos || index >= self->cart_pos + NCASH_ROWS)
            continue;
        draw_collection_bitmap(wnd, item, 0x1c5, y);
        memset(text, 0, sizeof text);
        memset(money, 0, sizeof money);
        draw_goods_name(wnd, item, 0x1c5, 0x217, y);
        len = sprintf(text, msg_str(MSG_NCASH_GOODS_PRICE));
        ui_window_text_out(wnd, 0x235, y + 0x18, text, len, 0, 0xc, 0, 0, 0);
        len = sprintf(text, " %s C", format_money(item->price, money));
        ui_window_text_out(wnd, 0x226, y + 0x2a, text, len, 0, 0xc, 0, 0, 0);
        len = sprintf(text, msg_str(MSG_NCASH_CART_COUNT));
        ui_window_text_out(wnd, 0x235, y + 0x42, text, len, 0, 0xc, 0, 0, 0);
        len = sprintf(text, " %d %s", item->count, msg_str(MSG_NCASH_CART_UNIT));
        ui_window_text_out_right(wnd, 0x24e, y + 0x54, text, len, 0, 0xc, 0, 0);
        if (++drawn > NCASH_ROWS - 1)
            break;
    }
}

static unsigned long long ncash_items_key(unsigned long long key, struct List *list, int from, int count)
{
    struct ListNode *node;
    int index = 0;

    key = draw_key_int(key, (int)list->size);
    for (node = list->head->next; node != list->head && index < from + count; node = node->next, index++)
        if (index >= from)
            key = draw_key_item(key, LIST_VALUE(node, struct ItemInfo));
    return key;
}

static unsigned long long ncash_key(struct UInCashWnd *self)
{
    unsigned long long key = DRAW_KEY_START;
    int first = self->page * NCASH_PAGE_GOODS - NCASH_PAGE_GOODS;

    key = draw_key_int(key, self->base.frame.base.height);
    key = draw_key_int(key, self->page);
    key = draw_key_int(key, self->page_count);
    key = draw_key_int(key, self->points);
    key = draw_key_int(key, self->cash);
    key = draw_key_int(key, self->cart_total);
    key = draw_key_int(key, self->cart_pos);
    key = draw_key_int(key, self->row_height);
    key = draw_key_int(key, self->grid_top);
    key = ncash_items_key(key, &self->base.items, first < 0 ? 0 : first, NCASH_PAGE_GOODS);
    key = ncash_items_key(key, &self->cart, self->cart_pos, NCASH_ROWS);
    return draw_key_children(key, &self->base.frame.base);
}

BYTE u_in_cash_wnd_render(struct UIWindow *window, int force)
{
    struct UInCashWnd *self = (struct UInCashWnd *)window;

    if (ncash_key(self) != self->drawn_key)
        ui_window_invalidate_base(window);
    return ui_window_render_base(window, force);
}

void u_in_cash_wnd_on_draw(struct UIWindow *window)
{
    struct UInCashWnd *self = (struct UInCashWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct BitmapRes *bitmap;
    char text[0x40], money[0x40];
    int len, h = wnd->height;

    bitmap = res_mgr_get(g_resMgr(), ui_skin_path(NCASH_DIR "NC_BackGround3.bmp"));
    if (bitmap)
        ui_window_draw_bitmap(wnd, 0, 0, bitmap, 0);
    memset(text, 0, sizeof text);
    memset(money, 0, sizeof money);

    len = sprintf(text, "%d / %d", self->page, self->page_count);
    ui_window_text_out_right(wnd, 0x10e, h - 0x37, text, len, 0, 0xc, 0, 0);

    len = sprintf(text, msg_str(MSG_NCASH_POINTS), format_money(self->points, money));
    ui_window_draw_shadow_text(wnd, 0x4a, h - 0x24, text, len, 0, 0xffffff, 0, 0xc, 0);
    len = sprintf(text, msg_str(MSG_NCASH_CASH), format_money(self->cash, money));
    ui_window_draw_shadow_text(wnd, 0x4a, h - 0x11, text, len, 0, 0xffffff, 0, 0xc, 0);

    ui_window_text_out(wnd, 0x104, h - 0x24, msg_str(MSG_NCASH_TOTAL), 0, 0, 0xc, 0, 0, 0);
    ui_window_text_out(wnd, 0x1ab, h - 0x24, "C", 0, 0, 0xc, 0, 0, 0);
    len = sprintf(text, "Total              :      %s", format_money(self->cart_total, money));
    ui_window_draw_shadow_text(wnd, 0x104, h - 0x11, text, len, 0, 0xffffff, 0, 0xc, 0);
    ui_window_text_out(wnd, 0x1ab, h - 0x11, "C", 0, 0, 0xc, 0, 0, 0);
    draw_goods_and_cart(self);
    self->drawn_key = ncash_key(self);
}

void u_in_cash_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UInCashWnd *self = (struct UInCashWnd *)window;
    struct ItemInfo info;

    if (y < 0x11)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->base.frame), x, y);
    u_in_cash_wnd_hit_test(self, &info, x, y);
    if (info.index) {
        struct GameMode *mode = game_current_mode();

        if (self->hit < 0) {
            self->hit = ~self->hit;
            game_mode_drag_item(mode, DRAG_CASH_CART, &info, 1, -1);
        } else {
            self->goods_bind_type = info.bind_on_equip_type;
            game_mode_drag_item(mode, DRAG_CASH_GOODS, &info, 1, -1);
        }
    }
    item_info_destruct(&info);
}

void u_in_cash_wnd_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UInCashWnd *self = (struct UInCashWnd *)window;
    struct ItemInfo info;

    u_in_cash_wnd_hit_test(self, &info, x, y);
    if (info.index && self->hit < 0) {
        game_current_base_mode();
        self->hit = ~self->hit;
        take_off_cart(self, &info);
        copy_cart(self);
        cart_shrunk(self);
    }
    item_info_destruct(&info);
}

void u_in_cash_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UInCashWnd *self = (struct UInCashWnd *)window;
    struct ItemInfo info;

    u_in_cash_wnd_hit_test(self, &info, x, y);
    if (info.index && self->hit >= 0) {
        struct UIWindow *item_wnd = ui_window_open(ITEM_WND_ID);

        ui_window_send(item_wnd, NULL, UI_MSG_SHOW_ITEM, (int)&info, 0, 0, 0);
    }
    item_info_destruct(&info);
}

void u_in_cash_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UInCashWnd *self = (struct UInCashWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    if (session_get_shop_buy_list_size(&g_session1) < 4)
        return;
    u_in_cash_wnd_send_msg(wnd, wnd, delta >= 0 ? NCASH_MSG_CART_UP : NCASH_MSG_CART_DOWN, 0, 0, 0, 0);
}

static void reg_key_path(char *key)
{
    strcpy(key, g_registryKeyPath);
    strcat(key, REG_SUBKEY);
}

static void place_scroll(struct UInCashWnd *self)
{
    struct UIWindow *scroll = &self->scroll->base;

    if (self->tab == NCASH_TAB_SALE)
        ui_window_move(scroll, self->base.frame.base.width - scroll->width - 10, 0x16);
    else
        ui_window_move(scroll, -100, -100);
}

static void open_shop(struct UInCashWnd *self, int cash, int points)
{
    char key[0x200];
    struct PlatformSettingsKey *hkey = NULL;
    uint32_t type = PLATFORM_SETTING_DWORD, size = 4;
    struct UIWindow *button;

    reg_key_path(key);
    self->tab = 0;
    if (platform_settings_open(key, PLATFORM_SETTINGS_QUERY, &hkey))
        platform_settings_read(hkey, REG_TAB_VALUE, &type, &self->tab, &size);
    platform_settings_close(hkey);
    if (self->tab > 10)
        self->tab = 10;
    else if (self->tab < 0)
        self->tab = 0;
    button = self->buttons[self->tab];
    BUTTON_DISABLED(button) = 1;
    ui_window_invalidate(button);
    place_scroll(self);
    self->cash = cash;
    self->points = points;
}

static void select_tab(struct UInCashWnd *self, int id)
{
    char key[0x200];
    struct PlatformSettingsKey *hkey;
    struct UIWindow *button;

    if ((unsigned)self->tab <= 10 && self->buttons[self->tab]) {
        button = self->buttons[self->tab];
        BUTTON_DISABLED(button) = 0;
        ui_window_invalidate(button);
        ui_window_set_state(self->buttons[self->tab], 0);
    }
    self->tab = id - NCASH_ID_TAB0;
    button = self->buttons[self->tab];
    BUTTON_DISABLED(button) = 1;
    ui_window_invalidate(button);
    place_scroll(self);
    reg_key_path(key);
    if (!platform_settings_create(key, PLATFORM_SETTINGS_ALL, &hkey))
        return;
    platform_settings_write(hkey, REG_TAB_VALUE, PLATFORM_SETTING_DWORD, &self->tab, 4);
    platform_settings_close(hkey);
}

static void clear_cart(struct UInCashWnd *self)
{
    self->cart_total = 0;
    session_clear_shop_buy_list(&g_session1);
    list_clear(&self->cart, item_info_destroy);
    self->cart_pos = 0;
    ui_scroll_bar_set_pos(self->scroll, 0);
    ui_window_invalidate_base(&self->scroll->base);
    ui_window_set_show_base(&self->scroll->base, 0);
    ui_window_invalidate_base(&self->base.frame.base);
}

#pragma pack(push, 1)
struct NCashBuyHeader {
    WORD id;
    WORD len;
    WORD count;
    int points;
};
struct NCashBuyLine {
    int item_id;
    int count;
    WORD tab;
};
struct NCashRefresh {
    WORD id;
    int aid;
    int item_id;
};
#pragma pack(pop)

static void buy(struct UInCashWnd *self)
{
    struct NCashBuyHeader header;
    char text[0x800];
    char packet[NCASH_BUY_MAX_LEN];
    int count, len, i;

    if (self->cart_total <= 0)
        return;
    if (atoi(ui_edit_ctrl_get_text(self->count)) > self->points
        || self->cart_total > atoi(ui_edit_ctrl_get_text(self->count)) + self->cash) {
        ui_chat_line(msg_str(MSG_NCASH_TOO_MUCH), 0xff);
        return;
    }
    if (atoi(ui_edit_ctrl_get_text(self->count)) > self->cart_total) {
        ui_chat_line(msg_str(MSG_NCASH_POINTS_OVER), 0xff);
        return;
    }
    header.id = PACKET_NCASH_BUY;
    header.points = atoi(ui_edit_ctrl_get_text(self->count));
    if (header.points)
        _snprintf(text, 0x800, msg_str(MSG_NCASH_CONFIRM_POINTS), self->cart_total - header.points,
                     header.points);
    else
        _snprintf(text, 0x800, msg_str(MSG_NCASH_CONFIRM), self->cart_total);
    if (ui_window_mgr_message_box(&g_windowMgr, text, UI_MSGBOX_QUESTION, 1, 1, 0, "메시지", 0x118, 0x78,
                               0) != UI_MSGBOX_ANSWER_YES)
        return;
    count = session_get_shop_buy_list_size(&g_session1);
    len = (count + 1) * 10;
    if ((unsigned)len > NCASH_BUY_MAX_LEN)
        return;
    header.len = (WORD)len;
    header.count = (WORD)count;
    memcpy(packet, &header, sizeof header);
    for (i = 0; i < count; i++) {
        struct ItemInfo info;
        struct NCashBuyLine line;

        session_get_shop_buy_item_at(&g_session1, &info, i);
        line.count = info.count;
        line.item_id = atoi(string_cstr(&info.id_text));
        line.tab = info.bind_on_equip_type;
        memcpy(packet + sizeof header + i * sizeof line, &line, sizeof line);
        item_info_destruct(&info);
    }
    if (count == 0)
        return;
    rag_connection_send_packet(rag_connection_instance(), (short)len, packet);
}

static void find_goods(struct UInCashWnd *self)
{
    struct UIWindow *wnd = &self->base.frame.base;
    struct String text;
    int tab, i, n;

    string_init(&text);
    string_assign_cstr(&text, ui_edit_ctrl_get_text(self->search));
    if (text.size) {
        session_clear_item_list(&g_session1, CASH_TAB_SEARCH);
        for (tab = 0; tab < 9; tab++) {
            n = session_cash_goods_count(&g_session1, tab);
            for (i = 0; i < n; i++) {
                struct ItemInfo info;
                struct String name;

                session_get_cash_goods(&g_session1, &info, tab, i);
                string_copy(&name, &info.name);
                if (text_view_find(string_view(&name), string_view(&text), 0) != STRING_NPOS
                    && !session_has_cash_search_result(&g_session1, item_info_get_item_id(&info)))
                    item_info_copy(list_push_back(&g_session1.cashShopGoods[10], NULL, sizeof(struct ItemInfo)), &info);
                string_free(&name);
                item_info_destruct(&info);
            }
        }
        if (session_cash_goods_count(&g_session1, CASH_TAB_SEARCH) == 0)
            ui_message_box(msg_str(MSG_NCASH_NOT_FOUND));
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->search, "");
        u_in_cash_wnd_send_msg(wnd, wnd, UI_MSG_COMMAND, NCASH_ID_TAB0 + 9, 0, 0, 0);
    }
    string_free(&text);
}

static void refresh_goods(struct UInCashWnd *self)
{
    struct ListNode *node;

    for (node = self->base.items.head->next; node != self->base.items.head; node = node->next) {
        struct NCashRefresh packet;

        packet.id = PACKET_NCASH_REFRESH;
        packet.aid = g_session1.ownAid;
        packet.item_id = item_info_get_item_id(LIST_VALUE(node, struct ItemInfo));
        rag_connection_send_packet(rag_connection_instance(),
                                  rag_connection_get_packet_length(rag_connection_instance(), packet.id),
                                  (char *)&packet);
    }
}

static int limit_sale_drop(struct UInCashWnd *self, struct Mode *mode)
{
    struct DragInfo *drag = game_current_drag();
    const char *id_text = string_cstr(&drag->name);
    int in_cart = session_get_shop_buy_count_by_name(&g_session1, id_text);
    struct ListNode *node;

    for (node = self->base.items.head->next; node != self->base.items.head; node = node->next) {
        struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);
        struct DragInfo saved;
        struct String text;

        if (atoi(string_cstr(&item->id_text)) != atoi(string_cstr(&drag->name)))
            continue;
        if (item->count <= 0) {
            ui_message_box(msg_str(MSG_NCASH_SOLD_OUT));
            mode_send_message(mode, GAMEMODE_DRAG_END, 0, 0, 0, 0);
            return 0;
        }
        if (drag->drop_count + in_cart <= item->count)
            continue;
        drag_info_copy(&saved, drag);
        string_init(&text);
        string_format(&text, msg_str(MSG_NCASH_LIMIT), item->count);
        ui_message_box(string_cstr(&text));
        string_free(&text);
        drag_info_assign(drag, &saved);
        drag->drop_count = item->count - in_cart;
        drag_info_destroy(&saved);
        break;
    }
    return 1;
}

static void drop(struct UInCashWnd *self)
{
    struct Mode *mode = game_current_base_mode();
    const struct DragInfo *drag = game_current_drag();
    struct ItemInfo info;

    switch (drag->source) {
    case DRAG_CASH_CART:
        item_info_construct(&info);
        item_info_assign_from_drag(&info, drag);
        take_off_cart(self, &info);
        cart_shrunk(self);
        item_info_destruct(&info);
        break;
    case DRAG_CASH_GOODS:
        if (session_get_shop_buy_list_size(&g_session1) >= NCASH_CART_MAX) {
            mode_send_message(mode, GAMEMODE_DRAG_END, 0, 0, 0, 0);
            return;
        }
        if (self->tab == NCASH_TAB_SALE && !limit_sale_drop(self, mode))
            return;
        item_info_construct(&info);
        item_info_assign_from_drag(&info, drag);
        info.bind_on_equip_type = (WORD)self->goods_bind_type;
        session_add_shop_buy_item(&g_session1, &info);
        self->cart_total += info.count * info.charged_price;
        cart_grew(self);
        item_info_destruct(&info);
        break;
    }
    copy_cart(self);
    mode_send_message(mode, GAMEMODE_DRAG_END, 0, 0, 0, 0);
}

static void list_goods(struct UInCashWnd *self)
{
    int i, n;

    list_clear(&self->base.items, item_info_destroy);
    n = session_cash_goods_count(&g_session1, self->tab);
    for (i = 0; i < n; i++) {
        struct ItemInfo info;

        session_get_cash_goods(&g_session1, &info, self->tab, i);
        if (info.index && !(self->tab == NCASH_TAB_SALE && info.bind_on_equip_type == 0xffff))
            item_info_copy(list_push_back(&self->base.items, NULL, sizeof(struct ItemInfo)), &info);
        item_info_destruct(&info);
    }
    ui_window_invalidate_base(&self->base.frame.base);
}

static void show_cart_from(struct UInCashWnd *self, int pos)
{
    ui_scroll_bar_set_pos(self->scroll, pos);
    ui_window_invalidate_base(&self->scroll->base);
}

static int command(struct UInCashWnd *self, int id)
{
    struct UIWindow *wnd = &self->base.frame.base;

    if (id > NCASH_ID_1D5)
        return 0;
    if (id >= NCASH_ID_TAB0 && id < NCASH_ID_TAB0 + NCASH_TAB_COUNT || id == NCASH_ID_1D5) {
        select_tab(self, id);
        u_in_cash_wnd_send_msg(wnd, wnd, NCASH_MSG_PAGES, 0, 0, 0, 0);
        return 0;
    }
    switch (id) {
    case NCASH_ID_CLOSE: {
        WORD packet = PACKET_NCASH_CLOSE;

        rag_connection_send_packet(rag_connection_instance(),
                                  rag_connection_get_packet_length(rag_connection_instance(), PACKET_NCASH_CLOSE),
                                  (char *)&packet);
        ui_close_window(0xb8);
        return 0;
    }
    case NCASH_ID_BUY:
        buy(self);
        return 0;
    case NCASH_ID_PREV_PAGE:
        if (self->page > 1)
            self->page--;
        return 0;
    case NCASH_ID_NEXT_PAGE:
        if (self->page < self->page_count)
            self->page++;
        return 0;
    case NCASH_ID_FIND:
        find_goods(self);
        return 0;
    case NCASH_ID_CANCEL:
        clear_cart(self);
        return 0;
    case NCASH_ID_FIRST_PAGE:
        self->page = 1;
        return 0;
    case NCASH_ID_LAST_PAGE:
        self->page = self->page_count;
        return 0;
    case NCASH_ID_REFRESH:
        refresh_goods(self);
        return 0;
    }
    return 0;
}

int u_in_cash_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, int val1, int val2,
                                  int val3, int val4)
{
    struct UInCashWnd *self = (struct UInCashWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    int n, lim;

    switch (msg) {
    case UI_MSG_COMMAND:
        return command(self, val1);

    case NCASH_MSG_CART_WHEEL:
        if (session_get_shop_buy_list_size(&g_session1) < 4)
            return 0;
        if (val1 < 0)
            self->cart_pos--;
        else if (val1 > 0)
            self->cart_pos++;
        if (self->cart_pos < 0)
            self->cart_pos = 0;
        if (self->cart_pos > session_get_shop_buy_list_size(&g_session1) - 3)
            self->cart_pos = session_get_shop_buy_list_size(&g_session1) - 3;
        show_cart_from(self, self->cart_pos);
        return 0;

    case NCASH_MSG_CART_UP:
        if (--self->cart_pos < 0)
            self->cart_pos = 0;
        show_cart_from(self, self->cart_pos);
        return 0;

    case NCASH_MSG_CART_DOWN:
        self->cart_pos++;
        n = session_get_shop_buy_list_size(&g_session1) - 3;
        lim = n < 4 ? n : 4;
        if (self->cart_pos > lim)
            self->cart_pos = lim;
        show_cart_from(self, self->cart_pos);
        return 0;

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!n_cash_info_is_on_screen(self->info))
            n_cash_info_set_default(self->info);
        ui_frame_wnd_move(wnd, self->info->x, self->info->y);
        return 0;

    case NCASH_MSG_DROP:
        drop(self);
        return 0;

    case NCASH_MSG_LIST_GOODS:
        list_goods(self);
        return 0;

    case NCASH_MSG_OPEN:
        open_shop(self, val1, val2);
        u_in_cash_wnd_send_msg(wnd, wnd, NCASH_MSG_PAGES, 0, 0, 0, 0);
        return 0;

    case NCASH_MSG_PAGES:
        self->page_count = (session_cash_goods_count(&g_session1, self->tab) + NCASH_PAGE_GOODS - 1)
                           / NCASH_PAGE_GOODS;
        if (self->page_count < 1)
            self->page_count = 1;
        self->page = 1;
        u_in_cash_wnd_send_msg(wnd, wnd, NCASH_MSG_LIST_GOODS, 0, 0, 0, 0);
        return 0;

    case NCASH_MSG_RESET:
        self->cash = val1;
        self->points = val2;
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->count, "0");
        u_in_cash_wnd_send_msg(wnd, wnd, UI_MSG_COMMAND, NCASH_ID_CANCEL, 0, 0, 0);
        return 0;
    }
    return ui_frame_wnd_send_msg((struct UIWindow *)(&self->base.frame), sender, msg, val1, val2, val3, val4);
}

static void ncash_destruct(struct UInCashWnd *self)
{
    self->base.frame.base.ops = &u_in_cash_wnd_ops;
    list_free(&self->cart, item_info_destroy);
    ui_item_base_wnd_destruct(&self->base);
}

void u_in_cash_wnd_destroy(struct UIWindow *window)
{
    struct UInCashWnd *self = (struct UInCashWnd *)window;
    ncash_destruct(self);
    mem_free(self);
}

#define NCASH_X_SPLIT 0x1bd
#define COUNT_FOR_CART 0x13
#define COUNT_FOR_GOODS 0x14

void u_in_cash_wnd_drag_and_drop(struct UIWindow *self, int x, int y, const struct DragInfo *drag)
{
    struct Mode *mode;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(self, x, y, drag);
        return;
    }
    mode = game_current_base_mode();
    if (drag->source == DRAG_CASH_CART && x < NCASH_X_SPLIT) {
        int count = session_get_shop_buy_count_by_name(&g_session1, string_cstr(&drag->name));

        if (count <= 1) {
            ui_window_send_msg(self, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
            return;
        }
        ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
        ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, COUNT_FOR_GOODS, count, 0, 0);
        return;
    }
    if (drag->source == DRAG_CASH_GOODS && x >= NCASH_X_SPLIT) {
        ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
        ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, COUNT_FOR_CART, 0, 0, 0);
        return;
    }
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
}

void u_in_cash_wnd_store_info(struct UIWindow *self);
void u_in_cash_wnd_on_create(struct UIWindow *self, int width, int height);
void u_in_cash_wnd_on_draw(struct UIWindow *self);
BYTE u_in_cash_wnd_render(struct UIWindow *self, int force);
void u_in_cash_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void u_in_cash_wnd_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void u_in_cash_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void u_in_cash_wnd_on_wheel(struct UIWindow *self, int delta);
void u_in_cash_wnd_destroy(struct UIWindow *window);
const struct UIWindowOps u_in_cash_wnd_ops = {
    .destroy = u_in_cash_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 1,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = u_in_cash_wnd_drag_and_drop,
    .store_info = u_in_cash_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = u_in_cash_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = u_in_cash_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = u_in_cash_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = u_in_cash_wnd_on_l_btn_dbl_clk,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = u_in_cash_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = u_in_cash_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = u_in_cash_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = u_in_cash_wnd_render,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_FRAME,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UInCashWnd *u_in_cash_wnd_construct(struct UInCashWnd *self)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &u_in_cash_wnd_ops;
    self->base.frame.base.redraws_tree = 1;
    self->drawn_key = 0;
    list_init(&self->cart);
    self->info = NULL;
    self->tab = 0;
    self->hit = 0;
    self->page_count = 0;
    self->page = 0;
    self->goods_bind_type = 0;
    self->cart_total = 0;
    self->cart_pos = 0;
    self->cash = 0;
    self->points = 0;
    self->row_height = 0x6a;
    self->grid_top = 0x38;
    self->top = 0x20;
    return self;
}

struct UIWindow *u_in_cash_wnd_new(void)
{
    return (struct UIWindow *)u_in_cash_wnd_construct(mem_alloc(sizeof(struct UInCashWnd)));
}
