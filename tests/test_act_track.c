#include "world/act_track.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* The server's bytes (eos-protocol.test.mjs): a swing of 0x04030201 at 0x08070605 from tick
   1000, landing at 1216, its animation 432 ms and over at 1432, the record's revision 0x1234. */
static const unsigned char SWING[37] = {
    0x21, 0x0f, 0x01, 0x02, 0x03, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x01, 0x98, 0x05, 0x00, 0x00, 0xc0, 0x04,
    0x00, 0x00, 0x05, 0x06, 0x07, 0x08, 0, 0, 0, 0, 0, 0, 0, 0, 0xb0, 0x01, 0, 0, 0x34, 0x12,
};
/* A cast of skill 28 at level 5 on 0x08070605, cell (41,42), from 1000 to 2500, with an attack
   motion of 432, element 6, disposable, the record's revision 0xfffe. */
static const unsigned char CAST[37] = {
    0x21, 0x0f, 0x01, 0x02, 0x03, 0x04, 0xe8, 0x03, 0x00, 0x00, 0x02, 0xc4, 0x09, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x05, 0x06, 0x07, 0x08, 0x29, 0x00, 0x2a, 0x00, 0x1c, 0x00, 0x05, 0x00, 0xb0, 0x01, 0x06, 0x01,
    0xfe, 0xff,
};
/* 40 of 55 HP from tick 1216, the record's revision 7. */
static const unsigned char VITALS[20] = {
    0x22, 0x0f, 0x01, 0x02, 0x03, 0x04, 0xc0, 0x04, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x37, 0x00, 0x00, 0x00,
    0x07, 0x00,
};
/* A double hit for 15 and 3 to SP at tick 1216, with a flinch of 288 ms. */
static const unsigned char HIT[32] = {
    0x23, 0x0f, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0xc0, 0x04, 0x00, 0x00, 0x0f, 0x00,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x08, 0x01, 0x20, 0x01, 0, 0, 0, 0,
};

/* Records are made one after the other: each has the next revision. */
static uint16_t g_made;

/* The clock the item taken last was taken on. */
static uint32_t on;

static struct TimedItem act(uint32_t actor, uint32_t tick, int kind)
{
    struct TimedItem item;

    memset(&item, 0, sizeof item);
    item.kind = TIMED_ACT;
    item.actor = actor;
    item.tick = tick;
    item.u.act.gid = actor;
    item.u.act.at_tick = tick;
    item.u.act.kind = (unsigned char)kind;
    item.u.act.rev = ++g_made;
    return item;
}

static struct TimedItem hit(uint32_t source, uint32_t target, uint32_t tick, int damage)
{
    struct TimedItem item;

    memset(&item, 0, sizeof item);
    item.kind = TIMED_HIT;
    item.actor = source;
    item.tick = tick;
    item.u.hit.source = source;
    item.u.hit.target = target;
    item.u.hit.at_tick = tick;
    item.u.hit.damage = damage;
    return item;
}

static struct TimedItem vitals(uint32_t gid, uint32_t tick, int hp)
{
    struct TimedItem item;

    memset(&item, 0, sizeof item);
    item.kind = TIMED_VITALS;
    item.actor = gid;
    item.tick = tick;
    item.u.vitals.gid = gid;
    item.u.vitals.at_tick = tick;
    item.u.vitals.hp = hp;
    item.u.vitals.rev = ++g_made;
    return item;
}

/* Files with no record played yet. */
static void file(struct TimedQueue *queue, struct TimedItem item)
{
    static const struct RecordFloors none = { 0 };
    struct TimedItem early;

    assert(timed_queue_file(queue, &item, &none, 0, &early) == TIMED_FILED);
}

static void reads_the_servers_bytes(void)
{
    struct ActRecord swing;
    struct VitalsRecord hp;
    struct HitNotice notice;

    assert(act_record_read(SWING, &swing));
    assert(swing.gid == 0x04030201 && swing.at_tick == 1000 && swing.kind == ACT_SWING);
    assert(swing.end_tick == 1432 && swing.mark_tick == 1216 && swing.target == 0x08070605 && swing.motion_ms == 432);
    assert(swing.rev == 0x1234);

    assert(act_record_read(CAST, &swing));
    assert(swing.kind == ACT_CAST && swing.at_tick == 1000 && swing.end_tick == 2500 && swing.target == 0x08070605);
    assert(swing.x == 41 && swing.y == 42 && swing.skill_id == 28 && swing.skill_lv == 5 && swing.motion_ms == 432);
    assert(swing.element == 6 && swing.disposable == 1 && swing.rev == 0xfffe);

    vitals_record_read(VITALS, &hp);
    assert(hp.gid == 0x04030201 && hp.at_tick == 1216 && hp.hp == 40 && hp.max_hp == 55 && hp.rev == 7);

    hit_notice_read(HIT, &notice);
    assert(notice.source == 0x04030201 && notice.target == 0x08070605 && notice.at_tick == 1216);
    assert(notice.damage == 15 && notice.left_damage == 3 && notice.count == 2 && notice.kind == 8);
    assert(notice.sp_damage == 1 && notice.flinch_ms == 288);
}

static void reads_a_heal_and_a_negative_count(void)
{
    unsigned char packet[32];
    struct HitNotice notice;

    memcpy(packet, HIT, sizeof packet);
    memset(packet + 14, 0xff, 4);
    packet[14] = 0xfb; /* -5 */
    packet[22] = 0xff;
    packet[23] = 0xff;
    packet[25] = 0;
    hit_notice_read(packet, &notice);
    assert(notice.damage == -5 && notice.count == -1 && notice.sp_damage == 0);
}

static void an_act_of_a_kind_it_does_not_know_is_not_read(void)
{
    unsigned char packet[37];
    struct ActRecord record;

    memcpy(packet, CAST, sizeof packet);
    memset(&record, 0x5a, sizeof record);
    packet[10] = 3;
    assert(!act_record_read(packet, &record));
    assert(record.kind == 0x5a);
    packet[10] = ACT_IDLE;
    assert(act_record_read(packet, &record) && record.kind == ACT_IDLE && !record.old);
    /* the top bit marks a record sent later than it was made, and is not part of the kind */
    packet[10] = ACT_RECORD_OLD | ACT_SWING;
    assert(act_record_read(packet, &record) && record.kind == ACT_SWING && record.old);
    memset(&record, 0x5a, sizeof record);
    packet[10] = ACT_RECORD_OLD | 3;
    assert(!act_record_read(packet, &record) && record.kind == 0x5a);
    packet[10] = 0xFF;
    assert(!act_record_read(packet, &record));
    /* a negative element is kept */
    packet[10] = ACT_CAST;
    packet[33] = 0xff;
    packet[34] = 0;
    assert(act_record_read(packet, &record) && record.element == -1 && record.disposable == 0);
}

static void items_come_out_by_tick_and_in_arrival_order(void)
{
    struct TimedQueue queue = { 0 };
    struct TimedItem got;

    file(&queue, hit(7, 9, 1500, 1));
    file(&queue, act(7, 1000, ACT_SWING));
    file(&queue, vitals(9, 1500, 40));
    file(&queue, hit(8, 9, 1500, 2));
    assert(!timed_queue_take(&queue, 0, 0, 999, &got, &on));
    assert(timed_queue_take(&queue, 0, 0, 1000, &got, &on) && got.kind == TIMED_ACT);
    assert(!timed_queue_take(&queue, 0, 0, 1499, &got, &on));
    assert(timed_queue_take(&queue, 0, 0, 1500, &got, &on) && got.kind == TIMED_HIT && got.u.hit.damage == 1);
    assert(timed_queue_take(&queue, 0, 0, 1500, &got, &on) && got.kind == TIMED_VITALS);
    assert(timed_queue_take(&queue, 0, 0, 1500, &got, &on) && got.kind == TIMED_HIT && got.u.hit.damage == 2);
    assert(queue.count == 0 && !timed_queue_take(&queue, 0, 0, 9999, &got, &on));
}

static void a_newer_act_removes_the_actors_later_ones(void)
{
    struct TimedQueue queue = { 0 };
    struct TimedItem got;

    file(&queue, act(7, 1000, ACT_SWING));
    file(&queue, act(8, 1000, ACT_SWING));
    file(&queue, hit(7, 9, 1216, 5));
    file(&queue, act(7, 1500, ACT_SWING));
    /* 7 moved during its windup: idle from 1100. Its swings at or after that go, the hit stays
       (the server sends none for a swing it cancelled), and so does the other actor's swing. */
    file(&queue, act(7, 1100, ACT_IDLE));
    assert(queue.count == 4);
    assert(timed_queue_take(&queue, 0, 0, 5000, &got, &on) && got.actor == 7 && got.tick == 1000);
    assert(timed_queue_take(&queue, 0, 0, 5000, &got, &on) && got.actor == 8 && got.tick == 1000);
    assert(timed_queue_take(&queue, 0, 0, 5000, &got, &on) && got.kind == TIMED_ACT && got.u.act.kind == ACT_IDLE);
    assert(timed_queue_take(&queue, 0, 0, 5000, &got, &on) && got.kind == TIMED_HIT);
    /* the same tick: the later one made is the one */
    file(&queue, act(7, 2000, ACT_SWING));
    file(&queue, act(7, 2000, ACT_IDLE));
    assert(queue.count == 1 && queue.items[0].u.act.kind == ACT_IDLE);
}

static void the_own_actors_items_are_played_on_its_own_clock(void)
{
    struct TimedQueue queue = { 0 };
    struct TimedItem got;

    file(&queue, act(8, 1000, ACT_SWING));
    file(&queue, act(7, 1040, ACT_SWING));  /* the own actor's, later than another's that waits */
    file(&queue, hit(7, 9, 1256, 5));
    file(&queue, hit(9, 8, 1300, 3));       /* between two others: the render clock */
    /* the own clock is 160 ms ahead of the render clock */
    assert(timed_queue_take(&queue, 7, 1100, 940, &got, &on) && got.actor == 7 && got.kind == TIMED_ACT && on == 1100);
    assert(!timed_queue_take(&queue, 7, 1100, 940, &got, &on));
    assert(timed_queue_take(&queue, 7, 1260, 1100, &got, &on) && got.actor == 8 && on == 1100);
    assert(timed_queue_take(&queue, 7, 1260, 1100, &got, &on) && got.kind == TIMED_HIT && got.actor == 7 && on == 1260);
    assert(!timed_queue_take(&queue, 7, 1400, 1240, &got, &on));
    assert(timed_queue_take(&queue, 7, 1460, 1300, &got, &on) && got.actor == 9 && on == 1300);
}

/* An act of `actor` at `target`. */
static struct TimedItem act_at(uint32_t actor, uint32_t tick, int kind, uint32_t target)
{
    struct TimedItem item = act(actor, tick, kind);

    item.u.act.target = target;
    return item;
}

/* What another actor aims at the own character is played on the own clock: its swing, its cast
   and its hit. The same at someone else, and a cast on the ground, wait for the render clock. */
static void what_is_aimed_at_the_own_actor_is_played_on_its_clock(void)
{
    struct TimedQueue queue = { 0 };
    struct TimedItem got, ground = act_at(8, 1000, ACT_CAST, 0);

    assert(timed_queue_on_own_clock(&queue, &ground, 7) == 0);
    file(&queue, act_at(8, 1000, ACT_SWING, 7));
    file(&queue, act_at(9, 1000, ACT_SWING, 8));
    file(&queue, act_at(10, 1000, ACT_CAST, 7));
    file(&queue, act_at(11, 1000, ACT_CAST, 0));
    file(&queue, hit(12, 7, 1000, 3));
    file(&queue, hit(13, 9, 1000, 4));
    /* the own clock has reached their tick, the render clock is 160 ms behind */
    assert(!timed_queue_take(&queue, 7, 999, 839, &got, &on));
    assert(timed_queue_take(&queue, 7, 1000, 840, &got, &on) && got.actor == 8 && on == 1000);
    assert(timed_queue_take(&queue, 7, 1000, 840, &got, &on) && got.actor == 10 && on == 1000);
    assert(timed_queue_take(&queue, 7, 1000, 840, &got, &on) && got.actor == 12 && on == 1000);
    assert(!timed_queue_take(&queue, 7, 1159, 999, &got, &on) && queue.count == 3);
    assert(timed_queue_take(&queue, 7, 1160, 1000, &got, &on) && got.actor == 9 && on == 1000);
    assert(timed_queue_take(&queue, 7, 1160, 1000, &got, &on) && got.actor == 11 && on == 1000);
    assert(timed_queue_take(&queue, 7, 1160, 1000, &got, &on) && got.actor == 13 && on == 1000);
    /* with no own character nothing is: not a swing without a target either */
    file(&queue, act_at(8, 2000, ACT_SWING, 0));
    file(&queue, hit(8, 0, 2000, 1));
    assert(!timed_queue_take(&queue, 0, 2000, 1999, &got, &on));
    assert(queue.aimed_count == 2); /* 8 and 10, from before */
    assert(timed_queue_take(&queue, 0, 0, 2000, &got, &on) && timed_queue_take(&queue, 0, 0, 2000, &got, &on));
    assert(queue.aimed_count == 2 && queue.aimed[0] == 8 && queue.aimed[1] == 10);
}

/* What an actor does is played in the order of its ticks: an item that is due on the own clock
   brings the earlier acts and hits of its actor with it, and nothing else. */
static void an_actors_items_keep_their_order(void)
{
    struct TimedQueue queue = { 0 };
    struct TimedItem got;

    file(&queue, act_at(8, 1000, ACT_IDLE, 0)); /* 8 let go of someone else */
    file(&queue, hit(8, 9, 1010, 5));           /* its hit on that one */
    file(&queue, vitals(8, 1010, 30));          /* its own HP: not what it does */
    file(&queue, hit(9, 8, 1020, 6));           /* another actor's hit */
    file(&queue, act_at(8, 1040, ACT_SWING, 7)); /* then it swings at the own character */
    file(&queue, act_at(8, 1300, ACT_SWING, 7)); /* and again, later */
    /* the own clock is past the swing's tick, the render clock has reached none of them */
    assert(timed_queue_take(&queue, 7, 1100, 940, &got, &on) && got.kind == TIMED_ACT && got.u.act.kind == ACT_IDLE
           && on == 1100);
    assert(timed_queue_take(&queue, 7, 1100, 940, &got, &on) && got.kind == TIMED_HIT && got.actor == 8 && on == 1100);
    assert(timed_queue_take(&queue, 7, 1100, 940, &got, &on) && got.kind == TIMED_ACT && got.tick == 1040 && on == 1100);
    assert(!timed_queue_take(&queue, 7, 1100, 940, &got, &on) && queue.count == 3);
    /* the rest when their own clocks come */
    assert(timed_queue_take(&queue, 7, 1170, 1010, &got, &on) && got.kind == TIMED_VITALS && on == 1010);
    assert(timed_queue_take(&queue, 7, 1180, 1020, &got, &on) && got.actor == 9 && on == 1020);
    assert(timed_queue_take(&queue, 7, 1300, 1140, &got, &on) && got.tick == 1300 && on == 1300);
    /* an item of the own clock that is not due brings nothing: the earlier one waits for its own */
    file(&queue, hit(8, 9, 2000, 1));
    file(&queue, act_at(8, 2100, ACT_SWING, 7));
    assert(!timed_queue_take(&queue, 7, 2099, 1939, &got, &on));
    assert(timed_queue_take(&queue, 7, 2100, 1940, &got, &on) && got.kind == TIMED_HIT);
    assert(timed_queue_take(&queue, 7, 2100, 1940, &got, &on) && got.kind == TIMED_ACT);
}

/* The idle record that ends a swing or a cast at the own character is told on the own clock
   too; one that ends anything else, on the render clock. */
static void the_end_of_what_was_aimed_is_told_as_soon(void)
{
    struct TimedQueue queue = { 0 };
    struct TimedItem got, early, item;
    uint32_t actor;
    int i;

    /* a swing at the own character is cancelled */
    file(&queue, act_at(8, 1000, ACT_SWING, 7));
    assert(timed_queue_take(&queue, 7, 1000, 840, &got, &on) && queue.aimed_count == 1);
    file(&queue, act_at(8, 1100, ACT_IDLE, 0));
    assert(!timed_queue_take(&queue, 7, 1099, 939, &got, &on));
    assert(timed_queue_take(&queue, 7, 1100, 940, &got, &on) && got.u.act.kind == ACT_IDLE && on == 1100);
    assert(queue.aimed_count == 0);
    /* the next idle of that actor ends nothing aimed */
    file(&queue, act_at(8, 1200, ACT_IDLE, 0));
    assert(!timed_queue_take(&queue, 7, 1359, 1199, &got, &on));
    assert(timed_queue_take(&queue, 7, 1360, 1200, &got, &on) && on == 1200);
    /* a cast at the own character, then a swing at someone else: its end is not aimed */
    file(&queue, act_at(8, 2000, ACT_CAST, 7));
    assert(timed_queue_take(&queue, 7, 2000, 1840, &got, &on) && queue.aimed_count == 1);
    file(&queue, act_at(8, 2100, ACT_SWING, 9));
    assert(!timed_queue_take(&queue, 7, 2259, 2099, &got, &on));
    assert(timed_queue_take(&queue, 7, 2260, 2100, &got, &on) && queue.aimed_count == 0);
    file(&queue, act_at(8, 2200, ACT_IDLE, 0));
    assert(!timed_queue_take(&queue, 7, 2359, 2199, &got, &on));
    assert(timed_queue_take(&queue, 7, 2360, 2200, &got, &on));
    /* a hit ends nothing: the actor is still in what its record said */
    file(&queue, act_at(8, 3000, ACT_SWING, 7));
    file(&queue, hit(8, 7, 3100, 5));
    assert(timed_queue_take(&queue, 7, 3100, 2940, &got, &on) && timed_queue_take(&queue, 7, 3100, 2940, &got, &on));
    assert(queue.aimed_count == 1 && queue.aimed[0] == 8);
    /* a second swing of the same actor is kept once */
    file(&queue, act_at(8, 3200, ACT_SWING, 7));
    assert(timed_queue_take(&queue, 7, 3200, 3040, &got, &on) && queue.aimed_count == 1);
    /* one more actor than are kept: the one that began longest ago is told on the render clock */
    timed_queue_clear(&queue);
    for (actor = 100; actor < 100 + TIMED_AIMED_ACTORS + 1; actor++) {
        file(&queue, act_at(actor, 4000, ACT_SWING, 7));
        assert(timed_queue_take(&queue, 7, 4000, 3840, &got, &on));
    }
    assert(queue.aimed_count == TIMED_AIMED_ACTORS && queue.aimed[0] == 101);
    file(&queue, act_at(100, 4100, ACT_IDLE, 0));
    file(&queue, act_at(101, 4100, ACT_IDLE, 0));
    assert(timed_queue_take(&queue, 7, 4100, 3940, &got, &on) && got.actor == 101);
    assert(!timed_queue_take(&queue, 7, 4259, 4099, &got, &on));
    assert(timed_queue_take(&queue, 7, 4260, 4100, &got, &on) && got.actor == 100);
    /* the item a full queue hands back has left it as a taken one does */
    timed_queue_clear(&queue);
    assert(queue.count == 0 && queue.aimed_count == 0);
    file(&queue, act_at(8, 5000, ACT_SWING, 7));
    for (i = 1; i < TIMED_QUEUE_ITEMS; i++)
        file(&queue, hit(20, 21, 6000 + (uint32_t)i, i));
    item = hit(20, 21, 7000, 0);
    assert(timed_queue_file(&queue, &item, NULL, 7, &early) == TIMED_FILED_EARLY && early.actor == 8);
    assert(queue.aimed_count == 1 && queue.aimed[0] == 8);
}

static void a_hit_shown_early_takes_its_hp_along(void)
{
    struct TimedQueue queue = { 0 };
    struct TimedItem got;

    file(&queue, vitals(8, 1200, 50));
    file(&queue, vitals(9, 1250, 40));
    file(&queue, vitals(9, 1256, 35));
    file(&queue, vitals(9, 1300, 30));
    assert(timed_queue_take_vitals(&queue, 9, 1256, &got) && got.u.vitals.hp == 40);
    assert(timed_queue_take_vitals(&queue, 9, 1256, &got) && got.u.vitals.hp == 35);
    assert(!timed_queue_take_vitals(&queue, 9, 1256, &got)); /* the later one waits for its tick */
    assert(queue.count == 2);
    /* two of one tick: the later one made is the HP */
    file(&queue, vitals(9, 1400, 25));
    file(&queue, vitals(9, 1400, 20));
    assert(queue.count == 3 && queue.items[2].u.vitals.hp == 20);
    assert(!timed_queue_take_vitals(&queue, 7, 9999, &got));
}

static void a_full_queue_hands_back_its_earliest(void)
{
    struct TimedQueue queue = { 0 };
    struct TimedItem item, early;
    int i;

    for (i = 0; i < TIMED_QUEUE_ITEMS; i++)
        file(&queue, hit(7, 9, 1000 + (uint32_t)i, i));
    item = hit(7, 9, 5000, 999);
    assert(timed_queue_file(&queue, &item, NULL, 0, &early) == TIMED_FILED_EARLY && early.tick == 1000
           && early.u.hit.damage == 0);
    assert(queue.count == TIMED_QUEUE_ITEMS && queue.items[0].tick == 1001);
    assert(queue.items[TIMED_QUEUE_ITEMS - 1].u.hit.damage == 999);
}

static void ticks_may_wrap(void)
{
    struct TimedQueue queue = { 0 };
    struct TimedItem got;

    file(&queue, hit(7, 9, 0x10, 2));
    file(&queue, hit(7, 9, 0xfffffff0u, 1));
    assert(queue.items[0].tick == 0xfffffff0u);
    assert(!timed_queue_take(&queue, 0, 0, 0xffffffe0u, &got, &on));
    assert(timed_queue_take(&queue, 0, 0, 0xfffffff8u, &got, &on) && got.u.hit.damage == 1);
    assert(!timed_queue_take(&queue, 0, 0, 0x0f, &got, &on));
    assert(timed_queue_take(&queue, 0, 0, 0x10, &got, &on) && got.u.hit.damage == 2);
    file(&queue, act(7, 0x20, ACT_SWING));
    file(&queue, act(7, 0xffffff00u, ACT_IDLE)); /* before the wrap: it replaces the swing after */
    assert(queue.count == 1 && queue.items[0].u.act.kind == ACT_IDLE);
}

/* Whether the queue holds the record of `kind`, `tick` and revision `rev`. */
static int holds(const struct TimedQueue *queue, int kind, uint32_t tick, uint16_t rev)
{
    int i;

    for (i = 0; i < queue->count; i++)
        if (queue->items[i].kind == kind && queue->items[i].tick == tick
            && (kind == TIMED_ACT ? queue->items[i].u.act.rev : queue->items[i].u.vitals.rev) == rev)
            return 1;
    return 0;
}

/* The same records of one actor's tracks, in tick order. Records of two tracks with the same
   tick come out in the order they arrived, which says nothing of the tracks. */
static int same_queue(const struct TimedQueue *a, const struct TimedQueue *b)
{
    int i;

    if (a->count != b->count)
        return 0;
    for (i = 0; i < a->count; i++) {
        const struct TimedItem *item = &a->items[i];

        if (!holds(b, item->kind, item->tick, item->kind == TIMED_ACT ? item->u.act.rev : item->u.vitals.rev))
            return 0;
        if (i && (int32_t)(item->tick - a->items[i - 1].tick) < 0)
            return 0;
    }
    return 1;
}

static uint32_t g_seed = 4242;

static uint32_t rnd(void)
{
    g_seed ^= g_seed << 13;
    g_seed ^= g_seed >> 17;
    g_seed ^= g_seed << 5;
    return g_seed;
}

/* Five records of one actor's act and vitals tracks, made one after the other with ticks from
   a few, filed in random orders with some twice: the queue is the one filing them once in the
   order they were made gives. */
static void any_order_leaves_the_same_queue(void)
{
    static const struct RecordFloors none = { 0 };
    int round, i;

    for (round = 0; round < 20000; round++) {
        struct TimedItem made[5], early;
        struct TimedQueue in_order = { 0 }, queue = { 0 };
        int order[5] = { 0, 1, 2, 3, 4 };

        g_made = (uint16_t)(0xfffd + round);
        for (i = 0; i < 5; i++) {
            uint32_t tick = 0xffffff00u + 100 * (rnd() % 5);

            made[i] = rnd() % 2 ? act(7, tick, (int)(rnd() % 3)) : vitals(7, tick, (int)i);
        }
        for (i = 0; i < 5; i++)
            timed_queue_file(&in_order, &made[i], &none, 0, &early);
        for (i = 4; i > 0; i--) {
            int j = (int)(rnd() % (unsigned)(i + 1)), t = order[i];

            order[i] = order[j];
            order[j] = t;
        }
        for (i = 0; i < 5; i++) {
            timed_queue_file(&queue, &made[order[i]], &none, 0, &early);
            if (rnd() % 3 == 0)
                assert(timed_queue_file(&queue, &made[order[rnd() % (unsigned)(i + 1)]], &none, 0, &early) == TIMED_DROPPED);
        }
        assert(same_queue(&queue, &in_order));
    }
}

/* A record not newer than the one of its track played last is the past; a newer one is not,
   whatever its tick. Hits know no floor. */
static void what_was_played_is_a_floor(void)
{
    struct TimedQueue queue = { 0 };
    struct RecordFloors floors = { 0 };
    struct TimedItem swing = act(7, 1000, ACT_SWING), idle = act(7, 1100, ACT_IDLE), late, hp = vitals(7, 1000, 40),
                     more = vitals(7, 1200, 30), blow = hit(7, 9, 1000, 5), got, early;

    late = act(7, 900, ACT_IDLE); /* made after the swing and the idle, from before them */
    assert(timed_queue_file(&queue, &idle, &floors, 0, &early) == TIMED_FILED);
    assert(timed_queue_take(&queue, 0, 0, 1150, &got, &on) && got.u.act.rev == idle.u.act.rev);
    record_floors_played(&floors, &got);
    assert(timed_queue_file(&queue, &swing, &floors, 0, &early) == TIMED_DROPPED); /* the past */
    assert(timed_queue_file(&queue, &idle, &floors, 0, &early) == TIMED_DROPPED);  /* played */
    assert(timed_queue_file(&queue, &late, &floors, 0, &early) == TIMED_FILED);   /* newer */
    /* the vitals track has its own floor */
    assert(timed_queue_file(&queue, &more, &floors, 0, &early) == TIMED_FILED);
    assert(timed_queue_take(&queue, 0, 0, 1150, &got, &on) && got.kind == TIMED_ACT);
    record_floors_played(&floors, &got);
    assert(timed_queue_take(&queue, 0, 0, 1250, &got, &on) && got.kind == TIMED_VITALS);
    record_floors_played(&floors, &got);
    assert(timed_queue_file(&queue, &hp, &floors, 0, &early) == TIMED_DROPPED);
    assert(timed_queue_file(&queue, &blow, &floors, 0, &early) == TIMED_FILED);
    assert(timed_queue_file(&queue, &blow, NULL, 0, &early) == TIMED_FILED);
    /* an older record played does not lower the floor */
    record_floors_played(&floors, &hp);
    assert(floors.vitals == more.u.vitals.rev);
    record_floors_played(&floors, &swing);
    assert(floors.act == late.u.act.rev);
}

/* Another actor's records, and the other track of the same actor, are not touched. */
static void a_record_touches_only_its_own_track(void)
{
    static const struct RecordFloors none = { 0 };
    struct TimedQueue queue = { 0 };
    struct TimedItem swing8 = act(8, 1500, ACT_SWING), hp7 = vitals(7, 1500, 40), idle7 = act(7, 1000, ACT_IDLE), early;

    file(&queue, swing8);
    file(&queue, hp7);
    assert(timed_queue_file(&queue, &idle7, &none, 0, &early) == TIMED_FILED);
    assert(queue.count == 3);
}

int main(void)
{
    reads_the_servers_bytes();
    reads_a_heal_and_a_negative_count();
    an_act_of_a_kind_it_does_not_know_is_not_read();
    items_come_out_by_tick_and_in_arrival_order();
    a_newer_act_removes_the_actors_later_ones();
    the_own_actors_items_are_played_on_its_own_clock();
    what_is_aimed_at_the_own_actor_is_played_on_its_clock();
    an_actors_items_keep_their_order();
    the_end_of_what_was_aimed_is_told_as_soon();
    a_hit_shown_early_takes_its_hp_along();
    a_full_queue_hands_back_its_earliest();
    ticks_may_wrap();
    any_order_leaves_the_same_queue();
    what_was_played_is_a_floor();
    a_record_touches_only_its_own_track();
    puts("act_track: ok");
    return 0;
}
