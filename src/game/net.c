#include "contents/connection_api.h"
#include "game/net.h"

void net_send_header_only(unsigned short packet_id)
{
    rag_connection_send_packet(rag_connection_instance(), 2, &packet_id);
}

void net_send_header_only2(unsigned short packet_id)
{
    rag_connection_send_packet(rag_connection_instance(), 2, &packet_id);
}
