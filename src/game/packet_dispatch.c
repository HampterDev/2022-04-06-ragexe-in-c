#include "contents/connection_api.h"
#include "game/actor_packets.h"
#include "game/barter_packets.h"
#include "game/chat_room.h"
#include "game/companion_packets.h"
#include "game/connection.h"
#include "game/entry_packet_handlers.h"
#include "game/ground_item_packets.h"
#include "game/item_list_packets.h"
#include "game/item_upgrade.h"
#include "game/navigation.h"
#include "game/packet_data.h"
#include "game/packet_handlers.h"
#include "game/quest_packets.h"
#include "platform/error_message.h"
#include "session/skill_commands.h"
#include "game/skill_cooldowns.h"
#include "game/gamemode.h"
#include "game/packet_ids.h"
#include "game/packet_dispatch.h"

#pragma pack(push, 1)
struct PACKET_ZC_STOPMOVE {
    unsigned short packet_type;
    int aid;
    unsigned short x;
    unsigned short y;
};

struct PACKET_ZC_NOTIFY_ACT {
    unsigned short packet_type;
    int aid;
    int target;
    int start_time;
    int attack_mt;
    int attacked_mt;
    short damage;
    unsigned short count;
    unsigned char action;
    short left_damage;
};

struct PACKET_ZC_NOTIFY_ACT2 {
    unsigned short packet_type;
    int aid;
    int target;
    int start_time;
    int attack_mt;
    int attacked_mt;
    int damage;
    unsigned short count;
    unsigned char action;
    int left_damage;
};

struct PACKET_ZC_ADD_SKILL {
    unsigned short packet_type;
    unsigned short skill_id;
    int type;
    unsigned short level;
    unsigned short sp_cost;
    unsigned short range;
    char skill_name[24];
    unsigned char upgradable;
};

struct PACKET_ZC_USE_SKILL {
    unsigned short packet_type;
    unsigned short skill_id;
    short level;
    int target_aid;
    int src_aid;
    unsigned char result;
};

struct PACKET_ZC_SPIRITS {
    unsigned short packet_type;
    int aid;
    unsigned short num;
};

struct PACKET_ZC_SKILLINFO_DELETE {
    unsigned short packet_type;
    unsigned short skill_id;
};

struct PACKET_ZC_SKILLINFO_UPDATE2 {
    unsigned short packet_type;
    unsigned short skill_id;
    int type;
    unsigned short level;
    unsigned short sp_cost;
    unsigned short range;
    unsigned char upgradable;
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_ZC_STOPMOVE, 10);
SIZE_ASSERT(PACKET_ZC_NOTIFY_ACT, 29);
SIZE_ASSERT(PACKET_ZC_NOTIFY_ACT2, 33);
SIZE_ASSERT(PACKET_ZC_ADD_SKILL, 39);
SIZE_ASSERT(PACKET_ZC_USE_SKILL, 15);
SIZE_ASSERT(PACKET_ZC_SPIRITS, 8);
SIZE_ASSERT(PACKET_ZC_SKILLINFO_DELETE, 4);
SIZE_ASSERT(PACKET_ZC_SKILLINFO_UPDATE2, 15);

void game_mode_dispatch_packet(struct GameMode *self, int packet_id,
                             unsigned char *pkt, size_t packet_size)
{
    if (!pkt || packet_size < sizeof(unsigned short))
        return;

    switch (packet_id) {
    case HEADER_ZC_ACCEPT_ENTER: game_mode_accept_enter(pkt); break;
    case HEADER_ZC_NOTIFY_STANDENTRY: game_mode_zc_notify_standentry(self, (const struct PacketStandEntry *)pkt); break;
    case HEADER_ZC_NOTIFY_NEWENTRY: game_mode_zc_notify_newentry(self, (const struct PacketNewEntry *)pkt); break;
    case HEADER_ZC_NOTIFY_MOVEENTRY: game_mode_zc_notify_moveentry(self, (const struct PacketMoveEntry *)pkt); break;
    case HEADER_ZC_NOTIFY_STANDENTRY_NPC: game_mode_zc_notify_standentry_npc(self, (const struct PacketStandEntryNpc *)pkt); break;
    case HEADER_ZC_NOTIFY_TIME: game_mode_zc_notify_time(self, pkt); break;
    case HEADER_ZC_NOTIFY_VANISH: game_mode_zc_notify_vanish(self, pkt); break;
    case HEADER_SC_NOTIFY_BAN: game_mode_notify_ban(self, (const struct PACKET_SC_NOTIFY_BAN *)pkt); break;
    case HEADER_ZC_NOTIFY_MOVE: game_mode_zc_notify_move(self, pkt); break;
    case HEADER_ZC_NOTIFY_PLAYERMOVE: game_mode_zc_notify_player_move(self, pkt); break;
    case HEADER_ZC_STOPMOVE: {
        const struct PACKET_ZC_STOPMOVE *packet = (const struct PACKET_ZC_STOPMOVE *)pkt;
        game_mode_zc_stopmove(self, packet->aid, packet->x, packet->y, 0);
        break;
    }
    case HEADER_ZC_NOTIFY_ACT:
    {
        const struct PACKET_ZC_NOTIFY_ACT *packet = (const struct PACKET_ZC_NOTIFY_ACT *)pkt;
        game_mode_notify_act(self, packet->aid, packet->target, packet->start_time, packet->attack_mt,
                           packet->attacked_mt, packet->damage, packet->count, packet->action,
                           packet->left_damage, 0);
        break;
    }
    case HEADER_ZC_NOTIFY_ACT_POSITION: game_mode_zc_notify_act_position(self, (const struct PACKET_ZC_NOTIFY_ACT_POSITION *)pkt); break;
    case HEADER_ZC_NOTIFY_CHAT: game_mode_zc_notify_chat(self, pkt); break;
    case HEADER_ZC_NOTIFY_PLAYERCHAT: game_mode_zc_notify_player_chat(self, pkt); break;
    case HEADER_ZC_NPCACK_MAPMOVE: game_mode_zc_npcack_mapmove(self, pkt); break;
    case HEADER_ZC_NPCACK_SERVERMOVE: game_mode_zc_npcack_servermove(self, pkt); break;
    case HEADER_ZC_NPCACK_ENABLE: self->server_move_pending = 0; break;
    case HEADER_ZC_ACK_REQNAME: game_mode_ack_req_name(self, pkt); break;
    case HEADER_ZC_WHISPER: game_mode_zc_whisper(self, pkt); break;
    case HEADER_ZC_BROADCAST: game_mode_broadcast(self, pkt); break;
    case HEADER_ZC_CHANGE_DIRECTION: game_mode_zc_change_direction(self, pkt); break;
    case HEADER_ZC_ITEM_ENTRY: game_mode_item_entry(self, (const struct PacketGroundItemEntry *)pkt); break;
    case HEADER_ZC_ITEM_FALL_ENTRY: game_mode_item_fall_entry(self, (const struct PacketGroundItemFall *)pkt); break;
    case HEADER_ZC_ITEM_PICKUP_ACK: game_mode_item_pickup_ack(self, pkt); break;
    case HEADER_ZC_ITEM_DISAPPEAR: game_mode_zc_item_disappear(self, pkt); break;
    case HEADER_ZC_NORMAL_ITEMLIST: game_mode_zc_normal_item_list(pkt, 1); break;
    case HEADER_ZC_EQUIPMENT_ITEMLIST: game_mode_zc_equipment_item_list(pkt); break;
    case HEADER_ZC_STORE_NORMAL_ITEMLIST: game_mode_zc_store_normal_item_list(pkt, 1); break;
    case HEADER_ZC_STORE_EQUIPMENT_ITEMLIST: game_mode_zc_store_equipment_item_list(self, (const struct PACKET_STORE_LIST *)pkt); break;
    case HEADER_ZC_USE_ITEM_ACK: game_mode_zc_use_item_ack(self, (const struct PACKET_ZC_USE_ITEM_ACK *)pkt); break;
    case HEADER_ZC_REQ_WEAR_EQUIP_ACK: game_mode_zc_req_wear_equip_ack(self, pkt); break;
    case HEADER_ZC_REQ_TAKEOFF_EQUIP_ACK: game_mode_zc_req_takeoff_equip_ack(self, pkt); break;
    case HEADER_ZC_ITEM_THROW_ACK: game_mode_zc_item_throw_ack(self, pkt); break;
    case HEADER_ZC_PAR_CHANGE: game_mode_zc_par_change(self, (struct PACKET_ZC_PAR_CHANGE *)pkt); break;
    case HEADER_ZC_LONGPAR_CHANGE: game_mode_on_long_par_change(self, (const struct PacketLongParChange *)pkt); break;
    case HEADER_ZC_RESTART_ACK: game_mode_zc_restart_ack(pkt); break;
    case HEADER_ZC_SAY_DIALOG: game_mode_zc_say_dialog(self, pkt); break;
    case HEADER_ZC_WAIT_DIALOG: game_mode_zc_wait_dialog(self, pkt); break;
    case HEADER_ZC_CLOSE_DIALOG: game_mode_zc_close_dialog(self, pkt); break;
    case HEADER_ZC_MENU_LIST: game_mode_zc_menu_list(self, pkt); break;
    case HEADER_ZC_STATUS_CHANGE_ACK: game_mode_zc_status_change_ack(self, (const struct PACKET_ZC_STATUS_CHANGE_ACK *)pkt); break;
    case HEADER_ZC_STATUS: game_mode_zc_status(self, (const struct PACKET_ZC_STATUS *)pkt); break;
    case HEADER_ZC_STATUS_CHANGE: game_mode_zc_status_change(self, (const struct PACKET_ZC_STATUS_CHANGE *)pkt); break;
    case HEADER_ZC_EMOTION: game_mode_zc_emotion(self, pkt); break;
    case HEADER_ZC_USER_COUNT: game_mode_zc_user_count(pkt); break;
    case HEADER_ZC_SPRITE_CHANGE: game_mode_zc_sprite_change(self, pkt); break;
    case HEADER_ZC_SELECT_DEALTYPE: game_mode_zc_select_dealtype(self, pkt); break;
    case HEADER_ZC_PC_PURCHASE_ITEMLIST: game_mode_zc_pc_purchase_itemlist(self, pkt); break;
    case HEADER_ZC_PC_SELL_ITEMLIST: game_mode_zc_pc_sell_itemlist(self, pkt); break;
    case HEADER_ZC_PC_PURCHASE_RESULT: game_mode_zc_pc_purchase_result(self, pkt); break;
    case HEADER_ZC_PC_SELL_RESULT: game_mode_zc_pc_sell_result(self, pkt); break;
    case HEADER_ZC_ACK_DISCONNECT_CHARACTER: game_mode_zc_ack_disconnect_character(pkt); break;
    case HEADER_ZC_SETTING_WHISPER_PC: game_mode_zc_setting_whisper_pc(self, pkt); break;
    case HEADER_ZC_SETTING_WHISPER_STATE: game_mode_zc_setting_whisper_state(self, pkt); break;
    case HEADER_ZC_WHISPER_LIST: game_mode_zc_whisper_list(self, pkt); break;
    case HEADER_ZC_ACK_CREATE_CHATROOM: game_mode_zc_ack_create_chatroom(self, pkt); break;
    case HEADER_ZC_ROOM_NEWENTRY: game_mode_zc_room_newentry(self, (const struct PacketRoomEntry *)pkt); break;
    case HEADER_ZC_DESTROY_ROOM: game_mode_zc_destroy_room(self, pkt); break;
    case HEADER_ZC_REFUSE_ENTER_ROOM: game_mode_zc_refuse_enter_room(self, pkt); break;
    case HEADER_ZC_ENTER_ROOM: game_mode_zc_enter_room(self, (const struct PACKET_ZC_ENTER_ROOM *)pkt); break;
    case HEADER_ZC_MEMBER_NEWENTRY: game_mode_zc_member_newentry(self, pkt); break;
    case HEADER_ZC_MEMBER_EXIT: game_mode_zc_member_exit(self, pkt); break;
    case HEADER_ZC_CHANGE_CHATROOM: game_mode_zc_change_chatroom(self, pkt); break;
    case HEADER_ZC_ROLE_CHANGE: game_mode_zc_role_change(self, pkt); break;
    case HEADER_ZC_REQ_EXCHANGE_ITEM: game_mode_zc_req_exchange_item(self, pkt); break;
    case HEADER_ZC_ACK_EXCHANGE_ITEM: game_mode_ack_exchange_item(self, pkt); break;
    case HEADER_ZC_ACK_ADD_EXCHANGE_ITEM: game_mode_ack_add_exchange_item(self, pkt); break;
    case HEADER_ZC_CONCLUDE_EXCHANGE_ITEM: game_mode_zc_conclude_exchange_item(self, pkt); break;
    case HEADER_ZC_CANCEL_EXCHANGE_ITEM: game_mode_zc_cancel_exchange_item(self, pkt); break;
    case HEADER_ZC_EXEC_EXCHANGE_ITEM: game_mode_zc_exec_exchange_item(self, pkt); break;
    case HEADER_ZC_EXCHANGEITEM_UNDO: game_mode_zc_exchange_item_undo(self, pkt); break;
    case HEADER_ZC_NOTIFY_STOREITEM_COUNTINFO: game_mode_zc_notify_store_item_count_info(pkt); break;
    case HEADER_ZC_DELETE_ITEM_FROM_STORE: game_mode_zc_delete_item_from_store(self, pkt); break;
    case HEADER_ZC_CLOSE_STORE: game_mode_zc_close_store(); break;
    case HEADER_ZC_ACK_MAKE_GROUP: game_mode_zc_ack_make_group(pkt); break;
    case HEADER_ZC_GROUP_LIST: game_mode_group_list(pkt); break;
    case HEADER_ZC_ACK_REQ_JOIN_GROUP:
    case HEADER_ZC_PARTY_JOIN_REQ_ACK: game_mode_party_join_ack(pkt); break;
    case HEADER_ZC_REQ_JOIN_GROUP:
    case HEADER_ZC_PARTY_JOIN_REQ: game_mode_zc_req_join_group(pkt); break;
    case HEADER_ZC_GROUPINFO_CHANGE: game_mode_zc_group_info_change(pkt); break;
    case HEADER_ZC_ADD_MEMBER_TO_GROUP: game_mode_add_member_to_group(self, pkt); break;
    case HEADER_ZC_DELETE_MEMBER_FROM_GROUP: game_mode_delete_member_from_group(self, pkt); break;
    case HEADER_ZC_NOTIFY_HP_TO_GROUPM: game_mode_zc_notify_hp_to_group_m(self, pkt); break;
    case HEADER_ZC_NOTIFY_POSITION_TO_GROUPM: game_mode_zc_notify_position_to_group_m(self, pkt); break;
    case HEADER_ZC_NOTIFY_CHAT_PARTY: game_mode_notify_chat_party(self, pkt); break;
    case HEADER_ZC_MVP_GETTING_ITEM: game_mode_item_name_msg(pkt); break;
    case HEADER_ZC_MVP_GETTING_SPECIAL_EXP: game_mode_zc_mvp_getting_special_exp(pkt); break;
    case HEADER_ZC_MVP: game_mode_zc_mvp(self, pkt); break;
    case HEADER_ZC_THROW_MVPITEM: game_mode_zc_throw_mvp_item(); break;
    case HEADER_ZC_SKILLINFO_UPDATE:
    case HEADER_ZC_HOSKILLINFO_UPDATE:
    case HEADER_ZC_MER_SKILLINFO_UPDATE: game_mode_zc_skill_info_update(self, (const struct PACKET_ZC_SKILLINFO_UPDATE *)pkt); break;
    case HEADER_ZC_SKILLINFO_LIST:
    case HEADER_ZC_HOSKILLINFO_LIST:
    case HEADER_ZC_MER_SKILLINFO_LIST: game_mode_zc_skill_info_list(self, pkt); break;
    case HEADER_ZC_ACK_TOUSESKILL: game_mode_ack_to_use_skill(self, (const struct PACKET_ZC_ACK_TOUSESKILL *)pkt); break;
    case HEADER_ZC_ADD_SKILL:
    {
        const struct PACKET_ZC_ADD_SKILL *packet = (const struct PACKET_ZC_ADD_SKILL *)pkt;
        client_add_skill(packet->skill_id, packet->level, packet->type, packet->upgradable,
                        packet->sp_cost, packet->range, packet->level);
        break;
    }
    case HEADER_ZC_NOTIFY_SKILL: game_mode_on_notify_skill(self, (const struct PacketNotifySkill *)pkt); break;
    case HEADER_ZC_NOTIFY_SKILL_POSITION: game_mode_on_notify_skill_position(self, (const struct PacketNotifySkillPosition *)pkt); break;
    case HEADER_ZC_NOTIFY_GROUNDSKILL: game_mode_on_notify_ground_skill(self, (const struct PacketNotifyGroundSkill *)pkt); break;
    case HEADER_ZC_STATE_CHANGE: game_mode_zc_state_change(self, (const struct PACKET_ZC_STATE_CHANGE *)pkt); break;
    case HEADER_ZC_USE_SKILL:
    {
        const struct PACKET_ZC_USE_SKILL *packet = (const struct PACKET_ZC_USE_SKILL *)pkt;
        game_mode_show_skill_use(self, packet->skill_id, packet->level, packet->src_aid,
                              packet->target_aid, packet->result);
        break;
    }
    case HEADER_ZC_WARPLIST: game_mode_on_warp_list(self, (const struct PacketWarpList *)pkt); break;
    case HEADER_ZC_ACK_REMEMBER_WARPPOINT: game_mode_zc_ack_remember_warppoint(pkt); break;
    case HEADER_ZC_SKILL_ENTRY: game_mode_zc_skill_entry(self, (struct PACKET_ZC_SKILL_ENTRY *)pkt); break;
    case HEADER_ZC_SKILL_DISAPPEAR: game_mode_zc_skill_disappear(self, pkt); break;
    case HEADER_ZC_NOTIFY_CARTITEM_COUNTINFO: game_mode_zc_notify_cart_item_count_info(pkt); break;
    case HEADER_ZC_CART_EQUIPMENT_ITEMLIST: game_mode_zc_cart_equipment_item_list(pkt); break;
    case HEADER_ZC_CART_NORMAL_ITEMLIST: game_mode_zc_cart_normal_item_list(pkt, 1); break;
    case HEADER_ZC_ADD_ITEM_TO_CART: game_mode_zc_add_item_to_cart(pkt); break;
    case HEADER_ZC_DELETE_ITEM_FROM_CART: game_mode_zc_delete_item_from_cart(self, pkt); break;
    case HEADER_ZC_CARTOFF: game_mode_zc_cart_off(); break;
    case HEADER_ZC_ACK_ADDITEM_TO_CART: game_mode_zc_ack_add_item_to_cart(pkt); break;
    case HEADER_ZC_OPENSTORE: game_mode_zc_open_store(self, pkt); break;
    case HEADER_ZC_STORE_ENTRY: game_mode_zc_store_entry(self, pkt); break;
    case HEADER_ZC_DISAPPEAR_ENTRY: game_mode_zc_disappear_entry(self, pkt); break;
    case HEADER_ZC_PC_PURCHASE_ITEMLIST_FROMMC:
    case HEADER_ZC_PC_PURCHASE_ITEMLIST_FROMMC2: game_mode_zc_pc_purchase_itemlist_from_mc(self, pkt); break;
    case HEADER_ZC_PC_PURCHASE_RESULT_FROMMC: game_mode_zc_pc_purchase_result_from_mc(self, pkt); break;
    case HEADER_ZC_PC_PURCHASE_MYITEMLIST: game_mode_zc_pc_purchase_my_itemlist(self, pkt); break;
    case HEADER_ZC_DELETEITEM_FROM_MCSTORE: game_mode_zc_delete_item_from_mc_store(pkt); break;
    case HEADER_ZC_ATTACK_FAILURE_FOR_DISTANCE: game_mode_zc_attack_failure_for_distance(self, pkt); break;
    case HEADER_ZC_ATTACK_RANGE: game_mode_zc_attack_range(pkt); break;
    case HEADER_ZC_ACTION_FAILURE: game_mode_on_action_failure(self, (const struct PacketActionFailure *)pkt); break;
    case HEADER_ZC_EQUIP_ARROW: game_mode_zc_equip_arrow(self, pkt); break;
    case HEADER_ZC_RECOVERY: game_mode_on_recovery(self, pkt, 0); break;
    case HEADER_ZC_USESKILL_ACK: game_mode_on_use_skill_ack(self, (const struct PacketUseSkillAck *)pkt, 0); break;
    case HEADER_ZC_COUPLESTATUS: game_mode_zc_couple_status(self, (const struct PACKET_ZC_COUPLESTATUS *)pkt); break;
    case HEADER_ZC_OPEN_EDITDLG: game_mode_zc_open_edit_dlg(self, pkt); break;
    case HEADER_ZC_COMPASS: game_mode_zc_compass(self, pkt); break;
    case HEADER_ZC_SHOW_IMAGE: game_mode_zc_show_image(self, (const struct PACKET_ZC_SHOW_IMAGE *)pkt); break;
    case HEADER_ZC_AUTORUN_SKILL: game_mode_zc_autorun_skill(self, pkt); break;
    case HEADER_ZC_RESURRECTION: game_mode_zc_resurrection(self, (const struct PACKET_ZC_RESURRECTION *)pkt); break;
    case HEADER_ZC_ACK_GIVE_MANNER_POINT: game_mode_zc_ack_give_manner_point(pkt); break;
    case HEADER_ZC_NOTIFY_MANNER_POINT_GIVEN: game_mode_zc_notify_manner_point_given(pkt); break;
    case HEADER_ZC_MYGUILD_BASIC_INFO: game_mode_zc_my_guild_basic_info(self, (const struct PacketGuildRelations *)pkt); break;
    case HEADER_ZC_ACK_GUILD_MENUINTERFACE: game_mode_zc_ack_guild_menu_interface(pkt); break;
    case HEADER_ZC_GUILD_INFO: game_mode_zc_guild_info(pkt); break;
    case HEADER_ZC_GUILD_EMBLEM_IMG: game_mode_zc_guild_emblem_img(self, pkt); break;
    case HEADER_ZC_MEMBERMGR_INFO: game_mode_zc_member_mgr_info(self, (const struct PacketMemberList *)pkt); break;
    case HEADER_ZC_ACK_REQ_CHANGE_MEMBERS: game_mode_zc_ack_req_change_members(pkt); break;
    case HEADER_ZC_ACK_LEAVE_GUILD: game_mode_zc_ack_leave_guild(self, (const struct PacketGuildLeave *)pkt); break;
    case HEADER_ZC_ACK_DISORGANIZE_GUILD_RESULT: game_mode_zc_ack_disorganize_guild_result(pkt); break;
    case HEADER_ZC_ACK_DISORGANIZE_GUILD: game_mode_zc_ack_disorganize_guild(pkt); break;
    case HEADER_ZC_POSITION_INFO: game_mode_zc_position_info(self, (const struct PacketPositions *)pkt); break;
    case HEADER_ZC_GUILD_SKILLINFO: game_mode_zc_guild_skill_info(self, (const struct PACKET_ZC_GUILD_SKILLINFO *)pkt); break;
    case HEADER_ZC_BAN_LIST: game_mode_zc_ban_list(self, (const struct PacketBanList *)pkt); break;
    case HEADER_ZC_POSITION_ID_NAME_INFO: game_mode_zc_position_id_name_info(self, (const struct PacketPositionNames *)pkt); break;
    case HEADER_ZC_RESULT_MAKE_GUILD: game_mode_zc_result_make_guild((const struct PACKET_ZC_RESULT_MAKE_GUILD *)pkt); break;
    case HEADER_ZC_ACK_REQ_JOIN_GUILD: game_mode_zc_ack_req_join_guild(pkt); break;
    case HEADER_ZC_REQ_JOIN_GUILD: game_mode_zc_req_join_guild((const struct PACKET_ZC_REQ_GUILD_INVITE *)pkt); break;
    case HEADER_ZC_UPDATE_GDID: game_mode_zc_update_gdid(self, (const struct PACKET_ZC_UPDATE_GDID *)pkt); break;
    case HEADER_ZC_UPDATE_CHARSTAT: game_mode_zc_update_charstat(self, (const struct PACKET_ZC_UPDATE_CHARSTAT *)pkt); break;
    case HEADER_ZC_GUILD_NOTICE: game_mode_zc_guild_notice(self, (struct PacketGuildNotice *)pkt); break;
    case HEADER_ZC_REQ_ALLY_GUILD: game_mode_zc_req_ally_guild((const struct PACKET_ZC_REQ_GUILD_INVITE *)pkt); break;
    case HEADER_ZC_ACK_REQ_ALLY_GUILD: game_mode_zc_ack_req_ally_guild(pkt); break;
    case HEADER_ZC_ACK_CHANGE_GUILD_POSITIONINFO: game_mode_zc_ack_change_guild_position_info((const struct PacketPositionInfo *)pkt); break;
    case HEADER_ZC_ITEMIDENTIFY_LIST: game_mode_item_identify_list(self, pkt); break;
    case HEADER_ZC_ACK_ITEMIDENTIFY: game_mode_zc_ack_item_identify(pkt); break;
    case HEADER_ZC_ITEMCOMPOSITION_LIST: game_mode_item_composition_list(self, pkt); break;
    case HEADER_ZC_ACK_ITEMCOMPOSITION: game_mode_zc_ack_item_composition(self, (const struct PacketItemComposition *)pkt); break;
    case HEADER_ZC_GUILD_CHAT: game_mode_zc_guild_chat(self, pkt); break;
    case HEADER_ZC_ACK_REQ_HOSTILE_GUILD: game_mode_zc_ack_req_hostile_guild(pkt); break;
    case HEADER_ZC_MEMBER_ADD: game_mode_zc_member_add(self, (const struct PacketMemberFull *)pkt); break;
    case HEADER_ZC_DELETE_RELATED_GUILD: game_mode_zc_delete_related_guild(pkt); break;
    case HEADER_ZC_ADD_RELATED_GUILD: game_mode_zc_add_related_guild(pkt); break;
    case HEADER_ZC_ACK_ITEMREFINING: game_mode_zc_ack_item_refining(self, (const struct PacketItemRefining *)pkt); break;
    case HEADER_ZC_NOTIFY_MAPINFO: game_mode_zc_notify_map_info(self, (const struct PACKET_ZC_NOTIFY_MAPINFO *)pkt); break;
    case HEADER_ZC_ACK_REQ_DISCONNECT: game_mode_ack_req_disconnect(self, (const struct PACKET_ZC_ACK_REQ_DISCONNECT *)pkt); break;
    case HEADER_ZC_MONSTER_INFO: game_mode_zc_monster_info(self, (const struct PACKET_ZC_MONSTER_INFO *)pkt); break;
    case HEADER_ZC_MAKABLEITEMLIST: game_mode_zc_makable_item_list(self, (const struct PacketMakableItemList *)pkt); break;
    case HEADER_ZC_ACK_REQMAKINGITEM: game_mode_zc_ack_req_making_item(self, (const struct PacketMakingItem *)pkt); break;
    case HEADER_ZC_TALKBOX_CHATCONTENTS: game_mode_zc_talkbox_chat_contents(self, (const struct PACKET_ZC_TALKBOX_CHATCONTENTS *)pkt); break;
    case HEADER_ZC_UPDATE_MAPINFO: game_mode_zc_update_map_info(self, (const struct PACKET_ZC_UPDATE_MAPINFO *)pkt); break;
    case HEADER_ZC_ACK_REQNAME_BYGID: game_mode_ack_req_name_by_gid(pkt); break;
    case HEADER_ZC_ACK_REQNAMEALL: game_mode_ack_req_name_all(self, pkt); break;
    case HEADER_ZC_MSG_STATE_CHANGE: game_mode_on_msg_state_change(self, (const struct PacketMsgStateChange *)pkt); break;
    case HEADER_ZC_NOTIFY_MAPPROPERTY: game_mode_notify_map_property(self, (const struct PacketMapProperty *)pkt); break;
    case HEADER_ZC_NOTIFY_RANKING: game_mode_zc_notify_ranking(self, (const struct PacketNotifyRanking *)pkt); break;
    case HEADER_ZC_NOTIFY_EFFECT: game_mode_on_notify_effect(self, (const struct PacketNotifyEffect *)pkt); break;
    case HEADER_ZC_START_CAPTURE: game_mode_on_start_capture(self, pkt); break;
    case HEADER_ZC_TRYCAPTURE_MONSTER: game_mode_on_try_capture_monster(self, (const struct PacketTryResult *)pkt); break;
    case HEADER_ZC_PROPERTY_PET: game_mode_zc_property_pet(self, (unsigned char *)pkt); break;
    case HEADER_ZC_FEED_PET:
    case HEADER_ZC_FEED_MER: game_mode_zc_feed(self, pkt); break;
    case HEADER_ZC_CHANGESTATE_PET: game_mode_zc_change_state_pet(self, pkt); break;
    case HEADER_ZC_PETEGG_LIST: game_mode_zc_pet_egg_list(self, pkt); break;
    case HEADER_ZC_PET_ACT: game_mode_zc_pet_act(self, (unsigned char *)pkt); break;
    case HEADER_ZC_PAR_CHANGE_USER: game_mode_zc_par_change_user(self, pkt); break;
    case HEADER_ZC_SKILL_UPDATE: game_mode_zc_skill_update(self, (const struct PACKET_ZC_SKILL_UPDATE *)pkt); break;
    case HEADER_ZC_MAKINGARROW_LIST: game_mode_zc_making_arrow_list(pkt); break;
    case HEADER_ZC_NPCSPRITE_CHANGE: game_mode_zc_npc_sprite_change(self, pkt); break;
    case HEADER_ZC_SHOWDIGIT: game_mode_zc_show_digit(self, (const struct PACKET_ZC_SHOWDIGIT *)pkt); break;
    case HEADER_ZC_SHOW_IMAGE2: game_mode_zc_show_image2(self, (const struct PACKET_ZC_SHOW_IMAGE2 *)pkt); break;
    case HEADER_ZC_CHANGE_GUILD: game_mode_zc_change_guild(self, (const struct PACKET_ZC_CHANGE_GUILD *)pkt); break;
    case HEADER_ZC_GUILD_INFO2: game_mode_zc_guild_info2((const struct PACKET_ZC_GUILD_INFO2 *)pkt); break;
    case HEADER_ZC_DISPEL: game_mode_zc_dispel(self, (const struct PacketDispel *)pkt); break;
    case HEADER_ZC_REPLY_REMAINTIME: game_mode_zc_reply_remain_time(self, (const struct PACKET_ZC_REPLY_REMAINTIME *)pkt); break;
    case HEADER_ZC_INFO_REMAINTIME: game_mode_zc_info_remain_time(self, (const struct PACKET_ZC_INFO_REMAINTIME *)pkt); break;
    case HEADER_ZC_BROADCAST2: game_mode_broadcast2(self, pkt); break;
    case HEADER_ZC_ADD_ITEM_TO_STORE2: game_mode_add_item_to_store2(pkt); break;
    case HEADER_ZC_ADD_ITEM_TO_CART2: game_mode_add_item_to_cart2(pkt); break;
    case HEADER_SC_ACK_ENCRYPTION: error_msg("No Packet Encryption !!!", 0); break;
    case HEADER_ZC_USE_ITEM_ACK2: game_mode_zc_use_item_ack2(self, (const struct PACKET_ZC_USE_ITEM_ACK2 *)pkt); break;
    case HEADER_ZC_SKILL_ENTRY2: game_mode_zc_skill_entry2(self, (struct PACKET_ZC_SKILL_ENTRY2 *)pkt); break;
    case HEADER_ZC_AUTOSPELLLIST: game_mode_zc_auto_spell_list(pkt); break;
    case HEADER_ZC_DEVOTIONLIST: game_mode_zc_devotion_list(self, (const struct PacketDevotionList *)pkt); break;
    case HEADER_ZC_SPIRITS: {
        const struct PACKET_ZC_SPIRITS *packet = (const struct PACKET_ZC_SPIRITS *)pkt;
        game_mode_on_spirits(self, packet->aid, packet->num);
        break;
    }
    case HEADER_ZC_BLADESTOP: game_mode_zc_blade_stop(self, pkt); break;
    case HEADER_ZC_COMBODELAY: game_mode_zc_combo_delay(self, (const struct PACKET_ZC_COMBODELAY *)pkt); break;
    case HEADER_ZC_SOUND: game_mode_zc_sound(self, pkt); break;
    case HEADER_ZC_OPEN_EDITDLGSTR: game_mode_zc_open_edit_dlg_str(self, pkt); break;
    case HEADER_ZC_NOTIFY_MAPPROPERTY2: game_mode_notify_map_property2(self, (const struct PacketMapProperty *)pkt); break;
    case HEADER_ZC_SPRITE_CHANGE2: game_mode_zc_sprite_change2(self, (const struct PACKET_ZC_SPRITE_CHANGE2 *)pkt); break;
    case HEADER_ZC_NOTIFY_STANDENTRY2: game_mode_zc_notify_standentry2(self, (const struct PacketStandEntry2 *)pkt); break;
    case HEADER_ZC_NOTIFY_NEWENTRY2: game_mode_zc_notify_newentry2(self, (const struct PacketNewEntry2 *)pkt); break;
    case HEADER_ZC_NOTIFY_MOVEENTRY2: game_mode_zc_notify_moveentry2(self, (const struct PacketMoveEntry2 *)pkt); break;
    case HEADER_ZC_NOTIFY_SKILL2: game_mode_on_notify_skill2(self, (const struct PacketNotifySkill2 *)pkt); break;
    case HEADER_ZC_SPIRITS2: game_mode_on_spirits2(self, (const struct PacketSpirits2 *)pkt); break;
    case HEADER_ZC_REQ_COUPLE: game_mode_zc_req_couple(self, pkt); break;
    case HEADER_ZC_START_COUPLE: game_mode_on_start_couple(self, pkt); break;
    case HEADER_ZC_COUPLENAME: game_mode_zc_couple_name(self, pkt); break;
    case HEADER_ZC_CONGRATULATION: game_mode_on_congratulation(self, (const struct PacketCongratulation *)pkt); break;
    case HEADER_ZC_NOTIFY_POSITION_TO_GUILDM: game_mode_zc_notify_position_to_guild_m(self, pkt); break;
    case HEADER_ZC_GUILD_MEMBER_MAP_CHANGE: game_mode_zc_guild_member_map_change(self, pkt); break;
    case HEADER_ZC_NORMAL_ITEMLIST2: game_mode_zc_normal_item_list(pkt, 2); break;
    case HEADER_ZC_CART_NORMAL_ITEMLIST2: game_mode_zc_cart_normal_item_list(pkt, 2); break;
    case HEADER_ZC_STORE_NORMAL_ITEMLIST2: game_mode_zc_store_normal_item_list(pkt, 2); break;
    case HEADER_ZC_UPDATE_CHARSTAT2: game_mode_zc_update_charstat2(self, (const struct PACKET_ZC_UPDATE_CHARSTAT2 *)pkt); break;
    case HEADER_ZC_NOTIFY_EFFECT2: game_mode_on_notify_effect2(self, (const struct PacketNotifyEffect2 *)pkt); break;
    case HEADER_ZC_REQ_EXCHANGE_ITEM2: game_mode_zc_req_exchange_item2(self, pkt); break;
    case HEADER_ZC_ACK_EXCHANGE_ITEM2: game_mode_ack_exchange_item2(self, pkt); break;
    case HEADER_ZC_REQ_BABY: game_mode_zc_req_baby(self, pkt); break;
    case HEADER_ZC_START_BABY: game_mode_on_start_baby(self, pkt); break;
    case HEADER_ZC_REPAIRITEMLIST: game_mode_zc_repair_item_list(self, (const struct PacketRepairItemList *)pkt); break;
    case HEADER_ZC_ACK_ITEMREPAIR: game_mode_zc_ack_item_repair(self, (const struct PacketItemRepair *)pkt); break;
    case HEADER_ZC_HIGHJUMP: game_mode_zc_high_jump(self, pkt); break;
    case HEADER_ZC_FRIENDS_LIST: game_mode_friends_list(pkt); break;
    case HEADER_ZC_DIVORCE: game_mode_zc_divorce(self, (const struct PACKET_ZC_DIVORCE *)pkt); break;
    case HEADER_ZC_FRIENDS_STATE: game_mode_friends_state(pkt); break;
    case HEADER_ZC_REQ_ADD_FRIENDS: game_mode_zc_req_add_friends(self, pkt); break;
    case HEADER_ZC_ADD_FRIENDS_LIST: game_mode_add_friends_list(pkt); break;
    case HEADER_ZC_DELETE_FRIENDS: game_mode_zc_delete_friends(self, pkt); break;
    case HEADER_ZC_STARSKILL: game_mode_zc_star_skill(self, (struct PACKET_ZC_STARSKILL *)pkt); break;
    case HEADER_ZC_ACK_STATUS_GM: game_mode_ack_status_gm((const struct PACKET_ZC_ACK_STATUS_GM *)pkt); break;
    case HEADER_ZC_SKILLMSG: game_mode_zc_skill_msg(self, (const struct PACKET_ZC_SKILLMSG *)pkt); break;
    case HEADER_ZC_BABYMSG: game_mode_zc_baby_msg(pkt); break;
    case HEADER_ZC_NOTIFY_WEAPONITEMLIST: game_mode_zc_notify_weapon_item_list(self, (const struct PacketWeaponItemList *)pkt); break;
    case HEADER_ZC_ACK_WEAPONREFINE: game_mode_zc_ack_weapon_refine(self, (const struct PacketWeaponRefine *)pkt); break;
    case HEADER_ZC_STATE_CHANGE3: game_mode_zc_state_change3(self, (const struct PACKET_ZC_STATE_CHANGE3 *)pkt); break;
    case HEADER_ZC_NOTIFY_STANDENTRY3: game_mode_zc_notify_standentry3(self, (const struct PacketStandEntry4 *)pkt); break;
    case HEADER_ZC_NOTIFY_NEWENTRY3: game_mode_zc_notify_newentry3(self, (const struct PacketNewEntry4 *)pkt); break;
    case HEADER_ZC_NOTIFY_MOVEENTRY3: game_mode_zc_notify_moveentry3(self, (const struct PacketMoveEntry3 *)pkt); break;
    case HEADER_ZC_PROPERTY_HOMUN: game_mode_zc_property_homun(self, pkt); break;
    case HEADER_ZC_CHANGESTATE_MER: game_mode_zc_change_state_homun(self, pkt); break;
    case HEADER_ZC_STARPLACE: game_mode_zc_star_place(pkt); break;
    case HEADER_ZC_MAKINGITEM_LIST: game_mode_zc_making_item_list(pkt); break;
    case HEADER_ZC_HUNTINGLIST: game_mode_zc_hunting_list(self, (const struct PACKET_ZC_HUNTINGLIST *)pkt); break;
    case HEADER_ZC_PCBANG_EFFECT: game_mode_zc_pc_bang_effect(self, (const struct PACKET_ZC_PCBANG_EFFECT *)pkt); break;
    case HEADER_ZC_NOTIFY_EFFECT3: game_mode_on_notify_effect3(self, (const struct PacketNotifyEffect3 *)pkt); break;
    case HEADER_ZC_PC_CASH_POINT_ITEMLIST: game_mode_zc_pc_cash_point_itemlist(self, pkt); break;
    case HEADER_ZC_PC_CASH_POINT_UPDATE: game_mode_zc_pc_cash_point_update(self, pkt); break;
    case HEADER_ZC_NPC_SHOWEFST_UPDATE: game_mode_on_npc_show_efst_update(self, (const struct PacketNpcShowEfstUpdate *)pkt); break;
    case HEADER_ZC_MSG: game_mode_msg(pkt); break;
    case HEADER_ZC_BOSS_INFO: game_mode_zc_boss_info(self, (const struct PACKET_ZC_BOSS_INFO *)pkt); break;
    case HEADER_ZC_EQUIPMENT_ITEMLIST2: game_mode_equipment_item_list2(pkt); break;
    case HEADER_ZC_STORE_EQUIPMENT_ITEMLIST2: game_mode_zc_store_equipment_item_list2(self, (const struct PACKET_STORE_LIST *)pkt); break;
    case HEADER_ZC_CART_EQUIPMENT_ITEMLIST2: game_mode_cart_equipment_item_list2(pkt); break;
    case HEADER_ZC_CASH_TIME_COUNTER: game_mode_cash_time_counter(pkt); break;
    case HEADER_ZC_CASH_ITEM_DELETE: game_mode_cash_item_delete(pkt); break;
    case HEADER_ZC_ITEM_PICKUP_ACK2: game_mode_item_pickup_ack2(self, pkt); break;
    case HEADER_ZC_MER_INIT: game_mode_zc_mer_init(self, (const struct PacketMerInit *)pkt); break;
    case HEADER_ZC_MER_PAR_CHANGE: game_mode_zc_mer_par_change(pkt); break;
    case HEADER_ZC_DEL_QUEST: game_mode_del_quest(self, pkt, packet_size); break;
    case HEADER_ZC_ACTIVE_QUEST: game_mode_zc_active_quest(pkt, packet_size); break;
    case HEADER_ZC_ITEM_PICKUP_PARTY: game_mode_item_pickup_party(pkt); break;
    case HEADER_ZC_EQUIPITEM_DAMAGED: game_mode_zc_equip_item_damaged(self, (const struct PACKET_ZC_EQUIPITEM_DAMAGED *)pkt); break;
    case HEADER_ZC_NPC_CHAT: game_mode_zc_npc_chat(self, pkt); break;
    case HEADER_ZC_FORMATSTRING_MSG: game_mode_format_string_msg(pkt); break;
    case HEADER_ZC_PARTY_CONFIG: game_mode_zc_party_config(pkt); break;
    case HEADER_ZC_MEMORIALDUNGEON_SUBSCRIPTION_INFO: game_mode_memorial_dungeon_subscription_info(pkt); break;
    case HEADER_ZC_MEMORIALDUNGEON_SUBSCRIPTION_NOTIFY: game_mode_memorial_dungeon_subscription_notify(pkt); break;
    case HEADER_ZC_MEMORIALDUNGEON_INFO: game_mode_memorial_dungeon_info(pkt); break;
    case HEADER_ZC_MEMORIALDUNGEON_NOTIFY: game_mode_memorial_dungeon_notify(pkt); break;
    case HEADER_ZC_EQUIPMENT_ITEMLIST3: game_mode_equipment_item_list3(pkt); break;
    case HEADER_ZC_STORE_EQUIPMENT_ITEMLIST3: game_mode_zc_store_equipment_item_list3(self, (const struct PACKET_STORE_LIST *)pkt); break;
    case HEADER_ZC_CART_EQUIPMENT_ITEMLIST3: game_mode_cart_equipment_item_list3(pkt); break;
    case HEADER_ZC_NOTIFY_BIND_ON_EQUIP: game_mode_zc_notify_bind_on_equip(pkt); break;
    case HEADER_ZC_ITEM_PICKUP_ACK3: game_mode_item_pickup_ack3(self, pkt); break;
    case HEADER_ZC_ISVR_DISCONNECT: game_mode_isvr_disconnect(self); break;
    case HEADER_ZC_EQUIPWIN_MICROSCOPE:
    case HEADER_ZC_EQUIPWIN_MICROSCOPE2: game_mode_equip_win_microscope2(pkt); break;
    case HEADER_ZC_CONFIG: game_mode_zc_config(self, pkt); break;
    case HEADER_ZC_CONFIG_NOTIFY: game_mode_zc_config_notify(self, pkt); break;
    case HEADER_ZC_BATTLEFIELD_CHAT: game_mode_zc_battlefield_chat(self, pkt); break;
    case HEADER_ZC_BATTLEFIELD_NOTIFY_CAMPINFO: game_mode_zc_battlefield_notify_camp_info(self, pkt); break;
    case HEADER_ZC_BATTLEFIELD_NOTIFY_POINT: game_mode_zc_battlefield_notify_point(pkt); break;
    case HEADER_ZC_BATTLEFIELD_NOTIFY_POSITION: game_mode_zc_battlefield_notify_position(self, pkt); break;
    case HEADER_ZC_BATTLEFIELD_NOTIFY_HP: game_mode_zc_battlefield_notify_hp(self, pkt); break;
    case HEADER_ZC_NOTIFY_ACT2:
    {
        const struct PACKET_ZC_NOTIFY_ACT2 *packet = (const struct PACKET_ZC_NOTIFY_ACT2 *)pkt;
        game_mode_notify_act(self, packet->aid, packet->target, packet->start_time, packet->attack_mt,
                           packet->attacked_mt, packet->damage, packet->count, packet->action,
                           packet->left_damage, 0);
        break;
    }
    case HEADER_ZC_NORMAL_ITEMLIST3: game_mode_zc_normal_item_list(pkt, 3); break;
    case HEADER_ZC_CART_NORMAL_ITEMLIST3: game_mode_zc_cart_normal_item_list(pkt, 3); break;
    case HEADER_ZC_STORE_NORMAL_ITEMLIST3: game_mode_zc_store_normal_item_list(pkt, 3); break;
    case HEADER_ZC_ACCEPT_ENTER2: game_mode_accept_enter2(pkt); break;
    case HEADER_ZC_NOTIFY_MOVEENTRY4:
    case HEADER_ZC_NOTIFY_MOVEENTRY7:
    case HEADER_ZC_NOTIFY_MOVEENTRY8: game_mode_zc_notify_moveentry6(self, (const struct PacketMoveEntry6 *)pkt); break;
    case HEADER_ZC_NOTIFY_NEWENTRY4: game_mode_zc_notify_newentry4(self, (const struct PacketNewEntry4 *)pkt); break;
    case HEADER_ZC_NOTIFY_STANDENTRY4: game_mode_zc_notify_standentry4(self, (const struct PacketStandEntry4 *)pkt); break;
    case HEADER_ZC_NOTIFY_FONT: game_mode_zc_notify_font(self, pkt); break;
    case HEADER_ZC_PROGRESS: game_mode_zc_progress(self, pkt); break;
    case HEADER_ZC_PROGRESS_CANCEL: game_mode_zc_progress_cancel(self, pkt); break;
    case HEADER_ZC_UPDATE_GDID2: game_mode_zc_update_gdid2(self, (const struct PACKET_ZC_UPDATE_GDID *)pkt); break;
    case HEADER_ZC_SKILL_POSTDELAY: skill_cooldowns_receive_delay(pkt, packet_size); break;
    case HEADER_ZC_SKILL_POSTDELAY_LIST: skill_cooldowns_receive_list(pkt, packet_size); break;
    case HEADER_ZC_MSG_STATE_CHANGE2: game_mode_on_msg_state_change2(self, (const struct PacketMsgStateChange2 *)pkt); break;
    case HEADER_ZC_MILLENNIUMSHIELD: game_mode_on_millennium_shield(self, (const struct PacketMillenniumShield *)pkt); break;
    case HEADER_ZC_SKILLINFO_DELETE: session_drop_skill(&g_session1, ((const struct PACKET_ZC_SKILLINFO_DELETE *)pkt)->skill_id); break;
    case HEADER_ZC_SKILL_SELECT_REQUEST: game_mode_zc_skill_select_request((const struct PACKET_ZC_SKILL_SELECT_REQUEST *)pkt); break;
    case HEADER_ZC_QUEST_NOTIFY_EFFECT: game_mode_zc_quest_notify_effect(self, (const struct PACKET_ZC_QUEST_NOTIFY_EFFECT *)pkt); break;
    case HEADER_ZC_REQ_GROUPINFO_CHANGE_V2: game_mode_zc_req_groupinfo_change_v2(pkt); break;
    case HEADER_ZC_HO_PAR_CHANGE: game_mode_zc_ho_par_change(pkt); break;
    case HEADER_ZC_SKILLINFO_UPDATE2:
    {
        const struct PACKET_ZC_SKILLINFO_UPDATE2 *packet = (const struct PACKET_ZC_SKILLINFO_UPDATE2 *)pkt;
        game_mode_skill_update(self, packet->skill_id, packet->level, packet->sp_cost, packet->upgradable,
                             packet->range, packet->type, packet->level);
        break;
    }
    case HEADER_ZC_MSG_VALUE: game_mode_msg_value(pkt); break;
    case HEADER_ZC_ITEMLISTWIN_OPEN: game_mode_item_list_win_open(self, pkt); break;
    case HEADER_ZC_MSG_SKILL: game_mode_zc_msg_skill(pkt); break;
    case HEADER_ZC_NOTIFY_EXP: game_mode_on_notify_exp(self, (const struct PacketNotifyExp *)pkt); break;
    case HEADER_ZC_NOTIFY_NEWENTRY5:
    case HEADER_ZC_NOTIFY_NEWENTRY6: game_mode_zc_notify_newentry5(self, (const struct PacketNewEntry5 *)pkt); break;
    case HEADER_ZC_NOTIFY_STANDENTRY5:
    case HEADER_ZC_NOTIFY_STANDENTRY7: game_mode_zc_notify_standentry5(self, (const struct PacketStandEntry5 *)pkt); break;
    case HEADER_ZC_DELETE_ITEM_FROM_BODY: game_mode_delete_item_from_body(self, pkt); break;
    case HEADER_ZC_USESKILL_ACK2: game_mode_on_use_skill_ack(self, (const struct PacketUseSkillAck *)pkt, 1); break;
    case HEADER_ZC_CHANGE_GROUP_MASTER: game_mode_change_group_master(pkt); break;
    case HEADER_ZC_BROADCASTING_SPECIAL_ITEM_OBTAIN: game_mode_broadcast_special_item_obtain(self, pkt); break;
    case HEADER_ZC_PLAY_NPC_BGM: game_mode_zc_play_npc_bgm(self, pkt); break;
    case HEADER_ZC_NOTIFY_INITCHAR:
    case HEADER_ZC_NOTIFY_UPDATECHAR:
    case HEADER_ZC_NOTIFY_UPDATEPLAYER:
    case HEADER_ZC_NOTIFY_ACTENTRY:
    case HEADER_ZC_ACCEPT_QUIT:
    case HEADER_ZC_REFUSE_QUIT:
    case HEADER_ZC_ACK_WHISPER:
    case HEADER_ZC_OTHER_GUILD_LIST:
    case HEADER_PING:
    case HEADER_ZC_MONSTER_TALK:
    case HEADER_ZC_ACK_ACCOUNTNAME:
    case HEADER_ZC_ADD_MEMBER_TO_GROUP2:
    case HEADER_ZC_AID:
        break;
    default:
        game_mode_dispatch_packet2(self, packet_id, pkt, packet_size);
        break;
    }
}
