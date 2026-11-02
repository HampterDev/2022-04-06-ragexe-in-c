#include "packet_dispatch_trace.h"
#include <stddef.h>
#include <stdint.h>
#ifndef PACKET_DISPATCH_REFERENCE
#include "game/packet_handlers.h"
#include "session/skill_commands.h"
#include "platform/error_message.h"
#endif

struct AchievementEntry;
struct BuyingStoreEntry;
struct Connection;
struct GameMode;
struct RagConnection;
struct Session;
struct EquipItemV7;
struct MarketBoughtItem;
struct NormalItem;
struct NpcMarketEntry;
struct PACKET_SC_NOTIFY_BAN;
struct PACKET_STORE_LIST;
struct PACKET_STORE_LIST_NAMED;
struct PACKET_ZC_ACK_ADD_ITEM_RODEX;
struct PACKET_ZC_ACK_ADD_ITEM_RODEX2;
struct PACKET_ZC_ACK_ALL_RODEX_LIST;
struct PACKET_ZC_ACK_CLOSE_GUILD_STORAGE;
struct PACKET_ZC_ACK_DELETE_RODEX;
struct PACKET_ZC_ACK_EXTEND_BODYITEM_SIZE;
struct PACKET_ZC_ACK_GUILDSTORAGE_LOG;
struct PACKET_ZC_ACK_GUILDSTORAGE_LOG2;
struct PACKET_ZC_ACK_MERGE_ITEM;
struct PACKET_ZC_ACK_OPEN_GUILD_STORAGE;
struct PACKET_ZC_ACK_OPEN_MSGBOX_EXTEND_BODYITEM_SIZE;
struct PACKET_ZC_ACK_OPEN_WRITE_RODEX;
struct PACKET_ZC_ACK_READ_RODEX2;
struct PACKET_ZC_ACK_REMOVE_RODEX_ITEM;
struct PACKET_ZC_ACK_REQ_DISCONNECT;
struct PACKET_ZC_ACK_RODEX_LIST;
struct PACKET_ZC_ACK_SEND_RODEX;
struct PACKET_ZC_ACK_STATUS_GM;
struct PACKET_ZC_ACK_TOUSESKILL;
struct PACKET_ZC_ACK_ZENY_FROM_RODEX;
struct PACKET_ZC_BOSS_INFO;
struct PACKET_ZC_CHANGE_GUILD;
struct PACKET_ZC_CHANGE_ITEM_OPTION3;
struct PACKET_ZC_COMBODELAY;
struct PACKET_ZC_COUPLESTATUS;
struct PACKET_ZC_C_MARKERINFO;
struct PACKET_ZC_DIVORCE;
struct PACKET_ZC_ENTER_ROOM;
struct PACKET_ZC_EQUIPITEM_DAMAGED;
struct PACKET_ZC_EXTEND_BODYITEM_SIZE;
struct PACKET_ZC_GUILD_INFO2;
struct PACKET_ZC_GUILD_POSITION;
struct PACKET_ZC_GUILD_SKILLINFO;
struct PACKET_ZC_HP_INFO;
struct PACKET_ZC_HP_INFO_TINY;
struct PACKET_ZC_HUNTINGLIST;
struct PACKET_ZC_INFO_REMAINTIME;
struct PACKET_ZC_MONSTER_INFO;
struct PACKET_ZC_NOTIFY_MAPINFO;
struct PACKET_ZC_PAR_CHANGE;
struct PACKET_ZC_PCBANG_EFFECT;
struct PACKET_ZC_QUEST_NOTIFY_EFFECT;
struct PACKET_ZC_REPLY_REMAINTIME;
struct PACKET_ZC_REQ_GUILD_INVITE;
struct PACKET_ZC_RESULT_MAKE_GUILD;
struct PACKET_ZC_RESURRECTION;
struct PACKET_ZC_SEARCH_STORE_POS;
struct PACKET_ZC_SHORTCUT_KEY_LIST_V4;
struct PACKET_ZC_SHOWDIGIT;
struct PACKET_ZC_SHOW_IMAGE;
struct PACKET_ZC_SHOW_IMAGE2;
struct PACKET_ZC_SKILLINFO_UPDATE;
struct PACKET_ZC_SKILLMSG;
struct PACKET_ZC_SKILL_ENTRY;
struct PACKET_ZC_SKILL_ENTRY2;
struct PACKET_ZC_SKILL_ENTRY3;
struct PACKET_ZC_SKILL_ENTRY4;
struct PACKET_ZC_SKILL_ENTRY5;
struct PACKET_ZC_SKILL_SELECT_REQUEST;
struct PACKET_ZC_SKILL_UPDATE;
struct PACKET_ZC_SPLIT_SEND_ITEMLIST_EQUIPV8;
struct PACKET_ZC_SPLIT_SEND_ITEMLIST_SET;
struct PACKET_ZC_SPRITE_CHANGE2;
struct PACKET_ZC_STARSKILL;
struct PACKET_ZC_STATE_CHANGE;
struct PACKET_ZC_STATE_CHANGE3;
struct PACKET_ZC_STATUS;
struct PACKET_ZC_STATUS_CHANGE;
struct PACKET_ZC_STATUS_CHANGE_ACK;
struct PACKET_ZC_TALKBOX_CHATCONTENTS;
struct PACKET_ZC_UPDATE_CARDSLOT;
struct PACKET_ZC_UPDATE_CHARSTAT;
struct PACKET_ZC_UPDATE_CHARSTAT2;
struct PACKET_ZC_UPDATE_GDID;
struct PACKET_ZC_UPDATE_MAPINFO;
struct PACKET_ZC_USE_ITEM_ACK;
struct PACKET_ZC_USE_ITEM_ACK2;
struct PacketAckReqNameAll2;
struct PacketActionFailure;
struct PacketAddExchangeItemGrade;
struct PacketAddExchangeItemOpt;
struct PacketAddMemberToGroup;
struct PacketAddMemberToGroup2;
struct PacketBanList;
struct PacketBankingAck;
struct PacketBankingCheck;
struct PacketClearEffect;
struct PacketCongratulation;
struct PacketDevotionList;
struct PacketDispel;
struct PacketEfstSetEnter;
struct PacketEfstSetEnter2;
struct PacketEquipmentEffect;
struct PacketGroundItemEntry;
struct PacketGroundItemFall;
struct PacketGroundItemFall4;
struct PacketGroupList2;
struct PacketGroupList3;
struct PacketGuildAgitInfo;
struct PacketGuildBanByGid;
struct PacketGuildInfo;
struct PacketGuildInfoNew;
struct PacketGuildLeave;
struct PacketGuildLeaveByGid;
struct PacketGuildMember;
struct PacketGuildNotice;
struct PacketGuildRelations;
struct PacketItemComposition;
struct PacketItemFallEntry5;
struct PacketItemPickupParty2;
struct PacketItemRefining;
struct PacketItemRepair;
struct PacketLatestRodexList;
struct PacketListFlagClear;
struct PacketLongLongParChange;
struct PacketLongParChange;
struct PacketMakableItemList;
struct PacketMakingItem;
struct PacketMapProperty;
struct PacketMapPropertyR2;
struct PacketMemberFull;
struct PacketMemberList;
struct PacketMemberMemo;
struct PacketMemberMgrInfo;
struct PacketMemberNamed;
struct PacketMerInit;
struct PacketMergeItemOpen;
struct PacketMillenniumShield;
struct PacketMoveEntry;
struct PacketMoveEntry10;
struct PacketMoveEntry11;
struct PacketMoveEntry2;
struct PacketMoveEntry3;
struct PacketMoveEntry6;
struct PacketMoveEntry9;
struct PacketMsgStateChange;
struct PacketMsgStateChange2;
struct PacketMsgStateChange3;
struct PacketNavigationActive;
struct PacketNewEntry;
struct PacketNewEntry10;
struct PacketNewEntry11;
struct PacketNewEntry2;
struct PacketNewEntry4;
struct PacketNewEntry5;
struct PacketNewEntry7;
struct PacketNotifyEffect;
struct PacketNotifyEffect2;
struct PacketNotifyEffect3;
struct PacketNotifyEffect4;
struct PacketNotifyExp;
struct PacketNotifyExp2;
struct PacketNotifyGroundSkill;
struct PacketNotifyMemberInfoToGroupm;
struct PacketNotifyRanking;
struct PacketNotifySkill;
struct PacketNotifySkill2;
struct PacketNotifySkillPosition;
struct PacketNotifyUnreadRodex;
struct PacketNpcShowEfstUpdate;
struct PacketPartyMemberState;
struct PacketPersonalInfo;
struct PacketPositionInfo;
struct PacketPositionNames;
struct PacketPositions;
struct PacketPrNpcState;
struct PacketPurchaseItemListFromMc4;
struct PacketPurchaseMyItemList3;
struct PacketRecoverPenaltyOverweight;
struct PacketRepairItemList;
struct PacketRepairItemList2;
struct PacketRoomEntry;
struct PacketShowScript;
struct PacketShowWarpList;
struct PacketSoulEnergy;
struct PacketSpirits2;
struct PacketSpiritsAttribute;
struct PacketStandEntry;
struct PacketStandEntry10;
struct PacketStandEntry11;
struct PacketStandEntry2;
struct PacketStandEntry4;
struct PacketStandEntry5;
struct PacketStandEntry8;
struct PacketStandEntryNpc;
struct PacketTryResult;
struct PacketUseSkillAck;
struct PacketVendingList4;
struct PacketWarpList;
struct PacketWarpList2;
struct PacketWeaponItemList;
struct PacketWeaponRefine;
struct UserConfigEntry;
struct VendingEntry;

void game_mode_item_composition_list(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("CGameMode_ItemCompositionList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_item_identify_list(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("CGameMode_ItemIdentifyList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void session_drop_skill(struct Session *arg0, int arg1)
{
    packet_trace_call("CSession_DropSkill");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}

void client_add_skill(unsigned short arg0, short arg1, int arg2, unsigned char arg3, short arg4,
                     short arg5, short arg6)
{
    packet_trace_call("Client_AddSkill");
    packet_trace_value(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
    packet_trace_value(arg5);
    packet_trace_value(arg6);
}

void game_mode_accept_enter(const unsigned char *arg0)
{
    packet_trace_call("GameMode_AcceptEnter");
    packet_trace_pointer(arg0);
}

void game_mode_accept_enter2(const unsigned char *arg0)
{
    packet_trace_call("GameMode_AcceptEnter2");
    packet_trace_pointer(arg0);
}

void game_mode_ack_add_exchange_item(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_AckAddExchangeItem");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ack_banking_deposit(struct GameMode *arg0, const struct PacketBankingAck *arg1)
{
    packet_trace_call("GameMode_AckBankingDeposit");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ack_banking_withdraw(struct GameMode *arg0, const struct PacketBankingAck *arg1)
{
    packet_trace_call("GameMode_AckBankingWithdraw");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ack_buy_styling_shop(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_AckBuyStylingShop");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ack_buying_store_list(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_AckBuyingStoreList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ack_check_attendance(struct GameMode *arg0, int arg1, int arg2)
{
    packet_trace_call("GameMode_AckCheckAttendance");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
}

void game_mode_ack_close_guild_storage(const struct PACKET_ZC_ACK_CLOSE_GUILD_STORAGE *arg0)
{
    packet_trace_call("GameMode_AckCloseGuildStorage");
    packet_trace_pointer(arg0);
}

void game_mode_ack_exchange_item(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_AckExchangeItem");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ack_exchange_item2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_AckExchangeItem2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ack_guild_storage_log(const struct PACKET_ZC_ACK_GUILDSTORAGE_LOG *arg0)
{
    packet_trace_call("GameMode_AckGuildStorageLog");
    packet_trace_pointer(arg0);
}

void game_mode_ack_guild_storage_log2(const struct PACKET_ZC_ACK_GUILDSTORAGE_LOG2 *arg0)
{
    packet_trace_call("GameMode_AckGuildStorageLog2");
    packet_trace_pointer(arg0);
}

void game_mode_ack_open_guild_storage(const struct PACKET_ZC_ACK_OPEN_GUILD_STORAGE *arg0)
{
    packet_trace_call("GameMode_AckOpenGuildStorage");
    packet_trace_pointer(arg0);
}

void game_mode_ack_req_disconnect(struct GameMode *arg0,
                               const struct PACKET_ZC_ACK_REQ_DISCONNECT *arg1)
{
    packet_trace_call("GameMode_AckReqDisconnect");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ack_req_name(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_AckReqName");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ack_req_name_all(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_AckReqNameAll");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ack_req_name_all_npc(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_AckReqNameAllNpc");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ack_req_name_by_gid(const unsigned char *arg0)
{
    packet_trace_call("GameMode_AckReqNameByGid");
    packet_trace_pointer(arg0);
}

void game_mode_ack_req_name_by_gid2(const unsigned char *arg0)
{
    packet_trace_call("GameMode_AckReqNameByGid2");
    packet_trace_pointer(arg0);
}

void game_mode_ack_status_gm(const struct PACKET_ZC_ACK_STATUS_GM *arg0)
{
    packet_trace_call("GameMode_AckStatusGM");
    packet_trace_pointer(arg0);
}

void game_mode_ack_takeoff_equip_v5(struct GameMode *arg0, unsigned char arg1, int arg2,
                                unsigned short arg3)
{
    packet_trace_call("GameMode_AckTakeoffEquipV5");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
}

void game_mode_ack_to_use_skill(struct GameMode *arg0, const struct PACKET_ZC_ACK_TOUSESKILL *arg1)
{
    packet_trace_call("GameMode_AckToUseSkill");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ack_whisper(struct GameMode *arg0, unsigned char arg1, int arg2)
{
    packet_trace_call("GameMode_AckWhisper");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
}

void game_mode_add_exchange_item2(const unsigned char *arg0)
{
    packet_trace_call("GameMode_AddExchangeItem2");
    packet_trace_pointer(arg0);
}

void game_mode_add_exchange_item3(const unsigned char *arg0)
{
    packet_trace_call("GameMode_AddExchangeItem3");
    packet_trace_pointer(arg0);
}

void game_mode_add_friends_list(const unsigned char *arg0)
{
    packet_trace_call("GameMode_AddFriendsList");
    packet_trace_pointer(arg0);
}

void game_mode_add_item_to_cart2(const unsigned char *arg0)
{
    packet_trace_call("GameMode_AddItemToCart2");
    packet_trace_pointer(arg0);
}

void game_mode_add_item_to_cart4(const unsigned char *arg0)
{
    packet_trace_call("GameMode_AddItemToCart4");
    packet_trace_pointer(arg0);
}

void game_mode_add_item_to_store2(const unsigned char *arg0)
{
    packet_trace_call("GameMode_AddItemToStore2");
    packet_trace_pointer(arg0);
}

void game_mode_add_item_to_store3(const unsigned char *arg0)
{
    packet_trace_call("GameMode_AddItemToStore3");
    packet_trace_pointer(arg0);
}

void game_mode_add_item_to_store4(const unsigned char *arg0)
{
    packet_trace_call("GameMode_AddItemToStore4");
    packet_trace_pointer(arg0);
}

void game_mode_add_member_to_group(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_AddMemberToGroup");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_all_quest_list3(struct GameMode *arg0, const void *arg1, size_t arg2)
{
    packet_trace_call("GameMode_AllQuestList3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
}

void game_mode_all_quest_list4(struct GameMode *arg0, const void *arg1, size_t arg2)
{
    packet_trace_call("GameMode_AllQuestList4");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
}

void game_mode_banking_check(struct GameMode *arg0, const struct PacketBankingCheck *arg1)
{
    packet_trace_call("GameMode_BankingCheck");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_broadcast(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Broadcast");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_broadcast2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Broadcast2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_broadcast4(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Broadcast4");
    packet_trace_pointer(arg0);
}

void game_mode_broadcast_special_item_obtain(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_BroadcastSpecialItemObtain");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_buying_store_bought_packet(const unsigned char *arg0)
{
    packet_trace_call("GameMode_BuyingStoreBoughtPacket");
    packet_trace_pointer(arg0);
}

void game_mode_buying_store_entry(struct GameMode *arg0,
                               const struct PACKET_ZC_TALKBOX_CHATCONTENTS *arg1)
{
    packet_trace_call("GameMode_BuyingStoreEntry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_camera_info(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_CameraInfo");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_cart_equipment_item_list2(const unsigned char *arg0)
{
    packet_trace_call("GameMode_CartEquipmentItemList2");
    packet_trace_pointer(arg0);
}

void game_mode_cart_equipment_item_list3(const unsigned char *arg0)
{
    packet_trace_call("GameMode_CartEquipmentItemList3");
    packet_trace_pointer(arg0);
}

void game_mode_cart_item_list_equip_v5(const unsigned char *arg0)
{
    packet_trace_call("GameMode_CartItemListEquipV5");
    packet_trace_pointer(arg0);
}

void game_mode_cart_item_list_equip_v6(const unsigned char *arg0)
{
    packet_trace_call("GameMode_CartItemListEquipV6");
    packet_trace_pointer(arg0);
}

void game_mode_cart_item_list_normal_v5(const unsigned char *arg0)
{
    packet_trace_call("GameMode_CartItemListNormalV5");
    packet_trace_pointer(arg0);
}

void game_mode_cash_item_delete(const unsigned char *arg0)
{
    packet_trace_call("GameMode_CashItemDelete");
    packet_trace_pointer(arg0);
}

void game_mode_cash_time_counter(const unsigned char *arg0)
{
    packet_trace_call("GameMode_CashTimeCounter");
    packet_trace_pointer(arg0);
}

void game_mode_change_group_master(const unsigned char *arg0)
{
    packet_trace_call("GameMode_ChangeGroupMaster");
    packet_trace_pointer(arg0);
}

void game_mode_config_notify2(struct GameMode *arg0, unsigned char arg1, unsigned char arg2)
{
    packet_trace_call("GameMode_ConfigNotify2");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
}

void game_mode_config_notify3(struct GameMode *arg0, unsigned char arg1, unsigned char arg2,
                            unsigned char arg3)
{
    packet_trace_call("GameMode_ConfigNotify3");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
}

void game_mode_config_notify4(struct GameMode *arg0, unsigned char arg1, unsigned char arg2,
                            unsigned char arg3, unsigned char arg4)
{
    packet_trace_call("GameMode_ConfigNotify4");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
}

void game_mode_del_quest(struct GameMode *arg0, const void *arg1, size_t arg2)
{
    packet_trace_call("GameMode_DelQuest");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
}

void game_mode_delete_item_from_body(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_DeleteItemFromBody");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_delete_member_from_group(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_DeleteMemberFromGroup");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_disappear_buying_store_entry(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_DisappearBuyingStoreEntry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_equip_win_microscope2(const unsigned char *arg0)
{
    packet_trace_call("GameMode_EquipWinMicroscope2");
    packet_trace_pointer(arg0);
}

void game_mode_equip_win_microscope4(const unsigned char *arg0)
{
    packet_trace_call("GameMode_EquipWinMicroscope4");
    packet_trace_pointer(arg0);
}

void game_mode_equip_win_microscope_v5(const unsigned char *arg0)
{
    packet_trace_call("GameMode_EquipWinMicroscopeV5");
    packet_trace_pointer(arg0);
}

void game_mode_equip_win_microscope_v6(const unsigned char *arg0)
{
    packet_trace_call("GameMode_EquipWinMicroscopeV6");
    packet_trace_pointer(arg0);
}

void game_mode_equip_win_microscope_v6_body(const unsigned char *arg0)
{
    packet_trace_call("GameMode_EquipWinMicroscopeV6Body");
    packet_trace_pointer(arg0);
}

void game_mode_equip_win_microscope_v7(const unsigned char *arg0)
{
    packet_trace_call("GameMode_EquipWinMicroscopeV7");
    packet_trace_pointer(arg0);
}

void game_mode_equipment_item_list2(const unsigned char *arg0)
{
    packet_trace_call("GameMode_EquipmentItemList2");
    packet_trace_pointer(arg0);
}

void game_mode_equipment_item_list3(const unsigned char *arg0)
{
    packet_trace_call("GameMode_EquipmentItemList3");
    packet_trace_pointer(arg0);
}

void game_mode_equipment_item_list4(const unsigned char *arg0)
{
    packet_trace_call("GameMode_EquipmentItemList4");
    packet_trace_pointer(arg0);
}

void game_mode_equipment_item_list_v5(const unsigned char *arg0)
{
    packet_trace_call("GameMode_EquipmentItemListV5");
    packet_trace_pointer(arg0);
}

void game_mode_equipment_item_list_v6(const unsigned char *arg0)
{
    packet_trace_call("GameMode_EquipmentItemListV6");
    packet_trace_pointer(arg0);
}

void game_mode_equipment_item_list_v7(const unsigned char *arg0)
{
    packet_trace_call("GameMode_EquipmentItemListV7");
    packet_trace_pointer(arg0);
}

void game_mode_failed_open_buying_store(const unsigned char *arg0)
{
    packet_trace_call("GameMode_FailedOpenBuyingStore");
    packet_trace_pointer(arg0);
}

void game_mode_failed_trade_buying_store_to_buyer(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_FailedTradeBuyingStoreToBuyer");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_failed_trade_buying_store_to_seller(const unsigned char *arg0)
{
    packet_trace_call("GameMode_FailedTradeBuyingStoreToSeller");
    packet_trace_pointer(arg0);
}

void game_mode_format_string_msg(const unsigned char *arg0)
{
    packet_trace_call("GameMode_FormatStringMsg");
    packet_trace_pointer(arg0);
}

void game_mode_friends_list(const unsigned char *arg0)
{
    packet_trace_call("GameMode_FriendsList");
    packet_trace_pointer(arg0);
}

void game_mode_friends_state(const unsigned char *arg0)
{
    packet_trace_call("GameMode_FriendsState");
    packet_trace_pointer(arg0);
}

void game_mode_get_user_config_list(struct GameMode *arg0, int arg1,
                                const struct UserConfigEntry *arg2)
{
    packet_trace_call("GameMode_GetUserConfigList");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_pointer(arg2);
}

void game_mode_group_list(const unsigned char *arg0)
{
    packet_trace_call("GameMode_GroupList");
    packet_trace_pointer(arg0);
}

void game_mode_inventory_item_list_normal_v5(const unsigned char *arg0)
{
    packet_trace_call("GameMode_InventoryItemListNormalV5");
    packet_trace_pointer(arg0);
}

void game_mode_inventory_tab(const unsigned char *arg0)
{
    packet_trace_call("GameMode_InventoryTab");
    packet_trace_pointer(arg0);
}

void game_mode_isvr_disconnect(struct GameMode *arg0)
{
    packet_trace_call("GameMode_IsvrDisconnect");
    packet_trace_pointer(arg0);
}

void game_mode_item_delete_buying_store(const unsigned char *arg0)
{
    packet_trace_call("GameMode_ItemDeleteBuyingStore");
    packet_trace_pointer(arg0);
}

void game_mode_item_entry(struct GameMode *arg0, const struct PacketGroundItemEntry *arg1)
{
    packet_trace_call("GameMode_ItemEntry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_item_fall_entry(struct GameMode *arg0, const struct PacketGroundItemFall *arg1)
{
    packet_trace_call("GameMode_ItemFallEntry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_item_fall_entry4(struct GameMode *arg0, const struct PacketGroundItemFall4 *arg1)
{
    packet_trace_call("GameMode_ItemFallEntry4");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_item_list_win_open(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_ItemListWinOpen");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_item_name_msg(const unsigned char *arg0)
{
    packet_trace_call("GameMode_ItemNameMsg");
    packet_trace_pointer(arg0);
}

void game_mode_item_pickup_ack(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_ItemPickupAck");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_item_pickup_ack2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_ItemPickupAck2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_item_pickup_ack3(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_ItemPickupAck3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_item_pickup_ack_v6(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_ItemPickupAckV6");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_item_pickup_ack_v7(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_ItemPickupAckV7");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_item_pickup_ack_v8(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_ItemPickupAckV8");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_item_pickup_party(const unsigned char *arg0)
{
    packet_trace_call("GameMode_ItemPickupParty");
    packet_trace_pointer(arg0);
}

void game_mode_map_property_r2(struct GameMode *arg0, const struct PacketMapPropertyR2 *arg1)
{
    packet_trace_call("GameMode_MapPropertyR2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_memorial_dungeon_info(const unsigned char *arg0)
{
    packet_trace_call("GameMode_MemorialDungeonInfo");
    packet_trace_pointer(arg0);
}

void game_mode_memorial_dungeon_notify(const unsigned char *arg0)
{
    packet_trace_call("GameMode_MemorialDungeonNotify");
    packet_trace_pointer(arg0);
}

void game_mode_memorial_dungeon_subscription_info(const unsigned char *arg0)
{
    packet_trace_call("GameMode_MemorialDungeonSubscriptionInfo");
    packet_trace_pointer(arg0);
}

void game_mode_memorial_dungeon_subscription_notify(const unsigned char *arg0)
{
    packet_trace_call("GameMode_MemorialDungeonSubscriptionNotify");
    packet_trace_pointer(arg0);
}

void game_mode_msg(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Msg");
    packet_trace_pointer(arg0);
}

void game_mode_zc_mvp_getting_special_exp(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Msg91");
    packet_trace_pointer(arg0);
}

void game_mode_msg_color(const unsigned char *arg0)
{
    packet_trace_call("GameMode_MsgColor");
    packet_trace_pointer(arg0);
}

void game_mode_msg_value(const unsigned char *arg0)
{
    packet_trace_call("GameMode_MsgValue");
    packet_trace_pointer(arg0);
}

void game_mode_my_buying_store_list(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_MyBuyingStoreList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_notify_act(struct GameMode *arg0, unsigned int arg1, unsigned int arg2, int arg3,
                        int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10)
{
    packet_trace_call("GameMode_NotifyAct");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
    packet_trace_value(arg5);
    packet_trace_value(arg6);
    packet_trace_value(arg7);
    packet_trace_value(arg8);
    packet_trace_value(arg9);
    packet_trace_value(arg10);
}

void game_mode_notify_ban(struct GameMode *arg0, const struct PACKET_SC_NOTIFY_BAN *arg1)
{
    packet_trace_call("GameMode_NotifyBan");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_notify_chat_party(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_NotifyChatParty");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_notify_map_property(struct GameMode *arg0, const struct PacketMapProperty *arg1)
{
    packet_trace_call("GameMode_NotifyMapProperty");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_notify_map_property2(struct GameMode *arg0, const struct PacketMapProperty *arg1)
{
    packet_trace_call("GameMode_NotifyMapProperty2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_ack_create_assistant(unsigned char arg0, unsigned char arg1)
{
    packet_trace_call("GameMode_OnAckCreateAssistant");
    packet_trace_value(arg0);
    packet_trace_value(arg1);
}

void game_mode_on_ack_private_airplane_call(struct GameMode *arg0, int arg1)
{
    packet_trace_call("GameMode_OnAckPrivateAirplaneCall");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}

void game_mode_on_ack_req_name_all2(struct GameMode *arg0, const struct PacketAckReqNameAll2 *arg1)
{
    packet_trace_call("GameMode_OnAckReqNameAll2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_action_failure(struct GameMode *arg0, const struct PacketActionFailure *arg1)
{
    packet_trace_call("GameMode_OnActionFailure");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_actor_guild(struct GameMode *arg0, int arg1, int arg2, int arg3)
{
    packet_trace_call("GameMode_OnActorGuild");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
}

void game_mode_on_add_exchange_item_grade(struct GameMode *arg0,
                                     const struct PacketAddExchangeItemGrade *arg1)
{
    packet_trace_call("GameMode_OnAddExchangeItemGrade");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_add_exchange_item_opt(struct GameMode *arg0,
                                   const struct PacketAddExchangeItemOpt *arg1)
{
    packet_trace_call("GameMode_OnAddExchangeItemOpt");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_add_member_to_group(struct GameMode *arg0, const struct PacketAddMemberToGroup *arg1)
{
    packet_trace_call("GameMode_OnAddMemberToGroup");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_add_member_to_group2(struct GameMode *arg0,
                                  const struct PacketAddMemberToGroup2 *arg1)
{
    packet_trace_call("GameMode_OnAddMemberToGroup2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_autospell_list2(int arg0, const int *arg1)
{
    packet_trace_call("GameMode_OnAutospellList2");
    packet_trace_value(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_buying_store_item_list(struct GameMode *arg0, int arg1, int arg2, char arg3, int arg4,
                                    int arg5, const struct BuyingStoreEntry *arg6, int arg7)
{
    packet_trace_call("GameMode_OnBuyingStoreItemList");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
    packet_trace_value(arg5);
    packet_trace_pointer(arg6);
    packet_trace_value(arg7);
}

void game_mode_on_change_agit_owner(struct GameMode *arg0, unsigned char arg1, int arg2, int arg3)
{
    packet_trace_call("GameMode_OnChangeAgitOwner");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
}

void game_mode_on_clear_effect(struct GameMode *arg0, const struct PacketClearEffect *arg1)
{
    packet_trace_call("GameMode_OnClearEffect");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_congratulation(struct GameMode *arg0, const struct PacketCongratulation *arg1)
{
    packet_trace_call("GameMode_OnCongratulation");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_ef_warning_plane(struct GameMode *arg0, int arg1, unsigned short arg2, int arg3,
                               short arg4, short arg5, int arg6)
{
    packet_trace_call("GameMode_OnEfWarningPlane");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
    packet_trace_value(arg5);
    packet_trace_value(arg6);
}

void game_mode_on_efst_set_enter(struct GameMode *arg0, const struct PacketEfstSetEnter *arg1)
{
    packet_trace_call("GameMode_OnEfstSetEnter");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_efst_set_enter2(struct GameMode *arg0, const struct PacketEfstSetEnter2 *arg1)
{
    packet_trace_call("GameMode_OnEfstSetEnter2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_emblem_download(int arg0, int arg1, int arg2, const char *arg3, int arg4)
{
    packet_trace_call("GameMode_OnEmblemDownload");
    packet_trace_value(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_pointer(arg3);
    packet_trace_value(arg4);
}

void game_mode_on_equipment_effect(struct GameMode *arg0, const struct PacketEquipmentEffect *arg1)
{
    packet_trace_call("GameMode_OnEquipmentEffect");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_group_list2(struct GameMode *arg0, const struct PacketGroupList2 *arg1)
{
    packet_trace_call("GameMode_OnGroupList2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_group_list3(struct GameMode *arg0, const struct PacketGroupList3 *arg1)
{
    packet_trace_call("GameMode_OnGroupList3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_guild_agit_info(struct GameMode *arg0, const struct PacketGuildAgitInfo *arg1)
{
    packet_trace_call("GameMode_OnGuildAgitInfo");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_guild_member(const struct PacketGuildMember *arg0)
{
    packet_trace_call("GameMode_OnGuildMember");
    packet_trace_pointer(arg0);
}

void game_mode_on_item_fall_entry5(struct GameMode *arg0, const struct PacketItemFallEntry5 *arg1)
{
    packet_trace_call("GameMode_OnItemFallEntry5");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_item_pickup_party2(struct GameMode *arg0, const struct PacketItemPickupParty2 *arg1)
{
    packet_trace_call("GameMode_OnItemPickupParty2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}



void game_mode_on_latest_rodex_list(struct GameMode *arg0, const struct PacketLatestRodexList *arg1)
{
    packet_trace_call("GameMode_OnLatestRodexList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_list_flag_clear(struct GameMode *arg0, const struct PacketListFlagClear *arg1)
{
    packet_trace_call("GameMode_OnListFlagClear");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_long_long_par_change(struct GameMode *arg0,
                                  const struct PacketLongLongParChange *arg1)
{
    packet_trace_call("GameMode_OnLongLongParChange");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_long_par_change(struct GameMode *arg0, const struct PacketLongParChange *arg1)
{
    packet_trace_call("GameMode_OnLongParChange");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_map_move_private_airplane(struct GameMode *arg0, const char *arg1,
                                       unsigned short arg2, unsigned short arg3)
{
    packet_trace_call("GameMode_OnMapMovePrivateAirplane");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
}

void game_mode_on_member_mgr_info(const struct PacketMemberMgrInfo *arg0)
{
    packet_trace_call("GameMode_OnMemberMgrInfo");
    packet_trace_pointer(arg0);
}

void game_mode_on_merge_item_open(struct GameMode *arg0, const struct PacketMergeItemOpen *arg1)
{
    packet_trace_call("GameMode_OnMergeItemOpen");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_millennium_shield(struct GameMode *arg0, const struct PacketMillenniumShield *arg1)
{
    packet_trace_call("GameMode_OnMillenniumShield");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_msg_state_change(struct GameMode *arg0, const struct PacketMsgStateChange *arg1)
{
    packet_trace_call("GameMode_OnMsgStateChange");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_msg_state_change2(struct GameMode *arg0, const struct PacketMsgStateChange2 *arg1)
{
    packet_trace_call("GameMode_OnMsgStateChange2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_msg_state_change3(struct GameMode *arg0, const struct PacketMsgStateChange3 *arg1)
{
    packet_trace_call("GameMode_OnMsgStateChange3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_msg_value_string(struct GameMode *arg0, unsigned short arg1, int arg2,
                               const char *arg3)
{
    packet_trace_call("GameMode_OnMsgValueString");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_pointer(arg3);
}

void game_mode_on_navigation_active(struct GameMode *arg0, const struct PacketNavigationActive *arg1)
{
    packet_trace_call("GameMode_OnNavigationActive");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_notify_effect(struct GameMode *arg0, const struct PacketNotifyEffect *arg1)
{
    packet_trace_call("GameMode_OnNotifyEffect");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_notify_effect2(struct GameMode *arg0, const struct PacketNotifyEffect2 *arg1)
{
    packet_trace_call("GameMode_OnNotifyEffect2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_notify_effect3(struct GameMode *arg0, const struct PacketNotifyEffect3 *arg1)
{
    packet_trace_call("GameMode_OnNotifyEffect3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_notify_effect4(struct GameMode *arg0, const struct PacketNotifyEffect4 *arg1)
{
    packet_trace_call("GameMode_OnNotifyEffect4");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_notify_exp(struct GameMode *arg0, const struct PacketNotifyExp *arg1)
{
    packet_trace_call("GameMode_OnNotifyExp");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_notify_exp2(struct GameMode *arg0, const struct PacketNotifyExp2 *arg1)
{
    packet_trace_call("GameMode_OnNotifyExp2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_notify_ground_skill(struct GameMode *arg0,
                                  const struct PacketNotifyGroundSkill *arg1)
{
    packet_trace_call("GameMode_OnNotifyGroundSkill");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_notify_member_info_to_groupm(struct GameMode *arg0,
                                         const struct PacketNotifyMemberInfoToGroupm *arg1)
{
    packet_trace_call("GameMode_OnNotifyMemberInfoToGroupm");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_notify_skill(struct GameMode *arg0, const struct PacketNotifySkill *arg1)
{
    packet_trace_call("GameMode_OnNotifySkill");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_notify_skill2(struct GameMode *arg0, const struct PacketNotifySkill2 *arg1)
{
    packet_trace_call("GameMode_OnNotifySkill2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_notify_skill_position(struct GameMode *arg0,
                                    const struct PacketNotifySkillPosition *arg1)
{
    packet_trace_call("GameMode_OnNotifySkillPosition");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_notify_unread_rodex(struct GameMode *arg0,
                                  const struct PacketNotifyUnreadRodex *arg1)
{
    packet_trace_call("GameMode_OnNotifyUnreadRodex");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_npc_market_open(struct GameMode *arg0, int arg1, const struct NpcMarketEntry *arg2)
{
    packet_trace_call("GameMode_OnNpcMarketOpen");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_pointer(arg2);
}

void game_mode_on_npc_market_purchase_result(struct GameMode *arg0, unsigned char arg1, int arg2,
                                        const struct MarketBoughtItem *arg3)
{
    packet_trace_call("GameMode_OnNpcMarketPurchaseResult");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_pointer(arg3);
}

void game_mode_on_npc_show_efst_update(struct GameMode *arg0,
                                  const struct PacketNpcShowEfstUpdate *arg1)
{
    packet_trace_call("GameMode_OnNpcShowEfstUpdate");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}



void game_mode_on_open_ui_assistant_buying(struct GameMode *arg0, unsigned char arg1)
{
    packet_trace_call("GameMode_OnOpenUiAssistantBuying");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}

void game_mode_on_open_ui_assistant_vending(struct GameMode *arg0, unsigned char arg1, int arg2,
                                       const void *arg3)
{
    packet_trace_call("GameMode_OnOpenUiAssistantVending");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_pointer(arg3);
}

void game_mode_on_party_member_state(struct GameMode *arg0, const struct PacketPartyMemberState *arg1)
{
    packet_trace_call("GameMode_OnPartyMemberState");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_personal_info(struct GameMode *arg0, const struct PacketPersonalInfo *arg1)
{
    packet_trace_call("GameMode_OnPersonalInfo");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_pr_npc_state(struct GameMode *arg0, const struct PacketPrNpcState *arg1)
{
    packet_trace_call("GameMode_OnPrNpcState");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_purchase_item_list_from_mc4(struct GameMode *arg0,
                                        const struct PacketPurchaseItemListFromMc4 *arg1)
{
    packet_trace_call("GameMode_OnPurchaseItemListFromMc4");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_purchase_my_item_list3(struct GameMode *arg0,
                                    const struct PacketPurchaseMyItemList3 *arg1)
{
    packet_trace_call("GameMode_OnPurchaseMyItemList3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_recover_penalty_overweight(struct GameMode *arg0,
                                         const struct PacketRecoverPenaltyOverweight *arg1)
{
    packet_trace_call("GameMode_OnRecoverPenaltyOverweight");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_recovery(struct GameMode *arg0, const void *arg1, int arg2)
{
    packet_trace_call("GameMode_OnRecovery");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
}

void game_mode_on_refining_material_list(struct GameMode *arg0, short arg1, unsigned char arg2,
                                     const void *arg3, unsigned char arg4)
{
    packet_trace_call("GameMode_OnRefiningMaterialList");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_pointer(arg3);
    packet_trace_value(arg4);
}

void game_mode_on_remove_neighborhood_characters(struct GameMode *arg0, int arg1, const int *arg2)
{
    packet_trace_call("GameMode_OnRemoveNeighborhoodCharacters");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_pointer(arg2);
}

void game_mode_on_repair_item_list2(struct GameMode *arg0, const struct PacketRepairItemList2 *arg1)
{
    packet_trace_call("GameMode_OnRepairItemList2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}



void game_mode_on_req_ack_agit_investment(struct GameMode *arg0, unsigned char arg1, int arg2, int arg3)
{
    packet_trace_call("GameMode_OnReqAckAgitInvestment");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
}

void game_mode_on_req_ack_move_guild_agit(struct GameMode *arg0, short arg1)
{
    packet_trace_call("GameMode_OnReqAckMoveGuildAgit");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}



void game_mode_on_server_move_private_airplane(struct GameMode *arg0, const char *arg1, short arg2,
                                          short arg3, int arg4, short arg5)
{
    packet_trace_call("GameMode_OnServerMovePrivateAirplane");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
    packet_trace_value(arg5);
}

void game_mode_on_set_clairvoyance(struct GameMode *arg0, char arg1)
{
    packet_trace_call("GameMode_OnSetClairvoyance");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}

void game_mode_on_show_script(struct GameMode *arg0, const struct PacketShowScript *arg1)
{
    packet_trace_call("GameMode_OnShowScript");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_show_warp_list(struct GameMode *arg0, struct PacketShowWarpList *arg1)
{
    packet_trace_call("GameMode_OnShowWarpList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_soul_energy(struct GameMode *arg0, const struct PacketSoulEnergy *arg1)
{
    packet_trace_call("GameMode_OnSoulEnergy");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_spirits(struct GameMode *arg0, int arg1, short arg2)
{
    packet_trace_call("GameMode_OnSpirits");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
}

void game_mode_on_spirits2(struct GameMode *arg0, const struct PacketSpirits2 *arg1)
{
    packet_trace_call("GameMode_OnSpirits2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_spirits_attribute(struct GameMode *arg0, const struct PacketSpiritsAttribute *arg1)
{
    packet_trace_call("GameMode_OnSpiritsAttribute");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_start_baby(struct GameMode *arg0, const void *arg1)
{
    packet_trace_call("GameMode_OnStartBaby");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_start_capture(struct GameMode *arg0, const void *arg1)
{
    packet_trace_call("GameMode_OnStartCapture");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_start_collection(struct GameMode *arg0, const void *arg1)
{
    packet_trace_call("GameMode_OnStartCollection");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_start_couple(struct GameMode *arg0, const void *arg1)
{
    packet_trace_call("GameMode_OnStartCouple");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_store_assistant_disappear(struct GameMode *arg0, int arg1)
{
    packet_trace_call("GameMode_OnStoreAssistantDisappear");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}

void game_mode_on_sync_attacked_pos(struct GameMode *arg0, const void *arg1)
{
    packet_trace_call("GameMode_OnSyncAttackedPos");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_target_spirits(struct GameMode *arg0, int arg1, short arg2)
{
    packet_trace_call("GameMode_OnTargetSpirits");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
}

void game_mode_on_try_capture_monster(struct GameMode *arg0, const struct PacketTryResult *arg1)
{
    packet_trace_call("GameMode_OnTryCaptureMonster");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_try_collection(struct GameMode *arg0, const struct PacketTryResult *arg1)
{
    packet_trace_call("GameMode_OnTryCollection");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}





void game_mode_on_use_skill_ack(struct GameMode *arg0, const struct PacketUseSkillAck *arg1, int arg2)
{
    packet_trace_call("GameMode_OnUseSkillAck");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
}

void game_mode_on_use_skill_ack3(struct GameMode *arg0, const struct PacketUseSkillAck *arg1)
{
    packet_trace_call("GameMode_OnUseSkillAck3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_vending_store_item_list(struct GameMode *arg0, int arg1, int arg2, char arg3,
                                     int arg4, int arg5, const struct VendingEntry *arg6)
{
    packet_trace_call("GameMode_OnVendingStoreItemList");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
    packet_trace_value(arg5);
    packet_trace_pointer(arg6);
}

void game_mode_on_vending_store_item_list4(struct GameMode *arg0, const struct PacketVendingList4 *arg1)
{
    packet_trace_call("GameMode_OnVendingStoreItemList4");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_warp_list(struct GameMode *arg0, const struct PacketWarpList *arg1)
{
    packet_trace_call("GameMode_OnWarpList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_on_warp_list2(struct GameMode *arg0, struct PacketWarpList2 *arg1)
{
    packet_trace_call("GameMode_OnWarpList2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_party_join_ack(const unsigned char *arg0)
{
    packet_trace_call("GameMode_PartyJoinAck");
    packet_trace_pointer(arg0);
}

void game_mode_select_cart(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_SelectCart");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_set_search_store_pos(const struct PACKET_ZC_SEARCH_STORE_POS *arg0)
{
    packet_trace_call("GameMode_SetSearchStorePos");
    packet_trace_pointer(arg0);
}

void game_mode_set_user_config_ack(struct GameMode *arg0, int arg1, int arg2, int arg3)
{
    packet_trace_call("GameMode_SetUserConfigAck");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
}

void game_mode_shop_item_sold_packet(const unsigned char *arg0)
{
    packet_trace_call("GameMode_ShopItemSoldPacket");
    packet_trace_pointer(arg0);
}

void game_mode_shop_not_movable_items(const unsigned char *arg0)
{
    packet_trace_call("GameMode_ShopNotMovableItems");
    packet_trace_pointer(arg0);
}

void game_mode_shop_open_result(struct GameMode *arg0, unsigned char arg1)
{
    packet_trace_call("GameMode_ShopOpenResult");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}

void game_mode_show_skill_use(struct GameMode *arg0, unsigned short arg1, int arg2, int arg3,
                           int arg4, char arg5)
{
    packet_trace_call("GameMode_ShowSkillUse");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
    packet_trace_value(arg5);
}

void game_mode_skill_update(struct GameMode *arg0, unsigned short arg1, short arg2, short arg3,
                          unsigned char arg4, short arg5, int arg6, unsigned short arg7)
{
    packet_trace_call("GameMode_SkillUpdate");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
    packet_trace_value(arg5);
    packet_trace_value(arg6);
    packet_trace_value(arg7);
}

void game_mode_zc_store_item_list_equip_v2(struct GameMode *arg0, const struct PACKET_STORE_LIST_NAMED *arg1)
{
    packet_trace_call("GameMode_StoreEquipList_976");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_store_item_list_equip_v5(struct GameMode *arg0, const struct PACKET_STORE_LIST_NAMED *arg1)
{
    packet_trace_call("GameMode_StoreEquipList_996");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_store_item_list_equip_v6(struct GameMode *arg0, const struct PACKET_STORE_LIST_NAMED *arg1)
{
    packet_trace_call("GameMode_StoreEquipList_a10");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_store_item_list_equip_v5(const unsigned char *arg0)
{
    packet_trace_call("GameMode_StoreItemListEquipV5");
    packet_trace_pointer(arg0);
}

void game_mode_store_item_list_equip_v6(const unsigned char *arg0)
{
    packet_trace_call("GameMode_StoreItemListEquipV6");
    packet_trace_pointer(arg0);
}

void game_mode_store_item_list_normal_v5(const unsigned char *arg0)
{
    packet_trace_call("GameMode_StoreItemListNormalV5");
    packet_trace_pointer(arg0);
}

void game_mode_zc_store_item_list_normal_v5(struct GameMode *arg0,
                                  const struct PACKET_STORE_LIST_NAMED *arg1)
{
    packet_trace_call("GameMode_StoreNormalList_995");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_ui_open(unsigned char arg0)
{
    packet_trace_call("GameMode_UiOpen");
    packet_trace_value(arg0);
}

void game_mode_ui_open_v2(struct GameMode *arg0, unsigned char arg1, int arg2)
{
    packet_trace_call("GameMode_UiOpenV2");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
}

void game_mode_zc_config_notify(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_ZcConfigNotify");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ach_update(struct GameMode *arg0, int arg1, unsigned int arg2, int arg3, int arg4,
                            int arg5, unsigned int arg6, const int *arg7, int arg8,
                            unsigned int arg9)
{
    packet_trace_call("GameMode_Zc_Ach_Update");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
    packet_trace_value(arg5);
    packet_trace_value(arg6);
    packet_trace_pointer(arg7);
    packet_trace_value(arg8);
    packet_trace_value(arg9);
}

void game_mode_zc_ack_add_item_to_cart(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_AddItem_To_Cart");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_add_item_rodex(struct GameMode *arg0,
                                    const struct PACKET_ZC_ACK_ADD_ITEM_RODEX *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Add_Item_Rodex");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_add_item_rodex2(struct GameMode *arg0,
                                     const struct PACKET_ZC_ACK_ADD_ITEM_RODEX2 *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Add_Item_Rodex2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_all_rodex_list(struct GameMode *arg0,
                                    const struct PACKET_ZC_ACK_ALL_RODEX_LIST *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_All_Rodex_List");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_ban_guild_del_name(struct GameMode *arg0,
                                       const struct PacketGuildBanByGid *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Ban_Guild_DelName");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_ban_guild_sso(struct GameMode *arg0, const struct PacketGuildLeave *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Ban_Guild_Sso");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_change_guild_position_info(const struct PacketPositionInfo *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Change_Guild_PositionInfo");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_change_title(struct GameMode *arg0, unsigned int arg1, int arg2)
{
    packet_trace_call("GameMode_Zc_Ack_Change_Title");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
}

void game_mode_zc_ack_create_chatroom(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Create_Chatroom");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_delete_rodex(struct GameMode *arg0,
                                  const struct PACKET_ZC_ACK_DELETE_RODEX *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Delete_Rodex");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_disconnect_character(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Disconnect_Character");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_disorganize_guild(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Disorganize_Guild");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_disorganize_guild_result(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Disorganize_Guild_Result");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_extend_body_item_size(struct GameMode *arg0,
                                          const struct PACKET_ZC_ACK_EXTEND_BODYITEM_SIZE *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Extend_BodyItem_Size");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_failed_all_rodex_list(struct GameMode *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Failed_All_Rodex_List");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_give_manner_point(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Give_Manner_Point");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_guild_menu_interface(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Guild_MenuInterface");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_item_composition(struct GameMode *arg0,
                                     const struct PacketItemComposition *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_ItemComposition");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_item_identify(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_ItemIdentify");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_item_refining(struct GameMode *arg0, const struct PacketItemRefining *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_ItemRefining");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_item_repair(struct GameMode *arg0, const struct PacketItemRepair *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_ItemRepair");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_item_from_rodex(struct GameMode *arg0,
                                     const struct PACKET_ZC_ACK_ZENY_FROM_RODEX *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Item_From_Rodex");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_leave_guild(struct GameMode *arg0, const struct PacketGuildLeave *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Leave_Guild");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_leave_guild_del_name(struct GameMode *arg0,
                                         const struct PacketGuildLeaveByGid *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Leave_Guild_DelName");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_make_group(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Make_Group");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_merge_item(struct GameMode *arg0, const struct PACKET_ZC_ACK_MERGE_ITEM *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Merge_Item");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_open_msg_box_extend_body_item_size(
    struct GameMode *arg0, const struct PACKET_ZC_ACK_OPEN_MSGBOX_EXTEND_BODYITEM_SIZE *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Open_MsgBox_Extend_BodyItem_Size");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_open_write_rodex(struct GameMode *arg0,
                                      const struct PACKET_ZC_ACK_OPEN_WRITE_RODEX *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Open_Write_Rodex");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_read_rodex2(struct GameMode *arg0,
                                 const struct PACKET_ZC_ACK_READ_RODEX2 *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Read_Rodex2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_remember_warppoint(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Remember_Warppoint");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_remove_rodex_item(struct GameMode *arg0,
                                       const struct PACKET_ZC_ACK_REMOVE_RODEX_ITEM *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Remove_Rodex_Item");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_req_making_item(struct GameMode *arg0, const struct PacketMakingItem *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_ReqMakingItem");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_req_ally_guild(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Req_Ally_Guild");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_req_change_members(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Req_Change_Members");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_req_hostile_guild(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Req_Hostile_Guild");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_req_join_guild(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ack_Req_Join_Guild");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ack_rodex_list(struct GameMode *arg0, const struct PACKET_ZC_ACK_RODEX_LIST *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Rodex_List");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_send_rodex(struct GameMode *arg0, const struct PACKET_ZC_ACK_SEND_RODEX *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Send_Rodex");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_take_off_equip_all(struct GameMode *arg0, unsigned int arg1)
{
    packet_trace_call("GameMode_Zc_Ack_TakeOff_Equip_All");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}

void game_mode_zc_ack_weapon_refine(struct GameMode *arg0, const struct PacketWeaponRefine *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_WeaponRefine");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ack_wear_equip_v5(struct GameMode *arg0, unsigned char arg1, unsigned short arg2,
                                   unsigned short arg3, int arg4)
{
    packet_trace_call("GameMode_Zc_Ack_Wear_Equip_V5");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
}

void game_mode_zc_ack_zeny_from_rodex(struct GameMode *arg0,
                                     const struct PACKET_ZC_ACK_ZENY_FROM_RODEX *arg1)
{
    packet_trace_call("GameMode_Zc_Ack_Zeny_From_Rodex");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_active_quest(const void *arg0, size_t arg1)
{
    packet_trace_call("GameMode_Zc_Active_Quest");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}

void game_mode_zc_add_item_to_cart(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Add_Item_To_Cart");
    packet_trace_pointer(arg0);
}

void game_mode_zc_add_item_to_cart3(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Add_Item_To_Cart3");
    packet_trace_pointer(arg0);
}

void game_mode_zc_add_quest_ex(struct GameMode *arg0, const void *arg1, size_t arg2)
{
    packet_trace_call("GameMode_Zc_Add_Quest_Ex");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
}

void game_mode_zc_add_quest_ex2(struct GameMode *arg0, const void *arg1, size_t arg2)
{
    packet_trace_call("GameMode_Zc_Add_Quest_Ex2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
}

void game_mode_zc_add_related_guild(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Add_Related_Guild");
    packet_trace_pointer(arg0);
}

void game_mode_zc_all_ach_list(struct GameMode *arg0, int arg1, int arg2, unsigned int arg3,
                              int arg4, int arg5, const struct AchievementEntry *arg6)
{
    packet_trace_call("GameMode_Zc_All_Ach_List");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
    packet_trace_value(arg5);
    packet_trace_pointer(arg6);
}

void game_mode_zc_attack_failure_for_distance(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Attack_Failure_For_Distance");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_attack_range(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Attack_Range");
    packet_trace_pointer(arg0);
}

void game_mode_zc_auto_spell_list(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_AutoSpellList");
    packet_trace_pointer(arg0);
}

void game_mode_zc_autorun_skill(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Autorun_Skill");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_baby_msg(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_BabyMsg");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ban_list(struct GameMode *arg0, const struct PacketBanList *arg1)
{
    packet_trace_call("GameMode_Zc_Ban_List");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ban_list_del_name(struct GameMode *arg0, const struct PacketBanList *arg1)
{
    packet_trace_call("GameMode_Zc_Ban_List_DelName");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ban_list_new(struct GameMode *arg0, const struct PacketBanList *arg1)
{
    packet_trace_call("GameMode_Zc_Ban_List_New");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_battlefield_chat(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Battlefield_Chat");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_battlefield_notify_camp_info(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Battlefield_Notify_CampInfo");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_battlefield_notify_hp(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Battlefield_Notify_Hp");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_battlefield_notify_hp2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Battlefield_Notify_Hp2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

int game_mode_zc_battlefield_notify_hp_sp(struct GameMode *arg0, int arg1, int arg2, int arg3,
                                        int arg4, int arg5)
{
    packet_trace_call("GameMode_Zc_Battlefield_Notify_HpSp");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
    packet_trace_value(arg5);
    return 0;
}

void game_mode_zc_battlefield_notify_point(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Battlefield_Notify_Point");
    packet_trace_pointer(arg0);
}

void game_mode_zc_battlefield_notify_position(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Battlefield_Notify_Position");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_blade_stop(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_BladeStop");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_boss_info(struct GameMode *arg0, const struct PACKET_ZC_BOSS_INFO *arg1)
{
    packet_trace_call("GameMode_Zc_Boss_Info");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_broadcast_item_refining_result(struct GameMode *arg0, const char *arg1, int arg2,
                                               unsigned int arg3, unsigned int arg4)
{
    packet_trace_call("GameMode_Zc_Broadcast_ItemRefining_Result");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
}

void game_mode_zc_c_marker_info(const struct PACKET_ZC_C_MARKERINFO *arg0)
{
    packet_trace_call("GameMode_Zc_C_MarkerInfo");
    packet_trace_pointer(arg0);
}

void game_mode_zc_cancel_exchange_item(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Cancel_Exchange_Item");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_cart_off(void)
{
    packet_trace_call("GameMode_Zc_CartOff");
}

void game_mode_zc_cart_equipment_item_list(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Cart_Equipment_ItemList");
    packet_trace_pointer(arg0);
}

void game_mode_zc_cart_item_list_equip(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Cart_ItemList_Equip");
    packet_trace_pointer(arg0);
}

void game_mode_zc_cart_item_list_normal(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Cart_ItemList_Normal");
    packet_trace_pointer(arg0);
}

void game_mode_zc_cart_normal_item_list(const unsigned char *arg0, int arg1)
{
    packet_trace_call("GameMode_Zc_Cart_Normal_ItemList");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}

void game_mode_zc_change_state_homun(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_ChangeState_Homun");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_change_state_pet(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_ChangeState_Pet");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_change_chatroom(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Change_Chatroom");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_change_direction(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Change_Direction");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_change_guild(struct GameMode *arg0, const struct PACKET_ZC_CHANGE_GUILD *arg1)
{
    packet_trace_call("GameMode_Zc_Change_Guild");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_change_item_option2_packet(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Change_Item_Option2_Packet");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_change_item_option3(struct GameMode *arg0,
                                     const struct PACKET_ZC_CHANGE_ITEM_OPTION3 *arg1)
{
    packet_trace_call("GameMode_Zc_Change_Item_Option3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_change_item_option_packet(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Change_Item_Option_Packet");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_check_receive_character_name2(struct GameMode *arg0, int arg1,
                                               unsigned short arg2, unsigned short arg3,
                                               const char *arg4)
{
    packet_trace_call("GameMode_Zc_Check_Receive_Character_Name2");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_pointer(arg4);
}

void game_mode_zc_clear_dialog(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Clear_Dialog");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_close_dialog(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Close_Dialog");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_close_store(void)
{
    packet_trace_call("GameMode_Zc_Close_Store");
}

void game_mode_zc_combo_delay(struct GameMode *arg0, const struct PACKET_ZC_COMBODELAY *arg1)
{
    packet_trace_call("GameMode_Zc_ComboDelay");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_compass(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Compass");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_conclude_exchange_item(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Conclude_Exchange_Item");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_config(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Config");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_couple_name(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_CoupleName");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_couple_status(struct GameMode *arg0, const struct PACKET_ZC_COUPLESTATUS *arg1)
{
    packet_trace_call("GameMode_Zc_CoupleStatus");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_debug_msg(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_DebugMsg");
    packet_trace_pointer(arg0);
}

void game_mode_zc_delete_item_from_mc_store(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_DeleteItem_From_McStore");
    packet_trace_pointer(arg0);
}

void game_mode_zc_delete_friends(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Delete_Friends");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_delete_item_from_cart(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Delete_Item_From_Cart");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_delete_item_from_store(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Delete_Item_From_Store");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_delete_related_guild(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Delete_Related_Guild");
    packet_trace_pointer(arg0);
}

void game_mode_zc_destroy_room(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Destroy_Room");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_devotion_list(struct GameMode *arg0, const struct PacketDevotionList *arg1)
{
    packet_trace_call("GameMode_Zc_DevotionList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_dialog_text_align(struct GameMode *arg0, unsigned char arg1)
{
    packet_trace_call("GameMode_Zc_Dialog_Text_Align");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}

void game_mode_zc_disappear_entry(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Disappear_Entry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_dispel(struct GameMode *arg0, const struct PacketDispel *arg1)
{
    packet_trace_call("GameMode_Zc_Dispel");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_divorce(struct GameMode *arg0, const struct PACKET_ZC_DIVORCE *arg1)
{
    packet_trace_call("GameMode_Zc_Divorce");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_dressroom_open(void)
{
    packet_trace_call("GameMode_Zc_Dressroom_Open");
}

void game_mode_zc_el_init(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_El_Init");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_el_par_change(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_El_Par_Change");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_emotion(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Emotion");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_enter_room(struct GameMode *arg0, const struct PACKET_ZC_ENTER_ROOM *arg1)
{
    packet_trace_call("GameMode_Zc_Enter_Room");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_equip_item_damaged(struct GameMode *arg0,
                                   const struct PACKET_ZC_EQUIPITEM_DAMAGED *arg1)
{
    packet_trace_call("GameMode_Zc_EquipItem_Damaged");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_equip_arrow(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Equip_Arrow");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_equipment_item_list(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Equipment_ItemList");
    packet_trace_pointer(arg0);
}

void game_mode_zc_exchange_item_undo(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_ExchangeItem_Undo");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_exec_exchange_item(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Exec_Exchange_Item");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_extend_body_item_size(struct GameMode *arg0,
                                      const struct PACKET_ZC_EXTEND_BODYITEM_SIZE *arg1)
{
    packet_trace_call("GameMode_Zc_Extend_BodyItem_Size");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_failed_get_item_from_zone_da(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Failed_Get_Item_From_ZoneDA");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_fast_move(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_FastMove");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_feed(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Feed");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_group_info_change(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_GroupInfo_Change");
    packet_trace_pointer(arg0);
}

void game_mode_zc_guild_chat(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Guild_Chat");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_guild_emblem_img(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Guild_Emblem_Img");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_guild_info(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Guild_Info");
    packet_trace_pointer(arg0);
}

void game_mode_zc_guild_info2(const struct PACKET_ZC_GUILD_INFO2 *arg0)
{
    packet_trace_call("GameMode_Zc_Guild_Info2");
    packet_trace_pointer(arg0);
}

void game_mode_zc_guild_info_del_name(struct GameMode *arg0, const struct PacketGuildInfo *arg1)
{
    packet_trace_call("GameMode_Zc_Guild_Info_DelName");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_guild_info_new(struct GameMode *arg0, const struct PacketGuildInfoNew *arg1)
{
    packet_trace_call("GameMode_Zc_Guild_Info_New");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_guild_member_map_change(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Guild_Member_Map_Change");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_guild_notice(struct GameMode *arg0, struct PacketGuildNotice *arg1)
{
    packet_trace_call("GameMode_Zc_Guild_Notice");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_guild_position(struct GameMode *arg0, const struct PACKET_ZC_GUILD_POSITION *arg1)
{
    packet_trace_call("GameMode_Zc_Guild_Position");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_guild_skill_info(struct GameMode *arg0,
                                 const struct PACKET_ZC_GUILD_SKILLINFO *arg1)
{
    packet_trace_call("GameMode_Zc_Guild_SkillInfo");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_high_jump(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_HighJump");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_ho_par_change(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ho_Par_Change");
    packet_trace_pointer(arg0);
}

void game_mode_zc_ho_par_change2(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Ho_Par_Change2");
    packet_trace_pointer(arg0);
}

void game_mode_zc_hp_info(struct GameMode *arg0, const struct PACKET_ZC_HP_INFO *arg1)
{
    packet_trace_call("GameMode_Zc_Hp_Info");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_hp_info_tiny(struct GameMode *arg0, const struct PACKET_ZC_HP_INFO_TINY *arg1)
{
    packet_trace_call("GameMode_Zc_Hp_Info_Tiny");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_hunting_list(struct GameMode *arg0, const struct PACKET_ZC_HUNTINGLIST *arg1)
{
    packet_trace_call("GameMode_Zc_HuntingList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_hunting_quest_info(struct GameMode *arg0, const void *arg1, size_t arg2)
{
    packet_trace_call("GameMode_Zc_Hunting_Quest_Info");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
}

void game_mode_zc_info_remain_time(struct GameMode *arg0,
                                 const struct PACKET_ZC_INFO_REMAINTIME *arg1)
{
    packet_trace_call("GameMode_Zc_Info_RemainTime");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_inventory_item_list_normal(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Inventory_ItemList_Normal");
    packet_trace_pointer(arg0);
}

void game_mode_zc_item_disappear(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Item_Disappear");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_item_pickup_ack_v5(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Item_Pickup_Ack_V5");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_item_throw_ack(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Item_Throw_Ack");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_makable_item_list(struct GameMode *arg0, const struct PacketMakableItemList *arg1)
{
    packet_trace_call("GameMode_Zc_MakableItemList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_making_arrow_list(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_MakingArrow_List");
    packet_trace_pointer(arg0);
}

void game_mode_zc_making_item_list(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_MakingItem_List");
    packet_trace_pointer(arg0);
}

void game_mode_zc_member_mgr_info(struct GameMode *arg0, const struct PacketMemberList *arg1)
{
    packet_trace_call("GameMode_Zc_MemberMgr_Info");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_member_mgr_info_del_name(struct GameMode *arg0, const struct PacketMemberList *arg1)
{
    packet_trace_call("GameMode_Zc_MemberMgr_Info_DelName");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_member_mgr_info_new(struct GameMode *arg0, const struct PacketMemberList *arg1)
{
    packet_trace_call("GameMode_Zc_MemberMgr_Info_New");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_member_add(struct GameMode *arg0, const struct PacketMemberFull *arg1)
{
    packet_trace_call("GameMode_Zc_Member_Add");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_member_add_del_name(struct GameMode *arg0, const struct PacketMemberMemo *arg1)
{
    packet_trace_call("GameMode_Zc_Member_Add_DelName");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_member_add_new(struct GameMode *arg0, const struct PacketMemberNamed *arg1)
{
    packet_trace_call("GameMode_Zc_Member_Add_New");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_member_exit(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Member_Exit");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_member_newentry(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Member_Newentry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_menu_list(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Menu_List");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_mer_init(struct GameMode *arg0, const struct PacketMerInit *arg1)
{
    packet_trace_call("GameMode_Zc_Mer_Init");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_mer_par_change(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Mer_Par_Change");
    packet_trace_pointer(arg0);
}

void game_mode_zc_monster_info(struct GameMode *arg0, const struct PACKET_ZC_MONSTER_INFO *arg1)
{
    packet_trace_call("GameMode_Zc_Monster_Info");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_msg_skill(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Msg_Skill");
    packet_trace_pointer(arg0);
}

void game_mode_zc_mvp(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Mvp");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_my_guild_basic_info(struct GameMode *arg0, const struct PacketGuildRelations *arg1)
{
    packet_trace_call("GameMode_Zc_MyGuild_Basic_Info");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_normal_item_list(const unsigned char *arg0, int arg1)
{
    packet_trace_call("GameMode_Zc_Normal_ItemList");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}

void game_mode_zc_normal_system_msg(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Normal_System_Msg");
    packet_trace_pointer(arg0);
}

void game_mode_zc_notify_act_position(struct GameMode *arg0, const struct PACKET_ZC_NOTIFY_ACT_POSITION *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Act_Position");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_actor_init(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_ActorInit");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_bind_on_equip(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Notify_Bind_On_Equip");
    packet_trace_pointer(arg0);
}

void game_mode_zc_notify_cart_item_count_info(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Notify_CartItem_CountInfo");
    packet_trace_pointer(arg0);
}

void game_mode_zc_notify_chat(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Chat");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_font(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Font");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_hp_sp_to_group_m_r2(struct GameMode *arg0, int arg1, int arg2, int arg3,
                                          int arg4, int arg5)
{
    packet_trace_call("GameMode_Zc_Notify_HpSp_To_GroupM_R2");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
    packet_trace_value(arg5);
}

void game_mode_zc_notify_hp_to_group_m(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Hp_To_GroupM");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_hp_to_group_m_r2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Hp_To_GroupM_R2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_manner_point_given(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Notify_Manner_Point_Given");
    packet_trace_pointer(arg0);
}

void game_mode_zc_notify_map_info(struct GameMode *arg0, const struct PACKET_ZC_NOTIFY_MAPINFO *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_MapInfo");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_move(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Move");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_moveentry(struct GameMode *arg0, const struct PacketMoveEntry *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Moveentry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_moveentry10(struct GameMode *arg0, const struct PacketMoveEntry10 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Moveentry10");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_moveentry11(struct GameMode *arg0, const struct PacketMoveEntry11 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Moveentry11");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_moveentry2(struct GameMode *arg0, const struct PacketMoveEntry2 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Moveentry2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_moveentry3(struct GameMode *arg0, const struct PacketMoveEntry3 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Moveentry3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_moveentry6(struct GameMode *arg0, const struct PacketMoveEntry6 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Moveentry6");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_moveentry9(struct GameMode *arg0, const struct PacketMoveEntry9 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Moveentry9");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_newentry(struct GameMode *arg0, const struct PacketNewEntry *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Newentry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_newentry10(struct GameMode *arg0, const struct PacketNewEntry10 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Newentry10");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_newentry11(struct GameMode *arg0, const struct PacketNewEntry11 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Newentry11");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_newentry2(struct GameMode *arg0, const struct PacketNewEntry2 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Newentry2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_newentry3(struct GameMode *arg0, const struct PacketNewEntry4 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Newentry3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_newentry4(struct GameMode *arg0, const struct PacketNewEntry4 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Newentry4");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_newentry5(struct GameMode *arg0, const struct PacketNewEntry5 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Newentry5");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_newentry7(struct GameMode *arg0, const struct PacketNewEntry7 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Newentry7");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_player_chat(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_PlayerChat");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_player_move(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_PlayerMove");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_position_to_group_m(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Position_To_GroupM");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_position_to_guild_m(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Position_To_GuildM");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_ranking(struct GameMode *arg0, const struct PacketNotifyRanking *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Ranking");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_standentry(struct GameMode *arg0, const struct PacketStandEntry *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Standentry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_standentry10(struct GameMode *arg0, const struct PacketStandEntry10 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Standentry10");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_standentry11(struct GameMode *arg0, const struct PacketStandEntry11 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Standentry11");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_standentry2(struct GameMode *arg0, const struct PacketStandEntry2 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Standentry2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_standentry3(struct GameMode *arg0, const struct PacketStandEntry4 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Standentry3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_standentry4(struct GameMode *arg0, const struct PacketStandEntry4 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Standentry4");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_standentry5(struct GameMode *arg0, const struct PacketStandEntry5 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Standentry5");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_standentry8(struct GameMode *arg0, const struct PacketStandEntry8 *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Standentry8");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_standentry_npc(struct GameMode *arg0,
                                       const struct PacketStandEntryNpc *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Standentry_Npc");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_store_item_count_info(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Notify_StoreItem_CountInfo");
    packet_trace_pointer(arg0);
}

void game_mode_zc_notify_time(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Time");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_vanish(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_Vanish");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_notify_weapon_item_list(struct GameMode *arg0,
                                       const struct PacketWeaponItemList *arg1)
{
    packet_trace_call("GameMode_Zc_Notify_WeaponItemList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_npc_sprite_change(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_NpcSprite_Change");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_npc_barter_market_item_info(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Npc_Barter_Market_ItemInfo");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_npc_barter_market_item_info2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Npc_Barter_Market_ItemInfo2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_npc_chat(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Npc_Chat");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_npc_expanded_barter_market_item_info(struct GameMode *arg0,
                                                     const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Npc_Expanded_Barter_Market_ItemInfo");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_npc_expanded_barter_market_item_info2(struct GameMode *arg0,
                                                      const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Npc_Expanded_Barter_Market_ItemInfo2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_npc_market_open(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Npc_Market_Open");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_npc_market_open2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Npc_Market_Open2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_npc_market_purchase_result_v2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Npc_Market_Purchase_Result_V2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_npcack_mapmove(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Npcack_Mapmove");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_npcack_servermove(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Npcack_Servermove");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_npcack_servermove_domain(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Npcack_Servermove_Domain");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_open_store(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_OpenStore");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_open_buying_store(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Open_Buying_Store");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_open_edit_dlg(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Open_EditDlg");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_open_edit_dlg_str(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Open_EditDlgStr");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_open_search_store_info(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Open_Search_Store_Info");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_par_change(struct GameMode *arg0, struct PACKET_ZC_PAR_CHANGE *arg1)
{
    packet_trace_call("GameMode_Zc_Par_Change");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_par_change_user(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Par_Change_User");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_party_config(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Party_Config");
    packet_trace_pointer(arg0);
}

void game_mode_zc_party_join_req_ack_from_master(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Party_Join_Req_Ack_From_Master");
    packet_trace_pointer(arg0);
}

void game_mode_zc_party_req_master_to_join(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Party_Req_Master_To_Join");
    packet_trace_pointer(arg0);
}

void game_mode_zc_pc_bang_effect(struct GameMode *arg0, const struct PACKET_ZC_PCBANG_EFFECT *arg1)
{
    packet_trace_call("GameMode_Zc_PcBang_Effect");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_pc_cash_point_itemlist(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Pc_Cash_Point_Itemlist");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_pc_cash_point_update(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Pc_Cash_Point_Update");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_pc_purchase_itemlist(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Pc_Purchase_Itemlist");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_pc_purchase_itemlist2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Pc_Purchase_Itemlist2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_pc_purchase_itemlist_from_mc(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Pc_Purchase_Itemlist_FromMc");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_pc_purchase_my_itemlist(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Pc_Purchase_MyItemlist");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_pc_purchase_result(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Pc_Purchase_Result");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_pc_purchase_result_from_mc(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Pc_Purchase_Result_FromMc");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_pc_sell_itemlist(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Pc_Sell_Itemlist");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_pc_sell_result(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Pc_Sell_Result");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_pet_egg_list(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_PetEgg_List");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_pet_act(struct GameMode *arg0, unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Pet_Act");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_play_npc_bgm(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Play_Npc_Bgm");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_position_id_name_info(struct GameMode *arg0,
                                       const struct PacketPositionNames *arg1)
{
    packet_trace_call("GameMode_Zc_Position_Id_Name_Info");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_position_info(struct GameMode *arg0, const struct PacketPositions *arg1)
{
    packet_trace_call("GameMode_Zc_Position_Info");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_progress(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Progress");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_progress_actor(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Progress_Actor");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_progress_cancel(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Progress_Cancel");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_property_homun(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Property_Homun");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_property_homun_no_accessory(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Property_Homun_No_Accessory");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_property_homun_no_accessory2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Property_Homun_No_Accessory2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_property_homun_no_accessory3(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Property_Homun_No_Accessory3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_property_pet(struct GameMode *arg0, unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Property_Pet");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_quest_notify_effect(struct GameMode *arg0,
                                     const struct PACKET_ZC_QUEST_NOTIFY_EFFECT *arg1)
{
    packet_trace_call("GameMode_Zc_Quest_Notify_Effect");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_refuse_enter_room(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Refuse_Enter_Room");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_repair_item_list(struct GameMode *arg0, const struct PacketRepairItemList *arg1)
{
    packet_trace_call("GameMode_Zc_RepairItemList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_reply_remain_time(struct GameMode *arg0,
                                  const struct PACKET_ZC_REPLY_REMAINTIME *arg1)
{
    packet_trace_call("GameMode_Zc_Reply_RemainTime");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_req_ach_reward_ack(struct GameMode *arg0, unsigned int arg1, int arg2)
{
    packet_trace_call("GameMode_Zc_Req_Ach_Reward_Ack");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
}

void game_mode_zc_req_add_friends(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Req_Add_Friends");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_req_ally_guild(const struct PACKET_ZC_REQ_GUILD_INVITE *arg0)
{
    packet_trace_call("GameMode_Zc_Req_Ally_Guild");
    packet_trace_pointer(arg0);
}

void game_mode_zc_req_baby(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Req_Baby");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_req_couple(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Req_Couple");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_req_exchange_item(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Req_Exchange_Item");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_req_exchange_item2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Req_Exchange_Item2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_req_groupinfo_change_v2(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Req_Groupinfo_Change_V2");
    packet_trace_pointer(arg0);
}

void game_mode_zc_req_join_group(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Req_Join_Group");
    packet_trace_pointer(arg0);
}

void game_mode_zc_req_join_guild(const struct PACKET_ZC_REQ_GUILD_INVITE *arg0)
{
    packet_trace_call("GameMode_Zc_Req_Join_Guild");
    packet_trace_pointer(arg0);
}

void game_mode_zc_req_takeoff_equip_ack(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Req_Takeoff_Equip_Ack");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_req_takeoff_equip_ack2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Req_Takeoff_Equip_Ack2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_req_wear_equip_ack(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Req_Wear_Equip_Ack");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_req_wear_equip_ack2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Req_Wear_Equip_Ack2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_restart_ack(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Restart_Ack");
    packet_trace_pointer(arg0);
}

void game_mode_zc_result_make_guild(const struct PACKET_ZC_RESULT_MAKE_GUILD *arg0)
{
    packet_trace_call("GameMode_Zc_Result_Make_Guild");
    packet_trace_pointer(arg0);
}

void game_mode_zc_result_package_item_test(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Result_Package_Item_Test");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_resurrection(struct GameMode *arg0, const struct PACKET_ZC_RESURRECTION *arg1)
{
    packet_trace_call("GameMode_Zc_Resurrection");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_rodex_block_receive_packet(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Rodex_Block_Receive_Packet");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_rodex_return_ack(struct GameMode *arg0, int arg1, int arg2)
{
    packet_trace_call("GameMode_Zc_Rodex_Return_Ack");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
}

void game_mode_zc_role_change(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Role_Change");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_room_newentry(struct GameMode *arg0, const struct PacketRoomEntry *arg1)
{
    packet_trace_call("GameMode_Zc_Room_Newentry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_say_dialog(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Say_Dialog");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_search_store_info_ack(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Search_Store_Info_Ack");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_search_store_info_ack2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Search_Store_Info_Ack2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_search_store_info_failed(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Search_Store_Info_Failed");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_select_dealtype(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Select_Dealtype");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_setting_whisper_pc(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Setting_Whisper_Pc");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_setting_whisper_pc2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Setting_Whisper_Pc2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_setting_whisper_state(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Setting_Whisper_State");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_short_cut_key_list_v4(struct GameMode *arg0,
                                      const struct PACKET_ZC_SHORTCUT_KEY_LIST_V4 *arg1)
{
    packet_trace_call("GameMode_Zc_ShortCut_Key_List_V4");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_show_digit(struct GameMode *arg0, const struct PACKET_ZC_SHOWDIGIT *arg1)
{
    packet_trace_call("GameMode_Zc_ShowDigit");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_show_image(struct GameMode *arg0, const struct PACKET_ZC_SHOW_IMAGE *arg1)
{
    packet_trace_call("GameMode_Zc_Show_Image");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_show_image2(struct GameMode *arg0, const struct PACKET_ZC_SHOW_IMAGE2 *arg1)
{
    packet_trace_call("GameMode_Zc_Show_Image2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_skill_info_list(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_SkillInfo_List");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_skill_info_list3(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_SkillInfo_List3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_skill_info_update(struct GameMode *arg0,
                                  const struct PACKET_ZC_SKILLINFO_UPDATE *arg1)
{
    packet_trace_call("GameMode_Zc_SkillInfo_Update");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_skill_msg(struct GameMode *arg0, const struct PACKET_ZC_SKILLMSG *arg1)
{
    packet_trace_call("GameMode_Zc_SkillMsg");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_skill_disappear(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Skill_Disappear");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_skill_entry(struct GameMode *arg0, struct PACKET_ZC_SKILL_ENTRY *arg1)
{
    packet_trace_call("GameMode_Zc_Skill_Entry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_skill_entry2(struct GameMode *arg0, struct PACKET_ZC_SKILL_ENTRY2 *arg1)
{
    packet_trace_call("GameMode_Zc_Skill_Entry2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_skill_entry3(struct GameMode *arg0, const struct PACKET_ZC_SKILL_ENTRY3 *arg1)
{
    packet_trace_call("GameMode_Zc_Skill_Entry3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_skill_entry4(struct GameMode *arg0, const struct PACKET_ZC_SKILL_ENTRY4 *arg1)
{
    packet_trace_call("GameMode_Zc_Skill_Entry4");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_skill_entry5(struct GameMode *arg0, const struct PACKET_ZC_SKILL_ENTRY5 *arg1)
{
    packet_trace_call("GameMode_Zc_Skill_Entry5");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_skill_select_request(const struct PACKET_ZC_SKILL_SELECT_REQUEST *arg0)
{
    packet_trace_call("GameMode_Zc_Skill_Select_Request");
    packet_trace_pointer(arg0);
}

void game_mode_zc_skill_update(struct GameMode *arg0, const struct PACKET_ZC_SKILL_UPDATE *arg1)
{
    packet_trace_call("GameMode_Zc_Skill_Update");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_sound(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Sound");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_split_send_item_list_equip(struct GameMode *arg0, unsigned char arg1, int arg2,
                                           const struct EquipItemV7 *arg3)
{
    packet_trace_call("GameMode_Zc_Split_Send_ItemList_Equip");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_pointer(arg3);
}

void game_mode_zc_split_send_item_list_equip_v8(
    struct GameMode *arg0, const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_EQUIPV8 *arg1)
{
    packet_trace_call("GameMode_Zc_Split_Send_ItemList_EquipV8");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_split_send_item_list_normal(struct GameMode *arg0, unsigned char arg1, int arg2,
                                            const struct NormalItem *arg3)
{
    packet_trace_call("GameMode_Zc_Split_Send_ItemList_Normal");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_pointer(arg3);
}

void game_mode_zc_split_send_item_list_result(struct GameMode *arg0, unsigned char arg1,
                                            unsigned char arg2)
{
    packet_trace_call("GameMode_Zc_Split_Send_ItemList_Result");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
}

void game_mode_zc_split_send_item_list_set(struct GameMode *arg0,
                                         const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_SET *arg1)
{
    packet_trace_call("GameMode_Zc_Split_Send_ItemList_Set");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_sprite_change(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Sprite_Change");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_sprite_change2(struct GameMode *arg0, const struct PACKET_ZC_SPRITE_CHANGE2 *arg1)
{
    packet_trace_call("GameMode_Zc_Sprite_Change2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_star_place(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_StarPlace");
    packet_trace_pointer(arg0);
}

void game_mode_zc_star_skill(struct GameMode *arg0, struct PACKET_ZC_STARSKILL *arg1)
{
    packet_trace_call("GameMode_Zc_StarSkill");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_state_change(struct GameMode *arg0, const struct PACKET_ZC_STATE_CHANGE *arg1)
{
    packet_trace_call("GameMode_Zc_State_Change");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_state_change3(struct GameMode *arg0, const struct PACKET_ZC_STATE_CHANGE3 *arg1)
{
    packet_trace_call("GameMode_Zc_State_Change3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_status(struct GameMode *arg0, const struct PACKET_ZC_STATUS *arg1)
{
    packet_trace_call("GameMode_Zc_Status");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_status_change(struct GameMode *arg0, const struct PACKET_ZC_STATUS_CHANGE *arg1)
{
    packet_trace_call("GameMode_Zc_Status_Change");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_status_change_ack(struct GameMode *arg0,
                                   const struct PACKET_ZC_STATUS_CHANGE_ACK *arg1)
{
    packet_trace_call("GameMode_Zc_Status_Change_Ack");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_stopmove(struct GameMode *arg0, unsigned int arg1, short arg2, short arg3,
                          unsigned char arg4)
{
    packet_trace_call("GameMode_Zc_Stopmove");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    packet_trace_value(arg2);
    packet_trace_value(arg3);
    packet_trace_value(arg4);
}

void game_mode_zc_store_assistant_entry(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Store_Assistant_Entry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_store_assistant_entry_v2(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Store_Assistant_Entry_V2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_store_entry(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Store_Entry");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_store_equipment_item_list(struct GameMode *arg0,
                                          const struct PACKET_STORE_LIST *arg1)
{
    packet_trace_call("GameMode_Zc_Store_Equipment_ItemList");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_store_equipment_item_list2(struct GameMode *arg0,
                                           const struct PACKET_STORE_LIST *arg1)
{
    packet_trace_call("GameMode_Zc_Store_Equipment_ItemList2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_store_equipment_item_list3(struct GameMode *arg0,
                                           const struct PACKET_STORE_LIST *arg1)
{
    packet_trace_call("GameMode_Zc_Store_Equipment_ItemList3");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_store_item_list_equip(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Store_ItemList_Equip");
    packet_trace_pointer(arg0);
}

void game_mode_zc_store_item_list_normal(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Store_ItemList_Normal");
    packet_trace_pointer(arg0);
}

void game_mode_zc_store_item_list_normal_v2(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_Store_ItemList_Normal_V2");
    packet_trace_pointer(arg0);
}

void game_mode_zc_store_normal_item_list(const unsigned char *arg0, int arg1)
{
    packet_trace_call("GameMode_Zc_Store_Normal_ItemList");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
}

void game_mode_zc_talkbox_chat_contents(struct GameMode *arg0,
                                      const struct PACKET_ZC_TALKBOX_CHATCONTENTS *arg1)
{
    packet_trace_call("GameMode_Zc_Talkbox_ChatContents");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_throw_mvp_item(void)
{
    packet_trace_call("GameMode_Zc_Throw_MvpItem");
}

void game_mode_zc_update_card_slot(struct GameMode *arg0,
                                 const struct PACKET_ZC_UPDATE_CARDSLOT *arg1)
{
    packet_trace_call("GameMode_Zc_Update_CardSlot");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_update_charstat(struct GameMode *arg0,
                                 const struct PACKET_ZC_UPDATE_CHARSTAT *arg1)
{
    packet_trace_call("GameMode_Zc_Update_Charstat");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_update_charstat2(struct GameMode *arg0,
                                  const struct PACKET_ZC_UPDATE_CHARSTAT2 *arg1)
{
    packet_trace_call("GameMode_Zc_Update_Charstat2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_update_gdid(struct GameMode *arg0, const struct PACKET_ZC_UPDATE_GDID *arg1)
{
    packet_trace_call("GameMode_Zc_Update_Gdid");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_update_gdid2(struct GameMode *arg0, const struct PACKET_ZC_UPDATE_GDID *arg1)
{
    packet_trace_call("GameMode_Zc_Update_Gdid2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_update_item_from_buying_store(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Update_Item_From_Buying_Store");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_update_map_info(struct GameMode *arg0, const struct PACKET_ZC_UPDATE_MAPINFO *arg1)
{
    packet_trace_call("GameMode_Zc_Update_MapInfo");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_update_mission_hunt_ex(struct GameMode *arg0, const void *arg1, size_t arg2)
{
    packet_trace_call("GameMode_Zc_Update_Mission_Hunt_Ex");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
}

void game_mode_zc_update_mission_hunt_ex2(struct GameMode *arg0, const void *arg1, size_t arg2)
{
    packet_trace_call("GameMode_Zc_Update_Mission_Hunt_Ex2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
    packet_trace_value(arg2);
}

void game_mode_zc_use_item_ack(struct GameMode *arg0, const struct PACKET_ZC_USE_ITEM_ACK *arg1)
{
    packet_trace_call("GameMode_Zc_Use_Item_Ack");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_use_item_ack2(struct GameMode *arg0, const struct PACKET_ZC_USE_ITEM_ACK2 *arg1)
{
    packet_trace_call("GameMode_Zc_Use_Item_Ack2");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_user_count(const unsigned char *arg0)
{
    packet_trace_call("GameMode_Zc_User_Count");
    packet_trace_pointer(arg0);
}

void game_mode_zc_wait_dialog(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Wait_Dialog");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_whisper(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Whisper");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_whisper02(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Whisper02");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void game_mode_zc_whisper_list(struct GameMode *arg0, const unsigned char *arg1)
{
    packet_trace_call("GameMode_Zc_Whisper_List");
    packet_trace_pointer(arg0);
    packet_trace_pointer(arg1);
}

void connection_reset_queues(struct Connection *arg0)
{
    packet_trace_call("connection_reset_queues");
    packet_trace_pointer(arg0);
}

int ranking_receive_ids(const void *arg0, size_t arg1)
{
    packet_trace_call("ranking_receive_ids");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    return 0;
}

int ranking_receive_names(const void *arg0, size_t arg1)
{
    packet_trace_call("ranking_receive_names");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    return 0;
}

int ranking_receive_points(const void *arg0, size_t arg1)
{
    packet_trace_call("ranking_receive_points");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    return 0;
}

int skill_cooldowns_receive_delay(const void *arg0, size_t arg1)
{
    packet_trace_call("skill_cooldowns_receive_delay");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    return 0;
}

int skill_cooldowns_receive_delay2(const void *arg0, size_t arg1)
{
    packet_trace_call("skill_cooldowns_receive_delay2");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    return 0;
}

int skill_cooldowns_receive_list(const void *arg0, size_t arg1)
{
    packet_trace_call("skill_cooldowns_receive_list");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    return 0;
}

int skill_cooldowns_receive_list2(const void *arg0, size_t arg1)
{
    packet_trace_call("skill_cooldowns_receive_list2");
    packet_trace_pointer(arg0);
    packet_trace_value(arg1);
    return 0;
}

struct AdventurerAgencyMgr *g_adventurerAgency;
void adventurer_agency_receive(struct AdventurerAgencyMgr *self, const void *p, size_t size)
{ (void)self; packet_trace_call("adventurer_agency_receive"); packet_trace_pointer(p); packet_trace_value(size); }
