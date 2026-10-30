#include "session/state.h"
#include "ui/window_manager_state.h"
#include "contents/connection_api.h"
#include "session/csession.h"
#include "session/session_state.h"

#define CHAT_COLOR_FEED     0x64ffff

void session_set_homun_auto_feed(struct Session *self, BYTE on)
{
    int msg;

    if (on == 1) {
        self->homunAutoFeed = 1;
        msg = 0xcd2;
    } else {
        g_session1.homunAutoFeed = 0;
        msg = 0xcd3;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str(msg), CHAT_COLOR_FEED, 0, 0);
}

void session_set_pet_auto_feed(struct Session *self, BYTE on)
{
    g_session1.petAutoFeed = on == 1;
    ui_window_mgr_send_msg_(&g_windowMgr, 1, (int)msg_str((on ^ 1) + 0xa13), CHAT_COLOR_FEED, 0, 0);
}

#pragma pack(push, 1)
struct PacketAutoFeed {
    WORD id;
    int kind;
    int on;
};
#pragma pack(pop)
SIZE_ASSERT(PacketAutoFeed, 10);

static void send_auto_feed(int kind, BYTE on)
{
    struct PacketAutoFeed packet;
    int len;

    packet.kind = kind;
    packet.id = 0x2d8;
    packet.on = on;
    len = rag_connection_get_packet_length(rag_connection_instance(), 0x2d8);
    rag_connection_send_packet(rag_connection_instance(), len, &packet);
}

void session_send_homun_auto_feed(struct Session *self, BYTE on)
{
    send_auto_feed(3, on);
}

void session_send_pet_auto_feed(struct Session *self, BYTE on)
{
    send_auto_feed(2, on);
}
