#include "game/packet_handlers.h"
#include "session/state.h"
#include <stdint.h>
#include <string.h>
#include "base.h"
#include "game/client.h"
#include "game/gamemode.h"
#include "game/item_records.h"
#include "session/session_items.h"
#include "session/session_jobs.h"
#include "ui/wndmgr/wndmgr.h"

#define WND_EQUIP_MICROSCOPE 0x8b

#pragma pack(push, 1)
struct EquipWinLookHead {
    uint16_t packet_type;
    short length;
    char name[24];
    short job;
    short head;
    short accessory;
    short accessory2;
    short accessory3;
    short robe;
    short hair_palette;
    short body_palette;
};
struct EquipWinLook {
    struct EquipWinLookHead head;
    uint8_t sex;
};
struct EquipWinLookBody {
    struct EquipWinLookHead head;
    short body;
    uint8_t sex;
};
struct EquipRecord3 {
    uint8_t item[0x1e];
    int expire_time;
    uint16_t bind_on_equip_type;
    uint16_t view;
};
struct PACKET_ZC_EQUIPWIN_MICROSCOPE2 {
    struct EquipWinLook look;
    struct EquipRecord3 records[];
};
struct PACKET_ZC_EQUIPWIN_MICROSCOPE_LOOK {
    struct EquipWinLook look;
    uint8_t records[];
};
struct PACKET_ZC_EQUIPWIN_MICROSCOPE_LOOK_BODY {
    struct EquipWinLookBody look;
    uint8_t records[];
};
#pragma pack(pop)
SIZE_ASSERT(EquipWinLookHead, 0x2c);
SIZE_ASSERT(EquipWinLook, 0x2d);
SIZE_ASSERT(EquipWinLookBody, 0x2f);
SIZE_ASSERT(EquipRecord3, 38);
SIZE_ASSERT(PACKET_ZC_EQUIPWIN_MICROSCOPE2, 0x2d);
SIZE_ASSERT(PACKET_ZC_EQUIPWIN_MICROSCOPE_LOOK, 0x2d);
SIZE_ASSERT(PACKET_ZC_EQUIPWIN_MICROSCOPE_LOOK_BODY, 0x2f);

static void read_look(const unsigned char *pkt, int has_body)
{
    const struct EquipWinLookHead *look = (const struct EquipWinLookHead *)pkt;

    g_session1.viewedLook[0] = look->job;
    g_session1.viewedLook[1] = has_body ? ((const struct EquipWinLookBody *)pkt)->sex
                                        : ((const struct EquipWinLook *)pkt)->sex;
    g_session1.viewedLook[2] = look->head;
    g_session1.viewedLook[3] = look->accessory;
    g_session1.viewedLook[4] = look->accessory2;
    g_session1.viewedLook[5] = look->accessory3;
    g_session1.viewedLook[6] = look->body_palette;
    g_session1.viewedLook[7] = look->hair_palette;
    memcpy(g_session1.viewedName, look->name, sizeof g_session1.viewedName);
    g_session1.viewedLook[8] = look->robe;
    if (has_body)
        g_session1.viewedBody = ((const struct EquipWinLookBody *)pkt)->body;
}

static void open_window(void)
{
    if (!ui_window_mgr_post_quit1(&g_windowMgr, WND_EQUIP_MICROSCOPE))
        ui_window_open(WND_EQUIP_MICROSCOPE);
}

static void view_item(const struct ItemInfo *item)
{
    if (item->worn_location == 0)
        return;
    if (session_is_costume_or_shadow_location(&g_session1, item->worn_location))
        session_set_viewed_costume(&g_session1, item);
    else
        session_set_viewed_item(&g_session1, item);
}

static int record_count(const unsigned char *pkt, int look_size, int size)
{
    return (int)((unsigned)(((const struct EquipWinLookHead *)pkt)->length - look_size) / size);
}

static void clear(void)
{
    session_clear_viewed_items(&g_session1);
    session_clear_viewed_costumes(&g_session1);
}

void game_mode_equip_win_microscope2(const unsigned char *pkt)
{
    const struct PACKET_ZC_EQUIPWIN_MICROSCOPE2 *packet = (const struct PACKET_ZC_EQUIPWIN_MICROSCOPE2 *)pkt;
    int n;
    const struct EquipRecord3 *r = packet->records;
    struct ItemInfo item;

    clear();
    for (n = record_count(pkt, 0x2d, 38); n > 0; n--, r++) {
        item_from_equip_record(&item, (const unsigned char *)r);
        item.expire_time = r->expire_time;
        item.bind_on_equip_type = r->bind_on_equip_type;
        item.view = r->view;
        view_item(&item);
        item_info_destruct(&item);
    }
    read_look(pkt, 0);
    open_window();
}

void game_mode_equip_win_microscope4(const unsigned char *pkt)
{
    const struct PACKET_ZC_EQUIPWIN_MICROSCOPE_LOOK *packet =
        (const struct PACKET_ZC_EQUIPWIN_MICROSCOPE_LOOK *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    clear();
    for (n = record_count(pkt, 0x2d, 37); n > 0; n--, r += 37) {
        item_from_equip_record4(&item, r);
        view_item(&item);
        item_info_destruct(&item);
    }
    read_look(pkt, 0);
    open_window();
}

void game_mode_equip_win_microscope_v5(const unsigned char *pkt)
{
    const struct PACKET_ZC_EQUIPWIN_MICROSCOPE_LOOK *packet =
        (const struct PACKET_ZC_EQUIPWIN_MICROSCOPE_LOOK *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    clear();
    for (n = record_count(pkt, 0x2d, 41); n >= 1; n--, r += 41) {
        item_from_v5_record(&item, r);
        view_item(&item);
        item_info_destruct(&item);
    }
    read_look(pkt, 0);
    open_window();
}

static void view_checking_eggs(const struct ItemInfo *item)
{
    game_mode_note_hatched_egg(item);
    view_item(item);
    game_mode_check_pet_auto_feed(item);
}

static void v6_items(const unsigned char *pkt, int look_size)
{
    int n;
    const unsigned char *r = pkt + look_size;
    struct ItemInfo item;

    clear();
    for (n = record_count(pkt, look_size, 67); n > 0; n--, r += 67) {
        item_from_v6_record(&item, r);
        view_checking_eggs(&item);
        item_info_destruct(&item);
    }
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
}

void game_mode_equip_win_microscope_v6(const unsigned char *pkt)
{
    v6_items(pkt, 0x2d);
    read_look(pkt, 0);
    open_window();
}

void game_mode_equip_win_microscope_v6_body(const unsigned char *pkt)
{
    v6_items(pkt, 0x2f);
    read_look(pkt, 1);
    open_window();
}

void game_mode_equip_win_microscope_v7(const unsigned char *pkt)
{
    const struct PACKET_ZC_EQUIPWIN_MICROSCOPE_LOOK_BODY *packet =
        (const struct PACKET_ZC_EQUIPWIN_MICROSCOPE_LOOK_BODY *)pkt;
    int n;
    const unsigned char *r = packet->records;
    struct ItemInfo item;

    clear();
    for (n = record_count(pkt, 0x2f, 68); n > 0; n--, r += 68) {
        item_from_v7_record(&item, r);
        view_checking_eggs(&item);
        item_info_destruct(&item);
    }
    ui_window_mgr_refresh_item_windows(&g_windowMgr);
    read_look(pkt, 1);
    open_window();
}
