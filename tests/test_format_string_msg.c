#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/msg_str.h"
#include "game/ui_bridge.h"
#include "std/string.h"
#include "ui/window_manager_state.h"
#include "ui/window_messages.h"

/* Run the production game_mode_format_string_msg (ZC_FORMATSTRING_MSG) on packets
   followed by bytes that are not NUL. Its arguments are NUL-terminated strings one
   after another; each must be read no further than the packet length, so an argument
   that runs to the end of the packet ends there and those after it are empty. */

struct UIWindowMgr g_windowMgr;
static char pushed[0x200];
static int pushes, cases;

__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }

char *msg_str(int id)
{
    switch (id) {
    case 1: return (char *)"[%s|%s|%s]";
    case 2: return (char *)"<%s/%s>";
    case 3: return (char *)"(%s)";
    case 4: return (char *)"plain";
    case 5: return (char *)"%s%s%s%s";
    }
    abort();
}

int ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, int val1, int val2, int val3, int val4)
{
    const char *text = (const char *)val1;

    assert(self == &g_windowMgr && msg == UIM_PUSH_INTO_CHAT && val2 == 0xffffff && val3 == 0 && val4 == 0);
    assert(strlen(text) < sizeof pushed);
    strcpy(pushed, text);
    ++pushes;
    return 0;
}

#include "format_string_msg.inc"

/* Sends msg_id with the given argument bytes as a packet of exactly that length,
   followed in memory by 'Z' bytes. */
static void send_packet_of(unsigned short msg_id, const char *strings, size_t size)
{
    unsigned char buffer[0x400];
    size_t length = 6 + size;

    assert(length + 32 < sizeof buffer);
    memset(buffer, 'Z', sizeof buffer);
    buffer[sizeof buffer - 1] = 0;
    buffer[0] = 0xc2;
    buffer[1] = 0x02;
    buffer[2] = (unsigned char)length;
    buffer[3] = (unsigned char)(length >> 8);
    buffer[4] = (unsigned char)msg_id;
    buffer[5] = (unsigned char)(msg_id >> 8);
    memcpy(buffer + 6, strings, size);
    pushed[0] = 0;
    game_mode_format_string_msg(buffer);
}

static void check(unsigned short msg_id, const char *strings, size_t size, const char *expected)
{
    int before = pushes;

    send_packet_of(msg_id, strings, size);
    if (expected) {
        assert(pushes == before + 1 && strcmp(pushed, expected) == 0);
    } else {
        assert(pushes == before);
    }
    ++cases;
}

#define ARGS(s) s, sizeof s - 1

int main(void)
{
    char longer[0x300], cut[0x105];

    check(1, ARGS("a\0b\0c\0"), "[a|b|c]");
    check(1, ARGS("a\0b\0c"), "[a|b|c]");
    check(1, ARGS("a\0bc"), "[a|bc|]");
    check(1, ARGS("abc"), "[abc||]");
    check(1, ARGS(""), "[||]");
    check(1, ARGS("\0\0\0"), "[||]");
    check(2, ARGS("one\0two\0three\0"), "<one/two>");
    check(2, ARGS("one"), "<one/>");
    check(2, ARGS(""), "</>");
    check(3, ARGS("abc"), "(abc)");
    check(3, ARGS("abc\0"), "(abc)");
    check(3, ARGS(""), "()");
    check(4, ARGS("ignored"), "plain");
    check(5, ARGS("a\0b\0c\0d\0"), NULL);

    /* An argument without a NUL that is longer than the line fills its 0x104 bytes. */
    memset(longer, 'L', sizeof longer);
    cut[0] = '(';
    memset(cut + 1, 'L', 0x103);
    cut[0x104] = 0;
    check(3, longer, sizeof longer, cut);
    printf("Format string messages: %d packets; arguments end at the packet length.\n", cases);
    return 0;
}
