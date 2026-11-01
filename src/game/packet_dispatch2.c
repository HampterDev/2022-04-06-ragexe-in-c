#include "contents/adventurer_agency.h"
#include "game/actor_packets.h"
#include "game/barter_packets.h"
#include "game/chat_room.h"
#include "game/companion_packets.h"
#include "game/connection.h"
#include "game/entry_packet_handlers.h"
#include "game/ground_item_packets.h"
#include "game/item_list_packets.h"
#include "game/item_upgrade.h"
#include "game/macro_detector.h"
#include "game/navigation.h"
#include "game/packet_data.h"
#include "game/packet_handlers.h"
#include "game/packets_macro_detector.h"
#include "game/party_booking_packets.h"
#include "game/quest_packets.h"
#include "platform/error_message.h"
#include "session/skill_commands.h"
#include <string.h>
#include "game/captcha_packets.h"
#include "game/store_entries.h"
#include "game/user_config.h"
#include "game/ranking_packets.h"
#include "game/skill_cooldowns.h"
#include "contents/connection_api.h"
#include "game/gamemode.h"
#include "game/packet_ids.h"
#include "game/packet_dispatch.h"
#include "game/client.h"
#include "diag/diag.h"
#include "core/packet_table.h"

#define WID_REFINING_UI     0x127

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_ACT3 {
    unsigned short packet_type;
    int aid;
    int target;
    int start_time;
    int attack_mt;
    int attacked_mt;
    int damage;
    unsigned char is_sp_damage;
    unsigned short count;
    unsigned char action;
    int left_damage;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NOTIFY_ACT3, 34);

#pragma pack(push, 1)
struct PACKET_ZC_STOPMOVE_FORCE {
    unsigned short packet_type;
    int aid;
    unsigned short x;
    unsigned short y;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_STOPMOVE_FORCE, 10);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_WEAR_EQUIP_V5 {
    unsigned short packet_type;
    unsigned short index;
    int wear_location;
    unsigned short view;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_WEAR_EQUIP_V5, 11);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_TAKEOFF_EQUIP_V5 {
    unsigned short packet_type;
    unsigned short index;
    int wear_location;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_TAKEOFF_EQUIP_V5, 9);

#pragma pack(push, 1)
struct PACKET_ZC_USE_SKILL2 {
    unsigned short packet_type;
    unsigned short skill_id;
    int level;
    int target_aid;
    int src_aid;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_USE_SKILL2, 17);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_WHISPER02 {
    unsigned short packet_type;
    unsigned char result;
    int receiver_cid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_WHISPER02, 7);

#pragma pack(push, 1)
struct PACKET_ZC_ALL_ACH_LIST {
    unsigned short packet_type;
    unsigned short packet_length;
    int count;
    int total_points;
    unsigned short rank;
    int current_points;
    int next_points;
    unsigned char entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ALL_ACH_LIST, 22);

#pragma pack(push, 1)
struct PACKET_ZC_ACH_UPDATE {
    unsigned short packet_type;
    int total_points;
    unsigned short rank;
    int current_points;
    int next_points;
    int id;
    unsigned char completed;
    int objectives[10];
    int completed_at;
    unsigned char reward;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACH_UPDATE, 66);

#pragma pack(push, 1)
struct PACKET_ZC_REQ_ACH_REWARD_ACK {
    unsigned short packet_type;
    unsigned char result;
    int id;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REQ_ACH_REWARD_ACK, 7);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_OPENSTORE2 {
    unsigned short packet_type;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_OPENSTORE2, 3);

#pragma pack(push, 1)
struct PACKET_ZC_REQ_AU_BOT {
    unsigned short packet_type;
    int aid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REQ_AU_BOT, 6);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_CHANGE_TITLE {
    unsigned short packet_type;
    unsigned char result;
    int title_id;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_CHANGE_TITLE, 7);

#pragma pack(push, 1)
struct PACKET_ZC_UI_OPEN {
    unsigned short packet_type;
    unsigned char type;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_UI_OPEN, 3);

#pragma pack(push, 1)
struct PACKET_ZC_EF_WARNING_PLANE {
    unsigned short packet_type;
    int aid;
    unsigned short skill_id;
    short level;
    unsigned short x;
    unsigned short y;
    int duration;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_EF_WARNING_PLANE, 18);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_PRIVATE_AIRPLANE_CALL {
    unsigned short packet_type;
    int result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_PRIVATE_AIRPLANE_CALL, 6);

#pragma pack(push, 1)
struct PACKET_ZC_MAPMOVE_PRIVATE_AIRPLANE {
    unsigned short packet_type;
    char map_name[16];
    unsigned short x;
    unsigned short y;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MAPMOVE_PRIVATE_AIRPLANE, 22);

#pragma pack(push, 1)
struct PACKET_ZC_SERVERMOVE_PRIVATE_AIRPLANE {
    unsigned short packet_type;
    char map_name[16];
    unsigned short x;
    unsigned short y;
    int ip;
    unsigned short port;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SERVERMOVE_PRIVATE_AIRPLANE, 28);

#pragma pack(push, 1)
struct PACKET_ZC_RANDOM_COMBINE_ITEM_UI_OPEN {
    unsigned short packet_type;
    int value;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_RANDOM_COMBINE_ITEM_UI_OPEN, 6);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_RANDOM_COMBINE_ITEM {
    unsigned short packet_type;
    unsigned short result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_RANDOM_COMBINE_ITEM, 4);

#pragma pack(push, 1)
struct PACKET_ZC_CHECK_RECEIVE_CHARACTER_NAME2 {
    unsigned short packet_type;
    int char_id;
    unsigned short job;
    unsigned short level;
    char name[24];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_CHECK_RECEIVE_CHARACTER_NAME2, 34);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_PLAYER_AID_IN_RANGE {
    unsigned short packet_type;
    short packet_length;
    int aids[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_PLAYER_AID_IN_RANGE, 4);

#pragma pack(push, 1)
struct PACKET_ZC_MSG_VALUE_STRING {
    unsigned short packet_type;
    short packet_length;
    unsigned short msg_id;
    int color;
    char value[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_MSG_VALUE_STRING, 10);

#pragma pack(push, 1)
struct PACKET_ZC_OPEN_UI_ASSISTANT_VENDING {
    unsigned short packet_type;
    short packet_length;
    unsigned char slots;
    unsigned char items[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_OPEN_UI_ASSISTANT_VENDING, 5);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_CREATE_ASSISTANT {
    unsigned short packet_type;
    unsigned char kind;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_CREATE_ASSISTANT, 4);

#pragma pack(push, 1)
struct PACKET_ZC_STORE_ASSISTANT_DISAPPEAR {
    unsigned short packet_type;
    int aid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_STORE_ASSISTANT_DISAPPEAR, 6);

#pragma pack(push, 1)
struct PACKET_ZC_ITEMLIST_FROM_ASSISTANT_VENDING {
    unsigned short packet_type;
    short packet_length;
    int owner_aid;
    int store_id;
    unsigned char is_mine;
    int duration;
    struct VendingEntry entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ITEMLIST_FROM_ASSISTANT_VENDING, 17);

#pragma pack(push, 1)
struct PACKET_ZC_ITEMLIST_FROM_ASSISTANT_BUYING {
    unsigned short packet_type;
    short packet_length;
    int owner_aid;
    int store_id;
    unsigned char is_mine;
    int duration;
    int zeny_limit;
    unsigned char skipped[4];
    struct BuyingStoreEntry entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ITEMLIST_FROM_ASSISTANT_BUYING, 25);

#pragma pack(push, 1)
struct PACKET_ZC_OPEN_UI_ASSISTANT_BUYING {
    unsigned short packet_type;
    unsigned char slots;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_OPEN_UI_ASSISTANT_BUYING, 3);

#pragma pack(push, 1)
struct PACKET_ZC_CONFIG_NOTIFY2 {
    unsigned short packet_type;
    unsigned char open_equip_window;
    unsigned char call_allowed;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_CONFIG_NOTIFY2, 4);

#pragma pack(push, 1)
struct PACKET_ZC_REFINING_MATERIAL_LIST {
    unsigned short packet_type;
    short packet_length;
    unsigned short index;
    unsigned char blessing_count;
    unsigned char materials[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REFINING_MATERIAL_LIST, 7);

#pragma pack(push, 1)
struct PACKET_ZC_CONFIG_NOTIFY3 {
    unsigned short packet_type;
    unsigned char open_equip_window;
    unsigned char call_allowed;
    unsigned char pet_auto_feed;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_CONFIG_NOTIFY3, 5);

#pragma pack(push, 1)
struct PACKET_ZC_NPC_MARKETGEAR_OPEN {
    unsigned short packet_type;
    short packet_length;
    unsigned char items[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NPC_MARKETGEAR_OPEN, 4);

#pragma pack(push, 1)
struct PACKET_ZC_NPC_MARKETGEAR_PURCHASE_RESULT {
    unsigned short packet_type;
    short packet_length;
    unsigned char result;
    unsigned char items[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NPC_MARKETGEAR_PURCHASE_RESULT, 5);

#pragma pack(push, 1)
struct PACKET_ZC_RANDOM_UPGRADE_ITEM_UI_OPEN {
    unsigned short packet_type;
    int value;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_RANDOM_UPGRADE_ITEM_UI_OPEN, 6);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_RANDOM_UPGRADE_ITEM {
    unsigned short packet_type;
    unsigned short result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_RANDOM_UPGRADE_ITEM, 4);

#pragma pack(push, 1)
struct PACKET_ZC_BROADCAST_ITEMREFINING_RESULT {
    unsigned short packet_type;
    char name[24];
    int item_id;
    unsigned char refine;
    unsigned char success;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_BROADCAST_ITEMREFINING_RESULT, 32);

#pragma pack(push, 1)
struct PACKET_ZC_CONFIG_NOTIFY4 {
    unsigned short packet_type;
    unsigned char open_equip_window;
    unsigned char call_allowed;
    unsigned char pet_auto_feed;
    unsigned char homun_auto_feed;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_CONFIG_NOTIFY4, 6);

#pragma pack(push, 1)
struct PACKET_ZC_UI_OPEN_V2 {
    unsigned short packet_type;
    unsigned char type;
    int value;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_UI_OPEN_V2, 7);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_CHECK_ATTENDANCE {
    unsigned short packet_type;
    int result;
    int day;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_CHECK_ATTENDANCE, 10);

#pragma pack(push, 1)
struct PACKET_ZC_SET_CLAIRVOYANCE {
    unsigned short packet_type;
    unsigned char on;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SET_CLAIRVOYANCE, 3);

#pragma pack(push, 1)
struct PACKET_ZC_AUTOSPELLLIST2 {
    unsigned short packet_type;
    short packet_length;
    int skills[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_AUTOSPELLLIST2, 4);

#pragma pack(push, 1)
struct PACKET_ZC_SPLIT_SEND_ITEMLIST_NORMAL {
    unsigned short packet_type;
    short packet_length;
    unsigned char type;
    unsigned char items[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SPLIT_SEND_ITEMLIST_NORMAL, 5);

#pragma pack(push, 1)
struct PACKET_ZC_SPLIT_SEND_ITEMLIST_EQUIP {
    unsigned short packet_type;
    short packet_length;
    unsigned char type;
    unsigned char items[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SPLIT_SEND_ITEMLIST_EQUIP, 5);

#pragma pack(push, 1)
struct PACKET_ZC_SPLIT_SEND_ITEMLIST_RESULT {
    unsigned short packet_type;
    unsigned char type;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SPLIT_SEND_ITEMLIST_RESULT, 4);

#pragma pack(push, 1)
struct PACKET_ZC_ADVANCED_STATUS_CHANGE_ACK {
    unsigned short packet_type;
    unsigned short status;
    unsigned char result;
    unsigned char value;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ADVANCED_STATUS_CHANGE_ACK, 6);

#pragma pack(push, 1)
struct PACKET_ZC_CHANGE_AGIT_OWNER {
    unsigned short packet_type;
    unsigned char agit_id;
    int old_guild_id;
    int new_guild_id;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_CHANGE_AGIT_OWNER, 11);

#pragma pack(push, 1)
struct PACKET_ZC_REQ_ACK_AGIT_INVESTMENT {
    unsigned short packet_type;
    unsigned char agit_id;
    int economy;
    int defense;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REQ_ACK_AGIT_INVESTMENT, 11);

#pragma pack(push, 1)
struct PACKET_ZC_REQ_ACK_MOVE_GUILD_AGIT {
    unsigned short packet_type;
    unsigned short result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REQ_ACK_MOVE_GUILD_AGIT, 4);

#pragma pack(push, 1)
struct PACKET_ZC_REMOVE_NEIGHBORHOOD_CHARACTERS {
    unsigned short packet_type;
    short packet_length;
    int aids[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_REMOVE_NEIGHBORHOOD_CHARACTERS, 4);

#pragma pack(push, 1)
struct PACKET_ZC_ADD_SKILL3 {
    unsigned short packet_type;
    unsigned short id;
    int type;
    unsigned short level;
    unsigned short sp_cost;
    unsigned short range;
    unsigned char upgradable;
    unsigned short level2;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ADD_SKILL3, 17);

#pragma pack(push, 1)
struct PACKET_ZC_SKILLINFO_UPDATE3 {
    unsigned short packet_type;
    unsigned short id;
    int type;
    unsigned short level;
    unsigned short sp;
    unsigned short range;
    unsigned char upgradable;
    unsigned short level2;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SKILLINFO_UPDATE3, 17);

#pragma pack(push, 1)
struct PACKET_CR_NEW_EMBLEM_DOWNLOAD {
    unsigned short packet_type;
    short packet_length;
    unsigned short part;
    int guild_id;
    int version;
    char data[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_CR_NEW_EMBLEM_DOWNLOAD, 14);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_ADD_NEW_EMBLEM {
    unsigned short packet_type;
    int guild_id;
    int emblem_version;
    int aid;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_ADD_NEW_EMBLEM, 14);

#pragma pack(push, 1)
struct PACKET_ZC_ADD_ACCOUNT_LIMTIED_SALE_ITEM {
    unsigned short packet_type;
    unsigned short result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ADD_ACCOUNT_LIMTIED_SALE_ITEM, 4);

#pragma pack(push, 1)
struct PACKET_ZC_DEL_ACCOUNT_LIMTIED_SALE_ITEM {
    unsigned short packet_type;
    unsigned short result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_DEL_ACCOUNT_LIMTIED_SALE_ITEM, 4);

#pragma pack(push, 1)
struct PACKET_ZC_SEARCH_ACCOUNT_LIMTIED_SALE_ITEM {
    unsigned short packet_type;
    unsigned short result;
    int item;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SEARCH_ACCOUNT_LIMTIED_SALE_ITEM, 8);

#pragma pack(push, 1)
struct PACKET_ZC_TARGET_SPIRITS {
    unsigned short packet_type;
    int aid;
    unsigned zero;
    unsigned short num;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_TARGET_SPIRITS, 12);

#pragma pack(push, 1)
struct PACKET_ZC_SE_CASHSHOP_OPEN_V2 {
    unsigned short packet_type;
    int cash_points;
    int kafra_points;
    int tab;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SE_CASHSHOP_OPEN_V2, 14);

#pragma pack(push, 1)
struct PACKET_ZC_SET_USERCONFIG_ACK {
    unsigned short packet_type;
    int type;
    int result;
    int value;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_SET_USERCONFIG_ACK, 14);

#pragma pack(push, 1)
struct PACKET_ZC_GET_USERCONFIG_LIST {
    unsigned short packet_type;
    short packet_length;
    struct UserConfigEntry entries[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_GET_USERCONFIG_LIST, 4);

#pragma pack(push, 1)
struct PACKET_ZC_RODEX_RETURN_ACK {
    unsigned short packet_type;
    int mail_id;
    int result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_RODEX_RETURN_ACK, 10);

#pragma pack(push, 1)
struct PACKET_ZC_DIALOG_TEXT_ALIGN {
    unsigned short packet_type;
    unsigned char align;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_DIALOG_TEXT_ALIGN, 3);

#pragma pack(push, 1)
struct PACKET_ZC_BATTLEFIELD_NOTIFY_HPSP {
    unsigned short packet_type;
    int aid;
    int hp;
    int max_hp;
    int sp;
    int max_sp;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_BATTLEFIELD_NOTIFY_HPSP, 22);

#pragma pack(push, 1)
struct PACKET_ZC_NOTIFY_HPSP_TO_GROUPM_R2 {
    unsigned short packet_type;
    int aid;
    int hp;
    int max_hp;
    int sp;
    int max_sp;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_NOTIFY_HPSP_TO_GROUPM_R2, 22);

#pragma pack(push, 1)
struct PACKET_ZC_ACK_TAKEOFF_EQUIP_ALL {
    unsigned short packet_type;
    unsigned char result;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_ACK_TAKEOFF_EQUIP_ALL, 3);

void game_mode_dispatch_packet2(struct GameMode *self, int packet_id,
                             unsigned char *pkt, size_t packet_size)
{
    if (!pkt || packet_size < sizeof(unsigned short))
        return;

    switch (packet_id) {
    case 0x0bf1: adventurer_agency_receive(g_adventurerAgency, pkt, packet_size); break;
    case HEADER_ZC_PARTY_BOOKING_ACK_REGISTER: if (packet_size >= 4) game_mode_zc_party_booking_ack_register(pkt); break;
    case HEADER_ZC_PARTY_BOOKING_ACK_SEARCH: game_mode_zc_party_booking_ack_search(pkt, packet_size); break;
    case HEADER_ZC_PARTY_BOOKING_ACK_DELETE: if (packet_size >= 4) game_mode_zc_party_booking_ack_delete(pkt); break;
    case HEADER_ZC_PARTY_BOOKING_NOTIFY_INSERT: if (packet_size >= 50) game_mode_zc_party_booking_notify_insert(pkt); break;
    case HEADER_ZC_PARTY_BOOKING_NOTIFY_UPDATE: if (packet_size >= 18) game_mode_zc_party_booking_notify_update(pkt); break;
    case HEADER_ZC_PARTY_BOOKING_NOTIFY_DELETE: if (packet_size >= 6) game_mode_zc_party_booking_notify_delete(pkt); break;
    case HEADER_ZC_NOTIFY_HP_TO_GROUPM_R2: game_mode_zc_notify_hp_to_group_m_r2(self, pkt); break;
    case HEADER_ZC_ADD_EXCHANGE_ITEM2: game_mode_add_exchange_item2(pkt); break;
    case HEADER_ZC_OPEN_BUYING_STORE: game_mode_zc_open_buying_store(self, pkt); break;
    case HEADER_ZC_FAILED_OPEN_BUYING_STORE_TO_BUYER: game_mode_failed_open_buying_store(pkt); break;
    case HEADER_ZC_MYITEMLIST_BUYING_STORE: game_mode_my_buying_store_list(self, pkt); break;
    case HEADER_ZC_BUYING_STORE_ENTRY: game_mode_buying_store_entry(self, (const struct PACKET_ZC_TALKBOX_CHATCONTENTS *)pkt); break;
    case HEADER_ZC_DISAPPEAR_BUYING_STORE_ENTRY: game_mode_disappear_buying_store_entry(self, pkt); break;
    case HEADER_ZC_ACK_ITEMLIST_BUYING_STORE: game_mode_ack_buying_store_list(self, pkt); break;
    case HEADER_ZC_FAILED_TRADE_BUYING_STORE_TO_BUYER: game_mode_failed_trade_buying_store_to_buyer(self, pkt); break;
    case HEADER_ZC_UPDATE_ITEM_FROM_BUYING_STORE: game_mode_zc_update_item_from_buying_store(self, pkt); break;
    case HEADER_ZC_ITEM_DELETE_BUYING_STORE: game_mode_item_delete_buying_store(pkt); break;
    case HEADER_ZC_EL_INIT: game_mode_zc_el_init(self, pkt); break;
    case HEADER_ZC_EL_PAR_CHANGE: game_mode_zc_el_par_change(self, pkt); break;
    case HEADER_ZC_BROADCAST4: game_mode_broadcast4(pkt); break;
    case HEADER_ZC_FAILED_TRADE_BUYING_STORE_TO_SELLER: game_mode_failed_trade_buying_store_to_seller(pkt); break;
    case HEADER_ZC_SEARCH_STORE_INFO_ACK: game_mode_zc_search_store_info_ack(self, pkt); break;
    case HEADER_ZC_SEARCH_STORE_INFO_FAILED: game_mode_zc_search_store_info_failed(self, pkt); break;
    case HEADER_ZC_ACK_BAN_GUILD_SSO: game_mode_zc_ack_ban_guild_sso(self, (const struct PacketGuildLeave *)pkt); break;
    case HEADER_ZC_OPEN_SEARCH_STORE_INFO: game_mode_zc_open_search_store_info(self, pkt); break;
    case HEADER_ZC_SSILIST_ITEM_CLICK_ACK: game_mode_set_search_store_pos((const struct PACKET_ZC_SEARCH_STORE_POS *)pkt); break;
    case HEADER_ZC_SE_CASHSHOP_OPEN:
    case HEADER_ZC_SE_CASHSHOP_OPEN2: game_mode_zc_se_cashshop_open(self, pkt); break;
    case HEADER_ZC_SE_PC_BUY_CASHITEM_RESULT: game_mode_zc_se_pc_buy_cashitem_result(self, pkt); break;
    case HEADER_ZC_ITEM_FALL_ENTRY4: game_mode_item_fall_entry4(self, (const struct PacketGroundItemFall4 *)pkt); break;

    case HEADER_ZC_MCSTORE_NOTMOVEITEM_LIST: game_mode_shop_not_movable_items(pkt); break;
    case HEADER_ZC_SHOWSCRIPT: game_mode_on_show_script(self, (const struct PacketShowScript *)pkt); break;
    case HEADER_ZC_START_COLLECTION: game_mode_on_start_collection(self, pkt); break;
    case HEADER_ZC_TRYCOLLECTION: game_mode_on_try_collection(self, (const struct PacketTryResult *)pkt); break;
    case HEADER_ZC_SKILL_ENTRY3: game_mode_zc_skill_entry3(self, (const struct PACKET_ZC_SKILL_ENTRY3 *)pkt); break;
    case HEADER_ZC_NOTIFY_ACT3: {
        const struct PACKET_ZC_NOTIFY_ACT3 *packet = (const struct PACKET_ZC_NOTIFY_ACT3 *)pkt;

        game_mode_notify_act(self, packet->aid, packet->target, packet->start_time, packet->attack_mt,
                           packet->attacked_mt, packet->damage, packet->count, packet->action, packet->left_damage,
                           packet->is_sp_damage);
        break;
    }
    case HEADER_ZC_ACK_SCHEDULER_CASHITEM: game_mode_item_list_by_kind(pkt); break;
    case HEADER_ZC_PERSONAL_INFOMATION:
    case HEADER_ZC_PERSONAL_INFOMATION2: game_mode_on_personal_info(self, (const struct PacketPersonalInfo *)pkt); break;
    case HEADER_ZC_STOPMOVE_FORCE: {
        const struct PACKET_ZC_STOPMOVE_FORCE *packet = (const struct PACKET_ZC_STOPMOVE_FORCE *)pkt;

        game_mode_zc_stopmove(self, packet->aid, packet->x, packet->y, 1);
        break;
    }
    case HEADER_ZC_FAILED_GET_ITEM_FROM_ZONEDA: game_mode_zc_failed_get_item_from_zone_da(self, pkt); break;
    case HEADER_ZC_SPIRITS_ATTRIBUTE: game_mode_on_spirits_attribute(self, (const struct PacketSpiritsAttribute *)pkt); break;
    case HEADER_ZC_REQ_WEAR_EQUIP_ACK2: game_mode_zc_req_wear_equip_ack2(self, pkt); break;
    case HEADER_ZC_REQ_TAKEOFF_EQUIP_ACK2: game_mode_zc_req_takeoff_equip_ack2(self, pkt); break;
    case HEADER_ZC_FASTMOVE: game_mode_zc_fast_move(self, pkt); break;
    case HEADER_ZC_SE_CASHSHOP_UPDATE: game_mode_zc_se_cashshop_points(self, pkt); break;
    case HEADER_ZC_CLEAR_DIALOG: game_mode_zc_clear_dialog(self, pkt); break;
    case HEADER_ZC_NAVIGATION_ACTIVE:
        if (packet_size >= sizeof(struct PacketNavigationActive))
            game_mode_on_navigation_active(self, (const struct PacketNavigationActive *)pkt);
        break;
    case HEADER_ZC_HUNTING_QUEST_INFO: game_mode_zc_hunting_quest_info(self, pkt, packet_size); break;
    case HEADER_ZC_EFST_SET_ENTER: game_mode_on_efst_set_enter(self, (const struct PacketEfstSetEnter *)pkt); break;
    case HEADER_ZC_INVENTORY_ITEMLIST_NORMAL: game_mode_zc_inventory_item_list_normal(pkt); break;
    case HEADER_ZC_INVENTORY_ITEMLIST_EQUIP: game_mode_equipment_item_list4(pkt); break;
    case HEADER_ZC_CART_ITEMLIST_NORMAL: game_mode_zc_cart_item_list_normal(pkt); break;
    case HEADER_ZC_CART_ITEMLIST_EQUIP: game_mode_zc_cart_item_list_equip(pkt); break;
    case HEADER_ZC_STORE_ITEMLIST_NORMAL: game_mode_zc_store_item_list_normal(pkt); break;
    case HEADER_ZC_STORE_ITEMLIST_EQUIP: game_mode_zc_store_item_list_equip(pkt); break;
    case HEADER_ZC_MICROSCOPE: game_mode_equip_win_microscope4(pkt); break;
    case HEADER_ZC_INVENTORY_TAB: game_mode_inventory_tab(pkt); break;
    case HEADER_ZC_NOTIFY_NEWENTRY7: game_mode_zc_notify_newentry7(self, (const struct PacketNewEntry7 *)pkt); break;
    case HEADER_ZC_NOTIFY_MOVEENTRY9: game_mode_zc_notify_moveentry9(self, (const struct PacketMoveEntry9 *)pkt); break;
    case HEADER_ZC_NOTIFY_STANDENTRY8: game_mode_zc_notify_standentry8(self, (const struct PacketStandEntry8 *)pkt); break;
    case HEADER_ZC_PRNPC_STATE: game_mode_on_pr_npc_state(self, (const struct PacketPrNpcState *)pkt); break;
    case HEADER_ZC_MERGE_ITEM_OPEN: game_mode_on_merge_item_open(self, (const struct PacketMergeItemOpen *)pkt); break;
    case HEADER_ZC_ACK_MERGE_ITEM: game_mode_zc_ack_merge_item(self, (const struct PACKET_ZC_ACK_MERGE_ITEM *)pkt); break;
    case HEADER_ZC_STORE_ITEMLIST_NORMAL_V2: game_mode_zc_store_item_list_normal_v2(pkt); break;
    case HEADER_ZC_STORE_ITEMLIST_EQUIP_V2: game_mode_zc_store_item_list_equip_v2(self, (const struct PACKET_STORE_LIST_NAMED *)pkt); break;
    case HEADER_ZC_HP_INFO: game_mode_zc_hp_info(self, (const struct PACKET_ZC_HP_INFO *)pkt); break;
    case HEADER_ZC_ACK_RANKING: ranking_receive_names(pkt, packet_size); break;
    case HEADER_ZC_UPDATE_RANKING_POINT: ranking_receive_points(pkt, packet_size); break;
    case HEADER_ZC_SELECTCART: game_mode_select_cart(self, pkt); break;
    case HEADER_ZC_MSG_STATE_CHANGE3: game_mode_on_msg_state_change3(self, (const struct PacketMsgStateChange3 *)pkt); break;
    case HEADER_ZC_EFST_SET_ENTER2: game_mode_on_efst_set_enter2(self, (const struct PacketEfstSetEnter2 *)pkt); break;
    case HEADER_ZC_SKILL_POSTDELAY_LIST2: skill_cooldowns_receive_list2(pkt, packet_size); break;
    case HEADER_ZC_ITEM_PICKUP_ACK_V5: game_mode_zc_item_pickup_ack_v5(self, pkt); break;
    case HEADER_ZC_INVENTORY_ITEMLIST_NORMAL_V5: game_mode_inventory_item_list_normal_v5(pkt); break;
    case HEADER_ZC_INVENTORY_ITEMLIST_EQUIP_V5: game_mode_equipment_item_list_v5(pkt); break;
    case HEADER_ZC_CART_ITEMLIST_NORMAL_V5: game_mode_cart_item_list_normal_v5(pkt); break;
    case HEADER_ZC_CART_ITEMLIST_EQUIP_V5: game_mode_cart_item_list_equip_v5(pkt); break;
    case HEADER_ZC_STORE_ITEMLIST_NORMAL_V5: game_mode_zc_store_item_list_normal_v5(self, (const struct PACKET_STORE_LIST_NAMED *)pkt); break;
    case HEADER_ZC_STORE_ITEMLIST_EQUIP_V5: game_mode_zc_store_item_list_equip_v5(self, (const struct PACKET_STORE_LIST_NAMED *)pkt); break;
    case HEADER_ZC_EQUIPWIN_MICROSCOPE_V5: game_mode_equip_win_microscope_v5(pkt); break;
    case HEADER_ZC_ACK_WEAR_EQUIP_V5: {
        const struct PACKET_ZC_ACK_WEAR_EQUIP_V5 *packet = (const struct PACKET_ZC_ACK_WEAR_EQUIP_V5 *)pkt;

        game_mode_zc_ack_wear_equip_v5(self, packet->result, packet->index, packet->view, packet->wear_location);
        break;
    }
    case HEADER_ZC_ACK_TAKEOFF_EQUIP_V5: {
        const struct PACKET_ZC_ACK_TAKEOFF_EQUIP_V5 *packet = (const struct PACKET_ZC_ACK_TAKEOFF_EQUIP_V5 *)pkt;

        game_mode_ack_takeoff_equip_v5(self, packet->result, packet->wear_location, packet->index);
        break;
    }
    case HEADER_ZC_MAPPROPERTY_R2: game_mode_map_property_r2(self, (const struct PacketMapPropertyR2 *)pkt); break;
    case HEADER_ZC_SKILL_ENTRY4: game_mode_zc_skill_entry4(self, (const struct PACKET_ZC_SKILL_ENTRY4 *)pkt); break;
    case HEADER_ZC_BANKING_CHECK: game_mode_banking_check(self, (const struct PacketBankingCheck *)pkt); break;
    case HEADER_ZC_ACK_BANKING_DEPOSIT: game_mode_ack_banking_deposit(self, (const struct PacketBankingAck *)pkt); break;
    case HEADER_ZC_ACK_BANKING_WITHDRAW: game_mode_ack_banking_withdraw(self, (const struct PacketBankingAck *)pkt); break;
    case HEADER_ZC_ACK_OPEN_GUILD_STORAGE: game_mode_ack_open_guild_storage((const struct PACKET_ZC_ACK_OPEN_GUILD_STORAGE *)pkt); break;
    case HEADER_ZC_ACK_CLOSE_GUILD_STORAGE: game_mode_ack_close_guild_storage((const struct PACKET_ZC_ACK_CLOSE_GUILD_STORAGE *)pkt); break;
    case HEADER_ZC_C_MARKERINFO: game_mode_zc_c_marker_info((const struct PACKET_ZC_C_MARKERINFO *)pkt); break;
    case HEADER_ZC_SKILL_ENTRY5: game_mode_zc_skill_entry5(self, (const struct PACKET_ZC_SKILL_ENTRY5 *)pkt); break;
    case HEADER_ZC_USE_SKILL2: {
        const struct PACKET_ZC_USE_SKILL2 *packet = (const struct PACKET_ZC_USE_SKILL2 *)pkt;

        game_mode_show_skill_use(self, packet->skill_id, packet->level, packet->src_aid, packet->target_aid,
                              packet->result);
        break;
    }
    case HEADER_ZC_MSG_COLOR: game_mode_msg_color(pkt); break;
    case HEADER_ZC_PROGRESS_ACTOR: game_mode_zc_progress_actor(self, pkt); break;
    case HEADER_ZC_GUILDSTORAGE_ITEMLIST_NORMAL_V5: game_mode_store_item_list_normal_v5(pkt); break;
    case HEADER_ZC_GUILDSTORAGE_ITEMLIST_EQUIP_V5: game_mode_store_item_list_equip_v5(pkt); break;
    case HEADER_ZC_NPC_MARKET_OPEN: game_mode_zc_npc_market_open(self, pkt); break;
    case HEADER_ZC_ACK_GUILDSTORAGE_LOG: game_mode_ack_guild_storage_log((const struct PACKET_ZC_ACK_GUILDSTORAGE_LOG *)pkt); break;
    case HEADER_ZC_NOTIFY_MOVEENTRY10: game_mode_zc_notify_moveentry10(self, (const struct PacketMoveEntry10 *)pkt); break;
    case HEADER_ZC_NOTIFY_NEWENTRY10: game_mode_zc_notify_newentry10(self, (const struct PacketNewEntry10 *)pkt); break;
    case HEADER_ZC_NOTIFY_STANDENTRY10: game_mode_zc_notify_standentry10(self, (const struct PacketStandEntry10 *)pkt); break;
    case HEADER_ZC_WHISPER02: game_mode_zc_whisper02(self, pkt); break;
    case HEADER_ZC_ACK_WHISPER02: {
        const struct PACKET_ZC_ACK_WHISPER02 *packet = (const struct PACKET_ZC_ACK_WHISPER02 *)pkt;

        game_mode_ack_whisper(self, packet->result, packet->receiver_cid);
        break;
    }
    case HEADER_ZC_DELETEITEM_FROM_MCSTORE2: game_mode_shop_item_sold_packet(pkt); break;
    case HEADER_ZC_UPDATE_ITEM_FROM_BUYING_STORE2: game_mode_buying_store_bought_packet(pkt); break;
    case HEADER_ZC_NOTIFY_UNREAD_RODEX: game_mode_on_notify_unread_rodex(self, (const struct PacketNotifyUnreadRodex *)pkt); break;
    case HEADER_ZC_ACK_SEND_RODEX: game_mode_zc_ack_send_rodex(self, (const struct PACKET_ZC_ACK_SEND_RODEX *)pkt); break;
    case HEADER_ZC_ACK_RODEX_LIST: game_mode_zc_ack_rodex_list(self, (const struct PACKET_ZC_ACK_RODEX_LIST *)pkt); break;
    case HEADER_ZC_ACK_ZENY_FROM_RODEX: game_mode_zc_ack_zeny_from_rodex(self, (const struct PACKET_ZC_ACK_ZENY_FROM_RODEX *)pkt); break;
    case HEADER_ZC_ACK_ITEM_FROM_RODEX: game_mode_zc_ack_item_from_rodex(self, (const struct PACKET_ZC_ACK_ZENY_FROM_RODEX *)pkt); break;
    case HEADER_ZC_ACK_DELETE_RODEX: game_mode_zc_ack_delete_rodex(self, (const struct PACKET_ZC_ACK_DELETE_RODEX *)pkt); break;
    case HEADER_ZC_ALL_QUEST_LIST3: game_mode_all_quest_list3(self, pkt, packet_size); break;
    case HEADER_ZC_ADD_QUEST_EX: game_mode_zc_add_quest_ex(self, pkt, packet_size); break;
    case HEADER_ZC_UPDATE_MISSION_HUNT_EX: game_mode_zc_update_mission_hunt_ex(self, pkt, packet_size); break;
    case HEADER_ZC_NOTIFY_MOVEENTRY11: game_mode_zc_notify_moveentry11(self, (const struct PacketMoveEntry11 *)pkt); break;
    case HEADER_ZC_NOTIFY_NEWENTRY11: game_mode_zc_notify_newentry11(self, (const struct PacketNewEntry11 *)pkt); break;
    case HEADER_ZC_NOTIFY_STANDENTRY11: game_mode_zc_notify_standentry11(self, (const struct PacketStandEntry11 *)pkt); break;
    case HEADER_ZC_DRESSROOM_OPEN: game_mode_zc_dressroom_open(); break;
    case HEADER_ZC_ACK_ADD_ITEM_RODEX: game_mode_zc_ack_add_item_rodex(self, (const struct PACKET_ZC_ACK_ADD_ITEM_RODEX *)pkt); break;
    case HEADER_ZC_ACK_REMOVE_RODEX_ITEM: game_mode_zc_ack_remove_rodex_item(self, (const struct PACKET_ZC_ACK_REMOVE_RODEX_ITEM *)pkt); break;
    case HEADER_ZC_ADD_EXCHANGE_ITEM3: game_mode_add_exchange_item3(pkt); break;
    case HEADER_ZC_ADD_ITEM_TO_STORE3: game_mode_add_item_to_store3(pkt); break;
    case HEADER_ZC_ADD_ITEM_TO_CART3: game_mode_zc_add_item_to_cart3(pkt); break;
    case HEADER_ZC_ITEM_PICKUP_ACK_V6: game_mode_item_pickup_ack_v6(self, pkt); break;
    case HEADER_ZC_INVENTORY_ITEMLIST_EQUIP_V6: game_mode_equipment_item_list_v6(pkt); break;
    case HEADER_ZC_BATTLEFIELD_NOTIFY_HP2: game_mode_zc_battlefield_notify_hp2(self, pkt); break;
    case HEADER_ZC_CART_ITEMLIST_EQUIP_V6: game_mode_cart_item_list_equip_v6(pkt); break;
    case HEADER_ZC_STORE_ITEMLIST_EQUIP_V6: game_mode_zc_store_item_list_equip_v6(self, (const struct PACKET_STORE_LIST_NAMED *)pkt); break;
    case HEADER_ZC_GUILDSTORAGE_ITEMLIST_EQUIP_V6: game_mode_store_item_list_equip_v6(pkt); break;
    case HEADER_ZC_ACK_OPEN_WRITE_RODEX: game_mode_zc_ack_open_write_rodex(self, (const struct PACKET_ZC_ACK_OPEN_WRITE_RODEX *)pkt); break;
    case HEADER_ZC_GOLDPCCAFE_POINT: game_mode_gold_pc_cafe_point(self, pkt); break;
    case HEADER_ZC_DYNAMICNPC_CREATE_RESULT: game_mode_zc_dynamic_npc_create_result(self, pkt); break;
    case HEADER_ZC_ALL_ACH_LIST: {
        const struct PACKET_ZC_ALL_ACH_LIST *packet = (const struct PACKET_ZC_ALL_ACH_LIST *)pkt;

        game_mode_zc_all_ach_list(self, packet->count, packet->total_points, packet->rank, packet->current_points,
                                 packet->next_points, (const struct AchievementEntry *)packet->entries);
        break;
    }
    case HEADER_ZC_ACH_UPDATE: {
        const struct PACKET_ZC_ACH_UPDATE *packet = (const struct PACKET_ZC_ACH_UPDATE *)pkt;

        game_mode_zc_ach_update(self, packet->total_points, packet->rank, packet->current_points, packet->next_points,
                               packet->id, packet->completed, packet->objectives, packet->completed_at,
                               packet->reward);
        break;
    }
    case HEADER_ZC_REQ_ACH_REWARD_ACK: {
        const struct PACKET_ZC_REQ_ACH_REWARD_ACK *packet = (const struct PACKET_ZC_REQ_ACH_REWARD_ACK *)pkt;

        game_mode_zc_req_ach_reward_ack(self, packet->result, packet->id);
        break;
    }
    case HEADER_ZC_RECOVERY2: game_mode_on_recovery(self, pkt, 1); break;
    case HEADER_ZC_ACK_OPENSTORE2: {
        const struct PACKET_ZC_ACK_OPENSTORE2 *packet = (const struct PACKET_ZC_ACK_OPENSTORE2 *)pkt;

        game_mode_shop_open_result(self, packet->result);
        break;
    }
    case HEADER_ZC_REQ_AU_BOT: {
        const struct PACKET_ZC_REQ_AU_BOT *packet = (const struct PACKET_ZC_REQ_AU_BOT *)pkt;

        game_mode_on_req_au_bot(self, packet->aid);
        break;
    }
    case HEADER_ZC_EQUIPWIN_MICROSCOPE_V6: game_mode_equip_win_microscope_v6(pkt); break;
    case HEADER_ZC_ACK_CHANGE_TITLE: {
        const struct PACKET_ZC_ACK_CHANGE_TITLE *packet = (const struct PACKET_ZC_ACK_CHANGE_TITLE *)pkt;

        game_mode_zc_ack_change_title(self, packet->result, packet->title_id);
        break;
    }
    case HEADER_ZC_ACK_REQNAMEALL2: game_mode_on_ack_req_name_all2(self, (const struct PacketAckReqNameAll2 *)pkt); break;
    case HEADER_ZC_RESULT_PACKAGE_ITEM_TEST: game_mode_zc_result_package_item_test(self, pkt); break;
    case HEADER_ZC_HP_INFO_TINY: game_mode_zc_hp_info_tiny(self, (const struct PACKET_ZC_HP_INFO_TINY *)pkt); break;
    case HEADER_ZC_ITEM_PICKUP_ACK_V7: game_mode_item_pickup_ack_v7(self, pkt); break;
    case HEADER_ZC_UI_OPEN: {
        const struct PACKET_ZC_UI_OPEN *packet = (const struct PACKET_ZC_UI_OPEN *)pkt;

        game_mode_ui_open(packet->type);
        break;
    }
    case HEADER_ZC_SKILL_POSTDELAY2: skill_cooldowns_receive_delay2(pkt, packet_size); break;
    case HEADER_ZC_EQUIPMENT_EFFECT: game_mode_on_equipment_effect(self, (const struct PacketEquipmentEffect *)pkt); break;
    case HEADER_ZC_UPDATE_CARDSLOT: game_mode_zc_update_card_slot(self, (const struct PACKET_ZC_UPDATE_CARDSLOT *)pkt); break;
    case HEADER_ZC_EF_WARNING_PLANE: {
        const struct PACKET_ZC_EF_WARNING_PLANE *packet = (const struct PACKET_ZC_EF_WARNING_PLANE *)pkt;

        game_mode_on_ef_warning_plane(self, packet->aid, packet->skill_id, packet->level, packet->x, packet->y,
                                  packet->duration);
        break;
    }
    case HEADER_ZC_ADD_MEMBER_TO_GROUP3: game_mode_on_add_member_to_group(self, (const struct PacketAddMemberToGroup *)pkt); break;
    case HEADER_ZC_GROUP_LIST2: game_mode_on_group_list2(self, (const struct PacketGroupList2 *)pkt); break;
    case HEADER_ZC_ACK_BUY_STYLINGSHOP: game_mode_ack_buy_styling_shop(self, pkt); break;
    case HEADER_ZC_ACK_PRIVATE_AIRPLANE_CALL: {
        const struct PACKET_ZC_ACK_PRIVATE_AIRPLANE_CALL *packet = (const struct PACKET_ZC_ACK_PRIVATE_AIRPLANE_CALL *)pkt;

        game_mode_on_ack_private_airplane_call(self, packet->result);
        break;
    }
    case HEADER_ZC_MAPMOVE_PRIVATE_AIRPLANE: {
        const struct PACKET_ZC_MAPMOVE_PRIVATE_AIRPLANE *packet = (const struct PACKET_ZC_MAPMOVE_PRIVATE_AIRPLANE *)pkt;

        game_mode_on_map_move_private_airplane(self, packet->map_name, packet->x, packet->y);
        break;
    }
    case HEADER_ZC_SERVERMOVE_PRIVATE_AIRPLANE: {
        const struct PACKET_ZC_SERVERMOVE_PRIVATE_AIRPLANE *packet = (const struct PACKET_ZC_SERVERMOVE_PRIVATE_AIRPLANE *)pkt;

        game_mode_on_server_move_private_airplane(self, packet->map_name, packet->x, packet->y, packet->ip, packet->port);
        break;
    }
    case HEADER_ZC_RANDOM_COMBINE_ITEM_UI_OPEN: {
        const struct PACKET_ZC_RANDOM_COMBINE_ITEM_UI_OPEN *packet = (const struct PACKET_ZC_RANDOM_COMBINE_ITEM_UI_OPEN *)pkt;

        game_mode_on_random_combine_item_ui_open(self, packet->value);
        break;
    }
    case HEADER_ZC_ACK_RANDOM_COMBINE_ITEM: {
        const struct PACKET_ZC_ACK_RANDOM_COMBINE_ITEM *packet = (const struct PACKET_ZC_ACK_RANDOM_COMBINE_ITEM *)pkt;

        game_mode_zc_ack_random_combine_item(self, packet->result);
        break;
    }
    case HEADER_ZC_CHECK_RECEIVE_CHARACTER_NAME2: {
        const struct PACKET_ZC_CHECK_RECEIVE_CHARACTER_NAME2 *packet = (const struct PACKET_ZC_CHECK_RECEIVE_CHARACTER_NAME2 *)pkt;

        game_mode_zc_check_receive_character_name2(self, packet->char_id, packet->job, packet->level, packet->name);
        break;
    }
    case HEADER_ZC_ACK_APPLY_MACRO_DETECTOR:
    case HEADER_ZC_APPLY_MACRO_DETECTOR:
    case HEADER_ZC_APPLY_MACRO_DETECTOR_CAPTCHA:
    case HEADER_ZC_REQ_ANSWER_MACRO_DETECTOR:
    case HEADER_ZC_CLOSE_MACRO_DETECTOR:
    case HEADER_ZC_ACK_PREVIEW_MACRO_DETECTOR:
    case HEADER_ZC_PREVIEW_MACRO_DETECTOR_CAPTCHA:
        game_mode_dispatch_captcha_packet(self, packet_id, pkt, packet_size);
        break;
    case HEADER_ZC_ACK_PLAYER_AID_IN_RANGE: {
        const struct PACKET_ZC_ACK_PLAYER_AID_IN_RANGE *packet = (const struct PACKET_ZC_ACK_PLAYER_AID_IN_RANGE *)pkt;

        game_mode_ack_player_aid_in_range(self, (unsigned)(packet->packet_length - 4) >> 2, packet->aids);
        break;
    }
    case HEADER_ZC_MSG_VALUE_STRING: {
        const struct PACKET_ZC_MSG_VALUE_STRING *packet = (const struct PACKET_ZC_MSG_VALUE_STRING *)pkt;

        game_mode_on_msg_value_string(self, packet->msg_id, packet->color, packet->value);
        break;
    }
    case HEADER_ZC_VIEW_CAMERAINFO: game_mode_camera_info(self, pkt); break;
    case HEADER_ZC_LATEST_RODEX_LIST: game_mode_on_latest_rodex_list(self, (const struct PacketLatestRodexList *)pkt); break;
    case HEADER_ZC_OPEN_UI_ASSISTANT_VENDING: {
        const struct PACKET_ZC_OPEN_UI_ASSISTANT_VENDING *packet = (const struct PACKET_ZC_OPEN_UI_ASSISTANT_VENDING *)pkt;

        game_mode_on_open_ui_assistant_vending(self, packet->slots, (unsigned)(packet->packet_length - 5) >> 1,
                                          packet->items);
        break;
    }
    case HEADER_ZC_ACK_CREATE_ASSISTANT: {
        const struct PACKET_ZC_ACK_CREATE_ASSISTANT *packet = (const struct PACKET_ZC_ACK_CREATE_ASSISTANT *)pkt;

        game_mode_on_ack_create_assistant(packet->kind, packet->result);
        break;
    }
    case HEADER_ZC_ACK_BAN_GUILD_DELNAME: game_mode_zc_ack_ban_guild_del_name(self, (const struct PacketGuildBanByGid *)pkt); break;
    case HEADER_ZC_ACK_LEAVE_GUILD_DELNAME: game_mode_zc_ack_leave_guild_del_name(self, (const struct PacketGuildLeaveByGid *)pkt); break;
    case HEADER_ZC_GUILD_INFO_DELNAME: game_mode_zc_guild_info_del_name(self, (const struct PacketGuildInfo *)pkt); break;
    case HEADER_ZC_MEMBER_ADD_DELNAME: game_mode_zc_member_add_del_name(self, (const struct PacketMemberMemo *)pkt); break;
    case HEADER_ZC_MEMBERMGR_INFO_DELNAME: game_mode_zc_member_mgr_info_del_name(self, (const struct PacketMemberList *)pkt); break;
    case HEADER_ZC_BAN_LIST_DELNAME: game_mode_zc_ban_list_del_name(self, (const struct PacketBanList *)pkt); break;
    case HEADER_ZC_STORE_ASSISTANT_ENTRY: game_mode_zc_store_assistant_entry(self, pkt); break;
    case HEADER_ZC_STORE_ASSISTANT_DISAPPEAR: {
        const struct PACKET_ZC_STORE_ASSISTANT_DISAPPEAR *packet = (const struct PACKET_ZC_STORE_ASSISTANT_DISAPPEAR *)pkt;

        game_mode_on_store_assistant_disappear(self, packet->aid);
        break;
    }
    case HEADER_ZC_ITEMLIST_FROM_ASSISTANT_VENDING: {
        const struct PACKET_ZC_ITEMLIST_FROM_ASSISTANT_VENDING *packet = (const struct PACKET_ZC_ITEMLIST_FROM_ASSISTANT_VENDING *)pkt;

        game_mode_on_vending_store_item_list(self, packet->owner_aid, packet->store_id, packet->is_mine,
                                        (unsigned)(packet->packet_length - 0x11) / 63, packet->duration,
                                        packet->entries);
        break;
    }
    case HEADER_ZC_ITEMLIST_FROM_ASSISTANT_BUYING: {
        const struct PACKET_ZC_ITEMLIST_FROM_ASSISTANT_BUYING *packet = (const struct PACKET_ZC_ITEMLIST_FROM_ASSISTANT_BUYING *)pkt;

        game_mode_on_buying_store_item_list(self, packet->owner_aid, packet->store_id, packet->is_mine,
                                       (unsigned)(packet->packet_length - 0x19) / 11, packet->duration,
                                       packet->entries, packet->zeny_limit);
        break;
    }
    case HEADER_ZC_OPEN_UI_ASSISTANT_BUYING: {
        const struct PACKET_ZC_OPEN_UI_ASSISTANT_BUYING *packet = (const struct PACKET_ZC_OPEN_UI_ASSISTANT_BUYING *)pkt;

        game_mode_on_open_ui_assistant_buying(self, packet->slots);
        break;
    }
    case HEADER_ZC_CONFIG_NOTIFY2: {
        const struct PACKET_ZC_CONFIG_NOTIFY2 *packet = (const struct PACKET_ZC_CONFIG_NOTIFY2 *)pkt;

        game_mode_config_notify2(self, packet->open_equip_window, packet->call_allowed);
        break;
    }
    case HEADER_ZC_ADD_EXCHANGE_ITEM4: game_mode_on_add_exchange_item_opt(self, (const struct PacketAddExchangeItemOpt *)pkt); break;
    case HEADER_ZC_ACK_ADD_SWAP_EQUIPITEM: game_mode_on_alt_equipment_equip(self, (const struct PacketAltEquipment *)pkt); break;
    case HEADER_ZC_ACK_REMOVE_SWQP_EQUIPITEM: game_mode_on_alt_equipment_remove(self, (const struct PacketAltEquipment *)pkt); break;
    case HEADER_ZC_SEND_SWAP_EQUIPITEM_INFO: game_mode_on_alt_equipment_items(self, (const struct PacketAltEquipmentItems *)pkt); break;
    case HEADER_ZC_ACK_DO_SWAP_EQUIPITEM: game_mode_on_alt_equipment_switch_ack(self, (const struct PacketAltEquipmentSwitchAck *)pkt); break;
    case HEADER_ZC_OPEN_REFINING_UI: ui_window_open(WID_REFINING_UI); break;
    case HEADER_ZC_REFINING_MATERIAL_LIST: {
        const struct PACKET_ZC_REFINING_MATERIAL_LIST *packet = (const struct PACKET_ZC_REFINING_MATERIAL_LIST *)pkt;

        game_mode_on_refining_material_list(self, packet->index, (unsigned)(packet->packet_length - 7) / 9,
                                        packet->materials, packet->blessing_count);
        break;
    }
    case HEADER_ZC_MEMBERMGR_INFO2: game_mode_on_member_mgr_info((const struct PacketMemberMgrInfo *)pkt); break;
    case HEADER_ZC_MEMBER_ADD2: game_mode_on_guild_member((const struct PacketGuildMember *)pkt); break;
    case HEADER_ZC_MOVE_ITEM_FAILED: game_mode_on_list_flag_clear(self, (const struct PacketListFlagClear *)pkt); break;
    case HEADER_ZC_CONFIG_NOTIFY3: {
        const struct PACKET_ZC_CONFIG_NOTIFY3 *packet = (const struct PACKET_ZC_CONFIG_NOTIFY3 *)pkt;

        game_mode_config_notify3(self, packet->open_equip_window, packet->call_allowed, packet->pet_auto_feed);
        break;
    }
    case HEADER_ZC_NPC_MARKETGEAR_OPEN: {
        const struct PACKET_ZC_NPC_MARKETGEAR_OPEN *packet = (const struct PACKET_ZC_NPC_MARKETGEAR_OPEN *)pkt;

        game_mode_on_npc_market_open(self, (unsigned)(packet->packet_length - 4) / 34,
                                 (const struct NpcMarketEntry *)packet->items);
        break;
    }
    case HEADER_ZC_NPC_MARKETGEAR_PURCHASE_RESULT: {
        const struct PACKET_ZC_NPC_MARKETGEAR_PURCHASE_RESULT *packet = (const struct PACKET_ZC_NPC_MARKETGEAR_PURCHASE_RESULT *)pkt;

        game_mode_on_npc_market_purchase_result(self, packet->result, (unsigned)(packet->packet_length - 5) / 29,
                                           (const struct MarketBoughtItem *)packet->items);
        break;
    }
    case HEADER_ZC_NOTIFY_DEATH_TO_GROUPM: game_mode_on_party_member_state(self, (const struct PacketPartyMemberState *)pkt); break;
    case HEADER_ZC_RANDOM_UPGRADE_ITEM_UI_OPEN: {
        const struct PACKET_ZC_RANDOM_UPGRADE_ITEM_UI_OPEN *packet = (const struct PACKET_ZC_RANDOM_UPGRADE_ITEM_UI_OPEN *)pkt;

        game_mode_on_random_upgrade_item_ui_open(self, packet->value);
        break;
    }
    case HEADER_ZC_ACK_RANDOM_UPGRADE_ITEM: {
        const struct PACKET_ZC_ACK_RANDOM_UPGRADE_ITEM *packet = (const struct PACKET_ZC_ACK_RANDOM_UPGRADE_ITEM *)pkt;

        game_mode_zc_ack_random_upgrade_item(self, packet->result);
        break;
    }
    case HEADER_ZC_SYNC_ATTACKED_POS: game_mode_on_sync_attacked_pos(self, pkt); break;
    case HEADER_ZC_CHANGE_ITEM_OPTION: game_mode_zc_change_item_option_packet(self, pkt); break;
    case HEADER_ZC_NOTIFY_MEMBERINFO_TO_GROUPM: game_mode_on_notify_member_info_to_groupm(self, (const struct PacketNotifyMemberInfoToGroupm *)pkt); break;
    case HEADER_ZC_WARPLIST2: game_mode_on_warp_list2(self, (struct PacketWarpList2 *)pkt); break;
    case HEADER_ZC_SHOW_WARPLIST: game_mode_on_show_warp_list(self, (struct PacketShowWarpList *)pkt); break;
    case HEADER_ZC_ACK_ALL_RODEX_LIST: game_mode_zc_ack_all_rodex_list(self, (const struct PACKET_ZC_ACK_ALL_RODEX_LIST *)pkt); break;
    case HEADER_ZC_ACK_FAILED_ALL_RODEX_LIST: game_mode_zc_ack_failed_all_rodex_list(self); break;
    case HEADER_ZC_NPCACK_SERVERMOVE_DOMAIN: game_mode_zc_npcack_servermove_domain(self, pkt); break;
    case HEADER_ZC_CLEAR_SWAP_EQUIPITEM_LIST: game_mode_on_alt_equipment_clear(self, pkt); break;
    case HEADER_ZC_LONGLONGPAR_CHANGE: game_mode_on_long_long_par_change(self, (const struct PacketLongLongParChange *)pkt); break;
    case HEADER_ZC_NOTIFY_EXP2: game_mode_on_notify_exp2(self, (const struct PacketNotifyExp2 *)pkt); break;
    case HEADER_ZC_BROADCAST_ITEMREFINING_RESULT: {
        const struct PACKET_ZC_BROADCAST_ITEMREFINING_RESULT *packet = (const struct PACKET_ZC_BROADCAST_ITEMREFINING_RESULT *)pkt;

        game_mode_zc_broadcast_item_refining_result(self, packet->name, packet->item_id, packet->refine,
                                                  packet->success);
        break;
    }
    case HEADER_ZC_DEBUGMSG: game_mode_zc_debug_msg(pkt); break;
    case HEADER_ZC_CONFIG_NOTIFY4: {
        const struct PACKET_ZC_CONFIG_NOTIFY4 *packet = (const struct PACKET_ZC_CONFIG_NOTIFY4 *)pkt;

        game_mode_config_notify4(self, packet->open_equip_window, packet->call_allowed, packet->pet_auto_feed,
                               packet->homun_auto_feed);
        break;
    }
    case HEADER_ZC_ITEM_FALL_ENTRY5: game_mode_on_item_fall_entry5(self, (const struct PacketItemFallEntry5 *)pkt); break;
    case HEADER_ZC_RECOVER_PENALTY_OVERWEIGHT: game_mode_on_recover_penalty_overweight(self, (const struct PacketRecoverPenaltyOverweight *)pkt); break;
    case HEADER_ZC_ACK_REQNAMEALL_NPC: game_mode_ack_req_name_all_npc(self, pkt); break;
    case HEADER_ZC_SETTING_WHISPER_PC2: game_mode_zc_setting_whisper_pc2(self, pkt); break;
    case HEADER_ZC_UI_OPEN_V2: {
        const struct PACKET_ZC_UI_OPEN_V2 *packet = (const struct PACKET_ZC_UI_OPEN_V2 *)pkt;

        game_mode_ui_open_v2(self, packet->type, packet->value);
        break;
    }
    case HEADER_ZC_ADD_MEMBER_TO_GROUP4: game_mode_on_add_member_to_group2(self, (const struct PacketAddMemberToGroup2 *)pkt); break;
    case HEADER_ZC_GROUP_LIST3: game_mode_on_group_list3(self, (const struct PacketGroupList3 *)pkt); break;
    case HEADER_ZC_PARTY_REQ_MASTER_TO_JOIN: game_mode_zc_party_req_master_to_join(pkt); break;
    case HEADER_ZC_ACK_CHECK_ATTENDANCE: {
        const struct PACKET_ZC_ACK_CHECK_ATTENDANCE *packet = (const struct PACKET_ZC_ACK_CHECK_ATTENDANCE *)pkt;

        game_mode_ack_check_attendance(self, packet->result, packet->day);
        break;
    }
    case HEADER_ZC_SET_CLAIRVOYANCE: {
        const struct PACKET_ZC_SET_CLAIRVOYANCE *packet = (const struct PACKET_ZC_SET_CLAIRVOYANCE *)pkt;

        game_mode_on_set_clairvoyance(self, packet->on);
        break;
    }
    case HEADER_ZC_ACK_RANKING2: ranking_receive_ids(pkt, packet_size); break;
    case HEADER_ZC_ACK_REQNAME_BYGID2: game_mode_ack_req_name_by_gid2(pkt); break;
    case HEADER_ZC_PARTY_JOIN_REQ_ACK_FROM_MASTER: game_mode_zc_party_join_req_ack_from_master(pkt); break;
    case HEADER_ZC_AUTOSPELLLIST2: {
        const struct PACKET_ZC_AUTOSPELLLIST2 *packet = (const struct PACKET_ZC_AUTOSPELLLIST2 *)pkt;

        game_mode_on_autospell_list2((unsigned)(packet->packet_length - 4) >> 2, packet->skills);
        break;
    }
    case HEADER_ZC_GUILD_POSITIONNAME: game_mode_zc_guild_position(self, (const struct PACKET_ZC_GUILD_POSITION *)pkt); break;
    case HEADER_ZC_UPDATE_MISSION_HUNT_EX2: game_mode_zc_update_mission_hunt_ex2(self, pkt, packet_size); break;
    case HEADER_ZC_ALL_QUEST_LIST4: game_mode_all_quest_list4(self, pkt, packet_size); break;
    case HEADER_ZC_EQUIPWIN_MICROSCOPE_V7: game_mode_equip_win_microscope_v6_body(pkt); break;
    case HEADER_ZC_STORE_ASSISTANT_ENTRY_V2: game_mode_zc_store_assistant_entry_v2(self, pkt); break;
    case HEADER_ZC_SPLIT_SEND_ITEMLIST_SET: game_mode_zc_split_send_item_list_set(self, (const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_SET *)pkt); break;
    case HEADER_ZC_SPLIT_SEND_ITEMLIST_NORMAL: {
        const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_NORMAL *packet = (const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_NORMAL *)pkt;

        game_mode_zc_split_send_item_list_normal(self, packet->type, (unsigned)(packet->packet_length - 5) / 34,
                                               (const struct NormalItem *)packet->items);
        break;
    }
    case HEADER_ZC_SPLIT_SEND_ITEMLIST_EQUIP: {
        const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_EQUIP *packet = (const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_EQUIP *)pkt;

        game_mode_zc_split_send_item_list_equip(self, packet->type, (unsigned)(packet->packet_length - 5) / 67,
                                              (const struct EquipItemV7 *)packet->items);
        break;
    }
    case HEADER_ZC_SPLIT_SEND_ITEMLIST_RESULT: {
        const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_RESULT *packet = (const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_RESULT *)pkt;

        game_mode_zc_split_send_item_list_result(self, packet->type, packet->result);
        break;
    }
    case HEADER_ZC_ADD_QUEST_EX2: game_mode_zc_add_quest_ex2(self, pkt, packet_size); break;
    case HEADER_ZC_CLEAR_EFFECT: game_mode_on_clear_effect(self, (const struct PacketClearEffect *)pkt); break;
    case HEADER_ZC_NPC_BARTER_MARKET_ITEMINFO: game_mode_zc_npc_barter_market_item_info(self, pkt); break;
    case HEADER_ZC_CHANGE_ITEM_OPTION2: game_mode_zc_change_item_option2_packet(self, pkt); break;
    case HEADER_ZC_ACK_OPEN_MSGBOX_EXTEND_BODYITEM_SIZE:
        game_mode_zc_ack_open_msg_box_extend_body_item_size(self, (const struct PACKET_ZC_ACK_OPEN_MSGBOX_EXTEND_BODYITEM_SIZE *)pkt);
        break;
    case HEADER_ZC_ACK_EXTEND_BODYITEM_SIZE: game_mode_zc_ack_extend_body_item_size(self, (const struct PACKET_ZC_ACK_EXTEND_BODYITEM_SIZE *)pkt); break;
    case HEADER_ZC_EXTEND_BODYITEM_SIZE: game_mode_zc_extend_body_item_size(self, (const struct PACKET_ZC_EXTEND_BODYITEM_SIZE *)pkt); break;
    case HEADER_ZC_USESKILL_ACK3: game_mode_on_use_skill_ack3(self, (const struct PacketUseSkillAck *)pkt); break;
    case HEADER_ZC_NOTIFY_ACTORINIT: game_mode_zc_notify_actor_init(self, pkt); break;
    case HEADER_ZC_SHORTCUT_KEY_LIST_V4: game_mode_zc_short_cut_key_list_v4(self, (const struct PACKET_ZC_SHORTCUT_KEY_LIST_V4 *)pkt); break;
    case HEADER_ZC_ADVANCED_STATUS_CHANGE_ACK: {
        const struct PACKET_ZC_ADVANCED_STATUS_CHANGE_ACK *packet = (const struct PACKET_ZC_ADVANCED_STATUS_CHANGE_ACK *)pkt;

        game_mode_on_advanced_status_change_ack(self, packet->result, packet->status, packet->value);
        break;
    }
    case HEADER_ZC_GUILD_AGIT_INFO: game_mode_on_guild_agit_info(self, (const struct PacketGuildAgitInfo *)pkt); break;
    case HEADER_ZC_CHANGE_AGIT_OWNER: {
        const struct PACKET_ZC_CHANGE_AGIT_OWNER *packet = (const struct PACKET_ZC_CHANGE_AGIT_OWNER *)pkt;

        game_mode_on_change_agit_owner(self, packet->agit_id, packet->old_guild_id, packet->new_guild_id);
        break;
    }
    case HEADER_ZC_REQ_ACK_AGIT_INVESTMENT: {
        const struct PACKET_ZC_REQ_ACK_AGIT_INVESTMENT *packet = (const struct PACKET_ZC_REQ_ACK_AGIT_INVESTMENT *)pkt;

        game_mode_on_req_ack_agit_investment(self, packet->agit_id, packet->economy, packet->defense);
        break;
    }
    case HEADER_ZC_REQ_ACK_MOVE_GUILD_AGIT: {
        const struct PACKET_ZC_REQ_ACK_MOVE_GUILD_AGIT *packet = (const struct PACKET_ZC_REQ_ACK_MOVE_GUILD_AGIT *)pkt;

        game_mode_on_req_ack_move_guild_agit(self, packet->result);
        break;
    }
    case HEADER_ZC_PROPERTY_HOMUN_NO_ACCESSORY: game_mode_zc_property_homun_no_accessory(self, pkt); break;
    case HEADER_ZC_REMOVE_NEIGHBORHOOD_CHARACTERS: {
        const struct PACKET_ZC_REMOVE_NEIGHBORHOOD_CHARACTERS *packet = (const struct PACKET_ZC_REMOVE_NEIGHBORHOOD_CHARACTERS *)pkt;

        game_mode_on_remove_neighborhood_characters(self, (unsigned)(packet->packet_length - 4) >> 2, packet->aids);
        break;
    }
    case HEADER_ZC_ADD_SKILL3: {
        const struct PACKET_ZC_ADD_SKILL3 *packet = (const struct PACKET_ZC_ADD_SKILL3 *)pkt;

        client_add_skill(packet->id, packet->level, packet->type, packet->upgradable, packet->sp_cost, packet->range,
                        packet->level2);
        break;
    }
    case HEADER_ZC_SKILLINFO_LIST3: game_mode_zc_skill_info_list3(self, pkt); break;
    case HEADER_ZC_SKILLINFO_UPDATE3: {
        const struct PACKET_ZC_SKILLINFO_UPDATE3 *packet = (const struct PACKET_ZC_SKILLINFO_UPDATE3 *)pkt;

        game_mode_skill_update(self, packet->id, packet->level, packet->sp, packet->upgradable, packet->range,
                             packet->type, packet->level2);
        break;
    }
    case HEADER_CR_NEW_EMBLEM_DOWNLOAD: {
        const struct PACKET_CR_NEW_EMBLEM_DOWNLOAD *packet = (const struct PACKET_CR_NEW_EMBLEM_DOWNLOAD *)pkt;

        game_mode_on_emblem_download(packet->part, packet->guild_id, packet->version, packet->data,
                                  packet->packet_length - 0xe);
        break;
    }
    case HEADER_ZC_EQUIPWIN_MICROSCOPE_V8: game_mode_equip_win_microscope_v7(pkt); break;
    case HEADER_ZC_SPLIT_SEND_ITEMLIST_EQUIPV8: game_mode_zc_split_send_item_list_equip_v8(self, (const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_EQUIPV8 *)pkt); break;
    case HEADER_ZC_INVENTORY_ITEMLIST_EQUIP_V8: game_mode_equipment_item_list_v7(pkt); break;
    case HEADER_ZC_PC_PURCHASE_ITEMLIST_FROMMC4: game_mode_on_purchase_item_list_from_mc4(self, (const struct PacketPurchaseItemListFromMc4 *)pkt); break;
    case HEADER_ZC_ACK_GUILDSTORAGE_LOG2: game_mode_ack_guild_storage_log2((const struct PACKET_ZC_ACK_GUILDSTORAGE_LOG2 *)pkt); break;
    case HEADER_ZC_ACK_ADD_ITEM_RODEX2: game_mode_zc_ack_add_item_rodex2(self, (const struct PACKET_ZC_ACK_ADD_ITEM_RODEX2 *)pkt); break;
    case HEADER_ZC_PC_PURCHASE_MYITEMLIST3: game_mode_on_purchase_my_item_list3(self, (const struct PacketPurchaseMyItemList3 *)pkt); break;
    case HEADER_ZC_ITEM_PICKUP_ACK_V8: game_mode_item_pickup_ack_v8(self, pkt); break;
    case HEADER_ZC_ADD_EXCHANGE_ITEM5: game_mode_on_add_exchange_item_grade(self, (const struct PacketAddExchangeItemGrade *)pkt); break;
    case HEADER_ZC_CHANGE_ITEM_OPTION3: game_mode_zc_change_item_option3(self, (const struct PACKET_ZC_CHANGE_ITEM_OPTION3 *)pkt); break;
    case HEADER_ZC_ADD_ITEM_TO_STORE4: game_mode_add_item_to_store4(pkt); break;
    case HEADER_ZC_ADD_ITEM_TO_CART4: game_mode_add_item_to_cart4(pkt); break;
    case HEADER_ZC_ACK_ADD_NEW_EMBLEM: {
        const struct PACKET_ZC_ACK_ADD_NEW_EMBLEM *packet = (const struct PACKET_ZC_ACK_ADD_NEW_EMBLEM *)pkt;

        game_mode_on_actor_guild(self, packet->aid, packet->guild_id, packet->emblem_version);
        break;
    }
    case HEADER_ZC_ADD_ACCOUNT_LIMTIED_SALE_ITEM: {
        const struct PACKET_ZC_ADD_ACCOUNT_LIMTIED_SALE_ITEM *packet = (const struct PACKET_ZC_ADD_ACCOUNT_LIMTIED_SALE_ITEM *)pkt;

        game_mode_add_limited_sale_item(self, packet->result);
        break;
    }
    case HEADER_ZC_DEL_ACCOUNT_LIMTIED_SALE_ITEM: {
        const struct PACKET_ZC_DEL_ACCOUNT_LIMTIED_SALE_ITEM *packet = (const struct PACKET_ZC_DEL_ACCOUNT_LIMTIED_SALE_ITEM *)pkt;

        game_mode_del_limited_sale_item(self, packet->result);
        break;
    }
    case HEADER_ZC_GET_ACCOUNT_LIMTIED_SALE_LIST: game_mode_limited_sale_list(self, (const struct PacketLimitedSaleList *)pkt); break;
    case HEADER_ZC_NPC_MARKET_PURCHASE_RESULT_V2: game_mode_zc_npc_market_purchase_result_v2(self, pkt); break;
    case HEADER_ZC_OPEN_ACCOUNT_LIMTIED_SALE_TOOL: game_mode_on_open_limited_sale_tool(self); break;
    case HEADER_ZC_CLOSE_ACCOUNT_LIMTIED_SALE_TOOL: game_mode_close_limited_sale_tool(self); break;
    case HEADER_ZC_SEARCH_ACCOUNT_LIMTIED_SALE_ITEM: {
        const struct PACKET_ZC_SEARCH_ACCOUNT_LIMTIED_SALE_ITEM *packet = (const struct PACKET_ZC_SEARCH_ACCOUNT_LIMTIED_SALE_ITEM *)pkt;

        game_mode_on_search_limited_sale_item(self, packet->result, packet->item);
        break;
    }
    case HEADER_ZC_NORMAL_SYSTEM_MSG: game_mode_zc_normal_system_msg(pkt); break;
    case HEADER_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO: game_mode_zc_npc_expanded_barter_market_item_info(self, pkt); break;
    case HEADER_ZC_GRADE_ENCHANT_MATERIAL_LIST: game_mode_on_grade_enchant_material_list(self, (const struct PacketGradeEnchantMaterialList *)pkt); break;
    case HEADER_ZC_GRADE_ENCHANT_ACK: game_mode_zc_grade_enchant_ack(self, (const struct PACKET_ZC_GRADE_ENCHANT_ACK *)pkt); break;
    case HEADER_ZC_GRADE_ENCHANT_BROADCAST_RESULT: game_mode_zc_grade_enchant_broadcast_result(self, (const struct PACKET_ZC_GRADE_ENCHANT_BROADCAST_RESULT *)pkt); break;
    case HEADER_ZC_ITEMLIST_FROM_ASSISTANT_VENDING4: game_mode_on_vending_store_item_list4(self, (const struct PacketVendingList4 *)pkt); break;
    case HEADER_ZC_ACK_READ_RODEX2: game_mode_zc_ack_read_rodex2(self, (const struct PACKET_ZC_ACK_READ_RODEX2 *)pkt); break;
    case HEADER_ZC_SEARCH_STORE_INFO_ACK2: game_mode_zc_search_store_info_ack2(self, pkt); break;
    case HEADER_ZC_REPAIRITEMLIST2: game_mode_on_repair_item_list2(self, (const struct PacketRepairItemList2 *)pkt); break;
    case HEADER_ZC_ITEM_PICKUP_PARTY2: game_mode_on_item_pickup_party2(self, (const struct PacketItemPickupParty2 *)pkt); break;
    case HEADER_ZC_TARGET_SPIRITS: {
        const struct PACKET_ZC_TARGET_SPIRITS *packet = (const struct PACKET_ZC_TARGET_SPIRITS *)pkt;

        game_mode_on_target_spirits(self, packet->aid, packet->num);
        break;
    }
    case HEADER_ZC_NOTIFY_EFFECT4: game_mode_on_notify_effect4(self, (const struct PacketNotifyEffect4 *)pkt); break;
    case HEADER_ZC_ABR_INIT: game_mode_zc_abr_init(self, (const struct PACKET_ZC_ABR_INIT *)pkt); break;
    case HEADER_ZC_ABR_PAR_CHANGE: game_mode_zc_abr_par_change(self, (const struct PACKET_ZC_ABR_PAR_CHANGE *)pkt); break;
    case HEADER_ZC_SE_CASHSHOP_OPEN_V2: {
        const struct PACKET_ZC_SE_CASHSHOP_OPEN_V2 *packet = (const struct PACKET_ZC_SE_CASHSHOP_OPEN_V2 *)pkt;

        game_mode_on_cash_shop_open(self, packet->cash_points, packet->kafra_points, packet->tab);
        break;
    }
    case HEADER_ZC_SOULENERGY: game_mode_on_soul_energy(self, (const struct PacketSoulEnergy *)pkt); break;
    case HEADER_ZC_PROPERTY_HOMUN_NO_ACCESSORY2: game_mode_zc_property_homun_no_accessory2(self, pkt); break;
    case HEADER_ZC_PC_PURCHASE_ITEMLIST2: game_mode_zc_pc_purchase_itemlist2(self, pkt); break;
    case HEADER_ZC_NPC_BARTER_MARKET_ITEMINFO2: game_mode_zc_npc_barter_market_item_info2(self, pkt); break;
    case HEADER_ZC_NPC_EXPANDED_BARTER_MARKET_ITEMINFO2: game_mode_zc_npc_expanded_barter_market_item_info2(self, pkt); break;
    case HEADER_ZC_NPC_MARKET_OPEN2: game_mode_zc_npc_market_open2(self, pkt); break;
    case HEADER_ZC_GUILD_INFO_NEW: game_mode_zc_guild_info_new(self, (const struct PacketGuildInfoNew *)pkt); break;
    case HEADER_ZC_BAN_LIST_NEW: game_mode_zc_ban_list_new(self, (const struct PacketBanList *)pkt); break;
    case HEADER_ZC_MEMBERMGR_INFO_NEW: game_mode_zc_member_mgr_info_new(self, (const struct PacketMemberList *)pkt); break;
    case HEADER_ZC_MEMBER_ADD_NEW: game_mode_zc_member_add_new(self, (const struct PacketMemberNamed *)pkt); break;
    case HEADER_ZC_SET_USERCONFIG_ACK: {
        const struct PACKET_ZC_SET_USERCONFIG_ACK *packet = (const struct PACKET_ZC_SET_USERCONFIG_ACK *)pkt;

        game_mode_set_user_config_ack(self, packet->type, packet->result, packet->value);
        break;
    }
    case HEADER_ZC_GET_USERCONFIG_LIST: {
        const struct PACKET_ZC_GET_USERCONFIG_LIST *packet = (const struct PACKET_ZC_GET_USERCONFIG_LIST *)pkt;

        game_mode_get_user_config_list(self, (unsigned)(packet->packet_length - 4) >> 3, packet->entries);
        break;
    }
    case HEADER_ZC_RODEX_BLOCK_RECEIVE: game_mode_zc_rodex_block_receive_packet(self, pkt); break;
    case HEADER_ZC_RODEX_RETURN_ACK: {
        const struct PACKET_ZC_RODEX_RETURN_ACK *packet = (const struct PACKET_ZC_RODEX_RETURN_ACK *)pkt;

        game_mode_zc_rodex_return_ack(self, packet->mail_id, packet->result);
        break;
    }
    case HEADER_ZC_DIALOG_TEXT_ALIGN: {
        const struct PACKET_ZC_DIALOG_TEXT_ALIGN *packet = (const struct PACKET_ZC_DIALOG_TEXT_ALIGN *)pkt;

        game_mode_zc_dialog_text_align(self, packet->align);
        break;
    }
    case HEADER_ZC_PROPERTY_HOMUN_NO_ACCESSORY3: game_mode_zc_property_homun_no_accessory3(self, pkt); break;
    case HEADER_ZC_HO_PAR_CHANGE2: game_mode_zc_ho_par_change2(pkt); break;
    case HEADER_ZC_BATTLEFIELD_NOTIFY_HPSP: {
        const struct PACKET_ZC_BATTLEFIELD_NOTIFY_HPSP *packet = (const struct PACKET_ZC_BATTLEFIELD_NOTIFY_HPSP *)pkt;

        game_mode_zc_battlefield_notify_hp_sp(self, packet->aid, packet->hp, packet->max_hp, packet->sp,
                                            packet->max_sp);
        break;
    }
    case HEADER_ZC_NOTIFY_HPSP_TO_GROUPM_R2: {
        const struct PACKET_ZC_NOTIFY_HPSP_TO_GROUPM_R2 *packet = (const struct PACKET_ZC_NOTIFY_HPSP_TO_GROUPM_R2 *)pkt;

        game_mode_zc_notify_hp_sp_to_group_m_r2(self, packet->aid, packet->hp, packet->max_hp, packet->sp,
                                             packet->max_sp);
        break;
    }
    case HEADER_ZC_ACK_TAKEOFF_EQUIP_ALL: {
        const struct PACKET_ZC_ACK_TAKEOFF_EQUIP_ALL *packet = (const struct PACKET_ZC_ACK_TAKEOFF_EQUIP_ALL *)pkt;

        game_mode_zc_ack_take_off_equip_all(self, packet->result);
        break;
    }
    case HEADER_ZC_DDUKDDAK_BOX_TEST: game_mode_zc_ddukddak_box_test(self, pkt); break;
    case HEADER_ZC_GPK_DYNCODE:
    case HEADER_ZC_GPK_AUTH:
    case HEADER_ZC_MACRO_ITEMPICKUP_FAIL:
    case HEADER_ZC_PREMIUM_CAMPAIGN_INFO:
    case HEADER_ZC_ACK_OPEN_BANKING:
    case HEADER_ZC_ACK_CLOSE_BANKING:
    case HEADER_ZC_SECRETSCAN_DATA:
    case HEADER_ZC_ACK_READ_RODEX:
    case HEADER_ZC_SEND_EAC_MESSAGE:
    case HEADER_ZC_PLAYTIME_NOTIFY_CHN_TAREN:
    case HEADER_ZC_WORLDSTORE_INFO:
    case HEADER_ZC_WORLDSTORE_ITEMLIST_NORMAL_V5:
    case HEADER_ZC_WORLDSTORE_ITEMLIST_EQUIP_V6:
    case HEADER_ZC_WORLDSTORE_CLOSE:
    case HEADER_ZC_WORLDSTORE_MOVEITEMINFO:
    case HEADER_ZC_CELLATTR_UPDATE:
    case HEADER_ZC_PING_LIVE:
        break;
    default:
        connection_reset_queues(rag_connection_transport(rag_connection_instance()));
        break;
    }
}
