#include "session/state.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "contents/connection.h"
#include "std/std.h"

#include "core/packet_table.h"
#include "delivery/delivery.h"
#include "route/delivery_route.h"
#include "diag/diag.h"
#include "platform/clock.h"

struct RagConnection g_ragConnection;
struct Once g_ragConnection_guard;
void rag_connection_atexit(void);

int rag_connection_get_packet_length(struct RagConnection *conn, int packet_id)
{
    struct PacketLengthInfo info;

    (void)conn;
    packet_table_get_length(&g_packetLengthMap, &info, packet_id);
    return info.length > 2 ? info.length : 2;
}

unsigned short rag_connection_get_packet_type(const void *packet)
{
    unsigned short id;
    memcpy(&id, packet, sizeof id);
    return id;
}

/* The next whole packet of one channel's stream. */
static char take_packet(struct RagConnection *self, struct ByteQueue *queue, void *buf,
                        unsigned short *out_len)
{
    struct PacketLengthInfo info;
    unsigned short id;
    unsigned head;
    unsigned short len;
    if (!byte_queue_peek(queue, 2, &id))
        return 0;
    if (packet_table_get_length(&g_packetLengthMap, &info, id)->kind == PACKET_KIND_FIXED) {
        len = (unsigned short)rag_connection_get_packet_length(self, id);
    } else {
        if (!byte_queue_peek(queue, 4, &head))
            return 0;
        len = (unsigned short)(head >> 16);
        if (len < 4) {
            connection_reset_queues(&self->base);
            return 0;
        }
    }
    if (!byte_queue_read(queue, len, buf))
        return 0;
    if (info.kind == PACKET_KIND_VARIABLE && len < info.length)
        memset((char *)buf + len, 0, (size_t)(info.length - len));
    if (out_len)
        *out_len = len;
    return 1;
}

/* The next whole packet of any channel, the world's first. Packets of one channel come in the
   order they were sent, the delivery channel's in the order they arrived; between channels
   there is no order. */
char rag_connection_get_packet(struct RagConnection *self, void *buf,
                                         unsigned short *out_len)
{
    int channel;

    for (channel = 0; channel < PLATFORM_SOCKET_CHANNELS; channel++)
        if (take_packet(self, &self->base.recv_queue[channel], buf, out_len))
            return 1;
    return 0;
}

/* What this client sends outside the world channel (plans/transport.md). Nothing on the
   world channel has to be read before or after one of these, and each group's packets only
   need their order among themselves. */
static const unsigned short g_listsChannelIds[] = {
    /* whisper, party and guild text */
    0x0096, 0x00cf, 0x00d0, 0x00d3, 0x0108, 0x017e,
    /*   CZ_WHISPER, CZ_SETTING_WHISPER_PC, CZ_SETTING_WHISPER_STATE, CZ_REQ_WHISPER_LIST, CZ_REQUEST_CHAT_PARTY, CZ_GUILD_CHAT */
    /* guild emblem images */
    0x0151, 0x0153,
    /*   CZ_REQ_GUILD_EMBLEM_IMG, CZ_REGISTER_GUILD_EMBLEM_IMG */
    /* friends */
    0x0202, 0x0203, 0x0208,
    /*   CZ_ADD_FRIENDS, CZ_DELETE_FRIENDS, CZ_ACK_REQ_ADD_FRIENDS */
    /* mail */
    0x023f, 0x0241, 0x0243, 0x0244, 0x0246, 0x0247, 0x0248, 0x0273, 0x02f3, 0x02f6, 0x09e8,
    0x09e9, 0x09ea, 0x09ec, 0x09ee, 0x09ef, 0x09f1, 0x09f3, 0x09f5, 0x0a03, 0x0a04, 0x0a06,
    0x0a08, 0x0a6e, 0x0ac0, 0x0ac1, 0x0b97, 0x0b98,
    /*   CZ_MAIL_GET_LIST, CZ_MAIL_OPEN, CZ_MAIL_DELETE, CZ_MAIL_GET_ITEM, CZ_MAIL_RESET_ITEM, CZ_MAIL_ADD_ITEM, CZ_MAIL_SEND, CZ_REQ_MAIL_RETURN, CZ_IRMAIL_SEND, CZ_IRMAIL_LIST, CZ_OPEN_RODEXBOX, CZ_CLOSE_RODEXBOX, CZ_REQ_READ_RODEX, CZ_REQ_SEND_RODEX, CZ_REQ_NEXT_RODEX, CZ_REQ_REFRESH_RODEX, CZ_REQ_ZENY_FROM_RODEX, CZ_REQ_ITEM_FROM_RODEX, CZ_REQ_DELETE_RODEX, CZ_REQ_CANCEL_WRITE_RODEX, CZ_REQ_ADD_ITEM_RODEX, CZ_REQ_REMOVE_RODEX_ITEM, CZ_REQ_OPEN_WRITE_RODEX, CZ_REQ_SEND_RODEX2, CZ_OPEN_ALL_RODEX, CZ_UPDATE_ALL_RODEX, CZ_CHECK_RODEX_RECEIVE, CZ_RODEX_RETURN */
    /* quest log */
    0x02b6,
    /*   CZ_ACTIVE_QUEST */
    /* rankings */
    0x0217, 0x0218, 0x0225, 0x0237, 0x0281, 0x097c,
    /*   CZ_BLACKSMITH_RANK, CZ_ALCHEMIST_RANK, CZ_TAEKWON_RANK, CZ_KILLER_RANK, CZ_GANGSI_RANK, CZ_REQ_RANKING */
};

#define CZ_REQUEST_TIME2 0x0360

int rag_connection_packet_channel(unsigned short id)
{
    unsigned char head[2];
    struct DeliveryRoute route;
    size_t i;

    /* a timed exchange that had to be resent would time the resend */
    if (id == CZ_REQUEST_TIME2)
        return PLATFORM_CHANNEL_NEWEST;
    /* what the player asks of the server, each with the stamp that goes out before it: the
       table the gateway reads too (route/delivery_route.h), which needs no more than the
       id of a request */
    head[0] = (unsigned char)id;
    head[1] = (unsigned char)(id >> 8);
    if (delivery_route(head, sizeof head, &route) && route.kind == DELIVERY_ORDERED)
        return PLATFORM_CHANNEL_DELIVERY;
    for (i = 0; i < sizeof g_listsChannelIds / sizeof g_listsChannelIds[0]; i++)
        if (g_listsChannelIds[i] == id)
            return PLATFORM_CHANNEL_LISTS;
    return PLATFORM_CHANNEL_WORLD;
}

/* Queues a packet without sending it, so it leaves in one write with the packet sent next (or
   at the next poll). */
char rag_connection_queue_packet(struct RagConnection *self, int len, const void *packet)
{
    unsigned short id;
    struct ByteQueue *queue;
    if (!packet || len < (int)sizeof id)
        return 0;
    g_session1.lastSendTime = platform_ticks_ms();
    memcpy(&id, packet, sizeof id);
    if (!self->base.socket)
        return 1;
    if (self->base.hold_sends) {
        queue = &self->base.held;
        byte_queue_write(queue, sizeof id, &id);
        byte_queue_write(queue, len - sizeof id, (const char *)packet + sizeof id);
        return 1;
    }
    connection_queue_send(&self->base, rag_connection_packet_channel(id), len, packet);
    return 1;
}

char rag_connection_send_packet(struct RagConnection *self, int len, const void *packet)
{
    if (!rag_connection_queue_packet(self, len, packet))
        return 0;
    if (self->base.socket && !self->base.hold_sends)
        connection_flush(&self->base);
    return 1;
}

struct Connection *rag_connection_transport(struct RagConnection *connection)
{
    return &connection->base;
}

struct RagConnection *rag_connection_instance(void)
{
    if (once_begin(&g_ragConnection_guard)) {
        rag_connection_construct(&g_ragConnection);
        atexit(rag_connection_atexit);
        once_complete(&g_ragConnection_guard);
    }
    return &g_ragConnection;
}

void rag_connection_atexit(void)
{
    connection_destruct(&g_ragConnection.base);
}
