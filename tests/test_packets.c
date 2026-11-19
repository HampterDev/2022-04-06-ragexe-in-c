#include "session/state.h"
#include "contents/connection.h"
#include "core/packet_table.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct PacketTable g_packetLengthMap;
struct Session g_session1;
static int flush_calls, reset_calls;
static char open_socket;
#define OPEN_SOCKET ((struct PlatformSocket *)&open_socket)

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
    int channel;

    assert(connection == rag_connection_transport(rag_connection_instance()));
    for (channel = 0; channel < PLATFORM_SOCKET_CHANNELS; channel++) {
        byte_queue_reset(&connection->recv_queue[channel], CONNECTION_QUEUE_SIZE);
        byte_queue_reset(&connection->send_queue[channel], CONNECTION_QUEUE_SIZE);
    }
    byte_queue_reset(&connection->held, CONNECTION_QUEUE_SIZE);
    ++reset_calls;
}

char connection_flush(struct Connection *connection)
{
    (void)connection;
    ++flush_calls;
    return 1;
}

/* Everything is kept in the world channel's queue, and the channel it was sent on is noted. */
static int last_channel;

void connection_queue_send(struct Connection *connection, int channel, int count, const void *bytes)
{
    last_channel = channel;
    byte_queue_write(&connection->send_queue[0], count, bytes);
}

struct RagConnection *rag_connection_construct(struct RagConnection *connection)
{
    memset(connection, 0, sizeof *connection);
    connection->base.socket = OPEN_SOCKET;
    return connection;
}

void connection_destruct(struct Connection *connection)
{
    int channel;

    for (channel = 0; channel < PLATFORM_SOCKET_CHANNELS; channel++) {
        byte_queue_destroy(&connection->send_queue[channel]);
        byte_queue_destroy(&connection->recv_queue[channel]);
    }
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
    int on_channel[PLATFORM_SOCKET_CHANNELS] = {0};

    for (id = 0; id <= 0xffff; ++id) {
        for (i = 0; i < sizeof packet; ++i)
            packet[i] = (unsigned char)(id + 31 * i);
        packet[0] = (unsigned char)id;
        packet[1] = (unsigned char)(id >> 8);
        memcpy(expected, packet, sizeof expected);
        length = 2 + id % 31;
        for (mode = 0; mode < 3; ++mode) {
            byte_queue_reset(&connection->base.send_queue[0], 0);
            byte_queue_reset(&connection->base.held, 0);
            connection->base.hold_sends = mode == 1;
            connection->base.socket = mode == 2 ? NULL : OPEN_SOCKET;
            before = flush_calls;
            assert(rag_connection_send_packet(connection, length, packet));
            assert(!memcmp(packet, expected, sizeof packet));
            assert(flush_calls == before + (mode == 0));
            if (mode == 0) {
                on_channel[last_channel]++;
                expect_bytes(&connection->base.send_queue[0], expected, length);
                assert(byte_queue_size(&connection->base.held) == 0);
            } else if (mode == 1) {
                expect_bytes(&connection->base.held, expected, length);
                assert(byte_queue_size(&connection->base.send_queue[0]) == 0);
            } else {
                assert(byte_queue_size(&connection->base.send_queue[0]) == 0);
                assert(byte_queue_size(&connection->base.held) == 0);
            }
        }
    }
    /* one packet is never resent, 46 are text and lists, 8 are the player's requests and their
       stamp, everything else is the world's */
    assert(on_channel[PLATFORM_CHANNEL_NEWEST] == 1 && on_channel[PLATFORM_CHANNEL_LISTS] == 46);
    assert(on_channel[PLATFORM_CHANNEL_DELIVERY] == 8);
    assert(on_channel[PLATFORM_CHANNEL_WORLD] == 0x10000 - 55);
}

int main(void)
{
    static const unsigned char packet[] = { 0x34, 0x12, 1, 2, 3, 4 };
    static const unsigned char variable[] = { 0x35, 0x12, 7, 0, 9, 8, 7 };
    struct RagConnection *connection = rag_connection_instance();
    struct ByteQueue *received = &connection->base.recv_queue[0];
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
    expect_bytes(&connection->base.send_queue[0], packet, sizeof packet);
    assert(packet[0] == 0x34 && packet[1] == 0x12);
    assert(flush_calls == 1);

    connection->base.hold_sends = 1;
    assert(rag_connection_send_packet(connection, sizeof packet, packet));
    expect_bytes(&connection->base.held, packet, sizeof packet);
    assert(flush_calls == 1);
    connection->base.socket = NULL;
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
            byte_queue_write(&connection->base.send_queue[0], 2, packet);
            previous = reset_calls;
            length = 0xeeee;
            assert(!rag_connection_get_packet(connection, output, &length));
            assert(reset_calls == previous + 1 && length == 0xeeee);
            assert(byte_queue_size(received) == 0);
            assert(byte_queue_size(&connection->base.send_queue[0]) == 0);
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
        int framed_length, previous, i;

        for (framed_length = 4; framed_length < 8; ++framed_length) {
            memcpy(framed, minimum, sizeof framed);
            framed[2] = (unsigned char)framed_length;
            byte_queue_write(received, framed_length, framed);
            byte_queue_write(received, sizeof unknown, unknown);
            previous = reset_calls;
            memset(output, 0xcc, 16);
            assert(rag_connection_get_packet(connection, output, &length));
            assert(length == framed_length && memcmp(framed, output, length) == 0);
            for (i = framed_length; i < 8; ++i)
                assert(output[i] == 0);
            assert(output[8] == 0xcc && reset_calls == previous);
            assert(rag_connection_get_packet(connection, output, &length));
            assert(length == sizeof unknown && memcmp(unknown, output, length) == 0);
            assert(byte_queue_size(received) == 0);
        }
        byte_queue_write(received, sizeof minimum, minimum);
        byte_queue_write(received, sizeof unknown, unknown);
        assert(rag_connection_get_packet(connection, output, &length));
        assert(length == sizeof minimum && memcmp(minimum, output, length) == 0);
        assert(rag_connection_get_packet(connection, output, &length));
        assert(length == sizeof unknown && memcmp(unknown, output, length) == 0);
        assert(byte_queue_size(received) == 0 && reset_calls == 8);
    }
    {
        /* A queued packet waits, and leaves in one flush with the packet sent after it. */
        unsigned char both[sizeof variable + sizeof packet];
        int before = flush_calls;

        memcpy(both, variable, sizeof variable);
        memcpy(both + sizeof variable, packet, sizeof packet);
        connection->base.socket = OPEN_SOCKET;
        connection->base.hold_sends = 0;
        byte_queue_reset(&connection->base.send_queue[0], 0);
        byte_queue_reset(&connection->base.held, 0);
        assert(!rag_connection_queue_packet(connection, 1, variable));
        assert(!rag_connection_queue_packet(connection, sizeof variable, NULL));
        assert(rag_connection_queue_packet(connection, sizeof variable, variable));
        expect_bytes(&connection->base.send_queue[0], variable, sizeof variable);
        assert(flush_calls == before);
        assert(rag_connection_send_packet(connection, sizeof packet, packet));
        expect_bytes(&connection->base.send_queue[0], both, sizeof both);
        assert(flush_calls == before + 1);

        byte_queue_reset(&connection->base.send_queue[0], 0);
        connection->base.hold_sends = 1;
        assert(rag_connection_queue_packet(connection, sizeof variable, variable));
        assert(rag_connection_send_packet(connection, sizeof packet, packet));
        expect_bytes(&connection->base.held, both, sizeof both);
        assert(byte_queue_size(&connection->base.send_queue[0]) == 0);
        connection->base.socket = NULL;
        assert(rag_connection_queue_packet(connection, sizeof variable, variable));
        expect_bytes(&connection->base.held, both, sizeof both);
        assert(flush_calls == before + 1);
    }
    {
        /* The channel a packet leaves on: the timed request, a whisper, a mail request, the
           delivery piece's for an input stamp and each request it goes before (a move, an act,
           a stop, a skill on an actor, on the ground and with a text, an intent), and the
           world's for chat said aloud, a turn, an item used and the pickup nothing sends. */
        static const struct { unsigned short id; int channel; } sends[] = {
            { 0x0360, PLATFORM_CHANNEL_NEWEST },   { 0x0096, PLATFORM_CHANNEL_LISTS },    { 0x09e8, PLATFORM_CHANNEL_LISTS },
            { 0x0f01, PLATFORM_CHANNEL_DELIVERY }, { 0x035f, PLATFORM_CHANNEL_DELIVERY }, { 0x0437, PLATFORM_CHANNEL_DELIVERY },
            { 0x0118, PLATFORM_CHANNEL_DELIVERY }, { 0x0438, PLATFORM_CHANNEL_DELIVERY }, { 0x0af4, PLATFORM_CHANNEL_DELIVERY },
            { 0x0367, PLATFORM_CHANNEL_DELIVERY }, { 0x0f02, PLATFORM_CHANNEL_DELIVERY }, { 0x0362, PLATFORM_CHANNEL_WORLD },
            { 0x008c, PLATFORM_CHANNEL_WORLD },    { 0x0361, PLATFORM_CHANNEL_WORLD },    { 0x0439, PLATFORM_CHANNEL_WORLD },
        };
        unsigned char bytes[4] = {0};
        unsigned i;

        connection->base.socket = OPEN_SOCKET;
        connection->base.hold_sends = 0;
        for (i = 0; i < sizeof sends / sizeof sends[0]; i++) {
            memcpy(bytes, &sends[i].id, 2);
            last_channel = -1;
            assert(rag_connection_send_packet(connection, sizeof bytes, bytes));
            assert(last_channel == sends[i].channel);
        }
        /* held packets leave on the world channel later, whatever they are */
        byte_queue_reset(&connection->base.send_queue[0], 0);
    }
    {
        /* Reading: a whole packet of any channel is taken, the world's first, and half a packet
           on one channel does not hold up another's. */
        struct ByteQueue *world = &connection->base.recv_queue[PLATFORM_CHANNEL_WORLD];
        struct ByteQueue *lists = &connection->base.recv_queue[PLATFORM_CHANNEL_LISTS];
        struct ByteQueue *newest = &connection->base.recv_queue[PLATFORM_CHANNEL_NEWEST];

        byte_queue_reset(world, 0);
        byte_queue_reset(lists, 0);
        byte_queue_reset(newest, 0);
        byte_queue_write(lists, sizeof variable, variable);
        byte_queue_write(newest, sizeof packet, packet);
        byte_queue_write(world, 3, packet);
        assert(rag_connection_get_packet(connection, output, &length));
        assert(length == sizeof variable && memcmp(variable, output, length) == 0);
        assert(rag_connection_get_packet(connection, output, &length));
        assert(length == sizeof packet && memcmp(packet, output, length) == 0);
        assert(!rag_connection_get_packet(connection, output, &length));
        byte_queue_write(world, sizeof packet - 3, packet + 3);
        byte_queue_write(lists, sizeof variable, variable);
        assert(rag_connection_get_packet(connection, output, &length));
        assert(length == sizeof packet && memcmp(packet, output, length) == 0);
        assert(rag_connection_get_packet(connection, output, &length));
        assert(length == sizeof variable && memcmp(variable, output, length) == 0);
        assert(!rag_connection_get_packet(connection, output, &length) && reset_calls == 8);
    }
    test_all_packet_ids(connection);
    puts("Packets: framing, short framed lengths, packets below their table minimum read with zeroed tails, partial reads, immutable plain sends, queued sends leaving with the next packet, the channel of what is sent, reading from three channels, holding, disconnected sends and 196608 packet-ID/send-mode cases passed.");
    return 0;
}
