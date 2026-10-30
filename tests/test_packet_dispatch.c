#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "packet_dispatch_trace.h"
#include "game/gamemode.h"
#include "game/packet_dispatch.h"
#include "game/packet_ids.h"
#include "game/navigation.h"
#include "contents/connection_api.h"
#include "ui/social/entry_queue_state.h"
#include "ui/window_access.h"
#include "replay/event.h"

#include "diag/diag.h"

struct Session g_session1;
static struct EntryQueueMgr entry_queue;
struct EntryQueueMgr *g_entryQueueMgr = &entry_queue;
static struct GameMode mode, expected_mode;
static unsigned char packet[4096], expected_packet[4096];
static int connection_token, transport_token;
static uint64_t trace[128], digest = UINT64_C(14695981039346656037);
static size_t trace_count, cases;
static FILE *trace_file;

static uint64_t hash_bytes(uint64_t value, const void *bytes, size_t size)
{
    const unsigned char *p = bytes;
    while (size--) {
        value ^= *p++;
        value *= UINT64_C(1099511628211);
    }
    return value;
}

void packet_trace_value(uint64_t value)
{
    assert(trace_count < sizeof trace / sizeof *trace);
    trace[trace_count++] = value;
}

void packet_trace_call(const char *name)
{
    packet_trace_value(UINT64_MAX);
    packet_trace_value(hash_bytes(UINT64_C(14695981039346656037), name, strlen(name)));
}

void packet_trace_pointer(const void *pointer)
{
    uintptr_t p = (uintptr_t)pointer, base = (uintptr_t)packet;
    if (!pointer)
        packet_trace_value(0);
    else if (p >= base && p <= base + sizeof packet)
        packet_trace_value(0x10000 + (p - base));
    else if (pointer == &mode)
        packet_trace_value(1);
    else if (pointer == &g_session1)
        packet_trace_value(2);
    else if (pointer == g_entryQueueMgr)
        packet_trace_value(3);
    else if (pointer == &connection_token)
        packet_trace_value(4);
    else if (pointer == &transport_token)
        packet_trace_value(5);
    else {
        fprintf(stderr, "Unexpected pointer in dispatcher case %zu\n", cases);
        abort();
    }
}

struct RagConnection *rag_connection_instance(void)
{
    packet_trace_call("RagConnection_Instance");
    return (struct RagConnection *)&connection_token;
}

struct Connection *rag_connection_transport(struct RagConnection *connection)
{
    packet_trace_call("rag_connection_transport");
    packet_trace_pointer(connection);
    return (struct Connection *)&transport_token;
}

char rag_connection_send_packet(struct RagConnection *connection, int size, const void *bytes)
{
    int i;
    packet_trace_call("CRagConnection_SendPacket");
    packet_trace_pointer(connection);
    packet_trace_value(size);
    assert(size == 2);
    for (i = 0; i < size; ++i)
        packet_trace_value(((const unsigned char *)bytes)[i]);
    return 1;
}

void error_msg(const char *text, int flags)
{
    packet_trace_call("ErrorMsg");
    packet_trace_value(hash_bytes(UINT64_C(14695981039346656037), text, strlen(text)));
    packet_trace_value(flags);
}

struct UIWindow *ui_window_open(int id)
{
    packet_trace_call("ui_window_open");
    packet_trace_value(id);
    return NULL;
}

static void run_case(int second, int id, size_t size, int null_packet)
{
    uint64_t count;
    memset(&mode, 0xa5, sizeof mode);
    memcpy(&expected_mode, &mode, sizeof mode);
    trace_count = 0;
    packet_trace_value(second);
    packet_trace_value(id);
    packet_trace_value(size);
    packet_trace_value(null_packet);
    if (second)
        game_mode_dispatch_packet2(&mode, id, null_packet ? NULL : packet, size);
    else
        game_mode_dispatch_packet(&mode, id, null_packet ? NULL : packet, size);
    if (!null_packet && size >= 2 && !second && id == HEADER_ZC_NPCACK_ENABLE)
        expected_mode.server_move_pending = 0;
    if (null_packet || size < 2)
        assert(trace_count == 4);
    assert(!memcmp(&mode, &expected_mode, sizeof mode));
    assert(!memcmp(packet, expected_packet, sizeof packet));
    packet_trace_value(mode.server_move_pending);
    count = trace_count;
    digest = hash_bytes(digest, &count, sizeof count);
    digest = hash_bytes(digest, trace, trace_count * sizeof *trace);
    if (trace_file) {
        assert(fwrite(&count, sizeof count, 1, trace_file) == 1);
        assert(fwrite(trace, sizeof *trace, trace_count, trace_file) == trace_count);
    }
    ++cases;
}

int main(int argc, char **argv)
{
    static const unsigned short replay_kinds[] = {0x388, 0x38f, 0x38e, 0x390, 0x38a, 0, 0xffff};
    int pattern, second, id;
    size_t i, size;
    if (argc == 2) {
        trace_file = fopen(argv[1], "wb");
        assert(trace_file);
    }
    for (pattern = 0; pattern < 6; ++pattern) {
        for (i = 0; i < sizeof packet; ++i) {
            static const unsigned char fixed[] = {0, 0xff, 0x80, 0x7f};
            packet[i] = pattern < 4 ? fixed[pattern] : (unsigned char)(pattern == 4 ? i : 255 - i);
        }
        memcpy(expected_packet, packet, sizeof packet);
        for (second = 0; second < 2; ++second) {
            for (id = 0; id <= 0xffff; ++id) {
                run_case(second, id, sizeof packet, 0);
                if (pattern == 0) {
                    run_case(second, id, 0, 0);
                    run_case(second, id, 1, 0);
                    run_case(second, id, sizeof packet, 1);
                }
            }
        }
    }
    for (second = 0; second < 2; ++second) {
        for (size = 2; size <= sizeof(struct PacketNavigationActive) + 1; ++size)
            run_case(second, HEADER_ZC_NAVIGATION_ACTIVE, size, 0);
        for (i = 0; i < sizeof replay_kinds / sizeof *replay_kinds; ++i) {
            memcpy(packet + 2, &replay_kinds[i], sizeof replay_kinds[i]);
            memcpy(expected_packet, packet, sizeof packet);
            for (size = 2; size <= sizeof(struct ReplayEventHeader) + 1; ++size)
                run_case(second, HEADER_CC_REPLAYPACKET, size, 0);
            run_case(second, HEADER_CC_REPLAYPACKET, sizeof packet, 0);
        }
    }
    if (trace_file)
        assert(fclose(trace_file) == 0);
    assert(cases == 1179800);
    assert(digest == UINT64_C(0x32d81ae6406478e3));
    printf("Packet dispatch: %zu cases, digest=%016llx passed.\n", cases, (unsigned long long)digest);
    return 0;
}
