#include "session/state.h"
#include "contents/connection.h"
#include "core/packet_table.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct PacketTable g_packetLengthMap;
void *g_packetKeys;
struct Session g_session1;
static int key_calls, flush_calls;

struct PacketLengthInfo *packet_table_get_length(struct PacketTable *table,
        struct PacketLengthInfo *out, int id)
{
    (void)table;
    out->kind = id == 0x1234 ? PACKET_KIND_FIXED : PACKET_KIND_VARIABLE;
    out->length = id == 0x1234 ? 6 : 4;
    return out;
}

int packet_keys_next(void *keys, int reseed)
{
    (void)keys;
    (void)reseed;
    ++key_calls;
    return 0x4567;
}

char connection_flush(struct Connection *connection)
{
    (void)connection;
    ++flush_calls;
    return 1;
}

struct RagConnection *rag_connection_construct(struct RagConnection *connection)
{
    memset(connection, 0, sizeof *connection);
    connection->base.socket = 123;
    return connection;
}

void connection_destruct(struct Connection *connection)
{
    byte_queue_destroy(&connection->send_queue);
    byte_queue_destroy(&connection->recv_queue);
    byte_queue_destroy(&connection->held);
}

static void expect_bytes(struct ByteQueue *queue, const void *expected, int count)
{
    unsigned char actual[32];
    assert(byte_queue_size(queue) == count);
    assert(byte_queue_peek(queue, count, actual));
    assert(memcmp(actual, expected, count) == 0);
}

int main(void)
{
    static const unsigned char packet[] = { 0x34, 0x12, 1, 2, 3, 4 };
    static const unsigned char encoded[] = { 0x53, 0x57, 1, 2, 3, 4 };
    static const unsigned char variable[] = { 0x35, 0x12, 7, 0, 9, 8, 7 };
    struct RagConnection *connection = rag_connection_instance();
    struct ByteQueue *received = &connection->base.recv_queue;
    unsigned char output[32];
    unsigned short length = 0xeeee;

    assert(rag_connection_get_packet_length(connection, 0x2bf) == 12);
    assert(rag_connection_get_packet_length(connection, 0x2c0) == 4);
    assert(rag_connection_get_packet_length(connection, 0x1234) == 6);
    assert(rag_connection_get_packet_type(packet) == 0x1234);
    assert(!rag_connection_send_packet(connection, 1, packet));
    assert(key_calls == 0);
    assert(rag_connection_send_packet(connection, sizeof packet, packet));
    expect_bytes(&connection->base.send_queue, encoded, sizeof encoded);
    assert(packet[0] == 0x34 && packet[1] == 0x12);
    assert(key_calls == 1 && flush_calls == 1);

    connection->base.hold_sends = 1;
    assert(rag_connection_send_packet(connection, sizeof packet, packet));
    expect_bytes(&connection->base.held, encoded, sizeof encoded);
    assert(key_calls == 2 && flush_calls == 1);
    rag_connection_drop_sends(connection, 1);
    assert(rag_connection_send_packet(connection, sizeof packet, packet));
    assert(key_calls == 2);
    rag_connection_drop_sends(connection, 0);
    connection->base.socket = INVALID_SOCKET;
    assert(rag_connection_send_packet(connection, sizeof packet, packet));
    assert(key_calls == 3 && flush_calls == 1);
    expect_bytes(&connection->base.held, encoded, sizeof encoded);

    byte_queue_write(received, 1, packet);
    assert(!rag_connection_get_packet(connection, output, &length));
    assert(length == 0xeeee && byte_queue_size(received) == 1);
    byte_queue_write(received, sizeof packet - 1, packet + 1);
    assert(rag_connection_get_packet(connection, output, &length));
    assert(length == sizeof packet && memcmp(packet, output, length) == 0);
    byte_queue_write(received, 3, variable);
    assert(!rag_connection_get_packet(connection, output, NULL));
    byte_queue_write(received, 2, variable + 3);
    assert(!rag_connection_get_packet(connection, output, NULL));
    assert(byte_queue_size(received) == 5);
    byte_queue_write(received, 2, variable + 5);
    byte_queue_write(received, sizeof packet, packet);
    assert(rag_connection_get_packet(connection, output, &length));
    assert(length == sizeof variable && memcmp(variable, output, length) == 0);
    assert(rag_connection_get_packet(connection, output, &length));
    assert(length == sizeof packet && memcmp(packet, output, length) == 0);
    assert(byte_queue_size(received) == 0);
    puts("packets: framing, partial reads, immutable send, holding and key sequence passed");
    return 0;
}
