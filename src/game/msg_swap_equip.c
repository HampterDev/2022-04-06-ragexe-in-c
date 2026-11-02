#include "iteminfo/item_api.h"
#include "ui/window_manager_state.h"
#include "platform/win_types.h"
#include "game/mode_msg.h"
#include "game/item_info.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "platform/clock.h"

#define CZ_REQ_ADD_SWAP_EQUIP       0xa97
#define CZ_REQ_REMOVE_SWAP_EQUIP    0xa99
#define CZ_REQ_DO_SWAP_EQUIP        0xa9c
#define CZ_REQ_SWAP_EQUIP_1_ON_1    0xace
#define CZ_REQ_LIMITED_SALE_LIST    0xb4c

#define SWAP_WAIT       10000
#define SALE_LIST_WAIT  5000
#define COLOR_SWAP_WAIT 0xc8ff00

static int swap_refused(struct GameMode *self)
{
    int id;

    if (!self->world || !self->world->player)
        return 1;
    if (g_windowMgr.exchange_wnd)
        id = 0xbb9;
    else if (self->player_dead)
        id = 0xbb7;
    else if (g_windowMgr.merchant_item_my_shop_wnd || g_windowMgr.merchant_item_purchase_wnd || g_windowMgr.merchant_shop_make_wnd)
        id = 0xbba;
    else if (self->world->player->recharge_gage_wnd)
        id = 0xbb8;
    else if (g_windowMgr.say_dialog_wnd || g_windowMgr.choose_wnd)
        id = 0xbbb;
    else
        return 0;
    chat_print_msg(id, CHAT_COLOR_RED);
    return 1;
}

static void slot_item(struct ItemInfo *info, unsigned location, int slot)
{
    struct ItemInfo tmp;

    if (session_is_costume_or_shadow_location(&g_session1, (int)location))
        item_info_assign(info, session_get_switch_costume(&g_session1, &tmp, slot));
    else
        item_info_assign(info, session_get_switch_equip(&g_session1, &tmp, slot));
    item_info_destruct(&tmp);
}

int game_mode_msg_add_swap_equip(struct GameMode *self, int index, unsigned location)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short index;
        unsigned location;
    } packet;
#pragma pack(pop)
    struct ItemInfo item, info;
    int packed[3], slots[3], i;

    if (game_mode_is_blocking_window_open() == 1)
        return 0;
    if (swap_refused(self))
        return 0;
    session_find_item_for_switch(&g_session1, &item, index, location);
    if (!item.identified || item.damaged) {
        item_info_destruct(&item);
        return 0;
    }
    if ((unsigned)item.index > 0
            && (item.type == 0xa || item.type == 0x10 || item.type == 0x11 || item.type == 0x13)) {
        if (index == g_session1.switchArrowIndex) {
            item_info_destruct(&item);
            return 0;
        }
    } else {
        session_equip_slots_of(packed, location);
        slots[0] = packed[2];
        slots[1] = packed[1];
        slots[2] = packed[0];
        for (i = 0; i < 3; i++) {
            item_info_construct(&info);
            slot_item(&info, location, slots[i]);
            if (index == info.index) {
                item_info_destruct(&info);
                item_info_destruct(&item);
                return 0;
            }
            item_info_destruct(&info);
        }
    }
    self->wear_ack_time = platform_ticks_ms();
    packet.id = CZ_REQ_ADD_SWAP_EQUIP;
    packet.index = (unsigned short)index;
    packet.location = location;
    send_packet(CZ_REQ_ADD_SWAP_EQUIP, &packet);
    item_info_destruct(&item);
    return 0;
}

int game_mode_msg_remove_swap_equip(struct GameMode *self, int slot, int swap_set)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short index;
    } packet;
#pragma pack(pop)
    struct ItemInfo info, tmp;

    if (swap_refused(self))
        return 0;
    item_info_construct_with_arg(&info);
    if (swap_set)
        item_info_assign(&info, session_get_switch_costume(&g_session1, &tmp, slot));
    else
        item_info_assign(&info, session_get_switch_equip(&g_session1, &tmp, slot));
    item_info_destruct(&tmp);
    if (info.index != 0 && info.count > 0) {
        packet.index = (unsigned short)info.index;
        packet.id = CZ_REQ_REMOVE_SWAP_EQUIP;
        send_packet(CZ_REQ_REMOVE_SWAP_EQUIP, &packet);
    }
    item_info_destruct(&info);
    return 0;
}

static void print_wait(unsigned sent_at, unsigned wait_s, int channel)
{
    struct String line;
    const char *text = msg_str(0xc9e);
    unsigned now = platform_ticks_ms();

    string_init(&line);
    string_format(&line, "%d%s", wait_s - (now - sent_at) / 1000, text);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)string_cstr(&line), COLOR_SWAP_WAIT, channel, 0);
    string_free(&line);
}

int game_mode_msg_do_swap_equip(void)
{
    unsigned sent_at = g_session1.equipSwapSentAt;

    if (platform_ticks_ms() - sent_at < SWAP_WAIT) {
        print_wait(g_session1.equipSwapSentAt, SWAP_WAIT / 1000, 6);
        return 0;
    }
    g_session1.equipSwapSentAt = platform_ticks_ms();
    send_id(CZ_REQ_DO_SWAP_EQUIP);
    return 0;
}

int game_mode_msg_swap_equip1_on1(const struct ItemInfo *item)
{
    unsigned sent_at = g_session1.equipSwapSentAt;

    if (platform_ticks_ms() - sent_at < SWAP_WAIT) {
        print_wait(g_session1.equipSwapSentAt, SWAP_WAIT / 1000, 6);
        return 0;
    }
    send_u16(CZ_REQ_SWAP_EQUIP_1_ON_1, (unsigned short)item->index);
    return 0;
}

int game_mode_msg_limited_sale_list(void)
{
    unsigned sent_at = g_session1.saleListSentAt;

    if (platform_ticks_ms() - sent_at < SALE_LIST_WAIT) {
        print_wait(g_session1.saleListSentAt, SALE_LIST_WAIT / 1000, 0);
        return 0;
    }
    g_session1.saleListSentAt = platform_ticks_ms();
    send_id(CZ_REQ_LIMITED_SALE_LIST);
    return 0;
}
