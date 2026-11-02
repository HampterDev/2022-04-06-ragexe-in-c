#include "contents/client_lua_calls.h"
#include "session/state.h"
#include "core/object_type_tables.h"
#include "core/memory.h"
#include "contents/connection_api.h"
#include "ui/items/styling_shop.h"
#include <stdlib.h>
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "session/session_names.h"

struct UIShowInfoWnd;
void ui_show_info_wnd_destroy(struct UIWindow *window);
void ui_show_info_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_show_info_wnd_on_draw(struct UIWindow *self);
intptr_t ui_show_info_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_show_info_wnd_ops = {
    .destroy = ui_show_info_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_window_drag_and_drop_base,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_show_info_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_show_info_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_window_on_l_btn_down_base,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_show_info_wnd_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_window_render_base,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

#define STYLE_CHECK_ABLE STYLE_SKIN "check_able.bmp"
#define STYLE_NOT_CHECK STYLE_SKIN "Notcheck.bmp"
#define STYLE_ZENY_COLOUR 0x5a5a84
#define STYLE_GREY 0xbfbfbf

struct StyleKindNode {
    struct StyleKindNode *next, *prev;
    int kind;
};

struct UIShowInfoWnd *ui_show_info_wnd_construct(struct UIShowInfoWnd *self)
{
    struct StyleKindNode *head;

    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_show_info_wnd_ops;
    self->display = NULL;
    self->show_hint = 0;
    self->draw_icons = 0;
    self->draw_lines = 0;
    self->row_top = 5;
    self->font_height = 0xc;
    memset(self->added, 0, sizeof self->added);
    self->kinds.head = NULL;
    self->kinds.size = 0;
    head = mem_alloc(sizeof *head);
    head->next = head;
    head->prev = head;
    self->kinds.head = (struct ListNode *)head;
    self->added_body_style = 0;
    self->icon_x = 0;
    self->icon_y = 0;
    self->text_x = 0;
    self->text_y = 0;
    self->row_bitmap = NULL;
    return self;
}

void ui_show_info_wnd_destroy(struct UIWindow *window)
{
    struct UIShowInfoWnd *self = (struct UIShowInfoWnd *)window;
    self->base.ops = &ui_show_info_wnd_ops;
    std_list_styling_destroy(&self->kinds);
    ui_window_destruct(&self->base);
    mem_free(self);
}

static void show_info_clear_kinds(struct UIShowInfoWnd *self)
{
    struct StyleKindNode *head = (struct StyleKindNode *)self->kinds.head, *node = head->next, *next;

    head->next = head;
    ((struct StyleKindNode *)self->kinds.head)->prev = (struct StyleKindNode *)self->kinds.head;
    self->kinds.size = 0;
    for (; node != (struct StyleKindNode *)self->kinds.head; node = next) {
        next = node->next;
        mem_free(node);
    }
}

static void show_info_clear_flags(struct UIShowInfoWnd *self)
{
    memset(self->added, 0, sizeof self->added);
    self->added_body_style = 0;
}

static void show_info_payment(struct UIShowInfoWnd *self, const int *kind, int *price, int *item_a, int *item_b)
{
    struct String name;

    if (*kind < STYLE_HEAD_PALETTE || *kind > STYLE_BODY_STYLE)
        return;
    style_kind_name(&name, *kind);
    client_lua_call(g_session1.luaState, "StylingShop_GetPayment_by_goods", "sd>ddd", string_cstr(&name),
            style_kind_value(self->display, *kind), price, item_a, item_b);
    string_free(&name);
}

BYTE show_info_buy(struct UIShowInfoWnd *self)
{
    struct PACKET_CZ_REQ_BUY_STYLINGSHOP2 packet;
    struct StyleKindNode *node;
    struct UICheckBox **box = self->boxes;
    BYTE any = 0;
    void *conn;
    int len;

    memset(&packet, 0, sizeof packet);
    packet.packet_type = STYLE_PACKET_BUY;
    for (node = ((struct StyleKindNode *)self->kinds.head)->next; node != (struct StyleKindNode *)self->kinds.head;
         node = node->next, box++) {
        struct String name;
        int kind = node->kind, seq;

        if (kind < STYLE_HEAD_PALETTE || kind > STYLE_BODY_STYLE || (*box)->base.state == 0)
            continue;
        any = 1;
        seq = 0;
        style_kind_name(&name, kind);
        style_lua_seq(string_cstr(&name), style_kind_value(self->display, kind), &seq);
        string_free(&name);
        if (kind == STYLE_HEAD_PALETTE && self->display->head_palette == 0)
            seq = -1;
        packet.look[kind - 1] = (short)seq;
    }
    if (any != 1)
        return 0;
    conn = rag_connection_instance();
    len = rag_connection_get_packet_length(conn, packet.packet_type);
    rag_connection_send_packet(rag_connection_instance(), len, &packet);
    return 1;
}

static BYTE show_info_check_zeny(struct UIShowInfoWnd *self, int row, int price)
{
    if (g_session1.zeny >= price) {
        ui_check_box_set_tool_tip(&self->boxes[row]->base, "");
        return 1;
    }
    ui_check_box_set_tool_tip(&self->boxes[row]->base,
                          msg_str((long long)g_session1.zeny + g_session1.bankZeny >= (long long)price ? 0xb04 : 0xadc));
    return 0;
}

static BYTE show_info_has_item(int item_id)
{
    struct ItemInfo info;
    int found;

    session_get_inventory_item_by_id(&g_session1, &info, item_id, 0);
    found = atoi(string_cstr(&info.id_text));
    item_info_destruct(&info);
    return found == item_id;
}

static BYTE show_info_can_pay(struct UIShowInfoWnd *self, const int *row, const int *price, const int *item_a,
                              const int *item_b)
{
    if (*price)
        return show_info_check_zeny(self, *row, *price);
    if (*item_a)
        return show_info_has_item(*item_a);
    if (*item_b) {
        if (!show_info_has_item(*item_b))
            return 0;
        ui_check_box_set_bitmaps_fit(self->boxes[*row], STYLE_SKIN "check_disable.bmp", STYLE_NOT_CHECK);
    }
    return 1;
}

static void show_info_validate(struct UIShowInfoWnd *self, BYTE *ok)
{
    struct UIDisplayCharacter *d;
    unsigned i;

    if (style_is_doram() == 1)
        client_lua_call(g_session1.luaState, "StylingShop_GetIsColoring_by_goods", "d>b", self->display->head_style,
                ok);
    d = self->display;
    if (*ok && !(self->added[0] == 1 && d->head_palette == g_session1.hairColor)
        && !(self->added[1] == 1 && d->head_style == g_session1.hairStyle)
        && !(self->added_body_style == 1 && d->body_style == g_session1.body)
        && !(self->added[2] == 1 && d->body_palette == g_session1.clothesColor)
        && !(self->added[3] == 1 && (d->accessory_top == g_session1.headTop || d->accessory_top == 0))
        && !(self->added[4] == 1 && (d->accessory_mid == g_session1.headMid || d->accessory_mid == 0))) {
        if (self->added[5] != 1)
            return;
        if (d->accessory_bottom != g_session1.headBottom && d->accessory_mid != 0)
            return;
    }
    for (i = 0; i < self->kinds.size; i++) {
        ui_check_box_set_tool_tip(&self->boxes[i]->base, "");
        ui_check_box_set_check(&self->boxes[i]->base, 0);
    }
    show_info_clear_kinds(self);
    show_info_clear_flags(self);
}

static void show_info_add_row(struct UIShowInfoWnd *self, int kind)
{
    ui_check_box_set_check(&self->boxes[self->kinds.size]->base, 1);
    std_list_styling_push_back(&self->kinds, kind);
}

static void show_info_collect(struct UIShowInfoWnd *self)
{
    BYTE ok = 1;
    int seq;

    show_info_validate(self, &ok);
    if (!self->added[0] && self->display->head_palette != g_session1.hairColor && ok == 1) {
        show_info_add_row(self, STYLE_HEAD_PALETTE);
        self->added[0] = 1;
    }
    if (!self->added[1] && self->display->head_style != g_session1.hairStyle) {
        show_info_add_row(self, STYLE_HEAD_STYLE);
        self->added[1] = 1;
    }
    if (!self->added[2] && self->display->body_palette != g_session1.clothesColor) {
        show_info_add_row(self, STYLE_BODY_PALETTE);
        self->added[2] = 1;
    }
    seq = 0;
    style_lua_seq("TopAccessory", self->display->goods_top, &seq);
    if (!self->added[3] && self->display->accessory_top != g_session1.headTop && self->display->accessory_top != 0
        && seq != 0) {
        show_info_add_row(self, STYLE_TOP_ACCESSORY);
        self->added[3] = 1;
    }
    style_lua_seq("MidAccessory", self->display->goods_mid, &seq);
    if (!self->added[4] && self->display->accessory_mid != g_session1.headMid && self->display->accessory_mid != 0
        && seq != 0) {
        show_info_add_row(self, STYLE_MID_ACCESSORY);
        self->added[4] = 1;
    }
    style_lua_seq("BottomAccessory", self->display->goods_bottom, &seq);
    if (!self->added[5] && self->display->accessory_bottom != g_session1.headBottom
        && self->display->accessory_bottom != 0 && seq != 0) {
        show_info_add_row(self, STYLE_BOTTOM_ACCESSORY);
        self->added[5] = 1;
    }
    if (!self->added_body_style && self->display->body_style != g_session1.body) {
        show_info_add_row(self, STYLE_BODY_STYLE);
        self->added_body_style = 1;
    }
}

static void show_info_draw_icon(struct UIShowInfoWnd *self, const int *kind, const int *price, const int *item,
                                BYTE ok, const int *row)
{
    struct UIWindow *wnd = &self->base;
    struct BitmapRes *bitmap = NULL;
    char path[0x80];
    char id_text[0x20];
    int item_id, y;

    if (!self->draw_icons)
        return;
    item_id = *item;
    if (*price) {
        memset(path, 0, sizeof path);
        if (*kind >= STYLE_TOP_ACCESSORY && *kind <= STYLE_BOTTOM_ACCESSORY) {
            memset(id_text, 0, sizeof id_text);
            sprintf(id_text, "%d", style_kind_value(self->display, *kind));
            session_get_item_bitmap_file_name(id_text, path, 1);
        } else {
            strcpy_s(path, sizeof path, STYLE_SKIN "icon_zeny.bmp");
        }
        bitmap = ui_skin_bitmap(path);
    } else if (item_id) {
        if (*kind >= STYLE_TOP_ACCESSORY && *kind <= STYLE_BOTTOM_ACCESSORY)
            item_id = style_kind_value(self->display, *kind);
        memset(path, 0, sizeof path);
        memset(id_text, 0, sizeof id_text);
        sprintf(id_text, "%d", item_id);
        session_get_item_bitmap_file_name(id_text, path, 1);
        bitmap = ui_skin_bitmap(path);
    }
    y = wnd->height / 7 * *row + self->icon_y;
    if (!ok)
        ui_window_draw_bitmap_tinted(wnd, 0, self->icon_x, y, bitmap, 1, 0);
    else if (bitmap)
        ui_window_draw_bitmap(wnd, self->icon_x, y, bitmap, 0);
}

static void show_info_update_buy(struct UIShowInfoWnd *self)
{
    struct UIStylingShopWnd *shop = (struct UIStylingShopWnd *)ui_get_window(WND_STYLING_SHOP);
    struct StyleTab *my_style;
    struct TreeNode *node;

    if (!shop)
        return;
    my_style = style_my_style_tab(shop);
    if (!my_style || my_style->tab->base.visible == 0) {
        styling_shop_set_buyable(shop, self->all_ok);
    } else {
        for (node = tree_first(&shop->tabs); node != shop->tabs.head; node = tree_next(node)) {
            struct StyleTab *entry = TREE_VALUE(node, struct StyleTab);

            if (entry->button->id == STYLE_TAB_MY_STYLE)
                styling_shop_set_buyable(shop, my_style_all_ok((struct UIMyStyleWnd *)entry->tab));
        }
    }
    ui_window_invalidate(&shop->frame.base);
}

static void show_info_text(struct UIShowInfoWnd *self, int x, int y, const char *text, COLORREF colour)
{
    ui_window_text_out(&self->base, x, y, text, 0, 0, self->font_height, colour, 0, 0);
}

static void show_info_strike(struct UIShowInfoWnd *self, int x, int y, const char *text, COLORREF colour)
{
    int width = ui_window_text_width(&self->base, text, self->font_height, 0, 0xc, 0, 0);

    ui_window_fill_rect(&self->base, x - 2, y + 5, width + 2, 1, colour);
}

static void show_info_draw_rows(struct UIShowInfoWnd *self)
{
    struct UIWindow *wnd = &self->base;
    struct StyleKindNode *node;
    int row_x[SHOW_INFO_ROWS], row_y[SHOW_INFO_ROWS];
    int total = 0, i;
    unsigned k;

    for (i = 0; i < SHOW_INFO_ROWS; i++) {
        row_x[i] = self->text_x;
        row_y[i] = self->text_y + i * (wnd->height / 7);
    }
    self->all_ok = 1;
    i = 0;
    for (node = ((struct StyleKindNode *)self->kinds.head)->next; node != (struct StyleKindNode *)self->kinds.head;
         node = node->next, i++) {
        struct BitmapRes *bg = ui_skin_bitmap(self->row_bitmap);
        int x = row_x[i], y = row_y[i];
        COLORREF colour = STYLE_GREY;
        int price = 0, item_a = 0, item_b = 0, zero = 0, due;
        BYTE ok;
        char text[0x10];

        if (bg)
            ui_window_draw_bitmap(wnd, self->boxes[i]->base.x, self->boxes[i]->base.y, bg, 0);
        show_info_payment(self, &node->kind, &price, &item_a, &item_b);
        if (price) {
            due = price + total;
            ok = show_info_can_pay(self, &i, &due, &item_a, &zero);
        } else {
            due = 0;
            ok = show_info_can_pay(self, &i, &price, &item_a, &due);
        }
        if (ok == 1) {
            colour = STYLE_ZENY_COLOUR;
            if (self->boxes[i]->base.state)
                total += price;
            ui_check_box_set_check(&self->boxes[i]->base, 1);
        } else {
            self->all_ok = 0;
            ui_check_box_set_check(&self->boxes[i]->base, 0);
        }
        if (price) {
            show_info_draw_icon(self, &node->kind, &price, &item_a, ok, &i);
            memset(text, 0, sizeof text);
            sprintf(text, "%d\tZeny", price);
            if (!ok)
                show_info_strike(self, x, y, text, colour);
            show_info_text(self, x, y, text, colour);
        } else if (item_a) {
            show_info_draw_icon(self, &node->kind, &price, &item_a, ok, &i);
            if (ok) {
                show_info_text(self, x, y, item_table_get(item_a)->name, colour);
            } else {
                ui_check_box_set_tool_tip(&self->boxes[i]->base, msg_str(0xafe));
                if (item_b) {
                    int zero_a = 0, zero_b = 0;
                    BYTE ok_b = show_info_can_pay(self, &i, &zero_a, &zero_b, &item_b);

                    if (ok_b == 1) {
                        show_info_draw_icon(self, &node->kind, &price, &item_a, 1, &i);
                        ui_check_box_set_tool_tip(&self->boxes[i]->base, msg_str(0xaff));
                        show_info_text(self, x, y, item_table_get(item_b)->name, colour);
                    } else {
                        const char *name = item_table_get(item_a)->name;

                        if (!ok_b)
                            show_info_strike(self, x, y, name, colour);
                        show_info_text(self, x, y, name, colour);
                    }
                }
            }
        } else {
            char buf[0x104];

            ui_window_draw_bitmap(wnd, self->icon_x, wnd->height / 7 * i + self->icon_y,
                                ui_skin_bitmap(STYLE_SKIN "icon_color_off.bmp"), 0);
            memset(buf, 0, sizeof buf);
            _snprintf_s(buf, sizeof buf, (size_t)-1, msg_str(0xf48));
            show_info_text(self, x, y, buf, STYLE_ZENY_COLOUR);
        }
    }
    if (self->kinds.size < SHOW_INFO_ROWS)
        for (k = self->kinds.size; k < SHOW_INFO_ROWS; k++)
            ui_check_box_set_check(&self->boxes[k]->base, 0);
    if (self->kinds.size == 0) {
        self->all_ok = 0;
        if (self->show_hint == 1) {
            struct TextArray lines = { 0 };
            unsigned n;

            ui_window_break_lines(wnd, msg_str(0xb13), &lines, wnd->width - 0x14);
            for (n = 0; n < lines.size
                            && n < SHOW_INFO_ROWS; n++)
                ui_window_text_out(wnd, 0x14, row_y[n], text_array_at(&lines, n).data,
                                 0, 0, 0xb, 0, 0, 0);
            text_array_clear(&lines);
        }
    }
    show_info_update_buy(self);
}

void ui_show_info_wnd_on_draw(struct UIWindow *window)
{
    struct UIShowInfoWnd *self = (struct UIShowInfoWnd *)window;
    struct UIWindow *wnd = &self->base;
    int i;

    ui_window_clear_canvas(wnd, UI_TRANSPARENT);
    if (!self->display)
        return;
    if (self->draw_lines == 1) {
        for (i = 0; i < SHOW_INFO_ROWS; i++) {
            struct BitmapRes *line = ui_skin_bitmap(STYLE_SKIN "line_dotted_1px_light.bmp");

            if (line) {
                struct UIWindow *box = &self->boxes[i]->base;

                ui_window_draw_bitmap(wnd, box->x, box->y + box->height, line, 0);
            }
        }
    }
    show_info_collect(self);
    show_info_draw_rows(self);
}

static void show_info_reset(struct UIShowInfoWnd *self)
{
    int i;

    for (i = 0; i < SHOW_INFO_ROWS; i++) {
        ui_check_box_set_check(&self->boxes[i]->base, 0);
        ui_check_box_set_bitmaps_fit(self->boxes[i], STYLE_CHECK_ABLE, STYLE_NOT_CHECK);
    }
    show_info_clear_flags(self);
    if (self->kinds.size)
        show_info_clear_kinds(self);
}

void ui_show_info_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIShowInfoWnd *self = (struct UIShowInfoWnd *)window;
    struct UIWindow *wnd = &self->base;
    int i;

    (void)width;
    (void)height;
    for (i = 0; i < SHOW_INFO_ROWS; i++) {
        struct UICheckBox *box = mem_try_alloc(sizeof *box);

        if (box)
            box = ui_check_box_construct(&box->base);
        self->boxes[i] = box;
        ui_window_create(&box->base, 0x11, 0xd);
        ui_check_box_set_bitmaps_fit(self->boxes[i], STYLE_CHECK_ABLE, STYLE_NOT_CHECK);
        ui_check_box_set_check(&self->boxes[i]->base, 0);
        ui_window_move_base(&self->boxes[i]->base, (int)(wnd->width * 0.1), wnd->height / 7 * i + self->row_top);
        self->boxes[i]->base.id = STYLE_ID_ROW;
        ui_window_add_child(wnd, &self->boxes[i]->base);
    }
    show_info_reset(self);
}

void ui_show_info_wnd_setup(struct UIShowInfoWnd *self, const int *tab)
{
    struct UIWindow *wnd = &self->base;
    unsigned i;

    switch (*tab) {
    case STYLE_TAB_HAIR:
    case STYLE_TAB_2ND_COSTUME:
        self->row_top = 6;
        self->font_height = 0xc;
        self->draw_icons = 1;
        self->draw_lines = 1;
        self->show_hint = 1;
        self->icon_x = 0x23;
        self->icon_y = 1;
        self->text_x = 0x3e;
        self->text_y = 8;
        self->row_bitmap = STYLE_SKIN "checkbox_off.bmp";
        for (i = self->kinds.size; i < SHOW_INFO_ROWS; i++) {
            ui_check_box_set_bitmaps_fit(self->boxes[i], STYLE_CHECK_ABLE, STYLE_NOT_CHECK);
            ui_window_move_base(&self->boxes[i]->base, 2, wnd->height / 7 * (int)i + self->row_top);
        }
        break;
    case STYLE_TAB_MY_STYLE:
    case STYLE_TAB_ACCESSORY:
        self->row_top = 1;
        self->font_height = 0xb;
        self->text_x = 0x18;
        self->text_y = 0;
        self->row_bitmap = STYLE_SKIN "checkbox_mini_off.bmp";
        for (i = self->kinds.size; i < SHOW_INFO_ROWS; i++) {
            ui_check_box_set_bitmaps_fit(self->boxes[i], STYLE_SKIN "checkbox_mini_on.bmp",
                                     STYLE_SKIN "checkbox_mini_non.bmp");
            ui_window_move_base(&self->boxes[i]->base, 7, wnd->height / 7 * (int)i + self->row_top);
        }
        break;
    }
}

intptr_t ui_show_info_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                     intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIShowInfoWnd *self = (struct UIShowInfoWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
    if (val1 == STYLE_ID_ROW)
        ui_window_invalidate_base(&self->base);
    return 0;
}
