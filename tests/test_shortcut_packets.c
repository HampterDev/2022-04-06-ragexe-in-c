#include "session/state.h"
#include "contents/connection_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "shortcut packet check failed at line %d: %s\n", __LINE__,             \
                    #condition);                                                                   \
            exit(2);                                                                               \
        }                                                                                          \
    } while (0)

#ifdef SHORTCUT_PACKETS_REFERENCE
#else
#include "../src/game/msg_misc.c"
#endif
#include "shortcut_packets_unreached.h"
struct Session g_session1;
static unsigned char expected[13];
static unsigned sent;
static int connection_token;
static FILE *trace;

struct RagConnection *rag_connection_instance(void)
{
    return (struct RagConnection *)&connection_token;
}

int rag_connection_get_packet_length(struct RagConnection *connection, int id)
{
    CHECK(connection == (struct RagConnection *)&connection_token && id == 0xb21);
    return 13;
}

char rag_connection_send_packet(struct RagConnection *connection, int length, const void *packet)
{
    CHECK(connection == (struct RagConnection *)&connection_token && length == 13);
    if (memcmp(packet, expected, 13)) {
        unsigned i;
        for (i = 0; i < 13; ++i)
            fprintf(stderr, "%02x%s", ((const unsigned char *)packet)[i], i == 12 ? "\n" : " ");
        CHECK(!"shortcut packet bytes differ");
    }
    ++sent;
    if (trace)
        CHECK(fwrite(packet, 1, 13, trace) == 13);
    return 1;
}

static void initialize_slot(struct HotkeySlot *slot, unsigned group, unsigned index)
{
    slot->is_skill = (unsigned char)((group + index) & 1);
    slot->id = (int)(0x4a312000u + group * 0x100 + index);
    slot->level = (short)(0x170 + group * 0x10 + index);
}

static void check_packet(int index, int page, int group)
{
    unsigned id, level, source_index;
    memset(expected, 0, sizeof expected);
    expected[0] = 0x21;
    expected[1] = 0x0b;
    expected[2] = group == 1;
    expected[4] = (unsigned char)index;
    expected[5] = (unsigned char)((unsigned)index >> 8);
    if (group >= 0) {
        source_index = group == 2 ? (unsigned)(index - 36) : (unsigned)index;
        id = 0x4a312000u + group * 0x100 + source_index;
        level = 0x170 + group * 0x10 + source_index;
        expected[6] = (unsigned char)((group + source_index) & 1);
        expected[7] = (unsigned char)id;
        expected[8] = (unsigned char)(id >> 8);
        expected[9] = (unsigned char)(id >> 16);
        expected[10] = (unsigned char)(id >> 24);
        expected[11] = (unsigned char)level;
        expected[12] = (unsigned char)(level >> 8);
    }
    CHECK(game_mode_msg_shortcut_key_change(index, page) == 0);
}

int main(void)
{
    int i;
    const char *path = getenv("RAGEXE_SHORTCUT_PACKET_TRACE");
    if (path) {
        trace = fopen(path, "wb");
        CHECK(trace);
    }
    for (i = 0; i < HOTKEY_SLOTS; ++i) {
        initialize_slot(&g_session1.hotkeys0[i], 0, i);
        initialize_slot(&g_session1.hotkeys1[i], 1, i);
    }
    for (i = 0; i < 2; ++i)
        initialize_slot(&g_session1.quickSlots[i], 2, i);
    for (i = 0; i < HOTKEY_SLOTS; ++i) {
        check_packet(i, 0, 0);
        check_packet(i, 1, 1);
    }
    check_packet(36, 0, 2);
    check_packet(37, 1, 2);
    check_packet(36, -1, 2);
    check_packet(37, 99, 2);
    check_packet(0, -1, -1);
    check_packet(35, 2, -1);
    check_packet(38, 0, -1);
    check_packet(65535, 1, -1);
    CHECK(sent == 80);
    if (trace)
        CHECK(!fclose(trace));
    puts("Shortcut packets: all 72 bar slots, both extra slots, flags, indices and zero fallback "
         "bytes passed.");
    return 0;
}
