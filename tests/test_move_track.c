#include "world/move_track.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* The server's bytes for a walk of a 150 ms unit from (156,153), a diagonal step north-east
   (7) and two east (6), the first landing after 210 ms, the record's revision 0x1234
   (eos-protocol.test.mjs, ZC_TRACK_MOVE). */
static const unsigned char WALK[27] = {
    0x20, 0x0f, 0x1b, 0x00, 0x01, 0x02, 0x03, 0x04, 0x98, 0xba, 0xdc, 0xfe, 0x9c, 0x00, 0x99, 0x00,
    0xd2, 0x00, 0x96, 0x00, 0x00, 0x00, 0x03, 0x34, 0x12, 0xb7, 0x01,
};

/* A record at `tick`, made after every one made before it: its revision is the next. */
static uint16_t g_made;

static struct MoveRecord at(uint32_t tick)
{
    struct MoveRecord record;

    memset(&record, 0, sizeof record);
    record.at_tick = tick;
    record.rev = ++g_made;
    return record;
}

static void reads_the_servers_bytes(void)
{
    struct MoveRecord record;
    uint32_t gid = 0;

    assert(move_record_read(WALK, sizeof WALK, &gid, &record));
    assert(gid == 0x04030201 && record.at_tick == 0xfedcba98u);
    assert(record.x == 156 && record.y == 153 && record.land_ms == 210 && record.speed_ms == 150);
    assert(record.dir == 0 && record.cause == MOVE_CAUSE_WALKED && record.steps == 3 && !record.old);
    assert(record.path[0] == 7 && record.path[1] == 6 && record.path[2] == 6 && record.rev == 0x1234);
}

static void reads_the_mark_of_a_record_sent_later(void)
{
    unsigned char packet[27];
    struct MoveRecord record;
    uint32_t gid;

    memcpy(packet, WALK, sizeof packet);
    packet[21] = MOVE_RECORD_OLD | MOVE_CAUSE_PUSHED;
    assert(move_record_read(packet, sizeof packet, &gid, &record));
    assert(record.old && record.cause == MOVE_CAUSE_PUSHED);
}

static void reads_every_step_of_the_longest_walk(void)
{
    unsigned char packet[25 + 12];
    struct MoveRecord record;
    uint32_t gid;
    int i;

    memset(packet, 0, sizeof packet);
    packet[0] = 0x20;
    packet[1] = 0x0f;
    packet[2] = sizeof packet;
    packet[22] = MOVE_TRACK_STEPS;
    for (i = 0; i < MOVE_TRACK_STEPS; i++) {
        unsigned v = (unsigned)((i * 5 + 3) & 7) << (3 * i % 8);

        packet[25 + 3 * i / 8] |= (unsigned char)v;
        if (v > 0xff)
            packet[25 + 3 * i / 8 + 1] |= (unsigned char)(v >> 8);
    }
    assert(move_record_read(packet, sizeof packet, &gid, &record));
    for (i = 0; i < MOVE_TRACK_STEPS; i++)
        assert(record.path[i] == ((i * 5 + 3) & 7));
}

static void refuses_what_does_not_fit(void)
{
    unsigned char packet[27];
    struct MoveRecord record;
    uint32_t gid = 77;

    memset(&record, 0x5a, sizeof record);
    memcpy(packet, WALK, sizeof packet);
    assert(!move_record_read(packet, 24, &gid, &record));            /* shorter than its fields */
    assert(!move_record_read(packet, 26, &gid, &record));            /* not the length it says */
    packet[2] = 24;
    assert(!move_record_read(packet, 24, &gid, &record));            /* the layout before the revision */
    packet[2] = sizeof packet;
    packet[22] = 9;                                                  /* nine steps need four bytes */
    assert(!move_record_read(packet, sizeof packet, &gid, &record));
    packet[22] = MOVE_TRACK_STEPS + 1;
    assert(!move_record_read(packet, sizeof packet, &gid, &record));
    packet[22] = 3;
    packet[21] = 3;                                                  /* no such cause */
    assert(!move_record_read(packet, sizeof packet, &gid, &record));
    packet[21] = MOVE_RECORD_OLD | 3;
    assert(!move_record_read(packet, sizeof packet, &gid, &record));
    assert(gid == 77 && record.steps == 0x5a);                       /* nothing was written */
}

static void a_walk_from_standing_gives_each_cell_its_tick(void)
{
    struct MoveRecord record;
    struct MoveCell cells[MOVE_TRACK_STEPS + 1];
    uint32_t gid;

    assert(move_record_read(WALK, sizeof WALK, &gid, &record));
    record.at_tick = 1000;
    assert(move_record_cells(&record, cells) == 4);
    assert(cells[0].x == 156 && cells[0].y == 153 && cells[0].tick == 1000);
    assert(cells[1].x == 157 && cells[1].y == 154 && cells[1].dir == 7 && cells[1].tick == 1210);
    assert(cells[2].x == 158 && cells[2].y == 154 && cells[2].dir == 6 && cells[2].tick == 1360);
    assert(cells[3].x == 159 && cells[3].y == 154 && cells[3].tick == 1510);
}

static void a_delayed_walk_stands_first(void)
{
    struct MoveRecord record = at(5000);
    struct MoveCell cells[MOVE_TRACK_STEPS + 1];

    record.x = 60;
    record.y = 60;
    record.speed_ms = 200;
    record.land_ms = 430; /* a hit: the 200 ms step lands 430 ms on */
    record.steps = 2;
    record.path[0] = 6;
    record.path[1] = 4;
    assert(move_record_cells(&record, cells) == 3);
    assert(cells[0].tick == 5230);                                    /* it stands until then */
    assert(cells[1].x == 61 && cells[1].y == 60 && cells[1].tick == 5430);
    assert(cells[2].x == 61 && cells[2].y == 59 && cells[2].tick == 5630);
}

static void a_step_under_way_started_before_the_record(void)
{
    struct MoveRecord record = at(5000);
    struct MoveCell cells[MOVE_TRACK_STEPS + 1];

    record.speed_ms = 150;
    record.land_ms = 40;
    record.steps = 1;
    record.path[0] = 2;
    assert(move_record_cells(&record, cells) == 2);
    assert(cells[0].tick == 4890 && cells[1].tick == 5040 && cells[1].x == -1);
}

static void a_standing_record_is_one_cell(void)
{
    struct MoveRecord record = at(0xfffffff0u);
    struct MoveCell cells[MOVE_TRACK_STEPS + 1];

    record.x = 10;
    record.y = 20;
    record.dir = 4;
    assert(move_record_cells(&record, cells) == 1);
    assert(cells[0].x == 10 && cells[0].y == 20 && cells[0].dir == 4 && cells[0].tick == 0xfffffff0u);
}

static void the_record_that_holds_is_the_latest_one_due(void)
{
    struct MoveTrack track = { 0 };
    struct MoveRecord a = at(1000), b = at(1200), c = at(1500), got;

    assert(!move_track_take(&track, 5000, &got));
    move_track_file(&track, &a);
    move_track_file(&track, &b);
    move_track_file(&track, &c);
    assert(!move_track_take(&track, 999, &got));
    assert(move_track_take(&track, 1300, &got) && got.at_tick == 1200); /* 1000 is behind it */
    assert(track.count == 1);
    assert(!move_track_take(&track, 1499, &got));
    assert(move_track_take(&track, 1500, &got) && got.at_tick == 1500);
    assert(track.count == 0);
}

static void a_newer_record_removes_what_it_replaces(void)
{
    struct MoveTrack track = { 0 };
    struct MoveRecord a = at(1000), turn = at(1450), again = at(1450), stop = at(1100), got;

    move_track_file(&track, &a);
    move_track_file(&track, &turn);
    again.x = 7;
    move_track_file(&track, &again); /* the same tick: the later one made is the one */
    assert(track.count == 2 && track.held[1].x == 7);
    move_track_file(&track, &stop);  /* made knowing the turn: the turn is gone */
    assert(track.count == 2 && track.held[0].at_tick == 1000 && track.held[1].at_tick == 1100);
    assert(move_track_take(&track, 2000, &got) && got.at_tick == 1100 && track.count == 0);
}

static void what_waits_for_later_can_be_dropped(void)
{
    struct MoveTrack track = { 0 };
    struct MoveRecord a = at(1000), b = at(1200), c = at(1500), got;

    move_track_drop_after(&track, 1000);
    assert(track.count == 0);
    move_track_file(&track, &a);
    move_track_file(&track, &b);
    move_track_file(&track, &c);
    move_track_drop_after(&track, 1500); /* its tick has come: it stays */
    assert(track.count == 3);
    move_track_drop_after(&track, 1200);
    assert(track.count == 2 && track.held[1].at_tick == 1200);
    move_track_drop_after(&track, 999);
    assert(track.count == 0 && !move_track_take(&track, 5000, &got));
    /* across the wrap */
    a = at(0xfffffff0u);
    b = at(0x10);
    move_track_file(&track, &a);
    move_track_file(&track, &b);
    move_track_drop_after(&track, 0xfffffff8u);
    assert(track.count == 1 && track.held[0].at_tick == 0xfffffff0u);
}

static void a_full_track_lets_its_earliest_go(void)
{
    struct MoveTrack track = { 0 };
    struct MoveRecord record, got;
    uint32_t tick;

    for (tick = 100; tick <= 100 * (MOVE_TRACK_HELD + 1); tick += 100) {
        record = at(tick);
        move_track_file(&track, &record);
    }
    assert(track.count == MOVE_TRACK_HELD && track.held[0].at_tick == 200);
    assert(move_track_take(&track, 250, &got) && got.at_tick == 200);
}

static void ticks_may_wrap(void)
{
    struct MoveTrack track = { 0 };
    struct MoveRecord a = at(0xfffffff0u), b = at(0x10), got;

    move_track_file(&track, &a);
    move_track_file(&track, &b);
    assert(track.count == 2);
    assert(!move_track_take(&track, 0xffffffe0u, &got));
    assert(move_track_take(&track, 0xfffffff8u, &got) && got.at_tick == 0xfffffff0u);
    assert(!move_track_take(&track, 0xf, &got));
    assert(move_track_take(&track, 0x10, &got) && got.at_tick == 0x10);
}

/* The records a track holds, by tick, as revision and tick pairs, for comparing two tracks. */
static int same_track(const struct MoveTrack *a, const struct MoveTrack *b)
{
    int i;

    if (a->count != b->count)
        return 0;
    for (i = 0; i < a->count; i++)
        if (a->held[i].rev != b->held[i].rev || a->held[i].at_tick != b->held[i].at_tick)
            return 0;
    return 1;
}

static uint32_t g_seed = 12345;

static uint32_t rnd(void)
{
    g_seed ^= g_seed << 13;
    g_seed ^= g_seed >> 17;
    g_seed ^= g_seed << 5;
    return g_seed;
}

/* Four records made one after the other with ticks from a few, filed in every order of the 24,
   some of them twice: the track is the one filing them once in the order they were made gives. */
static void any_order_leaves_the_same_track(void)
{
    int round, order[24][4], orders = 0, a, b, c, d, i;

    for (a = 0; a < 4; a++)
        for (b = 0; b < 4; b++)
            for (c = 0; c < 4; c++)
                for (d = 0; d < 4; d++)
                    if (a != b && a != c && a != d && b != c && b != d && c != d) {
                        order[orders][0] = a;
                        order[orders][1] = b;
                        order[orders][2] = c;
                        order[orders][3] = d;
                        orders++;
                    }
    assert(orders == 24);
    for (round = 0; round < 2000; round++) {
        struct MoveRecord made[4];
        struct MoveTrack in_order = { 0 };
        int o;

        /* ticks around the wrap, revisions around theirs */
        g_made = (uint16_t)(0xfffd + round);
        for (i = 0; i < 4; i++)
            made[i] = at(0xffffff00u + 100 * (rnd() % 5));
        for (i = 0; i < 4; i++)
            move_track_file(&in_order, &made[i]);
        for (o = 0; o < orders; o++) {
            struct MoveTrack track = { 0 };

            for (i = 0; i < 4; i++) {
                move_track_file(&track, &made[order[o][i]]);
                if (rnd() % 3 == 0)
                    assert(!move_track_file(&track, &made[order[o][rnd() % (unsigned)(i + 1)]]));
            }
            assert(same_track(&track, &in_order));
        }
    }
}

/* What came twice, and what a newer held record was made knowing, is dropped and says so. */
static void a_record_twice_or_overtaken_is_dropped(void)
{
    struct MoveTrack track = { 0 };
    struct MoveRecord walk = at(1000), stop = at(900), late = at(1200);

    assert(move_track_file(&track, &walk));
    assert(!move_track_file(&track, &walk));             /* twice */
    assert(move_track_file(&track, &stop));              /* newer and earlier: the walk is gone */
    assert(track.count == 1 && track.held[0].rev == stop.rev);
    assert(!move_track_file(&track, &walk));             /* the stop was made knowing it */
    assert(move_track_file(&track, &late));
    assert(track.count == 2 && track.held[1].rev == late.rev);
}

/* A record not newer than the one taken last is the past; a newer one is not, whatever its tick. */
static void what_was_played_is_a_floor(void)
{
    struct MoveTrack track = { 0 };
    struct MoveRecord walk = at(1000), turn = at(1400), cancel = at(1100), got;

    /* the turn came, the walk is late */
    assert(move_track_file(&track, &turn));
    assert(move_track_take(&track, 1500, &got) && got.rev == turn.rev);
    assert(!move_track_file(&track, &walk));             /* older than the turn on screen */
    assert(!move_track_file(&track, &turn));             /* the turn again */
    assert(track.count == 0);
    /* a newer record from before the turn's tick puts the turn right, at once */
    assert(move_track_file(&track, &cancel));
    assert(move_track_take(&track, 1500, &got) && got.rev == cancel.rev);
}

/* Revisions are compared by their difference: one after the wrap is newer. */
static void revisions_may_wrap(void)
{
    struct MoveTrack track = { 0 };
    struct MoveRecord a = at(100), b, got;

    a.rev = 0xfffe;
    b = at(50);
    b.rev = 0x0001;
    assert(move_track_file(&track, &a));
    assert(move_track_file(&track, &b));                 /* newer and earlier: a goes */
    assert(track.count == 1 && track.held[0].rev == 0x0001);
    assert(move_track_take(&track, 60, &got));
    assert(!move_track_file(&track, &a));
}

/* A record without steps: what it asks of an actor drawn on (drawn_x, drawn_y). */
static int stand(int cause, int drawn_x, int drawn_y, int walking, int held)
{
    struct MoveRecord record = at(1000);
    struct MoveStandAsk ask;

    record.x = 150;
    record.y = 160;
    record.cause = (unsigned char)cause;
    ask.drawn_x = drawn_x;
    ask.drawn_y = drawn_y;
    ask.walking = walking;
    ask.held = held;
    ask.jump_cells = 4;
    return move_record_stand(&record, &ask);
}

static void a_standing_record_is_walked_to_or_set(void)
{
    int held;

    for (held = 0; held < 2; held++) {
        /* pushed or placed: set, wherever the actor is and whatever it does */
        assert(stand(MOVE_CAUSE_PUSHED, 150, 160, 0, held) == MOVE_STAND_SET);
        assert(stand(MOVE_CAUSE_PLACED, 151, 160, 1, held) == MOVE_STAND_SET);
        /* standing on the cell: left as it is */
        assert(stand(MOVE_CAUSE_WALKED, 150, 160, 0, held) == MOVE_STAND_LEAVE);
        /* on another cell, standing or walking, towards it or not: it walks there */
        assert(stand(MOVE_CAUSE_WALKED, 149, 161, 0, held) == MOVE_STAND_WALK);
        assert(stand(MOVE_CAUSE_WALKED, 149, 161, 1, held) == MOVE_STAND_WALK);
        /* up to the cells a steered walk closes, either way and on either axis; further it is set */
        assert(stand(MOVE_CAUSE_WALKED, 154, 156, 1, held) == MOVE_STAND_WALK);
        assert(stand(MOVE_CAUSE_WALKED, 146, 164, 0, held) == MOVE_STAND_WALK);
        assert(stand(MOVE_CAUSE_WALKED, 155, 160, 1, held) == MOVE_STAND_SET);
        assert(stand(MOVE_CAUSE_WALKED, 145, 160, 0, held) == MOVE_STAND_SET);
        assert(stand(MOVE_CAUSE_WALKED, 150, 165, 0, held) == MOVE_STAND_SET);
        assert(stand(MOVE_CAUSE_WALKED, 150, 155, 1, held) == MOVE_STAND_SET);
    }
    /* walking and on the cell: the walk ends on the cell's middle */
    assert(stand(MOVE_CAUSE_WALKED, 150, 160, 1, 0) == MOVE_STAND_FINISH);
    /* held there, it stops where it is drawn: not moved to the middle, which would be a jump */
    assert(stand(MOVE_CAUSE_WALKED, 150, 160, 1, 1) == MOVE_STAND_HALT);
}

/* A record that waits for its tick will move an actor that stands, unless it only says the
   actor stands where it is drawn. */
static void a_record_that_waits_will_move_the_actor_or_not(void)
{
    struct MoveTrack track = { 0 };
    struct MoveRecord here = at(1000), there = at(1100), walk = at(1200), pushed = at(1300);

    here.x = there.x = walk.x = pushed.x = 150;
    here.y = there.y = walk.y = pushed.y = 160;
    assert(!move_track_will_move(&track, 150, 160, 9000));
    /* it stands on the record's cell: nothing is asked of it */
    assert(move_track_file(&track, &here) && !move_track_will_move(&track, 150, 160, 9000));
    /* drawn on another cell, by one or by many, it walks or is set there */
    assert(move_track_will_move(&track, 151, 160, 9000) && move_track_will_move(&track, 150, 159, 9000));
    assert(move_track_will_move(&track, 150, 190, 9000));
    /* but not by a record of a later tick than asked for: at its tick, not before */
    assert(!move_track_will_move(&track, 151, 160, 999) && move_track_will_move(&track, 151, 160, 1000));
    /* a walk moves it wherever it starts, from its tick on */
    walk.steps = 1;
    assert(move_track_file(&track, &walk) && track.count == 2 && move_track_will_move(&track, 150, 160, 9000));
    assert(!move_track_will_move(&track, 150, 160, 1199) && move_track_will_move(&track, 150, 160, 1200));
    /* taken, the record is no more to come */
    assert(move_track_take(&track, 1250, &there) && track.count == 0 && !move_track_will_move(&track, 150, 160, 9000));
    /* pushed or placed, it is set, also onto the cell it is on */
    pushed.cause = MOVE_CAUSE_PUSHED;
    assert(move_track_file(&track, &pushed) && move_track_will_move(&track, 150, 160, 9000));
    /* ticks wrap */
    walk = at(0x00000010);
    walk.steps = 1;
    track.count = 0;
    track.played = 0;
    assert(move_track_file(&track, &walk));
    assert(!move_track_will_move(&track, 150, 160, 0xfffffff0u) && move_track_will_move(&track, 150, 160, 0x00000020));
}

int main(void)
{
    a_record_that_waits_will_move_the_actor_or_not();
    reads_the_servers_bytes();
    reads_the_mark_of_a_record_sent_later();
    reads_every_step_of_the_longest_walk();
    refuses_what_does_not_fit();
    a_walk_from_standing_gives_each_cell_its_tick();
    a_delayed_walk_stands_first();
    a_step_under_way_started_before_the_record();
    a_standing_record_is_one_cell();
    the_record_that_holds_is_the_latest_one_due();
    a_newer_record_removes_what_it_replaces();
    what_waits_for_later_can_be_dropped();
    a_full_track_lets_its_earliest_go();
    ticks_may_wrap();
    any_order_leaves_the_same_track();
    a_record_twice_or_overtaken_is_dropped();
    what_was_played_is_a_floor();
    revisions_may_wrap();
    a_standing_record_is_walked_to_or_set();
    puts("move_track: ok");
    return 0;
}
