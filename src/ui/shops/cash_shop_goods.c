#include "world/tooltip.h"
#include "render/rp_pool.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include <string.h>
#include "ui/shops/shops.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "ui/wndmgr/wndmgr.h"
#include "platform/clock.h"

#define MSG_CASH_ADD 0x6f3
#define MSG_CASH_SOLD_OUT 0xe4d
#define MSG_CASH_BADGE 0xe4b
#define MSG_CASH_BADGE_ZERO_PRICE 0xf4b
#define MSG_CASH_ZERO_PRICE_NOTE 0xf4e
#define MSG_CASH_STOCK_LABEL 0xe4a
#define MSG_CASH_START_FMT 0xe49
#define MSG_CASH_END_FMT 0xe4f

struct UICashShopWnd_Item;
struct UIWindow;
void ui_cash_shop_wnd_item_destroy(struct UIWindow *window);
void ui_cash_shop_wnd_item_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_cash_shop_wnd_item_on_create(struct UIWindow *self, int width, int height);
void ui_cash_shop_wnd_item_on_draw(struct UIWindow *self);
void ui_cash_shop_wnd_item_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_cash_shop_wnd_item_on_mouse_hover(struct UIWindow *self, int x, int y);
void ui_cash_shop_wnd_item_show_info(struct UIWindow *self, int x, int y);
int ui_cash_shop_wnd_item_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, int val1, int val2, int val3, int val4);
const struct UIWindowOps ui_cash_shop_wnd_item_ops = {
    .destroy = ui_cash_shop_wnd_item_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 0,
    .refreshes_children = 0,
    .move = ui_window_move_base,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_cash_shop_wnd_item_drag_and_drop,
    .store_info = ui_window_store_info_base,
    .save_pos = ui_window_save_pos_base,
    .move_delta = ui_window_move_delta_base,
    .set_show = ui_window_set_show_base,
    .on_create = ui_cash_shop_wnd_item_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_cash_shop_wnd_item_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_cash_shop_wnd_item_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_window_on_mouse_move_base,
    .on_mouse_hover = ui_cash_shop_wnd_item_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_window_on_l_btn_up_base,
    .on_r_btn_down = ui_window_on_r_btn_down_base,
    .on_r_btn_up = ui_cash_shop_wnd_item_show_info,
    .on_wheel = ui_window_on_wheel_base,
    .snap = ui_window_snap_base,
    .send_msg = ui_cash_shop_wnd_item_send_msg,
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

struct UICashShopWnd_Item {
    struct UIWindow base;
    struct UIBmpTextButton *add;
    struct ItemInfo info;
    BYTE sale;
    struct UIStaticBitmap *sold_out;
};

static struct UICashShopWnd_Item *ui_cash_shop_wnd_item_construct(struct UICashShopWnd_Item *self)
{
    ui_window_construct(&self->base, 0);
    self->base.ops = &ui_cash_shop_wnd_item_ops;
    self->add = NULL;
    item_info_construct(&self->info);
    self->sale = 0;
    self->sold_out = NULL;
    return self;
}

struct UICashShopWnd_Item *ui_cash_shop_wnd_item_new(void)
{
    struct UICashShopWnd_Item *self = mem_alloc(sizeof *self);

    return self ? ui_cash_shop_wnd_item_construct(self) : NULL;
}

void ui_cash_shop_wnd_item_destruct(struct UICashShopWnd_Item *self)
{
    self->base.ops = &ui_cash_shop_wnd_item_ops;
    item_info_destruct(&self->info);
    ui_window_destruct(&self->base);
}

void ui_cash_shop_wnd_item_destroy(struct UIWindow *window)
{
    struct UICashShopWnd_Item *self = (struct UICashShopWnd_Item *)window;
    ui_cash_shop_wnd_item_destruct(self);
    mem_free(self);
}

void ui_cash_shop_wnd_item_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UICashShopWnd_Item *self = (struct UICashShopWnd_Item *)window;
    if (self->base.parent)
        ui_window_drag_and_drop(self->base.parent, x, y, drag);
}

static void add_caption_style(struct UIBmpTextButton *add, COLORREF colour, char arg4)
{
    int state;

    for (state = UI_BTN_UP; state <= UI_BTN_DOWN; state++)
        ui_bmp_text_button_set_text_style(add, state, 12, colour, arg4, 1, UI_TEXT_CENTRED, UI_TEXT_CENTRED);
}

static void set_add_caption(struct UIBmpTextButton *add, int msg)
{
    ui_bmp_text_button_set_caption(add, text_view_cstr(msg_str(msg)));
}

void ui_cash_shop_wnd_item_on_create(struct UIWindow *window, int width, int height)
{
    struct UICashShopWnd_Item *self = (struct UICashShopWnd_Item *)window;
    struct UIBmpTextButton *add = mem_alloc(sizeof *add);
    struct UIStaticBitmap *stamp;
    const char *caption;

    if (add)
        add = ui_bmp_text_button_construct(add);
    self->add = add;
    ui_bitmap_button_set_bitmap(&add->bmp, UI_SKIN "cashshop\\btn_add_normal.bmp", UI_BTN_UP, 0);
    ui_bitmap_button_set_bitmap(&self->add->bmp, UI_SKIN "cashshop\\btn_add_press.bmp", UI_BTN_HOVER, 0);
    ui_bitmap_button_set_bitmap(&self->add->bmp, UI_SKIN "cashshop\\btn_add_press.bmp", UI_BTN_DOWN, 0);
    ui_bitmap_button_set_bitmap(&self->add->bmp, UI_SKIN "cashshop\\btn_add_disable.bmp", UI_BMP_DISABLED, 0);
    add = self->add;
    ui_window_create(&add->bmp.button.base, add->bmp.bitmap_width, add->bmp.bitmap_height);
    caption = msg_str(MSG_CASH_ADD);
    string_assign_cstr(&self->add->caption, caption);
    ui_window_invalidate_base(&self->add->bmp.button.base);
    ui_window_move_base(&self->add->bmp.button.base, 0x53, 0x55);
    self->add->bmp.button.base.id = CASH_ID_ADD;
    ui_window_add_child(&self->base, &self->add->bmp.button.base);

    stamp = mem_alloc(sizeof *stamp);
    if (stamp)
        stamp = ui_static_bitmap_construct(stamp);
    self->sold_out = stamp;
    ui_static_bitmap_set_bitmap(stamp, UI_SKIN "cashshop\\img_shop_soldout.bmp");
    ui_window_create(&self->sold_out->base, self->sold_out->base.width, self->sold_out->base.height);
    ui_window_move_base(&self->sold_out->base, 9, 9);
    ui_window_set_show_base(&self->sold_out->base, 0);
    ui_window_add_child(&self->base, &self->sold_out->base);
}

static void draw_goods(struct UICashShopWnd_Item *self, int icon_y)
{
    struct String path;
    struct String text = {0};

    ui_window_fit_text(&self->base, &text, 0xa8, string_cstr(&self->info.name), 0, 12, 1);
    ui_window_text_out_centred(&self->base, 0, 6, string_cstr(&text), self->base.width, 0, 12, 0xa44f3b, 1);
    string_init(&path);
    string_format(&path, UI_SKIN "collection\\%s.bmp", item_info_get_res_name(&self->info));
    ui_window_draw_bitmap(&self->base, 5, icon_y, res_mgr_get_string(g_resMgr(), path), 1);
    string_init(&path);
    string_format(&path, "%d", CASH_GOODS_PRICE(&self->info));
    ui_window_text_out_right(&self->base, 0xa0, 0x32, string_cstr(&path), 0, 0, 12, 0xffffff, 1);
    string_free(&path);
    string_free(&text);
}

void ui_cash_shop_wnd_item_on_draw(struct UIWindow *window)
{
    struct UICashShopWnd_Item *self = (struct UICashShopWnd_Item *)window;
    struct ItemInfo *info = &self->info;
    struct String text, from, until;
    int32_t now;
    struct tm start, end;
    int x;

    if (!self->sale) {
        ui_window_draw_bitmap(&self->base, 0, 0,
                            res_mgr_get(g_resMgr(), UI_SKIN "cashshop\\img_shop_itemBg.bmp"), 0);
        if (info->index)
            draw_goods(self, 0x17);
        return;
    }

    ui_window_draw_bitmap(&self->base, 0, 0,
                        res_mgr_get(g_resMgr(), UI_SKIN "cashshop\\img_shop_item_bg2.bmp"), 0);
    string_init(&text);
    if (CASH_GOODS_PRICE(info) == 0)
        string_format(&text, msg_str(MSG_CASH_BADGE_ZERO_PRICE));
    else
        string_format(&text, msg_str(MSG_CASH_BADGE), info->count);
    ui_window_text_out_centred_outlined(&self->base, 6, 0x22, string_cstr(&text), 0x49, 0, 0xffffff, 0, 0,
                                    11, 1);
    string_free(&text);
    if (info->index)
        draw_goods(self, 0x34);

    if (CASH_GOODS_PRICE(info) == 0) {
        ui_window_text_out(&self->base, 0x53, 0x4c, msg_str(MSG_CASH_ZERO_PRICE_NOTE), 0, 0, 11, 0xff, 0, 0);
    } else {
        int left = CASH_GOODS_STOCK(info) < 0 ? 0 : CASH_GOODS_STOCK(info);

        string_format(&text, "%d/%d", left, info->count);
        ui_window_text_out(&self->base, 0x53, 0x4c, msg_str(MSG_CASH_STOCK_LABEL), 0, 0, 11, 0x6e4b41, 0, 0);
        ui_window_text_out_centred_outlined(&self->base, 0x53, 0x5b, string_cstr(&text), 0x51, 0, 0xffffff,
                                        0xd29374, 0, 16, 1);
        string_free(&text);
    }

    now = platform_time_now();
    start = *platform_time_local(CASH_GOODS_START(info));
    end = *platform_time_local(CASH_GOODS_END(info));
    string_init(&from);
    string_format(&from, msg_str(MSG_CASH_START_FMT), start.tm_mon + 1, start.tm_mday, start.tm_hour,
                  start.tm_min);
    string_init(&until);
    string_format(&until, msg_str(MSG_CASH_END_FMT), end.tm_mon + 1, end.tm_mday, end.tm_hour, end.tm_min);
    x = (int)((double)(self->base.width / 2) -
              (double)ui_window_text_width(&self->base, string_cstr(&from), 0, 0, 12, 0, 0) * 0.5);
    ui_window_text_out(&self->base, x, 0xa0, string_cstr(&from), 0, 0, 11, 0, 0, 0);
    x = 0x9e - ui_window_text_width(&self->base, string_cstr(&until), 0, 0, 12, 0, 0);
    ui_window_text_out(&self->base, x, 0xad, string_cstr(&until), 0, 0, 11, 0, 0, 0);
    string_free(&until);
    string_free(&from);
}

void ui_cash_shop_wnd_item_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UICashShopWnd_Item *self = (struct UICashShopWnd_Item *)window;
    if (self->info.index)
        game_mode_drag_item(game_current_mode(), DRAG_CASH_GOODS, &self->info, 1, -1);
}

void ui_cash_shop_wnd_item_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UICashShopWnd_Item *self = (struct UICashShopWnd_Item *)window;
    struct String name;
    int tip_x, tip_y;

    if (y > 0x17)
        return;
    tip_x = 0;
    tip_y = -20;
    ui_window_to_screen(&self->base, &tip_x, &tip_y);
    string_copy(&name, &self->info.name);
    mode_show_tooltip(client_current_mode(), string_cstr(&name), tip_x, tip_y, 0xffffffff, 0);
    string_free(&name);
}

void ui_cash_shop_wnd_item_show_info(struct UIWindow *window, int x, int y)
{
    struct UICashShopWnd_Item *self = (struct UICashShopWnd_Item *)window;
    struct UIWindow *wnd;

    if ((unsigned)(x - 5) > 0x4b || (unsigned)(y - 0x17) > 0x64)
        return;
    wnd = ui_make_window(ITEM_WND_ID);
    if (wnd)
        ui_window_send_msg(wnd, NULL, UI_MSG_SHOW_ITEM, (int)&self->info, 0, 0, 0);
}

int ui_cash_shop_wnd_item_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                          int val1, int val2, int val3, int val4)
{
    struct UICashShopWnd_Item *self = (struct UICashShopWnd_Item *)window;
    if (msg == CASH_MSG_SET_GOODS) {
        item_info_assign(&self->info, (const struct ItemInfo *)val1);
        ui_window_set_show_base(&self->base, 1);
        self->add->bmp.button.disabled = 0;
        ui_window_invalidate_base(&self->add->bmp.button.base);
        ui_window_set_show_base(&self->sold_out->base, 0);
        self->sale = val2 == CASH_TAB_SALE;
        if (val2 != CASH_TAB_SALE) {
            ui_window_move_base(&self->add->bmp.button.base, 0x53, 0x55);
            set_add_caption(self->add, MSG_CASH_ADD);
            add_caption_style(self->add, 0, 0);
        } else if (CASH_GOODS_STOCK(&self->info) > 0) {
            ui_window_move_base(&self->add->bmp.button.base, 0x53, 0x78);
            set_add_caption(self->add, MSG_CASH_ADD);
            add_caption_style(self->add, 0, 0);
        } else {
            ui_window_move_base(&self->add->bmp.button.base, 0x53, 0x78);
            self->add->bmp.button.disabled = 1;
            ui_window_invalidate_base(&self->add->bmp.button.base);
            set_add_caption(self->add, MSG_CASH_SOLD_OUT);
            add_caption_style(self->add, 0x313131, 1);
            ui_window_set_show_base(&self->sold_out->base, 1);
        }
        ui_window_invalidate_base(&self->base);
    } else if (msg == UI_MSG_COMMAND && val1 == CASH_ID_ADD && self->base.parent) {
        ui_window_send_msg(self->base.parent, &self->base, CASH_MSG_ADD, (int)&self->info, 0, 0, 0);
    }
    return ui_window_send_msg_base(&self->base, sender, msg, val1, val2, val3, val4);
}
