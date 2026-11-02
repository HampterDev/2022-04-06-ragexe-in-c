#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "session/state.h"
#include "contents/ranking.h"
#include "owned_storage_test_allocator.h"

static void *checked_realloc(void *old, size_t size)
{
    void *next = checked_malloc(size);
    if (!next) return NULL;
    if (old) {
        size_t before = ((struct Allocation *)old - 1)->size;
        memcpy(next, old, before < size ? before : size);
        checked_free(old);
    }
    return next;
}

#define malloc checked_malloc
#define realloc checked_realloc
#define free checked_free
#include "../src/contents/ranking.c"
#include "../src/contents/ranking_output.c"
#undef free
#undef realloc
#undef malloc

static int32_t ticks = 100;
static int32_t fixture_time(void) { return ticks; }
#define platform_time_now fixture_time
#include "../src/session/session_ranking.c"
#undef platform_time_now
#include "../src/game/mode_ranking.c"

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
static struct TimeScheduler scheduler;
struct TimeScheduler *g_timeScheduler = &scheduler;
static struct Schedule *queued;
static unsigned queue_adds, next_id = 1, string_blocks, line_count, lookups[4];
static int last_msg;
static const char *names[4];
static char *lines[32];
static int colors[32];

void *mem_alloc(size_t size)
{
    void *p = malloc(size); assert(p); ++string_blocks; return p;
}
void mem_free(void *p)
{
     assert(p && string_blocks); --string_blocks; free(p);
}
void fatal_error(const char *message) { (void)message; abort(); }
int schedule_next_id(void) { return (int)next_id++; }
struct Schedule *schedule_construct(struct Schedule *s, int now, int interval, int kind, int id, int tag)
{
    *s = (struct Schedule){0}; s->last_run = now; s->interval = interval;
    s->kind = kind; s->id = id; s->tag = tag; s->alive = 1; return s;
}
struct Schedule *time_scheduler_find(struct TimeScheduler *s, int id)
{
    assert(s == &scheduler); return queued && queued->id == id ? queued : NULL;
}
struct Schedule *time_scheduler_add(struct TimeScheduler *s, struct Schedule *job)
{
    assert(s == &scheduler && !queued); ++queue_adds; queued = job;
    job->last_run = ticks; return job;
}
struct TextView player_name_lookup(unsigned id)
{
    assert(id > 0 && id < 4); ++lookups[id];
    return text_view_cstr(names[id] ? names[id] : "");
}
char *msg_str(int id)
{
    last_msg = id;
    if (id == 0x245) return "이름없음";
    if (id == 0xd99) return "waiting";
    assert(id == 0x385 || id == 0x386 || id == 0x39e || id == 0x3b9);
    return "points %d total %d";
}
int ui_window_mgr_send_msg_(struct UIWindowMgr *s, int msg, int a, int color, int c, int d)
{
    const char *text = (const char *)a;
    assert(s == &g_windowMgr && msg == 1 && !c && !d && line_count < 32);
    lines[line_count] = malloc(strlen(text) + 1); assert(lines[line_count]);
    strcpy(lines[line_count], text); colors[line_count++] = color; return 0;
}
static void clear_lines(void)
{
    while (line_count) free(lines[--line_count]);
}
static void clear_queue(void)
{
    if (queued) ranking_destroy((struct RankingSchedule *)queued);
    queued = NULL;
}

static void test_storage(void)
{
    struct RankingSchedule *r;
    fail_at = allocation_calls + 1;
    assert(!ranking_create(100, 1, SCHEDULE_REPEAT)); fail_at = 0;
    r = ranking_create(100, 1, SCHEDULE_REPEAT); assert(r);
    assert(r->base.last_run == 100 && r->base.interval == 1 && r->base.alive);
    assert(!r->nameless && !r->nameless_count && !r->rank.packet_id);
    ranking_destruct(r); ranking_destruct(r);
    assert(!r->nameless && !r->nameless_count && !r->nameless_capacity);
    ranking_destroy(r); assert(!outstanding);
}

static void test_names_and_timeout(void)
{
    struct RankingSchedule r;
    struct RankPacket p = {0};
    char long_name[5001];
    unsigned before;
    p.packet_id = 1; p.char_ids[0] = 1; p.char_ids[1] = 2;
    p.points[0] = 42; p.my_points = -7;
    ranking_construct(&r, 100, 1, SCHEDULE_REPEAT); r.timeout = 3;
    ranking_set(&r, &p); p.char_ids[0] = 0;
    assert(r.rank.char_ids[0] == 1);
    names[1] = "패트릭%p"; names[2] = NULL;
    ranking_run(&r, 102);
    assert(r.base.alive && line_count == 1 && !strcmp(lines[0], "waiting"));
    clear_lines();
    ranking_run(&r, 103);
    assert(!r.base.alive && line_count == 14 && strstr(lines[1], "패트릭%p"));
    assert(strstr(lines[2], "이름없음") && strstr(lines[12], "-7 POINT"));
    clear_lines();
    memset(long_name, 'K', 5000); long_name[5000] = 0; names[1] = long_name; names[2] = "Other";
    r.base.alive = 1; ranking_run(&r, 101);
    assert(!r.base.alive && line_count == 14 && strlen(lines[1]) > 5000);
    clear_lines();
    before = outstanding; fail_at = allocation_calls + 1;
    assert(!ranking_line(1, "%s", long_name)); fail_at = 0;
    assert(!line_count && outstanding == before);
    names[1] = "prefix<NAMELESS>suffix";
    r.base.alive = 1; ranking_run(&r, 101);
    assert(!r.base.alive && strstr(lines[1], "이름없음")); clear_lines();
    r.rank.packet_id = 0; r.base.alive = 1; ranking_run(&r, 100);
    assert(!r.base.alive && !line_count);
    ranking_destruct(&r); assert(!outstanding && !string_blocks);
    names[1] = "Name"; names[2] = "Other";
}

static void test_packets(void)
{
    static const int title_colors[] = {0x9b9b9b, 0x9bcd9b, 0xcd9b9b, 0x9b9b9b};
    static const int line_colors[] = {0xcdcdcd, 0xcdffcd, 0xffcdcd, 0xcdcdcd};
    static const int messages[] = {0x385, 0x386, 0x39e, 0x3b9};
    struct PacketAckRanking p = {0};
    struct PacketUpdateRankingPoint points = {0};
    struct RankPacket ids = {0};
    unsigned char unaligned[sizeof p + 1];
    unsigned i, before;
    memset(p.names[0], 'A', sizeof p.names[0]);
    memset(p.names[1], 'B', sizeof p.names[1]); p.points[0] = 17; p.my_points = 25;
    memcpy(unaligned + 1, &p, sizeof p);
    assert(!ranking_receive_names(NULL, sizeof p));
    for (i = 0; i < sizeof p; ++i) assert(!ranking_receive_names(unaligned + 1, i));
    assert(!line_count);
    for (i = 0; i < 4; ++i) {
        p.type = (short)i; memcpy(unaligned + 1, &p, sizeof p);
        assert(ranking_receive_names(unaligned + 1, sizeof p));
        assert(line_count == 14 && colors[0] == title_colors[i] && colors[1] == line_colors[i]);
        assert(strstr(lines[1], "AAAAAAAAAAAAAAAAAAAAAAAA") && !strchr(lines[1], 'B'));
        assert(!strcmp(lines[0], ranking_style((int)i)->title)); clear_lines();
        points.type = (short)i; points.points = -3; points.total = 456;
        assert(ranking_receive_points(&points, sizeof points) && last_msg == messages[i]);
        assert(line_count == 1 && !strcmp(lines[0], "points -3 total 456") && colors[0] == line_colors[i]);
        clear_lines();
    }
    p.type = -1; assert(ranking_receive_names(&p, sizeof p));
    assert(!strcmp(lines[0], RANK_SEPARATOR) && !colors[0] && !colors[1]); clear_lines();
    assert(ranking_style(INT_MIN)->title_color == 0 && ranking_style(INT_MAX)->line_color == 0);
    for (i = 0; i < sizeof points; ++i) assert(!ranking_receive_points(&points, i));
    assert(!ranking_receive_points(NULL, sizeof points)); points.type = 99;
    assert(ranking_receive_points(&points, sizeof points) && !line_count);
    ids.packet_id = 1; ids.char_ids[0] = 1; ids.rank_type = 2;
    memcpy(unaligned + 1, &ids, sizeof ids);
    for (i = 0; i < sizeof ids; ++i) assert(!ranking_receive_ids(unaligned + 1, i));
    assert(!ranking_receive_ids(NULL, sizeof ids));
    g_session1.rankingScheduleId = -1; before = next_id;
    fail_at = allocation_calls + 1;
    assert(ranking_receive_ids(&ids, sizeof ids)); fail_at = 0;
    assert(!queued && next_id == before && g_session1.rankingScheduleId == -1);
    assert(ranking_receive_ids(unaligned + 1, sizeof ids));
    assert(queued && queue_adds == 1 && g_session1.rankingScheduleId == queued->id);
    assert(queued->kind == SCHEDULE_REPEAT && queued->interval == 1);
    assert(((struct RankingSchedule *)queued)->timeout == 3);
    memset(unaligned, 0, sizeof unaligned);
    assert(((struct RankingSchedule *)queued)->rank.char_ids[0] == 1);
    before = allocation_calls; assert(ranking_receive_ids(&ids, sizeof ids));
    assert(allocation_calls == before && queue_adds == 1);
    session_on_ranking_packet(&g_session1, 0, NULL); assert(!queued->alive);
    clear_queue(); clear_lines();
    session_on_ranking_packet(&g_session1, 1, NULL); assert(!queued);
    assert(!outstanding && !string_blocks);
}

int main(void)
{
    test_storage(); test_names_and_timeout(); test_packets();
    puts("Ranking: C ownership, long/Korean names, timeout order, both style paths, bounded packets and session scheduling passed");
    return 0;
}
