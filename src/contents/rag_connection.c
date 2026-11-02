#include "session/state.h"
#include "contents/connection_api.h"
#include <stdlib.h>
#include <string.h>
#include "contents/connection.h"
#include "std/std.h"
#include <mmsystem.h>

#include "core/packet_table.h"
#include "diag/diag.h"
#include "ui/net/packet_crypt.h"
#define PACKET_ID_ZC_SRPACKETR2_INIT 0x2bf
#define PACKET_ID_CZ_SRPACKETR2_START 0x2c0

struct RagConnection g_ragConnection;
INIT_ONCE g_ragConnection_guard;
void rag_connection_atexit(void);

int rag_connection_get_packet_length(struct RagConnection *conn, int packet_id)
{
    struct PacketLengthInfo info;

    (void)conn;
    if (packet_id == PACKET_ID_ZC_SRPACKETR2_INIT)
        return 12;
    if (packet_id == PACKET_ID_CZ_SRPACKETR2_START)
        return 4;
    packet_table_get_length(&g_packetLengthMap, &info, packet_id);
    return info.length > 2 ? info.length : 2;
}

unsigned short rag_connection_get_packet_type(const void *packet)
{
    unsigned short id;
    memcpy(&id, packet, sizeof id);
    return id;
}

char rag_connection_get_packet(struct RagConnection *self, void *buf,
                                         unsigned short *out_len)
{
    struct ByteQueue *queue = &self->base.recv_queue;
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
    if (out_len)
        *out_len = len;
    return 1;
}

char rag_connection_send_packet(struct RagConnection *self, int len, const void *packet)
{
    unsigned short id;
    struct ByteQueue *queue;
    unsigned short key;
    if (!packet || len < (int)sizeof id)
        return 0;
    if (self->drop_sends)
        return 1;
    g_session1.lastSendTime = timeGetTime();
    key = (unsigned short)packet_keys_next(g_packetKeys, 0);
    memcpy(&id, packet, sizeof id);
    id ^= key;
    if (self->base.socket == INVALID_SOCKET)
        return 1;
    queue = self->base.hold_sends ? &self->base.held : &self->base.send_queue;
    byte_queue_write(queue, sizeof id, &id);
    byte_queue_write(queue, len - sizeof id, (const char *)packet + sizeof id);
    if (!self->base.hold_sends)
        connection_flush(&self->base);
    return 1;
}

struct Connection *rag_connection_transport(struct RagConnection *connection)
{
    return &connection->base;
}

void rag_connection_drop_sends(struct RagConnection *connection, int drop)
{
    connection->drop_sends = drop != 0;
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
