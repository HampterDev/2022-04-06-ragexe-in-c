#include "session/state.h"
#include "contents/connection_api.h"
#ifndef CONTENTS_CONNECTION_H
#define CONTENTS_CONNECTION_H

#include "std/tree.h"

#include "platform/win_types.h"
#include "platform/net.h"
#include "base.h"

#include "contents/byte_queue.h"

#define CONNECTION_QUEUE_SIZE 0xa000

/* Bytes held back for a fixed time, in the order they came: the latency rig's delay in one
   direction. Each chunk is released whole when its time comes. */
#define LAG_LINE_CHUNKS 512

struct LagLine {
    struct ByteQueue bytes;
    struct LagChunk {
        DWORD due;
        int channel;
        int count;
    } chunks[LAG_LINE_CHUNKS];
    int first, used;
};

struct Connection {
    struct PlatformSocket *socket;
    /* The socket of a login stage that ended with connection_hop, and where it goes: kept
       alive until the next connect takes it over or closes it. */
    struct PlatformSocket *kept;
    uint32_t link_address;
    uint16_t link_port;
    char hold_sends;
    DWORD send_after;
    int failed;
    /* One queue each way per channel of the socket (platform/net.h). Raw reads and the packets
       held back while a map loads use the world channel. */
    struct ByteQueue send_queue[PLATFORM_SOCKET_CHANNELS];
    struct ByteQueue recv_queue[PLATFORM_SOCKET_CHANNELS];
    struct ByteQueue held;
    /* The latency rig (RAGEXE_LAG_MS): every byte waits this long before it is sent and again
       before it is read, so a round trip takes twice as long. 0 is off. */
    unsigned lag_ms;
    struct LagLine lag_out;
    struct LagLine lag_in;
};

void connection_queue_send(struct Connection *connection, int channel, int count, const void *bytes);

struct RagConnection {
    struct Connection base;
};

OFFSET_ASSERT(ServerAddress, port, 0x10);
OFFSET_ASSERT(ServerAddress, host, 0x14);
SIZE_ASSERT(ServerAddress, 0x98);

struct Tree;
struct TreeNode;

#endif
