#include "session/state.h"
#include "contents/connection_api.h"
#ifndef CONTENTS_CONNECTION_H
#define CONTENTS_CONNECTION_H

#include "std/tree.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include "base.h"

#include "contents/byte_queue.h"

#define CONNECTION_QUEUE_SIZE 0xa000

struct Connection {
    SOCKET socket;
    struct sockaddr_in addr;
    char hold_sends;
    DWORD send_after;
    int failed;
    struct ByteQueue send_queue;
    struct ByteQueue recv_queue;
    struct ByteQueue held;
};

struct RagConnection {
    struct Connection base;
};

OFFSET_ASSERT(ServerAddress, port, 0x10);
OFFSET_ASSERT(ServerAddress, host, 0x14);
SIZE_ASSERT(ServerAddress, 0x98);

struct Tree;
struct TreeNode;

#endif
