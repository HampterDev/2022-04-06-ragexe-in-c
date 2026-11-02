#include "game/actor_packets.h"
#include "game/ground_item_packets.h"
#include "game/pickup_requests.h"
#include "session/state.h"
#include "ui/window_access.h"
#include <stdlib.h>
#include <string.h>
#include "game/packet_handlers.h"
#include "game/client.h"
#include "game/gamemode.h"
#include "game/item_info.h"
#include "game/item_list_packets.h"
#include "game/party_member.h"
#include "core/msg_str.h"
#include "core/text_util.h"
#include "session/session_items.h"
#include "session/session_party.h"
#include "ui/wndmgr/wndmgr.h"

static void clear_pickup_requests(struct GameMode *self)
{
    list_clear(&self->pickup_requests, NULL);
}

#define WINDOW_ITEM_OBTAIN 0x3a
#define WINDOW_MENU_ICON 0x133
#define CHANNEL_PICKUP 5
#define CHANNEL_PARTY 8
#define COLOUR_PICKUP 0xffff00

char game_mode_pickup_failed(int result)
{
    int msg;

    switch (result) {
    case 1:
    case 6:
        msg = 0x35;
        break;
    case 2:
        msg = 0x34;
        break;
    case 4:
        msg = 0xdc;
        break;
    case 5:
        msg = 0x117;
        break;
    case 7:
        msg = 0x58a;
        break;
    case 9:
        msg = 0xf90;
        break;
    default:
        return 0;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_REFRESH, 0, 0, 0, 0);
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)msg_str(msg), 0xff, 0, 0);
    return 1;
}

static void chat_line(const char *text, int channel)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)text, COLOUR_PICKUP, channel, 0);
}

static void announce_pickup(struct ItemInfo *item, int count)
{
    struct String line;

    struct String name = {0};

    item_info_get_display_name(item, &name);
    string_init(&line);
    string_format(&line, msg_str(0x99), string_cstr(&name), count);
    chat_line(string_cstr(&line), CHANNEL_PICKUP);
    string_free(&line);
    string_free(&name);
}

static void show_obtained_window(struct ItemInfo *item, int count)
{
    struct UIWindow *w = ui_window_open(WINDOW_ITEM_OBTAIN);

    UIWINDOW_SENDMSG(w, 0x4c, item->identified, 0, 0, 0);
    UIWINDOW_SENDMSG(w, 0x24, 0, count, (intptr_t)string_cstr(&item->id_text), 0);
    UIWINDOW_SENDMSG(w, 0x22, (intptr_t)item, 0, 0, 0);
}

static void mark_item_menu_new(void)
{
    struct UIWindow *w = ui_window_find(WINDOW_MENU_ICON);

    if (w != 0)
        UIWINDOW_SENDMSG(w, 6, 0x116, 0xba, 1, 0);
}

static void mark_type(struct ItemInfo *item, unsigned char type)
{
    if (type == 10 || type == 16 || type == 17 || type == 19)
        item->worn_location = 0x8000;
}

static void stacked_location(struct ItemInfo *item, unsigned char type)
{
    switch (type) {
    case 4: case 5: case 8: case 9: case 11: case 12: case 13: case 14: case 15:
        break;
    default:
        item->equip_location = 0;
    }
}

static int added_count(const struct ItemInfo *old, int count)
{
    return (unsigned)old->index > 0 ? count - old->count : count;
}

static void end_exchange(struct GameMode *mode)
{
    if (mode->exchange_open != 0) {
        mode->exchange_open = 0;
        session_end_exchange2(&g_session1);
    }
}

static void set_options(struct ItemInfo *item, const void *packet_options)
{
    struct ItemOption opts[5];
    int n = 0, i;

    memcpy(opts, packet_options, sizeof opts);
    for (i = 0; i < 5; i++)
        if (opts[i].id != 0)
            n++;
    item->option_count = n;
    for (i = 0; i < n; i++)
        item->options[i] = opts[i];
}

#pragma pack(push, 1)
struct PACKET_ZC_ITEM_PICKUP_ACK {
    unsigned short packet_type;
    unsigned short index;
    unsigned short count;
    int item_id;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
    unsigned short location;
    unsigned char type;
    unsigned char result;
};
struct PACKET_ZC_ITEM_PICKUP_ACK2 {
    struct PACKET_ZC_ITEM_PICKUP_ACK ack;
    int expire_time;
};
struct PACKET_ZC_ITEM_PICKUP_ACK3 {
    struct PACKET_ZC_ITEM_PICKUP_ACK ack;
    int expire_time;
    unsigned short bind_on_equip_type;
};
struct PACKET_ZC_ITEM_PICKUP_ACK_V5 {
    unsigned short packet_type;
    unsigned short index;
    unsigned short count;
    int item_id;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
    int location;
    unsigned char type;
    unsigned char result;
    int expire_time;
    unsigned short bind_on_equip_type;
};
struct PACKET_ZC_ITEM_PICKUP_ACK_V6 {
    struct PACKET_ZC_ITEM_PICKUP_ACK_V5 v5;
    struct ItemOption options[ITEM_OPTION_SLOTS];
};
struct PACKET_ZC_ITEM_PICKUP_ACK_V7 {
    struct PACKET_ZC_ITEM_PICKUP_ACK_V6 v6;
    unsigned char favorite;
    unsigned short view;
};
struct PACKET_ZC_ITEM_PICKUP_ACK_V8 {
    unsigned short packet_type;
    unsigned short index;
    unsigned short count;
    int item_id;
    unsigned char identified;
    unsigned char damaged;
    int cards[ITEM_CARD_SLOTS];
    int location;
    unsigned char type;
    unsigned char result;
    int expire_time;
    unsigned short bind_on_equip_type;
    struct ItemOption options[ITEM_OPTION_SLOTS];
    unsigned char favorite;
    unsigned short view;
    unsigned char refine;
    unsigned char grade;
};
struct PACKET_ZC_ITEM_PICKUP_PARTY {
    unsigned short packet_type;
    int aid;
    int item_id;
    unsigned char identified;
    unsigned char damaged;
    unsigned char refine;
    int cards[ITEM_CARD_SLOTS];
    unsigned short location;
    unsigned char type;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ITEM_PICKUP_ACK, 0x21);
SIZE_ASSERT(PACKET_ZC_ITEM_PICKUP_ACK2, 0x25);
SIZE_ASSERT(PACKET_ZC_ITEM_PICKUP_ACK3, 0x27);
SIZE_ASSERT(PACKET_ZC_ITEM_PICKUP_ACK_V5, 0x29);
OFFSET_ASSERT(PACKET_ZC_ITEM_PICKUP_ACK_V6, options, 0x29);
OFFSET_ASSERT(PACKET_ZC_ITEM_PICKUP_ACK_V7, favorite, 0x42);
OFFSET_ASSERT(PACKET_ZC_ITEM_PICKUP_ACK_V8, favorite, 0x41);
SIZE_ASSERT(PACKET_ZC_ITEM_PICKUP_ACK_V8, 0x46);
SIZE_ASSERT(PACKET_ZC_ITEM_PICKUP_PARTY, 0x20);

static void read_old_ack(struct ItemInfo *item, const struct PACKET_ZC_ITEM_PICKUP_ACK *packet)
{
    item_info_construct(item);
    item->type = packet->type;
    item->equip_location = packet->location;
    item->index = packet->index;
    item->worn_location = 0;
    item->count = packet->count;
    item_info_set_item_id(item, packet->item_id);
    memcpy(item->slot, packet->cards, sizeof item->slot);
    item->identified = packet->identified;
    item->damaged = packet->damaged;
    item->refine = packet->refine;
}

void game_mode_item_pickup_ack(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEM_PICKUP_ACK *packet = (const struct PACKET_ZC_ITEM_PICKUP_ACK *)pkt;
    struct ItemInfo item, old;
    struct String line, text;
    struct String name = {0};
    int count;

    clear_pickup_requests(self);
    if (packet->result == 0) {
        read_old_ack(&item, packet);

        mark_type(&item, packet->type);
        session_add_inventory_item(&g_session1, &item, 1);
        announce_pickup(&item, packet->count);
        show_obtained_window(&item, packet->count);
        item_info_destruct(&item);
    } else if (packet->result == 3) {
        read_old_ack(&item, packet);

        mark_type(&item, packet->type);
        stacked_location(&item, packet->type);
        session_get_inventory_item(&g_session1, &old, packet->index);
        string_init(&line);
        if ((unsigned)old.index > 0) {
            item_info_get_display_name(&item, &name);
            string_init(&text);
            string_format(&text, msg_str(0x99), string_cstr(&name), packet->count - old.count);
            string_assign(&line, string_cstr(&text), text.size);
            string_free(&text);
            string_free(&name);
            count = packet->count - old.count;
        } else {
            item_info_get_display_name(&item, &name);
            string_init(&text);
            string_format(&text, msg_str(0x99), string_cstr(&name), packet->count);
            string_assign(&line, string_cstr(&text), text.size);
            string_free(&text);
            string_free(&name);
            count = packet->count;
        }
        chat_line(string_cstr(&line), CHANNEL_PICKUP);
        show_obtained_window(&item, count);
        end_exchange(self);
        session_inventory_update(&g_session1, &item, 1);
        string_free(&line);
        item_info_destruct(&old);
        item_info_destruct(&item);
    } else {
        game_mode_pickup_failed(packet->result);
    }
}

void game_mode_item_pickup_ack2(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEM_PICKUP_ACK2 *packet = (const struct PACKET_ZC_ITEM_PICKUP_ACK2 *)pkt;
    const struct PACKET_ZC_ITEM_PICKUP_ACK *ack = &packet->ack;
    struct ItemInfo item, old;
    int count;

    clear_pickup_requests(self);
    if (ack->result == 0) {
        read_old_ack(&item, ack);
        item.expire_time = packet->expire_time;

        mark_type(&item, ack->type);
        session_add_inventory_item(&g_session1, &item, 1);
        announce_pickup(&item, ack->count);
        show_obtained_window(&item, ack->count);
        item_info_destruct(&item);
    } else if (ack->result == 3) {
        read_old_ack(&item, ack);
        item.expire_time = packet->expire_time;

        mark_type(&item, ack->type);
        stacked_location(&item, ack->type);
        session_get_inventory_item(&g_session1, &old, ack->index);
        count = added_count(&old, ack->count);
        announce_pickup(&item, count);
        show_obtained_window(&item, count);
        end_exchange(self);
        session_inventory_update(&g_session1, &item, 1);
        item_info_destruct(&old);
        item_info_destruct(&item);
    } else {
        game_mode_pickup_failed(ack->result);
    }
}

void game_mode_item_pickup_ack3(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEM_PICKUP_ACK3 *packet = (const struct PACKET_ZC_ITEM_PICKUP_ACK3 *)pkt;
    const struct PACKET_ZC_ITEM_PICKUP_ACK *ack = &packet->ack;
    struct ItemInfo item, old;
    int count;

    clear_pickup_requests(self);
    if (ack->result == 0) {
        read_old_ack(&item, ack);
        item.expire_time = packet->expire_time;
        item.bind_on_equip_type = packet->bind_on_equip_type;
        item.is_new = 1;

        mark_type(&item, ack->type);
        session_add_inventory_item(&g_session1, &item, 1);
        announce_pickup(&item, ack->count);
        show_obtained_window(&item, ack->count);
        mark_item_menu_new();
        item_info_destruct(&item);
    } else if (ack->result == 3) {
        read_old_ack(&item, ack);
        item.expire_time = packet->expire_time;
        item.bind_on_equip_type = packet->bind_on_equip_type;

        mark_type(&item, ack->type);
        stacked_location(&item, ack->type);
        session_get_inventory_item(&g_session1, &old, ack->index);
        count = added_count(&old, ack->count);
        announce_pickup(&item, count);
        show_obtained_window(&item, count);
        end_exchange(self);
        session_inventory_update(&g_session1, &item, 1);
        mark_item_menu_new();
        item_info_destruct(&old);
        item_info_destruct(&item);
    } else {
        game_mode_pickup_failed(ack->result);
    }
}

struct Pickup {
    unsigned short index;
    unsigned short count;
    int item_id;
    unsigned char identified, damaged, refine, type;
    int cards[4];
    int location;
    int expire_time;
    unsigned short bind_on_equip_type;
};

static void read_pickup(struct ItemInfo *item, const struct Pickup *p)
{
    item_info_construct(item);
    item->type = p->type;
    item->equip_location = p->location;
    item->index = p->index;
    item->worn_location = 0;
    item->count = p->count;
    item_info_set_item_id(item, p->item_id);
    memcpy(item->slot, p->cards, sizeof item->slot);
    item->identified = p->identified;
    item->damaged = p->damaged;
    item->refine = p->refine;
    item->expire_time = p->expire_time;
    item->bind_on_equip_type = p->bind_on_equip_type;
    item->is_new = 1;
}

static void picked_up(struct GameMode *mode, struct ItemInfo *item, const struct Pickup *p)
{
    mark_type(item, p->type);
    session_add_inventory_item(&g_session1, item, 1);
    announce_pickup(item, p->count);
    show_obtained_window(item, p->count);
    mark_item_menu_new();
}

static void stacked(struct GameMode *mode, struct ItemInfo *item, const struct Pickup *p)
{
    struct ItemInfo old;
    int count;

    mark_type(item, p->type);
    stacked_location(item, p->type);
    session_get_inventory_item(&g_session1, &old, p->index);
    count = p->count;
    if ((unsigned)old.index > 0)
        count = p->count - old.count;
    announce_pickup(item, count);
    show_obtained_window(item, count);
    if (mode->exchange_open != 0) {
        mode->exchange_open = 0;
        session_end_exchange2(&g_session1);
    }
    session_inventory_update(&g_session1, item, 1);
    mark_item_menu_new();
    item_info_destruct(&old);
}

void game_mode_item_pickup_ack_v5(struct GameMode *self, unsigned short index, unsigned short count, int item_id,
                                         unsigned char identified, unsigned char damaged, unsigned char refine,
                                         struct ItemCards cards, int location, unsigned char type, unsigned char result,
                                         int expire_time, unsigned short bind_on_equip_type)
{
    struct Pickup p;
    struct ItemInfo item;

    clear_pickup_requests(self);
    p.index = index;
    p.count = count;
    p.item_id = item_id;
    p.identified = identified;
    p.damaged = damaged;
    p.refine = refine;
    p.type = type;
    memcpy(p.cards, cards.card, sizeof p.cards);
    p.location = location;
    p.expire_time = expire_time;
    p.bind_on_equip_type = bind_on_equip_type;
    if (result == 0) {
        read_pickup(&item, &p);
        item.option_count = 0;
        picked_up(self, &item, &p);
        item_info_destruct(&item);
        mark_item_menu_new();
    } else if (result == 3) {
        read_pickup(&item, &p);
        item.option_count = 0;
        stacked(self, &item, &p);
        item_info_destruct(&item);
    } else {
        game_mode_pickup_failed(result);
    }
}

static void read_v6(struct Pickup *p, const struct PACKET_ZC_ITEM_PICKUP_ACK_V5 *packet)
{
    p->index = packet->index;
    p->count = packet->count;
    p->item_id = packet->item_id;
    p->identified = packet->identified;
    p->damaged = packet->damaged;
    p->refine = packet->refine;
    memcpy(p->cards, packet->cards, sizeof p->cards);
    p->location = packet->location;
    p->type = packet->type;
    p->expire_time = packet->expire_time;
    p->bind_on_equip_type = packet->bind_on_equip_type;
}

void game_mode_item_pickup_ack_v6(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEM_PICKUP_ACK_V6 *packet = (const struct PACKET_ZC_ITEM_PICKUP_ACK_V6 *)pkt;
    struct Pickup p;
    struct ItemInfo item;

    clear_pickup_requests(self);
    read_v6(&p, &packet->v5);
    if (packet->v5.result == 0) {
        read_pickup(&item, &p);
        set_options(&item, packet->options);
        picked_up(self, &item, &p);
        item_info_destruct(&item);
    } else if (packet->v5.result == 3) {
        read_pickup(&item, &p);
        set_options(&item, packet->options);
        stacked(self, &item, &p);
        item_info_destruct(&item);
    } else {
        game_mode_pickup_failed(packet->v5.result);
    }
}

void game_mode_item_pickup_ack_v7(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEM_PICKUP_ACK_V7 *packet = (const struct PACKET_ZC_ITEM_PICKUP_ACK_V7 *)pkt;
    const struct PACKET_ZC_ITEM_PICKUP_ACK_V5 *v5 = &packet->v6.v5;
    struct Pickup p;
    struct ItemInfo item;

    clear_pickup_requests(self);
    read_v6(&p, v5);
    if (v5->result == 0 || v5->result == 3) {
        read_pickup(&item, &p);
        set_options(&item, packet->v6.options);
        item.favorite = packet->favorite;
        item.view = packet->view;
        item.grade = 0;
        if (v5->result == 0)
            picked_up(self, &item, &p);
        else
            stacked(self, &item, &p);
        item_info_destruct(&item);
    } else {
        game_mode_pickup_failed(v5->result);
    }
}

void game_mode_item_pickup_ack_v8(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEM_PICKUP_ACK_V8 *packet = (const struct PACKET_ZC_ITEM_PICKUP_ACK_V8 *)pkt;
    struct Pickup p;
    struct ItemInfo item;

    clear_pickup_requests(self);
    p.index = packet->index;
    p.count = packet->count;
    p.item_id = packet->item_id;
    p.identified = packet->identified;
    p.damaged = packet->damaged;
    p.refine = packet->refine;
    memcpy(p.cards, packet->cards, sizeof p.cards);
    p.location = packet->location;
    p.type = packet->type;
    p.expire_time = packet->expire_time;
    p.bind_on_equip_type = packet->bind_on_equip_type;
    if (packet->result == 0 || packet->result == 3) {
        read_pickup(&item, &p);
        set_options(&item, packet->options);
        item.favorite = packet->favorite;
        item.view = packet->view;
        item.grade = packet->grade;
        if (packet->result == 0)
            picked_up(self, &item, &p);
        else
            stacked(self, &item, &p);
        item_info_destruct(&item);
    } else {
        game_mode_pickup_failed(packet->result);
    }
}

void game_mode_item_pickup_party(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEM_PICKUP_PARTY *packet = (const struct PACKET_ZC_ITEM_PICKUP_PARTY *)pkt;
    struct PartyMember member;
    struct ItemInfo item;
    struct String line;
    struct String name = {0};

    session_get_party_member(&g_session1, &member, packet->aid);
    if (packet->aid != g_session1.ownAid && member.found != 0) {
        item_info_construct(&item);
        item.type = packet->type;
        item.equip_location = packet->location;
        item.index = packet->item_id;
        item.worn_location = 0;
        item_info_set_item_id(&item, packet->item_id);
        memcpy(item.slot, packet->cards, sizeof item.slot);
        item.identified = packet->identified;
        item.damaged = packet->damaged;
        item.refine = packet->refine;
        item_info_get_display_name(&item, &name);
        string_init(&line);
        string_format(&line, msg_str(0x4fc), string_cstr(&member.name), string_cstr(&name));
        chat_line(string_cstr(&line), CHANNEL_PARTY);
        string_free(&line);
        string_free(&name);
        item_info_destruct(&item);
    }
    party_member_info_destroy(&member);
}

void game_mode_add_pickup_request(struct GameMode *self, unsigned item_aid)
{
    list_push_back(&self->pickup_requests, &item_aid, sizeof item_aid);
}

char game_mode_is_pickup_requested(struct GameMode *self, unsigned item_aid)
{
    struct ListNode *head = self->pickup_requests.head;
    struct ListNode *node;

    for (node = head->next; node != head; node = node->next)
        if (*LIST_VALUE(node, unsigned) == item_aid)
            return 1;
    return 0;
}

void game_mode_remove_pickup_request(struct GameMode *self, unsigned item_aid)
{
    struct ListNode *head = self->pickup_requests.head;
    struct ListNode *node;

    for (node = head->next; node != head; node = node->next) {
        if (*LIST_VALUE(node, unsigned) == item_aid) {
            list_erase(&self->pickup_requests, node, NULL);
            return;
        }
    }
}

void game_mode_zc_item_pickup_ack_v5(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEM_PICKUP_ACK_V5 *packet = (const struct PACKET_ZC_ITEM_PICKUP_ACK_V5 *)pkt;
    struct ItemCards cards;

    memcpy(&cards, packet->cards, sizeof cards);
    game_mode_item_pickup_ack_v5(self, packet->index, packet->count,
                             packet->item_id, packet->identified, packet->damaged, packet->refine, cards,
                             packet->location, packet->type, packet->result, packet->expire_time,
                             packet->bind_on_equip_type);
}
