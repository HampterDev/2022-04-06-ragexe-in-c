#include "ui/controls/edit.h"
#include "ui/shops/shops.h"
#include "ui/window_access.h"
#include "world/tooltip.h"
#include "ui/items/item_tags.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "world/mode_manager.h"
#include "game/context.h"
#include <string.h>
#include "ui/quests/npc_input.h"
#include "ui/shops/merchant.h"
#include "ui/chat/chat_window_parts.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"
#include "ui/items/items.h"
#include "ui/wndmgr/wndmgr.h"

struct UIMerchantItemWnd;
void ui_merchant_item_wnd_destroy(struct UIWindow *window);
void ui_merchant_item_wnd_store_info(struct UIWindow *self);
void ui_merchant_item_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_merchant_item_wnd_on_draw(struct UIWindow *self);
void ui_merchant_item_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_merchant_item_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
const struct UIWindowOps ui_merchant_item_wnd_ops = {
    .destroy = ui_merchant_item_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_merchant_item_wnd_drag_and_drop,
    .store_info = ui_merchant_item_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_merchant_item_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_merchant_item_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_merchant_item_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_merchant_item_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_merchant_item_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_merchant_item_wnd_send_msg,
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

struct UIMerchantMirrorItemWnd;
void ui_merchant_mirror_item_wnd_destroy(struct UIWindow *window);
void ui_merchant_mirror_item_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag);
void ui_merchant_mirror_item_wnd_store_info(struct UIWindow *self);
void ui_merchant_mirror_item_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_merchant_mirror_item_wnd_on_l_btn_down(struct UIWindow *self, int x, int y);
void ui_merchant_mirror_item_wnd_on_mouse_hover(struct UIWindow *self, int x, int y);
intptr_t ui_merchant_mirror_item_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);

const struct UIWindowOps ui_merchant_mirror_item_wnd_ops = {
    .destroy = ui_merchant_mirror_item_wnd_destroy,
    .resize = ui_window_resize_base,
    .is_frame = 1,
    .refreshes_children = 0,
    .move = ui_frame_wnd_move,
    .is_focusable = ui_window_is_focusable_base,
    .get_backdrop_box = ui_window_get_backdrop_box_base,
    .is_click_through = ui_window_is_click_through_base,
    .is_hit_testable = ui_window_is_hit_testable_base,
    .drag_and_drop = ui_merchant_mirror_item_wnd_drag_and_drop,
    .store_info = ui_merchant_mirror_item_wnd_store_info,
    .save_pos = ui_frame_wnd_save_pos,
    .move_delta = ui_frame_wnd_move_delta,
    .set_show = ui_window_set_show_base,
    .on_create = ui_merchant_mirror_item_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_base_wnd_on_draw,
    .on_focus = ui_window_on_focus_base,
    .on_focus_lost = ui_window_on_focus_lost_base,
    .on_l_btn_down = ui_merchant_mirror_item_wnd_on_l_btn_down,
    .on_l_btn_dbl_clk = ui_window_on_l_btn_dbl_clk_base,
    .on_mouse_move = ui_frame_wnd_on_mouse_move,
    .on_mouse_hover = ui_merchant_mirror_item_wnd_on_mouse_hover,
    .track_mouse = ui_window_track_mouse_base,
    .on_mouse_shape = ui_window_on_mouse_shape_base,
    .on_l_btn_up = ui_frame_wnd_on_l_btn_up,
    .on_r_btn_down = ui_merchant_mirror_item_wnd_on_r_btn_down,
    .on_r_btn_up = ui_window_on_r_btn_up_base,
    .on_wheel = ui_item_base_wnd_on_wheel,
    .snap = ui_frame_wnd_snap,
    .send_msg = ui_merchant_mirror_item_wnd_send_msg,
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

void merchant_store_info(struct UIWindow *wnd, struct WindowInfo *info)
{
    if (!info)
        return;
    info->x = wnd->x;
    info->y = wnd->y;
    info->width = wnd->width;
    info->height = wnd->height;
}

static int merchant_info_usable(const struct WindowInfo *info)
{
    return info->width >= 0x118 && info->height >= 0x78 && ui_info_on_screen(info);
}

static void merchant_info_default(struct WindowInfo *info)
{
    info->x = 0;
    info->y = 0x78;
    info->width = 0x118;
    info->height = 0x78;
}

static void merchant_grid(struct UIItemBaseWnd *base, int width, int height)
{
    base->columns = (width - ITEM_GRID_X) / ITEM_GRID_CELL;
    base->rows = (height - MERCHANT_BOTTOM) / ITEM_GRID_CELL;
}

static void merchant_tooltip(struct UIItemBaseWnd *base, int x, int y, int options_msg)
{
    struct ItemInfo info;

    ui_item_base_wnd_item_at(base, &info, x, y);
    if ((unsigned)info.index > 0) {
        struct String text;
        struct String name = {0};
        int tip_x, tip_y;
        struct Mode *mode;

        shop_tooltip_cell(&base->frame.base, x, y, &tip_x, &tip_y);
        mode = client_current_mode();
        item_info_get_display_name(&info, &name);
        string_init(&text);
        if (info.identified == 1 && info.option_count)
            string_format(&text, msg_str(options_msg), string_cstr(&name), info.option_count, info.count);
        else
            string_format(&text, "%s %d %s", string_cstr(&name), info.count, msg_str(MSG_TIP_COUNT_WORD));
        mode_show_tooltip(mode, string_cstr(&text), tip_x, tip_y,
                          session_get_item_dis_color(&info), 0);
        string_free(&text);
        string_free(&name);
    }
    item_info_destruct(&info);
}

static void merchant_link_item(const struct ItemInfo *info)
{
    struct UIWindow *edit;

    if (!(GetAsyncKeyState(VK_SHIFT) & 0xff00))
        return;
    edit = ui_window_mgr_get_focus_edit(&g_windowMgr);
    if (!edit || !info->identified)
        return;
    switch (edit->id) {
    case EDIT_ID_MEGAPHONE:
        megaphone_wnd_add_item_link(g_windowMgr.npc_text_edit_dialog_wnd, info);
        break;
    case EDIT_ID_WHISPER:
    case EDIT_ID_CHAT_ROOM:
        ui_chat_edit_ctrl_add_item_link(edit, info);
        break;
    case EDIT_ID_CHAT:
        ui_chat_edit_ctrl_add_item_link(ui_chat_window_edit(g_windowMgr.new_chat_wnd), info);
        break;
    case EDIT_ID_RODEX:
        ui_rodex_compose_wnd_add_item_link(g_windowMgr.rodex_compose_wnd, info);
        break;
    }
}

struct UIMerchantItemWnd *ui_merchant_item_wnd_construct(struct UIMerchantItemWnd *self)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_merchant_item_wnd_ops;
    self->base.button_bar = 1;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_merchant_item_wnd_new(void)
{
    return (struct UIWindow *)ui_merchant_item_wnd_construct(mem_alloc(sizeof(struct UIMerchantItemWnd)));
}

void ui_merchant_item_wnd_destruct(struct UIMerchantItemWnd *self)
{
    self->base.frame.base.ops = &ui_merchant_item_wnd_ops;
    ui_item_base_wnd_destruct(&self->base);
}

void ui_merchant_item_wnd_destroy(struct UIWindow *window)
{
    struct UIMerchantItemWnd *self = (struct UIMerchantItemWnd *)window;
    ui_merchant_item_wnd_destruct(self);
    mem_free(self);
}

void ui_merchant_item_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIMerchantItemWnd *self = (struct UIMerchantItemWnd *)window;
    struct GameMode *mode;

    if (g_windowMgr.exchange_wnd || g_windowMgr.say_dialog_wnd || g_windowMgr.choose_wnd) {
        ui_window_drag_and_drop_base(&self->base.frame.base, x, y, drag);
        return;
    }
    mode = game_current_mode();
    if (drag->source != DRAG_INVENTORY && drag->source != DRAG_STORAGE) {
        mode_send_message((struct Mode *)mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
    if (game_mode_refuse_held_item_drag(mode))
        return;
    if (drag->drop_count == 1) {
        ui_merchant_item_wnd_send_msg(&self->base.frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
    ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN, COUNT_FOR_CART, 0, 0, 0);
}

void ui_merchant_item_wnd_store_info(struct UIWindow *window)
{
    struct UIMerchantItemWnd *self = (struct UIMerchantItemWnd *)window;
    merchant_store_info(&self->base.frame.base, self->info);
}

void ui_merchant_item_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMerchantItemWnd *self = (struct UIMerchantItemWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    struct UIWindow *status;
    char text[0x80];

    shop_add_title(wnd, MERCHANT_TITLE_WIDTH, MSG_CART_TITLE);

    status = mem_alloc(sizeof(struct UIStaticText));
    if (status)
        status = ui_static_text_construct(status);
    self->status = status;
    ui_window_create(status, 0xdc, 0xf);
    sprintf(text, "Num: %d/%d Weight: %d/%d", g_session1.cartCount, g_session1.cartCountMax, g_session1.cartWeight, g_session1.cartWeightMax);
    ui_static_text_set_text(self->status, text, 0);
    ui_window_move(self->status, 0x10, wnd->height - 0x13);
    ui_window_add_child(wnd, self->status);

    shop_add_resizer(&self->base, MERCHANT_RESIZE_EDGE);
    shop_add_scroll_bar(&self->base, wnd->height - MERCHANT_BOTTOM);
    self->close = ui_window_add_close_button(wnd, "Alt+W", 0);
    merchant_grid(&self->base, width, height);
    ui_merchant_item_wnd_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
    ui_window_set_show(self->status, 0);
}

static void cart_draw_count(struct UIWindow *wnd, int x, int value, int max)
{
    char text[0x20];
    int width;

    sprintf(text, "%d", value);
    ui_window_text_out(wnd, x, wnd->height - 0x13, text, 0, 0, 0xb,
                     value >= max ? MERCHANT_FULL_COLOUR : MERCHANT_COUNT_COLOUR, 0, 0);
    width = ui_window_text_width(wnd, text, 0, 0, 0xb, 0, 0);
    sprintf(text, "/%d", max);
    ui_window_text_out(wnd, x + width + 1, wnd->height - 0x13, text, 0, 0, 0xb, 0, 0, 0);
}

void ui_merchant_item_wnd_on_draw(struct UIWindow *window)
{
    struct UIMerchantItemWnd *self = (struct UIMerchantItemWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;

    ui_item_base_wnd_on_draw((struct UIWindow *)(&self->base));
    ui_window_draw_bitmap(wnd, 0xa, wnd->height - 0x16, ui_skin_bitmap(UI_SKIN "inventory\\icon_num.bmp"), 1);
    cart_draw_count(wnd, 0x17, g_session1.cartCount, g_session1.cartCountMax);
    ui_window_draw_bitmap(wnd, 0x4b, wnd->height - 0x14, ui_skin_bitmap(UI_SKIN "inventory\\icon_weight.bmp"),
                        1);
    cart_draw_count(wnd, 0x5a, g_session1.cartWeight, g_session1.cartWeightMax);
}

void ui_merchant_item_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemWnd *self = (struct UIMerchantItemWnd *)window;
    struct ItemInfo info;

    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->base.frame), x, y);

    ui_item_base_wnd_item_at(&self->base, &info, x, y);
    if ((unsigned)info.index > 0 && !g_windowMgr.merchant_shop_make_wnd && !info.move_pending) {
        struct GameMode *mode = game_current_mode();
        struct ItemInfo cart;

        session_cart_item_by_index(&g_session1, &cart, info.index);
        item_info_assign(&info, &cart);
        item_info_destruct(&cart);
        game_mode_drag_item(mode, DRAG_CART, &info, info.count, -1);
        merchant_link_item(&info);
    }
    item_info_destruct(&info);
}

void ui_merchant_item_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIMerchantItemWnd *self = (struct UIMerchantItemWnd *)window;
    merchant_tooltip(&self->base, x, y, MSG_TIP_CART_OPTIONS);
}

struct UIMerchantMirrorItemWnd *ui_merchant_mirror_item_wnd_construct(struct UIMerchantMirrorItemWnd *self,
                                                                             int variant)
{
    ui_item_base_wnd_construct(&self->base);
    self->base.frame.base.ops = &ui_merchant_mirror_item_wnd_ops;
    self->variant = variant;
    self->base.button_bar = 1;
    self->info = NULL;
    return self;
}

struct UIWindow *ui_merchant_mirror_item_wnd_new(int arg)
{
    return (struct UIWindow *)ui_merchant_mirror_item_wnd_construct(mem_alloc(sizeof(struct UIMerchantMirrorItemWnd)), arg);
}

void ui_merchant_mirror_item_wnd_destruct(struct UIMerchantMirrorItemWnd *self)
{
    self->base.frame.base.ops = &ui_merchant_mirror_item_wnd_ops;
    ui_item_base_wnd_destruct(&self->base);
}

void ui_merchant_mirror_item_wnd_destroy(struct UIWindow *window)
{
    struct UIMerchantMirrorItemWnd *self = (struct UIMerchantMirrorItemWnd *)window;
    ui_merchant_mirror_item_wnd_destruct(self);
    mem_free(self);
}

void ui_merchant_mirror_item_wnd_drag_and_drop(struct UIWindow *window, int x, int y, const struct DragInfo *drag)
{
    struct UIMerchantMirrorItemWnd *self = (struct UIMerchantMirrorItemWnd *)window;
    struct Mode *mode;

    if (g_windowMgr.exchange_wnd) {
        ui_window_drag_and_drop_base(&self->base.frame.base, x, y, drag);
        return;
    }
    mode = game_current_base_mode();
    switch (drag->source) {
    case DRAG_INVENTORY_A:
    case DRAG_BUYING_STORE_SELL:
        if (drag->drop_count != 1) {
            ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 1);
            ui_window_send_msg(g_windowMgr.item_drop_cnt_wnd, NULL, COUNT_MSG_OPEN,
                     self->variant == 2 ? COUNT_FOR_MIRROR_2 : COUNT_FOR_MIRROR, 0, 0, 0);
            return;
        }
    case DRAG_FROM_BUYING_SETUP:
        ui_merchant_mirror_item_wnd_send_msg(&self->base.frame.base, NULL, UI_MSG_ADD_DRAGGED, 0, 0, 0, 0);
        return;
    default:
        mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
        return;
    }
}

void ui_merchant_mirror_item_wnd_store_info(struct UIWindow *window)
{
    struct UIMerchantMirrorItemWnd *self = (struct UIMerchantMirrorItemWnd *)window;
    merchant_store_info(&self->base.frame.base, self->info);
}

void ui_merchant_mirror_item_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIMerchantMirrorItemWnd *self = (struct UIMerchantMirrorItemWnd *)window;
    struct UIWindow *wnd = &self->base.frame.base;
    int msg = self->variant == 1 ? MSG_MIRROR_TITLE_1 : self->variant == 2 ? MSG_MIRROR_TITLE_2 : MSG_MIRROR_TITLE;

    shop_add_title(wnd, MIRROR_TITLE_WIDTH, msg);
    shop_add_resizer(&self->base, MERCHANT_RESIZE_EDGE);
    shop_add_scroll_bar(&self->base, wnd->height - MERCHANT_BOTTOM);
    merchant_grid(&self->base, width, height);
    if (self->variant == 1)
        session_fill_buying_store_list(&g_session1);
    else if (self->variant == 2)
        session_fill_buying_store_list_priced(&g_session1);
    else session_mirror_from_cart(&g_session1);
    ui_merchant_mirror_item_wnd_send_msg(&self->base.frame.base, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
}

void ui_merchant_mirror_item_wnd_on_l_btn_down(struct UIWindow *window, int x, int y)
{
    struct UIMerchantMirrorItemWnd *self = (struct UIMerchantMirrorItemWnd *)window;
    struct ItemInfo info;

    if (y < ITEM_GRID_Y)
        ui_frame_wnd_on_l_btn_down((struct UIWindow *)(&self->base.frame), x, y);

    ui_item_base_wnd_item_at(&self->base, &info, x, y);
    if ((unsigned)info.index > 0) {
        struct GameMode *mode = game_current_mode();
        struct ItemInfo mirror;
        int kind = self->variant == 1 ? DRAG_MIRROR_1 : self->variant == 2 ? DRAG_MIRROR_2 : DRAG_MIRROR;

        session_mirror_item_by_index(&g_session1, &mirror, info.index);
        item_info_assign(&info, &mirror);
        item_info_destruct(&mirror);
        game_mode_drag_item(mode, kind, &info, info.count, -1);
    }
    item_info_destruct(&info);
}

void ui_merchant_mirror_item_wnd_on_mouse_hover(struct UIWindow *window, int x, int y)
{
    struct UIMerchantMirrorItemWnd *self = (struct UIMerchantMirrorItemWnd *)window;
    merchant_tooltip(&self->base, x, y, MSG_TIP_MIRROR_OPTIONS);
}

static void mirror_resize(struct UIMerchantMirrorItemWnd *self, int width, int height)
{
    struct UIItemBaseWnd *base = &self->base;
    struct UIWindow *wnd = &base->frame.base;

    if (width > 0x140)
        width = 0x140;
    if (height > 0xf0)
        height = 0xf0;
    if (width < 0xe8)
        width = 0xe8;
    if (height < 0x78)
        height = 0x78;
    width = (width - 0x118) / ITEM_GRID_CELL * ITEM_GRID_CELL + 0x118;
    height = (height - 0x78) / ITEM_GRID_CELL * ITEM_GRID_CELL + 0x78;
    if (width != wnd->width || height != wnd->height) {
        ui_window_resize_base(wnd, width, height);
        merchant_grid(base, width, height);
        ui_window_move_base(&base->resizer->base, wnd->width - 0xf, wnd->height - 0xf);
        ui_window_move_base(&base->scroll_bar->base, wnd->width - 0xf, 0xe);
        ui_scroll_bar_resize(&base->scroll_bar->base, base->scroll_bar->base.width, wnd->height - MERCHANT_BOTTOM);
        ui_item_base_wnd_update_scroll(base);
    }
    ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
}

static void mirror_add_dragged(struct UIMerchantMirrorItemWnd *self)
{
    struct Mode *mode = game_current_base_mode();
    const struct DragInfo *drag = game_current_drag();
    struct ItemInfo info;

    item_info_construct(&info);
    item_info_assign_from_drag(&info, drag);
    switch (drag->source) {
    case DRAG_INVENTORY_A:
        if (session_get_store_setup_count(&g_session1, drag->index)) {
            session_mirror_add(&g_session1, &info, 0, 1);
            session_remove_store_setup_item(&g_session1, &info, 0);
        }
        break;
    case DRAG_FROM_BUYING_SETUP:
        if (session_mirror_add(&g_session1, &info, self->variant, 1))
            session_remove_store_setup_item(&g_session1, &info, self->variant != 0);
        break;
    case DRAG_BUYING_STORE_SELL:
        session_mirror_add(&g_session1, &info, self->variant, 1);
        session_remove_store_purchase_item(&g_session1, &info);
        if (g_windowMgr.buying_store_item_sell_wnd)
            ui_window_send_msg(g_windowMgr.buying_store_item_sell_wnd, NULL, SHOP_MSG_REFRESH, 0, 0, 0, 0);
        break;
    }
    mode_send_message(mode, MODE_REQ_DRAG_END, 0, 0, 0, 0);
    item_info_destruct(&info);
}

intptr_t ui_merchant_mirror_item_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg,
                                               intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIMerchantMirrorItemWnd *self = (struct UIMerchantMirrorItemWnd *)window;
    switch (msg) {
    case UI_MSG_COMMAND:
        return 0;

    case UI_MSG_RESIZE:
        if (val1 == MERCHANT_RESIZE_EDGE)
            mirror_resize(self, val2, val3);
        return 0;

    case SHOP_MSG_REFRESH: {
        int i, n;

        list_clear(&self->base.items, item_info_destroy);
        n = session_mirror_count(&g_session1);
        for (i = 0; i < n; i++) {
            struct ItemInfo info;

            session_mirror_item_at(&g_session1, &info, i);
            if ((unsigned)info.index > 0)
                item_info_copy(list_push_back(&self->base.items, NULL, sizeof(struct ItemInfo)), &info);
            item_info_destruct(&info);
        }
        ui_window_invalidate_base(&self->base.frame.base);
        ui_item_base_wnd_update_scroll(&self->base);
        return 0;
    }

    case UI_MSG_SET_INFO:
        self->info = (struct WindowInfo *)val1;
        if (!merchant_info_usable(self->info))
            merchant_info_default(self->info);
        ui_merchant_mirror_item_wnd_send_msg(&self->base.frame.base, NULL, UI_MSG_RESIZE, MERCHANT_RESIZE_EDGE, self->info->width, self->info->height, 0);
        ui_frame_wnd_move(&self->base.frame.base, self->info->x, self->info->y);
        return 0;

    case UI_MSG_ADD_DRAGGED:
        mirror_add_dragged(self);
        return 0;
    }
    return ui_item_base_wnd_send_msg((struct UIWindow *)(&self->base), sender, msg, val1, val2, val3, val4);
}
