#include "iteminfo/item_api.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include "platform/win_types.h"
#include "game/mode_msg.h"
#include "session/session_items.h"
#include "session/session_skills.h"
#include "ui/items/refining_internal.h"
#include "ui/wndmgr/wndmgr.h"
#include "platform/clock.h"

#define CZ_REQ_TAKEOFF_EQUIP    0xab
#define CZ_ITEM_THROW           0x363
#define CZ_USE_ITEM             0x439
#define CZ_REQ_WEAR_EQUIP_V5    0x998

#define ACK_WAIT 2000

enum {
    DRAG_FROM_INVENTORY = 1,
    DRAG_FROM_EQUIP = 2,
    DRAG_FROM_SHORTCUT = 7,
    DRAG_FROM_QUICK_SLOT = 0x10,
    DRAG_FROM_REFINING = 0x1c,
};

enum {
    WID_DROP_COUNT = 0xf,
    WID_SHORTCUT = 0x24,
    WID_YOUR_ITEM = 0x8a,
    WID_QUICK_SLOT = 0x9f,
    WID_BARTER_MARKET = 0x14e,
};

static int refuse_pressed(void)
{
    chat_print_msg(0x6a3, CHAT_COLOR_RED);
    return 0;
}

int game_mode_msg_cancel_drag(struct GameMode *self)
{
    self->drag.source = 0;
    self->drag.index = 0;
    self->drag.drop_count = 0;
    ui_window_mgr_show_option_window(&g_windowMgr, WID_DROP_COUNT, 0);
    return 0;
}

static int cancel_drag(struct GameMode *self)
{
    self_msg(self, MODE_MSG_CANCEL_DRAG, 0, 0, 0, 0);
    return 0;
}

static int drop_refused(struct GameMode *self)
{
    cancel_drag(self);
    if (g_windowMgr.equip_wnd) {
        chat_refresh();
        chat_print_msg(0xbd, CHAT_COLOR_RED);
    }
    if (g_windowMgr.rodex_compose_wnd) {
        chat_refresh();
        chat_print_msg(0xab0, CHAT_COLOR_RED);
    }
    ui_window_mgr_close_drag_plate(&g_windowMgr);
    return 0;
}

static int drop_named(struct GameMode *self, int x, int y)
{
    struct ItemInfo item;
    struct UIWindow *w;

    if (self->drag.named) {
        if (self->drag.source == DRAG_FROM_QUICK_SLOT)
            w = ui_window_find(WID_QUICK_SLOT);
        else if (self->drag.source == DRAG_FROM_SHORTCUT)
            w = ui_window_find(WID_SHORTCUT);
        else
            return cancel_drag(self);
        if (w) {
            w->ops->drag_and_drop(w, x, y, &self->drag);
        }
    } else {
        session_get_inventory_item(&g_session1, &item, self->drag.index);
        if (item.index != 0) {
            chat_refresh();
            chat_print_msg(0x6a0, CHAT_COLOR_RED);
        }
        item_info_destruct(&item);
    }
    return cancel_drag(self);
}

int game_mode_msg_drop_dragged(struct GameMode *self, int x, int y)
{
    struct ItemInfo item;
    struct UIWindow *w;
    int source = self->drag.source;

    if (!source)
        return 0;
    if (source == DRAG_FROM_INVENTORY) {
        if (!session_has_inventory_index(&g_session1, self->drag.index)
            || self->wear_ack_time + ACK_WAIT > platform_ticks_ms()) {
            self->drag.source = 0;
            return 0;
        }
    } else if (source == DRAG_FROM_EQUIP) {
        if (!session_is_worn_index(&g_session1, self->drag.index)) {
            self->drag.source = 0;
            return 0;
        }
    }
    if (ui_window_mgr_drop_on(&g_windowMgr, x, y, &self->drag))
        return 0;

    source = self->drag.source;
    if (source == DRAG_FROM_QUICK_SLOT || source == DRAG_FROM_SHORTCUT)
        return drop_named(self, x, y);
    if (source == DRAG_FROM_INVENTORY) {
        if (!g_windowMgr.item_wnd) {
            chat_print_msg(0x2b2, CHAT_COLOR_RED);
            return cancel_drag(self);
        }
        if (g_session1.itemDropLock) {
            self->drag.source = 0;
            return 0;
        }
    }
    if (source == DRAG_FROM_REFINING) {
        w = ui_window_find(WID_REFINE);
        if (w)
            ui_refining_wnd_reset((struct UIRefiningWnd *)w);
        cancel_drag(self);
    }

    if (self->npc_dialog_open || g_windowMgr.exchange_wnd || g_windowMgr.equip_wnd || g_windowMgr.item_store_wnd || g_windowMgr.rodex_compose_wnd
        || ui_window_find(WID_REFINE) || ui_window_find(WID_BARTER_MARKET))
        return drop_refused(self);

    if (self->drag.source != DRAG_FROM_INVENTORY) {
        cancel_drag(self);
        ui_window_mgr_close_drag_plate(&g_windowMgr);
        return 0;
    }
    if (g_session1.wornArrowIndex == self->drag.index) {
        session_get_inventory_item(&g_session1, &item, self->drag.index);
        if (item.index != 0) {
            self_msg(self, MODE_MSG_CANCEL_DRAG, 0, 0, 0, 0);
            message_box_msg(0x291);
            item_info_destruct(&item);
            return 0;
        }
        item_info_destruct(&item);
    }
    if (self->drag.drop_count == 1) {
        self_msg(self, MODE_MSG_THROW_ITEM, self->drag.index, 1, 0, 0);
        ui_window_mgr_close_drag_plate(&g_windowMgr);
        return 0;
    }
    ui_window_mgr_show_option_window(&g_windowMgr, WID_DROP_COUNT, 1);
    ui_window_send(g_windowMgr.item_drop_cnt_wnd, NULL, 0xd, 0, 0, 0, 0);
    ui_window_mgr_close_drag_plate(&g_windowMgr);
    return 0;
}

int game_mode_msg_throw_dragged(struct GameMode *self, int count)
{
    self_msg(self, MODE_MSG_THROW_ITEM, self->drag.index, count, 0, 0);
    return 0;
}

int game_mode_msg_throw_item(struct GameMode *self, int index, int count)
{
#pragma pack(push, 1)
    struct { unsigned short id, index, count; } packet;
#pragma pack(pop)

    if (self->wear_ack_time + ACK_WAIT <= platform_ticks_ms()
        && !self->use_item_pending && !self->throw_item_pending && index != 0 && !self->player_dead) {
        self->throw_item_pending = 1;
        packet.index = (unsigned short)index;
        packet.count = (unsigned short)count;
        packet.id = CZ_ITEM_THROW;
        send_packet(CZ_ITEM_THROW, &packet);
    }
    return cancel_drag(self);
}

int game_mode_msg_wear_equip(struct GameMode *self, int index, int location)
{
#pragma pack(push, 1)
    struct { unsigned short id, index; int location; } packet;
#pragma pack(pop)
    struct ItemInfo worn, item;
    int slots[3], unused_slots[3];
    struct UIWindow *w;

    if (g_windowMgr.exchange_wnd)
        return refuse_pressed();
    if (g_windowMgr.bartermarket_wnd || g_windowMgr.expanded_bartermarket_wnd) {
        chat_print_msg(0xe68, CHAT_COLOR_RED);
        return 0;
    }
    if (self->wear_ack_time + ACK_WAIT > platform_ticks_ms())
        return 0;
    if (self->player_dead)
        return 0;
    session_equip_slots_of(slots, location);
    session_get_equipped_item(&g_session1, &worn, slots[2]);
    if (worn.damaged) {
        chat_print_msg(0x2bb, CHAT_COLOR_RED);
        item_info_destruct(&worn);
        return 0;
    }
    if (worn.index != 0 && worn.count == 1)
        session_equip_slots_of(unused_slots, worn.worn_location);
    session_get_inventory_item(&g_session1, &item, index);
    if (item.bind_on_equip_type == 1) {
        w = ui_window_open(WID_YOUR_ITEM);
        self->wear_ack_time = platform_ticks_ms();
        ui_window_send(w, NULL, 0x5b, index, location, 0, 0);
    } else {
        self->wear_ack_time = platform_ticks_ms();
        packet.id = CZ_REQ_WEAR_EQUIP_V5;
        packet.index = (unsigned short)index;
        packet.location = location;
        send_packet(CZ_REQ_WEAR_EQUIP_V5, &packet);
    }
    item_info_destruct(&item);
    item_info_destruct(&worn);
    return 0;
}

int game_mode_msg_wear_equip_direct(struct GameMode *self, int index, int location)
{
#pragma pack(push, 1)
    struct { unsigned short id, index; int location; } packet;
#pragma pack(pop)

    if (self->wear_ack_time + ACK_WAIT > platform_ticks_ms())
        return 0;
    if (self->player_dead)
        return 0;
    self->wear_ack_time = platform_ticks_ms();
    packet.id = CZ_REQ_WEAR_EQUIP_V5;
    packet.index = (unsigned short)index;
    packet.location = location;
    send_packet(CZ_REQ_WEAR_EQUIP_V5, &packet);
    return 0;
}

int game_mode_msg_takeoff_equip(struct GameMode *self, int index, int second_list)
{
#pragma pack(push, 1)
    struct { unsigned short id, index; } packet;
#pragma pack(pop)
    struct ItemInfo item, found;

    if (g_windowMgr.exchange_wnd)
        return refuse_pressed();
    if (self->takeoff_ack_time + ACK_WAIT > platform_ticks_ms())
        return 0;
    if (self->player_dead)
        return 0;
    item_info_construct_with_arg(&item);
    if (second_list)
        item_info_assign(&item, session_get_worn_costume(&g_session1, &found, index));
    else
        item_info_assign(&item, session_get_equipped_item(&g_session1, &found, index));
    item_info_destruct(&found);
    if ((unsigned)item.index == 0 || item.count <= 0) {
        item_info_destruct(&item);
        return 0;
    }
    self->takeoff_ack_time = platform_ticks_ms();
    packet.id = CZ_REQ_TAKEOFF_EQUIP;
    packet.index = (unsigned short)item.index;
    send_packet(CZ_REQ_TAKEOFF_EQUIP, &packet);
    item_info_destruct(&item);
    return 1;
}

int game_mode_msg_use_item(struct GameMode *self, int index)
{
#pragma pack(push, 1)
    struct { unsigned short id, index; int aid; } packet;
#pragma pack(pop)
    struct ItemInfo item;
    struct Player *player;
    int state, mode;

    if (g_windowMgr.exchange_wnd)
        return refuse_pressed();
    if (self->use_item_pending || self->throw_item_pending || index == 0 || self->player_dead)
        return 0;
    session_get_inventory_item(&g_session1, &item, index);
    player = self->world->player;
    state = actor_get_effect_state(player);
    mode = self->world->player->body_state;
    if (is_effect_state_burrow(state))
        goto done;
    if ((mode == 4 || mode == 1) && item_info_get_item_id(&item) != 0x31b5)
        goto done;
    if (self->world->player->state == 6) {
        if (item_info_get_item_id(&item) == 0x259 || item_info_get_item_id(&item) == 0x25a
            || item_info_get_item_id(&item) == 0x263 || item_info_get_item_id(&item) == 0x25d
            || item_info_get_item_id(&item) == 0x25e || item_info_get_item_id(&item) == 0x5813) {
            chat_print_msg(0x297, CHAT_COLOR_RED);
            goto done;
        }
    }
    self->use_item_pending = 1;
    packet.id = CZ_USE_ITEM;
    packet.aid = g_session1.ownAid;
    packet.index = (unsigned short)index;
    send_packet(CZ_USE_ITEM, &packet);
done:
    item_info_destruct(&item);
    return 0;
}
