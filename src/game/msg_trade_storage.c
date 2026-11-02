#include "game/mode_msg.h"
#include "platform/language.h"
#include "session/session_items.h"
#include "ui/ui_std.h"

#define CZ_REQ_EXCHANGE_ITEM        0xe4
#define CZ_ACK_EXCHANGE_ITEM        0xe6
#define CZ_ADD_EXCHANGE_ITEM        0xe8

int game_mode_msg_req_exchange(struct GameMode *self, int aid)
{
    struct String name;

    if (g_serviceType == 6 && g_windowMgr.exchange_accept_wnd) {
        chat_refresh();
        chat_print_msg(0x48, CHAT_COLOR_RED);
        return 0;
    }
    self_msg(self, 0x28, 0, 0, 0, 0);
    session_clear_lists(&g_session1);
    game_mode_get_actor_name(self, &name, aid);
    if (name.size != 0) {
        string_assign(&self->exchange_partner, string_cstr(&name), name.size);
        send_u32(CZ_REQ_EXCHANGE_ITEM, aid);
    }
    string_free(&name);
    return 0;
}

int game_mode_msg_ack_exchange(struct GameMode *self, int answer)
{
    self_msg(self, 0x28, 0, 0, 0, 0);
    session_clear_lists(&g_session1);
    send_u8(CZ_ACK_EXCHANGE_ITEM, answer);
    return 0;
}

int game_mode_msg_add_exchange_item(struct GameMode *self, int index, int count)
{
#pragma pack(push, 1)
    struct {
        unsigned short id;
        unsigned short index;
        int count;
    } packet;

#pragma pack(pop)

    if (session_is_exist_index_in_my_change_list(&g_session1, index)) {
        chat_refresh();
        chat_print_msg(0x33, CHAT_COLOR_RED);
        return 0;
    }
    if (session_is_exchange_a_full(&g_session1) && index) {
        chat_refresh();
        chat_print_msg(0x129, CHAT_COLOR_RED);
        return 0;
    }
    if (self->exchange_add_pending)
        return 0;
    self->exchange_add_pending = 1;
    packet.index = (unsigned short)index;
    packet.count = count;
    packet.id = CZ_ADD_EXCHANGE_ITEM;
    self->trade_add_amount = count;
    send_packet(CZ_ADD_EXCHANGE_ITEM, &packet);
    return 0;
}

#pragma pack(push, 1)
struct PacketMoveItem {
    unsigned short id;
    unsigned short index;
    int count;
};
#pragma pack(pop)

static void send_move_item(unsigned short id, int index, int count)
{
    struct PacketMoveItem packet;

    packet.id = id;
    packet.index = (unsigned short)index;
    packet.count = count;
    send_packet(id, &packet);
}

int game_mode_msg_item_to_storage(int index, int count)
{
    session_set_inventory_move_pending(&g_session1, index);
    send_move_item(g_session1.guildStorageOpen ? 0x9e1 : 0x364, index, count);
    return 0;
}

int game_mode_msg_item_from_storage(int index, int count)
{
    session_set_storage_move_pending(&g_session1, index);
    send_move_item(g_session1.guildStorageOpen ? 0x9e2 : 0x365, index, count);
    return 0;
}

int game_mode_msg_item_to_cart(struct GameMode *self, int index, int count)
{
    session_set_inventory_move_pending(&g_session1, index);
    send_move_item(0x126, index, count);
    return 0;
}

int game_mode_msg_item_from_cart(struct GameMode *self, int index, int count)
{
    session_set_cart_move_pending(&g_session1, index);
    send_move_item(0x127, index, count);
    return 0;
}

int game_mode_msg_storage_to_cart(struct GameMode *self, int index, int count)
{
    session_set_storage_move_pending(&g_session1, index);
    send_move_item(g_session1.guildStorageOpen ? 0x9e4 : 0x128, index, count);
    return 0;
}

int game_mode_msg_cart_to_storage(struct GameMode *self, int index, int count)
{
    session_set_cart_move_pending(&g_session1, index);
    send_move_item(g_session1.guildStorageOpen ? 0x9e3 : 0x129, index, count);
    return 0;
}

int game_mode_msg_close_storage(void)
{
    send_id(g_session1.guildStorageOpen ? 0x9be : 0x193);
    return 0;
}
