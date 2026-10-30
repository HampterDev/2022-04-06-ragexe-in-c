#include "game/packet_handlers.h"
#include <string.h>
#include "game/client.h"
#include "game/gamemode.h"
#include "game/item_records.h"
#include "game/item_list_packets.h"
#include "session/session_items.h"
#include "session/session_windows.h"

static int len_of(const struct PACKET_ZC_ITEMLIST *packet)
{
    return packet->packet_length - 4;
}

static void add_to_cart(struct ItemInfo *item)
{
    session_add_cart_item(&g_session1, item);
    item_info_destruct(item);
}

void game_mode_cart_equipment_item_list2(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    int n = (int)((unsigned)len_of(packet) / 34);
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    for (; n > 0; n--, r += 34) {
        const struct EquipRecordV2 *record = (const struct EquipRecordV2 *)r;

        item_from_equip_record(&item, record->base);
        item.expire_time = record->expire_time;
        add_to_cart(&item);
    }
    session_refresh_party_ui(&g_session1);
}

void game_mode_cart_equipment_item_list3(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    int n = (int)((unsigned)len_of(packet) / 38);
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    for (; n > 0; n--, r += 38) {
        const struct EquipRecordV3 *record = (const struct EquipRecordV3 *)r;

        item_from_equip_record(&item, record->base);
        item.expire_time = record->expire_time;
        item.bind_on_equip_type = record->bind_on_equip_type;
        item.view = record->view;
        add_to_cart(&item);
    }
    session_refresh_party_ui(&g_session1);
}

void game_mode_cart_item_list_equip_v5(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    int n = (int)((unsigned)len_of(packet) / 41);
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    for (; n > 0; n--, r += 41) {
        item_from_v5_record_full(&item, r);
        item.grade = 0;
        add_to_cart(&item);
    }
    session_refresh_party_ui(&g_session1);
}

void game_mode_cart_item_list_equip_v6(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    int n = (int)((unsigned)len_of(packet) / 67);
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    for (; n > 0; n--, r += 67) {
        item_from_v6_record(&item, r);
        item.grade = 0;
        add_to_cart(&item);
    }
    session_refresh_party_ui(&g_session1);
}

void game_mode_cart_item_list_normal_v5(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    int n = (int)((unsigned)len_of(packet) / 34);
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    for (; n > 0; n--, r += 34) {
        item_from_normal_record(&item, r, 1);
        add_to_cart(&item);
    }
    session_refresh_party_ui(&g_session1);
}

void game_mode_add_item_to_cart2(const unsigned char *pkt)
{
    const struct PACKET_ZC_ADD_ITEM_TO_CART2 *packet = (const struct PACKET_ZC_ADD_ITEM_TO_CART2 *)pkt;
    struct ItemOption none[5];
    struct ItemInfo item;

    memset(none, 0, sizeof none);
    item_from_added_record(&item, pkt);
    item.refine = packet->refine;
    memcpy(item.slot, packet->cards, sizeof item.slot);
    item_set_options(&item, none);
    item.grade = 0;
    add_to_cart(&item);
}

void game_mode_zc_add_item_to_cart3(const unsigned char *pkt)
{
    const struct PACKET_ZC_ADD_ITEM_TO_CART3 *packet = (const struct PACKET_ZC_ADD_ITEM_TO_CART3 *)pkt;
    struct ItemOption options[5];
    struct ItemInfo item;

    memcpy(options, packet->options, sizeof options);
    item_from_added_record(&item, pkt);
    item.refine = packet->refine;
    memcpy(item.slot, packet->cards, sizeof item.slot);
    item_set_options(&item, options);
    item.grade = 0;
    add_to_cart(&item);
}

void game_mode_add_item_to_cart4(const unsigned char *pkt)
{
    const struct PACKET_ZC_ADD_ITEM_TO_CART4 *packet = (const struct PACKET_ZC_ADD_ITEM_TO_CART4 *)pkt;
    struct ItemOption options[5];
    struct ItemInfo item;

    memcpy(options, packet->options, sizeof options);
    item_from_added_record(&item, pkt);
    item.refine = packet->refine;
    memcpy(item.slot, packet->cards, sizeof item.slot);
    item_set_options(&item, options);
    item.grade = packet->grade;
    add_to_cart(&item);
}
