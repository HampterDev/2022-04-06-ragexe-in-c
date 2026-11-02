#include "world/tooltip.h"
#include "ui/window_manager_state.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include <string.h>
#include <mbstring.h>
#include "ui/shops/storage_internal.h"
#include "ui/runtime.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "ui/wndmgr/wndmgr.h"
#include "platform/window.h"

static struct ItemInfo *store_item_at(struct UIItemBaseWnd *base, struct ItemInfo *out, int x, int y, int left)
{
    struct ListNode *node;
    int row, cell = 0, i = 0;

    if ((unsigned)(x - left) > 0x1f || y < ITEM_GRID_Y || y >= base->rows * ITEM_GRID_CELL + ITEM_GRID_Y)
        return item_info_construct_with_arg(out);
    row = (y - ITEM_GRID_Y) / ITEM_GRID_CELL;
    for (node = base->items.head->next; node != base->items.head; node = node->next, i++) {
        if (i < base->columns * base->scroll || item_grid_hidden(LIST_VALUE(node, struct ItemInfo)))
            continue;
        if (cell == base->columns * row + (x - left) / ITEM_GRID_CELL)
            return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
        cell++;
    }
    return item_info_construct_with_arg(out);
}

void storage_draw_items(struct UIItemBaseWnd *base, int x0, int name_width)
{
    struct UIWindow *wnd = &base->frame.base;
    struct ListNode *node;
    int drawn = 0, i = 0;

    for (node = base->items.head->next; node != base->items.head; node = node->next, i++) {
        struct ItemInfo *item = LIST_VALUE(node, struct ItemInfo);
        int x, y;

        if (i < base->scroll * base->columns || item_grid_hidden(item))
            continue;
        x = drawn % base->columns * ITEM_GRID_CELL + x0;
        y = drawn / base->columns * ITEM_GRID_CELL;
        ui_window_draw_item_icon_count(wnd, x + 4, y + 0x15, item, 0, 0);
        ui_window_draw_item_name_short(wnd, x + 0x28, y + 0x1d, name_width ? name_width : wnd->width - x - 0x42,
                                   item, 0xc);
        if (item->grade > 0) {
            struct String path;

            string_init(&path);
            string_format(&path, UI_SKIN "grade_enchant\\grade_icon%d.bmp", item->grade);
            ui_window_draw_bitmap(wnd, x + 4, y + 0x21, res_mgr_get(g_resMgr(), string_cstr(&path)),
                                1);
            string_free(&path);
        }
        if (++drawn == base->rows * base->columns)
            break;
    }
}

void storage_on_lbtn_down(struct UIItemBaseWnd *base, int x, int y, int left)
{
    struct ItemInfo info;

    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&base->frame), x, y);

    store_item_at(base, &info, x, y, left);
    if (info.index && !info.move_pending) {
        struct GameMode *mode = game_current_mode();
        struct ItemInfo stored;

        session_get_storage_item(&g_session1, &stored, info.index);
        item_info_assign(&info, &stored);
        item_info_destruct(&stored);
        game_mode_drag_item(mode, DRAG_CASH_GOODS, &info, info.count, -1);
    }
    item_info_destruct(&info);
}

void storage_on_mouse_hover(struct UIItemBaseWnd *base, int x, int y, int left)
{
    struct ItemInfo info;

    store_item_at(base, &info, x, y, left);
    if (info.index) {
        struct String name = {0};
        int tip_x, tip_y;

        shop_tooltip_cell(&base->frame.base, x, y, &tip_x, &tip_y);
        item_info_get_display_name(&info, &name);
        if (info.identified == 1 && info.option_count) {
            struct String text;

            string_init(&text);
            string_format(&text, msg_str(0xa28), string_cstr(&name), info.option_count);
            mode_show_tooltip(client_current_mode(), string_cstr(&text), tip_x, tip_y,
                              session_get_item_dis_color(&info), 0);
            string_free(&text);
        } else {
            mode_show_tooltip(client_current_mode(), string_cstr(&name), tip_x, tip_y,
                              session_get_item_dis_color(&info), 0);
        }
        string_free(&name);
    }
    item_info_destruct(&info);
}

void storage_on_r_btn_down(struct UIItemBaseWnd *base, int x, int y, int left)
{
    struct ItemInfo info;

    store_item_at(base, &info, x, y, left);
    if (info.index) {
        struct Mode *mode = game_current_base_mode();

        if ((platform_async_key_state(VK_MENU) >> 8) & 0xff) {
            if (g_windowMgr.item_wnd) {
                if (game_take_item_move())
                    mode_send_message(mode, MODE_REQ_FROM_STORAGE, info.index, info.count, 0, 0);
            } else if (g_windowMgr.merchant_item_wnd && game_take_item_move()) {
                mode_send_message(mode, MODE_REQ_STORAGE_TO_CART, info.index, info.count, 0, 0);
            }
        } else {
            struct UIWindow *wnd = ui_window_open(ITEM_WND_ID);

            ui_window_send(wnd, NULL, UI_MSG_SHOW_ITEM, (int)&info, 0, 0, 0);
        }
    }
    item_info_destruct(&info);
}

void storage_on_wheel(struct UIItemBaseWnd *base, int delta)
{
    int step;

    if (!base->scrollable)
        return;
    step = base->rows > 3 ? 3 : base->rows - 1;
    ui_window_send_msg(&base->frame.base, &base->scroll_bar->base, UI_MSG_SCROLL_V, delta < 0 ? step : -step, 0, 0, 0);
}

void storage_drop(struct UIItemBaseWnd *base, int x, int y, const struct DragInfo *drag)
{
    struct GameMode *mode;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&base->frame.base, x, y, drag);
        return;
    }
    mode = game_current_mode();
    if (drag->source != DRAG_INVENTORY && drag->source != DRAG_CART) {
        mode_send_message((struct Mode *)mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (game_mode_refuse_held_item_drag(mode))
        return;
    if (drag->drop_count == 1) {
        ui_window_send_msg(&base->frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, 7, 0, 0, 0);
}

void storage_add_dragged(void)
{
    struct Mode *mode = game_current_base_mode();
    const struct DragInfo *drag = game_current_drag();

    if (drag->source == DRAG_INVENTORY)
        mode_send_message(mode, MODE_REQ_TO_STORAGE, drag->index, drag->drop_count, 0, 0);
    else if (drag->source == DRAG_CART)
        mode_send_message(mode, MODE_REQ_CART_TO_STORAGE, drag->index, drag->drop_count, 0, 0);
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
}

void storage_store_info(struct UIWindow *wnd, struct WindowInfo *info)
{
    if (!info)
        return;
    info->width = wnd->width;
    info->height = wnd->height;
    info->x = wnd->x;
    info->y = wnd->y;
}

void storage_set_count_text(struct UIWindow *wnd, struct String *text, int count, int most)
{
    char buf[0x40];

    sprintf(buf, "%d/%d", count, most);
    string_assign_cstr(text, buf);
    ui_window_invalidate(wnd);
}

static int store_type_in_category(int type, int category)
{
    switch (category) {
    case STORE_CAT_CONSUMABLE:
        return type == 0 || type == 1 || type == 2;
    case STORE_CAT_CASH:
        return type == 0x12;
    case STORE_CAT_ARMOR:
        return type == 4 || (type > 0xa && type <= 0xe);
    case STORE_CAT_WEAPON:
        return type == 5 || type == 8 || type == 9 || type == 0xf;
    case STORE_CAT_AMMO:
        return type == 0xa || type == 0x10 || type == 0x11 || type == 0x13;
    case STORE_CAT_CARD:
        return type == 6;
    case STORE_CAT_ETC:
        return type == 3 || type == 7;
    }
    return 0;
}

static int store_in_category(const struct ItemInfo *info, int category)
{
    if (item_info_is_costume(ui_stoi(string_cstr(&info->id_text))))
        return category == STORE_CAT_COSTUME;
    return store_type_in_category(info->type, category);
}

void storage_fill(struct UIItemBaseWnd *base, int category)
{
    int n = session_storage_count(&g_session1), i;

    list_clear(&base->items, item_info_destroy);
    for (i = 0; i < n; i++) {
        struct ItemInfo info;

        session_storage_item_at(&g_session1, &info, i);
        if (info.index && (category < 0 || store_in_category(&info, category)))
            item_info_copy(list_push_back(&base->items, NULL, sizeof(struct ItemInfo)), &info);
        item_info_destruct(&info);
    }
}

static int store_compose_names(struct UIItemBaseWnd *base)
{
    struct ListNode *node;

    if (!base->items.size)
        return 0;
    for (node = base->items.head->next; node != base->items.head; node = node->next) {
        struct ItemNameRuns runs = {0};

        item_info_name_runs(&base->frame.base, LIST_VALUE(node, struct ItemInfo), &runs, 1, 0);
        item_info_set_name(LIST_VALUE(node, struct ItemInfo), string_cstr(&runs.text));
        string_free(&runs.text);
    }
    return 1;
}

static int less_by_name(const void *a, const void *b)
{
    return item_info_less_by_name(a, b);
}

int storage_sort(struct UIItemBaseWnd *base, int descending)
{
    struct ListNode *head = base->items.head, *node;

    if (!store_compose_names(base))
        return 0;
    list_sort(&base->items, less_by_name);
    if (descending) {
        node = head;
        do {
            struct ListNode *next = node->next;

            node->next = node->prev;
            node->prev = next;
            node = next;
        } while (node != head);
    }
    return 1;
}

static int store_text_find(const char *text, size_t text_len, const char *needle, size_t needle_len)
{
    size_t i;

    if (needle_len > text_len)
        return 0;
    if (!needle_len)
        return 1;
    for (i = 0; i + needle_len <= text_len; i++)
        if (text[i] == needle[0] && !memcmp(text + i, needle, needle_len))
            return 1;
    return 0;
}

int storage_filter(struct UIItemBaseWnd *base, const struct String *needle, int by_mbsstr)
{
    struct ListNode *node;

    if (!store_compose_names(base))
        return 0;
    for (node = base->items.head->next; node != base->items.head;) {
        struct ItemInfo copy;
        struct String text;
        int found;

        item_info_copy(&copy, LIST_VALUE(node, struct ItemInfo));
        string_copy(&text, &copy.name);
        if (by_mbsstr) {
            found = _mbsstr((const unsigned char *)string_cstr(&text),
                            (const unsigned char *)string_cstr(needle)) != NULL;
        } else {
            found = store_text_find(string_cstr(&text), text.size, string_cstr(needle), needle->size);
        }
        if (found)
            node = node->next;
        else
            node = list_erase(&base->items, node, item_info_destroy);
        string_free(&text);
        item_info_destruct(&copy);
    }
    return 1;
}

void storage_sort_chosen(struct UIItemBaseWnd *base)
{
    if (g_session1.itemSort[1])
        storage_sort(base, 0);
    else if (g_session1.itemSort[2])
        storage_sort(base, 1);
}

void storage_list_create(struct UIItemBaseWnd *base, struct UIWindow **title, struct UIResizer **resizer)
{
    struct UIWindow *wnd = &base->frame.base;
    char caption[0x40];
    struct UIResizer *grip;

    shop_add_scroll_bar(base, wnd->height - 0x26);
    sprintf(caption, "서브창고");
    *title = mem_alloc(sizeof(struct UIStaticText));
    if (*title)
        *title = ui_static_text_construct(*title);
    ui_window_create(*title, wnd->width - 0x20, 0xf);
    ui_static_text_set_text_ellipsis(*title, caption);
    ui_window_move(*title, 0x10, 2);
    ((struct UIStaticText *)*title)->white_shadow = 1;
    ui_window_add_child(wnd, *title);
    grip = mem_alloc(sizeof *grip);
    if (grip)
        grip = ui_resizer_construct(grip);
    *resizer = grip;
    ui_window_create(&grip->base, 0x10, 0x10);
    ui_resizer_set_edge(*resizer, UI_EDGE_BOTTOM_RIGHT);
    ui_resizer_set_bitmap(*resizer, UI_SKIN "btn_resize.bmp");
    ui_window_move_base(&(*resizer)->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours(*resizer, 0xffff00ff, 0xffff00ff);
    ui_window_add_child(wnd, &(*resizer)->base);
}

void storage_list_draw(struct UIItemBaseWnd *base, const struct String *count_text)
{
    struct UIWindow *wnd = &base->frame.base;
    int columns, rows, i;

    ui_window_draw_title_bar(wnd, 1, "", 0);
    ui_window_fill_rect(wnd, 0, ITEM_GRID_Y, wnd->width, wnd->height - 0x26, ui_skin_colour(2, 2));
    ui_window_draw_button_bar(wnd, 0, 0, 1);
    columns = (wnd->width - 0x28) / ITEM_GRID_CELL;
    rows = (wnd->height - 0x26) / ITEM_GRID_CELL;
    for (i = 0; i < rows; i++)
        ui_window_draw_item_win_mid(wnd, 0xa, ITEM_GRID_Y + i * ITEM_GRID_CELL);
    ui_window_fill_rect(wnd, 0x48, ITEM_GRID_Y, columns * ITEM_GRID_CELL - 0x20, rows * ITEM_GRID_CELL,
                      ui_skin_colour(2, 2));
    storage_draw_items(base, 0xa, 0);
    ui_window_text_out(wnd, 0x10, wnd->height - 0x14, string_cstr(count_text), 0, 0, 0xc, 0, 0, 0);
}

void storage_list_resize(struct UIItemBaseWnd *base, struct UIResizer *resizer, struct UIWindow *close,
                         int width, int height)
{
    struct UIWindow *wnd = &base->frame.base;

    if (width > 0x140)
        width = 0x140;
    if (height > 0x150)
        height = 0x150;
    if (width < 0x8c)
        width = 0x8c;
    if (height < 0x91)
        height = 0x91;
    width = (width - 0x8c) / ITEM_GRID_CELL * ITEM_GRID_CELL + 0x8c;
    height = (height - 0xa0) / ITEM_GRID_CELL * ITEM_GRID_CELL + 0x91;
    if (width != wnd->width || height != wnd->height) {
        base->scroll = 0;
        ui_window_resize_base(wnd, width, height);
        ui_window_move_base(&resizer->base, wnd->width - 0xf, wnd->height - 0xf);
        ui_window_move_base(&base->scroll_bar->base, wnd->width - 0xf, 0xe);
        ui_scroll_bar_resize(&base->scroll_bar->base, base->scroll_bar->base.width, wnd->height - 0x26);
        if (close)
            ui_window_move(close, wnd->width - 0x10, 3);
        base->rows = (height - 0x26) / ITEM_GRID_CELL;
        ui_item_base_wnd_update_scroll(base);
    }
    ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
}

int storage_list_info_usable(const struct WindowInfo *info)
{
    if (info->width < 0x8c || info->height < 0x91 || info->x < 0 ||
        info->x > g_renderer->width - info->width || info->y < 0 ||
        info->y > g_renderer->height - info->height)
        return 0;
    return 1;
}

void storage_apply_info(struct UIWindow *wnd, const struct WindowInfo *info)
{
    ui_window_move(wnd, info->x, info->y);
    ui_window_send_msg(wnd, NULL, UI_MSG_RESIZE, UI_EDGE_BOTTOM, info->width, info->height, 0);
}
