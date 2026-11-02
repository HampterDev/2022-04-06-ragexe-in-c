#ifndef GAMEMODE_PACKET_HANDLERS_H
#define GAMEMODE_PACKET_HANDLERS_H

struct AchievementEntry;
struct BuyingStoreEntry;
struct GameMode;
struct EquipItemV7;
struct MarketBoughtItem;
struct NormalItem;
struct NpcMarketEntry;
struct PACKET_STORE_LIST;
struct PACKET_STORE_LIST_NAMED;
struct PACKET_ZC_ABR_INIT;
struct PACKET_ZC_ABR_PAR_CHANGE;
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
struct PACKET_ZC_NOTIFY_ACT_POSITION;
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
struct PacketEquipmentEffect;
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
struct PacketMsgStateChange;
struct PacketMsgStateChange2;
struct PacketMsgStateChange3;
struct PacketNotifyEffect;
struct PacketNotifyEffect2;
struct PacketNotifyEffect3;
struct PacketNotifyEffect4;
struct PacketNotifyExp;
struct PacketNotifyExp2;
struct PacketNotifyGroundSkill;
struct PacketNotifyMemberInfoToGroupm;
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
struct PacketShowScript;
struct PacketShowWarpList;
struct PacketSoulEnergy;
struct PacketSpirits2;
struct PacketSpiritsAttribute;
struct PacketTryResult;
struct PacketUseSkillAck;
struct PacketVendingList4;
struct PacketWarpList;
struct PacketWarpList2;
struct PacketWeaponItemList;
struct PacketWeaponRefine;
struct UIWindow;
struct UserConfigEntry;
struct VendingEntry;

void game_mode_ack_req_name(struct GameMode *self, const unsigned char *pkt);
void game_mode_ack_req_name_all(struct GameMode *self, const unsigned char *pkt);
void game_mode_ack_req_name_all_npc(struct GameMode *self, const unsigned char *pkt);
void game_mode_ack_req_name_by_gid(const unsigned char *pkt);
void game_mode_ack_req_name_by_gid2(const unsigned char *pkt);

void game_mode_ack_banking_deposit(struct GameMode *self, const struct PacketBankingAck *p);
void game_mode_ack_banking_withdraw(struct GameMode *self, const struct PacketBankingAck *p);
void game_mode_banking_check(struct GameMode *self, const struct PacketBankingCheck *p);

void game_mode_zc_battlefield_chat(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_battlefield_notify_camp_info(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_battlefield_notify_hp(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_battlefield_notify_hp2(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_battlefield_notify_point(const unsigned char *pkt);
void game_mode_zc_battlefield_notify_position(struct GameMode *self, const unsigned char *pkt);

void game_mode_zc_info_remain_time(struct GameMode *self,
                                 const struct PACKET_ZC_INFO_REMAINTIME *packet);
void game_mode_zc_pc_bang_effect(struct GameMode *self,
                               const struct PACKET_ZC_PCBANG_EFFECT *packet);
void game_mode_zc_reply_remain_time(struct GameMode *self,
                                  const struct PACKET_ZC_REPLY_REMAINTIME *packet);

void game_mode_broadcast(struct GameMode *self, const unsigned char *pkt);
void game_mode_broadcast2(struct GameMode *self, const unsigned char *pkt);
void game_mode_broadcast4(const unsigned char *pkt);
void game_mode_broadcast_special_item_obtain(struct GameMode *self, const unsigned char *pkt);

void game_mode_camera_info(struct GameMode *self, const unsigned char *pkt);

void game_mode_add_item_to_cart2(const unsigned char *pkt);
void game_mode_add_item_to_cart4(const unsigned char *pkt);
void game_mode_cart_equipment_item_list2(const unsigned char *pkt);
void game_mode_cart_equipment_item_list3(const unsigned char *pkt);
void game_mode_cart_item_list_equip_v5(const unsigned char *pkt);
void game_mode_cart_item_list_equip_v6(const unsigned char *pkt);
void game_mode_cart_item_list_normal_v5(const unsigned char *pkt);
void game_mode_zc_add_item_to_cart3(const unsigned char *pkt);

void game_mode_ack_status_gm(const struct PACKET_ZC_ACK_STATUS_GM *packet);
void game_mode_zc_couple_status(struct GameMode *self, const struct PACKET_ZC_COUPLESTATUS *packet);
void game_mode_zc_status(struct GameMode *self, const struct PACKET_ZC_STATUS *packet);
void game_mode_zc_status_change(struct GameMode *self,
                               const struct PACKET_ZC_STATUS_CHANGE *packet);
void game_mode_zc_status_change_ack(struct GameMode *self,
                                   const struct PACKET_ZC_STATUS_CHANGE_ACK *packet);

void game_mode_zc_guild_chat(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_notify_player_chat(struct GameMode *self, const unsigned char *pkt);

void game_mode_game_mode_show_efst(struct GameMode *mode, int aid, int state, int level, int show);
void game_mode_on_clear_effect(struct GameMode *self, const struct PacketClearEffect *packet);
void game_mode_on_congratulation(struct GameMode *self, const struct PacketCongratulation *packet);
void game_mode_on_ef_warning_plane(struct GameMode *self, int aid, unsigned short skill_id, int level,
                               short x, short y, int duration);
void game_mode_on_notify_effect(struct GameMode *self, const struct PacketNotifyEffect *packet);
void game_mode_on_notify_effect2(struct GameMode *self, const struct PacketNotifyEffect2 *packet);
void game_mode_on_notify_effect3(struct GameMode *self, const struct PacketNotifyEffect3 *packet);
void game_mode_on_notify_effect4(struct GameMode *self, const struct PacketNotifyEffect4 *packet);
void game_mode_on_npc_show_efst_update(struct GameMode *self,
                                  const struct PacketNpcShowEfstUpdate *packet);
void game_mode_zc_devotion_list(struct GameMode *self, const struct PacketDevotionList *packet);
void game_mode_zc_dispel(struct GameMode *self, const struct PacketDispel *packet);


void game_mode_accept_enter(const unsigned char *pkt);
void game_mode_accept_enter2(const unsigned char *pkt);

void game_mode_equipment_item_list2(const unsigned char *pkt);
void game_mode_equipment_item_list3(const unsigned char *pkt);
void game_mode_equipment_item_list4(const unsigned char *pkt);
void game_mode_equipment_item_list_v5(const unsigned char *pkt);
void game_mode_equipment_item_list_v6(const unsigned char *pkt);
void game_mode_equipment_item_list_v7(const unsigned char *pkt);

void game_mode_add_friends_list(const unsigned char *pkt);
void game_mode_friends_list(const unsigned char *pkt);
void game_mode_friends_state(const unsigned char *pkt);

void game_mode_on_item_fall_entry5(struct GameMode *self, const struct PacketItemFallEntry5 *packet);
void game_mode_on_item_pickup_party2(struct GameMode *self,
                                 const struct PacketItemPickupParty2 *packet);

int game_mode_is_exception_state(int state);
void game_mode_zc_item_disappear(struct GameMode *self, const unsigned char *pkt);

void game_mode_on_notify_ground_skill(struct GameMode *self,
                                  const struct PacketNotifyGroundSkill *packet);

void game_mode_on_change_agit_owner(struct GameMode *self, unsigned char agit_id, int old_guild_id,
                                int new_guild_id);
void game_mode_on_guild_agit_info(struct GameMode *self, const struct PacketGuildAgitInfo *packet);
void game_mode_on_req_ack_agit_investment(struct GameMode *self, unsigned char agit_id, int economy,
                                     int defense);
void game_mode_on_req_ack_move_guild_agit(struct GameMode *self, short result);

void game_mode_ack_close_guild_storage(const struct PACKET_ZC_ACK_CLOSE_GUILD_STORAGE *packet);
void game_mode_ack_guild_storage_log(const struct PACKET_ZC_ACK_GUILDSTORAGE_LOG *packet);
void game_mode_ack_guild_storage_log2(const struct PACKET_ZC_ACK_GUILDSTORAGE_LOG2 *packet);
void game_mode_ack_open_guild_storage(const struct PACKET_ZC_ACK_OPEN_GUILD_STORAGE *packet);

void game_mode_inventory_item_list_normal_v5(const unsigned char *pkt);
void game_mode_inventory_tab(const unsigned char *pkt);

void game_mode_zc_split_send_item_list_equip(struct GameMode *self, unsigned char type, int count,
                                           const struct EquipItemV7 *items);
void game_mode_zc_split_send_item_list_equip_v8(
    struct GameMode *self, const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_EQUIPV8 *packet);
void game_mode_zc_split_send_item_list_normal(struct GameMode *self, unsigned char type, int count,
                                            const struct NormalItem *items);
void game_mode_zc_split_send_item_list_result(struct GameMode *self, unsigned char type,
                                            unsigned char result);
void game_mode_zc_split_send_item_list_set(struct GameMode *self,
                                         const struct PACKET_ZC_SPLIT_SEND_ITEMLIST_SET *packet);

void game_mode_cash_item_delete(const unsigned char *pkt);
void game_mode_cash_time_counter(const unsigned char *pkt);
void game_mode_delete_item_from_body(struct GameMode *self, const unsigned char *pkt);

void game_mode_zc_makable_item_list(struct GameMode *self,
                                 const struct PacketMakableItemList *packet);
void game_mode_zc_notify_weapon_item_list(struct GameMode *self,
                                       const struct PacketWeaponItemList *packet);

void game_mode_zc_ack_add_item_to_cart(const unsigned char *pkt);
void game_mode_zc_ack_item_identify(const unsigned char *pkt);
void game_mode_zc_add_item_to_cart(const unsigned char *pkt);
void game_mode_zc_cart_off(void);
void game_mode_zc_cart_equipment_item_list(const unsigned char *pkt);
void game_mode_zc_cart_item_list_equip(const unsigned char *pkt);
void game_mode_zc_cart_item_list_normal(const unsigned char *pkt);
void game_mode_zc_cart_normal_item_list(const unsigned char *pkt, int version);
void game_mode_zc_close_store(void);
void game_mode_zc_delete_item_from_cart(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_delete_item_from_store(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_equipment_item_list(const unsigned char *pkt);
void game_mode_zc_inventory_item_list_normal(const unsigned char *pkt);
void game_mode_zc_normal_item_list(const unsigned char *pkt, int version);
void game_mode_zc_notify_cart_item_count_info(const unsigned char *pkt);
void game_mode_zc_notify_store_item_count_info(const unsigned char *pkt);
void game_mode_zc_store_item_list_equip(const unsigned char *pkt);
void game_mode_zc_store_item_list_normal(const unsigned char *pkt);
void game_mode_zc_store_item_list_normal_v2(const unsigned char *pkt);
void game_mode_zc_store_normal_item_list(const unsigned char *pkt, int version);

void game_mode_on_merge_item_open(struct GameMode *self, const struct PacketMergeItemOpen *packet);

void game_mode_zc_ack_item_composition(struct GameMode *self,
                                     const struct PacketItemComposition *packet);
void game_mode_zc_ack_item_refining(struct GameMode *self, const struct PacketItemRefining *packet);
void game_mode_zc_ack_item_repair(struct GameMode *self, const struct PacketItemRepair *packet);
void game_mode_zc_ack_req_making_item(struct GameMode *self, const struct PacketMakingItem *packet);
void game_mode_zc_ack_weapon_refine(struct GameMode *self, const struct PacketWeaponRefine *packet);
void game_mode_zc_repair_item_list(struct GameMode *self, const struct PacketRepairItemList *packet);

void game_mode_map_property_r2(struct GameMode *self, const struct PacketMapPropertyR2 *p);
void game_mode_notify_map_property(struct GameMode *self, const struct PacketMapProperty *p);
void game_mode_notify_map_property2(struct GameMode *self, const struct PacketMapProperty *p);
void game_mode_zc_notify_map_info(struct GameMode *self,
                                const struct PACKET_ZC_NOTIFY_MAPINFO *packet);

void game_mode_equip_win_microscope2(const unsigned char *pkt);
void game_mode_equip_win_microscope4(const unsigned char *pkt);
void game_mode_equip_win_microscope_v5(const unsigned char *pkt);
void game_mode_equip_win_microscope_v6(const unsigned char *pkt);
void game_mode_equip_win_microscope_v6_body(const unsigned char *pkt);
void game_mode_equip_win_microscope_v7(const unsigned char *pkt);

void game_mode_set_search_store_pos(const struct PACKET_ZC_SEARCH_STORE_POS *packet);
void game_mode_zc_boss_info(struct GameMode *self, const struct PACKET_ZC_BOSS_INFO *packet);
void game_mode_zc_c_marker_info(const struct PACKET_ZC_C_MARKERINFO *packet);
void game_mode_zc_compass(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_quest_notify_effect(struct GameMode *self,
                                     const struct PACKET_ZC_QUEST_NOTIFY_EFFECT *packet);


void game_mode_zc_notify_time(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_restart_ack(const unsigned char *pkt);


void game_mode_memorial_dungeon_info(const unsigned char *pkt);
void game_mode_memorial_dungeon_notify(const unsigned char *pkt);
void game_mode_memorial_dungeon_subscription_info(const unsigned char *pkt);
void game_mode_memorial_dungeon_subscription_notify(const unsigned char *pkt);

void game_mode_ack_add_exchange_item(struct GameMode *self, const unsigned char *pkt);
void game_mode_ack_exchange_item(struct GameMode *self, const unsigned char *pkt);
void game_mode_ack_exchange_item2(struct GameMode *self, const unsigned char *pkt);
void game_mode_add_exchange_item2(const unsigned char *pkt);
void game_mode_add_exchange_item3(const unsigned char *pkt);

void game_mode_ack_whisper(struct GameMode *self, unsigned char result, int receiver_cid);
void game_mode_zc_whisper(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_whisper02(struct GameMode *self, const unsigned char *pkt);

void game_mode_on_pr_npc_state(struct GameMode *self, const struct PacketPrNpcState *packet);
void game_mode_on_remove_neighborhood_characters(struct GameMode *self, int count, const int *aids);
void game_mode_on_show_script(struct GameMode *self, const struct PacketShowScript *packet);
void game_mode_zc_skill_disappear(struct GameMode *self, const unsigned char *pkt);

void game_mode_on_npc_market_open(struct GameMode *self, int count,
                              const struct NpcMarketEntry *items);
void game_mode_on_npc_market_purchase_result(struct GameMode *self, unsigned char result, int count,
                                        const struct MarketBoughtItem *items);

void game_mode_zc_ach_update(struct GameMode *self, int total_points, unsigned rank,
                            int current_points, int next_points, int id, unsigned completed,
                            const int *objectives, int completed_at, unsigned reward);
void game_mode_zc_ack_change_title(struct GameMode *self, unsigned result, int title_id);
void game_mode_zc_all_ach_list(struct GameMode *self, int count, int total_points, unsigned rank,
                              int current_points, int next_points,
                              const struct AchievementEntry *entries);
void game_mode_zc_req_ach_reward_ack(struct GameMode *self, unsigned result, int id);

void game_mode_notify_act(struct GameMode *self, unsigned aid, unsigned target, int start_time,
                        int attack_mt, int attacked_mt, int damage, int count, int action,
                        int left_damage, int is_sp_damage);
void game_mode_on_ack_req_name_all2(struct GameMode *self, const struct PacketAckReqNameAll2 *packet);
void game_mode_zc_attack_failure_for_distance(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_attack_range(const unsigned char *pkt);
void game_mode_zc_blade_stop(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_change_direction(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_combo_delay(struct GameMode *self, const struct PACKET_ZC_COMBODELAY *packet);
void game_mode_zc_emotion(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_equip_arrow(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_fast_move(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_high_jump(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_mvp(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_notify_act_position(struct GameMode *self, const struct PACKET_ZC_NOTIFY_ACT_POSITION *packet);
void game_mode_zc_notify_actor_init(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_notify_font(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_notify_move(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_notify_player_move(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_notify_vanish(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_par_change_user(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_progress(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_progress_actor(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_progress_cancel(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_sprite_change(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_sprite_change2(struct GameMode *self, const struct PACKET_ZC_SPRITE_CHANGE2 *pkt);
void game_mode_zc_stopmove(struct GameMode *self, unsigned aid, short x, short y,
                          unsigned char dir);

void game_mode_zc_ack_create_chatroom(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_change_chatroom(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_destroy_room(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_member_exit(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_member_newentry(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_notify_chat(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_refuse_enter_room(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_role_change(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_setting_whisper_pc(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_setting_whisper_pc2(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_setting_whisper_state(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_whisper_list(struct GameMode *self, const unsigned char *pkt);

void game_mode_zc_dressroom_open(void);
void game_mode_zc_guild_position(struct GameMode *self,
                                const struct PACKET_ZC_GUILD_POSITION *packet);

void game_mode_ack_takeoff_equip_v5(struct GameMode *self, unsigned char result, int wear_location,
                                unsigned short index);
void game_mode_zc_ack_wear_equip_v5(struct GameMode *self, unsigned char result,
                                   unsigned short index, unsigned short view, int wear_location);
void game_mode_zc_req_takeoff_equip_ack(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_req_takeoff_equip_ack2(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_req_wear_equip_ack(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_req_wear_equip_ack2(struct GameMode *self, const unsigned char *pkt);


void game_mode_on_actor_guild(struct GameMode *self, int aid, int guild_id, int emblem_version);
void game_mode_on_emblem_download(int part, int guild_id, int version, const char *data, int size);
void game_mode_zc_ack_disorganize_guild(const unsigned char *pkt);
void game_mode_zc_ack_disorganize_guild_result(const unsigned char *pkt);
void game_mode_zc_ack_guild_menu_interface(const unsigned char *pkt);
void game_mode_zc_ack_req_ally_guild(const unsigned char *pkt);
void game_mode_zc_ack_req_change_members(const unsigned char *pkt);
void game_mode_zc_ack_req_hostile_guild(const unsigned char *pkt);
void game_mode_zc_ack_req_join_guild(const unsigned char *pkt);
void game_mode_zc_add_related_guild(const unsigned char *pkt);
void game_mode_zc_change_guild(struct GameMode *self, const struct PACKET_ZC_CHANGE_GUILD *packet);
void game_mode_zc_delete_related_guild(const unsigned char *pkt);
void game_mode_zc_guild_emblem_img(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_guild_info(const unsigned char *pkt);
void game_mode_zc_guild_info2(const struct PACKET_ZC_GUILD_INFO2 *packet);
void game_mode_zc_guild_member_map_change(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_notify_position_to_guild_m(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_req_ally_guild(const struct PACKET_ZC_REQ_GUILD_INVITE *packet);
void game_mode_zc_req_join_guild(const struct PACKET_ZC_REQ_GUILD_INVITE *packet);
void game_mode_zc_result_make_guild(const struct PACKET_ZC_RESULT_MAKE_GUILD *packet);
void game_mode_zc_update_charstat(struct GameMode *self,
                                 const struct PACKET_ZC_UPDATE_CHARSTAT *packet);
void game_mode_zc_update_charstat2(struct GameMode *self,
                                  const struct PACKET_ZC_UPDATE_CHARSTAT2 *packet);
void game_mode_zc_update_gdid(struct GameMode *self, const struct PACKET_ZC_UPDATE_GDID *packet);
void game_mode_zc_update_gdid2(struct GameMode *self, const struct PACKET_ZC_UPDATE_GDID *packet);

void game_mode_zc_ban_list(struct GameMode *self, const struct PacketBanList *packet);
void game_mode_zc_ban_list_del_name(struct GameMode *self, const struct PacketBanList *packet);
void game_mode_zc_ban_list_new(struct GameMode *self, const struct PacketBanList *packet);
void game_mode_zc_guild_info_del_name(struct GameMode *self, const struct PacketGuildInfo *packet);
void game_mode_zc_guild_info_new(struct GameMode *self, const struct PacketGuildInfoNew *packet);
void game_mode_zc_guild_notice(struct GameMode *self, struct PacketGuildNotice *packet);
void game_mode_zc_my_guild_basic_info(struct GameMode *self,
                                    const struct PacketGuildRelations *packet);
void game_mode_zc_position_id_name_info(struct GameMode *self,
                                       const struct PacketPositionNames *packet);
void game_mode_zc_position_info(struct GameMode *self, const struct PacketPositions *packet);

void game_mode_on_guild_member(const struct PacketGuildMember *packet);
void game_mode_on_member_mgr_info(const struct PacketMemberMgrInfo *packet);
void game_mode_zc_ack_ban_guild_del_name(struct GameMode *self,
                                       const struct PacketGuildBanByGid *packet);
void game_mode_zc_ack_ban_guild_sso(struct GameMode *self, const struct PacketGuildLeave *packet);
void game_mode_zc_ack_change_guild_position_info(const struct PacketPositionInfo *packet);
void game_mode_zc_ack_leave_guild(struct GameMode *self, const struct PacketGuildLeave *packet);
void game_mode_zc_ack_leave_guild_del_name(struct GameMode *self,
                                         const struct PacketGuildLeaveByGid *packet);

void game_mode_zc_member_mgr_info(struct GameMode *self, const struct PacketMemberList *packet);
void game_mode_zc_member_mgr_info_del_name(struct GameMode *self,
                                        const struct PacketMemberList *packet);
void game_mode_zc_member_mgr_info_new(struct GameMode *self, const struct PacketMemberList *packet);
void game_mode_zc_member_add(struct GameMode *self, const struct PacketMemberFull *packet);
void game_mode_zc_member_add_del_name(struct GameMode *self, const struct PacketMemberMemo *packet);
void game_mode_zc_member_add_new(struct GameMode *self, const struct PacketMemberNamed *packet);

void game_mode_zc_abr_init(struct GameMode *self, const struct PACKET_ZC_ABR_INIT *packet);
void game_mode_zc_abr_par_change(struct GameMode *self,
                                const struct PACKET_ZC_ABR_PAR_CHANGE *packet);
int game_mode_zc_battlefield_notify_hp_sp(struct GameMode *self, int aid, int hp, int max_hp, int sp,
                                        int max_sp);
void game_mode_zc_hp_info(struct GameMode *self, const struct PACKET_ZC_HP_INFO *packet);
void game_mode_zc_hp_info_tiny(struct GameMode *self, const struct PACKET_ZC_HP_INFO_TINY *packet);
void game_mode_zc_notify_hp_sp_to_group_m_r2(struct GameMode *self, int aid, int hp, int max_hp,
                                          int sp, int max_sp);

void game_mode_zc_ack_extend_body_item_size(struct GameMode *self,
                                          const struct PACKET_ZC_ACK_EXTEND_BODYITEM_SIZE *packet);
void game_mode_zc_ack_merge_item(struct GameMode *self,
                                const struct PACKET_ZC_ACK_MERGE_ITEM *packet);
void game_mode_zc_ack_open_msg_box_extend_body_item_size(
    struct GameMode *self, const struct PACKET_ZC_ACK_OPEN_MSGBOX_EXTEND_BODYITEM_SIZE *packet);
void game_mode_zc_ack_take_off_equip_all(struct GameMode *self, unsigned result);
void game_mode_zc_broadcast_item_refining_result(struct GameMode *self, const char *name24,
                                               int item_id, unsigned refine, unsigned success);
void game_mode_zc_extend_body_item_size(struct GameMode *self,
                                      const struct PACKET_ZC_EXTEND_BODYITEM_SIZE *packet);
void game_mode_zc_failed_get_item_from_zone_da(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_item_throw_ack(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_notify_bind_on_equip(const unsigned char *pkt);
void game_mode_zc_use_item_ack(struct GameMode *self, const struct PACKET_ZC_USE_ITEM_ACK *packet);
void game_mode_zc_use_item_ack2(struct GameMode *self,
                               const struct PACKET_ZC_USE_ITEM_ACK2 *packet);

void game_mode_zc_result_package_item_test(struct GameMode *self, const unsigned char *pkt);

void game_mode_zc_change_item_option2_packet(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_change_item_option3(struct GameMode *self,
                                     const struct PACKET_ZC_CHANGE_ITEM_OPTION3 *packet);
void game_mode_zc_change_item_option_packet(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_equip_item_damaged(struct GameMode *self,
                                   const struct PACKET_ZC_EQUIPITEM_DAMAGED *packet);
void game_mode_zc_update_card_slot(struct GameMode *self,
                                 const struct PACKET_ZC_UPDATE_CARDSLOT *packet);

void game_mode_zc_el_init(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_el_par_change(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_mer_init(struct GameMode *self, const struct PacketMerInit *pkt);
void game_mode_zc_mer_par_change(const unsigned char *pkt);

void game_mode_zc_clear_dialog(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_close_dialog(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_dialog_text_align(struct GameMode *self, unsigned char align);
void game_mode_zc_menu_list(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_npc_sprite_change(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_npc_chat(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_npc_market_open(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_npc_market_open2(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_npc_market_purchase_result_v2(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_npcack_mapmove(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_npcack_servermove(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_npcack_servermove_domain(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_open_edit_dlg(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_open_edit_dlg_str(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_play_npc_bgm(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_say_dialog(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_select_dealtype(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_sound(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_wait_dialog(struct GameMode *self, const unsigned char *pkt);

void game_mode_party_join_ack(const unsigned char *pkt);
void game_mode_zc_ack_make_group(const unsigned char *pkt);
void game_mode_zc_group_info_change(const unsigned char *pkt);
void game_mode_zc_notify_hp_to_group_m(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_notify_hp_to_group_m_r2(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_notify_position_to_group_m(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_party_config(const unsigned char *pkt);
void game_mode_zc_party_join_req_ack_from_master(const unsigned char *pkt);
void game_mode_zc_party_req_master_to_join(const unsigned char *pkt);
void game_mode_zc_req_groupinfo_change_v2(const unsigned char *pkt);
void game_mode_zc_req_join_group(const unsigned char *pkt);

void game_mode_on_add_member_to_group(struct GameMode *self,
                                 const struct PacketAddMemberToGroup *packet);
void game_mode_on_add_member_to_group2(struct GameMode *self,
                                  const struct PacketAddMemberToGroup2 *packet);
void game_mode_on_list_flag_clear(struct GameMode *self, const struct PacketListFlagClear *packet);
void game_mode_on_party_member_state(struct GameMode *self,
                                 const struct PacketPartyMemberState *packet);

void game_mode_zc_change_state_homun(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_change_state_pet(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_feed(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_ho_par_change(const unsigned char *pkt);
void game_mode_zc_ho_par_change2(const unsigned char *pkt);
void game_mode_zc_pet_egg_list(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_pet_act(struct GameMode *self, unsigned char *pkt);
void game_mode_zc_property_pet(struct GameMode *self, unsigned char *pkt);

void game_mode_on_recovery(struct GameMode *self, const void *packet, int version);

void game_mode_zc_ack_all_rodex_list(struct GameMode *self,
                                    const struct PACKET_ZC_ACK_ALL_RODEX_LIST *packet);
void game_mode_zc_ack_delete_rodex(struct GameMode *self,
                                  const struct PACKET_ZC_ACK_DELETE_RODEX *packet);
void game_mode_zc_ack_failed_all_rodex_list(struct GameMode *self);
void game_mode_zc_ack_item_from_rodex(struct GameMode *self,
                                     const struct PACKET_ZC_ACK_ZENY_FROM_RODEX *packet);
void game_mode_zc_ack_read_rodex2(struct GameMode *self,
                                 const struct PACKET_ZC_ACK_READ_RODEX2 *packet);
void game_mode_zc_ack_rodex_list(struct GameMode *self,
                                const struct PACKET_ZC_ACK_RODEX_LIST *packet);
void game_mode_zc_ack_zeny_from_rodex(struct GameMode *self,
                                     const struct PACKET_ZC_ACK_ZENY_FROM_RODEX *packet);
void game_mode_zc_check_receive_character_name2(struct GameMode *self, int char_id,
                                               unsigned short job, unsigned short level,
                                               const char *name);
void game_mode_zc_rodex_block_receive_packet(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_rodex_return_ack(struct GameMode *self, int mail_id, int result);

void game_mode_zc_ack_add_item_rodex(struct GameMode *self,
                                    const struct PACKET_ZC_ACK_ADD_ITEM_RODEX *packet);
void game_mode_zc_ack_add_item_rodex2(struct GameMode *self,
                                     const struct PACKET_ZC_ACK_ADD_ITEM_RODEX2 *packet);
void game_mode_zc_ack_open_write_rodex(struct GameMode *self,
                                      const struct PACKET_ZC_ACK_OPEN_WRITE_RODEX *packet);
void game_mode_zc_ack_remove_rodex_item(struct GameMode *self,
                                       const struct PACKET_ZC_ACK_REMOVE_RODEX_ITEM *packet);
void game_mode_zc_ack_send_rodex(struct GameMode *self,
                                const struct PACKET_ZC_ACK_SEND_RODEX *packet);

void game_mode_zc_pc_cash_point_itemlist(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_pc_cash_point_update(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_pc_purchase_itemlist(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_pc_purchase_itemlist2(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_pc_purchase_result(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_pc_sell_itemlist(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_pc_sell_result(struct GameMode *self, const unsigned char *pkt);

void game_mode_on_add_exchange_item_grade(struct GameMode *self,
                                     const struct PacketAddExchangeItemGrade *packet);
void game_mode_on_add_exchange_item_opt(struct GameMode *self,
                                   const struct PacketAddExchangeItemOpt *packet);
void game_mode_zc_ack_give_manner_point(const unsigned char *pkt);
void game_mode_zc_baby_msg(const unsigned char *pkt);
void game_mode_zc_cancel_exchange_item(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_conclude_exchange_item(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_couple_name(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_delete_friends(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_divorce(struct GameMode *self, const struct PACKET_ZC_DIVORCE *packet);
void game_mode_zc_exchange_item_undo(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_exec_exchange_item(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_notify_manner_point_given(const unsigned char *pkt);
void game_mode_zc_req_add_friends(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_req_baby(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_req_couple(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_req_exchange_item(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_req_exchange_item2(struct GameMode *self, const unsigned char *pkt);

void game_mode_zc_delete_item_from_mc_store(const unsigned char *pkt);
void game_mode_zc_disappear_entry(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_open_store(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_open_buying_store(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_open_search_store_info(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_pc_purchase_itemlist_from_mc(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_pc_purchase_my_itemlist(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_pc_purchase_result_from_mc(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_search_store_info_ack(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_search_store_info_ack2(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_search_store_info_failed(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_store_entry(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_update_item_from_buying_store(struct GameMode *self, const unsigned char *pkt);

void game_mode_on_use_skill_ack(struct GameMode *self, const struct PacketUseSkillAck *packet,
                            int version);
void game_mode_on_use_skill_ack3(struct GameMode *self, const struct PacketUseSkillAck *packet);

void game_mode_zc_monster_info(struct GameMode *self, const struct PACKET_ZC_MONSTER_INFO *packet);
void game_mode_zc_resurrection(struct GameMode *self, const struct PACKET_ZC_RESURRECTION *packet);
void game_mode_zc_show_image(struct GameMode *self, const struct PACKET_ZC_SHOW_IMAGE *packet);
void game_mode_zc_show_image2(struct GameMode *self, const struct PACKET_ZC_SHOW_IMAGE2 *packet);
void game_mode_zc_star_place(const unsigned char *pkt);
void game_mode_zc_talkbox_chat_contents(struct GameMode *self,
                                      const struct PACKET_ZC_TALKBOX_CHATCONTENTS *packet);
void game_mode_zc_update_map_info(struct GameMode *self,
                                const struct PACKET_ZC_UPDATE_MAPINFO *packet);

void game_mode_zc_par_change(struct GameMode *self, struct PACKET_ZC_PAR_CHANGE *packet);

void game_mode_add_member_to_group(struct GameMode *self, const unsigned char *pkt);
void game_mode_change_group_master(const unsigned char *pkt);
void game_mode_delete_member_from_group(struct GameMode *self, const unsigned char *pkt);
void game_mode_group_list(const unsigned char *pkt);
void game_mode_notify_chat_party(struct GameMode *self, const unsigned char *pkt);
void game_mode_on_group_list2(struct GameMode *self, const struct PacketGroupList2 *packet);
void game_mode_on_group_list3(struct GameMode *self, const struct PacketGroupList3 *packet);
void game_mode_on_notify_member_info_to_groupm(struct GameMode *self,
                                         const struct PacketNotifyMemberInfoToGroupm *packet);

void game_mode_item_pickup_ack(struct GameMode *self, const unsigned char *pkt);
void game_mode_item_pickup_ack2(struct GameMode *self, const unsigned char *pkt);
void game_mode_item_pickup_ack3(struct GameMode *self, const unsigned char *pkt);
void game_mode_item_pickup_ack_v6(struct GameMode *self, const unsigned char *pkt);
void game_mode_item_pickup_ack_v7(struct GameMode *self, const unsigned char *pkt);
void game_mode_item_pickup_ack_v8(struct GameMode *self, const unsigned char *pkt);
void game_mode_item_pickup_party(const unsigned char *pkt);
char game_mode_pickup_failed(int result);
void game_mode_zc_item_pickup_ack_v5(struct GameMode *self, const unsigned char *pkt);

void game_mode_on_action_failure(struct GameMode *self, const struct PacketActionFailure *packet);
void game_mode_on_personal_info(struct GameMode *self, const struct PacketPersonalInfo *packet);
void game_mode_on_recover_penalty_overweight(struct GameMode *self,
                                         const struct PacketRecoverPenaltyOverweight *packet);
void game_mode_on_sync_attacked_pos(struct GameMode *self, const void *packet);

void game_mode_on_ack_private_airplane_call(struct GameMode *self, int result);
void game_mode_on_map_move_private_airplane(struct GameMode *self, const char *map_name,
                                       unsigned short x, unsigned short y);
void game_mode_on_server_move_private_airplane(struct GameMode *self, const char *map_name, short x,
                                          short y, int ip, short port);

void game_mode_on_latest_rodex_list(struct GameMode *self, const struct PacketLatestRodexList *packet);
void game_mode_on_notify_unread_rodex(struct GameMode *self,
                                  const struct PacketNotifyUnreadRodex *packet);

void game_mode_zc_show_digit(struct GameMode *self, const struct PACKET_ZC_SHOWDIGIT *packet);

void game_mode_ack_buying_store_list(struct GameMode *self, const unsigned char *pkt);
void game_mode_buying_store_bought(int time, int item_id, int count, int zeny, int zeny_left,
                                int seller_cid);
void game_mode_buying_store_bought_packet(const unsigned char *pkt);
void game_mode_disappear_buying_store_entry(struct GameMode *self, const unsigned char *pkt);
void game_mode_failed_open_buying_store(const unsigned char *pkt);
void game_mode_failed_trade_buying_store_to_buyer(struct GameMode *self, const unsigned char *pkt);
void game_mode_failed_trade_buying_store_to_seller(const unsigned char *pkt);
void game_mode_item_delete_buying_store(const unsigned char *pkt);
void game_mode_my_buying_store_list(struct GameMode *self, const unsigned char *pkt);
void game_mode_shop_item_sold(int date, unsigned short index, unsigned short amount, int zeny,
                           int buyer_cid);
void game_mode_shop_item_sold_packet(const unsigned char *pkt);
void game_mode_shop_not_movable_items(const unsigned char *pkt);
void game_mode_shop_open_result(struct GameMode *self, unsigned char result);

void game_mode_zc_short_cut_key_list_v4(struct GameMode *self,
                                      const struct PACKET_ZC_SHORTCUT_KEY_LIST_V4 *packet);

void game_mode_ack_to_use_skill(struct GameMode *self, const struct PACKET_ZC_ACK_TOUSESKILL *packet);

void game_mode_on_autospell_list2(int count, const int *skills);
void game_mode_skill_update(struct GameMode *self, unsigned short id, short level, short sp,
                          unsigned char upgradable, short range, int type, unsigned short level2);
void game_mode_zc_auto_spell_list(const unsigned char *pkt);
void game_mode_zc_autorun_skill(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_guild_skill_info(struct GameMode *self,
                                 const struct PACKET_ZC_GUILD_SKILLINFO *packet);
void game_mode_zc_making_arrow_list(const unsigned char *pkt);
void game_mode_zc_making_item_list(const unsigned char *pkt);
void game_mode_zc_msg_skill(const unsigned char *pkt);
void game_mode_zc_skill_info_list(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_skill_info_list3(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_skill_info_update(struct GameMode *self,
                                  const struct PACKET_ZC_SKILLINFO_UPDATE *packet);
void game_mode_zc_skill_select_request(const struct PACKET_ZC_SKILL_SELECT_REQUEST *packet);

void game_mode_on_notify_skill(struct GameMode *self, const struct PacketNotifySkill *packet);
void game_mode_on_notify_skill2(struct GameMode *self, const struct PacketNotifySkill2 *packet);
void game_mode_on_notify_skill_position(struct GameMode *self,
                                    const struct PacketNotifySkillPosition *packet);

void game_mode_zc_skill_entry(struct GameMode *self, struct PACKET_ZC_SKILL_ENTRY *packet);
void game_mode_zc_skill_entry2(struct GameMode *self, struct PACKET_ZC_SKILL_ENTRY2 *packet);
void game_mode_zc_skill_entry3(struct GameMode *self, const struct PACKET_ZC_SKILL_ENTRY3 *packet);
void game_mode_zc_skill_entry4(struct GameMode *self, const struct PACKET_ZC_SKILL_ENTRY4 *packet);
void game_mode_zc_skill_entry5(struct GameMode *self, const struct PACKET_ZC_SKILL_ENTRY5 *packet);
void game_mode_zc_skill_update(struct GameMode *self, const struct PACKET_ZC_SKILL_UPDATE *packet);

void game_mode_show_skill_use(struct GameMode *self, unsigned short skill_id, int level, int src_aid,
                           int target_aid, char result);

void game_mode_on_millennium_shield(struct GameMode *self,
                                 const struct PacketMillenniumShield *packet);
void game_mode_on_soul_energy(struct GameMode *self, const struct PacketSoulEnergy *packet);
void game_mode_on_spirits(struct GameMode *self, int aid, short num);
void game_mode_on_spirits2(struct GameMode *self, const struct PacketSpirits2 *packet);
void game_mode_on_spirits_attribute(struct GameMode *self,
                                 const struct PacketSpiritsAttribute *packet);
void game_mode_on_target_spirits(struct GameMode *self, int aid, short num);

void game_mode_zc_star_skill(struct GameMode *self, struct PACKET_ZC_STARSKILL *packet);

void game_mode_on_efst_set_enter(struct GameMode *self, const struct PacketEfstSetEnter *packet);
void game_mode_on_equipment_effect(struct GameMode *self, const struct PacketEquipmentEffect *packet);
void game_mode_on_msg_state_change(struct GameMode *self, const struct PacketMsgStateChange *packet);
void game_mode_on_msg_state_change2(struct GameMode *self, const struct PacketMsgStateChange2 *packet);
void game_mode_on_msg_state_change3(struct GameMode *self, const struct PacketMsgStateChange3 *packet);
void game_mode_on_set_clairvoyance(struct GameMode *self, char on);

void game_mode_show_player_states(struct GameMode *self, int body_state, int health_state);
void game_mode_zc_state_change(struct GameMode *self, const struct PACKET_ZC_STATE_CHANGE *packet);
void game_mode_zc_state_change3(struct GameMode *self,
                               const struct PACKET_ZC_STATE_CHANGE3 *packet);

void game_mode_on_long_long_par_change(struct GameMode *self,
                                  const struct PacketLongLongParChange *packet);
void game_mode_on_long_par_change(struct GameMode *self, const struct PacketLongParChange *packet);
void game_mode_on_notify_exp(struct GameMode *self, const struct PacketNotifyExp *packet);
void game_mode_on_notify_exp2(struct GameMode *self, const struct PacketNotifyExp2 *packet);

void game_mode_add_item_to_store2(const unsigned char *pkt);
void game_mode_add_item_to_store3(const unsigned char *pkt);
void game_mode_add_item_to_store4(const unsigned char *pkt);
void game_mode_store_item_list_equip_v5(const unsigned char *pkt);
void game_mode_store_item_list_equip_v6(const unsigned char *pkt);
void game_mode_store_item_list_normal_v5(const unsigned char *pkt);

void game_mode_zc_store_item_list_equip_v2(struct GameMode *self,
                                 const struct PACKET_STORE_LIST_NAMED *packet);
void game_mode_zc_store_item_list_equip_v5(struct GameMode *self,
                                 const struct PACKET_STORE_LIST_NAMED *packet);
void game_mode_zc_store_item_list_equip_v6(struct GameMode *self,
                                 const struct PACKET_STORE_LIST_NAMED *packet);
void game_mode_zc_store_item_list_normal_v5(struct GameMode *self,
                                  const struct PACKET_STORE_LIST_NAMED *packet);
void game_mode_zc_store_equipment_item_list(struct GameMode *self,
                                          const struct PACKET_STORE_LIST *packet);
void game_mode_zc_store_equipment_item_list2(struct GameMode *self,
                                           const struct PACKET_STORE_LIST *packet);
void game_mode_zc_store_equipment_item_list3(struct GameMode *self,
                                           const struct PACKET_STORE_LIST *packet);

void game_mode_on_ack_create_assistant(unsigned char kind, unsigned char result);
void game_mode_on_buying_store_item_list(struct GameMode *self, int owner_aid, int store_id,
                                    char is_mine, int count, int duration,
                                    const struct BuyingStoreEntry *entries, int zeny_limit);
void game_mode_on_open_ui_assistant_buying(struct GameMode *self, unsigned char slots);
void game_mode_on_open_ui_assistant_vending(struct GameMode *self, unsigned char slots, int count,
                                       const void *items);
void game_mode_on_purchase_item_list_from_mc4(struct GameMode *self,
                                        const struct PacketPurchaseItemListFromMc4 *packet);
void game_mode_on_purchase_my_item_list3(struct GameMode *self,
                                    const struct PacketPurchaseMyItemList3 *packet);
void game_mode_on_store_assistant_disappear(struct GameMode *self, int aid);
void game_mode_on_vending_store_item_list(struct GameMode *self, int owner_aid, int store_id,
                                     char is_mine, int count, int duration,
                                     const struct VendingEntry *entries);
void game_mode_on_vending_store_item_list4(struct GameMode *self,
                                      const struct PacketVendingList4 *packet);
void game_mode_zc_store_assistant_entry(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_store_assistant_entry_v2(struct GameMode *self, const unsigned char *pkt);

void game_mode_format_string_msg(const unsigned char *pkt);
void game_mode_item_name_msg(const unsigned char *pkt);
void game_mode_msg(const unsigned char *pkt);
void game_mode_zc_mvp_getting_special_exp(const unsigned char *pkt);
void game_mode_msg_color(const unsigned char *pkt);
void game_mode_msg_value(const unsigned char *pkt);
void game_mode_on_msg_value_string(struct GameMode *self, unsigned short msg_id, int color,
                               const char *value);
void game_mode_zc_ack_disconnect_character(const unsigned char *pkt);
void game_mode_zc_debug_msg(const unsigned char *pkt);
void game_mode_zc_hunting_list(struct GameMode *self, const struct PACKET_ZC_HUNTINGLIST *packet);
void game_mode_zc_normal_system_msg(const unsigned char *pkt);
void game_mode_zc_skill_msg(struct GameMode *self, const struct PACKET_ZC_SKILLMSG *packet);
void game_mode_zc_throw_mvp_item(void);
void game_mode_zc_user_count(const unsigned char *pkt);

void game_mode_on_start_baby(struct GameMode *self, const void *packet);
void game_mode_on_start_capture(struct GameMode *self, const void *packet);
void game_mode_on_start_collection(struct GameMode *self, const void *packet);
void game_mode_on_start_couple(struct GameMode *self, const void *packet);
void game_mode_on_try_capture_monster(struct GameMode *self, const struct PacketTryResult *packet);
void game_mode_on_try_collection(struct GameMode *self, const struct PacketTryResult *packet);

void game_mode_ack_buy_styling_shop(struct GameMode *self, const unsigned char *pkt);
void game_mode_ack_check_attendance(struct GameMode *self, int result, int day);
void game_mode_item_list_win_open(struct GameMode *self, const unsigned char *pkt);
void game_mode_select_cart(struct GameMode *self, const unsigned char *pkt);
void game_mode_ui_open(unsigned char type);
void game_mode_ui_open_v2(struct GameMode *self, unsigned char type, int value);

void game_mode_config_notify2(struct GameMode *self, unsigned char open_equip_window,
                            unsigned char call_allowed);
void game_mode_config_notify3(struct GameMode *self, unsigned char open_equip_window,
                            unsigned char call_allowed, unsigned char pet_auto_feed);
void game_mode_config_notify4(struct GameMode *self, unsigned char open_equip_window,
                            unsigned char call_allowed, unsigned char pet_auto_feed,
                            unsigned char homun_auto_feed);
void game_mode_get_user_config_list(struct GameMode *self, int count,
                                const struct UserConfigEntry *entries);
void game_mode_set_user_config_ack(struct GameMode *self, int type, int result, int value);
void game_mode_zc_config_notify(struct GameMode *self, const unsigned char *pkt);
void game_mode_zc_config(struct GameMode *self, const unsigned char *pkt);

void game_mode_on_show_warp_list(struct GameMode *self, struct PacketShowWarpList *packet);
void game_mode_on_warp_list(struct GameMode *self, const struct PacketWarpList *packet);
void game_mode_on_warp_list2(struct GameMode *self, struct PacketWarpList2 *packet);
void game_mode_zc_ack_remember_warppoint(const unsigned char *pkt);

#endif
