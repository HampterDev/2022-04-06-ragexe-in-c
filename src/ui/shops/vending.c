#include "render/rp_pool.h"
#include "ui/actor_labels.h"
#include "ui/controls/static.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "game/context.h"
#include <string.h>
#include "ui/shops/shops.h"
#include "ui/ui_std.h"
#include "core/msg_str.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/shops/merchant.h"

#define UI_ID_SHOP_TITLE 0xbd
#define MODE_REQ_OPEN_VENDING 0x54
#define MODE_REQ_OPEN_BUYING_STORE 0x110
#define MODE_REQ_NPC_STORE_WINDOWS_CLOSED 0xf5

struct UIMerchantShopTitle;
void ui_merchant_shop_title_destroy(struct UIWindow *window);
void ui_merchant_shop_title_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_merchant_shop_title_on_draw(struct UIWindow *self);
BYTE ui_merchant_shop_title_render(struct UIWindow *self, int force);
void ui_merchant_shop_title_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_merchant_shop_title_on_l_btn_dbl_clk(struct UIWindow *self, int x, int y);
void ui_merchant_shop_title_on_mouse_shape(struct UIWindow *self, int x, int y);
void ui_merchant_shop_title_on_r_btn_down(struct UIWindow *self, int x, int y);
int ui_merchant_shop_title_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_merchant_shop_title_ops = {
    .destroy = ui_merchant_shop_title_destroy,
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
    .on_create = ui_merchant_shop_title_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_merchant_shop_title_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_merchant_shop_title_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_merchant_shop_title_on_l_btn_dbl_clk,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_window_on_mouse_hover_base,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_merchant_shop_title_on_mouse_shape,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_merchant_shop_title_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_merchant_shop_title_send_msg,
    .invalidate = ui_window_invalidate_base,
    .render = ui_merchant_shop_title_render,
    .has_custom_backdrop = 0,
    .draw_backdrop = ui_window_draw_backdrop_base,
    .type = UI_TYPE_WINDOW,
    .on_hover_enter = ui_window_on_hover_enter_base,
    .on_hover_leave = ui_window_on_hover_leave_base,
    .make_quad = ui_window_make_quad_base,
    .hit_test = ui_window_hit_test_base,
    .render_overlay = ui_window_render_overlay_base,
};

struct UIMerchantShopTitle {
    struct UIWindow base;
    struct UIWindow *title;
    int owner;
    short font_code;
    BYTE highlighted;
    int buying;
};

static struct UIMerchantShopTitle *ui_merchant_shop_title_construct(struct UIMerchantShopTitle *self, int buying)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_merchant_shop_title_ops;
    self->base.redraws_tree = 1;
    self->buying = buying;
    self->highlighted = 0;
    self->base.id = UI_ID_SHOP_TITLE;
    return self;
}

struct UIMerchantShopTitle *ui_merchant_shop_title_new(int buying)
{
    struct UIMerchantShopTitle *self = mem_alloc(sizeof *self);

    return self ? ui_merchant_shop_title_construct(self, buying) : NULL;
}

void ui_merchant_shop_title_destruct(struct UIMerchantShopTitle *self)
{
    self->base.ops = &ui_merchant_shop_title_ops;
    ui_window_destruct(&self->base);
}

void ui_merchant_shop_title_destroy(struct UIWindow *window)
{
    struct UIMerchantShopTitle *self = (struct UIMerchantShopTitle *)window;
    ui_merchant_shop_title_destruct(self);
    mem_free(self);
}

void ui_merchant_shop_title_on_create(struct UIWindow *window, int width_unused, int height_unused)
{
    struct UIMerchantShopTitle *self = (struct UIMerchantShopTitle *)window;
    struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

    if (text)
        text = ui_static_text_construct(text);
    self->title = text;
    ui_window_create(text, self->base.width - 0x22, 0xf);
    ui_window_move(self->title, 0x1f, 8);
    ui_static_text_set_text_ellipsis(self->title, "");
    ((struct UIStaticText *)self->title)->font_face = 0;
    ((struct UIStaticText *)self->title)->font_height = 12;
    ((struct UIStaticText *)self->title)->font_arg3 = 0;
    ((struct UIStaticText *)self->title)->font_arg4 = 0;
    ui_window_add_child(&self->base, self->title);
}

static BYTE shop_title_highlighted(const struct UIMerchantShopTitle *self)
{
    return self->owner == g_session1.storeSearchClickedAid;
}

void ui_merchant_shop_title_on_draw(struct UIWindow *window)
{
    struct UIMerchantShopTitle *self = (struct UIMerchantShopTitle *)window;
    struct UIWindow *wnd = &self->base;

    self->highlighted = shop_title_highlighted(self);
    ui_window_clear_canvas(wnd, 0xffff00ff);
    ui_window_fill_round_rect(wnd, 0, 0, wnd->width, wnd->height - 6, self->highlighted ? 0x7dfaff : 0xffffff);
    ui_window_draw_round_frame(wnd, 2, 2, wnd->width - 4, wnd->height - 10, 0xc3c3c3);
    ui_window_draw_bitmap(wnd, 3, 3,
                        ui_skin_bitmap(self->buying ? UI_SKIN "buyingshop.bmp" : UI_SKIN "shop.bmp"), 1);
}

BYTE ui_merchant_shop_title_render(struct UIWindow *window, int force)
{
    struct UIMerchantShopTitle *self = (struct UIMerchantShopTitle *)window;

    if (shop_title_highlighted(self) != self->highlighted)
        ui_window_invalidate_base(window);
    return ui_window_render_base(window, force);
}

static void open_shop(struct UIMerchantShopTitle *self)
{
    struct Mode *mode = game_current_base_mode();

    if (self->buying) {
        if (!g_windowMgr.merchant_item_purchase_wnd)
            mode_send_message(mode, MODE_REQ_OPEN_BUYING_STORE, self->owner, 0, 0, 0);
    } else if (!g_windowMgr.buying_store_item_shop_wnd)
        mode_send_message(mode, MODE_REQ_OPEN_VENDING, self->owner, 0, 0, 0);
}

void ui_merchant_shop_title_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantShopTitle *self = (struct UIMerchantShopTitle *)window;
    if (g_session1.shopOneClick == 1)
        open_shop(self);
}

void ui_merchant_shop_title_on_l_btn_dbl_clk(struct UIWindow *window, int x, int y)
{
    struct UIMerchantShopTitle *self = (struct UIMerchantShopTitle *)window;
    open_shop(self);
}

void ui_merchant_shop_title_on_mouse_shape(struct UIWindow *window, int x, int y)
{
    struct UIMerchantShopTitle *self = (struct UIMerchantShopTitle *)window;
    if (x >= 0 && x < self->base.width && y >= 0 && y <= self->base.height)
        mode_set_cursor_action(g_modeMgr.curMode, CURSOR_HAND);
    else
        mode_set_cursor_action(g_modeMgr.curMode, CURSOR_ARROW);
}

void ui_merchant_shop_title_on_r_btn_down(struct UIWindow *self, int x, int y)
{
    if (g_session1.shopOneClick != 1)
        return;
    mode_send_message(g_modeMgr.curMode, MODE_REQ_NPC_STORE_WINDOWS_CLOSED, 0, 0, 0, 0);
    session_clear_purchase_lists(&g_session1);
}

#define SHOP_TITLE_MSG_OWNER 0x1c
#define SHOP_TITLE_MSG_NAME 0x22

static int shop_title_font_face(short font_code)
{
    switch (font_code) {
    case 1: return 0x15;
    case 2: return 0x14;
    case 3: return 0x16;
    case 4: return 0x17;
    case 5: return 0x18;
    case 6: return 0x19;
    case 7: return 0x1a;
    case 8: return 0x1b;
    case 9: return 0x1c;
    }
    return 0;
}

int ui_merchant_shop_title_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                           int val1, int val2, int val3, int val4)
{
    struct UIMerchantShopTitle *self = (struct UIMerchantShopTitle *)window;
    char *text = (char *)val1;
    struct String name;
    const char *chars, *cr;

    switch (msg) {
    case UI_MSG_COMMAND:
        return 0;
    case SHOP_TITLE_MSG_OWNER:
        self->owner = val1;
        return 0;
    case SHOP_TITLE_MSG_NAME:
        break;
    default:
        return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
    }
    string_init(&name);
    string_assign_cstr(&name, text);
    chars = string_cstr(&name);
    if (name.size && (cr = memchr(chars, '\r', name.size)) != NULL) {
        string_truncate(&name, (size_t)(cr - chars));
    }
    self->font_code = (short)val2;
    ((struct UIStaticText *)self->title)->font_face = shop_title_font_face(self->font_code);
    ((struct UIStaticText *)self->title)->font_arg3 = 0;
    ((struct UIStaticText *)self->title)->font_arg4 = 0;
    ((struct UIStaticText *)self->title)->font_height = 12;
    ui_static_text_set_text_ellipsis(self->title, string_cstr(&name));
    ui_window_invalidate_base(&self->base);
    string_free(&name);
    return 0;
}

#define WND_ID_VENDING_CONFIRM 0xe6
#define MODE_REQ_VENDING_BUY 0x53

#define MSG_VENDING_CONFIRM_TITLE 0x916
#define MSG_VENDING_CONFIRM_LINE1 0x917
#define MSG_VENDING_TOTAL_SUFFIX 0x918
#define MSG_VENDING_CONFIRM_LINE3 0x919

struct UIMerchantItemPurchaseMSGWnd;
void ui_merchant_item_purchase_msg_wnd_destroy(struct UIWindow *window);
void ui_merchant_item_purchase_msg_wnd_on_create(struct UIWindow *self, int width_unused, int height_unused);
void ui_merchant_item_purchase_msg_wnd_on_draw(struct UIWindow *self);
int ui_merchant_item_purchase_msg_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_merchant_item_purchase_msg_wnd_ops = {
    .destroy = ui_merchant_item_purchase_msg_wnd_destroy,
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
    .on_create = ui_merchant_item_purchase_msg_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_merchant_item_purchase_msg_wnd_on_draw,
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
    .send_msg = ui_merchant_item_purchase_msg_wnd_send_msg,
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

struct UIMerchantItemPurchaseMSGWnd {
    struct UIFrameWnd frame;
    struct UIWindow *title;
    struct UIWindow *line1;
    struct UIWindow *line3;
    struct UIWindow *total;
    struct UIWindow *buy_button;
    struct UIWindow *cancel_button;
    int request_arg1;
    int request_arg2;
};

static struct UIMerchantItemPurchaseMSGWnd *ui_merchant_item_purchase_msg_wnd_construct(struct UIMerchantItemPurchaseMSGWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_merchant_item_purchase_msg_wnd_ops;
    self->title = NULL;
    self->line1 = NULL;
    self->line3 = NULL;
    self->total = NULL;
    self->buy_button = NULL;
    self->cancel_button = NULL;
    self->request_arg1 = 0;
    self->request_arg2 = 0;
    return self;
}

struct UIMerchantItemPurchaseMSGWnd *ui_merchant_item_purchase_msg_wnd_new(void)
{
    struct UIMerchantItemPurchaseMSGWnd *self = mem_try_alloc(sizeof *self);

    return self ? ui_merchant_item_purchase_msg_wnd_construct(self) : NULL;
}

static void ui_merchant_item_purchase_msg_wnd_destruct(struct UIMerchantItemPurchaseMSGWnd *self)
{
    self->frame.base.ops = &ui_merchant_item_purchase_msg_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_merchant_item_purchase_msg_wnd_destroy(struct UIWindow *window)
{
    struct UIMerchantItemPurchaseMSGWnd *self = (struct UIMerchantItemPurchaseMSGWnd *)window;
    ui_merchant_item_purchase_msg_wnd_destruct(self);
    mem_free(self);
}

static struct UIWindow *add_text(int width)
{
    struct UIWindow *text = mem_alloc(sizeof(struct UIStaticText));

    if (text)
        text = ui_static_text_construct(text);
    ui_window_create(text, width, 15);
    return text;
}

static struct UIWindow *add_button(struct UIMerchantItemPurchaseMSGWnd *self, const char *up, const char *hover,
                                   const char *down, int from_right, int id)
{
    struct UIWindow *wnd = &self->frame.base;
    struct UIBitmapButton *button = mem_alloc(sizeof *button);

    if (button)
        button = ui_bitmap_button_construct(button);
    ui_bitmap_button_set_bitmap(button, up, UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(button, hover, UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(button, down, UI_BTN_DOWN, 0);
    ui_window_create(&button->button.base, button->bitmap_width, button->bitmap_height);
    ui_window_move_base(&button->button.base, wnd->width - from_right, wnd->height - 0x18);
    button->button.base.id = id;
    ui_window_add_child(wnd, &button->button.base);
    return &button->button.base;
}

void ui_merchant_item_purchase_msg_wnd_on_create(struct UIWindow *window, int width_unused,
                                                      int height_unused)
{
    struct UIMerchantItemPurchaseMSGWnd *self = (struct UIMerchantItemPurchaseMSGWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    self->title = add_text(0x64);
    ui_static_text_set_text_ellipsis(self->title, msg_str(MSG_VENDING_CONFIRM_TITLE));
    ui_window_move(self->title, 0x10, 2);
    ((struct UIStaticText *)self->title)->white_shadow = 1;
    ui_window_add_child(wnd, self->title);

    self->buy_button = add_button(self, UI_SKIN "btn_buy.bmp", UI_SKIN "btn_buy_a.bmp", UI_SKIN "btn_buy_b.bmp",
                                  0x6a, UI_ID_OK);
    self->cancel_button = add_button(self, UI_SKIN "btn_cancel.bmp", UI_SKIN "btn_cancel_a.bmp",
                                     UI_SKIN "btn_cancel_b.bmp", 0x3d, UI_ID_CANCEL);

    self->line1 = add_text(0xc8);
    ui_static_text_set_text(self->line1, msg_str(MSG_VENDING_CONFIRM_LINE1), 0);
    ui_window_move(self->line1, 10, 0x14);
    ui_window_add_child(wnd, self->line1);

    self->total = add_text(0xc8);
    ui_window_move(self->total, 10, 0x28);
    ui_window_add_child(wnd, self->total);

    self->line3 = add_text(0xc8);
    ui_static_text_set_text(self->line3, msg_str(MSG_VENDING_CONFIRM_LINE3), 0);
    ui_window_move(self->line3, 10, 0x3c);
    ui_window_add_child(wnd, self->line3);
}

void ui_merchant_item_purchase_msg_wnd_on_draw(struct UIWindow *window)
{
    struct UIMerchantItemPurchaseMSGWnd *self = (struct UIMerchantItemPurchaseMSGWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    ui_window_clear_canvas(wnd, 0xffffffff);
    ui_window_draw_title_bar(wnd, 0, "", 0);
    ui_window_draw_button_bar(wnd, 2, 0, 1);
}

int ui_merchant_item_purchase_msg_wnd_send_msg(struct UIWindow *window,
                                                    struct UIWindow *sender, int msg, int val1, int val2,
                                                    int val3, int val4)
{
    struct UIMerchantItemPurchaseMSGWnd *self = (struct UIMerchantItemPurchaseMSGWnd *)window;
    if (msg != UI_MSG_COMMAND)
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    if (val1 != UI_ID_OK && val1 != UI_ID_CANCEL)
        return 0;
    if (val1 == UI_ID_OK)
        mode_send_message(g_modeMgr.curMode, MODE_REQ_VENDING_BUY, self->request_arg1, self->request_arg2, 0, 0);
    ui_window_mgr_post_quit1(&g_windowMgr, WND_ID_VENDING_CONFIRM);
    session_clear_purchase_lists(&g_session1);
    return 0;
}

void ui_merchant_item_purchase_msg_wnd_set_purchase(struct UIMerchantItemPurchaseMSGWnd *self, int arg1,
                                                         int arg2)
{
    struct String suffix, number, text;
    const char *s;
    int count, i, total = 0;

    self->request_arg1 = arg1;
    self->request_arg2 = arg2;
    count = session_purchase_count(&g_session1);
    for (i = 0; i < count; i++) {
        struct ItemInfo item, entry;

        item_info_construct(&item);
        item_info_assign(&item, session_purchase_item(&g_session1, &entry, i));
        item_info_destruct(&entry);
        if ((unsigned)item.index > 0)
            total += item.count * item.price;
        item_info_destruct(&item);
    }

    s = msg_str(MSG_VENDING_TOTAL_SUFFIX);
    string_init(&suffix);
    string_assign_cstr(&suffix, s);
    number_text(&number, total);
    string_append(&number, string_cstr(&suffix), suffix.size);
    string_move(&text, &number);
    string_free(&suffix);
    ui_static_text_set_text(self->total, string_cstr(&text), 0);
    ui_window_invalidate_base(&self->frame.base);
    string_free(&text);
}
