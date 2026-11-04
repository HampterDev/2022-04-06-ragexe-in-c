#include "session/state.h"
#include "login/loginmode.h"
#include "contents/connection_api.h"
#include "contents/character_settings.h"
#include "login/zone_addr.h"
#include "session/session_talk.h"
#include "session/option_pairs.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Run the production login-mode sender of map entry 0x0436 against stack
   memory filled with a marker, and check every byte that reaches the wire. */
struct Session g_session1;
struct CharUserDataMgr *g_charUserDataMgr;
struct ServerAddress g_mapServerIp;

static struct RagConnection *connection = (struct RagConnection *)&g_mapServerIp;
static unsigned char sent[64];
static int sent_length, sends, hold_sends, wait_windows, cases;

static DWORD test_time(void) { return 0x13579bdf; }

void char_user_data_load(struct CharUserDataMgr *self, int aid, int gid, const char *world)
{
    (void)self; (void)aid; (void)gid; (void)world;
}
struct Connection *rag_connection_transport(struct RagConnection *c) { return (struct Connection *)c; }
struct RagConnection *rag_connection_instance(void) { return connection; }
char connection_connect(struct Connection *c, const struct ServerAddress *address)
{
    assert(c == (struct Connection *)connection && address == &g_mapServerIp);
    return 1;
}
int session_get_sex(struct Session *self) { assert(self == &g_session1); return 1; }
void session_set_pair(struct Session *session, int key, long long value)
{
    assert(session == &g_session1 && key == 0x11 && value == 1);
}
void connection_set_hold_sends(struct Connection *c, char hold)
{
    assert(c == (struct Connection *)connection && hold == 1);
    ++hold_sends;
}
int rag_connection_get_packet_length(struct RagConnection *c, int id)
{
    assert(c == connection && id == 0x436);
    return 23;
}
char rag_connection_send_packet(struct RagConnection *c, int len, const void *packet)
{
    assert(c == connection && len > 0 && len <= (int)sizeof sent);
    memcpy(sent, packet, (size_t)len);
    sent_length = len;
    ++sends;
    return 1;
}

static void default_wallpaper(struct LoginMode *self) { (void)self; }
static void connect_failed(struct LoginMode *self) { (void)self; assert(!"connection succeeds"); }
static void *wait_window(int msg, int a)
{
    assert(msg == 0x7a && a == 0x10);
    ++wait_windows;
    return NULL;
}

#define timeGetTime test_time
#define platform_ticks_ms test_time
#include "map_enter_packet.inc"
#undef platform_ticks_ms
#undef timeGetTime

static __declspec(noinline) void fill_stack(unsigned char marker)
{
    volatile unsigned char area[16384];
    size_t i;

    for (i = 0; i < sizeof area; ++i)
        area[i] = marker;
}

static __declspec(noinline) void send_map_entry(struct LoginMode *mode)
{
    enter_map(mode);
}

static DWORD le32(int offset)
{
    return (DWORD)sent[offset] | (DWORD)sent[offset + 1] << 8 | (DWORD)sent[offset + 2] << 16
           | (DWORD)sent[offset + 3] << 24;
}

static void check_entry(unsigned char marker)
{
    static struct LoginMode mode;
    int previous = sends;

    memset(sent, 0xee, sizeof sent);
    fill_stack(marker);
    send_map_entry(&mode);
    assert(sends == previous + 1 && sent_length == 23 && mode.base.connected == 1);
    assert(sent[0] == 0x36 && sent[1] == 0x04);
    assert(le32(2) == 0x01020304 && le32(6) == 0x0a0b0c0d && le32(10) == 0x55667788);
    assert(le32(14) == 0);
    assert(le32(18) == 0x13579bdf && sent[22] == 1);
    ++cases;
}

int main(void)
{
    g_session1.ownAid = 0x01020304;
    g_session1.ownGid = 0x0a0b0c0d;
    g_session1.authCode = 0x55667788;
    check_entry(0xa5);
    check_entry(0x5a);
    check_entry(0xff);
    assert(hold_sends == cases && wait_windows == cases);
    printf("Map entry packet: %d cases passed (all 23 bytes defined, reserved field zero).\n", cases);
    return 0;
}
