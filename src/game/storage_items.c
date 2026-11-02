#include "game/packet_handlers.h"
#include <string.h>
#include "game/client.h"
#include "game/gamemode.h"
#include "game/item_records.h"
#include "game/item_list_packets.h"
#include "ui/shops/storage.h"
#include "session/session_items.h"
#include "session/session_windows.h"

#define WND_STORAGE 0x21

static void open_storage(const struct PACKET_ZC_STORE_ITEMLIST *packet)
{
    struct UIWindow *w = ui_window_open(WND_STORAGE);
    const char *name = packet->name;
    struct String title;

    if (*name == 0)
        return;
    string_init(&title);
    string_assign_cstr(&title, name);
    ui_storage_set_title(w, string_cstr(&title));
    string_free(&title);
}

static int record_count(const struct PACKET_ZC_STORE_ITEMLIST *packet, int size)
{
    return (int)((unsigned)(packet->packet_length - 0x1c) / size);
}

void game_mode_store_item_list_equip_v5(const unsigned char *pkt)
{
    const struct PACKET_ZC_STORE_ITEMLIST *packet = (const struct PACKET_ZC_STORE_ITEMLIST *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    open_storage(packet);
    for (n = record_count(packet, 41); n > 0; n--, r += 41) {
        item_from_v5_record_full(&item, r);
        session_add_storage_item(&g_session1, &item, 1);
        item_info_destruct(&item);
    }
    session_refresh_friend_ui(&g_session1);
}

void game_mode_store_item_list_equip_v6(const unsigned char *pkt)
{
    const struct PACKET_ZC_STORE_ITEMLIST *packet = (const struct PACKET_ZC_STORE_ITEMLIST *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    open_storage(packet);
    for (n = record_count(packet, 67); n > 0; n--, r += 67) {
        item_from_v6_record(&item, r);
        session_add_storage_item(&g_session1, &item, 1);
        item_info_destruct(&item);
    }
    session_refresh_friend_ui(&g_session1);
}

void game_mode_store_item_list_normal_v5(const unsigned char *pkt)
{
    const struct PACKET_ZC_STORE_ITEMLIST *packet = (const struct PACKET_ZC_STORE_ITEMLIST *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    open_storage(packet);
    for (n = record_count(packet, 34); n > 0; n--, r += 34) {
        item_from_normal_record(&item, r, 1);
        session_add_storage_item(&g_session1, &item, 0);
        item_info_destruct(&item);
    }
    session_refresh_friend_ui(&g_session1);
}

static void add_stored(struct ItemInfo *item, const struct ItemOption *options)
{
    item_set_options(item, options);
    session_add_storage_item(&g_session1, item, 1);
    item_info_destruct(item);
}

void game_mode_add_item_to_store2(const unsigned char *pkt)
{
    const struct PACKET_ZC_ADD_ITEM_TO_STORE2 *packet = (const struct PACKET_ZC_ADD_ITEM_TO_STORE2 *)pkt;
    struct ItemOption none[5];
    struct ItemInfo item;

    memset(none, 0, sizeof none);
    item_from_added_record(&item, pkt);
    item.refine = packet->refine;
    memcpy(item.slot, packet->cards, sizeof item.slot);
    add_stored(&item, none);
}

void game_mode_add_item_to_store3(const unsigned char *pkt)
{
    const struct PACKET_ZC_ADD_ITEM_TO_STORE3 *packet = (const struct PACKET_ZC_ADD_ITEM_TO_STORE3 *)pkt;
    struct ItemOption options[5];
    struct ItemInfo item;

    memcpy(options, packet->options, sizeof options);
    item_from_added_record(&item, pkt);
    item.refine = packet->refine;
    memcpy(item.slot, packet->cards, sizeof item.slot);
    add_stored(&item, options);
}

void game_mode_add_item_to_store4(const unsigned char *pkt)
{
    const struct PACKET_ZC_ADD_ITEM_TO_STORE4 *packet = (const struct PACKET_ZC_ADD_ITEM_TO_STORE4 *)pkt;
    struct ItemOption options[5];
    struct ItemInfo item;

    memcpy(options, packet->options, sizeof options);
    item_from_added_record(&item, pkt);
    item.refine = packet->refine;
    memcpy(item.slot, packet->cards, sizeof item.slot);
    add_stored(&item, options);
}
