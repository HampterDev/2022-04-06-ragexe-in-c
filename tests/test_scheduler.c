#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h>
#include "owned_storage_test_allocator.h"
#include "contents/schedule.h"

static __time32_t clock_seconds = 100;
static unsigned clock_reads, ticks_ms;
static __time32_t fixture_time(__time32_t *out)
{
    ++clock_reads;
    if (out) *out = clock_seconds;
    return clock_seconds;
}
static DWORD fixture_ticks(void) { return ticks_ms; }
static void *checked_realloc(void *old, size_t size)
{
    void *next = checked_malloc(size);
    if (!next) return NULL;
    if (old) {
        size_t previous = ((struct Allocation *)old - 1)->size;
        memcpy(next, old, previous < size ? previous : size);
        checked_free(old);
    }
    return next;
}
#define malloc checked_malloc
#define free checked_free
#define realloc checked_realloc
#define _time32 fixture_time
#define timeGetTime fixture_ticks
#include "../src/contents/schedule.c"
#include "../src/contents/time_scheduler_tick.c"
#include "../src/contents/shutdown_notice.c"
#include "../src/game/ping_schedule.c"
#include "../src/game/packets_macro_detector.c"
#include "../src/contents/ranking.c"
#include "../src/contents/ranking_output.c"
#include "../src/session/session_ranking.c"
#undef timeGetTime
#undef _time32
#undef free
#undef malloc
#undef realloc

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
struct ModeMgr g_modeMgr;
static struct UIWindow register_window;
static struct CaptchaData registered_image;
static int window_present, connected = 1, game_present = 1;
static unsigned chat_lines, boxes, disconnects, opens, requests, packets, packet_bytes;
static int last_request, last_msg;
static unsigned char wire[8192];
static int lengths[16];
static char notice_text[1024];
static unsigned id_counter;
static unsigned string_blocks;

void *mem_alloc(size_t size) { void *p = malloc(size); assert(p); ++string_blocks; return p; }
void mem_free(void *p)
{  assert(p && string_blocks); --string_blocks; free(p); }
void fatal_error(const char *message) { (void)message; abort(); }
struct TextView player_name_lookup(unsigned id) { assert(id == 1); return text_view("", 0); }

struct Mode *client_current_mode(void) { assert(0); return NULL; }
void mode_mgr_quit(struct ModeMgr *m) { (void)m; assert(0); }
char connection_ws2_unhooked(void) { assert(0); return 0; }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *w, int id) { (void)w; (void)id; assert(0); return 0; }
int crt_memcpy_s(void *d, size_t size, const void *s, size_t n) { (void)d; (void)size; (void)s; (void)n; assert(0); return 0; }
int captcha_preview_begin(struct UIWindow *w, const void *key, size_t size) { (void)w; (void)key; (void)size; assert(0); return 0; }
int captcha_detector_begin(struct UIWindow *w, const void *key, size_t size) { (void)w; (void)key; (void)size; assert(0); return 0; }
void captcha_detector_append(struct UIWindow *w, const void *key, const void *data, size_t size) { (void)w; (void)key; (void)data; (void)size; assert(0); }
void captcha_register_set_send_disabled(struct UIWindow *w, unsigned char flag) { assert(w == &register_window && !flag); }
const struct CaptchaData *captcha_register_data(struct UIWindow *w) { assert(w == &register_window); return &registered_image; }

int schedule_next_id(void) { return (int)++id_counter; }
struct Mode *game_current_base_mode(void) { return game_present ? (struct Mode *)1 : NULL; }
int mode_send_message(struct Mode *mode, int msg, int a, int b, int c, int d)
{
    assert(mode && a == 1 && !b && !c && !d); ++requests; last_request = msg; return 0;
}
struct RagConnection *rag_connection_instance(void) { return (struct RagConnection *)2; }
struct Connection *rag_connection_transport(struct RagConnection *c) { assert(c == (void *)2); return (struct Connection *)3; }
int connection_is_open(const struct Connection *c) { assert(c == (void *)3); return connected; }
void connection_disconnect(struct Connection *c) { assert(c == (void *)3); ++disconnects; }
char rag_connection_send_packet(struct RagConnection *c, int length, const void *data)
{
    assert(c == (void *)2 && packets < 16 && packet_bytes + length <= sizeof wire);
    lengths[packets++] = length;
    memcpy(wire + packet_bytes, data, length); packet_bytes += length; return 1;
}
struct UIWindow *ui_window_find(int id)
{
    assert(id == 0x11c); return window_present ? (struct UIWindow *)&register_window : NULL;
}
struct UIWindow *ui_window_open(int id) { assert(id == 0x14c); ++opens; return NULL; }
char *msg_str(int id)
{
    last_msg = id;
    if (id == 0x92c) return "%04d-%02d-%02d %02d:%02d:%02d";
    if (id == 0x92d) return "%d hours %d minutes";
    return "message";
}
int ui_window_mgr_send_msg_(struct UIWindowMgr *w, int msg, int text, int color, int c, int d)
{
    assert(w == &g_windowMgr && msg == 1 && text && !c && !d);
    (void)color; ++chat_lines; return 0;
}
int ui_window_mgr_message_box(struct UIWindowMgr *w, const char *text, int type, int a, int b, int c,
                          const char *title, int width, int height, int d)
{
    assert(w == &g_windowMgr && !type && a == 1 && b == 1 && !c && !d);
    assert(!strcmp(title, "메시지") && width == 0x118 && height == 0x78);
    strcpy(notice_text, text); ++boxes; return 0;
}

static int events[128], event_count;
static struct TimeScheduler queue;
static void record_run(struct Schedule *s, int now)
{
    events[event_count++] = s->tag;
    if (s->tag == 1) {
        struct Schedule *added = schedule_create(now, 1, SCHEDULE_REPEAT, s->ops);
        assert(added); added->tag = 3;
        assert(time_scheduler_add(&queue, added));
        added->last_run = 0;
        s->alive = 0;
    }
    if (s->tag == 4) queue.paused = 1;
    if (s->tag == 6) { time_scheduler_clear(&queue); assert(queue.count); }
    s->last_run = now;
}
static void record_destroy(struct Schedule *s)
{
    events[event_count++] = -s->tag;
    checked_free(s);
}
static const struct ScheduleOps record_ops = {record_run, record_destroy};
static struct Schedule *enqueue(int tag)
{
    struct Schedule *s = schedule_create(0, 1, SCHEDULE_REPEAT, &record_ops);
    assert(s); s->tag = tag; assert(time_scheduler_add(&queue, s)); s->last_run = 0; return s;
}

static void test_queue(void)
{
    struct Schedule *a, *b;
    unsigned before;
    time_scheduler_construct(&queue);
    assert(!time_scheduler_find(NULL, 1));
    time_scheduler_tick(NULL); time_scheduler_clear(NULL);
    a = enqueue(1); b = enqueue(2);
    assert(time_scheduler_find(&queue, a->id) == a);
    assert(!time_scheduler_add(&queue, a) && queue.count == 2);
    before = clock_reads; time_scheduler_tick(&queue);
    assert(clock_reads - before == 4);
    assert(queue.count == 3 && event_count == 3 && events[0] == 1 && events[1] == 2 && events[2] == 3);
    assert(!a->alive && time_scheduler_find(&queue, a->id) == a);
    event_count = 0; b->paused = 1; before = clock_reads;
    time_scheduler_tick(&queue);
    assert(event_count == 1 && events[0] == -1 && queue.count == 2 && clock_reads - before == 2);
    time_scheduler_clear(&queue);
    assert(events[1] == -2 && events[2] == -3 && !queue.first && !queue.last && !queue.count && !outstanding);
    time_scheduler_clear(&queue); time_scheduler_destruct(&queue);

    event_count = 0; enqueue(4); enqueue(5); time_scheduler_tick(&queue);
    assert(event_count == 2 && events[0] == 4 && events[1] == 5 && queue.paused);
    before = clock_reads; time_scheduler_tick(&queue); assert(clock_reads == before);
    time_scheduler_clear(&queue); assert(!queue.paused);
    event_count = 0; enqueue(6); enqueue(7); time_scheduler_tick(&queue);
    assert(event_count == 3 && events[0] == 6 && events[1] == -6 && events[2] == -7 && !outstanding);
    event_count = 0;

    a = schedule_create(5, 1, SCHEDULE_REPEAT, NULL); assert(a);
    fail_at = allocation_calls + 1; assert(!time_scheduler_add(&queue, a));
    assert(a->last_run == 5 && !queue.count && outstanding == 1); fail_at = 0;
    schedule_destroy(a); assert(!outstanding);
    fail_at = allocation_calls + 1; assert(!time_scheduler_create() && !g_timeScheduler);
    fail_at = 0; assert(time_scheduler_create() && !time_scheduler_create());
    time_scheduler_destroy(); time_scheduler_destroy(); assert(!outstanding && !g_timeScheduler);
}

static void test_clocks_and_jobs(void)
{
    struct Schedule s;
    struct PartialShutDownSchedule partial;
    unsigned before;
    schedule_construct(&s, 100, 5, SCHEDULE_ONCE, 1, 0);
    assert(!schedule_is_due(&s, 105) && schedule_is_due(&s, 106));
    s.last_run = INT_MAX - 1; assert(!schedule_is_due(&s, INT_MAX));
    s.interval = 0; assert(!schedule_is_due(&s, INT_MAX));
    schedule_run(&s, 200); assert(!s.alive && s.last_run == INT_MAX - 1);
    schedule_construct(&s, 100, 1, SCHEDULE_RANDOM, 1, 0);
    for (before = 0; before < 1000; ++before) { schedule_run(&s, 200); assert(s.interval >= 30 && s.interval <= 330 && s.last_run == 200); }

    partial_shutdown_construct(&partial, 100, 3600, SCHEDULE_REPEAT);
    assert(!partial_shutdown_start(&partial, 50, 100));
    assert(partial_shutdown_start(&partial, 7100, 100) && partial.base.interval == 4000);
    assert(partial_shutdown_start(&partial, 1900, 100) && partial.stage == 1 && partial.base.interval == 900);
    partial_shutdown_run(&partial, 101); assert(partial.remaining == 1799 && partial.stage == 2);
    partial.stage = 6; partial_shutdown_run(&partial, 101); assert(!partial.base.alive);
    fail_at = allocation_calls + 1; assert(!partial_shutdown_create(100, 1, SCHEDULE_REPEAT, 200)); fail_at = 0;
    assert(!partial_shutdown_create(100, 1, SCHEDULE_REPEAT, 50) && !outstanding);
}

static void test_captcha(void)
{
    unsigned char data[2051], key[] = {0, 0x78, 0x56, 0x34, 0x12};
    struct CaptchaUploadSchedule *upload;
    unsigned i, before, offset = 0;
    for (i = 0; i < sizeof data; ++i) data[i] = (unsigned char)i;
    assert(!captcha_upload_create(0, 1, -1, data) && !captcha_upload_create(0, 1, 1, NULL));
    for (i = 1; i <= 2; ++i) { fail_at = allocation_calls + i; assert(!captcha_upload_create(0, 1, sizeof data, data)); assert(!outstanding); }
    fail_at = 0;
    upload = captcha_upload_create(0, 0x12345678, sizeof data, data); assert(upload);
    assert(time_scheduler_add(&queue, &upload->base));
    clock_seconds = 102; packets = packet_bytes = 0; before = allocation_calls;
    for (i = 0; i < 3; ++i) time_scheduler_tick(&queue);
    assert(packets == 3 && lengths[0] == 1032 && lengths[1] == 1032 && lengths[2] == 11);
    assert(allocation_calls == before && queue.count == 1 && !upload->base.alive && upload->base.last_run == 100);
    for (i = 0; i < packets; ++i) {
        int wire_key; unsigned short id, len;
        memcpy(&id, wire + offset, 2); memcpy(&len, wire + offset + 2, 2); memcpy(&wire_key, wire + offset + 4, 4);
        assert(id == 0xa54 && len == lengths[i] && wire_key == 0x12345678);
        assert(!memcmp(wire + offset + 8, data + i * 1024, lengths[i] - 8)); offset += lengths[i];
    }
    time_scheduler_tick(&queue); assert(!queue.count && !outstanding);
    upload = captcha_upload_create(0, 1, 0, NULL); assert(upload);
    captcha_upload_run(upload); assert(!upload->base.alive); schedule_destroy(&upload->base);

    g_timeScheduler = &queue; window_present = 0; before = chat_lines;
    game_mode_zc_ack_upload_macro_detector(NULL, (char *)key + 1, 0);
    assert(chat_lines == before + 1 && last_msg == 0xb41 && !outstanding);
    window_present = 1; registered_image.bytes = data; registered_image.size = sizeof data;
    for (i = 1; i <= 3; ++i) {
        fail_at = allocation_calls + i;
        game_mode_zc_ack_upload_macro_detector(NULL, (char *)key + 1, 0);
        assert(!queue.count && !outstanding);
    }
    fail_at = 0; game_mode_zc_ack_upload_macro_detector(NULL, (char *)key + 1, 0);
    assert(queue.count == 1); upload = (struct CaptchaUploadSchedule *)queue.first->job;
    assert(upload->key == 0x12345678 && upload->data != data && !memcmp(upload->data, data, sizeof data));
    memset(data, 0, sizeof data); assert(upload->data[1] == 1);
    time_scheduler_clear(&queue); g_timeScheduler = NULL;
}

static void test_consumers(void)
{
    unsigned char packet[11] = {0};
    __time32_t end = 1767268800, now = end - 7200;
    unsigned i, before;
    struct Schedule *ping;
    memcpy(packet + 3, &end, 4); memcpy(packet + 7, &now, 4);
    g_timeScheduler = &queue;
    for (i = 0; i < 10; ++i) assert(!shutdown_notice_receive(packet + 1, i));
    assert(!shutdown_notice_receive(NULL, 10));
    for (i = 1; i <= 2; ++i) {
        fail_at = allocation_calls + i; assert(shutdown_notice_receive(packet + 1, 10));
        assert(!outstanding && !queue.count);
    }
    fail_at = 0; assert(shutdown_notice_receive(packet + 1, 10));
    assert(!strcmp(notice_text, "2026-01-01 12:00:00") && queue.count == 1);
    assert(((struct PartialShutDownSchedule *)queue.first->job)->end == end);
    time_scheduler_clear(&queue);
    for (i = 1; i <= 4; ++i) {
        g_session1.gameModeScheduleId = -1; fail_at = allocation_calls + i;
        game_mode_add_schedules();
        if (i >= 3) assert(g_session1.gameModeScheduleId == -1);
        else assert(time_scheduler_find(&queue, g_session1.gameModeScheduleId));
        time_scheduler_clear(&queue); assert(!outstanding);
    }
    fail_at = 0; game_mode_add_schedules();
    ping = time_scheduler_find(&queue, g_session1.gameModeScheduleId); assert(ping && queue.count == 2);
    connected = 1; g_session1.lastPacketTime = UINT_MAX - 100; g_session1.lastSendTime = UINT_MAX - 20000;
    ticks_ms = 100; packets = packet_bytes = 0; ping_run(ping, 1);
    assert(packets == 1 && lengths[0] == 2 && wire[0] == 0x1c && wire[1] == 0x0b && ping->alive);
    before = disconnects; ticks_ms = 60000; ping_run(ping, 2); assert(disconnects == before + 1 && !ping->alive && opens);
    time_scheduler_clear(&queue); g_timeScheduler = NULL; assert(!outstanding);
}

static void test_ranking_queue(void)
{
    struct RankPacket packet = {0};
    struct RankingSchedule *ranking;
    unsigned i, before;
    packet.packet_id = 0x97d; packet.char_ids[0] = 1;
    g_timeScheduler = &queue; clock_seconds = 100;
    for (i = 1; i <= 2; ++i) {
        g_session1.rankingScheduleId = -1;
        fail_at = allocation_calls + i;
        session_on_ranking_packet(&g_session1, 1, &packet);
        assert(!queue.count && !outstanding && g_session1.rankingScheduleId == -1);
    }
    fail_at = 0;
    session_on_ranking_packet(&g_session1, 1, &packet);
    ranking = (struct RankingSchedule *)time_scheduler_find(&queue, g_session1.rankingScheduleId);
    assert(ranking && ranking->base.ops == &ranking_ops);
    packet.char_ids[0] = 9; assert(ranking->rank.char_ids[0] == 1);
    clock_seconds = 102; before = chat_lines; time_scheduler_tick(&queue);
    assert(ranking->base.alive && chat_lines == before + 1 && last_msg == 0xd99);
    clock_seconds = 103; before = chat_lines; time_scheduler_tick(&queue);
    assert(!ranking->base.alive && chat_lines == before + 14 && queue.count == 1);
    time_scheduler_tick(&queue); assert(!outstanding && !string_blocks && !queue.count);
    g_timeScheduler = NULL;
}

int main(void)
{
    _putenv("TZ=UTC0"); _tzset();
    test_queue(); test_clocks_and_jobs(); clock_seconds = 100;
    test_captcha(); test_consumers(); test_ranking_queue();
    assert(!outstanding);
    puts("Scheduler: real queue/tick ownership, failures, callbacks, clocks, all jobs, captcha key/chunks and caller cleanup passed");
    return 0;
}
