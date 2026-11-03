#include "session/state.h"
#include "contents/connection.h"
#include "core/packet_table.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct PacketTable g_packetLengthMap;
struct Session g_session1;
static int flush_calls, reset_calls;

struct PacketLengthInfo *packet_table_get_length(struct PacketTable *table,
        struct PacketLengthInfo *out, int id)
{
    (void)table;
    out->kind = id == 0x1234 ? PACKET_KIND_FIXED : id == 0x7777 ? PACKET_KIND_UNKNOWN : PACKET_KIND_VARIABLE;
    out->length = id == 0x1234 ? 6 : id == 0x1236 ? 8 : 4;
    return out;
}

void connection_reset_queues(struct Connection *connection)
{
    assert(connection == rag_connection_transport(rag_connection_instance()));
    byte_queue_reset(&connection->recv_queue, CONNECTION_QUEUE_SIZE);
    byte_queue_reset(&connection->send_queue, CONNECTION_QUEUE_SIZE);
    byte_queue_reset(&connection->held, CONNECTION_QUEUE_SIZE);
    ++reset_calls;
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

static void test_all_packet_ids(struct RagConnection *connection)
{
    unsigned id;
    int mode, i, before, length;
    unsigned char packet[32], expected[32];

    for (id = 0; id <= 0xffff; ++id) {
        for (i = 0; i < sizeof packet; ++i)
            packet[i] = (unsigned char)(id + 31 * i);
        packet[0] = (unsigned char)id;
        packet[1] = (unsigned char)(id >> 8);
        memcpy(expected, packet, sizeof expected);
        length = 2 + id % 31;
        for (mode = 0; mode < 3; ++mode) {
            byte_queue_reset(&connection->base.send_queue, 0);
            byte_queue_reset(&connection->base.held, 0);
            connection->base.hold_sends = mode == 1;
            connection->base.socket = mode == 2 ? INVALID_SOCKET : 123;
            before = flush_calls;
            assert(rag_connection_send_packet(connection, length, packet));
            assert(!memcmp(packet, expected, sizeof packet));
            assert(flush_calls == before + (mode == 0));
            if (mode == 0) {
                expect_bytes(&connection->base.send_queue, expected, length);
                assert(byte_queue_size(&connection->base.held) == 0);
            } else if (mode == 1) {
                expect_bytes(&connection->base.held, expected, length);
                assert(byte_queue_size(&connection->base.send_queue) == 0);
            } else {
                assert(byte_queue_size(&connection->base.send_queue) == 0);
                assert(byte_queue_size(&connection->base.held) == 0);
            }
        }
    }
}

int main(void)
{
    static const unsigned char packet[] = { 0x34, 0x12, 1, 2, 3, 4 };
    static const unsigned char variable[] = { 0x35, 0x12, 7, 0, 9, 8, 7 };
    struct RagConnection *connection = rag_connection_instance();
    struct ByteQueue *received = &connection->base.recv_queue;
    unsigned char output[32];
    unsigned short length = 0xeeee;

    /* The SRPACKETR2 rows are gone: both IDs now use the unknown-packet length. */
    assert(rag_connection_get_packet_length(connection, 0x2bf) == 4);
    assert(rag_connection_get_packet_length(connection, 0x2c0) == 4);
    assert(rag_connection_get_packet_length(connection, 0x1234) == 6);
    assert(rag_connection_get_packet_type(packet) == 0x1234);
    g_session1.lastSendTime = 0x76543210;
    assert(!rag_connection_send_packet(connection, 1, packet));
    assert(!rag_connection_send_packet(connection, -1, packet));
    assert(!rag_connection_send_packet(connection, sizeof packet, NULL));
    assert(g_session1.lastSendTime == 0x76543210);
    assert(flush_calls == 0);
    assert(rag_connection_send_packet(connection, sizeof packet, packet));
    expect_bytes(&connection->base.send_queue, packet, sizeof packet);
    assert(packet[0] == 0x34 && packet[1] == 0x12);
    assert(flush_calls == 1);

    connection->base.hold_sends = 1;
    assert(rag_connection_send_packet(connection, sizeof packet, packet));
    expect_bytes(&connection->base.held, packet, sizeof packet);
    assert(flush_calls == 1);
    connection->base.socket = INVALID_SOCKET;
    assert(rag_connection_send_packet(connection, sizeof packet, packet));
    assert(flush_calls == 1);
    expect_bytes(&connection->base.held, packet, sizeof packet);

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
    assert(reset_calls == 0);

    {
        static const unsigned char shortest[] = { 0x35, 0x12, 4, 0 };
        unsigned char framed[4 + sizeof packet];
        int framed_length, previous;

        for (framed_length = 0; framed_length < 8; ++framed_length) {
            framed[0] = (unsigned char)(framed_length < 4 ? 0x35 : 0x77);
            framed[1] = (unsigned char)(framed_length < 4 ? 0x12 : 0x77);
            framed[2] = (unsigned char)(framed_length % 4);
            framed[3] = 0;
            memcpy(framed + 4, packet, sizeof packet);
            byte_queue_write(received, sizeof framed, framed);
            byte_queue_write(&connection->base.send_queue, 2, packet);
            previous = reset_calls;
            length = 0xeeee;
            assert(!rag_connection_get_packet(connection, output, &length));
            assert(reset_calls == previous + 1 && length == 0xeeee);
            assert(byte_queue_size(received) == 0);
            assert(byte_queue_size(&connection->base.send_queue) == 0);
            assert(byte_queue_size(&connection->base.held) == 0);
            assert(!rag_connection_get_packet(connection, output, &length));
            assert(reset_calls == previous + 1);
        }
        byte_queue_write(received, sizeof shortest, shortest);
        byte_queue_write(received, sizeof packet, packet);
        assert(rag_connection_get_packet(connection, output, &length));
        assert(length == sizeof shortest && memcmp(shortest, output, length) == 0);
        assert(rag_connection_get_packet(connection, output, &length));
        assert(length == sizeof packet && memcmp(packet, output, length) == 0);
        assert(byte_queue_size(received) == 0 && reset_calls == 8);
    }

    {
        /* 0x1236 is variable with an 8-byte table minimum; 0x7777 has no row. */
        static const unsigned char minimum[] = { 0x36, 0x12, 8, 0, 1, 2, 3, 4 };
        static const unsigned char unknown[] = { 0x77, 0x77, 5, 0, 9 };
        unsigned char framed[8];
        int framed_length, previous;

        for (framed_length = 4; framed_length < 8; ++framed_length) {
            memcpy(framed, minimum, sizeof framed);
            framed[2] = (unsigned char)framed_length;
            byte_queue_write(received, sizeof framed, framed);
            previous = reset_calls;
            length = 0xeeee;
            assert(!rag_connection_get_packet(connection, output, &length));
            assert(reset_calls == previous + 1 && length == 0xeeee && byte_queue_size(received) == 0);
        }
        byte_queue_write(received, sizeof minimum, minimum);
        byte_queue_write(received, sizeof unknown, unknown);
        assert(rag_connection_get_packet(connection, output, &length));
        assert(length == sizeof minimum && memcmp(minimum, output, length) == 0);
        assert(rag_connection_get_packet(connection, output, &length));
        assert(length == sizeof unknown && memcmp(unknown, output, length) == 0);
        assert(byte_queue_size(received) == 0 && reset_calls == 12);
    }
    test_all_packet_ids(connection);
    puts("Packets: framing, short framed lengths, table minimums, partial reads, immutable plain sends, holding, disconnected sends and 196608 packet-ID/send-mode cases passed.");
    return 0;
}
