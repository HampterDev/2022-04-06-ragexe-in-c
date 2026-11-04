#include "ui/controls/edit.h"
#include "render/rp_pool.h"
#include "iteminfo/item_api.h"
#include "ui/window_manager_state.h"
#include "core/object_type_tables.h"
#include "core/object_type.h"
#include "game/context.h"
#include "web/barter_market.h"
#include "web/barter_expanded.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include "ui/items/items.h"
#include "ui/wndmgr/wndmgr.h"
#include "game/gamemode.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "diag/perf.h"
#include "session/session_items.h"

void ui_item_drop_cnt_wnd_destroy(struct UIWindow *window);
void ui_item_drop_cnt_wnd_set_show(struct UIWindow *self, int visible);
void ui_item_drop_cnt_wnd_on_create(struct UIWindow *self, int width, int height);
void ui_item_drop_cnt_wnd_on_draw(struct UIWindow *self);
intptr_t ui_item_drop_cnt_wnd_send_msg(struct UIWindow *self, struct UIWindow *sender, int msg, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4);
const struct UIWindowOps ui_item_drop_cnt_wnd_ops = {
    .destroy = ui_item_drop_cnt_wnd_destroy,
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
    .set_show = ui_item_drop_cnt_wnd_set_show,
    .on_create = ui_item_drop_cnt_wnd_on_create,
    .on_destroy = ui_window_on_destroy_base,
    .on_process = ui_window_on_process_base,
    .on_draw = ui_item_drop_cnt_wnd_on_draw,
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
    .send_msg = ui_item_drop_cnt_wnd_send_msg,
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

#define UI_MSG_CHAT_TAKE_FOCUS 0x36
#define COUNT_MAX_DIGITS 6

static void drop_count_put_count(struct UIItemDropCntWnd *self, struct GameMode *game)
{
    char text[0x20];

    sprintf(text, "%d", game->drag.drop_count);
    ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edit, text);
    ui_window_mgr_set_focus_edit(&g_windowMgr, self->edit);
    ui_edit_ctrl_select_all(self->edit);
}

static void drop_count_item_name(struct GameMode *game, struct String *out)
{
    const char *id_text = game_mode_count_item_id_text(game);
    struct ItemInfo item;

    if (!id_text) {
        string_clear(out);
        return;
    }
    item_info_construct(&item);
    string_assign_cstr(&item.id_text, id_text);
    item.refine = 0;
    item.identified = game->drag.identified;
    item_info_get_display_name(&item, out);
    item_info_destruct(&item);
}

struct UIItemDropCntWnd *ui_item_drop_cnt_wnd_construct(struct UIItemDropCntWnd *self)
{
    ui_frame_wnd_construct(&self->frame, 0);
    self->frame.base.ops = &ui_item_drop_cnt_wnd_ops;
    return self;
}

struct UIWindow *ui_item_drop_cnt_wnd_new(void)
{
    return (struct UIWindow *)ui_item_drop_cnt_wnd_construct(mem_alloc(sizeof(struct UIItemDropCntWnd)));
}

void ui_item_drop_cnt_wnd_destruct(struct UIItemDropCntWnd *self)
{
    self->frame.base.ops = &ui_item_drop_cnt_wnd_ops;
    ui_frame_wnd_destruct(&self->frame);
}

void ui_item_drop_cnt_wnd_destroy(struct UIWindow *window)
{
    struct UIItemDropCntWnd *self = (struct UIItemDropCntWnd *)window;
    ui_item_drop_cnt_wnd_destruct(self);
    mem_free(self);
}

void ui_item_drop_cnt_wnd_set_show(struct UIWindow *window, int visible)
{
    struct UIItemDropCntWnd *self = (struct UIItemDropCntWnd *)window;
    struct UIWindow *wnd = &self->frame.base;

    wnd->visible = visible;
    if (visible) {
        struct GameMode *game = game_current_mode();
        struct String name = {0};

        drop_count_put_count(self, game);
        drop_count_item_name(game, &name);
        ui_static_text_set_text(&self->name->base, string_cstr(&name), 0);
        ui_window_invalidate_base(wnd);
        ui_window_mgr_update_snap_rect(&g_windowMgr, wnd);
        string_free(&name);
        return;
    }
    if (ui_window_mgr_get_focus_edit(&g_windowMgr) == self->edit && g_windowMgr.new_chat_wnd)
        ui_window_send_msg(g_windowMgr.new_chat_wnd, NULL, UI_MSG_CHAT_TAKE_FOCUS, 0, 0, 0, 0);
    ui_window_mgr_remove_snap_rect(&g_windowMgr, wnd);
}

void ui_item_drop_cnt_wnd_on_create(struct UIWindow *window, int width, int height)
{
    struct UIItemDropCntWnd *self = (struct UIItemDropCntWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct GameMode *game;
    struct String name = {0};
    struct UIWindow *edit, *text;

    (void)width;
    (void)height;
    edit = mem_alloc(sizeof(struct UIEditCtrl));
    if (edit)
        edit = ui_edit_ctrl_construct(edit);
    self->edit = edit;
    ui_window_create(edit, 0x50, 0x10);
    ui_window_move(self->edit, 0xf, 0x16);
    UI_EDIT_MAX_CHARS(self->edit) = COUNT_MAX_DIGITS;
    ui_edit_ctrl_set_back_colour(self->edit, 0xe8, 0xe8, 0xe8);
    ui_window_add_child(wnd, self->edit);

    game = game_current_mode();
    drop_count_put_count(self, game);
    drop_count_item_name(game, &name);

    text = mem_alloc(sizeof(struct UIStaticText));
    if (text)
        text = ui_static_text_construct(text);
    self->name = (struct UIStaticText *)text;
    ui_window_create(text, 0x64, 0xf);
    ui_window_move_base(&self->name->base, 0xf, 6);
    ui_static_text_set_text(&self->name->base, string_cstr(&name), 0);
    ui_window_add_child(wnd, &self->name->base);

    ui_add_skin_button(wnd, "btn_ok", wnd->width - 0x37, wnd->height - 0x20, UI_ID_OK);
    UI_FRAME_DEFAULT_ID(&self->frame) = UI_ID_OK;
    wnd->state = 0;
    string_free(&name);
}

void ui_item_drop_cnt_wnd_on_draw(struct UIWindow *window)
{
    struct UIItemDropCntWnd *self = (struct UIItemDropCntWnd *)window;
    ui_window_draw_sys_box(&self->frame.base, 0, 0, 0, 0);
}

#define DROP_MSG_SET_PURPOSE 0xd
#define DROP_MSG_SET_NAME 0x1d
#define DROP_PURPOSE_SET_COUNT 0x14
#define MODE_MSG_DRAG_DONE 0x12
#define MODE_MSG_DROP_COUNT 0x15
#define UI_MSG_ADD_DRAGGED 0x26
#define COUNT_CAP 30000
#define MSG_COUNT_CAPPED 0x6a8
#define MSG_COUNT_TOO_MANY 0x327
#define UIWM_MSG_SHOW_CHAT 3
#define CHAT_RED 0xff
#ifndef WND_ID_COUNT_INPUT
#define WND_ID_COUNT_INPUT 0xf
#endif

#define WND_ID_PARA_SHOP 0xfe
#define WND_ID_PARA_PURCHASE 0xff
#define WND_ID_BARTER_CART 0x14e
#define WND_ID_BARTER_GOODS 0x14f
#define WND_ID_EX_BARTER_CART 0x155
#define WND_ID_EX_BARTER_GOODS 0x156

typedef struct ItemInfo *(*SessionItemGetter)(struct Session *, struct ItemInfo *, int);

static int session_item_count(SessionItemGetter get, int index)
{
    struct ItemInfo item;
    int count;

    get(&g_session1, &item, index);
    count = item.index ? item.count : 0;
    item_info_destruct(&item);
    return count;
}

static int inventory_count(int index)
{
    struct ItemInfo item;
    int count;

    count = session_get_inventory_item(&g_session1, &item, index)->count;
    item_info_destruct(&item);
    return count;
}

static void drag_done(struct GameMode *game)
{
    mode_send_message((struct Mode *)game, MODE_MSG_DRAG_DONE, 0, 0, 0, 0);
}

static void add_dragged(struct UIWindow *wnd, int val1)
{
    if (wnd)
        ui_window_send_msg(wnd, NULL, UI_MSG_ADD_DRAGGED, val1, 0, 0, 0);
}

static void chat_red(const char *text)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_SHOW_CHAT, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIWM_MSG_CHAT_LINE, (intptr_t)text, CHAT_RED, 0, 0);
}

static struct UIWindow *para_window(int id, enum ObjectType type)
{
    return object_as_type(ui_window_find(id), type);
}

static void count_to_window(struct GameMode *game, int id, int count)
{
    struct UIWindow *wnd = ui_window_find(id);

    if (wnd)
        ui_window_send_msg(wnd, NULL, UI_MSG_ADD_DRAGGED, count, 0, 0, 0);
    else
        drag_done(game);
}

static void cap_count(struct GameMode *game, int count, int limit)
{
    if (limit)
        game->drag.drop_count = count <= limit ? count : limit;
}

intptr_t ui_item_drop_cnt_wnd_send_msg(struct UIWindow *window, struct UIWindow *sender, int msg, intptr_t val1,
                                         intptr_t val2, intptr_t val3, intptr_t val4)
{
    struct UIItemDropCntWnd *self = (struct UIItemDropCntWnd *)window;
    struct UIWindow *wnd = &self->frame.base;
    struct GameMode *game;
    struct String text;
    char buf[0xc8];
    int count, n;

    switch (msg) {
    case UI_MSG_COMMAND:
        break;
    case DROP_MSG_SET_PURPOSE:
        wnd->state = val1;
        if (val1 != DROP_PURPOSE_SET_COUNT)
            return 0;
        sprintf(buf, "%d", (int)val2);
        ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edit, buf);
        if (wnd->state == DROP_PURPOSE_SET_COUNT) {
            string_init(&text);
            string_format(&text, "%d", (int)val2);
            ui_edit_ctrl_set_text((struct UIEditCtrl *)self->edit, string_cstr(&text));
            string_free(&text);
        }
        return 0;
    case DROP_MSG_SET_NAME:
        if (val2) {
            ui_static_text_set_text(&self->name->base, (const char *)val2, 0);
            ui_window_invalidate_base(wnd);
        }
        return 0;
    default:
        return ui_frame_wnd_send_msg((struct UIWindow *)(&self->frame), sender, msg, val1, val2, val3, val4);
    }

    if (val1 != UI_ID_OK || !ui_edit_ctrl_get_text_size(self->edit))
        return 0;
    game = game_current_mode();
    count = atoi(ui_edit_ctrl_get_text(self->edit));

    switch (wnd->state) {
    case 0:
        if (count <= 0 || count > game->drag.drop_count)
            goto refuse;
        mode_send_message((struct Mode *)game, MODE_MSG_DROP_COUNT, count, 0, 0, 0);
        break;
    case 1:
        if (count <= 0)
            goto refuse;
        if (count > COUNT_CAP)
            count = COUNT_CAP;
        game->drag.drop_count = count;
        if (count == COUNT_CAP) {
            sprintf(buf, msg_str(MSG_COUNT_CAPPED), COUNT_CAP);
            chat_red(buf);
        }
        add_dragged(g_windowMgr.item_purchase_wnd, 0);
        break;
    case 2:
    case 0x10:
        n = session_get_shop_goods_count(&g_session1, game->drag.index);
        if (!n)
            goto refuse;
        if (count <= 0)
            drag_done(game);
        game->drag.drop_count = count > n ? n : count;
        if (wnd->state == 2)
            add_dragged(g_windowMgr.item_sell_wnd, 0);
        else if (wnd->state == 0x10)
            add_dragged(g_windowMgr.item_sell_wnd2, 0);
        break;
    case 3:
        n = session_get_shop_buy_count_by_name(&g_session1, string_cstr(&game->drag.name));
        if (!n)
            goto refuse;
        if (count <= 0)
            drag_done(game);
        game->drag.drop_count = count > n ? n : count;
        add_dragged(g_windowMgr.item_shop_wnd, 0);
        break;
    case 4:
    case 0xf:
        n = session_get_shop_sell_count(&g_session1, game->drag.index);
        if (!n)
            goto refuse;
        if (count <= 0)
            drag_done(game);
        game->drag.drop_count = count > n ? n : count;
        if (wnd->state == 4)
            add_dragged(g_windowMgr.item_shop_wnd, 1);
        else if (wnd->state == 0xf)
            add_dragged(g_windowMgr.item_shop_wnd2, 1);
        break;
    case 5:
        if (!session_has_inventory_index(&g_session1, game->drag.index))
            goto refuse;
        n = inventory_count(game->drag.index);
        if (count <= 0)
            drag_done(game);
        cap_count(game, count, n);
        add_dragged(g_windowMgr.exchange_wnd, 0);
        break;
    case 6:
        if (count <= 0)
            drag_done(game);
        n = 0;
        if (game->drag.source == 6) {
            if (session_get_storage_item_count(&g_session1, game->drag.index)) {
                struct ItemInfo item;

                n = session_get_storage_item(&g_session1, &item, game->drag.index)->count;
                item_info_destruct(&item);
            }
        } else if (game->drag.source == 9) {
            if (g_windowMgr.merchant_shop_make_wnd)
                break;
            if (session_get_cart_item_count(&g_session1, game->drag.index)) {
                struct ItemInfo item;

                n = session_cart_item_by_index(&g_session1, &item, game->drag.index)->count;
                item_info_destruct(&item);
            }
        } else if (game->drag.source == 0xf) {
            if (game->drag.drop_count)
                n = game->drag.drop_count;
        }
        cap_count(game, count, n);
        add_dragged(g_windowMgr.item_wnd, 0);
        break;
    case 7:
        n = 0;
        if (game->drag.source == 9)
            n = session_item_count(session_cart_item_by_index, game->drag.index);
        else if (game->drag.source == 1 && session_has_inventory_index(&g_session1, game->drag.index))
            n = inventory_count(game->drag.index);
        if (!n)
            break;
        if (count <= 0)
            count = 1;
        game->drag.drop_count = count > n ? n : count;
        add_dragged(g_windowMgr.item_store_wnd, 0);
        break;
    case 8:
        n = 0;
        if (game->drag.source == 1)
            n = session_item_count(session_get_inventory_item, game->drag.index);
        else if (game->drag.source == 6)
            n = session_item_count(session_get_storage_item, game->drag.index);
        if (!n)
            break;
        if (count <= 0)
            count = 1;
        game->drag.drop_count = count > n ? n : count;
        add_dragged(g_windowMgr.merchant_item_wnd, 0);
        break;
    case 9:
        n = game->drag.source == 0xb ? session_item_count(session_mirror_item_by_index, game->drag.index) : 0;
        if (!n)
            break;
        if (count <= 0)
            count = 1;
        game->drag.drop_count = count > n ? n : count;
        add_dragged(g_windowMgr.merchant_shop_make_wnd, 0);
        break;
    case 10:
        n = game->drag.source == 0xa ? session_item_count(session_get_store_setup_item_by_index, game->drag.index) : 0;
        if (!n)
            break;
        if (count <= 0)
            count = 1;
        game->drag.drop_count = count > n ? n : count;
        add_dragged(g_windowMgr.merchant_mirror_item_wnd, 0);
        break;
    case 11:
        n = game->drag.source == 0xd
                ? session_get_store_purchase_count_by_name(&g_session1, string_cstr(&game->drag.name))
                : 0;
        if (!n)
            break;
        if (count <= 0)
            count = 1;
        game->drag.drop_count = count > n ? n : count;
        add_dragged(g_windowMgr.merchant_item_shop_wnd, 0);
        break;
    case 12:
        if (count <= 0)
            drag_done(game);
        game->drag.drop_count = count;
        add_dragged(g_windowMgr.merchant_item_purchase_wnd, 0);
        break;
    case 13:
        if (!session_has_inventory_index(&g_session1, game->drag.index))
            goto refuse;
        n = inventory_count(game->drag.index);
        if (count <= 0)
            drag_done(game);
        cap_count(game, count, n);
        add_dragged(g_windowMgr.rodex_compose_wnd, 0);
        break;
    case 0x11:
        n = game->drag.source == 0x16 ? session_item_count(session_mirror_item_by_index, game->drag.index) : 0;
        if (!n)
            break;
        if (count <= 0)
            count = 1;
        game->drag.drop_count = count > n ? n : count;
        add_dragged(g_windowMgr.buying_store_item_sell_wnd, 0);
        break;
    case 0x12:
        n = game->drag.source == 0x15 ? session_item_count(session_get_store_purchase_item_by_index, game->drag.index) : 0;
        if (!n)
            break;
        if (count <= 0)
            count = 1;
        game->drag.drop_count = count > n ? n : count;
        add_dragged(g_windowMgr.buying_store_mirror_sell_list_wnd, 0);
        break;
    case 0x15:
        if (count <= 0)
            goto refuse;
        n = count > COUNT_CAP ? COUNT_CAP : count;
        game->drag.drop_count = n;
        if (n == COUNT_CAP) {
            memset(buf, 0, sizeof buf);
            snprintf(buf, sizeof buf, msg_str(MSG_COUNT_CAPPED), COUNT_CAP);
            chat_red(buf);
        }
        add_dragged(para_window(WND_ID_PARA_PURCHASE, OBJECT_PARA_PURCHASE_WINDOW), count);
        break;
    case 0x16:
        n = session_get_shop_buy_count_by_name(&g_session1, string_cstr(&game->drag.name));
        if (!n)
            goto refuse;
        if (count <= 0)
            drag_done(game);
        game->drag.drop_count = count > n ? n : count;
        add_dragged(para_window(WND_ID_PARA_SHOP, OBJECT_PARA_SHOP_WINDOW), 0);
        break;
    case 0x17:
        n = barter_cart_count(g_barter, game->drag.shop_index);
        if (n <= 0)
            goto refuse;
        game->drag.drop_count = count > n ? n : count;
        add_dragged(ui_window_find(WND_ID_BARTER_CART), 0);
        break;
    case 0x18:
        if (count <= 0)
            goto refuse;
        if (count > COUNT_CAP)
            goto too_many;
        n = barter_goods_count(g_barter, game->drag.shop_index);
        cap_count(game, count, n == -1 ? COUNT_CAP : n);
        count_to_window(game, WND_ID_BARTER_GOODS, count);
        break;
    case 0x19:
        n = expanded_barter_cart_count(g_expandedBarter, game->drag.shop_index);
        if (n <= 0)
            goto refuse;
        game->drag.drop_count = count > n ? n : count;
        add_dragged(ui_window_find(WND_ID_EX_BARTER_CART), 0);
        break;
    case 0x1a:
        if (count <= 0)
            goto refuse;
        if (count > COUNT_CAP)
            goto too_many;
        n = expanded_barter_goods_count(g_expandedBarter, game->drag.shop_index);
        cap_count(game, count, n == -1 ? COUNT_CAP : n);
        count_to_window(game, WND_ID_EX_BARTER_GOODS, count);
        break;
    default:
        break;
    }
    goto done;

too_many:
    chat_red(msg_str(MSG_COUNT_TOO_MANY));
refuse:
    drag_done(game);
done:
    drag_done(game);
    ui_window_mgr_show_option_window(&g_windowMgr, WND_ID_COUNT_INPUT, 0);
    return 0;
}
