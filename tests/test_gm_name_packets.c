#include "session/state.h"
#include "contents/connection_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            fprintf(stderr, "GM name packet check failed at line %d: %s\n", __LINE__, #condition); \
            exit(2);                                                                               \
        }                                                                                          \
    } while (0)

#include "../src/game/msg_misc.c"
#include "gm_name_packets_unreached.h"
struct Session g_session1;
static int connection_token;
static unsigned char sent_packet[26];
static unsigned sent;
static char chat_line[64];

struct RagConnection *rag_connection_instance(void)
{
    return (struct RagConnection *)&connection_token;
}

int rag_connection_get_packet_length(struct RagConnection *connection, int id)
{
    CHECK(connection == (struct RagConnection *)&connection_token);
    CHECK(id == 0x1bb || id == 0x1bd || id == CZ_REQ_STATUS_GM);
    return 26;
}

char rag_connection_send_packet(struct RagConnection *connection, int length, const void *packet)
{
    CHECK(connection == (struct RagConnection *)&connection_token && length == 26);
    memcpy(sent_packet, packet, 26);
    ++sent;
    return 1;
}

intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *mgr, int msg, intptr_t text, intptr_t color,
                                 intptr_t arg4, intptr_t arg5)
{
    CHECK(mgr == &g_windowMgr && msg == UIM_CHAT_LINE && color == CHAT_COLOR_GM_STATUS);
    CHECK(strlen((const char *)text) < sizeof chat_line);
    strcpy(chat_line, (const char *)text);
    return 0;
}

static char *name_at_page_end(const char *name)
{
    size_t size = strlen(name) + 1;
    char *pages = VirtualAlloc(NULL, 0x2000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    DWORD old;
    CHECK(pages);
    CHECK(VirtualProtect(pages + 0x1000, 0x1000, PAGE_NOACCESS, &old));
    memcpy(pages + 0x1000 - size, name, size);
    return pages + 0x1000 - size;
}

static void check_name_packet(unsigned short id, const char *name)
{
    unsigned char expected[26];
    size_t length = strlen(name);
    memset(expected, 0, sizeof expected);
    expected[0] = (unsigned char)id;
    expected[1] = (unsigned char)(id >> 8);
    memcpy(expected + 2, name, length < 24 ? length : 24);
    CHECK(!memcmp(sent_packet, expected, sizeof expected));
}

int main(void)
{
    static const char long_name[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    char *short_name = name_at_page_end("gm");
    char *full_name = name_at_page_end("ABCDEFGHIJKLMNOPQRSTUVWX");

    CHECK(game_mode_msg_send_name(0x1bb, short_name) == 0);
    check_name_packet(0x1bb, "gm");
    CHECK(game_mode_msg_send_name(0x1bd, long_name) == 0);
    check_name_packet(0x1bd, long_name);

    CHECK(game_mode_msg_req_status_gm(short_name) == 0);
    check_name_packet(CZ_REQ_STATUS_GM, "gm");
    CHECK(!strcmp(chat_line, "[ gm ]"));
    CHECK(game_mode_msg_req_status_gm(full_name) == 0);
    check_name_packet(CZ_REQ_STATUS_GM, full_name);
    CHECK(!strcmp(chat_line, "[ ABCDEFGHIJKLMNOPQRSTUVWX ]"));
    CHECK(game_mode_msg_req_status_gm(long_name) == 0);
    check_name_packet(CZ_REQ_STATUS_GM, long_name);
    CHECK(!strcmp(chat_line, "[ ABCDEFGHIJKLMNOPQRSTUVWX ]"));

    CHECK(sent == 5);
    puts("GM name packets: /shift, /recall and the GM status request copy at most the 24-byte "
         "name field, read no further than the name and print it within the field, passed.");
    return 0;
}
