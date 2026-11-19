#include "world/record_wait.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Records that wait for their actor (world/record_wait.h). */

static uint32_t g_present[8];
static int g_present_count;

static int present(void *context, uint32_t gid)
{
    int i;

    (void)context;
    for (i = 0; i < g_present_count; i++)
        if (g_present[i] == gid)
            return 1;
    return 0;
}

static struct RecordWaiting move_of(uint32_t gid, uint32_t tick)
{
    struct RecordWaiting item;

    memset(&item, 0, sizeof item);
    item.gid = gid;
    item.kind = RECORD_WAIT_MOVE;
    item.u.move.at_tick = tick;
    return item;
}

static struct RecordWaiting act_of(uint32_t gid, uint32_t tick)
{
    struct RecordWaiting item;

    memset(&item, 0, sizeof item);
    item.gid = gid;
    item.kind = RECORD_WAIT_TIMED;
    item.u.timed.kind = TIMED_ACT;
    item.u.timed.actor = gid;
    item.u.timed.tick = tick;
    return item;
}

static void a_record_waits_until_its_actor_comes(void)
{
    struct RecordWait wait = { 0 };
    struct RecordWaiting item, got;

    g_present_count = 0;
    item = move_of(7, 1000);
    record_wait_put(&wait, 100, &item);
    item = act_of(8, 1100);
    record_wait_put(&wait, 110, &item);
    item = move_of(7, 1200);
    record_wait_put(&wait, 120, &item);
    assert(!record_wait_take(&wait, 200, present, NULL, &got) && wait.count == 3);
    /* 7 comes: its records, in the order they arrived; 8's waits on */
    g_present[g_present_count++] = 7;
    assert(record_wait_take(&wait, 300, present, NULL, &got) && got.kind == RECORD_WAIT_MOVE && got.u.move.at_tick == 1000);
    assert(got.since == 100);
    assert(record_wait_take(&wait, 300, present, NULL, &got) && got.u.move.at_tick == 1200);
    assert(!record_wait_take(&wait, 300, present, NULL, &got) && wait.count == 1);
    g_present[g_present_count++] = 8;
    assert(record_wait_take(&wait, 300, present, NULL, &got) && got.kind == RECORD_WAIT_TIMED && got.u.timed.tick == 1100);
    assert(wait.count == 0);
}

static void a_record_whose_actor_does_not_come_goes(void)
{
    struct RecordWait wait = { 0 };
    struct RecordWaiting item, got;

    g_present_count = 0;
    /* across the wrap of the caller's clock */
    item = move_of(7, 1000);
    record_wait_put(&wait, 0xfffffc00u, &item);
    item = move_of(8, 1000);
    record_wait_put(&wait, 0xffffff00u, &item);
    assert(!record_wait_take(&wait, 0xfffffc00u + RECORD_WAIT_MS, present, NULL, &got) && wait.count == 2);
    assert(!record_wait_take(&wait, 0xfffffc00u + RECORD_WAIT_MS + 1, present, NULL, &got) && wait.count == 1);
    g_present[g_present_count++] = 7;
    g_present[g_present_count++] = 8;
    assert(record_wait_take(&wait, 0xfffffc00u + RECORD_WAIT_MS + 1, present, NULL, &got) && got.gid == 8);
}

static void a_full_wait_lets_its_longest_waiting_go(void)
{
    struct RecordWait wait = { 0 };
    struct RecordWaiting item, got;
    uint32_t i;

    g_present_count = 0;
    for (i = 0; i <= RECORD_WAIT_ITEMS; i++) {
        item = move_of(100 + i, i);
        record_wait_put(&wait, i, &item);
    }
    assert(wait.count == RECORD_WAIT_ITEMS && wait.items[0].gid == 101);
    g_present[g_present_count++] = 100;
    assert(!record_wait_take(&wait, 70, present, NULL, &got));
    record_wait_clear(&wait);
    assert(wait.count == 0);
}

int main(void)
{
    a_record_waits_until_its_actor_comes();
    a_record_whose_actor_does_not_come_goes();
    a_full_wait_lets_its_longest_waiting_go();
    puts("record_wait: ok");
    return 0;
}
