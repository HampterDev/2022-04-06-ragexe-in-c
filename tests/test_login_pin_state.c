#include "login/loginmode.h"
#include "core/unknown_packet.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Drives the real character-server packet loop (login_dispatch.c) and PIN-state handler
   (char_results.c). PIN codes are unsupported: only "no PIN required" states continue. */

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
BYTE g_packetBuffer[0x10000];
int g_serviceType;

#include "login_pin_state_unreached.h"

enum { EV_DISCONNECT = 1, EV_BOX, EV_RESET };
static struct RagConnection *const rag = (struct RagConnection *)0x1000;
static struct Connection *const transport = (struct Connection *)0x2000;
static struct LoginMode mode, expected_mode;
static int events[8], event_count;
static char box_text[256];
static BYTE queued[32];
static unsigned short queued_size;
static int queued_ready;
static const char unsupported[] =
    "PIN codes are not supported by this client. Please ask the server administrator to disable PIN codes.";

static void record(int event)
{
    assert(event_count < (int)(sizeof events / sizeof *events));
    events[event_count++] = event;
}

struct RagConnection *rag_connection_instance(void) { return rag; }
struct Connection *rag_connection_transport(struct RagConnection *connection)
{
    assert(connection == rag);
    return transport;
}
char connection_poll(struct Connection *connection)
{
    assert(connection == transport);
    return 1;
}
char rag_connection_get_packet(struct RagConnection *connection, void *buffer, unsigned short *length)
{
    assert(connection == rag);
    if (!queued_ready)
        return 0;
    queued_ready = 0;
    memcpy(buffer, queued, queued_size);
    *length = queued_size;
    return 1;
}
unsigned short rag_connection_get_packet_type(const void *packet)
{
    const BYTE *p = packet;
    return (unsigned short)(p[0] | p[1] << 8);
}
void connection_disconnect(struct Connection *connection)
{
    assert(connection == transport);
    record(EV_DISCONNECT);
}
void connection_reset_queues(struct Connection *connection)
{
    assert(connection == transport);
    record(EV_RESET);
}
int ui_window_mgr_message_box(struct UIWindowMgr *self, const char *text, int type, int arg3, int arg4,
                              int arg5, const char *title, int width, int height, int arg9)
{
    assert(self == &g_windowMgr && text && title);
    assert(type == 0 && arg3 == 1 && arg4 == 0 && arg5 == 0 && width == 0x118 && height == 0x78 && arg9 == 0);
    assert(strlen(text) < sizeof box_text);
    strcpy(box_text, text);
    record(EV_BOX);
    return 0xb0;
}

static unsigned random_state = 0x8b90ae9u;
static BYTE random_byte(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return (BYTE)(random_state >> 24);
}

/* Queue one packet; for PIN-state notices, bytes 10-11 hold the state. */
static void run(unsigned short id, unsigned short size, unsigned state)
{
    unsigned short i;

    assert(size >= 2 && size <= sizeof queued);
    for (i = 0; i < size; ++i)
        queued[i] = random_byte();
    queued[0] = (BYTE)id;
    queued[1] = (BYTE)(id >> 8);
    if (size >= 12) {
        queued[10] = (BYTE)state;
        queued[11] = (BYTE)(state >> 8);
    }
    queued_size = size;
    queued_ready = 1;
    memset(&mode, 0x5a, sizeof mode);
    mode.base.next_sub_mode = -1;
    memcpy(&expected_mode, &mode, sizeof mode);
    g_mapServerAidPending = 0x5a;
    event_count = 0;
    box_text[0] = 0;
    login_mode_process_packets(&mode);
    assert(!queued_ready);
}

static void expect_select(void)
{
    expected_mode.base.next_sub_mode = 7;
    assert(event_count == 0 && g_mapServerAidPending == 0x5a);
    assert(!memcmp(&mode, &expected_mode, sizeof mode));
}

static void expect_unsupported(void)
{
    expected_mode.base.next_sub_mode = 3;
    assert(event_count == 2 && events[0] == EV_DISCONNECT && events[1] == EV_BOX);
    assert(!strcmp(box_text, unsupported));
    assert(g_mapServerAidPending == 0);
    assert(!memcmp(&mode, &expected_mode, sizeof mode));
}

/* OTP is unsupported too: a mobile-OTP request (0x09a2) leaves like a refused PIN state. */
static void expect_otp_refused(void)
{
    expected_mode.base.next_sub_mode = 3;
    assert(event_count == 2 && events[0] == EV_DISCONNECT && events[1] == EV_BOX);
    assert(!strcmp(box_text, OTP_UNSUPPORTED));
    assert(g_mapServerAidPending == 0);
    assert(!memcmp(&mode, &expected_mode, sizeof mode));
}

static void expect_ignored(void)
{
    assert(event_count == 0 && g_mapServerAidPending == 0x5a);
    assert(!memcmp(&mode, &expected_mode, sizeof mode));
}

static void expect_unknown_packet(void)
{
    assert(event_count == 1 && events[0] == EV_RESET && g_mapServerAidPending == 0x5a);
    assert(!memcmp(&mode, &expected_mode, sizeof mode));
}

int main(void)
{
    static const unsigned short notices[2][2] = { { 0x08b9, 12 }, { 0x0ae9, 13 } };
    static const unsigned short retired[] = {
        0x08b7, 0x08b8, 0x08ba, 0x08bb, 0x08bc, 0x08bd, 0x08be, 0x08bf, 0x08c3, 0x08c4, 0x08c5, 0x08c6,
        /* 2026-11-03: billing/play-time notice and Taren refusal */
        0x01b5, 0x0aca,
    };
    static struct Session session_before;
    static struct UIWindowMgr manager_before;
    unsigned state, n, proceeded = 0, refused = 0, ignored = 0;
    size_t i;

    for (n = 0; n < 2; ++n) {
        for (state = 0; state <= 0xffff; ++state) {
            if (state < 32) {
                memcpy(&session_before, &g_session1, sizeof g_session1);
                memcpy(&manager_before, &g_windowMgr, sizeof g_windowMgr);
            }
            run(notices[n][0], notices[n][1], state);
            if (state == 0 || state == 7) {
                expect_select();
                ++proceeded;
            } else if (state <= 0xb) {
                expect_unsupported();
                ++refused;
            } else {
                expect_ignored();
                ++ignored;
            }
            if (state < 32) {
                assert(!memcmp(&session_before, &g_session1, sizeof g_session1));
                assert(!memcmp(&manager_before, &g_windowMgr, sizeof g_windowMgr));
            }
        }
    }
    assert(proceeded == 4 && refused == 20 && ignored == 2 * (0x10000 - 12));
    for (i = 0; i < sizeof retired / sizeof *retired; ++i) {
        for (n = 2; n <= 16; ++n) {
            run(retired[i], (unsigned short)n, 0);
            expect_unknown_packet();
        }
    }
    for (n = 6; n <= 16; ++n) {
        run(0x09a2, (unsigned short)n, 0);
        expect_otp_refused();
    }
    puts("Login PIN state: real character-server dispatch continues for states 0 and 7, refuses 1-6 and 8-11 "
         "with disconnect, message and login screen, ignores unknown states for 0x08b9/0x0ae9, and sends "
         "retired PIN, billing and Taren packets to the unknown-packet path; refuses mobile-OTP requests "
         "(0x09a2) the same way.");
    return 0;
}
