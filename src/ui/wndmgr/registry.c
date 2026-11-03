#include "platform/client_config.h"
#include "session/state.h"
#include <string.h>
#include "ui/wndmgr/wndmgr.h"
#include "session/session_names.h"
#include "session/session_options.h"
#include "session/session_state.h"

const char *g_regShortcutItemKey = "\\ShortcutItem";
const char *g_regUIRectInfoKey = "\\UIRectInfo";

static void shortcut_key_path(char *path)
{
    const char *server = string_cstr(&g_session1.serverName);

    strcpy(path, g_registryKeyPath);
    strcat(path, g_regShortcutItemKey);
    strcat(path, "\\");
    if (strstr(server, SAKRAY))
        strcat(path, SAKRAY);
    else
        strcat(path, server);
}

static int open_twice(const char *path, HKEY *key)
{
    HKEY probe;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &probe) != ERROR_SUCCESS)
        return 0;
    RegCloseKey(probe);
    return RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, key) == ERROR_SUCCESS;
}

void ui_window_mgr_init_shortcut_item_info_from_reg(struct UIWindowMgr *self)
{
    char path[0x200];
    char value_name[0x24];
    char name[0x21];
    DWORD type, size;
    HKEY key;
    int slot, is_skill, level;

    (void)self;
    shortcut_key_path(path);
    if (open_twice(path, &key)) {
        session_clear_shorten_item_name(&g_session1);
        session_clear_shorten_skill_levels(&g_session1);
        for (slot = 0; slot < SHORTCUT_SLOTS; slot++) {
            type = REG_SZ;
            size = 0x20;
            sprintf(value_name, "SITEM%d%d%d", slot, CLIENT_LANGTYPE, g_session1.shortcutCharSlot);
            memset(name, 0, sizeof name);
            is_skill = 0;
            g_session1.shortenSkillPage = (BYTE)(slot / SHORTCUT_PAGE_SLOTS);
            if (RegQueryValueExA(key, value_name, NULL, &type, (BYTE *)name, &size) != ERROR_SUCCESS)
                session_set_shorten_item_name(&g_session1, "nothing", slot);
            else if (name[0] == '#')
                is_skill = 1;
            else
                session_set_shorten_item_name(&g_session1, name, slot);

            type = REG_DWORD;
            size = 4;
            sprintf(value_name, "S_SKILL_USE_LEVEL%d%d%d", slot, CLIENT_LANGTYPE, g_session1.shortcutCharSlot);
            level = 0;
            if (RegQueryValueExA(key, value_name, NULL, &type, (BYTE *)&level, &size) == ERROR_SUCCESS) {
                session_set_shorten_skill_id(&g_session1, level, slot);
            } else {
                session_set_shorten_skill_id(&g_session1, 0, slot);
                level = 0;
            }
            if (is_skill)
                session_set_shorten_skill_name(&g_session1, name, level, slot);
        }
        RegCloseKey(key);
    }
    g_session1.shortenSkillPage = 0;
}

#define GET(name, field) RegQueryValueExA(key, name, NULL, &type, (BYTE *)(field), &size)
#define GET_RECT(name, info) \
    (GET(name ".X", &(info)->x), GET(name ".Y", &(info)->y), \
     GET(name ".W", &(info)->width), GET(name ".H", &(info)->height))
#define GET_OR(name, field, dflt) do { if (GET(name, field) != ERROR_SUCCESS) *(field) = (dflt); } while (0)

void ui_window_mgr_init_ui_rect_info_from_reg(struct UIWindowMgr *self)
{
    char path[0x200];
    char value_name[0x100];
    DWORD type, size;
    HKEY key, probe;
    int i;

    strcpy(path, g_registryKeyPath);
    strcat(path, g_regUIRectInfoKey);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &probe) != ERROR_SUCCESS) {
        ui_window_mgr_set_default_rect_info(self);
        return;
    }
    RegCloseKey(probe);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return;

    type = 4;
    size = 4;
    GET("m_minimapZoomWnd", &self->minimap_zoom);
    if (GET("M_ISDRAGALL", &self->drag_all) != ERROR_SUCCESS || self->drag_all != 0)
        self->drag_all = 1;
    GET_OR("M_ISDRAWCOMPASS", &self->draw_compass, 0);
    if (GET("M_MINIMAPARGB", &self->minimap_argb) != ERROR_SUCCESS || self->minimap_argb != 0xffffffff)
        self->minimap_argb = 0xaaffffff;
    GET("M_CHATWNDSTATUS", &self->chat_wnd_status);
    GET("M_CHATWNDHEIGHT", &self->chat_wnd_height);
    GET("M_CHATWNDX", &self->chat_wnd_x);
    GET("M_CHATWNDY", &self->chat_wnd_y);
    GET("M_CHATWNDSHOW", &self->chat_wnd_show);
    GET_RECT("SAYDIALOGWNDINFO", &self->say_dialog_info);
    GET_RECT("CHOOSEWNDINFO", &self->choose_info);
    GET_RECT("BASICINFOWNDINFO", &self->basic_info_info.rect);
    GET("BASICINFOWNDINFO.ISSHOW", &self->basic_info_info.show);
    GET_RECT("ITEMWNDINFO", &self->item_info.rect);
    GET("ITEMWNDINFO.ORGHEIGHT", &self->item_info.org_height);
    GET("ITEMWNDINFO.CURTAB", &self->item_info.cur_tab);
    GET("ITEMWNDINFO.CURRADIO1", &self->item_info.cur_radio[0]);
    GET("ITEMWNDINFO.CURRADIO2", &self->item_info.cur_radio[1]);
    GET("ITEMWNDINFO.CURRADIO3", &self->item_info.cur_radio[2]);
    GET("ITEMWNDINFO.SHOW", &self->item_info.show);
    GET_RECT("QUESTWNDINFO", &self->quest_info.rect);
    GET("QUESTWNDINFO.ORGHEIGHT", &self->quest_info.org_height);
    GET("QUESTWNDINFO.CURTAB", &self->quest_info.cur_tab);
    GET("QUESTWNDINFO.SHOW", &self->quest_info.show);
    GET("QUESTDISPWNDINFO.QUESTDISPLAY", &self->quest_display);
    GET_RECT("STATUSWNDINFO", &self->status_info.rect);
    GET("STATUSWNDINFO.ORGHEIGHT", &self->status_info.org_height);
    GET_OR("STATUSWNDINFO.SHOW", &self->status_info.show, 0);
    GET_RECT("EQUIPWNDINFO", &self->equip_info.rect);
    GET("EQUIPWNDINFO.ORGHEIGHT", &self->equip_info.org_height);
    GET_OR("EQUIPWNDINFO.SHOW", &self->equip_info.show, 0);
    GET_RECT("OPTIONWNDINFO", &self->option_info.rect);
    GET("OPTIONWNDINFO.ORGHEIGHT", &self->option_info.org_height);
    GET_OR("OPTIONWNDINFO.SHOW", &self->option_info.show, 0);
    GET_RECT("SHORTENITEMWNDINFO", &self->shorten_item_info.rect);
    GET_OR("SHORTENITEMWNDINFO.SHOW", &self->shorten_item_info.show, 0);
    GET_RECT("SHORTCUTWNDINFO", &self->shortcut_info.rect);
    GET_OR("SHORTCUTWNDINFO.SHOW", &self->shortcut_info.show, 0);
    GET_RECT("CHATROOMWNDINFO", &self->chat_room_info);
    GET_RECT("ITEMSTOREWNDINFO", &self->item_store_info.rect);
    GET("ITEMSTOREWNDINFO.subWnd", &self->item_store_info.sub_wnd);
    memset(value_name, 0, 0xff);
    GET("ITEMSTOREWNDNUM", &self->item_store_wnd_num);
    if (self->item_store_wnd_num != ITEM_STORE_SUB_WNDS) {
        for (i = 0; i < ITEM_STORE_SUB_WNDS; i++)
            item_store_sub_info_set_default(&self->item_store_info.sub_info[i], i);
        self->item_store_wnd_num = ITEM_STORE_SUB_WNDS;
        item_store_wnd_info_set_default(&self->item_store_info);
    } else {
        for (i = 0; i < ITEM_STORE_SUB_WNDS; i++) {
            struct WindowInfo *sub = &self->item_store_info.sub_info[i];
            sprintf(value_name, "ITEMSTOREWNDINFO_%d.X", i);
            GET(value_name, &sub->x);
            sprintf(value_name, "ITEMSTOREWNDINFO_%d.Y", i);
            GET(value_name, &sub->y);
            sprintf(value_name, "ITEMSTOREWNDINFO_%d.W", i);
            GET(value_name, &sub->width);
            sprintf(value_name, "ITEMSTOREWNDINFO_%d.H", i);
            GET(value_name, &sub->height);
        }
    }
    GET("ITEMSTOREWNDINFO.findWnd", &self->item_store_info.find_wnd);
    GET_RECT("ITEMSTOREFINDWNDINFO", &self->item_store_find_info);
    GET_RECT("ITEMSHOPWNDINFO", &self->item_shop_info);
    GET_RECT("ITEMSELLWNDINFO", &self->item_sell_info);
    GET_RECT("ITEMPURCHASEWNDINFO", &self->item_purchase_info);
    GET_RECT("MESSENGERGROUPWNDINFO", &self->messenger_group_info.rect);
    GET_OR("MESSENGERGROUPWNDINFO.SHOW", &self->messenger_group_info.show, 0);
    GET_OR("MESSENGERGROUPWNDINFO.RADIO", &self->messenger_group_info.radio, 1);
    GET_OR("MESSENGERGROUPWNDINFO.SIZEMODE", &self->messenger_group_info.size_mode, 0);
    GET_RECT("EXCHANGEACCEPTWNDINFO", &self->exchange_accept_info);
    GET_RECT("SKILLLISTWNDINFO", &self->skill_list_info.rect);
    GET_OR("SKILLLISTWNDINFO.SHOW", &self->skill_list_info.show, 0);
    GET_RECT("MERCHANTITEMWNDINFO", &self->merchant_item_info.rect);
    GET_OR("MERCHANTITEMWNDINFO.SHOW", &self->merchant_item_info.show, 0);
    GET_RECT("MERCHANTMIRRORITEMWNDINFO", &self->merchant_mirror_item_info);
    GET_RECT("MERCHANTSHOPMAKEWNDINFO", &self->merchant_shop_make_info);
    GET_RECT("MERCHANTITEMSHOPWNDINFO", &self->merchant_item_shop_info);
    GET_RECT("MERCHANTITEMMYSHOPWNDINFO", &self->merchant_item_my_shop_info);
    GET_RECT("MERCHANTITEMPURCHASEWNDINFO", &self->merchant_item_purchase_info);
    GET_RECT("ITEMCOLLECTIONWNDINFO", &self->item_collection_info);
    GET_RECT("COMBINEDCARDITEMCOLLECTIONWNDINFO", &self->combined_card_item_collection_info);
    GET_RECT("PARTYSETTINGWNDINFO", &self->party_setting_info);
    GET_RECT("DETAILLEVELWNDINFO", &self->detail_level_info);
    GET_RECT("WHISPERWNDINFO", &self->whisper_info);
    GET_RECT("FRIENDOPTIONWNDINFO", &self->friend_option_info);
    GET_RECT("ITEMCOMPOSITIONWNDINFO", &self->item_composition_info);
    GET_RECT("ITEMIDENTIFYWNDINFO", &self->item_identify_info);
    GET("CHATWNDSTICKON", &self->chat_wnd_stick_on);
    GET("MEMORIALDUNWND.X", &g_session1.mdWndX);
    GET("MEMORIALDUNWND.Y", &g_session1.mdWndY);
    GET_RECT("QUICKSLOTWNDINFO", &self->quick_slot_info);
    GET_RECT("GUILDHELPERWNDINFO", &self->guild_helper_info);
    GET_RECT("BUYINGSTOREMAKEWNDINFO", &self->buying_store_make_info);
    GET_RECT("BUYINGSTOREMIRRORITEMWNDINFO", &self->buying_store_mirror_item_info);
    GET_RECT("BUYINGSTOREITEMMYSHOPWNDINFO", &self->buying_store_item_my_shop_info);
    GET_RECT("BUYINGSTOREITEMSHOPWNDINFO", &self->buying_store_item_shop_info);
    GET_RECT("BUYINGSTOREITEMSELLWNDINFO", &self->buying_store_item_sell_info);
    GET_RECT("BUYINGSTOREMIRRORSELLLISTWNDINFO", &self->buying_store_mirror_sell_list_info);
    GET_RECT("NAVIGATIONWNDINFO", &self->navigation_info.rect);
    GET("NAVIGATIONWNDINFO.ISSCREEN", &self->navigation_info.is_screen);
    mini_map_info_set_default(&self->minimap_info);
    GET("MINIMAPWNDINFO.X", &self->minimap_info.x);
    GET("MINIMAPWNDINFO.Y", &self->minimap_info.y);
    GET("MINIMAPWNDINFO.SHOWQUEST", &self->minimap_info.show_quest);
    GET("MINIMAPWNDINFO.WHOWNPC", &self->minimap_info.show_npc);
    GET("MINIMAPWNDINFO.WHOWMEMBER", &self->minimap_info.show_member);
    GET("ACHTRACINGWNDINFO.X", &self->ach_tracing_pos.x);
    GET("ACHTRACINGWNDINFO.Y", &self->ach_tracing_pos.y);
    GET("PREVIEWEQUIPWNDINFO.X", &self->preview_equip_pos.x);
    GET("PREVIEWEQUIPWNDINFO.Y", &self->preview_equip_pos.y);
    GET_RECT("DETAILLEVELWNDINFO", &self->graphic_setting_info);
    GET_RECT("PETINFOWNDINFO", &self->pet_info_info);
    GET_RECT("HOMUNINFOWNDINFO", &self->homun_info_info);
    GET_RECT("MERINFOWNDINFO", &self->mer_info_info);
    GET_RECT("ADVEN_REGISTERWNDINFO", &self->adven_register_info);
    GET_RECT("ADVEN_JOINWNDINFO", &self->adven_join_info);
    GET_RECT("ADVEN_PARTYBOARDWNDINFO", &self->adven_party_board_info);
    RegCloseKey(key);
}
#undef GET
#undef GET_RECT
#undef GET_OR

void ui_window_mgr_write_shortcut_item_info_to_reg(struct UIWindowMgr *self)
{
    char path[0x200];
    char value_name[0x24];
    char name[0x21];
    DWORD disposition = 0;
    HKEY key;
    int slot, skill_id;

    (void)self;
    shortcut_key_path(path);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, &disposition)
        != ERROR_SUCCESS)
        return;
    for (slot = 0; slot < SHORTCUT_SLOTS; slot++) {
        memset(name, 0, sizeof name);
        strcpy(name, session_get_shorten_item_name(&g_session1, slot));
        sprintf(value_name, "SITEM%d%d%d", slot, CLIENT_LANGTYPE, g_session1.shortcutCharSlot);
        RegSetValueExA(key, value_name, 0, REG_SZ, (const BYTE *)name, 0x20);
        skill_id = session_get_shorten_skill_id(&g_session1, slot);
        sprintf(value_name, "S_SKILL_USE_LEVEL%d%d%d", slot, CLIENT_LANGTYPE, g_session1.shortcutCharSlot);
        RegSetValueExA(key, value_name, 0, REG_DWORD, (const BYTE *)&skill_id, 4);
    }
    RegCloseKey(key);
}

#define SET(name, field) RegSetValueExA(key, name, 0, REG_DWORD, (const BYTE *)(field), 4)
#define SET_RECT(name, info) \
    (SET(name ".X", &(info)->x), SET(name ".Y", &(info)->y), \
     SET(name ".W", &(info)->width), SET(name ".H", &(info)->height))

void ui_window_mgr_write_ui_rect_info_to_reg(struct UIWindowMgr *self)
{
    char path[0x200];
    char value_name[0x100];
    DWORD disposition = 0;
    HKEY key;
    int i;

    strcpy(path, g_registryKeyPath);
    strcat(path, g_regUIRectInfoKey);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &key, &disposition)
        != ERROR_SUCCESS)
        return;
    SET("m_minimapZoomWnd", &self->minimap_zoom);
    SET("M_ISDRAGALL", &self->drag_all);
    SET("M_ISDRAWCOMPASS", &self->draw_compass);
    SET("M_MINIMAPARGB", &self->minimap_argb);
    SET("M_CHATWNDSTATUS", &self->chat_wnd_status);
    SET("M_CHATWNDHEIGHT", &self->chat_wnd_height);
    SET("M_CHATWNDX", &self->chat_wnd_x);
    SET("M_CHATWNDY", &self->chat_wnd_y);
    SET("M_CHATWNDSHOW", &self->chat_wnd_show);
    SET_RECT("SAYDIALOGWNDINFO", &self->say_dialog_info);
    SET_RECT("CHOOSEWNDINFO", &self->choose_info);
    SET_RECT("BASICINFOWNDINFO", &self->basic_info_info.rect);
    SET("BASICINFOWNDINFO.ISSHOW", &self->basic_info_info.show);
    SET_RECT("ITEMWNDINFO", &self->item_info.rect);
    SET("ITEMWNDINFO.ORGHEIGHT", &self->item_info.org_height);
    SET("ITEMWNDINFO.CURTAB", &self->item_info.cur_tab);
    SET("ITEMWNDINFO.CURRADIO1", &self->item_info.cur_radio[0]);
    SET("ITEMWNDINFO.CURRADIO2", &self->item_info.cur_radio[1]);
    SET("ITEMWNDINFO.CURRADIO3", &self->item_info.cur_radio[2]);
    SET("ITEMWNDINFO.SHOW", &self->item_info.show);
    SET_RECT("QUESTWNDINFO", &self->quest_info.rect);
    SET("QUESTWNDINFO.ORGHEIGHT", &self->quest_info.org_height);
    SET("QUESTWNDINFO.CURTAB", &self->quest_info.cur_tab);
    SET("QUESTWNDINFO.SHOW", &self->quest_info.show);
    SET("QUESTDISPWNDINFO.QUESTDISPLAY", &self->quest_display);
    SET_RECT("STATUSWNDINFO", &self->status_info.rect);
    SET("STATUSWNDINFO.ORGHEIGHT", &self->status_info.org_height);
    SET("STATUSWNDINFO.SHOW", &self->status_info.show);
    SET_RECT("EQUIPWNDINFO", &self->equip_info.rect);
    SET("EQUIPWNDINFO.ORGHEIGHT", &self->equip_info.org_height);
    SET("EQUIPWNDINFO.SHOW", &self->equip_info.show);
    SET_RECT("OPTIONWNDINFO", &self->option_info.rect);
    SET("OPTIONWNDINFO.ORGHEIGHT", &self->option_info.org_height);
    SET("OPTIONWNDINFO.SHOW", &self->option_info.show);
    SET_RECT("SHORTENITEMWNDINFO", &self->shorten_item_info.rect);
    SET("SHORTENITEMWNDINFO.SHOW", &self->shorten_item_info.show);
    SET_RECT("SHORTCUTWNDINFO", &self->shortcut_info.rect);
    SET("SHORTCUTWNDINFO.SHOW", &self->shortcut_info.show);
    SET_RECT("CHATROOMWNDINFO", &self->chat_room_info);
    SET_RECT("ITEMSTOREWNDINFO", &self->item_store_info.rect);
    SET("ITEMSTOREWNDINFO.subWnd", &self->item_store_info.sub_wnd);
    memset(value_name, 0, 0xff);
    SET("ITEMSTOREWNDNUM", &self->item_store_wnd_num);
    for (i = 0; i < ITEM_STORE_SUB_WNDS; i++) {
        struct WindowInfo *sub = &self->item_store_info.sub_info[i];
        sprintf(value_name, "ITEMSTOREWNDINFO_%d.X", i);
        SET(value_name, &sub->x);
        sprintf(value_name, "ITEMSTOREWNDINFO_%d.Y", i);
        SET(value_name, &sub->y);
        sprintf(value_name, "ITEMSTOREWNDINFO_%d.W", i);
        SET(value_name, &sub->width);
        sprintf(value_name, "ITEMSTOREWNDINFO_%d.H", i);
        SET(value_name, &sub->height);
    }
    SET("ITEMSTOREWNDINFO.findWnd", &self->item_store_info.find_wnd);
    SET_RECT("ITEMSTOREFINDWNDINFO", &self->item_store_find_info);
    SET_RECT("ITEMSHOPWNDINFO", &self->item_shop_info);
    SET_RECT("ITEMSELLWNDINFO", &self->item_sell_info);
    SET_RECT("ITEMPURCHASEWNDINFO", &self->item_purchase_info);
    SET_RECT("MESSENGERGROUPWNDINFO", &self->messenger_group_info.rect);
    SET("MESSENGERGROUPWNDINFO.SHOW", &self->messenger_group_info.show);
    SET("MESSENGERGROUPWNDINFO.RADIO", &self->messenger_group_info.radio);
    SET("MESSENGERGROUPWNDINFO.SIZEMODE", &self->messenger_group_info.size_mode);
    SET_RECT("EXCHANGEACCEPTWNDINFO", &self->exchange_accept_info);
    SET_RECT("SKILLLISTWNDINFO", &self->skill_list_info.rect);
    SET("SKILLLISTWNDINFO.SHOW", &self->skill_list_info.show);
    SET_RECT("MERCHANTITEMWNDINFO", &self->merchant_item_info.rect);
    SET("MERCHANTITEMWNDINFO.SHOW", &self->merchant_item_info.show);
    SET_RECT("MERCHANTMIRRORITEMWNDINFO", &self->merchant_mirror_item_info);
    SET_RECT("MERCHANTSHOPMAKEWNDINFO", &self->merchant_shop_make_info);
    SET_RECT("MERCHANTITEMSHOPWNDINFO", &self->merchant_item_shop_info);
    SET_RECT("MERCHANTITEMMYSHOPWNDINFO", &self->merchant_item_my_shop_info);
    SET_RECT("MERCHANTITEMPURCHASEWNDINFO", &self->merchant_item_purchase_info);
    SET_RECT("ITEMCOLLECTIONWNDINFO", &self->item_collection_info);
    SET_RECT("COMBINEDCARDITEMCOLLECTIONWNDINFO", &self->combined_card_item_collection_info);
    SET_RECT("PARTYSETTINGWNDINFO", &self->party_setting_info);
    SET_RECT("DETAILLEVELWNDINFO", &self->detail_level_info);
    SET_RECT("WHISPERWNDINFO", &self->whisper_info);
    SET_RECT("FRIENDOPTIONWNDINFO", &self->friend_option_info);
    SET_RECT("ITEMCOMPOSITIONWNDINFO", &self->item_composition_info);
    SET_RECT("ITEMIDENTIFYWNDINFO", &self->item_identify_info);
    SET("CHATWNDSTICKON", &self->chat_wnd_stick_on);
    SET("MEMORIALDUNWND.X", &g_session1.mdWndX);
    SET("MEMORIALDUNWND.Y", &g_session1.mdWndY);
    SET_RECT("QUICKSLOTWNDINFO", &self->quick_slot_info);
    SET_RECT("GUILDHELPERWNDINFO", &self->guild_helper_info);
    SET_RECT("BUYINGSTOREMAKEWNDINFO", &self->buying_store_make_info);
    SET_RECT("BUYINGSTOREMIRRORITEMWNDINFO", &self->buying_store_mirror_item_info);
    SET_RECT("BUYINGSTOREITEMMYSHOPWNDINFO", &self->buying_store_item_my_shop_info);
    SET_RECT("BUYINGSTOREITEMSHOPWNDINFO", &self->buying_store_item_shop_info);
    SET_RECT("BUYINGSTOREITEMSELLWNDINFO", &self->buying_store_item_sell_info);
    SET_RECT("BUYINGSTOREMIRRORSELLLISTWNDINFO", &self->buying_store_mirror_sell_list_info);
    SET_RECT("NAVIGATIONWNDINFO", &self->navigation_info.rect);
    SET("NAVIGATIONWNDINFO.ISSCREEN", &self->navigation_info.is_screen);
    SET("MINIMAPWNDINFO.X", &self->minimap_info.x);
    SET("MINIMAPWNDINFO.Y", &self->minimap_info.y);
    SET("MINIMAPWNDINFO.SHOWQUEST", &self->minimap_info.show_quest);
    SET("MINIMAPWNDINFO.WHOWNPC", &self->minimap_info.show_npc);
    SET("MINIMAPWNDINFO.WHOWMEMBER", &self->minimap_info.show_member);
    SET("ACHTRACINGWNDINFO.X", &self->ach_tracing_pos.x);
    SET("ACHTRACINGWNDINFO.Y", &self->ach_tracing_pos.y);
    SET("PREVIEWEQUIPWNDINFO.X", &self->preview_equip_pos.x);
    SET("PREVIEWEQUIPWNDINFO.Y", &self->preview_equip_pos.y);
    SET_RECT("DETAILLEVELWNDINFO", &self->graphic_setting_info);
    SET_RECT("PETINFOWNDINFO", &self->pet_info_info);
    SET_RECT("HOMUNINFOWNDINFO", &self->homun_info_info);
    SET_RECT("MERINFOWNDINFO", &self->mer_info_info);
    SET_RECT("ADVEN_REGISTERWNDINFO", &self->adven_register_info);
    SET_RECT("ADVEN_JOINWNDINFO", &self->adven_join_info);
    SET_RECT("ADVEN_PARTYBOARDWNDINFO", &self->adven_party_board_info);
    RegCloseKey(key);
}
#undef SET
#undef SET_RECT
