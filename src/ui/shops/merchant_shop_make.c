#include "ui/controls/edit.h"
#include "ui/shops/merchant_shop_make.h"
#include "ui/window_access.h"
#include "render/rp_pool.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include <stdlib.h>
#include <string.h>
#include "ui/shops/merchant_shop_make_internal.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "ui/ui_items.h"
#include "ui/runtime.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/wndmgr/wndmgr.h"
#include <stdint.h>

struct DragInfo;
void ui_merchant_shop_make_wnd_destroy(struct UIWindow *window);
void ui_merchant_shop_make_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_merchant_shop_make_wnd_store_info(struct UIWindow *self);
void ui_merchant_shop_make_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_merchant_shop_make_wnd_on_process(struct UIWindow *self);
void ui_merchant_shop_make_wnd_on_draw(struct UIWindow *self);
void ui_merchant_shop_make_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_merchant_shop_make_wnd_on_r_btn_down(struct UIWindow *self, int x, int y);
void ui_merchant_shop_make_wnd_on_wheel(struct UIWindow *self, int delta);

const struct UIWindowOps ui_merchant_shop_make_wnd_ops = {
    .destroy = ui_merchant_shop_make_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_merchant_shop_make_wnd_drag_and_drop,
    .store_info = ui_merchant_shop_make_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_merchant_shop_make_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_merchant_shop_make_wnd_on_process,
    .on_draw = ui_merchant_shop_make_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_merchant_shop_make_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_frame_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_merchant_shop_make_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_merchant_shop_make_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_merchant_shop_make_wnd_send_msg,
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

static int64_t price_with_commission(int64_t price, double sign)
{
    double rate = (double)zeny_fee_percent(price) * (sign * 0.01);

    return price + (int64_t)((double)price * rate);
}

void msm_edit_amount_colour(struct UIWindow *wnd, struct UIWindow *edit, int64_t amount)
{
    struct AmountColours c;

    ui_window_amount_colours(wnd, &c, amount);
    ui_edit_ctrl_set_text_colour(edit, GetRValue(c.colour), GetGValue(c.colour), GetBValue(c.colour), c.double_colour);
}

void msm_edit_reset(struct UIWindow *edit, const char *text)
{
    ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, text);
    ui_window_mgr_set_focus_edit(&g_windowMgr, edit);
    ui_edit_ctrl_select_all(edit);
}

int msm_question_box(const char *text)
{
    return ui_window_mgr_message_box(&g_windowMgr, text, UI_MSGBOX_QUESTION, 1, 0, 0, "메시지", 0x118, 0x78,
                                  0);
}

void msm_item_message_box(int msg, const struct ItemInfo *info)
{
    struct String text;

    struct String name = {0};

    item_info_get_display_name((struct ItemInfo *)info, &name);
    string_init(&text);
    string_format(&text, msg_str(msg), string_cstr(&name));
    ui_message_box(string_cstr(&text));
    string_free(&text);
    string_free(&name);
}

struct UIMerchantShopMakeWnd *ui_merchant_shop_make_wnd_construct(struct UIMerchantShopMakeWnd *self, int buying)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_merchant_shop_make_wnd_ops;
    string_init(&self->title);
    self->buying = buying;
    list_init(&self->items);
    string_init(&self->last_text);
    self->scroll = NULL;
    self->first_row = 0;
    self->columns = 1;
    self->rows = 4;
    self->scrollable = 0;
    self->info = NULL;
    self->focused = -1;
    self->price_check = NULL;
    self->base_height = 0;
    self->shown_rows = 4;
    self->fit_rows = 4;
    self->buttons[0] = NULL;
    self->buttons[1] = NULL;
    self->last_size = 0;
    string_clear(&self->last_text);
    self->assistant = 0;
    self->waiting = 0;
    self->cell_x = -1;
    self->cell_y = -1;
    self->load_button = NULL;
    return self;
}

struct UIWindow *ui_merchant_shop_make_wnd_new(int arg)
{
    return (struct UIWindow *)ui_merchant_shop_make_wnd_construct(mem_alloc(sizeof(struct UIMerchantShopMakeWnd)), arg);
}

void ui_merchant_shop_make_wnd_destruct(struct UIMerchantShopMakeWnd *self)
{
    self->frame.base.ops = &ui_merchant_shop_make_wnd_ops;
    string_free(&self->last_text);
    list_free(&self->items, item_info_destroy);
    string_free(&self->title);
    ui_frame_wnd_destruct(&self->frame);
}

void ui_merchant_shop_make_wnd_destroy(struct UIWindow *window)
{
    struct UIMerchantShopMakeWnd *self = (struct UIMerchantShopMakeWnd *)window;
    ui_merchant_shop_make_wnd_destruct(self);
    mem_free(self);
}

static struct UIWindow *new_wheel_edit(void)
{
    struct UIWindow *edit = mem_alloc(sizeof(struct UIEditCtrl));

    if (!edit)
        return NULL;
    ui_edit_ctrl_construct(edit);
    edit->ops = &ui_edit_ctrl_wheel_ops.base;
    return edit;
}

static const char *const ok_button[3] = { "btn_ok", "btn_ok_a", "btn_ok_b" };

void ui_merchant_shop_make_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMerchantShopMakeWnd *self = (struct UIMerchantShopMakeWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    const char *title;
    int slots, i, y;

    (void)width;
    (void)height;
    if (self->buying) {
        slots = g_session1.buyingStoreSlots;
        title = msg_str(MSG_BUYING_TITLE);
    } else {
        slots = g_session1.vendingSlots;
        title = msg_str(MSG_VENDING_TITLE);
    }
    string_assign_cstr(&self->title, title);

    {
        struct UIWindow *edit = mem_alloc(sizeof(struct UIEditCtrl));

        if (edit)
            edit = ui_edit_ctrl_construct(edit);
        self->shop_name = edit;
        ui_window_create(edit, wnd->width - 0x42, 0x10);
        ui_window_move(self->shop_name, 0x29, 0x16);
        UI_EDIT_MAX_CHARS(self->shop_name) = 0x24;
        ui_edit_ctrl_set_back_colour(self->shop_name, EDIT_LIGHT, EDIT_LIGHT, EDIT_LIGHT);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->shop_name, "");
        ui_window_add_child(wnd, self->shop_name);
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->shop_name);
    }
    {
        struct UIScrollBar *bar = mem_alloc(sizeof(struct UIScrollBar));

        if (bar)
            bar = ui_scroll_bar_construct(bar);
        self->scroll = bar;
        ui_scroll_bar_create(bar, wnd->width - 0xf, 0x11, 1, wnd->height - 0x3a);
        ui_window_add_child(wnd, &self->scroll->base);
    }

    self->slots = slots;
    if (self->buying) {
        if (slots > BUYING_SLOTS_MAX)
            self->slots = slots = BUYING_SLOTS_MAX;
    } else if (slots > VENDING_SLOTS_MAX) {
        self->slots = slots = VENDING_SLOTS_MAX;
    }
    self->columns = 1;
    self->rows = 4;
    for (i = 0, y = ROW_FIRST_Y; i < self->slots; i++, y += ROW_HEIGHT) {
        struct UIWindow *edit;

        if (self->buying) {
            edit = new_wheel_edit();
            self->counts[i] = edit;
            ui_window_create(edit, 0x2a, 0x10);
            ui_window_move_base(self->counts[i], wnd->width - 0xe7, y);
            UI_EDIT_MAX_CHARS(self->counts[i]) = 5;
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->counts[i], "0");
            ui_edit_ctrl_set_back_colour(self->counts[i], EDIT_LIGHT, EDIT_LIGHT, EDIT_LIGHT);
            ui_window_add_child(wnd, self->counts[i]);
        }
        edit = new_wheel_edit();
        self->prices[i] = edit;
        ui_window_create(edit, 0x5e, 0x10);
        ui_window_move(self->prices[i], wnd->width - 0x77, y);
        UI_EDIT_MAX_CHARS(self->prices[i]) = 0xe;
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->prices[i], "0");
        if (self->buying) {
            ui_edit_ctrl_set_back_colour(self->prices[i], EDIT_LIGHT, EDIT_LIGHT, EDIT_LIGHT);
        } else {
            ((struct UIEditCtrl *)self->prices[i])->disabled = 1;
            ui_window_invalidate(self->prices[i]);
            ui_edit_ctrl_set_back_colour(self->prices[i], EDIT_GREY, EDIT_GREY, EDIT_GREY);
        }
        ui_window_add_child(wnd, self->prices[i]);
    }

    if (self->buying) {
        int x = ui_window_text_width(wnd, msg_str(MSG_TOTAL), 0, 0, 0xc, 0, 0);

        self->total = new_wheel_edit();
        ui_window_move_base(self->total, x + 0xa, wnd->height - 0x11);
        ui_window_create(self->total, 0x5e, 0x10);
        UI_EDIT_MAX_CHARS(self->total) = 0xf;
        ui_edit_ctrl_set_back_colour(self->total, EDIT_LIGHT, EDIT_LIGHT, EDIT_LIGHT);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->total, "0");
        ui_window_add_child(wnd, self->total);
    }

    self->buttons[0] = shop_add_button(wnd, ok_button, wnd->width - 0x5c, wnd->height - 0x18, UI_ID_OK);
    self->buttons[1] = shop_add_button(wnd, g_shopCancelButton, wnd->width - 0x2e, wnd->height - 0x18, UI_ID_CANCEL);
    self->frame.default_id = UI_ID_OK;
    session_clear_store_setup(&g_session1);

    if (!self->buying) {
        struct UIWindow *box = mem_alloc(sizeof(struct UICheckBox));

        if (box)
            box = (struct UIWindow *)ui_check_box_construct(box);
        self->price_check = box;
        ui_window_create(box, 0x8c, 0xc);
        ui_check_box_set_bitmaps(self->price_check, UI_SKIN "checkbox_1.bmp", UI_SKIN "checkbox_0.bmp",
                              msg_str(MSG_PRICE_CHECK));
        ui_check_box_fit_to_label((struct UICheckBox *)self->price_check);
        ui_window_move_base(self->price_check, 0xc, wnd->height - 0x14);
        ui_check_box_set_check(self->price_check, g_session1.noPriceCheckWarning);
        ui_window_add_child(wnd, self->price_check);
    }
    self->base_height = wnd->height;
    ui_merchant_shop_make_wnd_send_msg(wnd, NULL, MSM_MSG_REFRESH, 0, 0, 0, 0);

    {
        struct UIBitmapButton *info = mem_alloc(sizeof(struct UIBitmapButton));
        int state;

        if (info)
            info = ui_bitmap_button_construct(info);
        for (state = 0; state < 3; state++)
            ui_bitmap_button_set_bitmap(info, UI_SKIN "basic_interface\\bt_shopinfo.bmp", state, 0);
        ui_window_create(&info->button.base, info->bitmap_width, info->bitmap_height);
        ui_window_move_base(&info->button.base, wnd->width - 0x17, 0x14);
        info->button.base.id = ID_SHOP_INFO;
        ui_window_add_child(wnd, &info->button.base);
    }
}

void ui_merchant_shop_make_wnd_add_load_backup_button(struct UIMerchantShopMakeWnd *self)
{
    struct UIBasicButton *button = mem_alloc(sizeof *button);

    if (button)
        button = ui_basic_button_construct(button);
    self->load_button = button;
    ui_basic_button_set_label(&button->button.base, 0x3c, msg_str(MSG_LOAD_BACKUP));
    ui_window_move_base(&self->load_button->button.base, self->frame.base.width - 0x9c, self->frame.base.height - 0x18);
    ui_button_set_tool_tip(&self->load_button->button, msg_str(MSG_LOAD_BACKUP_TIP));
    self->load_button->button.base.id = ID_LOAD_BACKUP;
    ui_window_add_child(&self->frame.base, &self->load_button->button.base);
}

void ui_merchant_shop_make_wnd_set_waiting(struct UIMerchantShopMakeWnd *self, BYTE waiting)
{
    struct UIWindow *items = self->buying == 1 ? g_windowMgr.buying_store_mirror_item_wnd : g_windowMgr.merchant_mirror_item_wnd;

    if (!items)
        return;
    if (waiting == 1) {
        self->waiting = 1;
        ui_window_set_show_base(&self->frame.base, 0);
        ui_window_set_show(items, 0);
        ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (int)msg_str(MSG_WAITING), 0xffff, 0, 0);
    } else {
        self->waiting = 0;
        ui_window_set_show_base(&self->frame.base, 1);
        ui_window_set_show(items, 1);
        self->cell_x = -1;
        self->cell_y = -1;
    }
}

struct ItemInfo *ui_merchant_shop_make_wnd_item_at(struct UIMerchantShopMakeWnd *self, struct ItemInfo *out, int x,
                                                         int y)
{
    struct ListNode *node;
    int index, shown, row;

    if (x < 1 || x >= self->frame.base.width - 0x14 || y < ROW_HIT_TOP
        || y >= self->shown_rows * ROW_HEIGHT + ROW_TOP)
        return item_info_construct_with_arg(out);
    row = (y - ROW_TOP) / ROW_HEIGHT;
    shown = 0;
    for (node = self->items.head->next, index = 0; node != self->items.head; node = node->next, index++) {
        if (index < self->first_row * self->columns)
            continue;
        if (item_grid_hidden(LIST_VALUE(node, struct ItemInfo)))
            continue;
        if (shown == self->columns * row) {
            if (x < ROW_ICON_X)
                return item_info_copy(out, LIST_VALUE(node, struct ItemInfo));
            return item_info_construct_with_arg(out);
        }
        shown++;
    }
    return item_info_construct_with_arg(out);
}

void ui_merchant_shop_make_wnd_update_scroll(struct UIMerchantShopMakeWnd *self)
{
    struct UIWindow *wnd = &self->frame.base;
    int top, fit, size, i;

    if (!self->scroll)
        return;
    top = self->base_height - self->rows * ROW_HEIGHT;
    fit = (g_renderer->height - top) / ROW_HEIGHT;
    self->fit_rows = fit;
    if (fit > self->slots)
        self->fit_rows = fit = self->slots;
    size = self->items.size;
    if (size >= self->rows) {
        if (size > fit) {
            self->scrollable = 1;
            ui_scroll_bar_set_range(self->scroll, size - fit, fit, size);
            if (self->first_row < 0)
                self->first_row = 0;
            if (self->first_row > size - self->fit_rows)
                self->first_row = size - self->fit_rows;
            ui_scroll_bar_set_pos(self->scroll, self->first_row);
        } else {
            self->scrollable = 0;
            self->first_row = 0;
            ui_scroll_bar_set_range(self->scroll, 0, fit, size);
            fit = size;
        }
        ui_window_resize_base(wnd, wnd->width, fit * ROW_HEIGHT + top);
        if (!self->buying)
            ui_window_move_base(self->price_check, 0xc, wnd->height - 0x14);
        for (i = 0; i < 2; i++)
            ui_window_move_base(&self->buttons[i]->button.base, self->buttons[i]->button.base.x, wnd->height - 0x18);
        if (self->load_button)
            ui_window_move_base(&self->load_button->button.base, self->load_button->button.base.x, wnd->height - 0x18);
        ui_scroll_bar_resize(&self->scroll->base, 1, wnd->height - 0x3a);
    }
    self->shown_rows = (wnd->height - top) / ROW_HEIGHT;
    if (self->shown_rows < self->rows)
        self->shown_rows = self->rows;
    ui_window_invalidate_base(&self->scroll->base);
}

int ui_merchant_shop_make_wnd_commit_price(struct UIMerchantShopMakeWnd *self, int next_row)
{
    struct UIWindow *edit;
    char buf[0x18];
    int64_t price;

    if (self->focused == -1 || !g_session1.noPriceCheckWarning) {
        self->focused = next_row;
        return 1;
    }
    edit = self->prices[self->focused];
    sprintf(buf, "%s ", ui_edit_ctrl_get_text(edit));
    if (!session_is_number_text(&g_session1, buf)) {
        ui_message_box(msg_str(MSG_NOT_A_NUMBER));
        goto refused;
    }
    price = strtoll(buf, NULL, 10);
    if (price >= 0x7fffffff)
        goto too_high;
    price = strtoll(buf, NULL, 10);
    if (price <= 0)
        price = 0;
    else if (price > PRICE_MAX)
        goto too_high;
    goto checked;

too_high:
    if (msm_question_box(msg_str(MSG_PRICE_TOO_HIGH)) != UI_MSGBOX_ANSWER_YES)
        goto refused;
    price = PRICE_MAX;
    snprintf(buf, sizeof buf, "%d", PRICE_MAX);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, buf);
    ui_window_invalidate_base(&self->frame.base);

checked:
    if (price >= (int64_t)atoi(msg_str(MSG_PRICE_UNIT_SMALL))) {
        struct ItemInfo item;
        struct String text, part;
        const char *name;
        int big, small;

        session_get_store_setup_item_at(&g_session1, &item, self->focused);
        name = item_info_get_name(&item);
        string_init(&text);
        string_format(&text, msg_str(MSG_PRICE_CONFIRM), name);
        big = (int)(price / atoi(msg_str(MSG_PRICE_UNIT_BIG)));
        if (big > 0) {
            string_init(&part);
            string_format(&part, msg_str(MSG_PRICE_UNIT_BIG_TEXT), big);
            string_append(&text, string_cstr(&part), part.size);
            string_free(&part);
        }
        small = (int)(price % atoi(msg_str(MSG_PRICE_UNIT_BIG))) / atoi(msg_str(MSG_PRICE_UNIT_SMALL));
        if (small > 0) {
            string_init(&part);
            string_format(&part, msg_str(MSG_PRICE_UNIT_SMALL_TEXT), small);
            string_append(&text, string_cstr(&part), part.size);
            string_free(&part);
        }
        string_init(&part);
        string_assign_cstr(&part, msg_str(MSG_PRICE_CONFIRM_TAIL));
        string_append(&text, string_cstr(&part), part.size);
        string_free(&part);
        if (msm_question_box(string_cstr(&text)) != UI_MSGBOX_ANSWER_YES) {
            msm_edit_reset(edit, "0");
            string_free(&text);
            item_info_destruct(&item);
            return 0;
        }
        string_free(&text);
        item_info_destruct(&item);
    }
    msm_edit_amount_colour(&self->frame.base, edit, price);
    format_money64(price, buf, sizeof buf);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, buf);
    self->focused = next_row;
    if (next_row == -1) {
        ui_window_mgr_set_focus_edit(&g_windowMgr, NULL);
    } else {
        ui_edit_ctrl_set_text_colour(self->prices[next_row], 0, 0, 0, -1);
        ui_window_mgr_set_focus_edit(&g_windowMgr, self->prices[next_row]);
        ui_edit_ctrl_select_all(self->prices[next_row]);
    }
    return 1;

refused:
    msm_edit_reset(edit, "0");
    return 0;
}

static void draw_price_with_commission(struct UIWindow *wnd, struct UIWindow *edit, int x, int y, double sign)
{
    char buf[0x18] = { 0 };
    struct AmountColours c;
    const char *word;
    int w;
    int64_t price;

    strcpy(buf, ui_edit_ctrl_get_text(edit));
    if (session_is_number_text(&g_session1, buf) != 1)
        return;
    price = strtoll(buf, NULL, 10);
    if (price <= 0)
        return;
    ui_window_text_out(wnd, x, y, msg_str(MSG_WITH_COMMISSION), 0, 0, 0xc, 0, 0, 0);
    word = msg_str(MSG_WITH_COMMISSION);
    w = ui_window_text_width(wnd, word, 0, 0, 0xc, 0, 0);
    price = price_with_commission(price, sign);
    format_money64(price, buf, sizeof buf);
    ui_window_amount_colours(wnd, &c, price);
    if (c.double_colour == -1)
        ui_window_text_out(wnd, x + w, y, buf, 0, 0, 0xc, c.colour, 0, 0);
    else
        ui_window_text_out_doubled(wnd, x + w, y, buf, c.colour, c.double_colour, 0, 0, 0xc);
}

void ui_merchant_shop_make_wnd_on_draw(struct UIWindow *window)
{
    struct UIMerchantShopMakeWnd *self = (struct UIMerchantShopMakeWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct ListNode *node;
    int bar = 0, i, y, index, shown;

    ui_window_clear_canvas(wnd, 0xff);
    ui_window_draw_title_bar(wnd, 0, "", 0);
    ui_window_fill_rect(wnd, 0, 0x11, wnd->width, wnd->height - 0x26, ui_skin_colour(2, 2));
    if (self->buying)
        bar = ui_window_draw_button_bar(wnd, 2, 0, 2);
    else
        ui_window_draw_button_bar(wnd, 2, 0, 1);

    for (i = 0; i < self->slots; i++) {
        int show = i < self->shown_rows;

        ui_window_set_show(self->prices[i], show);
        if (self->buying)
            ui_window_set_show_base(self->counts[i], show);
    }
    for (i = 0; i < self->slots; i++) {
        BYTE hidden = 1;

        if (self->first_row <= i && i < self->first_row + self->shown_rows) {
            ui_window_move(self->prices[i], wnd->width - 0x77, (i - self->first_row) * ROW_HEIGHT + ROW_FIRST_Y);
            hidden = 0;
        }
        ui_window_set_show(self->prices[i], hidden ^ 1);
        if (self->buying)
            ui_window_move_base(self->counts[i], wnd->width + hidden * 0x154 - 0xdd,
                     (i - self->first_row) * ROW_HEIGHT + ROW_FIRST_Y);
    }

    if (self->buying) {
        unsigned rows = self->items.size <= (unsigned)self->rows ? (unsigned)self->rows : self->items.size;
        struct UIWindow *total = self->total;
        char money[0x10] = { 0 }, text[0x20] = { 0 };
        COLORREF colour;

        for (i = 0, y = 0x48; i < (int)rows; i++, y += ROW_HEIGHT) {
            int label_y = y - 0xf;

            ui_window_text_out(wnd, wnd->width - 0x9b, label_y, msg_str(MSG_BUYING_PRICE), 0, 0, 0xc, 0, 0, 0);
            ui_window_text_out(wnd, wnd->width - 0xb0, label_y, msg_str(MSG_COUNT_WORD), 0, 0, 0xc, 0, 0, 0);
            ui_window_draw_item_win_mid(wnd, 3, label_y - 3);
            if (i < self->slots)
                draw_price_with_commission(wnd, self->prices[i], wnd->width - 0x9b, y, 1.0);
        }
        ui_window_text_out(wnd, 0x11, 4, msg_str(MSG_BUYING_TITLE), 0, 0, 0xc, 0xffffff, 0, 0);
        ui_window_text_out(wnd, 0x10, 3, msg_str(MSG_BUYING_TITLE), 0, 0, 0xc, 0, 0, 0);
        ui_window_text_out(wnd, 2, 0x19, msg_str(MSG_SHOP_TITLE_LABEL), 0, 0, 0xc, 0, 0, 0);
        ui_window_move_base(total, ui_window_text_width(wnd, msg_str(MSG_TOTAL), 0, 0, 0xc, 0, 0) + 0xa,
                 wnd->height - bar + 5);
        ui_window_text_out(wnd, 5, wnd->height - bar + 5, msg_str(MSG_TOTAL), 0, 0, 0xc, 0, 0, 0);
        ui_window_text_out(wnd, self->total->x + 5 + self->total->width, wnd->height - bar + 5, "Zeny", 0, 0, 0xc,
                         0, 0, 0);
        format_money(g_session1.zeny, money);
        sprintf(text, "%s Zeny", money);
        ui_window_text_out(wnd, 5, wnd->height - bar * 2 + 5, msg_str(MSG_ZENY_LINE), 0, 0, 0xc, 0, 0, 0);
        ui_window_text_out_right(wnd, 0x9b, wnd->height - bar * 2 + 5, text, 0, 0, 0xc, 0, 0);
        if (self->assistant == 1) {
            sprintf(text, "Max Weight : %3d", g_session1.maxWeight);
            colour = 0;
        } else {
            sprintf(text, "Weight : %3d / %3d", g_session1.weight, g_session1.maxWeight);
            colour = g_session1.weight * 100 / (g_session1.maxWeight > 1 ? g_session1.maxWeight : 1) < 50 ? 0 : 0xff;
        }
        ui_window_text_out_right(wnd, wnd->width - 0x14, wnd->height - bar * 2 + 5, text, 0, 0, 0xc, colour, 0);
        ui_window_text_out(wnd, 5, wnd->height - 0x48, msg_str(MSG_BUYING_FOOTER), 0, 0, 0xc, 0xff, 0, 0);
    } else {
        for (i = 0, y = 0x48; i < self->shown_rows; i++, y += ROW_HEIGHT) {
            int label_y = y - 0x12;

            ui_window_text_out(wnd, wnd->width - 0xa5, label_y + 3, msg_str(MSG_VENDING_PRICE), 0, 0, 0xc, 0, 0,
                             0);
            ui_window_draw_item_win_mid(wnd, 3, label_y);
            if (i < self->slots)
                draw_price_with_commission(wnd, self->prices[self->first_row + i], wnd->width - 0xa5, y, -1.0);
        }
        ui_window_text_out(wnd, 0x11, 4, msg_str(MSG_VENDING_TITLE), 0, 0, 0xc, 0xffffff, 0, 0);
        ui_window_text_out(wnd, 0x10, 3, msg_str(MSG_VENDING_TITLE), 0, 0, 0xc, 0, 0, 0);
        ui_window_text_out(wnd, 2, 0x19, msg_str(MSG_SHOP_TITLE_LABEL), 0, 0, 0xc, 0, 0, 0);
        ui_window_text_out(wnd, 5, wnd->height - 0x2b, msg_str(MSG_VENDING_FOOTER), 0, 0, 0xc, 0xff, 0, 0);
    }

    shown = 0;
    for (node = self->items.head->next, index = 0; node != self->items.head; node = node->next, index++) {
        struct ItemInfo *info = LIST_VALUE(node, struct ItemInfo);
        int x, row_y;

        if (index < self->columns * self->first_row)
            continue;
        if (item_grid_hidden(info))
            continue;
        row_y = shown / self->columns * ROW_HEIGHT;
        x = shown % self->columns * ROW_HEIGHT;
        ui_window_draw_item_icon_count(wnd, x + 7, row_y + 0x30, info, 0, 0);
        ui_window_draw_item_name_short(wnd, x + 0x27, row_y + 0x39, 0x64, info, 0xc);
        if (info->grade > 0) {
            struct String path;

            string_init(&path);
            string_format(&path, UI_SKIN "grade_enchant\\grade_icon%d.bmp", (int)info->grade);
            ui_window_draw_bitmap(wnd, x + 7, row_y + 0x3c, res_mgr_get(g_resMgr(), string_cstr(&path)),
                                1);
            string_free(&path);
        }
        if (++shown == self->shown_rows * self->columns)
            break;
    }
}

void ui_merchant_shop_make_wnd_on_process(struct UIWindow *window)
{
    struct UIMerchantShopMakeWnd *self = (struct UIMerchantShopMakeWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct UIWindow *edit;
    char buf[0x18];
    int64_t price;
    int row;

    if (self->total && self->buying) {
        int64_t total = 0;
        char text[0x80];
        unsigned i;

        for (i = 0; i < self->items.size; i++) {
            char price_text[0x18] = { 0 };

            snprintf(price_text, sizeof price_text, "%s",
                        ui_edit_ctrl_get_text(self->prices[i]));
            if (session_is_number_text(&g_session1, price_text) == 1) {
                int64_t count;

                price = strtoll(price_text, NULL, 10);
                count = strtoll(ui_edit_ctrl_get_text(self->counts[i]), NULL, 10);
                total += price_with_commission(price, 1.0) * count;
            }
        }
        memset(text, 0, sizeof text);
        snprintf(text, sizeof text, "%lld", total);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->total, text);
        ui_window_invalidate_children(wnd);
        return;
    }

    if (g_windowMgr.modal_wnd)
        return;
    for (row = 0; row < self->slots; row++)
        if (self->prices[row] == ui_window_mgr_get_focus_edit(&g_windowMgr))
            break;
    if (row >= self->slots)
        row = -1;
    if (self->focused != row) {
        if (row != -1) {
            const char *text = ui_edit_ctrl_get_text(self->prices[row]);

            string_assign_cstr(&self->last_text, text);
        }
        ui_merchant_shop_make_wnd_commit_price(self, row);
        ui_window_invalidate_children(wnd);
        return;
    }
    if (row == -1)
        return;
    {
        int size = ui_edit_ctrl_get_text_size(self->prices[row]);

        if (size == self->last_size)
            return;
        self->last_size = size;
        if (!size)
            return;
    }
    edit = self->prices[row];
    sprintf(buf, "%s ", ui_edit_ctrl_get_text(edit));
    if (!session_is_number_text(&g_session1, buf)) {
        ui_message_box(msg_str(MSG_NOT_A_NUMBER));
        ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, string_cstr(&self->last_text));
        ui_window_mgr_set_focus_edit(&g_windowMgr, edit);
        return;
    }
    if (strtoll(buf, NULL, 10) <= 0) {
        price = 0;
    } else {
        price = strtoll(buf, NULL, 10);
        if (price > PRICE_MAX) {
            if (msm_question_box(msg_str(MSG_PRICE_TOO_HIGH)) == UI_MSGBOX_ANSWER_YES) {
                price = PRICE_MAX;
                snprintf(buf, sizeof buf, "%d", PRICE_MAX);
                ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, buf);
            } else {
                ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, string_cstr(&self->last_text));
                ui_window_mgr_set_focus_edit(&g_windowMgr, edit);
            }
        }
    }
    msm_edit_amount_colour(wnd, edit, price);
    format_money64(price, buf, sizeof buf);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)edit, buf);
    {
        const char *text = ui_edit_ctrl_get_text(self->prices[row]);

        string_assign_cstr(&self->last_text, text);
    }
    self->last_size = ui_edit_ctrl_get_text_size(self->prices[row]);
    ui_edit_ctrl_set_caret(edit, ui_edit_ctrl_get_text_size(edit));
}

void ui_merchant_shop_make_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantShopMakeWnd *self = (struct UIMerchantShopMakeWnd *)window;
    struct ItemInfo item, entry;
    struct GameMode *mode;

    if (y < 0x11) {
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->frame), x, y);
        return;
    }

    ui_merchant_shop_make_wnd_item_at(self, &item, x, y);
    if ((unsigned)item.index > 0) {
        mode = game_current_mode();
        item_info_assign(&item, session_get_store_setup_item_by_index(&g_session1, &entry, item.index));
        item_info_destruct(&entry);
        game_mode_drag_item(mode, self->buying ? DRAG_FROM_BUYING_SETUP : DRAG_FROM_VENDING_SETUP, &item,
                          item.count, -1);
    }
    item_info_destruct(&item);
}

void ui_merchant_shop_make_wnd_on_r_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantShopMakeWnd *self = (struct UIMerchantShopMakeWnd *)window;
    struct ItemInfo item, entry;
    struct GameMode *mode;

    ui_merchant_shop_make_wnd_item_at(self, &item, x, y);
    if ((unsigned)item.index > 0) {
        mode = game_current_mode();
        if ((BYTE)(GetAsyncKeyState(VK_MENU) >> 8)) {
            if (g_windowMgr.merchant_mirror_item_wnd) {
                item_info_assign(&item, session_get_store_setup_item_by_index(&g_session1, &entry, item.index));
                item_info_destruct(&entry);
                game_mode_drag_item(mode, DRAG_FROM_VENDING_SETUP, &item, item.count, -1);
                ui_window_send_msg(g_windowMgr.merchant_mirror_item_wnd, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
                ui_window_mgr_set_focus_edit(&g_windowMgr, NULL);
            } else if (g_windowMgr.buying_store_mirror_item_wnd) {
                item_info_assign(&item, session_get_store_setup_item_by_index(&g_session1, &entry, item.index));
                item_info_destruct(&entry);
                game_mode_drag_item(mode, DRAG_FROM_BUYING_SETUP, &item, item.count, -1);
                ui_window_send_msg(g_windowMgr.buying_store_mirror_item_wnd, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
            }
        } else {
            ui_window_send(ui_window_open(ITEM_WND_ID), NULL, UI_MSG_SHOW_ITEM, (int)&item, 0, 0, 0);
        }
    }
    item_info_destruct(&item);
}

void ui_merchant_shop_make_wnd_on_wheel(struct UIWindow *window, int delta)
{
    struct UIMerchantShopMakeWnd *self = (struct UIMerchantShopMakeWnd *)window;
    int step;

    if (!self->scrollable)
        return;
    step = self->rows <= 3 ? self->rows - 1 : 3;
    if (delta >= 0)
        step = -step;
    ui_merchant_shop_make_wnd_send_msg(&self->frame.base, &self->scroll->base, UI_MSG_SCROLL_V, step, 0, 0, 0);
}

void ui_merchant_shop_make_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIMerchantShopMakeWnd *self = (struct UIMerchantShopMakeWnd *)window;
    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->frame.base, x, y, drag);
        return;
    }
    if (drag->source == DRAG_VENDING_ITEM) {
        if (drag->drop_count == 1) {
            ui_merchant_shop_make_wnd_send_msg(&self->frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
            return;
        }
        ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
        ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, 9, 0, 0, 0);
        return;
    }
    if (drag->source == DRAG_BUYING_ITEM) {
        ui_merchant_shop_make_wnd_send_msg(&self->frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    game_send_message(MODE_REQ_DRAG_END, 0, 0, 0, 0);
}

void ui_merchant_shop_make_wnd_store_info(struct UIWindow *window)
{
    struct UIMerchantShopMakeWnd *self = (struct UIMerchantShopMakeWnd *)window;
    if (!self->info)
        return;
    self->info->x = self->frame.base.x;
    self->info->y = self->frame.base.y;
    self->info->width = self->frame.base.width;
    self->info->height = self->frame.base.height;
}
