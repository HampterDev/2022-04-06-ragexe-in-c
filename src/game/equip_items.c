#include "game/packet_handlers.h"
#include "session/item_predicates.h"
#include "contents/pet_evolution.h"
#include "session/state.h"
#include <stdlib.h>
#include "game/client.h"
#include "game/gamemode.h"
#include "game/item_records.h"
#include "game/item_list_packets.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "session/session_state.h"
#include "ui/wndmgr/wndmgr.h"

static int is_hatched_egg(const struct ItemInfo *item)
{
    return session_is_pet_egg_id(&g_session1, atoi(string_cstr(&item->id_text))) && item->damaged;
}

void game_mode_note_hatched_egg(const struct ItemInfo *item)
{
    if (is_hatched_egg(item))
        g_session1.hatchedEggIndex = item->index;
}

void game_mode_check_pet_auto_feed(const struct ItemInfo *item)
{
    struct ItemInfo egg;
    const struct ItemInfo *got;
    int egg_id;

    if (!is_hatched_egg(item))
        return;
    got = session_get_inventory_item(&g_session1, &egg, g_session1.hatchedEggIndex);
    egg_id = atoi(string_cstr(&got->id_text));
    item_info_destruct(&egg);
    if (!pet_evolution_is_auto_feeding(g_petTable, egg_id) && g_session1.petAutoFeed == 1)
        session_send_pet_auto_feed(&g_session1, 0);
}

static void to_inventory(const struct ItemInfo *item)
{
    session_add_inventory_item(&g_session1, item, 0);
    session_inventory_update(&g_session1, item, 0);
}

static void store_plain(const struct ItemInfo *item)
{
    if (item->worn_location != 0)
        session_set_worn_item(&g_session1, item);
    else
        to_inventory(item);
}

static void store(const struct ItemInfo *item)
{
    if (item->worn_location == 0)
        to_inventory(item);
    else if (session_is_costume_or_shadow_location(&g_session1, item->worn_location))
        session_set_worn_costume(&g_session1, item);
    else
        session_set_worn_item(&g_session1, item);
}

static int record_count(const struct PACKET_ZC_ITEMLIST *packet, int size)
{
    return (int)((unsigned)(packet->packet_length - 4) / size);
}

static void clear(int costumes)
{
    session_remove_inventory_filter_types(&g_session1);
    session_clear_worn_items(&g_session1);
    if (costumes)
        session_clear_worn_costumes(&g_session1);
}

void game_mode_equipment_item_list2(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    clear(0);
    for (n = record_count(packet, 34); n > 0; n--, r += 34) {
        const struct EquipRecordV2 *record = (const struct EquipRecordV2 *)r;

        item_from_equip_record(&item, record->base);
        item.expire_time = record->expire_time;
        game_mode_note_hatched_egg(&item);
        store_plain(&item);
        item_info_destruct(&item);
    }
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void game_mode_equipment_item_list3(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    clear(1);
    for (n = record_count(packet, 38); n > 0; n--, r += 38) {
        const struct EquipRecordV3 *record = (const struct EquipRecordV3 *)r;

        item_from_equip_record(&item, record->base);
        item.expire_time = record->expire_time;
        item.bind_on_equip_type = record->bind_on_equip_type;
        item.view = record->view;
        game_mode_note_hatched_egg(&item);
        store(&item);
        item_info_destruct(&item);
    }
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void game_mode_equipment_item_list4(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    clear(1);
    for (n = record_count(packet, 37); n > 0; n--, r += 37) {
        item_from_equip_record4(&item, r);
        game_mode_note_hatched_egg(&item);
        store(&item);
        item_info_destruct(&item);
    }
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void game_mode_equipment_item_list_v5(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    clear(1);
    for (n = record_count(packet, 41); n > 0; n--, r += 41) {
        item_from_v5_record_full(&item, r);
        item.grade = 0;
        game_mode_note_hatched_egg(&item);
        store(&item);
        game_mode_check_pet_auto_feed(&item);
        item_info_destruct(&item);
    }
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void game_mode_equipment_item_list_v6(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    clear(1);
    for (n = record_count(packet, 67); n > 0; n--, r += 67) {
        item_from_v6_record(&item, r);
        item.grade = 0;
        game_mode_note_hatched_egg(&item);
        store(&item);
        game_mode_check_pet_auto_feed(&item);
        item_info_destruct(&item);
    }
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void game_mode_equipment_item_list_v7(const unsigned char *pkt)
{
    const struct PACKET_ZC_ITEMLIST *packet = (const struct PACKET_ZC_ITEMLIST *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    clear(1);
    for (n = record_count(packet, 68); n > 0; n--, r += 68) {
        item_from_v7_record(&item, r);
        game_mode_note_hatched_egg(&item);
        store(&item);
        game_mode_check_pet_auto_feed(&item);
        item_info_destruct(&item);
    }
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}
