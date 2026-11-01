#include "world/tooltip.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "ui/window_access.h"
#include "ui/shops/npc_shop_internal.h"
#include "game/context.h"
#include "core/msg_str.h"
#include "core/skin_path.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "ui/wndmgr/wndmgr.h"

void npc_shop_price_unit_long(char *buf, int cash)
{
    sprintf(buf, "Zeny");
    if (!cash)
        return;
    if (g_serviceType == 6)
        sprintf(buf, "Koin");
    else if (g_serviceType != 0)
        sprintf(buf, "Point");
    else if (g_session1.channelCopId == 1)
        sprintf(buf, "한코인");
    else
        sprintf(buf, msg_str(MSG_SHOP_UNIT_CASH));
}

void shop_tooltip_cell(struct UIWindow *wnd, int x, int y, int *tip_x, int *tip_y)
{
    *tip_x = (x - ITEM_GRID_X) / ITEM_GRID_CELL * ITEM_GRID_CELL + ITEM_GRID_X;
    *tip_y = (y - ITEM_GRID_Y) / ITEM_GRID_CELL * ITEM_GRID_CELL + 1;
    ui_window_to_screen(wnd, tip_x, tip_y);
}

static void price_tooltip(struct String *out, const struct ItemInfo *info, const char *unit, int with_count,
                          int options)
{
    struct String text;

    struct String name = {0};
    char money[0x40];
    int changed = info->price != info->charged_price;
    const char *n;

    item_info_get_display_name((struct ItemInfo *)info, &name);
    n = string_cstr(&name);
    format_money(info->charged_price, money);
    if (options) {
        const char *u = unit ? unit : "Zeny";

        string_init(&text);
        if (with_count && changed)
            string_format(&text, msg_str(MSG_TIP_COUNT_OPTIONS), n, info->option_count, info->count,
                          msg_str(MSG_PRICE_COUNT), info->price, money, u);
        else if (with_count)
            string_format(&text, msg_str(MSG_TIP_COUNT_OPTIONS_SAME), n, info->option_count, info->count,
                          msg_str(MSG_PRICE_COUNT), money, u);
        else if (changed)
            string_format(&text, msg_str(MSG_TIP_OPTIONS), n, info->option_count, info->price, money, u);
        else
            string_format(&text, msg_str(MSG_TIP_OPTIONS_SAME), n, info->option_count, money, u);
    } else if (unit) {
        string_init(&text);
        if (with_count && changed)
            string_format(&text, "%s %d %s %d -> %s %s", n, info->count, msg_str(MSG_PRICE_COUNT), info->price,
                          money, unit);
        else if (with_count)
            string_format(&text, "%s %d %s %s %s", n, info->count, msg_str(MSG_PRICE_COUNT), money, unit);
        else if (changed)
            string_format(&text, "%s %d -> %s %s", n, info->price, money, unit);
        else
            string_format(&text, "%s %s %s", n, money, unit);
    } else {
        string_init(&text);
        if (with_count && changed)
            string_format(&text, "%s %d %s %d -> %s Zeny", n, info->count, msg_str(MSG_PRICE_COUNT),
                          info->price, money);
        else if (with_count)
            string_format(&text, "%s %d %s %s Zeny", n, info->count, msg_str(MSG_PRICE_COUNT), money);
        else if (changed)
            string_format(&text, "%s %d -> %s Zeny", n, info->price, money);
        else
            string_format(&text, "%s %s Zeny", n, money);
    }
    string_free(out);
    string_move(out, &text);
    string_free(&name);
}

void npc_shop_show_price_tooltip(struct UIItemBaseWnd *base, int x, int y, const char *unit, int with_count,
                               int options_ok)
{
    struct ItemInfo info;

    ui_item_base_wnd_item_at(base, &info, x, y);
    if (info.index) {
        struct String text;
        int tip_x, tip_y;

        shop_tooltip_cell(&base->frame.base, x, y, &tip_x, &tip_y);
        string_init(&text);
        price_tooltip(&text, &info, unit, with_count, options_ok && info.identified == 1 && info.option_count);
        mode_show_tooltip(client_current_mode(), string_cstr(&text), tip_x, tip_y,
                          session_get_item_dis_color(&info), 0);
        string_free(&text);
    }
    item_info_destruct(&info);
}

void shop_draw_side_strips(struct UIWindow *wnd, int height)
{
    int strips, i, y;

    strips = height / 8 + (height % 8 != 0);
    for (i = 0, y = ITEM_GRID_Y; i < strips; i++, y += 8)
        ui_window_draw_bitmap(wnd, 0, y,
                            res_mgr_get(g_resMgr(), ui_skin_path(UI_SKIN "basic_interface\\itemwin_left.bmp")),
                            0);
    strips = height / 8 + (height % 8 != 0);
    for (i = 0, y = ITEM_GRID_Y; i < strips; i++, y += 8) {
        struct BitmapRes *right = res_mgr_get(g_resMgr(),
                                                ui_skin_path(UI_SKIN "basic_interface\\itemwin_right.bmp"));

        ui_window_draw_bitmap(wnd, wnd->width - right->width, y, right, 0);
    }
}

void shop_draw_frame(struct UIItemBaseWnd *base, int body, int bar_variant)
{
    struct UIWindow *wnd = &base->frame.base;
    COLORREF colour;

    shop_draw_side_strips(wnd, body);
    colour = ui_skin_colour(2, 2);
    ui_window_fill_rect(wnd, 0x14, ITEM_GRID_Y, (wnd->width - 0x3c) % ITEM_GRID_CELL, body, colour);
    ui_window_fill_rect(wnd, 0x14, body / ITEM_GRID_CELL * ITEM_GRID_CELL + ITEM_GRID_Y, wnd->width - 0x18,
                      body % ITEM_GRID_CELL, colour);
    ui_window_draw_button_bar(wnd, bar_variant, 0, 1);
}

void shop_draw_cells(struct UIWindow *wnd, int rows)
{
    int i, y;

    for (i = 0, y = ITEM_GRID_Y; i < rows; i++, y += ITEM_GRID_CELL)
        ui_window_draw_item_win_mid(wnd, ITEM_GRID_X, y);
}

void shop_draw_backdrop(struct UIItemBaseWnd *base, int body, int cell_rows)
{
    shop_draw_frame(base, body, base->button_bar ? 2 : 0);
    shop_draw_cells(&base->frame.base, cell_rows);
}

void shop_fill_names(struct UIWindow *wnd, int rows)
{
    int columns = (wnd->width - ITEM_GRID_X) / ITEM_GRID_CELL;

    ui_window_fill_rect(wnd, 0x48, ITEM_GRID_Y, columns * ITEM_GRID_CELL - ITEM_GRID_CELL, rows * ITEM_GRID_CELL,
                      ui_skin_colour(2, 2));
}

void shop_draw_lines(struct UIItemBaseWnd *base, int hover, int priced, const char *unit)
{
    struct UIWindow *wnd = &base->frame.base;
    struct ListNode *node;
    int i, cell = 0;

    for (node = base->items.head->next, i = 0; node != base->items.head; node = node->next, i++) {
        struct ItemInfo *info = LIST_VALUE(node, struct ItemInfo);
        int row_y, col_x;

        if (i < base->scroll * base->columns || item_grid_hidden(info))
            continue;
        row_y = cell / base->columns * ITEM_GRID_CELL;
        col_x = cell % base->columns * ITEM_GRID_CELL;
        if (hover == i)
            ui_window_fill_rect(wnd, col_x + 0x48, row_y + 0x13, 0xbf, 0x1c, ui_skin_colour(6, 2));
        ui_window_draw_item_icon_count(wnd, col_x + 0x2c, row_y + 0x15, info, 0, 0);
        if (priced) {
            int name_width = 100;

            if (info->price != info->charged_price) {
                if (info->price >= 10000)
                    name_width = 0x2d;
                else if (info->price >= 1000)
                    name_width = 0x37;
                else if (info->price >= 100)
                    name_width = 0x46;
                else if (info->price >= 10)
                    name_width = 0x50;
            }
            ui_window_draw_item_name_short(wnd, col_x + 0x4c, row_y + 0x1d, name_width, info, 12);
        } else {
            ui_window_draw_item_name(wnd, col_x + 0x4c, row_y + 0x1d, info, 0, 0);
        }
        if (info->grade > 0) {
            struct String path;

            string_init(&path);
            string_format(&path, UI_SKIN "grade_enchant\\grade_icon%d.bmp", (int)info->grade);
            ui_window_draw_bitmap(wnd, col_x + 0x2c, row_y + 0x21,
                                res_mgr_get(g_resMgr(), string_cstr(&path)), 1);
            string_free(&path);
        }
        if (priced) {
            char text[0x40], money[0x40];
            int len;

            if (unit && info->price != info->charged_price)
                len = sprintf(text, " %d -> %s %s", info->price, format_money(info->charged_price, money), unit);
            else if (unit)
                len = sprintf(text, " %s %s", format_money(info->price, money), unit);
            else if (info->price != info->charged_price)
                len = sprintf(text, " %d -> %s Zeny", info->price, format_money(info->charged_price, money));
            else
                len = sprintf(text, " %s Zeny", format_money(info->price, money));
            ui_window_text_out_right(wnd, wnd->width - 0x12, row_y + 0x1d, text, len, 0, 12, 0, 0);
        }
        if (++cell == base->rows * base->columns)
            break;
    }
}

struct UIBitmapButton *shop_place_button(struct UIWindow *wnd, struct UIBitmapButton *button,
                                         const char *const names[3], int x, int y, int id)
{
    int state;

    for (state = 0; state < 3; state++) {
        char path[0x80];

        sprintf(path, UI_SKIN "%s.bmp", names[state]);
        ui_bitmap_button_set_bitmap(button, path, state, 0);
    }
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, x, y);
    button->button.base.id = id;
    ui_window_add_child(wnd, &button->button.base);
    return button;
}

struct UIBitmapButton *shop_add_button(struct UIWindow *wnd, const char *const names[3], int x, int y, int id)
{
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    if (button)
        button = ui_bitmap_button_construct(button);
    return shop_place_button(wnd, button, names, x, y, id);
}

const char *const g_shopBuyButton[3] = { "btn_buy", "btn_buy_a", "btn_buy_b" };
const char *const g_shopCancelButton[3] = { "btn_cancel", "btn_cancel_a", "btn_cancel_b" };

struct UIWindow *shop_add_title(struct UIWindow *wnd, int width, int msg)
{
    struct UIWindow *title = mem_alloc(sizeof(struct UIStaticText));

    if (title)
        title = ui_static_text_construct(title);
    ui_window_create(title, width, 0xf);
    ui_static_text_set_text(title, msg_str(msg), 0);
    ui_window_move(title, SHOP_TITLE_X, 2);
    ((struct UIStaticText *)title)->white_shadow = 1;
    ui_window_add_child(wnd, title);
    return title;
}

void shop_add_resizer(struct UIItemBaseWnd *base, int edge)
{
    struct UIWindow *wnd = &base->frame.base;
    struct UIResizer *resizer = mem_alloc(sizeof *resizer);

    if (resizer)
        resizer = ui_resizer_construct(resizer);
    base->resizer = resizer;
    ui_window_create(&resizer->base, 0x10, 0x10);
    ui_resizer_set_edge(base->resizer, edge);
    ui_resizer_set_bitmap(base->resizer, UI_SKIN "btn_resize.bmp");
    ui_window_move_base(&base->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours(base->resizer, 0xffff00ff, 0xffff00ff);
    ui_window_add_child(wnd, &base->resizer->base);
}

void shop_add_scroll_bar(struct UIItemBaseWnd *base, int length)
{
    struct UIWindow *wnd = &base->frame.base;
    struct UIScrollBar *bar = mem_alloc(sizeof *bar);

    if (bar)
        bar = ui_scroll_bar_construct(bar);
    base->scroll_bar = bar;
    ui_scroll_bar_create(bar, wnd->width - 0xf, 0xe, 1, length);
    ui_window_add_child(wnd, &base->scroll_bar->base);
}

void npc_shop_store_info(struct UIWindow *wnd, struct WindowInfo *info)
{
    if (!info)
        return;
    info->width = wnd->width;
    info->height = wnd->height;
    info->x = wnd->x;
    info->y = wnd->y;
}

static void show_item_window(const struct ItemInfo *info, int toggle)
{
    ui_window_send(ui_window_open(ITEM_WND_ID), NULL, UI_MSG_SHOW_ITEM, (int)info, toggle, 0, 0);
}

void npc_shop_drop_or_count(struct UIItemBaseWnd *base, int at_once, int val1, int for_what)
{
    if (at_once) {
        ui_window_send_msg(&base->frame.base, NULL, UI_MSG_ADD_DRAGGED, val1, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, for_what, 0, 0, 0);
}

void npc_shop_pass_on(struct UIItemBaseWnd *base, int x, int y, struct UIWindow *target, int target_id)
{
    const struct DragInfo *drag;

    ui_window_on_l_btn_down(&base->frame.base, x, y);
    drag = game_current_drag();
    if (!drag)
        return;

    ui_window_drag_and_drop(target, x, y, drag);
}

void npc_shop_list_press(struct UIItemBaseWnd *base, int *hover, int x, int y, int from_sell)
{
    struct ItemInfo info;
    int old;

    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&base->frame), x, y);
    old = *hover;
    *hover = ui_item_base_wnd_index_at(base, x, y);
    if (old != *hover)
        ui_window_invalidate_base(&base->frame.base);

    ui_item_base_wnd_item_at(base, &info, x, y);
    if (!info.index) {
        struct ItemInfo hovered;

        ui_item_base_wnd_get_item(base, &hovered, *hover);
        item_info_assign(&info, &hovered);
        item_info_destruct(&hovered);
        if (info.index)
            show_item_window(&info, 1);
    }
    if (info.index) {
        struct GameMode *mode = game_current_mode();
        struct ItemInfo held;

        if (from_sell) {
            session_get_shop_sell_item_by_index(&g_session1, &held, info.index);
            item_info_assign(&info, &held);
            item_info_destruct(&held);
            game_mode_drag_item(mode, DRAG_SELL_LIST, &info, info.count, -1);
        } else {
            session_get_inventory_item(&g_session1, &held, info.index);
            game_mode_drag_item(mode, DRAG_SHOP_GOODS, &info, held.index ? info.count : 1, -1);
            item_info_destruct(&held);
        }
    }
    item_info_destruct(&info);
}

void npc_shop_describe_hovered(struct UIItemBaseWnd *base, int *hover, int x, int y)
{
    struct ItemInfo info;
    int old = *hover;

    *hover = ui_item_base_wnd_index_at(base, x, y);
    if (old != *hover)
        ui_window_invalidate_base(&base->frame.base);
    ui_item_base_wnd_get_item(base, &info, *hover);
    if (info.index) {
        game_current_base_mode();
        show_item_window(&info, 0);
    }
    item_info_destruct(&info);
}

void npc_shop_describe_at(struct UIItemBaseWnd *base, int x, int y, int asks_mode)
{
    struct ItemInfo info;

    ui_item_base_wnd_item_at(base, &info, x, y);
    if (info.index) {
        if (asks_mode)
            game_current_base_mode();
        show_item_window(&info, 0);
    }
    item_info_destruct(&info);
}

int npc_shop_refill(struct UIItemBaseWnd *base, int count,
                  struct ItemInfo *(*at)(struct Session *, struct ItemInfo *, int))
{
    int i, total = 0;

    list_clear(&base->items, item_info_destroy);
    for (i = 0; i < count; i++) {
        struct ItemInfo info;

        at(&g_session1, &info, i);
        if (info.index) {
            total += info.count * info.charged_price;
            item_info_copy(list_push_back(&base->items, NULL, sizeof(struct ItemInfo)), &info);
        }
        item_info_destruct(&info);
    }
    return total;
}

void npc_shop_list_resize(struct UIItemBaseWnd *base, int width, int height, int margin)
{
    struct UIWindow *wnd = &base->frame.base;

    if (width > SHOP_MAX_WIDTH)
        width = SHOP_MAX_WIDTH;
    if (height > SHOP_MAX_HEIGHT)
        height = SHOP_MAX_HEIGHT;
    if (width < SHOP_MIN_WIDTH)
        width = SHOP_MIN_WIDTH;
    if (height < SHOP_MIN_HEIGHT)
        height = SHOP_MIN_HEIGHT;
    width = (width - SHOP_WIDTH_BASE) / ITEM_GRID_CELL * ITEM_GRID_CELL + SHOP_WIDTH_BASE;
    height = (height - SHOP_MIN_HEIGHT) / ITEM_GRID_CELL * ITEM_GRID_CELL + SHOP_MIN_HEIGHT;
    if (width != wnd->width || height != wnd->height) {
        ui_window_resize_base(wnd, width, height);
        base->columns = 1;
        base->rows = (wnd->height - margin) / ITEM_GRID_CELL;
        ui_window_move_base(&base->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
        ui_window_move_base(&base->scroll_bar->base, wnd->width - 0xf, 0xe);
        ui_scroll_bar_resize(&base->scroll_bar->base, base->scroll_bar->base.width, wnd->height - margin);
        ui_item_base_wnd_update_scroll(base);
    }
}

void npc_shop_list_place_buttons(struct UIItemBaseWnd *base)
{
    struct UIWindow *wnd = &base->frame.base;
    struct UIBitmapButton **buttons = (struct UIBitmapButton **)&base->close;

    ui_window_move_base(&buttons[0]->button.base, wnd->width - 0x6a, wnd->height - 0x18);
    ui_window_move_base(&buttons[1]->button.base, wnd->width - 0x3d, wnd->height - 0x18);
}

void npc_shop_list_set_info(struct UIItemBaseWnd *base, struct WindowInfo *info, int edge,
                          int (*usable)(struct WindowInfo *),
                          void (*make_default)(struct WindowInfo *))
{
    if (!usable(info))
        make_default(info);
    ui_window_send_msg(&base->frame.base, NULL, UI_MSG_RESIZE, edge, info->width, info->height, 0);
    ui_frame_wnd_move(&base->frame.base, info->x, info->y);
}

static int list_info_fits(const struct WindowInfo *info)
{
    return info->width >= SHOP_WIDTH_BASE && info->height >= SHOP_MIN_HEIGHT && info->x >= 0
           && info->x <= g_renderer->width - info->width && info->y >= 0
           && info->y <= g_renderer->height - info->height;
}

int purchase_info_usable(struct WindowInfo *self)
{
    return list_info_fits(self);
}

int sell_info_usable(struct WindowInfo *self)
{
    return list_info_fits(self);
}

int shop_info_usable(struct WindowInfo *self)
{
    return list_info_fits(self);
}

void purchase_info_default(struct WindowInfo *self)
{
    self->x = 0x118;
    self->y = 0x118;
    self->width = 0x118;
    self->height = 0x98;
}

void sell_info_default(struct WindowInfo *self)
{
    self->x = 0x118;
    self->y = 0x118;
    self->width = 0x118;
    self->height = 0x98;
}

void shop_info_default(struct WindowInfo *self)
{
    self->x = 0;
    self->y = SHOP_MIN_HEIGHT;
    self->width = SHOP_WIDTH_BASE;
    self->height = 0x138;
}
