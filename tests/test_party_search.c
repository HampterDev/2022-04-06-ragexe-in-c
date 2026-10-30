#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "session/state.h"
#include "contents/party_booking.h"
#include "owned_storage_test_allocator.h"

struct Session g_session1;
static char job_name[400];
char *msg_str(int id) { (void)id; return job_name; }

static void *checked_calloc(size_t count, size_t size)
{
    void *result = checked_malloc(count * size);
    if (result) memset(result, 0, count * size);
    return result;
}

static void *checked_realloc(void *old, size_t size)
{
    void *result = checked_malloc(size);
    if (!result) return NULL;
    if (old) {
        size_t old_size = ((struct Allocation *)old - 1)->size;
        memcpy(result, old, old_size < size ? old_size : size);
        checked_free(old);
    }
    return result;
}

#define malloc checked_malloc
#define calloc checked_calloc
#define realloc checked_realloc
#define free checked_free
#include "../src/core/owned_text.c"
#include "../src/core/text_array.c"
#include "../src/contents/party_booking_storage.c"
#undef free
#undef realloc
#undef calloc
#undef malloc
#include "../src/contents/party_booking_regions.c"
#include "../src/contents/party_booking.c"

struct UIWindowMgr g_windowMgr;
static int window_visible, window_matches = 1, window_alarm, wave_count, notice_count;
static int window_marker;
struct UIWindow *ui_window_find(int id)
{
    assert(id == 0xa9);
    return window_visible ? (struct UIWindow *)&window_marker : NULL;
}
char party_booking_wnd_matches(struct UISeekPartyListWnd *window, struct PartyBookingPost p)
{
    assert(window == (struct UISeekPartyListWnd *)&window_marker);
    (void)p;
    return (char)window_matches;
}
void party_booking_wnd_alarm(struct UISeekPartyListWnd *window)
{
    assert(window == (struct UISeekPartyListWnd *)&window_marker);
    ++window_alarm;
}
void sound_play_wave(const char *name, float x, float y, float z, int max_distance, int full_distance, float volume)
{
    assert(!strcmp(name, "party_alarm.wav") && x == 0 && y == 0 && z == 0);
    assert(max_distance == 250 && full_distance == 40 && volume == 1);
    ++wave_count;
}
int ui_window_mgr_send_msg_(struct UIWindowMgr *self, int msg, int v1, int v2, int v3, int v4)
{
    assert(self == &g_windowMgr && msg == 1 && v1 == (int)job_name && v2 == 0xffff00 && v3 == 0xe && v4 == 0);
    ++notice_count;
    return 0;
}

static unsigned lua_calls;
static int lua_mode;
int client_lua_call(struct ClientLua *lua, const char *function, const char *format, ...)
{
    static char scratch[100];
    va_list args;
    int region, map = 0, *a, *b = NULL, *c0 = NULL, *c1 = NULL, *c2 = NULL;
    const char **name;
    (void)lua;
    ++lua_calls;
    memset(scratch, '!', sizeof scratch);
    va_start(args, format);
    region = va_arg(args, int);
    if (!strcmp(function, "queryMapInfo")) map = va_arg(args, int);
    a = va_arg(args, int *);
    if (map) b = va_arg(args, int *);
    name = va_arg(args, const char **);
    if (map) {
        c0 = va_arg(args, int *); c1 = va_arg(args, int *); c2 = va_arg(args, int *);
    }
    va_end(args);
    if (lua_mode == 1 || (lua_mode == 2 && map)) return 0;
    if (lua_mode == 3 && ((!map && region == 2) || map == 2)) return 0;
    *a = 0;
    if (b) *b = 0;
    if ((map && map > 2) || (!map && region > 3)) { *name = NULL; return 1; }
    sprintf(scratch, "%s%d-%d", map ? "지도" : "지역", region, map);
    *name = scratch;
    if (map) { *c0 = -1; *c1 = 258; *c2 = 3; }
    return 1;
}

static struct PartyBookingPost post(int id)
{
    struct PartyBookingPost result = {0};
    int i;
    result.id = id;
    memcpy(result.leader, "모험가", sizeof "모험가");
    result.posted_at = 1600000000;
    result.level = 95;
    result.map = 0x100;
    for (i = 0; i < PARTY_BOOKING_JOBS; ++i) result.jobs[i] = -1;
    return result;
}

static void prepare(struct PartyBooking *board)
{
    party_booking_construct(board);
    lua_mode = 0;
    assert(party_search_read_regions(NULL, 2, &board->region_names, &board->region_maps));
    g_session1.baseLevel3rd = 99;
}

static void test_loader(void)
{
    struct TextArray names = {0};
    struct PartySearchRegions regions = {0};
    const struct PartySearchMaps *maps;
    unsigned n, calls, live;
    lua_mode = 0;
    calls = allocation_calls;
    assert(party_search_read_regions(NULL, 1, &names, &regions));
    calls = allocation_calls - calls;
    assert(names.size == 3 && regions.size == 3);
    assert(!strcmp(owned_text_cstr(&names.items[1]), "지역2-0"));
    maps = party_search_region_maps(&regions, 3);
    assert(maps->size == 2 && maps->items[1].color == 0x0302ff);
    assert(!strcmp(owned_text_cstr(&maps->items[0].name), "지도3-1"));
    live = outstanding;
    for (n = 1; n <= calls; ++n) {
        struct OwnedText *old_names = names.items;
        struct PartySearchRegion *old_regions = regions.items;
        fail_at = allocation_calls + n;
        assert(!party_search_read_regions(NULL, 1, &names, &regions));
        fail_at = 0;
        assert(outstanding == live && names.items == old_names && regions.items == old_regions);
    }
    for (lua_mode = 1; lua_mode <= 2; ++lua_mode) {
        lua_calls = 0;
        assert(!party_search_read_regions(NULL, 1, &names, &regions));
        assert(lua_calls < 270 && outstanding == live && names.size == 3);
    }
    lua_mode = 3;
    assert(party_search_read_regions(NULL, 1, &names, &regions));
    assert(names.size == 2 && regions.size == 3);
    assert(!strcmp(owned_text_cstr(&names.items[1]), "지역3-0"));
    assert(party_search_region_maps(&regions, 1)->size == 1);
    assert(party_search_region_maps(&regions, 2)->size == 1);
    lua_mode = 0;
    assert(party_search_read_regions(NULL, 2, &names, &regions));
    assert(names.size == 2 && regions.size == 2);
    text_array_clear(&names); party_search_regions_clear(&regions);
    assert(!outstanding);
}

static void test_rows(void)
{
    struct PartyBooking board;
    struct PartyBookingPost p = post(10);
    struct PartySearchRow row = {0};
    unsigned n, calls, live;
    prepare(&board);
    strcpy(job_name, "검사");
    p.jobs[0] = 1;
    calls = allocation_calls;
    assert(party_booking_make_row(&board, &row, p));
    calls = allocation_calls - calls;
    assert(!strcmp(owned_text_cstr(&row.leader), "모험가"));
    assert(!strcmp(owned_text_cstr(&row.levels), "95~99"));
    assert(!strcmp(owned_text_cstr(&row.place), "지역2-0 지도2-1"));
    assert(!strcmp(owned_text_cstr(&row.jobs), "검사"));
    live = outstanding;
    for (n = 1; n <= calls; ++n) {
        char *old = row.leader.data;
        fail_at = allocation_calls + n;
        assert(!party_booking_make_row(&board, &row, p));
        fail_at = 0;
        assert(row.leader.data == old && outstanding == live);
    }
    memset(p.leader, 'x', sizeof p.leader);
    memset(job_name, 'a', sizeof job_name - 1); job_name[sizeof job_name - 1] = 0;
    for (n = 0; n < PARTY_BOOKING_JOBS; ++n) p.jobs[n] = (short)n;
    assert(party_booking_make_row(&board, &row, p));
    assert(row.leader.size == 24 && row.jobs.size == 2399);
    assert(!party_search_map_at(NULL, 0));
    p.map = 0; assert(!party_booking_make_row(&board, &row, p));
    assert(row.leader.size == 24 && row.jobs.size == 2399);
    p.map = 0x102; assert(!party_booking_make_row(&board, &row, p));
    p.map = -1; assert(!party_booking_make_row(&board, &row, p));
    p.map = 0x700; assert(!party_booking_make_row(&board, &row, p));
    p.map = 0x100; p.posted_at = -1; assert(!party_booking_make_row(&board, &row, p));
    p = post(10); p.jobs[0] = 1; strcpy(job_name, "a");
    assert(party_booking_make_row(&board, &row, p) && !strcmp(row.jobs.data, "a,"));
    party_search_row_clear(&row);
    party_booking_set_filter(&board, 90, 0x100, 1);
    assert(party_booking_filter_matches(&board, p));
    p.level = 106; assert(!party_booking_filter_matches(&board, p));
    party_booking_set_filter(&board, INT_MAX, -1, -1);
    assert(!party_booking_filter_matches(&board, p));
    party_booking_destruct(&board); assert(!outstanding);
}

static void test_pending(void)
{
    struct PartyBooking board;
    struct PartySearchRow row = {0};
    struct PartyBookingPost p = post(12);
    int i;
    prepare(&board);
    assert(owned_text_assign(&row.leader, text_view_cstr("pending")));
    for (i = 0; i < 13; ++i) assert(party_search_rows_insert(&board.pending, i, &row));
    fail_at = allocation_calls + 1;
    assert(party_booking_take_pending(&board) && board.pending.size == 13 && !board.rows.size);
    fail_at = 0;
    assert(party_booking_take_pending(&board) && board.rows.size == 11 && board.pending.size == 2);
    assert(!party_booking_take_pending(&board) && board.rows.size == 13 && !board.pending.size);
    assert(!party_booking_take_pending(&board));
    assert(party_search_rows_insert(&board.pending, 12, &row));
    assert(!party_booking_take_pending(&board) && board.rows.size == 13);
    party_booking_add_rows(&board, &p, 1, 1);
    assert(board.last_id == 12 && board.queue_new == 1 && board.rows.size == 14);
    assert(!strcmp(party_search_rows_find(&board.rows, 12)->row.leader.data, "pending"));
    assert(party_search_rows_find(&board.rows, 13)->row.is_more_row);
    party_booking_set_jobs(&board, 12, p.jobs);
    assert(!party_search_rows_find(&board.rows, 12)->row.jobs.size);
    party_booking_remove_row(&board, 13);
    assert(!party_search_rows_find(&board.rows, 13));
    party_booking_remove(&board, 12);
    assert(!party_search_rows_find(&board.rows, 12));
    p.id = INT_MAX; party_booking_add_rows(&board, &p, 1, 1);
    assert(party_search_rows_find(&board.rows, INT_MIN)->row.is_more_row);
    board.window_open = 1; party_booking_reset(&board);
    assert(!board.rows.size && !board.pending.size && !board.window_open && board.last_id == -1 && board.queue_new);
    party_search_row_clear(&row); party_booking_destruct(&board); party_booking_destruct(&board);
    assert(!outstanding);
}

static void test_packets(void)
{
    struct PartyBooking board;
    struct PartyBookingPost p = post(100);
    struct PartyBookingSearch search = {0};
    unsigned char packet[103] = {0};
    unsigned short declared = 101;
    size_t i;
    prepare(&board);
    memcpy(packet + 2, &declared, 2); packet[4] = 1;
    memcpy(packet + 5, &p, sizeof p); p.id = 101; memcpy(packet + 53, &p, sizeof p);
    for (i = 0; i < 101; ++i) assert(!party_booking_decode_search(&search, packet, i));
    assert(!party_booking_decode_search(&search, NULL, 101));
    assert(party_booking_decode_search(&search, packet, 101));
    assert(search.count == 2 && search.posts == packet + 5 && search.more == 1);
    party_booking_add_search(&board, &search);
    assert(board.last_id == 101 && board.rows.size == 3);
    assert(party_search_rows_find(&board.rows, 102)->row.is_more_row);
    for (i = 0; i < 103; ++i) {
        declared = (unsigned short)i; memcpy(packet + 2, &declared, 2);
        assert(party_booking_decode_search(&search, packet, i) == (i == 5 || i == 53 || i == 101));
    }
    party_booking_destruct(&board); assert(!outstanding);
}

static void test_notices(void)
{
    struct PartyBooking board;
    struct PartyBookingPost p = post(1);
    prepare(&board);
    board.alarm = 1;
    party_booking_on_new(&board, p);
    assert(wave_count == 1 && notice_count == 1 && !board.rows.size && !board.pending.size);
    board.alarm_filtered = 1;
    board.filter_map = 100;
    party_booking_on_new(&board, p);
    assert(wave_count == 1 && notice_count == 1);
    window_visible = 1; window_matches = 0;
    party_booking_on_new(&board, p);
    assert(!window_alarm && !board.pending.size);
    window_matches = 1;
    party_booking_on_new(&board, p);
    assert(window_alarm == 1 && board.pending.size == 1 && !board.rows.size);
    p.id = 2; p.map = -1;
    party_booking_on_new(&board, p);
    assert(window_alarm == 2 && board.pending.size == 1);
    p = post(2); board.queue_new = 0;
    party_booking_on_new(&board, p);
    assert(window_alarm == 3 && !board.pending.size && board.rows.size == 2);
    party_booking_destruct(&board); assert(!outstanding);
}

int main(void)
{
    test_loader(); test_rows(); test_pending(); test_packets(); test_notices();
    puts("Party search: Lua reload and failure bounds, every allocation failure, row text, pending batches and packet spans passed");
    return 0;
}
