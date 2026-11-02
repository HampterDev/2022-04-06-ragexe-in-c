#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../src/game/quest_list.c"

struct RenewQuestMgr *g_questTable;
struct Session g_session1;
struct UIWindowMgr g_windowMgr;
const char *session_map_display_name(const char *name) { abort(); }
void mem_free(void *memory)
{ abort(); }
void string_free(struct String *s) { abort(); }
void string_assign(struct String *s, const char *text, size_t len) { abort(); }
void string_copy(struct String *s, const struct String *from) { abort(); }
void string_format(struct String *s, const char *format, ...) { abort(); }
struct UIWindow *ui_window_open(int id) { abort(); }
char *msg_str(int id) { abort(); }
int crt_snprintf_s(char *out, size_t size, size_t count, const char *format, ...) { abort(); }
char renew_quest_set_active(struct RenewQuestMgr *quests, unsigned id, char active) { abort(); }
void renew_quest_read_ongoing(struct RenewQuestMgr *quests, unsigned id, struct RenewQuestB *info) { abort(); }
void renew_quest_refresh_windows(void) { abort(); }
struct RenewQuestB *renew_quest_ongoing_info(struct RenewQuestMgr *quests, unsigned id) { abort(); }
intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *mgr, int msg, intptr_t a, intptr_t b, intptr_t c, intptr_t d) { abort(); }
const char *session_get_job_name(struct Session *s, unsigned job, int max_len) { abort(); }
struct RenewQuestBase *renew_quest_base_construct(struct RenewQuestBase *info) { abort(); }
void renew_quest_base_destruct(struct RenewQuestBase *info) { abort(); }
void vec_free(struct Vec *v) { abort(); }

static void expected_time(struct RenewQuestB *info, int seconds, int has_time)
{
    __time32_t timestamp = seconds;
    const struct tm *local = _localtime32(&timestamp);
    if (!local) {
        info->plain.deadline.year = info->plain.deadline.month = info->plain.deadline.day = 0;
        info->plain.deadline.hour = info->plain.deadline.minute = 0;
        return;
    }
    info->plain.deadline.year = (unsigned short)(local->tm_year + 1900);
    info->plain.deadline.month = (unsigned short)(local->tm_mon + 1);
    info->plain.deadline.day = (unsigned short)local->tm_mday;
    info->plain.deadline.hour = has_time ? (unsigned short)local->tm_hour : 0;
    info->plain.deadline.minute = has_time ? (unsigned short)local->tm_min : 0;
}

static void check_zone(const char *zone)
{
    static const int timestamps[] = {
        0, 1, 86399, 86400, 951782400, 1577934245,
        1583661599, 1583661600, 1593604800, 1604221199, 1604221200,
        2147480000, INT_MAX
    };
    struct RenewQuestB actual, expected;
    struct PacketAddQuest packet = {0};
    size_t i;
    assert(_putenv_s("TZ", zone) == 0);
    _tzset();
    for (i = 0; i < sizeof timestamps / sizeof timestamps[0]; ++i) {
        memset(&actual, 0xa5, sizeof actual);
        expected = actual;
        expected.base.active = 2;
        expected.plain.hunt_count = 3;
        expected_time(&expected, timestamps[i], timestamps[i] != 0);
        quest_info_set(&actual, 2, timestamps[i], 3);
        assert(!memcmp(&actual, &expected, sizeof actual));

        memset(&actual, 0x5a, sizeof actual);
        expected = actual;
        packet.active = 1;
        packet.hunt_count = 2;
        packet.start_time = INT_MIN;
        packet.end_time = timestamps[i];
        expected.base.active = packet.active;
        expected.plain.hunt_count = (unsigned char)packet.hunt_count;
        expected_time(&expected, timestamps[i], timestamps[i] != 0);
        quest_info_set_added(&actual, &packet);
        assert(!memcmp(&actual, &expected, sizeof actual));
    }

    memset(&actual, 0x3c, sizeof actual);
    expected = actual;
    expected.base.active = 1;
    expected.plain.hunt_count = 2;
    expected_time(&expected, 0, 1);
    quest_info_set(&actual, 1, INT_MIN, 2);
    assert(!memcmp(&actual, &expected, sizeof actual));

    for (i = 0; i < 2; ++i) {
        memset(&actual, 0x7e, sizeof actual);
        expected = actual;
        packet.end_time = i ? INT_MIN : -1;
        expected.base.active = packet.active;
        expected.plain.hunt_count = (unsigned char)packet.hunt_count;
        expected.plain.deadline.year = expected.plain.deadline.month = expected.plain.deadline.day = 0;
        expected.plain.deadline.hour = expected.plain.deadline.minute = 0;
        quest_info_set_added(&actual, &packet);
        assert(!memcmp(&actual, &expected, sizeof actual));
    }
}

int main(void)
{
    struct RenewQuestB info;
    check_zone("GMT0");
    quest_info_set(&info, 1, 1577934245, 0);
    assert(info.plain.deadline.year == 2020 && info.plain.deadline.month == 1 && info.plain.deadline.day == 2);
    assert(info.plain.deadline.hour == 3 && info.plain.deadline.minute == 4);
    check_zone("PST8PDT");
    check_zone("JST-9");
    puts("Quest deadlines: 78 runtime comparisons across three time zones, epoch, DST, 2038 limit, invalid dates and preserved quest fields passed");
    return 0;
}

struct RenewQuestB *quest_hunt_construct(struct RenewQuestB *info) { abort(); }
void quest_hunt_destroy(struct RenewQuestB *info) { abort(); }
void renew_quest_remove(struct RenewQuestMgr *quests, unsigned id) { abort(); }
