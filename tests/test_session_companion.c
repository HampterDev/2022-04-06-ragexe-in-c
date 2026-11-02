#include "ui/window_messages.h"
#include "game/companion_packets.h"
#include "session/state.h"
#include "ui/window_manager_state.h"
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

int g_serviceType;
struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static int redraws, skill_updates, warnings;

void session_invalidate_homun_info_wnd(struct Session *session)
{
    assert(session == &g_session1);
    ++redraws;
}

void session_invalidate_windows(struct Session *session)
{
    assert(session == &g_session1);
    ++skill_updates;
}

int crt_sprintf(char *out, const char *format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = vsprintf(out, format, args);
    va_end(args);
    return result;
}

int crt_snprintf_s(char *out, size_t size, size_t count, const char *format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = _vsnprintf_s(out, size, count, format, args);
    va_end(args);
    return result;
}

char *msg_str(int id)
{
    assert(id == 0x3fa);
    return "Hungry";
}

intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *manager, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(manager == &g_windowMgr && msg == 1 && b == 0xff && c == 0 && d == 0);
    assert(strcmp((const char *)a, "Hungry") == 0);
    ++warnings;
    return 0;
}

intptr_t ui_window_send(struct UIWindow *window, struct UIWindow *sender, int message,
                   intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(!"The skill window is absent in this fixture");
    return 0;
}

static void put_short(unsigned char *packet, size_t offset, short value)
{
    memcpy(packet + offset, &value, sizeof value);
}

static void put_int(unsigned char *packet, size_t offset, int value)
{
    memcpy(packet + offset, &value, sizeof value);
}

static void put_long_long(unsigned char *packet, size_t offset, long long value)
{
    memcpy(packet + offset, &value, sizeof value);
}

static void make_packet(unsigned char packet[0x55])
{
    int i;
    memset(packet, 0, 0x55);
    memcpy(packet + 2, "Companion", sizeof "Companion");
    packet[0x1a] = 1;
    put_short(packet, 0x1b, 123);
    put_short(packet, 0x1d, 5);
    put_short(packet, 0x1f, 900);
    for (i = 0; i < 8; ++i)
        put_short(packet, 0x21 + 2 * i, (short)(120 + i));
    put_int(packet, 0x31, 80000);
    put_int(packet, 0x35, 120000);
    put_int(packet, 0x39, 90000);
    put_int(packet, 0x3d, 130000);
}

int main(void)
{
    unsigned char packet[0x55];
    const long long experience = 0x12345678abcdef01LL;
    const long long threshold = 0x23456789bcdef012LL;
    int i;

    memset(&g_session1, 0, sizeof g_session1);
    g_session1.baseExp = 0x1020304050607080LL;
    g_session1.homunItem = 571;
    g_session1.homunMaxFullness = 100;
    g_session1.mercAid = 4567;
    make_packet(packet);
    put_long_long(packet, 0x41, experience);
    put_long_long(packet, 0x49, threshold);
    put_short(packet, 0x51, 15);
    put_short(packet, 0x53, 7);
    game_mode_zc_property_homun_no_accessory3(NULL, packet);
    assert(strcmp(g_session1.homunName, "Companion") == 0);
    assert(g_session1.homunRenamed == 1 && g_session1.homunLevel == 123);
    assert(g_session1.homunHp == 80000 && g_session1.homunMaxHp == 120000);
    assert(g_session1.homunSp == 90000 && g_session1.homunMaxSp == 130000);
    assert(g_session1.homunExp == experience && g_session1.homunMaxExp == threshold);
    assert(g_session1.homunFullness == 5 && g_session1.homunIntimacy == 900);
    assert(g_session1.homunSkillPoints == 15 && g_session1.homun_ai_state[2] == 7);
    for (i = 0; i < 8; ++i)
        assert(g_session1.homunStats[i] == 120 + i);
    assert(warnings == 1 && skill_updates == 1 && redraws == 7);
    assert(g_session1.baseExp == 0x1020304050607080LL);
    assert(g_session1.homunItem == 571 && g_session1.homunMaxFullness == 100);
    assert(g_session1.mercAid == 4567);

    make_packet(packet);
    put_int(packet, 0x41, -123);
    put_int(packet, 0x45, 0x76543210);
    put_short(packet, 0x49, 16);
    put_short(packet, 0x4b, 8);
    game_mode_zc_property_homun_no_accessory2(NULL, packet);
    assert(g_session1.homunExp == -123 && g_session1.homunMaxExp == 0x76543210LL);
    assert(g_session1.homunSkillPoints == 16 && g_session1.homun_ai_state[2] == 8);
    assert(warnings == 1 && skill_updates == 2 && redraws == 14);
    game_mode_zc_property_homun_no_accessory2(NULL, NULL);
    game_mode_zc_property_homun_no_accessory3(NULL, NULL);
    assert(skill_updates == 2 && redraws == 14 && g_session1.mercAid == 4567);
    puts("session companion: real packets, 64-bit experience, signed legacy values and adjacent fields passed");
    return 0;
}
