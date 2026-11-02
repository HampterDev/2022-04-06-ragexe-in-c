#include "session/state.h"
#include "ui/cooldown.h"
#include "game/shortcut_keys.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game/character_stats.c"

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static int notifications, clear_step, messages, expected_tab, expected_rotate;
static const unsigned char *key_bytes;
static int window_marker;

void session_invalidate_windows(struct Session *session)
{ assert(session == &g_session1); ++notifications; }
struct UIWindow *ui_window_open(int id) { (void)id; abort(); }
int ui_window_mgr_send_msg_(struct UIWindowMgr *manager, int msg, int a, int b, int c, int d)
{ (void)manager; (void)msg; (void)a; (void)b; (void)c; (void)d; abort(); }
int crt_sprintf(char *buffer, const char *format, ...)
{ (void)buffer; (void)format; abort(); }
int ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message,
                   int a, int b, int c, int d)
{
    assert(window == (struct UIWindow *)&window_marker && !sender);
    assert(message == 0x8f && a == expected_rotate && b == expected_tab && !c && !d);
    ++messages;
    return 0;
}
void session_clear_hotkey_tab0(struct Session *session)
{
    assert(session == &g_session1 && clear_step == 0);
    clear_step = 1;
    memset(session->hotkeys0, 0, sizeof session->hotkeys0);
}
void session_clear_hotkey_tab1(struct Session *session)
{
    assert(session == &g_session1 && clear_step == 0);
    clear_step = 1;
    memset(session->hotkeys1, 0, sizeof session->hotkeys1);
}
void session_clear_quick_slots(struct Session *session)
{
    assert(session == &g_session1 && clear_step == 1);
    assert(!memcmp(session->hotkeys0, key_bytes, sizeof session->hotkeys0));
    clear_step = 2;
    memset(session->quickSlots, 0, sizeof session->quickSlots);
}

static void packet_length(unsigned char *bytes, size_t size)
{
    unsigned short length = (unsigned short)size;
    memcpy(bytes + 2, &length, sizeof length);
}

static void blocked_characters(void)
{
    unsigned char bytes[4 + (SESSION_BLOCKED_CHARACTERS + 1) * sizeof(struct SessionBlockedChar) + 23];
    struct SessionBlockedChar expected[SESSION_BLOCKED_CHARACTERS];
    struct Session *before = malloc(sizeof *before);
    size_t count, size, i;
    assert(before);
    memset(bytes, 0x5c, sizeof bytes);
    bytes[0] = 0x0d; bytes[1] = 2;
    memset(expected, 0, sizeof expected);
    for (i = 0; i < SESSION_BLOCKED_CHARACTERS; ++i) {
        expected[i].gid = (int)(300 + i);
        memset(expected[i].name, 'a' + i % 26, sizeof expected[i].name);
    }
    memcpy(expected[0].name, "한글", sizeof "한글" - 1);
    memcpy(bytes + 4, expected, sizeof expected);
    for (count = 0; count <= SESSION_BLOCKED_CHARACTERS; ++count) {
        size = 4 + count * sizeof *expected;
        packet_length(bytes, size);
        memset(&g_session1, 0xa5, sizeof g_session1);
        memcpy(before, &g_session1, sizeof *before);
        assert(session_set_blocked_characters(&g_session1, bytes, size));
        memcpy(before->blockedChars, expected, count * sizeof *expected);
        before->charListCounts[5] = (int)count;
        assert(!memcmp(before, &g_session1, sizeof *before));
    }
    size = 4 + sizeof expected + 23; packet_length(bytes, size);
    assert(session_set_blocked_characters(&g_session1, bytes, size));
    assert(g_session1.charListCounts[5] == SESSION_BLOCKED_CHARACTERS);
    memcpy(before, &g_session1, sizeof *before);
    size = 4 + (SESSION_BLOCKED_CHARACTERS + 1) * sizeof *expected;
    packet_length(bytes, size);
    assert(!session_set_blocked_characters(&g_session1, bytes, size));
    assert(!memcmp(before, &g_session1, sizeof *before));
    for (i = 0; i < 4; ++i) {
        packet_length(bytes, i);
        assert(!session_set_blocked_characters(&g_session1, bytes, i));
        assert(!memcmp(before, &g_session1, sizeof *before));
    }
    packet_length(bytes, 28);
    for (i = 4; i < 28; ++i)
        assert(!session_set_blocked_characters(&g_session1, bytes, i));
    assert(!session_set_blocked_characters(&g_session1, NULL, 28));
    assert(!memcmp(before, &g_session1, sizeof *before));
    free(before);
}

static void shortcut_records(void)
{
    unsigned char keys[38 * sizeof(struct HotkeySlot)];
    struct Session *expected = malloc(sizeof *expected);
    size_t i;
    assert(expected);
    for (i = 0; i < sizeof keys; ++i) keys[i] = (unsigned char)(i * 59 + 17);
    key_bytes = keys;
    memset(&g_session1, 0x3a, sizeof g_session1);
    g_session1.hotkeyPage0 = 2; g_session1.hotkeyPage1 = 3;
    g_windowMgr.short_cut_wnd = (struct UIWindow *)&window_marker;
    memcpy(expected, &g_session1, sizeof *expected);
    memcpy(expected->hotkeys0, keys, sizeof expected->hotkeys0);
    memcpy(expected->quickSlots, keys + sizeof expected->hotkeys0, sizeof expected->quickSlots);
    expected_tab = 0; expected_rotate = 7; clear_step = 0;
    session_set_shortcut_keys(5, keys, 0);
    assert(clear_step == 2 && messages == 1 && !memcmp(expected, &g_session1, sizeof *expected));
    memcpy(expected->hotkeys1, keys, sizeof expected->hotkeys1);
    expected_tab = 1; expected_rotate = 6; clear_step = 0;
    session_set_shortcut_keys(5, keys, 1);
    assert(clear_step == 1 && messages == 2 && !memcmp(expected, &g_session1, sizeof *expected));
    expected_tab = 7; expected_rotate = 9; clear_step = 0;
    session_set_shortcut_keys(5, keys, 7);
    assert(!clear_step && messages == 3 && !memcmp(expected, &g_session1, sizeof *expected));
    g_windowMgr.short_cut_wnd = NULL;
    session_set_shortcut_keys(5, keys, 7);
    assert(messages == 3);
    free(expected);
}

static void retired_trait_costs(void)
{
    enum { RETIRED_SP_UPOW = 247 };
    struct PACKET_ZC_STATUS_CHANGE packet = {0};
    struct Session *expected = malloc(sizeof *expected);
    int sp;
    assert(expected);
    memset(&g_session1, 0xc7, sizeof g_session1);
    memset(&g_windowMgr, 0, sizeof g_windowMgr);
    memcpy(expected, &g_session1, sizeof *expected);
    for (sp = RETIRED_SP_UPOW - 1; sp <= RETIRED_SP_UPOW + 6; ++sp) {
        packet.sp = (unsigned short)sp; packet.value = (unsigned char)sp;
        game_mode_zc_status_change(NULL, &packet);
        assert(notifications == 0 && !memcmp(expected, &g_session1, sizeof *expected));
    }
    free(expected);
}

int main(void)
{
    blocked_characters(); shortcut_records(); retired_trait_costs();
    puts("Session fields: bounded blocked characters, full-width Korean names, packed hotkeys, retired trait costs ignored passed");
    return 0;
}
