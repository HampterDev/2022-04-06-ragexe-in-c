#include <stdint.h>
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
static uint32_t fixture_ticks(void) { return ticks_ms; }
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
#define platform_ticks_ms fixture_ticks
#include "../src/contents/schedule.c"
#include "../src/contents/time_scheduler_tick.c"
#include "../src/game/ping_schedule.c"
#include "../src/contents/ranking.c"
#include "../src/contents/ranking_output.c"
#include "../src/session/session_ranking.c"
#undef timeGetTime
#undef platform_ticks_ms
#undef _time32
#undef free
#undef malloc
#undef realloc

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static int connected = 1;
static unsigned chat_lines, disconnects, opens, packets, packet_bytes;
static int last_msg;
static unsigned char wire[8192];
static int lengths[16];
static unsigned id_counter;
static unsigned string_blocks;

void *mem_alloc(size_t size) { void *p = malloc(size); assert(p); ++string_blocks; return p; }
void mem_free(void *p)
{  assert(p && string_blocks); --string_blocks; free(p); }
void fatal_error(const char *message) { (void)message; abort(); }
struct TextView player_name_lookup(unsigned id) { assert(id == 1); return text_view("", 0); }

int schedule_next_id(void) { return (int)++id_counter; }

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

struct UIWindow *ui_window_open(int id) { assert(id == 0x14c); ++opens; return NULL; }
char *msg_str(int id)
{
    last_msg = id;
    return "message";
}
int ui_window_mgr_send_msg_(struct UIWindowMgr *w, int msg, int text, int color, int c, int d)
{
    assert(w == &g_windowMgr && msg == 1 && text && !c && !d);
    (void)color; ++chat_lines; return 0;
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
    schedule_construct(&s, 100, 5, SCHEDULE_ONCE, 1, 0);
    assert(!schedule_is_due(&s, 105) && schedule_is_due(&s, 106));
    s.last_run = INT_MAX - 1; assert(!schedule_is_due(&s, INT_MAX));
    s.interval = 0; assert(!schedule_is_due(&s, INT_MAX));
    schedule_run(&s, 200); assert(!s.alive && s.last_run == INT_MAX - 1);
}

static void test_consumers(void)
{
    unsigned i, before;
    struct Schedule *ping;
    g_timeScheduler = &queue;
    for (i = 1; i <= 2; ++i) {
        g_session1.gameModeScheduleId = -1; fail_at = allocation_calls + i;
        game_mode_add_schedules();
        assert(g_session1.gameModeScheduleId == -1 && !queue.count);
        time_scheduler_clear(&queue); assert(!outstanding);
    }
    fail_at = 0; game_mode_add_schedules();
    ping = time_scheduler_find(&queue, g_session1.gameModeScheduleId);
    assert(ping && queue.count == 1 && ping->ops == &ping_ops);
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
    test_queue(); test_clocks_and_jobs(); clock_seconds = 100;
    test_consumers(); test_ranking_queue();
    assert(!outstanding);
    puts("Scheduler: real queue/tick ownership, failures, callbacks, clocks, heartbeat, ranking and caller cleanup passed");
    return 0;
}
