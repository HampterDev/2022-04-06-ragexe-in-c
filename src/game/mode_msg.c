#include "skills/skill_bundle.h"
#include "web/service_managers.h"
#include "contents/sound.h"
#include "session/state.h"
#include <stdlib.h>
#include <string.h>
#include "game/mode_msg.h"
#include "platform/language.h"
#include "game/chat.h"
#include "game/mode_misc.h"
#include "session/session_talk.h"
#include "world/camera.h"

static void send_id_raw(unsigned short id)
{
    send_raw(2, &id);
}

#pragma pack(push, 1)
struct PACKET_CZ_REQUEST_CHAT { unsigned short packet_type; unsigned short packet_length; };
struct PacketU16U8 { unsigned short id; unsigned short value; unsigned char byte; };
struct PacketU32U8 { unsigned short id; int value; unsigned char byte; };
struct PacketName24 { unsigned short id; char name[24]; };
#pragma pack(pop)

int game_mode_send_msg(struct Mode *mode, int msg, int a, int b, int c, int d)
{
    struct GameMode *self = (struct GameMode *)mode;
    switch (msg) {
    case MODE_MSG_CONNECTION_LOST:
        return game_mode_msg_connection_lost(self);
    case MODE_MSG_POINT_MOUSE_AT:
        game_mode_point_mouse_at(self, (float)a, (float)b, c);
        return 0;
    case MODE_MSG_HIDE_MOUSE_POINTER:
        game_mode_mouse_pointer_msg2(self, a);
        return 0;
    case MODE_MSG_CHAT:
        return game_mode_msg_chat(self);
    case MODE_MSG_BROADCAST:
        return game_mode_msg_broadcast(a);
    case MODE_MSG_GET_MAP_NAME:
        return (int)self->map_name;
    case MODE_MSG_CONTACT_NPC:
        return game_mode_msg_contact_npc(self, (const struct NpcClick *)a);
    case CMODE_MSG_WHISPER:
        return game_mode_msg_whisper(self, (const char *)a);
    case MODE_MSG_SET_UNIT_CELL:
        return game_mode_msg_set_unit_cell(self, a, b, c);
    case MODE_MSG_ACTOR_REMOVED:
        return game_mode_msg_actor_removed(self, a);
    case MODE_MSG_DISCONNECT_CHARACTER:
        send_u32(0xcc, a);
        return 0;
    case MODE_MSG_DISCONNECT_ALL:
        send_id(0xce);
        return 0;
    case MODE_MSG_THROW_ITEM:
        return game_mode_msg_throw_item(self, a, b);
    case MODE_MSG_DROP_DRAGGED:
        return game_mode_msg_drop_dragged(self, a, b);
    case MODE_MSG_CANCEL_DRAG:
        return game_mode_msg_cancel_drag(self);
    case MODE_MSG_WEAR_EQUIP:
        return game_mode_msg_wear_equip(self, a, b);
    case MODE_MSG_TAKEOFF_EQUIP:
        return game_mode_msg_takeoff_equip(self, a, b);
    case MODE_MSG_THROW_DRAGGED:
        return game_mode_msg_throw_dragged(self, a);
    case MODE_MSG_NPC_NEXT:
        send_u32(0xb9, a);
        return 0;
    case MODE_MSG_NPC_CHOOSE_MENU: {
        struct PacketU32U8 packet;
        packet.id = 0xb8;
        packet.byte = (unsigned char)a;
        packet.value = b;
        send_packet(0xb8, &packet);
        return 0;
    }
    case MODE_MSG_POPUP_CHOICE:
        return game_mode_msg_popup_choice(self, a);
    case MODE_MSG_RESTART:
        send_u8(0xb2, a);
        return 0;
    case MODE_MSG_SET_ATTACK_IN_PROGRESS:
        self->base.attack_in_progress = a;
        if (a == 0) {
            self->pointed_player_aid = 0;
            self->balloon_follow_aid = 0;
        }
        return 0;
    case MODE_MSG_USE_ITEM:
        return game_mode_msg_use_item(self, a);
    case MODE_MSG_REQ_STATUS:
        return game_mode_msg_req_status(self);
    case MODE_MSG_STATUS_CHANGE:
        return game_mode_msg_status_change(self, a, b);
    case MODE_MSG_EMOTION:
        return game_mode_msg_emotion(self, a);
    case MODE_MSG_USER_COUNT:
        if (g_serviceType == 3 || g_serviceType == 8 || g_serviceType == 9)
            return 0;
        send_id(0xc1);
        return 0;
    case MODE_MSG_EMOTION_ANSWERED:
        self->emotion_pending = 0;
        return 0;
    case MODE_MSG_NPC_BUY:
        return game_mode_msg_npc_buy(self);
    case MODE_MSG_NPC_SELL:
        return game_mode_msg_npc_sell(self);
    case MODE_MSG_NPC_DEAL_SELL:
        return game_mode_msg_select_deal_type(self, a, 1);
    case MODE_MSG_NPC_DEAL_BUY:
        return game_mode_msg_select_deal_type(self, a, 0);
    case MODE_MSG_NPC_TRADE_CLOSED:
        return game_mode_msg_npc_trade_closed(self);
    case MODE_MSG_TOGGLE_STATUS_BARS:
        return game_mode_msg_toggle_player_gauge(self);
    case MODE_MSG_CHAT_ROOM_MSG:
        return game_mode_msg_chat_room_msg(self, a, b);
    case MODE_MSG_CREATE_CHAT_ROOM:
        return game_mode_msg_create_chat_room(self, (const struct ChatRoomInfo *)a);
    case MODE_MSG_CHANGE_CHAT_ROOM:
        return game_mode_msg_change_chat_room(self, (const struct ChatRoomInfo *)a);
    case MODE_MSG_ENTER_CHAT_ROOM:
        return game_mode_msg_enter_chat_room(self, a, (const char *)b);
    case MODE_MSG_CHAT_ROOM_ROLE:
        return game_mode_msg_chat_room_role((const char *)a, b);
    case MODE_MSG_CHAT_ROOM_EXPEL:
        return game_mode_msg_chat_room_expel((const char *)a);
    case MODE_MSG_EXIT_CHAT_ROOM:
        return game_mode_msg_exit_chat_room();
    case MODE_MSG_REQ_EXCHANGE:
        return game_mode_msg_req_exchange(self, a);
    case MODE_MSG_ACK_EXCHANGE:
        return game_mode_msg_ack_exchange(self, a);
    case MODE_MSG_ADD_EXCHANGE_ITEM:
        return game_mode_msg_add_exchange_item(self, a, b);
    case MODE_MSG_CONCLUDE_EXCHANGE:
        send_id(0xeb);
        return 0;
    case MODE_MSG_CANCEL_EXCHANGE:
        send_id(0xed);
        return 0;
    case MODE_MSG_EXEC_EXCHANGE:
        send_id(0xef);
        return 0;
    case MODE_MSG_ITEM_TO_STORAGE:
        return game_mode_msg_item_to_storage(a, b);
    case MODE_MSG_ITEM_FROM_STORAGE:
        return game_mode_msg_item_from_storage(a, b);
    case MODE_MSG_CLOSE_STORAGE:
        return game_mode_msg_close_storage();
    case MODE_MSG_MAKE_PARTY:
        return game_mode_msg_make_party((const char *)a);
    case MODE_MSG_PARTY_INVITE:
        return game_mode_msg_party_invite((const char *)a);
    case MODE_MSG_PARTY_INVITE_ANSWER:
        return game_mode_msg_party_invite_answer(a, b);
    case MODE_MSG_LEAVE_PARTY:
        return game_mode_msg_leave_party(self);
    case MODE_MSG_EXPEL_PARTY_MEMBER:
        return game_mode_msg_expel_party_member((const char *)a);
    case MODE_MSG_SET_SQUARE_MARK:
        return game_mode_msg_set_square_mark(self, a, b, c);
    case MODE_MSG_REMOVE_SQUARE_MARK:
        return game_mode_msg_remove_mark(&self->square_marks, a);
    case MODE_MSG_CLEAR_SQUARE_MARKS:
        return game_mode_msg_clear_marks(&self->square_marks);
    case MODE_MSG_PARTY_CHAT:
        return game_mode_msg_party_chat(self);
    case MODE_MSG_UPGRADE_SKILL:
        self->skill_upgrade_pending = 1;
        send_u16(0x112, a);
        return 0;
    case MODE_MSG_USE_SKILL:
        return game_mode_msg_use_skill(self, a, b, c, d);
    case MODE_MSG_USE_SKILL_TO_GROUND:
        return game_mode_msg_use_skill_to_ground(self, (const struct GroundSkillUse *)a, b);
    case MODE_MSG_CANCEL_SKILL_AIM:
        self->skill_use.type = 0;
        self->skill_use.skill_id = 0;
        self->skill_use.level = 0;
        g_skillAimGood = 0;
        return 0;
    case MODE_MSG_AIM_SKILL:
        return game_mode_msg_aim_skill(self, a, (const struct SkillInfo *)b, c);
    case MODE_MSG_CANCEL_LOCKON:
        send_id(0x118);
        return 0;
    case MODE_MSG_SELECT_WARP_POINT: {
#pragma pack(push, 1)
        struct { unsigned short id; unsigned short skill; char map[16]; } packet;
#pragma pack(pop)
        packet.id = 0x11b;
        strncpy(packet.map, (const char *)a, 0x10);
        packet.skill = (unsigned short)b;
        send_packet(0x11b, &packet);
        return 0;
    }
    case MODE_MSG_REMEMBER_WARP_POINT:
        send_id(0x11d);
        return 0;
    case MODE_MSG_ITEM_TO_CART:
        return game_mode_msg_item_to_cart(self, a, b);
    case MODE_MSG_ITEM_FROM_CART:
        return game_mode_msg_item_from_cart(self, a, b);
    case MODE_MSG_STORAGE_TO_CART:
        return game_mode_msg_storage_to_cart(self, a, b);
    case MODE_MSG_CART_TO_STORAGE:
        return game_mode_msg_cart_to_storage(self, a, b);
    case MODE_MSG_UNINSTALLATION:
        send_u8(0xb35, a);
        return 0;
    case MODE_MSG_CLOSE_VENDING:
        send_id(0x12e);
        return 0;
    case MODE_MSG_OPEN_VENDING:
        return game_mode_msg_open_vending((const char *)a, b);
    case MODE_MSG_BUY_FROM_VENDOR:
        return game_mode_msg_buy_from_vendor(a, b);
    case MODE_MSG_BROWSE_VENDOR:
        return game_mode_msg_browse_vendor(self, a);
    case MODE_MSG_SKILL_SHOUT:
        return game_mode_msg_skill_shout(self, a, (struct GameActor *)b, c);
    case MODE_MSG_WEAR_EQUIP_DIRECT:
        return game_mode_msg_wear_equip_direct(self, a, b);
    case MODE_MSG_QUIT_GAME:
        self_msg(self, MODE_MSG_REQ_DISCONNECT, 0, 0, 0, 0);
        return 0;
    case MODE_MSG_CLOSE_NPC_DIALOG:
        self->npc_dialog_open = 0;
        close_window(0x10);
        close_window(0x11);
        send_u32(0x146, a);
        return 0;
    case MODE_MSG_RESUME_BGM:
        if (g_sound_settings.bgm_on)
            sound_play_stream(0, 0);
        else
            sound_stop_stream();
        return 0;
    case MODE_MSG_CREATE_ITEM:
        return game_mode_msg_create_item((const char *)a);
    case MODE_MSG_MOVE_TO_MAP: {
#pragma pack(push, 1)
        struct { unsigned short id; char map[16]; unsigned short x, y; } packet;
#pragma pack(pop)
        packet.id = 0x140;
        strncpy(packet.map, (const char *)a, 0x10);
        packet.x = (unsigned short)b;
        packet.y = (unsigned short)c;
        send_packet(0x140, &packet);
        return 0;
    }
    case MODE_MSG_SKILL_AIM_LEVEL_UP:
        return game_mode_msg_skill_aim_level_change(self, a);
    case MODE_MSG_SKILL_AIM_LEVEL_DOWN:
        return game_mode_msg_skill_aim_level_change(self, -a);
    case MODE_MSG_GET_SKILL_AIM_TYPE:
        return self->skill_use.type;
    case MODE_MSG_PARTY_EXP_OPTION:
        send_u32(0x102, a);
        return 0;
    case MODE_MSG_GET_SKILL_AIM_LEVEL:
        return self->skill_use.level;
    case MODE_MSG_EDIT_DIALOG_INPUT:
        if (!mode_actor(self, a))
            return 0;
        send_u32_u32(0x143, a, b);
        return 0;
    case MODE_MSG_SHOW_ILLUSTRATION:
        return game_mode_msg_show_illustration(self, (const char *)a, b);
    case MODE_MSG_SET_POPUP_TARGET_NAME:
        return game_mode_msg_set_popup_target_name(self, (const char *)a);
    case MODE_MSG_REGISTER_GUILD_EMBLEM:
        return game_mode_msg_register_guild_emblem((const void *)a, b);
    case MODE_MSG_CHANGE_GUILD_MEMBER_POS:
        return game_mode_msg_change_guild_member_pos();
    case MODE_MSG_CHANGE_GUILD_POSITIONS:
        return game_mode_msg_change_guild_positions();
    case MODE_MSG_GUILD_MENU_INTERFACE:
        send_id(0x14d);
        return 0;
    case MODE_MSG_DISORGANIZE_GUILD:
        return game_mode_msg_disorganize_guild((const char *)a);
    case MODE_MSG_GUILD_MENU:
        send_u32(0x14f, a);
        return 0;
    case MODE_MSG_OPEN_GUILD_MEMBER_INFO:
        send_u32(0x157, a);
        return 0;
    case MODE_MSG_LEAVE_GUILD:
        return game_mode_msg_leave_guild((const char *)a);
    case MODE_MSG_BAN_GUILD_MEMBER:
        return game_mode_msg_ban_guild_member((const struct GuildBanTarget *)a, (const char *)b);
    case MODE_MSG_MAKE_GUILD:
        return game_mode_msg_make_guild((const char *)a, b);
    case MODE_MSG_SKILL_TARGET:
        return game_mode_msg_skill_target(self, (const struct SkillInfo *)a, b, c, d);
    case MODE_MSG_JOIN_GUILD_ANSWER:
        send_u32_u32(0x16b, a, b);
        return 0;
    case MODE_MSG_GUILD_INVITE:
        return game_mode_msg_guild_invite(self, a);
    case MODE_MSG_GIVE_MANNER_POINT: {
        struct PACKET_CZ_REQUEST_CHAT packet = { 0xf3, 2 };
        send_raw(4, &packet);
        return 0;
    }
    case MODE_MSG_GIVE_MANNER_POINT_BY_AID:
        return game_mode_msg_give_manner_point(a, b, c);
    case MODE_MSG_GUILD_NOTICE:
        return game_mode_msg_guild_notice(a, (const char *)b, (const char *)c);
    case MODE_MSG_GUILD_ALLY:
        return game_mode_msg_guild_ally(a);
    case MODE_MSG_GUILD_ALLY_ANSWER:
        send_u32_u32(0x172, a, b);
        return 0;
    case MODE_MSG_IDENTIFY_ITEM:
        send_u16(0x178, a);
        return 0;
    case MODE_MSG_QUAKE:
        return game_mode_msg_quake(self, a);
    case MODE_MSG_ITEM_COMPOSITION_LIST:
        self->composition_card_index = a;
        send_u16(0x17a, a);
        return 0;
    case MODE_MSG_ITEM_COMPOSITION: {
#pragma pack(push, 1)
        struct { unsigned short id; unsigned short first, second; } packet;
#pragma pack(pop)
        packet.id = 0x17c;
        packet.second = (unsigned short)a;
        packet.first = (unsigned short)b;
        send_packet(0x17c, &packet);
        return 0;
    }
    case MODE_MSG_GUILD_HOSTILE:
        send_u32(0x180, a);
        return 0;
    case MODE_MSG_DELETE_RELATED_GUILD:
        send_u32_u32(0x183, a, b);
        return 0;
    case MODE_MSG_REQ_DISCONNECT:
        send_u16(0x18a, 0);
        return 0;
    case MODE_MSG_GUILD_CHAT:
        return game_mode_msg_guild_chat(self);
    case MODE_MSG_MAKE_ITEM:
        return game_mode_msg_make_item(a, (const struct ItemInfo *)b);
    case MODE_MSG_USE_SKILL_TO_GROUND_TALKBOX:
        return game_mode_msg_use_skill_to_ground_talkbox(self, (const struct GroundSkillTalkbox *)a);
    case MODE_MSG_DRAG_BY_NAME:
        return game_mode_msg_drag_by_name(self, (const char *)a);
    case MODE_MSG_SET_VIEW:
        view_set_quake_amplitude(self->view, *(const float *)a, *(const float *)b, *(const float *)c);
        return 0;
    case MODE_MSG_PICKUP_ITEM:
        send_u32(0x362, a);
        return 0;
    case MODE_MSG_REQUEST_ACT:
        return game_mode_msg_request_act(self, a, b);
    case MODE_MSG_REQUEST_MOVE:
        return game_mode_msg_request_move(self, a, b);
    case MODE_MSG_CHANGE_DIRECTION: {
        struct PacketU16U8 packet;
        packet.id = 0x361;
        packet.byte = (unsigned char)a;
        packet.value = (unsigned short)b;
        send_packet(0x361, &packet);
        return 0;
    }
    case MODE_MSG_COUPLE_REQUEST:
        return game_mode_msg_couple_request((const char *)a, b);
    case MODE_MSG_GO_TO_LOGIN:
        return game_mode_msg_go_to_login();
    case MODE_MSG_RESET_STATUS: {
        struct PacketU16 packet;
        packet.id = 0x197;
        if (a == 1) {
            packet.value = 1;
            player_skill_bundle_zero_first_level_before(&g_session1.skills);
        } else {
            packet.value = 0;
        }
        send_packet(packet.id, &packet);
        return 0;
    }
    case MODE_MSG_CHANGE_MAP_TYPE: {
#pragma pack(push, 1)
        struct { unsigned short id; unsigned short x, y, type; } packet;
#pragma pack(pop)
        packet.id = 0x198;
        packet.x = (unsigned short)a;
        packet.y = (unsigned short)b;
        packet.type = c != 0;
        send_packet(0x198, &packet);
        return 0;
    }
    case MODE_MSG_CHANGE_EFFECT_STATE:
        send_u32(0x19d, a);
        return 0;
    case MODE_MSG_RENAME_PET:
        return game_mode_msg_rename(0x1a5, (const char *)a);
    case MODE_MSG_SELECT_PET_EGG:
        send_u16(0x1a7, a);
        return 0;
    case MODE_MSG_PET_EGG_INFO:
        send_u16(0x1a8, a);
        return 0;
    case MODE_MSG_PET_ACT:
        return game_mode_msg_pet_act(a);
    case MODE_MSG_CAPTURE_MONSTER:
        send_u32(0x19f, a);
        return 0;
    case MODE_MSG_COMMAND_PET:
        return game_mode_msg_command_pet(a, c);
    case MODE_MSG_PROHIBIT_REMOVE:
        self_msg(self, MODE_MSG_GIVE_MANNER_POINT_BY_AID, self->popup_target_aid, 0, a, 0);
        session_erase_prohibit(&g_session1, self->popup_target_aid, a);
        return 0;
    case MODE_MSG_PROHIBIT_ADD:
        self_msg(self, MODE_MSG_GIVE_MANNER_POINT_BY_AID, self->popup_target_aid, 1, a, 0);
        session_append_prohibit(&g_session1, self->popup_target_aid, a);
        return 0;
    case MODE_MSG_MAKE_ARROW:
        send_u32(0x1ae, a);
        return 0;
    case MODE_MSG_CHANGE_CART:
        if (b) {
            struct PacketU32U8 packet;
            packet.byte = (unsigned char)a;
            packet.id = 0x980;
            packet.value = b;
            send_packet(0x980, &packet);
        } else {
            send_u16(0x1af, a);
        }
        return 0;
    case MODE_MSG_REMOVE_AID:
        send_u32(0x843, atoi((const char *)a));
        return 0;
    case MODE_MSG_SHIFT:
        return game_mode_msg_send_name(0x1bb, (const char *)a);
    case MODE_MSG_RECALL_AID:
        send_u32(0x842, atoi((const char *)a));
        return 0;
    case MODE_MSG_RECALL_NAME:
        return game_mode_msg_send_name(0x1bd, (const char *)a);
    case MODE_MSG_REQ_REMAIN_TIME:
        send_id(0x1c0);
        return 0;
    case MODE_MSG_SELECT_AUTOSPELL:
        send_u32(0x1ce, a);
        return 0;
    case MODE_MSG_NPC_INPUT_TEXT:
        game_mode_send_npc_input_text(self, a, (const char *)b, (const char *)c);
        return 0;
    case MODE_MSG_SKILL_SCREAM_LINE:
        return game_mode_msg_scream_line(a);
    case MODE_MSG_JOIN_COUPLE:
    case MODE_MSG_JOIN_BABY: {
#pragma pack(push, 1)
        struct { unsigned short id; unsigned aid, char_id; int answer; } packet;
#pragma pack(pop)
        packet.id = msg == MODE_MSG_JOIN_COUPLE ? 0x1e3 : 0x1f7;
        packet.char_id = self->family_request_char_id;
        packet.aid = self->family_request_aid;
        packet.answer = b;
        send_packet(packet.id, &packet);
        return 0;
    }
    case MODE_MSG_MAKE_PARTY_WITH_OPTIONS:
        return game_mode_msg_make_party_with_options(b, c);
    case MODE_MSG_DORIDORI:
        send_id(0x1e7);
        return 0;
    case MODE_MSG_TRIANGLE_MARK_SET:
        return game_mode_msg_set_triangle_mark(self, a, b, c);
    case MODE_MSG_TRIANGLE_MARK_REMOVE:
        return game_mode_msg_remove_mark(&self->triangle_marks, a);
    case MODE_MSG_TRIANGLE_MARKS_CLEAR:
        return game_mode_msg_clear_marks(&self->triangle_marks);
    case MODE_MSG_REPAIR_ITEM:
        return game_mode_msg_repair_item((const void *)a);
    case MODE_MSG_ADD_FRIEND_ANSWER:
        return game_mode_msg_add_friend_answer(a, b, c);
    case MODE_MSG_DELETE_FRIEND:
        send_u32_u32(0x203, a, b);
        return 0;
    case MODE_MSG_REQ_STATUS_GM:
        return game_mode_msg_req_status_gm((const char *)a);
    case MODE_MSG_GIVE_MANNER_BY_NAME: {
        struct PacketName24 packet;
        memset(packet.name, 0, sizeof packet.name);
        packet.id = 0x212;
        memcpy(packet.name, (const char *)a, 24);
        send_packet(0x212, &packet);
        return 0;
    }
    case MODE_MSG_REQ_JOIN_BABY:
        send_u32(0x1f9, a);
        return 0;
    case MODE_MSG_WEAPON_REFINE:
        send_u32(0x222, a);
        return 0;
    case MODE_MSG_COMMAND_MERCENARY: {
#pragma pack(push, 1)
        struct { unsigned short id; unsigned short zero; unsigned char command; } packet;
#pragma pack(pop)
        packet.id = 0x22d;
        packet.zero = 0;
        packet.command = (unsigned char)a;
        send_packet(0x22d, &packet);
        return 0;
    }
    case MODE_MSG_RENAME_MERCENARY:
        return game_mode_msg_rename(0x231, (const char *)a);
    case MODE_MSG_MERCENARY_COMMAND:
        send_u8(0x29f, a);
        return 0;
    case MODE_MSG_SHOW_EQUIP_OF:
        return game_mode_msg_show_equip_of(self);
    case MODE_MSG_AGREE_STARPLACE:
        send_u8(0x254, a);
        return 0;
    case MODE_MSG_MAIL_GET_LIST:
        send_id(0x23f);
        return 0;
    case MODE_MSG_SEND_RODEX:
        return game_mode_msg_send_rodex((const struct RodexMail *)a);
    case MODE_MSG_MAIL_RESET_ITEM:
        send_u16(0x246, a);
        return 0;
    case MODE_MSG_READ_RODEX:
        return game_mode_msg_rodex_mail(0x9ea, a, b);
    case MODE_MSG_ADD_ITEM_RODEX:
        return game_mode_msg_rodex_item(1, a, b);
    case MODE_MSG_DELETE_RODEX:
        return game_mode_msg_rodex_mail(0x9f5, a, b);
    case MODE_MSG_RETURN_MAIL:
        return game_mode_msg_return_mail(a, (const char *)b);
    case MODE_MSG_MAIL_GET_ITEM:
        send_u32(0x244, a);
        return 0;
    case MODE_MSG_MAKING_ITEM: {
#pragma pack(push, 1)
        struct { unsigned short id; unsigned short kind; int item; } packet;
#pragma pack(pop)
        packet.id = 0x25b;
        packet.item = a;
        packet.kind = (unsigned short)b;
        send_packet(0x25b, &packet);
        return 0;
    }
    case MODE_MSG_NPC_STORE_WINDOWS_CLOSED:
        return game_mode_msg_npc_store_windows_closed(self);
    case MODE_MSG_HUNTING_LIST:
        send_id(0x279);
        return 0;
    case MODE_MSG_BUY_CASH_ITEM:
        return game_mode_msg_buy_cash_item(self, a);
    case MODE_MSG_STANDING_RESURRECTION:
        send_id(0x292);
        return 0;
    case MODE_MSG_CONFIG: {
#pragma pack(push, 1)
        struct { unsigned short id; int option; int value; } packet;
#pragma pack(pop)
        packet.id = 0x2d8;
        packet.option = 0;
        packet.value = a;
        send_packet(0x2d8, &packet);
        return 0;
    }
    case MODE_MSG_BATTLEFIELD_CHAT:
        return game_mode_msg_battlefield_chat(self);
    case MODE_MSG_SKILL_SELECT_RESPONSE: {
#pragma pack(push, 1)
        struct { unsigned short id; int skill; unsigned short why; } packet;
#pragma pack(pop)
        packet.id = 0x443;
        packet.why = (unsigned short)a;
        packet.skill = b;
        send_packet(0x443, &packet);
        return 0;
    }
    case MODE_MSG_GROUP_INFO_CHANGE: {
#pragma pack(push, 1)
        struct { unsigned short id; int exp; unsigned char pickup, share; } packet;
#pragma pack(pop)
        packet.share = (unsigned char)c;
        packet.id = 0x7d7;
        packet.exp = a;
        packet.pickup = (unsigned char)b;
        send_packet(0x7d7, &packet);
        return 0;
    }
    case MODE_MSG_CHANGE_GROUP_MASTER:
        adventurer_agency_cancel_own_booking(g_adventurerAgency);
        send_u32(0x7da, a);
        return 0;
    case MODE_MSG_OPEN_SEEK_PARTY_MB_WND:
        ui_window_open(0xa8);
        return 0;
    case MODE_MSG_NPC_MARKET_PURCHASE:
        return game_mode_msg_market_purchase(self);
    case MODE_MSG_NPC_MARKETGEAR_PURCHASE:
        return game_mode_msg_market_gear_purchase(self);
    case MODE_MSG_PARTY_JOIN_MASTER_ACK:
        return game_mode_msg_party_join_master_ack(a, b, (const int *)c);
    case MODE_MSG_PARTY_REQ_MASTER_TO_JOIN:
        return game_mode_msg_party_req_master_to_join((const int *)a, (const int *)b);
    case MODE_MSG_ITEMLISTWIN_SEND:
        return game_mode_msg_item_list_win_send(self, a);
    case MODE_MSG_ITEMLISTWIN_CANCEL:
        return game_mode_msg_item_list_win_cancel(a);
    case MODE_MSG_OPEN_WRITE_RODEX:
        return game_mode_msg_open_write_rodex((const char *)a);
    case MODE_MSG_OPEN_SEEK_PARTY_LIST_WND:
        ui_window_open(0xa9);
        return 0;
    case MODE_MSG_CLOSE_BUYING_STORE:
        send_id(0x815);
        return 0;
    case MODE_MSG_OPEN_BUYING_STORE:
        return game_mode_msg_open_buying_store((const char *)a, b, c);
    case MODE_MSG_CLICK_BUYING_STORE:
        return game_mode_msg_click_buying_store(self, a);
    case MODE_MSG_TRADE_BUYING_STORE:
        return game_mode_msg_trade_buying_store(a, b);
    case MODE_MSG_REPLAY_START:
        return game_mode_msg_replay_start(self);
    case MODE_MSG_REPLAY_STOP:
        return game_mode_msg_replay_stop();
    case MODE_MSG_REPLAY_PAUSE:
        return game_mode_msg_replay_pause(self, 1);
    case MODE_MSG_REPLAY_RESUME:
        return game_mode_msg_replay_pause(self, 0);
    case MODE_MSG_REPLAY_TO_LOGIN:
        return game_mode_msg_replay_to_login(0x19, 6);
    case MODE_MSG_REPLAY_SPEED_UP:
        return game_mode_msg_replay_speed(1);
    case MODE_MSG_REPLAY_SLOW_DOWN:
        return game_mode_msg_replay_speed(0);
    case MODE_MSG_REPLAY_END:
        return game_mode_msg_replay_to_login(0x1a, 1);
    case MODE_MSG_TRY_COLLECTION:
        send_u32(0x8b5, a);
        return 0;
    case MODE_MSG_SET_PARTY_NAME_WND:
        return game_mode_msg_set_party_name((const char *)a);
    case MODE_MSG_GUILD_INVITE_BY_NAME:
        return game_mode_msg_guild_invite_by_name((const char *)a);
    case MODE_MSG_REQ_RANKING:
        send_u16(0x97c, a);
        return 0;
    case MODE_MSG_CLAN_CHAT:
        return game_mode_msg_clan_chat(self);
    case MODE_MSG_NPC_TRADE_QUIT:
        send_id_raw(0x9d4);
        return 0;
    case MODE_MSG_NPC_MARKET_CLOSE:
        send_id_raw(0x9d8);
        return 0;
    case MODE_MSG_CANCEL_WRITE_RODEX:
        return game_mode_msg_cancel_write_rodex();
    case MODE_MSG_REMOVE_RODEX_ITEM:
        return game_mode_msg_rodex_item(0, a, b);
    case MODE_MSG_CREATE_ASSISTANT_VENDING:
        return game_mode_msg_create_assistant_vending((const char *)a);
    case MODE_MSG_CANCEL_ASSISTANT_VENDING:
        send_id(0xa8c);
        return 0;
    case MODE_MSG_CREATE_ASSISTANT_BUYING:
        return game_mode_msg_create_assistant_buying((const char *)a, b);
    case MODE_MSG_CANCEL_ASSISTANT_BUYING:
        send_id(0xa94);
        return 0;
    case MODE_MSG_DESTROY_STORE_ASSISTANT:
        send_u32(0xa80, a);
        return 0;
    case MODE_MSG_ADD_SWAP_EQUIP:
        return game_mode_msg_add_swap_equip(self, a, b);
    case MODE_MSG_REMOVE_SWAP_EQUIP:
        return game_mode_msg_remove_swap_equip(self, a, b);
    case MODE_MSG_DO_SWAP_EQUIP:
        return game_mode_msg_do_swap_equip();
    case MODE_MSG_SWAP_EQUIP_1_ON_1:
        return game_mode_msg_swap_equip1_on1((const struct ItemInfo *)a);
    case MODE_MSG_MEMORIAL_DUNGEON_COMMAND:
        send_u32(0x2cf, 3);
        return 0;
    case MODE_MSG_NPC_BARTER_PURCHASE:
        return game_mode_msg_barter_purchase(self);
    case MODE_MSG_NPC_BARTER_CLOSE:
        send_id_raw(0xb12);
        return 0;
    case MODE_MSG_OPEN_EXTEND_BODYITEM:
        send_id(0xb14);
        return 0;
    case MODE_MSG_EXTEND_BODYITEM:
        send_id(0xb16);
        return 0;
    case MODE_MSG_CLOSE_EXTEND_BODYITEM:
        send_id(0xb19);
        return 0;
    case MODE_MSG_SHORTCUT_KEY_CHANGE:
        return game_mode_msg_shortcut_key_change(a, b);
    case MODE_MSG_ADVANCED_STATUS_CHANGE:
        return game_mode_msg_advanced_status_change(self, a, b);
    case MODE_MSG_MOVE_GUILD_AGIT:
        if (a == 0)
            return 0;
        send_u8(0xb28, a);
        return 0;
    case MODE_MSG_AGIT_INVESTMENT:
        if (a == 0)
            return 0;
        send_u8(0xb2c, a);
        return 0;
    case MODE_MSG_PRISON:
        return game_mode_msg_prison((const char *)a);
    case MODE_MSG_OPEN_LIMITED_SALE_TOOL:
        send_id(0xb4f);
        return 0;
    case MODE_MSG_CLOSE_LIMITED_SALE_TOOL:
        send_id(0xb51);
        return 0;
    case MODE_MSG_LIMITED_SALE_LIST:
        return game_mode_msg_limited_sale_list();
    case MODE_MSG_NPC_EXPANDED_BARTER_PURCHASE:
        return game_mode_msg_expanded_barter_purchase(self);
    case MODE_MSG_NPC_EXPANDED_BARTER_CLOSE:
        send_id_raw(0xb58);
        return 0;
    case MODE_MSG_OPEN_CASH_SHOP:
        return game_mode_msg_open_cash_shop(a);
    case MODE_MSG_REBUILD_STATUS_BARS:
        return game_mode_msg_relayout_player_gauge(self);
    case MODE_MSG_USE_ITEM_WITH_TARGET:
        return game_mode_msg_use_item_with_target(self, a, b, c);
    case MODE_MSG_RESET_SKILL: {
        struct PacketU8 packet;
        packet.id = 0xbb1;
        packet.value = (unsigned char)a;
        player_skill_bundle_zero_first_level_before(&g_session1.skills);
        send_packet(packet.id, &packet);
        return 0;
    }

    case 0x24: case 0x55: case 0x62: case 0x9f: case 0xa0: case 0xa6: case 0x127:
        return 0;
    default:
        return mode_send_msg_base(mode, msg, a, b, c, 0);
    }
}
