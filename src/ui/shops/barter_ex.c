#include "world/tooltip.h"
#include "game/context.h"
#include "ui/window_access.h"
#include "ui/shops/barter_ex_internal.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"

static void destroy_goods(void *value)
{
    barter_goods_ex_destruct(value);
}

void exb_construct(struct ExpandedBarterWnd *self, const struct UIWindowOps *ops)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = ops;
    list_init(&self->goods);
    self->resizer = NULL;
    self->scroll_bar = NULL;
    self->scroll = 0;
    self->rows = 0;
    self->hover = 0;
    self->scrollable = 0;
}

void exb_destruct(struct ExpandedBarterWnd *self, const struct UIWindowOps *ops)
{
    self->frame.base.ops = ops;
    list_free(&self->goods, destroy_goods);
    ui_frame_wnd_destruct(&self->frame);
}

static int exb_in_lines(const struct ExpandedBarterWnd *self, int x, int y)
{
    return x >= ITEM_GRID_X && x < self->frame.base.width - 0x14 && y >= ITEM_GRID_Y
        && y < self->rows * EXB_ROW + ITEM_GRID_Y;
}

static int exb_index_in_row(const struct ExpandedBarterWnd *self, int row, const struct BarterGoodsEx **found)
{
    const struct ListNode *node;
    int i = 0, line = 0;

    for (node = self->goods.head->next; node != self->goods.head; node = node->next, i++) {
        if (i < self->scroll)
            continue;
        if (line == row) {
            if (found)
                *found = LIST_VALUE(node, const struct BarterGoodsEx);
            return i;
        }
        line++;
    }
    return -1;
}

struct BarterExFound *exb_goods_at(const struct ExpandedBarterWnd *self, struct BarterExFound *out, int x,
                                          int y)
{
    const struct BarterGoodsEx *goods = NULL;

    if (exb_in_lines(self, x, y))
        exb_index_in_row(self, (y - ITEM_GRID_Y) / EXB_ROW, &goods);
    out->found = goods != NULL;
    if (goods)
        barter_goods_ex_copy(&out->goods, goods);
    else
        barter_goods_ex_construct(&out->goods);
    return out;
}

static struct BarterExFound *exb_goods_or_pay_at(const struct ExpandedBarterWnd *self, struct BarterExFound *out,
                                                 int x, int y)
{
    const struct BarterGoodsEx *goods = NULL;

    if (exb_in_lines(self, x, y))
        exb_index_in_row(self, (y - ITEM_GRID_Y) / EXB_ROW, &goods);
    if (goods) {
        if ((y - ITEM_GRID_Y) / ITEM_GRID_CELL & 1) {
            const struct BarterExPay *pay;
            int left = EXB_PAY_X;

            for (pay = (const struct BarterExPay *)goods->pay.first; pay != (const struct BarterExPay *)goods->pay.last; pay++, left += EXB_PAY_STEP) {
                if (x > left && x < left + EXB_PAY_STEP) {
                    struct BarterGoodsEx item;
                    char id[0x10];

                    barter_goods_ex_construct(&item);
                    item.type = pay->type;
                    item.refine = pay->refine;
                    memset(id, 0, sizeof id);
                    snprintf(id, sizeof id, "%d", pay->id);
                    string_assign_cstr(&item.id_text, id);
                    out->found = 1;
                    barter_goods_ex_copy(&out->goods, &item);
                    barter_goods_ex_destruct(&item);
                    return out;
                }
            }
        }
        if (x > ITEM_GRID_X && x < EXB_PAY_X) {
            out->found = 1;
            barter_goods_ex_copy(&out->goods, goods);
            return out;
        }
    }
    out->found = 0;
    barter_goods_ex_construct(&out->goods);
    return out;
}

void exb_append_refine(struct String *text, int refine)
{
    char number[0xc];

    snprintf(number, sizeof number, "%d", refine);
    string_append_cstr(text, "+");
    string_append_cstr(text, number);
    string_append_cstr(text, " ");
}

void exb_add_resizer_and_bar(struct ExpandedBarterWnd *self, int edge)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIResizer *resizer = mem_alloc(sizeof *resizer);
    struct UIScrollBar *bar;

    if (resizer)
        resizer = ui_resizer_construct(resizer);
    self->resizer = resizer;
    ui_window_create(&resizer->base, 0x10, 0x10);
    ui_resizer_set_edge(self->resizer, edge);
    ui_resizer_set_bitmap(self->resizer, UI_SKIN "btn_resize.bmp");
    ui_window_move_base(&self->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_resizer_set_colours(self->resizer, 0xffff00ff, 0xffff00ff);
    ui_window_add_child(wnd, &self->resizer->base);

    bar = mem_alloc(sizeof *bar);
    if (bar)
        bar = ui_scroll_bar_construct(bar);
    self->scroll_bar = bar;
    ui_scroll_bar_create(bar, wnd->width - 0xf, 0xe, 1, wnd->height - EXB_BOTTOM);
    ui_window_add_child(wnd, &self->scroll_bar->base);
}

void exb_finish_create(struct ExpandedBarterWnd *self, int height)
{
    self->rows = (height - EXB_BOTTOM) / EXB_ROW;
    ui_window_send_msg(&self->frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
}

static void exb_draw_line(struct ExpandedBarterWnd *self, const struct BarterGoodsEx *goods, int y, int hovered,
                          int market)
{
    struct UIWindow *wnd = &self->frame.base;
    const struct BarterExPay *pay;
    struct ItemInfo info;
    char price[0x17];
    int x;

    ui_window_draw_item_win_mid(wnd, ITEM_GRID_X, y - 4);
    for (pay = (const struct BarterExPay *)goods->pay.first, x = EXB_PAY_X; pay != (const struct BarterExPay *)goods->pay.last; pay++, x += EXB_PAY_STEP)
        ui_window_draw_item_win_mid(wnd, x, y + 0x1c);
    if (hovered)
        ui_window_fill_rect(wnd, EXB_PAY_X, y - 2, wnd->width - 0x5a, 0x3c, ui_skin_colour(6, 2));

    barter_goods_ex_to_item_info(goods, &info);
    if (!market)
        ui_window_draw_item_icon_count(wnd, 0x2c, y, &info, 0, 0);
    else if (goods->no_stock == 1)
        ui_window_draw_item_icon(wnd, 0x2c, y, &info, 0);
    else if (goods->count == 0)
        ui_window_draw_item_icon_count_grey(wnd, 0, 0x2c, y, &info, 1, 0);
    else
        ui_window_draw_item_icon_count(wnd, 0x2c, y, &info, 1, 0);
    item_info_destruct(&info);
    barter_goods_ex_to_item_info(goods, &info);
    ui_window_draw_item_name_short(wnd, 0x4c, y + 8, 100, &info, 12);
    item_info_destruct(&info);

    for (pay = (const struct BarterExPay *)goods->pay.first, x = 0x4c; pay != (const struct BarterExPay *)goods->pay.last; pay++, x += EXB_PAY_STEP) {
        struct ItemInfo item;
        char id[0x24];

        item_info_construct(&item);
        item.count = market ? pay->count : pay->count * goods->count;
        snprintf(id, sizeof id, "%d", pay->id);
        string_assign_cstr(&item.id_text, id);
        item.refine = pay->refine;
        if (market && goods->count == 0)
            ui_window_draw_item_icon_count_grey(wnd, 0, x, y + 0x1c, &item, 1, 0);
        else
            ui_window_draw_item_icon_count(wnd, x, y + 0x1c, &item, 1, 0);
        if (market ? item.refine != 0 : item.refine > 0) {
            COLORREF colour = EXB_REFINE_COLOUR;
            char text[0x10];

            memset(text, 0, sizeof text);
            snprintf(text, sizeof text, "+%d", item.refine);
            ui_window_text_out_outlined_ptr(wnd, x + 8, y + 0x1c, text, strlen(text), &colour,
                                        EXB_REFINE_OUTLINE, 0, 12, 1);
        }
        item_info_destruct(&item);
    }

    memset(price, 0, sizeof price);
    snprintf(price, sizeof price, "%d z", market ? goods->price : goods->price * goods->count);
    ui_window_text_out_right(wnd, wnd->width - 0x14, y + 0x27, price, 0, 0, 12, 0, 0);
}

void exb_draw(struct ExpandedBarterWnd *self, int market)
{
    struct UIWindow *wnd = &self->frame.base;
    const struct ListNode *node;
    int i = 0, line = 1, y = ITEM_GRID_Y + 4;

    ui_window_draw_title_bar(wnd, 0, msg_str(market ? MSG_BARTER_TITLE : MSG_BARTER_PURCHASE_TITLE), 0);
    shop_draw_side_strips(wnd, wnd->height - EXB_BOTTOM);
    if (!market)
        ui_window_draw_button_bar(wnd, 2, 0, 1);
    ui_window_fill_rect(wnd, 0x14, ITEM_GRID_Y, wnd->width - 0x28, wnd->height - EXB_BOTTOM,
                      ui_skin_colour(2, 2));
    ui_window_draw_button_bar(wnd, 2, 0, 1);

    for (node = self->goods.head->next; node != self->goods.head; node = node->next, i++) {
        if (i < self->scroll)
            continue;
        exb_draw_line(self, LIST_VALUE(node, const struct BarterGoodsEx), y, self->hover == i, market);
        if (line == self->rows) {
            i++;
            break;
        }
        line++;
        y += EXB_ROW;
    }
    for (; i < self->rows; i++) {
        int cells_y = i * EXB_ROW + 0x31;

        ui_window_draw_item_win_mid(wnd, ITEM_GRID_X, cells_y - 0x20);
        ui_window_draw_item_win_mid(wnd, EXB_PAY_X, cells_y);
        ui_window_draw_item_win_mid(wnd, EXB_PAY_X + EXB_PAY_STEP, cells_y);
    }
}

void exb_press(struct ExpandedBarterWnd *self, int x, int y)
{
    int old = self->hover, hover = -1;

    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
    if (exb_in_lines(self, x, y))
        hover = exb_index_in_row(self, (y - ITEM_GRID_Y) / EXB_ROW, NULL);
    self->hover = hover;
    if (old != hover)
        ui_window_invalidate_base(&self->frame.base);
}

void exb_show_pay_tooltip(struct ExpandedBarterWnd *self, int x, int y, int times_count)
{
    struct BarterExFound at;

    exb_goods_at(self, &at, x, y);
    if (at.found && x >= 0x29 && x <= 0x47) {
        const struct BarterExPay *pay;
        unsigned j = 1, n = (unsigned)((const struct BarterExPay *)at.goods.pay.last - (const struct BarterExPay *)at.goods.pay.first);
        int tip_x = x, tip_y = (y - ITEM_GRID_Y) / EXB_ROW * EXB_ROW + 1;
        struct String text;
        struct ItemInfo info;
        struct Mode *mode;

        ui_window_to_screen(&self->frame.base, &tip_x, &tip_y);
        string_init(&text);
        for (pay = (const struct BarterExPay *)at.goods.pay.first; pay != (const struct BarterExPay *)at.goods.pay.last; pay++, j++) {
            const char *name = item_table_get(pay->id)->name;
            struct String line;

            if (pay->refine > 0)
                exb_append_refine(&text, pay->refine);
            string_init(&line);
            string_format(&line, msg_str(MSG_BARTER_PAY), name,
                          times_count ? pay->count * at.goods.count : pay->count);
            string_append(&text, string_cstr(&line), line.size);
            string_free(&line);
            if (j != n)
                string_append_cstr(&text, "\n");
        }
        mode = game_current_base_mode();
        barter_goods_ex_to_item_info(&at.goods, &info);
        if (mode)
            mode_show_tooltip(mode, string_cstr(&text), tip_x, tip_y,
                              session_get_item_dis_color(&info), 0);
        item_info_destruct(&info);
        string_free(&text);
    }
    barter_goods_ex_destruct(&at.goods);
}

void exb_show_item_at(const struct ExpandedBarterWnd *self, int x, int y)
{
    struct BarterExFound at;

    exb_goods_or_pay_at(self, &at, x, y);
    if (at.found == 1) {
        struct UIWindow *wnd = ui_window_open(ITEM_WND_ID);

        if (wnd) {
            struct ItemInfo info;

            barter_goods_ex_to_item_info(&at.goods, &info);
            ui_window_send_msg(wnd, NULL, UI_MSG_SHOW_ITEM, (int)&info, 0, 0, 0);
            item_info_destruct(&info);
        }
    }
    barter_goods_ex_destruct(&at.goods);
}

void exb_wheel(struct ExpandedBarterWnd *self, int delta)
{
    if (self->scrollable == 1) {
        int step = self->rows - 1 < 3 ? self->rows - 1 : 3;

        ui_window_send_msg(&self->frame.base, &self->scroll_bar->base, UI_MSG_SCROLL_V, delta > 0 ? -step : step, 0, 0, 0);
    }
}

static void exb_update_scroll(struct ExpandedBarterWnd *self)
{
    int over;

    if (!self->scroll_bar)
        return;
    over = (int)self->goods.size - self->rows;
    if (over < 0)
        over = 0;
    if (over) {
        self->scrollable = 1;
        ui_scroll_bar_set_range(self->scroll_bar, over, self->rows, (int)self->goods.size);
        if (self->scroll < 0)
            self->scroll = 0;
        if (self->scroll > (int)self->goods.size - self->rows)
            self->scroll = (int)self->goods.size - self->rows;
        ui_scroll_bar_set_pos(self->scroll_bar, self->scroll);
    } else {
        self->scrollable = 0;
        self->scroll = 0;
        ui_scroll_bar_set_range(self->scroll_bar, 0, self->rows, (int)self->goods.size);
    }
    ui_window_invalidate_base(&self->scroll_bar->base);
}

int exb_scroll(struct ExpandedBarterWnd *self, int msg, int val1)
{
    int old = self->scroll;

    if (msg == UI_MSG_PAGE_UP)
        self->scroll = old - self->rows + 1;
    else if (msg == UI_MSG_PAGE_DOWN)
        self->scroll = old + self->rows - 1;
    else
        self->scroll = old + val1;
    if (old == self->scroll)
        return 0;
    exb_update_scroll(self);
    ui_window_invalidate_base(&self->frame.base);
    return self->scroll - old;
}

void exb_refresh(struct ExpandedBarterWnd *self, const struct Vec *goods)
{
    const struct BarterGoodsEx *p;

    list_clear(&self->goods, destroy_goods);
    for (p = goods->first; p != goods->last; p++) {
        struct BarterGoodsEx copy;

        barter_goods_ex_copy(&copy, p);
        list_push_back(&self->goods, &copy, sizeof copy);
    }
    ui_window_invalidate_base(&self->frame.base);
    exb_update_scroll(self);
}

void exb_resize(struct ExpandedBarterWnd *self, int width, int height)
{
    struct UIWindow *wnd = &self->frame.base;

    ui_window_resize_base(wnd, width, height);
    self->rows = (height - EXB_BOTTOM) / EXB_ROW;
    ui_window_move_base(&self->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
    ui_window_move_base(&self->scroll_bar->base, wnd->width - 0xf, 0xe);
    ui_scroll_bar_resize(&self->scroll_bar->base, self->scroll_bar->base.width, wnd->height - EXB_BOTTOM);
    exb_update_scroll(self);
}

int exb_snap_height(int height)
{
    if (height < 0x16e)
        height = 0x16e;
    else if ((float)height > 549.0f)
        height = 0x225;
    return height / EXB_ROW * EXB_ROW;
}
