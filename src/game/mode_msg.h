#include "game/actor_names.h"
#include "game/camera.h"
#include "game/mode_util.h"
#include "actor/actor_ops.h"
#include "platform/code_page.h"
#include "platform/language_api.h"
#include "skills/skill_bundle.h"
#include "skills/skill_info.h"
#include "iteminfo/item_api.h"
#include "session/character_name.h"
#include "platform/service.h"
#include "contents/guild_info.h"
#include "session/state.h"
#include "contents/connection_api.h"
#ifndef GAMEMODE_MODE_MSG_H
#define GAMEMODE_MODE_MSG_H

int game_mode_to_login(void);
int game_mode_msg_go_to_login(void);

#include "game/gamemode.h"
#include "game/client.h"
#include "game/item_info.h"
#include "core/msg_str.h"

enum {
    MODE_MSG_CONNECTION_LOST = 0x1,
    MODE_MSG_POINT_MOUSE_AT = 0x4,
    MODE_MSG_HIDE_MOUSE_POINTER = 0x5,
    MODE_MSG_CHAT = 0x6,
    MODE_MSG_BROADCAST = 0x7,
    MODE_MSG_GET_MAP_NAME = 0x9,
    MODE_MSG_CONTACT_NPC = 0xa,
    MODE_MSG_SET_UNIT_CELL = 0xc,
    MODE_MSG_DISCONNECT_CHARACTER = 0xe,
    MODE_MSG_DISCONNECT_ALL = 0xf,
    MODE_MSG_THROW_ITEM = 0x10,
    MODE_MSG_DROP_DRAGGED = 0x11,
    MODE_MSG_CANCEL_DRAG = 0x12,
    MODE_MSG_WEAR_EQUIP = 0x13,
    MODE_MSG_TAKEOFF_EQUIP = 0x14,
    MODE_MSG_THROW_DRAGGED = 0x15,
    MODE_MSG_NPC_NEXT = 0x16,
    MODE_MSG_NPC_CHOOSE_MENU = 0x17,
    MODE_MSG_POPUP_CHOICE = 0x18,
    MODE_MSG_RESTART = 0x19,
    MODE_MSG_SET_ATTACK_IN_PROGRESS = 0x1a,
    MODE_MSG_USE_ITEM = 0x1b,
    MODE_MSG_REQ_STATUS = 0x1d,
    MODE_MSG_STATUS_CHANGE = 0x1e,
    MODE_MSG_EMOTION = 0x1f,
    MODE_MSG_USER_COUNT = 0x20,
    MODE_MSG_NPC_BUY = 0x22,
    MODE_MSG_NPC_SELL = 0x23,
    MODE_MSG_NPC_DEAL_SELL = 0x25,
    MODE_MSG_NPC_DEAL_BUY = 0x26,
    MODE_MSG_NPC_TRADE_CLOSED = 0x28,
    MODE_MSG_TOGGLE_STATUS_BARS = 0x29,
    MODE_MSG_CHAT_ROOM_MSG = 0x2a,
    MODE_MSG_CREATE_CHAT_ROOM = 0x2b,
    MODE_MSG_CHANGE_CHAT_ROOM = 0x2c,
    MODE_MSG_ENTER_CHAT_ROOM = 0x2d,
    MODE_MSG_CHAT_ROOM_ROLE = 0x2e,
    MODE_MSG_CHAT_ROOM_EXPEL = 0x2f,
    MODE_MSG_EXIT_CHAT_ROOM = 0x30,
    MODE_MSG_REQ_EXCHANGE = 0x31,
    MODE_MSG_ACK_EXCHANGE = 0x32,
    MODE_MSG_ADD_EXCHANGE_ITEM = 0x33,
    MODE_MSG_CONCLUDE_EXCHANGE = 0x34,
    MODE_MSG_CANCEL_EXCHANGE = 0x35,
    MODE_MSG_EXEC_EXCHANGE = 0x36,
    MODE_MSG_ITEM_TO_STORAGE = 0x37,
    MODE_MSG_ITEM_FROM_STORAGE = 0x38,
    MODE_MSG_CLOSE_STORAGE = 0x39,
    MODE_MSG_MAKE_PARTY = 0x3a,
    MODE_MSG_PARTY_INVITE = 0x3b,
    MODE_MSG_PARTY_INVITE_ANSWER = 0x3c,
    MODE_MSG_LEAVE_PARTY = 0x3d,
    MODE_MSG_EXPEL_PARTY_MEMBER = 0x3e,
    MODE_MSG_SET_SQUARE_MARK = 0x3f,
    MODE_MSG_REMOVE_SQUARE_MARK = 0x40,
    MODE_MSG_CLEAR_SQUARE_MARKS = 0x41,
    MODE_MSG_PARTY_CHAT = 0x42,
    MODE_MSG_UPGRADE_SKILL = 0x43,
    MODE_MSG_USE_SKILL = 0x45,
    MODE_MSG_USE_SKILL_TO_GROUND = 0x46,
    MODE_MSG_CANCEL_SKILL_AIM = 0x47,
    MODE_MSG_AIM_SKILL = 0x48,
    MODE_MSG_CANCEL_LOCKON = 0x49,
    MODE_MSG_SELECT_WARP_POINT = 0x4a,
    MODE_MSG_REMEMBER_WARP_POINT = 0x4b,
    MODE_MSG_ITEM_TO_CART = 0x4c,
    MODE_MSG_ITEM_FROM_CART = 0x4d,
    MODE_MSG_STORAGE_TO_CART = 0x4e,
    MODE_MSG_CART_TO_STORAGE = 0x4f,
    MODE_MSG_UNINSTALLATION = 0x50,
    MODE_MSG_CLOSE_VENDING = 0x51,
    MODE_MSG_OPEN_VENDING = 0x52,
    MODE_MSG_BUY_FROM_VENDOR = 0x53,
    MODE_MSG_BROWSE_VENDOR = 0x54,
    MODE_MSG_SKILL_SHOUT = 0x56,
    MODE_MSG_WEAR_EQUIP_DIRECT = 0x57,
    MODE_MSG_QUIT_GAME = 0x58,
    MODE_MSG_CLOSE_NPC_DIALOG = 0x59,
    MODE_MSG_RESUME_BGM = 0x5a,
    MODE_MSG_CREATE_ITEM = 0x5b,
    MODE_MSG_MOVE_TO_MAP = 0x5c,
    MODE_MSG_SKILL_AIM_LEVEL_UP = 0x5d,
    MODE_MSG_SKILL_AIM_LEVEL_DOWN = 0x5e,
    MODE_MSG_GET_SKILL_AIM_TYPE = 0x5f,
    MODE_MSG_PARTY_EXP_OPTION = 0x60,
    MODE_MSG_GET_SKILL_AIM_LEVEL = 0x61,
    MODE_MSG_EDIT_DIALOG_INPUT = 0x63,
    MODE_MSG_SHOW_ILLUSTRATION = 0x64,
    MODE_MSG_SET_POPUP_TARGET_NAME = 0x65,
    MODE_MSG_REGISTER_GUILD_EMBLEM = 0x66,
    MODE_MSG_CHANGE_GUILD_MEMBER_POS = 0x67,
    MODE_MSG_CHANGE_GUILD_POSITIONS = 0x68,
    MODE_MSG_GUILD_MENU_INTERFACE = 0x69,
    MODE_MSG_DISORGANIZE_GUILD = 0x6a,
    MODE_MSG_GUILD_MENU = 0x6b,
    MODE_MSG_OPEN_GUILD_MEMBER_INFO = 0x6d,
    MODE_MSG_LEAVE_GUILD = 0x6e,
    MODE_MSG_BAN_GUILD_MEMBER = 0x6f,
    MODE_MSG_MAKE_GUILD = 0x70,
    MODE_MSG_SKILL_TARGET = 0x71,
    MODE_MSG_JOIN_GUILD_ANSWER = 0x72,
    MODE_MSG_GUILD_INVITE = 0x73,
    MODE_MSG_GIVE_MANNER_POINT = 0x74,
    MODE_MSG_GIVE_MANNER_POINT_BY_AID = 0x75,
    MODE_MSG_GUILD_NOTICE = 0x76,
    MODE_MSG_GUILD_ALLY = 0x77,
    MODE_MSG_GUILD_ALLY_ANSWER = 0x78,
    MODE_MSG_IDENTIFY_ITEM = 0x79,
    MODE_MSG_QUAKE = 0x7a,
    MODE_MSG_ITEM_COMPOSITION_LIST = 0x7b,
    MODE_MSG_ITEM_COMPOSITION = 0x7c,
    MODE_MSG_GUILD_HOSTILE = 0x7d,
    MODE_MSG_DELETE_RELATED_GUILD = 0x7f,
    MODE_MSG_REQ_DISCONNECT = 0x80,
    MODE_MSG_GUILD_CHAT = 0x81,
    MODE_MSG_MAKE_ITEM = 0x82,
    MODE_MSG_USE_SKILL_TO_GROUND_TALKBOX = 0x83,
    MODE_MSG_DRAG_BY_NAME = 0x86,
    MODE_MSG_SET_VIEW = 0x87,
    MODE_MSG_PICKUP_ITEM = 0x88,
    MODE_MSG_REQUEST_ACT = 0x89,
    MODE_MSG_REQUEST_MOVE = 0x8a,
    MODE_MSG_CHANGE_DIRECTION = 0x8b,
    MODE_MSG_COUPLE_REQUEST = 0x8c,
    MODE_MSG_GO_TO_LOGIN = 0x8d,
    MODE_MSG_RESET_STATUS = 0x8e,
    MODE_MSG_CHANGE_MAP_TYPE = 0x8f,
    MODE_MSG_CHANGE_EFFECT_STATE = 0x90,
    MODE_MSG_RENAME_PET = 0x91,
    MODE_MSG_SELECT_PET_EGG = 0x92,
    MODE_MSG_PET_EGG_INFO = 0x93,
    MODE_MSG_PET_ACT = 0x94,
    MODE_MSG_CAPTURE_MONSTER = 0x95,
    MODE_MSG_COMMAND_PET = 0x96,
    MODE_MSG_PROHIBIT_REMOVE = 0x97,
    MODE_MSG_PROHIBIT_ADD = 0x98,
    MODE_MSG_MAKE_ARROW = 0x99,
    MODE_MSG_CHANGE_CART = 0x9a,
    MODE_MSG_REMOVE_AID = 0x9b,
    MODE_MSG_SHIFT = 0x9c,
    MODE_MSG_RECALL_AID = 0x9d,
    MODE_MSG_RECALL_NAME = 0x9e,
    MODE_MSG_REQ_REMAIN_TIME = 0xa1,
    MODE_MSG_SELECT_AUTOSPELL = 0xa3,
    MODE_MSG_NPC_INPUT_TEXT = 0xa4,
    MODE_MSG_SKILL_SCREAM_LINE = 0xa5,
    MODE_MSG_JOIN_COUPLE = 0xa7,
    MODE_MSG_MAKE_PARTY_WITH_OPTIONS = 0xa8,
    MODE_MSG_DORIDORI = 0xa9,
    MODE_MSG_TRIANGLE_MARK_SET = 0xaa,
    MODE_MSG_TRIANGLE_MARK_REMOVE = 0xab,
    MODE_MSG_TRIANGLE_MARKS_CLEAR = 0xac,
    MODE_MSG_JOIN_BABY = 0xad,
    MODE_MSG_REPAIR_ITEM = 0xae,
    MODE_MSG_ADD_FRIEND_ANSWER = 0xaf,
    MODE_MSG_DELETE_FRIEND = 0xb0,
    MODE_MSG_REQ_STATUS_GM = 0xb1,
    MODE_MSG_GIVE_MANNER_BY_NAME = 0xb2,
    MODE_MSG_REQ_JOIN_BABY = 0xb3,
    MODE_MSG_WEAPON_REFINE = 0xb6,
    MODE_MSG_COMMAND_MERCENARY = 0xb8,
    MODE_MSG_RENAME_MERCENARY = 0xb9,
    MODE_MSG_MERCENARY_COMMAND = 0xba,
    MODE_MSG_SHOW_EQUIP_OF = 0xbd,
    MODE_MSG_AGREE_STARPLACE = 0xbe,
    MODE_MSG_MAIL_GET_LIST = 0xbf,
    MODE_MSG_SEND_RODEX = 0xc0,
    MODE_MSG_MAIL_RESET_ITEM = 0xc1,
    MODE_MSG_READ_RODEX = 0xc2,
    MODE_MSG_ADD_ITEM_RODEX = 0xc3,
    MODE_MSG_DELETE_RODEX = 0xc4,
    MODE_MSG_RETURN_MAIL = 0xc5,
    MODE_MSG_MAIL_GET_ITEM = 0xc6,
    MODE_MSG_MAKING_ITEM = 0xcf,
    MODE_MSG_NPC_STORE_WINDOWS_CLOSED = 0xf5,
    MODE_MSG_HUNTING_LIST = 0xf6,
    MODE_MSG_BUY_CASH_ITEM = 0xf9,
    MODE_MSG_STANDING_RESURRECTION = 0xfa,
    MODE_MSG_CONFIG = 0xfd,
    MODE_MSG_BATTLEFIELD_CHAT = 0xfe,
    MODE_MSG_SKILL_SELECT_RESPONSE = 0x100,
    MODE_MSG_GROUP_INFO_CHANGE = 0x103,
    MODE_MSG_CHANGE_GROUP_MASTER = 0x104,
    MODE_MSG_OPEN_SEEK_PARTY_MB_WND = 0x105,
    MODE_MSG_NPC_MARKET_PURCHASE = 0x106,
    MODE_MSG_NPC_MARKETGEAR_PURCHASE = 0x107,
    MODE_MSG_PARTY_JOIN_MASTER_ACK = 0x108,
    MODE_MSG_PARTY_REQ_MASTER_TO_JOIN = 0x109,
    MODE_MSG_ITEMLISTWIN_SEND = 0x10a,
    MODE_MSG_ITEMLISTWIN_CANCEL = 0x10b,
    MODE_MSG_OPEN_WRITE_RODEX = 0x10c,
    MODE_MSG_OPEN_SEEK_PARTY_LIST_WND = 0x10d,
    MODE_MSG_CLOSE_BUYING_STORE = 0x10e,
    MODE_MSG_OPEN_BUYING_STORE = 0x10f,
    MODE_MSG_CLICK_BUYING_STORE = 0x110,
    MODE_MSG_TRADE_BUYING_STORE = 0x111,
    MODE_MSG_TRY_COLLECTION = 0x11a,
    MODE_MSG_SET_PARTY_NAME_WND = 0x11d,
    MODE_MSG_GUILD_INVITE_BY_NAME = 0x11e,
    MODE_MSG_REQ_RANKING = 0x11f,
    MODE_MSG_NPC_TRADE_QUIT = 0x123,
    MODE_MSG_NPC_MARKET_CLOSE = 0x124,
    MODE_MSG_CANCEL_WRITE_RODEX = 0x125,
    MODE_MSG_REMOVE_RODEX_ITEM = 0x126,
    MODE_MSG_CREATE_ASSISTANT_VENDING = 0x128,
    MODE_MSG_CANCEL_ASSISTANT_VENDING = 0x129,
    MODE_MSG_CREATE_ASSISTANT_BUYING = 0x12a,
    MODE_MSG_CANCEL_ASSISTANT_BUYING = 0x12b,
    MODE_MSG_DESTROY_STORE_ASSISTANT = 0x12d,
    MODE_MSG_ADD_SWAP_EQUIP = 0x12e,
    MODE_MSG_REMOVE_SWAP_EQUIP = 0x12f,
    MODE_MSG_DO_SWAP_EQUIP = 0x130,
    MODE_MSG_SWAP_EQUIP_1_ON_1 = 0x131,
    MODE_MSG_MEMORIAL_DUNGEON_COMMAND = 0x132,
    MODE_MSG_NPC_BARTER_PURCHASE = 0x134,
    MODE_MSG_NPC_BARTER_CLOSE = 0x135,
    MODE_MSG_OPEN_EXTEND_BODYITEM = 0x136,
    MODE_MSG_EXTEND_BODYITEM = 0x137,
    MODE_MSG_CLOSE_EXTEND_BODYITEM = 0x138,
    MODE_MSG_SHORTCUT_KEY_CHANGE = 0x139,
    MODE_MSG_ADVANCED_STATUS_CHANGE = 0x13a,
    MODE_MSG_MOVE_GUILD_AGIT = 0x13b,
    MODE_MSG_AGIT_INVESTMENT = 0x13c,
    MODE_MSG_PRISON = 0x13d,
    MODE_MSG_OPEN_LIMITED_SALE_TOOL = 0x13e,
    MODE_MSG_CLOSE_LIMITED_SALE_TOOL = 0x13f,
    MODE_MSG_LIMITED_SALE_LIST = 0x140,
    MODE_MSG_NPC_EXPANDED_BARTER_PURCHASE = 0x141,
    MODE_MSG_NPC_EXPANDED_BARTER_CLOSE = 0x142,
    MODE_MSG_OPEN_CASH_SHOP = 0x143,
    MODE_MSG_REBUILD_STATUS_BARS = 0x144,
    MODE_MSG_RESET_SKILL = 0x146,
};

#include "game/mode_callbacks.h"

struct SkillInfo;

static __inline intptr_t self_msg(struct GameMode *self, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    return mode_send_msg(self, msg, a, b, c, d);
}

static __inline void send_raw(int len, const void *packet)
{
    rag_connection_send_packet(rag_connection_instance(), len, packet);
}

#pragma pack(push, 1)
struct PacketU8 { unsigned short id; unsigned char value; };
struct PacketU16 { unsigned short id; unsigned short value; };
struct PacketU32 { unsigned short id; int value; };
struct PacketU32U32 { unsigned short id; int first, second; };
#pragma pack(pop)

static __inline void send_id(unsigned short id)
{
    unsigned short packet = id;
    send_packet(id, &packet);
}

static __inline void send_u8(unsigned short id, int value)
{
    struct PacketU8 packet;
    packet.id = id;
    packet.value = (unsigned char)value;
    send_packet(id, &packet);
}

static __inline void send_u16(unsigned short id, int value)
{
    struct PacketU16 packet;
    packet.id = id;
    packet.value = (unsigned short)value;
    send_packet(id, &packet);
}

static __inline void send_u32(unsigned short id, int value)
{
    struct PacketU32 packet;
    packet.id = id;
    packet.value = value;
    send_packet(id, &packet);
}

static __inline void send_u32_u32(unsigned short id, int first, int second)
{
    struct PacketU32U32 packet;
    packet.id = id;
    packet.first = first;
    packet.second = second;
    send_packet(id, &packet);
}

static __inline char close_window(int id)
{
    return ui_window_mgr_post_quit1(&g_windowMgr, id);
}

static __inline void chat_print(const char *text, unsigned color)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_CHAT_LINE, (intptr_t)text, color, 0, 0);
}

static __inline void chat_print_msg(int id, unsigned color)
{
    chat_print(msg_str(id), color);
}

static __inline void chat_refresh(void)
{
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_REFRESH, 0, 0, 0, 0);
}

static __inline int message_box_msg(int id)
{
    return ui_window_mgr_message_box(&g_windowMgr, msg_str(id), 0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
}

static __inline struct GameActor *mode_actor(struct GameMode *self, int aid)
{
    return world_get_game_actor_by_aid(self->world, aid);
}

struct Session;

struct Language;

int game_mode_msg_npc_sell(struct GameMode *self);
int game_mode_msg_item_list_win_send(struct GameMode *self, int window_type);
int game_mode_msg_item_list_win_cancel(int window_type);
int game_mode_msg_npc_buy(struct GameMode *self);
int game_mode_msg_select_deal_type(struct GameMode *self, int npc_id, int type);
int game_mode_msg_npc_trade_closed(struct GameMode *self);
int game_mode_msg_npc_store_windows_closed(struct GameMode *self);
int game_mode_msg_market_gear_purchase(struct GameMode *self);
int game_mode_msg_market_purchase(struct GameMode *self);
int game_mode_msg_barter_purchase(struct GameMode *self);
int game_mode_msg_expanded_barter_purchase(struct GameMode *self);
int game_mode_msg_buy_cash_item(struct GameMode *self, int points);

int game_mode_msg_cancel_drag(struct GameMode *self);
int game_mode_msg_drop_dragged(struct GameMode *self, int x, int y);
int game_mode_msg_throw_dragged(struct GameMode *self, int count);
int game_mode_msg_throw_item(struct GameMode *self, int index, int count);
int game_mode_msg_wear_equip(struct GameMode *self, int index, int location);
int game_mode_msg_wear_equip_direct(struct GameMode *self, int index, int location);
int game_mode_msg_takeoff_equip(struct GameMode *self, int index, int second_list);
int game_mode_msg_use_item(struct GameMode *self, int index);
int game_mode_msg_chat(struct GameMode *self);
int game_mode_msg_guild_chat(struct GameMode *self);
int game_mode_msg_party_chat(struct GameMode *self);
int game_mode_msg_battlefield_chat(struct GameMode *self);
int game_mode_msg_broadcast(int kind);
int game_mode_msg_whisper(struct GameMode *self, const char *target);
int game_mode_msg_set_square_mark(struct GameMode *self, unsigned aid, int x, int y);
int game_mode_msg_set_triangle_mark(struct GameMode *self, unsigned id, int x, int y);
int game_mode_msg_remove_mark(struct Tree *marks, unsigned id);
int game_mode_msg_clear_marks(struct Tree *marks);
int game_mode_msg_set_unit_cell(struct GameMode *self, unsigned aid, int x, int y);
int game_mode_msg_actor_removed(struct GameMode *self, unsigned aid);
int game_mode_msg_show_equip_of(struct GameMode *self);
int game_mode_msg_set_popup_target_name(struct GameMode *self, const char *name);
int game_mode_msg_popup_choice(struct GameMode *self, unsigned index);
int game_mode_msg_chat_room_msg(struct GameMode *self, int kind, intptr_t arg);
int game_mode_msg_create_chat_room(struct GameMode *self, const struct ChatRoomInfo *room);
int game_mode_msg_change_chat_room(struct GameMode *self, const struct ChatRoomInfo *room);
int game_mode_msg_enter_chat_room(struct GameMode *self, int room_id, const char *password);
int game_mode_msg_chat_room_role(const char *name, int role);
int game_mode_msg_chat_room_expel(const char *name);
int game_mode_msg_exit_chat_room(void);
int game_mode_msg_req_exchange(struct GameMode *self, int aid);
int game_mode_msg_ack_exchange(struct GameMode *self, int answer);
int game_mode_msg_add_exchange_item(struct GameMode *self, int index, int count);
int game_mode_msg_item_to_storage(int index, int count);
int game_mode_msg_item_from_storage(int index, int count);
int game_mode_msg_item_to_cart(struct GameMode *self, int index, int count);
int game_mode_msg_item_from_cart(struct GameMode *self, int index, int count);
int game_mode_msg_storage_to_cart(struct GameMode *self, int index, int count);
int game_mode_msg_cart_to_storage(struct GameMode *self, int index, int count);
int game_mode_msg_close_storage(void);
int game_mode_msg_make_party(const char *name);
int game_mode_msg_set_party_name(const char *name);
int game_mode_msg_make_party_with_options(int item_pickup, int item_share);
int game_mode_msg_party_invite(const char *name);
int game_mode_msg_party_invite_answer(int party_id, int accept);
int game_mode_msg_leave_party(struct GameMode *self);
int game_mode_msg_expel_party_member(const char *name);
int game_mode_msg_party_join_master_ack(int aid, int accept, const int *char_id);
int game_mode_msg_party_req_master_to_join(const int *char_id, const int *party_id);
int game_mode_msg_register_guild_emblem(const void *image, int size);
int game_mode_msg_change_guild_member_pos(void);
int game_mode_msg_change_guild_positions(void);
int game_mode_msg_disorganize_guild(const char *key);
int game_mode_msg_leave_guild(const char *reason);
int game_mode_msg_ban_guild_member(const struct GuildBanTarget *who, const char *reason);
int game_mode_msg_make_guild(const char *name, int b);
int game_mode_msg_guild_invite(struct GameMode *self, int aid);
int game_mode_msg_guild_invite_by_name(const char *name);
int game_mode_msg_guild_notice(int guild_id, const char *subject, const char *notice);
int game_mode_msg_give_manner_point(int aid, int type, int points);
int game_mode_msg_guild_ally(int aid);
int game_mode_msg_open_vending(const char *title, int open);
int game_mode_msg_buy_from_vendor(int aid, int store_id);
int game_mode_msg_browse_vendor(struct GameMode *self, int aid);
int game_mode_msg_click_buying_store(struct GameMode *self, int aid);
int game_mode_msg_open_buying_store(const char *title, int open, int limit);
int game_mode_msg_trade_buying_store(int aid, int store_id);
int game_mode_msg_create_assistant_vending(const char *title);
int game_mode_msg_create_assistant_buying(const char *title, int limit);
int game_mode_msg_skill_aim_level_change(struct GameMode *self, int by);
int game_mode_msg_skill_shout(struct GameMode *self, int skill_id, struct GameActor *actor, int level);
int game_mode_msg_aim_skill(struct GameMode *self, int type, const struct SkillInfo *info, int level);
int game_mode_msg_skill_target(struct GameMode *self, const struct SkillInfo *info, int level, int quiet, int d);
int game_mode_msg_use_skill(struct GameMode *self, int skill_id, int target, int level, int quiet);
int game_mode_msg_use_skill_to_ground(struct GameMode *self, const struct GroundSkillUse *use, int b);
int game_mode_msg_use_skill_to_ground_talkbox(struct GameMode *self, const struct GroundSkillTalkbox *talkbox);

struct RodexMail;
int game_mode_msg_send_rodex(const struct RodexMail *mail);
int game_mode_msg_cancel_write_rodex(void);
int game_mode_msg_rodex_item(int add, int index, int count);
int game_mode_msg_rodex_mail(unsigned short id, int mail_id, int type);
int game_mode_msg_return_mail(int mail_id, const char *name);
int game_mode_msg_open_write_rodex(const char *receiver);
int game_mode_msg_add_swap_equip(struct GameMode *self, int index, unsigned location);
int game_mode_msg_remove_swap_equip(struct GameMode *self, int slot, int swap_set);
int game_mode_msg_do_swap_equip(void);
int game_mode_msg_swap_equip1_on1(const struct ItemInfo *item);
int game_mode_msg_limited_sale_list(void);
int game_mode_msg_toggle_player_gauge(struct GameMode *self);
int game_mode_msg_relayout_player_gauge(struct GameMode *self);
struct NpcClick;
int game_mode_msg_connection_lost(struct GameMode *self);
int game_mode_msg_contact_npc(struct GameMode *self, const struct NpcClick *click);
int game_mode_msg_status_change(struct GameMode *self, int status, int amount);
int game_mode_msg_advanced_status_change(struct GameMode *self, int status, int amount);
int game_mode_msg_req_status(struct GameMode *self);
int game_mode_msg_emotion(struct GameMode *self, int emotion);
int game_mode_msg_request_move(struct GameMode *self, int x, int y);
int game_mode_msg_request_act(struct GameMode *self, int action, int target);
int game_mode_msg_drag_by_name(struct GameMode *self, const char *name);
int game_mode_msg_quake(struct GameMode *self, int strength);
int game_mode_msg_show_illustration(struct GameMode *self, const char *file, int position);
int game_mode_msg_couple_request(const char *name, int which);
int game_mode_msg_pet_act(int action);
int game_mode_msg_command_pet(int command);
int game_mode_msg_rename(unsigned short id, const char *name);
int game_mode_msg_create_item(const char *text);
int game_mode_msg_send_name(unsigned short id, const char *name);
int game_mode_msg_req_status_gm(const char *name);
int game_mode_msg_prison(const char *name);
int game_mode_msg_make_item(int item_id, const struct ItemInfo *stones);
int game_mode_msg_repair_item(const void *item);
int game_mode_msg_add_friend_answer(int aid, int char_id, int answer);
int game_mode_msg_shortcut_key_change(int index, int page);
int game_mode_msg_open_cash_shop(int tab);
int game_mode_msg_scream_line(int which);

struct PickInfo;
void game_mode_open_actor_popup_menu(struct GameMode *self, struct PickInfo *pick, int verdict);

extern int g_skillAimGood;

#endif
