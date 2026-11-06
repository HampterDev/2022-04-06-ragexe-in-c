#include "game/drag.h"
#include "session/character_name.h"
#include "session/state.h"
#include "game/scream_text.h"
#include <stdio.h>
#include "core/memory.h"
#include "ui/window_access.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "game/mode_msg.h"
#include "platform/language.h"
#include "core/text_util.h"
#include "game/chat.h"
#include "game/debug.h"
#include "game/packets_world.h"
#include "platform/clientinfo.h"
#include "session/session_items.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "session/session_talk.h"
#include "ui/ui_std.h"
#include "ui/wndmgr/wndmgr.h"
#include "world/camera.h"

#define CZ_CONTACTNPC               0x90
#define CZ_STATUS_CHANGE            0xbb
#define CZ_REQ_EMOTION              0xbf
#define CZ_REQUEST_CHAT             0xf3
#define CZ_COMMAND_PET              0x1a1
#define CZ_PET_ACT                  0x1a9
#define CZ_REQ_STATUS_GM            0x213
#define CZ_ACK_REQ_ADD_FRIENDS      0x208
#define CZ_REQMAKINGITEM            0x18e
#define CZ_REQ_ITEMREPAIR2          0xb66
#define CZ_ADVANCED_STATUS_CHANGE   0xb24
#define CZ_REQUEST_ACT2             0x437
#define CZ_REQUEST_MOVE2            0x35f
#define CZ_ITEM_CREATE_EX           0x9ce
#define CZ_SHORTCUTKEYBAR_ROTATE    0xb21
#define CZ_PRISON                   0xb34
#define CZ_CASH_SHOP_OPEN           0xb6d

#define EMOTION_WAIT 2500
#define CHAT_COLOR_GM_STATUS 0x6efa9b
#define CHAT_COLOR_PET_FOOD 0x1e1ef5
#define CHAT_COLOR_NOTICE 0xffff00
#define NPC_CLICK_WARP_SERVER 0x2d


int game_mode_msg_connection_lost(struct GameMode *self)
{
    self->server_move_pending = 1;
    self->server_move_time = timeGetTime();
    connection_disconnect(rag_connection_transport(rag_connection_instance()));
    if (ui_window_mgr_message_box(&g_windowMgr, msg_str(2), 0, 1, 1, 0, "메시지", 0x118, 0x78, 0) == 0x105)
        return 0;
    self_msg(self, MODE_MSG_GO_TO_LOGIN, 0, 0, 0, 0);
    return 0;
}

struct NpcClick {
    int npc_id;
    int type;
    unsigned char kind;
};

int game_mode_msg_contact_npc(struct GameMode *self, const struct NpcClick *click)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int npc_id;
        unsigned char kind;
    } packet;
#pragma pack(pop)
    struct Player *player = self->world->player;

    if (player)
        render_object_send(player, NULL, 8, 0, 0, 0, 0);
    if (click->type == NPC_CLICK_WARP_SERVER) {
        if (self->server_move_pending == 1)
            return 0;
        self->server_move_pending = 1;
        self->server_move_time = timeGetTime();
    }
    packet.id = CZ_CONTACTNPC;
    packet.npc_id = click->npc_id;
    packet.kind = click->kind;
    send_packet(CZ_CONTACTNPC, &packet);
    return 0;
}

int game_mode_msg_status_change(struct GameMode *self, int status, int amount)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short status;
        unsigned char amount;
    } packet;
#pragma pack(pop)

    if (self->status_request_pending)
        return 0;
    self->status_request_pending = 1;
    packet.id = CZ_STATUS_CHANGE;
    packet.status = (unsigned short)status;
    packet.amount = (unsigned char)amount;
    if (self->player_dead)
        return 0;
    send_packet(CZ_STATUS_CHANGE, &packet);
    return 0;
}

int game_mode_msg_advanced_status_change(struct GameMode *self, int status, int amount)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short status;
        unsigned short amount;
    } packet;
#pragma pack(pop)

    if (self->status_request_pending)
        return 0;
    packet.status = (unsigned short)status;
    packet.id = CZ_ADVANCED_STATUS_CHANGE;
    packet.amount = (unsigned short)amount;
    if (self->player_dead)
        return 0;
    self->status_request_pending = 1;
    send_packet(CZ_ADVANCED_STATUS_CHANGE, &packet);
    return 0;
}

int game_mode_msg_req_status(struct GameMode *self)
{
    if (self->status_request_pending)
        return 0;
    self->status_request_pending = 1;
    if (self->player_dead)
        return 0;
    send_id(0xba);
    return 0;
}

int game_mode_msg_emotion(struct GameMode *self, int emotion)
{
    DWORD now;

    if (emotion >= 0x5d || self->emotion_pending)
        return 0;
    now = timeGetTime();
    if ((DWORD)(now - self->emotion_time) <= EMOTION_WAIT)
        return 0;
    self->emotion_time = now;
    self->emotion_pending = 1;
    send_u8(CZ_REQ_EMOTION, emotion);
    return 0;
}

int game_mode_msg_request_move(struct GameMode *self, int x, int y)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned char cell[3];
    } packet;
#pragma pack(pop)

    if (game_mode_is_blocking_window_open() == 1)
        return 0;
    packet.id = CZ_REQUEST_MOVE2;
    packet.cell[0] = (unsigned char)(x >> 2);
    packet.cell[1] = (unsigned char)(((y >> 4) & 0x3f) | (x << 6));
    packet.cell[2] = (unsigned char)(y << 4);
    send_packet(CZ_REQUEST_MOVE2, &packet);
    return 0;
}

int game_mode_msg_request_act(struct GameMode *self, int action, int target)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int target;
        unsigned char action;
    } packet;
#pragma pack(pop)

    if (self->world->player->mounted && (action == 7 || action == 0)) {
        chat_print_msg(0x75d, CHAT_COLOR_RED);
        return 0;
    }
    packet.action = (unsigned char)action;
    packet.id = CZ_REQUEST_ACT2;
    packet.target = target;
    send_packet(CZ_REQUEST_ACT2, &packet);
    return 0;
}

int game_mode_msg_drag_by_name(struct GameMode *self, const char *name)
{
    game_mode_reset_drag(self);
    self->drag.source = 0xe;
    string_assign_cstr(&self->drag.name, name);
    string_assign_cstr(&self->drag.display_name, session_get_item_display_name(name, 1));
    return 0;
}

int game_mode_msg_quake(struct GameMode *self, int strength)
{
    if (!g_session1.screenShake)
        return 0;
    if (strength == 1)
        view_set_quake(self->view, 1, 1, 1.0f, 0.24f, 0.2f);
    else if (strength < 0x28a)
        view_set_quake(self->view, 1, 0, 1.0f, 0.24f, 0.2f);
    else
        view_set_quake(self->view, 1, strength, 1.0f, 0.24f, 0.2f);
    return 0;
}

int game_mode_msg_show_illustration(struct GameMode *self, const char *file, int position)
{
    struct String name;

    string_init(&name);
    string_assign_cstr(&name, file);
    self->illust_position = position;
    string_assign(&self->illust_file, string_cstr(&name), name.size);
    text_to_lower(&self->illust_file);
    if (string_find(&self->illust_file, ".bmp", 0) == STRING_NPOS
            && string_find(&self->illust_file, ".png", 0) == STRING_NPOS)
        string_append_cstr(&self->illust_file, ".bmp");
    game_mode_update_cut_in(self);
    string_free(&name);
    return 0;
}

int game_mode_msg_guild_member_status(const char *name, int which)
{
    char line[0x800];
    int id;

    if (g_session1.showLoginNotices != 1)
        return 0;
    if (which == 0)
        id = 0x1e6;
    else if (which == 1)
        id = 0x1e5;
    else
        return 0;
    sprintf(line, msg_str(id), name);
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)line, CHAT_COLOR_NOTICE, 4, 0);
    return 0;
}

int game_mode_msg_pet_act(int action)
{
    if (action == -1)
        return 0;
    if (action % 10 < 2 && g_session1.petIntimacy / 10 < 90 && !session_is_admin_option(&g_session1, 3))
        return 0;
    send_u32(CZ_PET_ACT, action);
    return 0;
}

int game_mode_msg_command_pet(int command, int feeding)
{
    struct ItemInfo item, food;
    struct String line;
    const char *name;
    int food_id, count;

    if (feeding == 1) {
        food_id = session_get_pet_food(&g_session1, g_session1.petJob);
        count = session_get_inventory_item_by_id(&g_session1, &item, food_id, 0)->count;
        item_info_destruct(&item);
        if (count <= 0) {
            item_info_construct(&food);
            item_info_set_item_id(&food, food_id);

            name = item_info_get_name(&food);
            string_init(&line);
            string_format(&line, msg_str(0x24f), name);
            ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)string_cstr(&line), CHAT_COLOR_PET_FOOD, 0, 0);
            ui_window_mgr_send_msg_(&g_windowMgr, 0x13, (int)string_cstr(&line), CHAT_COLOR_PET_FOOD, 0, 0);
            string_free(&line);
            item_info_destruct(&food);
            return 0;
        }
    }
    send_u8(CZ_COMMAND_PET, command);
    return 0;
}

int game_mode_msg_rename(unsigned short id, const char *name)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        char name[24];
    } packet;
#pragma pack(pop)
    size_t len = strlen(name);

    packet.id = id;
    if ((int)len < 0x18) {
        memcpy(packet.name, name, len + 1);
    } else {
        memcpy(packet.name, name, 24);
        packet.name[23] = 0;
    }
    send_packet(packet.id, &packet);
    return 0;
}

int game_mode_msg_create_item(const char *text)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        char text[0x64];
    } packet;
#pragma pack(pop)

    if (!text)
        return 0;
    if (_stricmp(text, "divide by zero") == 0)
        debug_divide_by_zero();
    memset(packet.text, 0, sizeof packet.text);
    packet.id = CZ_ITEM_CREATE_EX;
    _snprintf_s(packet.text, sizeof packet.text, (size_t)-1, "%s", text);
    send_raw(sizeof packet, &packet);
    return 0;
}

int game_mode_msg_send_name(unsigned short id, const char *name)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        char name[24];
    } packet;
#pragma pack(pop)

    packet.id = id;
    strncpy(packet.name, name, sizeof packet.name);
    send_packet(id, &packet);
    return 0;
}

int game_mode_msg_req_status_gm(const char *name)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        char name[24];
    } packet;
#pragma pack(pop)
    char line[0x1000];

    packet.id = CZ_REQ_STATUS_GM;
    strncpy(packet.name, name, sizeof packet.name);
    send_packet(CZ_REQ_STATUS_GM, &packet);
    sprintf(line, "[ %.24s ]", packet.name);
    chat_print(line, CHAT_COLOR_GM_STATUS);
    return 0;
}

int game_mode_msg_prison(const char *name)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        char gm[24];
        char target[24];
    } packet;
#pragma pack(pop)

    if (!clientinfo_is_admin_aid(g_session1.ownAid) && !clientinfo_is_yellow_aid(g_session1.ownAid))
        return 0;
    if (strnlen(name, 0x18) - 1 > 0x16)
        return 0;
    packet.id = CZ_PRISON;
    memset(packet.gm, 0, sizeof packet.gm);
    memset(packet.target, 0, sizeof packet.target);
    strncpy(packet.gm, session_character_name(&g_session1), 0x18);
    strncpy(packet.target, name, 0x18);
    send_packet(packet.id, &packet);
    return 0;
}

int game_mode_msg_make_item(int item_id, const struct ItemInfo *stones)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int item_id;
        int material[3];
    } packet;
#pragma pack(pop)

    packet.id = CZ_REQMAKINGITEM;
    packet.item_id = item_id;
    if (item_id == 0) {
        packet.material[2] = 0;
        packet.material[1] = 0;
        packet.material[0] = 0;
    } else {
        packet.material[0] = atoi(string_cstr(&stones[0].id_text));
        packet.material[1] = atoi(string_cstr(&stones[1].id_text));
        packet.material[2] = atoi(string_cstr(&stones[2].id_text));
    }
    send_packet(CZ_REQMAKINGITEM, &packet);
    return 0;
}

int game_mode_msg_repair_item(const void *item)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        union {
            unsigned char item[24];
            unsigned short none;
        };
    } packet;
#pragma pack(pop)

    packet.id = CZ_REQ_ITEMREPAIR2;
    if (item == (const void *)-1)
        packet.none = 0xffff;
    else
        memcpy(packet.item, item, 24);
    send_packet(CZ_REQ_ITEMREPAIR2, &packet);
    return 0;
}

int game_mode_msg_add_friend_answer(int aid, int char_id, int answer)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        int aid;
        int char_id;
        int answer;
    } packet;
#pragma pack(pop)

    if (answer)
        ui_window_send(ui_window_open(0x22), NULL, 6, 0xcf, 1, 0, 0);
    packet.answer = answer;
    packet.id = CZ_ACK_REQ_ADD_FRIENDS;
    packet.aid = aid;
    packet.char_id = char_id;
    send_packet(CZ_ACK_REQ_ADD_FRIENDS, &packet);
    return 0;
}

int game_mode_msg_shortcut_key_change(int index, int page)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short flag;
        unsigned short index;
        unsigned char key[7];
    } packet;
#pragma pack(pop)
    const struct HotkeySlot *key = NULL;

    memset(packet.key, 0, sizeof packet.key);
    packet.id = CZ_SHORTCUTKEYBAR_ROTATE;
    packet.flag = 0;
    if (index >= 0x24) {
        if ((unsigned)(index - 0x24) <= 1)
            key = &g_session1.quickSlots[index - 0x24];
    } else if (page == 0) {
        key = &g_session1.hotkeys0[index];
    } else if (page == 1) {
        key = &g_session1.hotkeys1[index];
        packet.flag = 1;
    }
    if (key)
        memcpy(packet.key, key, sizeof *key);
    packet.index = (unsigned short)index;
    send_packet(CZ_SHORTCUTKEYBAR_ROTATE, &packet);
    return 0;
}

int game_mode_msg_open_cash_shop(int tab)
{
    if (g_windowMgr.item_shop_wnd || ui_window_mgr_is_barter_open(&g_windowMgr) == 1 || g_windowMgr.n_cash_wnd)
        return 0;
    if (tab < 0 || tab >= 0xb)
        tab = 0;
    send_u32(CZ_CASH_SHOP_OPEN, tab);
    return 0;
}

#pragma pack(push, 1)
struct PacketScreamChat {
    unsigned short id;
    unsigned short length;
    char text[];
};
#pragma pack(pop)
SIZE_ASSERT(PacketScreamChat, 4);

int game_mode_msg_scream_line(int which)
{
    struct File file;
    char text[0x100], line[0x16a], packet_buffer[0x200];
    struct PacketScreamChat *packet = (struct PacketScreamChat *)packet_buffer;
    const char *name;
    char *buf;
    unsigned target;
    size_t size, len;

    if (which != 0 && which != 1)
        return 0;
    name = which == 0 ? "DC_scream.txt" : "BA_frostjoke.txt";
    if (g_serviceType == 10)
        name = which == 0 ? "english\\DC_scream.txt" : "english\\BA_frostjoke.txt";
    file_construct(&file);
    if (!file_open(&file, name, 0, 1, 0)) {
        file_destruct(&file);
        return 0;
    }
    size = file.size;
    buf = size ? malloc(size) : NULL;
    if (!buf) {
        file_close(&file);
        file_destruct(&file);
        return 0;
    }
    file_read(&file, buf, size);
    file_close(&file);
    target = (unsigned)(rand() % (which == 0 ? 114 : 117)) + 1;
    if (!scream_text_line(text, sizeof text, buf, size, target)) {
        free(buf);
        file_destruct(&file);
        return 0;
    }
    free(buf);
    if (snprintf(line, sizeof line, "%s : %s", session_character_name(&g_session1), text_decode(text)) < 0) {
        file_destruct(&file);
        return 0;
    }
    len = strlen(line);
    window_mgr_clear_chat_line(&g_windowMgr);
    packet->id = CZ_REQUEST_CHAT;
    packet->length = (unsigned short)(len + 5);
    memcpy(packet->text, line, len + 1);
    send_raw((unsigned short)(len + 5), packet);
    file_destruct(&file);
    return 0;
}
