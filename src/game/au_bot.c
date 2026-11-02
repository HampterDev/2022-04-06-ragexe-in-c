#include "contents/connection_api.h"
#include "game/au_bot.h"

enum {
    HEADER_CZ_ACK_AU_BOT = 0xa2a,
};

#pragma pack(push, 1)
struct PacketAckAuBot {
    unsigned short packet_type;
    int aid;
};
#pragma pack(pop)

void game_mode_on_req_au_bot(struct GameMode *self, int aid)
{
    struct PacketAckAuBot ack;
    int size;

    (void)self;
    ack.packet_type = HEADER_CZ_ACK_AU_BOT;
    ack.aid = aid;
    size = rag_connection_get_packet_length(rag_connection_instance(), HEADER_CZ_ACK_AU_BOT);
    rag_connection_send_packet(rag_connection_instance(), size, &ack);
}
