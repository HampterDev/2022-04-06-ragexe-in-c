#include "game/packet_handlers.h"
#include "ui/chat/balloon.h"
#include "iteminfo/item_api.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include "game/packets_inventory.h"
#include "ui/window.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "game/skill_use.h"
#include "game/item_list_packets.h"
#include "session/session_items.h"
#include "platform/clock.h"

#pragma pack(push, 1)
struct PACKET_ZC_ACK_MERGE_ITEM {
    unsigned short id;
    unsigned short index;
    unsigned short count;
    unsigned char result;
};
struct PACKET_ZC_EXTEND_BODYITEM_SIZE {
    unsigned short id;
    short size;
};
struct PACKET_ZC_ACK_EXTEND_BODYITEM_SIZE {
    unsigned short id;
    unsigned char result;
};
struct PACKET_ZC_ACK_OPEN_MSGBOX_EXTEND_BODYITEM_SIZE {
    unsigned short id;
    unsigned char result;
    int item_id;
};
#pragma pack(pop)

enum { UIM_EQUIP_REFRESH = 0x6a, UIM_INVENTORY_REFRESH = 0x17,
       UIM_MERGE_ITEM_DONE = 0x3c };
enum {
    WID_MERGE_ITEM = 0xe3,
};
enum {
    MM_EXTEND_BODYITEM_ACCEPT = 0x137,
    MM_EXTEND_BODYITEM_REFUSE = 0x138,
};
#define MSGBOX_YES 0xb3

#define REFINING_NOTICE_COLOR 0x62b0ff

void game_mode_zc_ack_take_off_equip_all(struct GameMode *self, unsigned result)
{
    (void)self;
    if ((unsigned char)((unsigned char)result - 1) <= 1) {
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(0xf74), 0xff, 0, 0);
        if (g_windowMgr.equip_wnd != 0)
            ui_window_send(g_windowMgr.equip_wnd, NULL, UIM_EQUIP_REFRESH, 0x237, 0, 0, 0);
    }
    if (g_windowMgr.equip_wnd != 0)
        ui_window_send(g_windowMgr.equip_wnd, NULL, UIM_EQUIP_REFRESH, 0x237, 0, 0, 0);
}

void game_mode_zc_extend_body_item_size(struct GameMode *self,
                                                 const struct PACKET_ZC_EXTEND_BODYITEM_SIZE *packet)
{
    (void)self;
    if (packet->size > 100)
        return;
    g_session1.inventoryExtension = packet->size;
    if (g_windowMgr.item_wnd != 0)
        ui_window_send(g_windowMgr.item_wnd, NULL, UIM_INVENTORY_REFRESH, 0, 0, 0, 0);
}

void game_mode_zc_ack_extend_body_item_size(struct GameMode *self,
                                                     const struct PACKET_ZC_ACK_EXTEND_BODYITEM_SIZE *packet)
{
    int msg, color = 0xff;

    (void)self;
    switch (packet->result) {
    case 0: msg = 0xdee; color = CHAT_COLOR_CYAN; break;
    case 1: msg = 0xdea; break;
    case 2: msg = 0xdeb; break;
    case 3: msg = 0xdec; break;
    case 4: msg = 0xded; break;
    default: return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(msg), color, 0, 0);
}

void game_mode_zc_ack_open_msg_box_extend_body_item_size(
    struct GameMode *self, const struct PACKET_ZC_ACK_OPEN_MSGBOX_EXTEND_BODYITEM_SIZE *packet)
{
    struct String text;
    const char *item_name, *format;
    int extension, answer, msg;

    switch (packet->result) {
    case 0:
        extension = g_session1.inventoryExtension;
        item_name = item_table_get(packet->item_id)->name;
        format = msg_str(0xde9);
        string_init(&text);
        string_format(&text, format, item_name, extension + 100, extension + 110);
        answer = ui_window_mgr_message_box(&g_windowMgr, string_cstr(&text), 2, 0, 0, 0, UI_MSGBOX_TITLE, 0x118, 0x78, 0);
        string_free(&text);
        mode_send_msg(self, answer == MSGBOX_YES ? MM_EXTEND_BODYITEM_ACCEPT : MM_EXTEND_BODYITEM_REFUSE, 0, 0, 0, 0);
        return;
    case 1: msg = 0xdea; break;
    case 2: msg = 0xdeb; break;
    case 3: msg = 0xdec; break;
    case 4: msg = 0xded; break;
    default: return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(msg), 0xff, 0, 0);
}

static void notice_open(struct GameMode *self)
{
    struct UIWindow *wnd;

    self->notice_shown_at = platform_ticks_ms();
    if (self->notice_wnd != 0)
        return;
    wnd = mem_alloc(sizeof(struct UITransBalloonText));
    if (wnd != 0)
        wnd = ui_trans_balloon_text_construct(wnd);
    self->notice_wnd = wnd;
    ui_window_create(wnd, 8, 8);
    ui_window_mgr_add_to_window_list(&g_windowMgr, self->notice_wnd);
}

static void notice_show(struct GameMode *self, const char *text)
{
    struct UIBalloonText *balloon = (struct UIBalloonText *)self->notice_wnd;
    int x;

    ui_balloon_text_set_text(balloon, text, 0x48);
    x = (g_renderer->width - 640) / 2 - self->notice_wnd->width / 2 + 320;
    ui_window_move_safe(self->notice_wnd, x, 0x33);
    balloon->colour = REFINING_NOTICE_COLOR;
    balloon->outline = 0;
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)text,
                        REFINING_NOTICE_COLOR, 0, 0);
}

static void broadcast_item_name(struct String *out, struct ItemInfo *item, int item_id)
{
    item_info_construct(item);
    item_info_set_item_id(item, item_id);
    item->identified = 1;
    item_info_get_display_name(item, out);
}

void game_mode_zc_broadcast_item_refining_result(struct GameMode *self, const char *name24, int item_id,
                                                          unsigned refine, unsigned success)
{
    struct String name, text;
    struct String item_name = {0};
    struct ItemInfo item;
    unsigned char level = (unsigned char)refine;

    string_init(&name);
    string_assign(&name, name24, 24);
    string_clear(&item_name);
    broadcast_item_name(&item_name, &item, item_id);
    notice_open(self);

    string_init(&text);
    string_format(&text, msg_str((unsigned char)success == 1 ? 0xcc7 : 0xcc8), string_cstr(&name), level,
                  string_cstr(&item_name));
    notice_show(self, string_cstr(&text));

    string_free(&text);
    item_info_destruct(&item);
    string_free(&item_name);
    string_free(&name);
}

void game_mode_zc_ack_merge_item(struct GameMode *self, const struct PACKET_ZC_ACK_MERGE_ITEM *packet)
{
    unsigned index = packet->index, count = packet->count;
    struct UIWindow *wnd;

    (void)self;
    switch (packet->result) {
    case 0:
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(0x87b), 0x64ffff, 0, 0);
        wnd = ui_window_find(WID_MERGE_ITEM);
        if (wnd != 0)
            ui_window_send(wnd, NULL, UIM_MERGE_ITEM_DONE, index, count, 0, 0);
        break;
    case 1:
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(0x87c), 0xff, 0, 0);
        break;
    case 2:
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(0x87d), 0xff, 0, 0);
        break;
    }
    ui_window_mgr_post_quit1(&g_windowMgr, WID_MERGE_ITEM);
}

#pragma pack(push, 1)
struct PACKET_ZC_USE_ITEM_ACK {
    unsigned short id;
    unsigned short index;
    short amount;
    unsigned char result;
};
struct PACKET_ZC_USE_ITEM_ACK2 {
    unsigned short id;
    unsigned short index;
    int item_id;
    unsigned aid;
    short amount;
    unsigned char result;
};
#pragma pack(pop)

enum { ACTOR_MSG_RECOVERY_ITEM = 0x14 };

void game_mode_zc_use_item_ack2(struct GameMode *self, const struct PACKET_ZC_USE_ITEM_ACK2 *packet)
{
    struct ItemInfo item;

    if (packet->aid == ((unsigned)g_session1.ownAid)) {
        self->use_item_pending = 0;
        if (packet->result == 0)
            return;
        session_get_inventory_item(&g_session1, &item, packet->index);
        if (item.index != 0) {
            session_remove_inventory_item(&g_session1, packet->index, item.count - packet->amount);
        }
        item_info_destruct(&item);
    }
    if (packet->result != 0)
        game_mode_show_item_use_effect(self, packet->item_id, packet->aid);
}

void game_mode_zc_use_item_ack(struct GameMode *self, const struct PACKET_ZC_USE_ITEM_ACK *packet)
{
    struct ItemInfo item;

    if (packet->result != 0) {
        session_get_inventory_item(&g_session1, &item, packet->index);
        if (item.index != 0) {
            session_remove_inventory_item(&g_session1, packet->index, item.count - packet->amount);
            if (item_info_id_above600(&item))
                render_object_send(self->world->player, NULL, ACTOR_MSG_RECOVERY_ITEM, 0, 0, 0, 0);
        }
        item_info_destruct(&item);
    }
    self->use_item_pending = 0;
}

void game_mode_zc_failed_get_item_from_zone_da(struct GameMode *self, const unsigned char *pkt)
{
    (void)self;
    (void)pkt;
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)msg_str(0x79e), 0xff, 0, 0);
}

#define MSG_ITEM_DROPPED        0x649
#define CHAT_CHANNEL_ITEM       5

void game_mode_zc_item_throw_ack(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEM_THROW_ACK *packet = (const struct PACKET_ZC_ITEM_THROW_ACK *)pkt;
    unsigned short index = packet->index;
    struct ItemInfo item;

    if (self->exchange_open != 0) {
        self->exchange_open = 0;
        session_end_exchange2(&g_session1);
    }
    session_get_inventory_item(&g_session1, &item, index);
    if (item.index != 0 && packet->count > 0) {
        struct String line;
        struct String name = {0};
        const char *format;

        item_info_get_display_name(&item, &name);
        format = msg_str(MSG_ITEM_DROPPED);
        string_init(&line);
        string_format(&line, format, string_cstr(&name), (int)packet->count);
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)string_cstr(&line), CHAT_COLOR_CYAN,
                            CHAT_CHANNEL_ITEM, 0);
        string_free(&line);
        string_free(&name);
    }
    session_remove_inventory_item(&g_session1, index, packet->count);
    self->throw_item_pending = 0;
    item_info_destruct(&item);
}

#define MSG_BOUND_ON_EQUIP      0x530
#define BOUND_ON_EQUIP_COLOR    0x6effff

void game_mode_zc_notify_bind_on_equip(const unsigned char *pkt)
{
    const struct PACKET_ZC_NOTIFY_BIND_ON_EQUIP *packet = (const struct PACKET_ZC_NOTIFY_BIND_ON_EQUIP *)pkt;
    struct ItemInfo item;

    session_get_inventory_item(&g_session1, &item, packet->index);
    if (item.index != 0) {
        struct String line;
        struct String name = {0};
        const char *format;

        item_info_get_display_name(&item, &name);
        format = msg_str(MSG_BOUND_ON_EQUIP);
        string_init(&line);
        string_format(&line, format, string_cstr(&name));
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (intptr_t)string_cstr(&line), BOUND_ON_EQUIP_COLOR,
                            0, 0);
        string_free(&line);
        string_free(&name);
    }
    item_info_destruct(&item);
}
