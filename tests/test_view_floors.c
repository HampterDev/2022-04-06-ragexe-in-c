#include "world/presence.h"
#include "world/view_floors.h"
#include <stdio.h>
#include <string.h>

/* The floors of units by their id and where their records go (world/view_floors.h,
   world/presence.h, plans/presence.md part 2). */

static int failures;

#define CHECK(x)                                                         \
    do {                                                                 \
        if (!(x)) {                                                      \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);      \
            failures++;                                                  \
        }                                                                \
    } while (0)

static uint32_t g_seed;

static uint32_t rnd(void)
{
    g_seed ^= g_seed << 13;
    g_seed ^= g_seed >> 17;
    g_seed ^= g_seed << 5;
    return g_seed;
}

static void put16(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
}

static void put32(unsigned char *p, uint32_t v)
{
    put16(p, v & 0xFFFFu);
    put16(p + 2, v >> 16);
}

static struct ViewStamp stamp_of(uint32_t gid, uint32_t tick, int kind, uint16_t move, uint16_t act, uint16_t vitals)
{
    struct ViewStamp stamp;

    memset(&stamp, 0, sizeof stamp);
    stamp.gid = gid;
    stamp.tick = tick;
    stamp.kind = (unsigned char)kind;
    stamp.rev_move = move;
    stamp.rev_act = act;
    stamp.rev_vitals = vitals;
    return stamp;
}

/* The packet: read bounded, an unknown kind refused. */
static void test_read(void)
{
    static struct ViewFloors view;
    unsigned char packet[VIEW_STAMP_SIZE + 4];
    struct ViewStamp stamp;

    memset(packet, 0xEE, sizeof packet);
    put16(packet, 0x0F26);
    put32(packet + 2, 0x01020304u);
    put32(packet + 6, 0xFFFFFFF0u);
    packet[10] = VIEW_GOES;
    put16(packet + 11, 0x1111);
    put16(packet + 13, 0x2222);
    put16(packet + 15, 0xFFFF);
    CHECK(view_stamp_read(packet, VIEW_STAMP_SIZE, &stamp) == 1);
    CHECK(stamp.gid == 0x01020304u && stamp.tick == 0xFFFFFFF0u && stamp.kind == VIEW_GOES);
    CHECK(stamp.rev_move == 0x1111 && stamp.rev_act == 0x2222 && stamp.rev_vitals == 0xFFFF);
    memset(&stamp, 0, sizeof stamp);
    CHECK(view_stamp_read(packet, VIEW_STAMP_SIZE - 1, &stamp) == 0 && stamp.gid == 0);
    CHECK(view_stamp_read(packet, 0, &stamp) == 0);
    packet[10] = 2;
    CHECK(view_stamp_read(packet, VIEW_STAMP_SIZE, &stamp) == 0 && stamp.gid == 0);

    /* a stamp belongs to the very next packet, if that is its unit's entry or vanish */
    {
        unsigned char next[16];

        memset(next, 0, sizeof next);
        stamp = stamp_of(0x01020304u, 5, VIEW_GOES, 1, 1, 1);
        put16(next, 0x0080);
        put32(next + 2, 0x01020304u);
        CHECK(view_stamp_stamps(&stamp, 0x0080, next, 7));
        CHECK(!view_stamp_stamps(&stamp, 0x0080, next, 5));          /* too short to say whose */
        put32(next + 2, 0x01020305u);
        CHECK(!view_stamp_stamps(&stamp, 0x0080, next, 7));          /* another unit's vanish */
        put32(next + 5, 0x01020304u);
        CHECK(!view_stamp_stamps(&stamp, 0x09fe, next, 16));         /* a going before an entry */
        stamp.kind = VIEW_COMES;
        CHECK(view_stamp_stamps(&stamp, 0x09fd, next, 16) && view_stamp_stamps(&stamp, 0x09fe, next, 16)
              && view_stamp_stamps(&stamp, 0x09ff, next, 9));
        CHECK(!view_stamp_stamps(&stamp, 0x09ff, next, 8));
        put32(next + 5, 0x01020306u);
        CHECK(!view_stamp_stamps(&stamp, 0x09fd, next, 16));         /* another unit's entry */
        put32(next + 5, 0x01020304u);
        CHECK(!view_stamp_stamps(&stamp, 0x0f21, next, 16));         /* any other packet */
        put32(next + 2, 0x01020304u);
        CHECK(!view_stamp_stamps(&stamp, 0x0080, next, 7));          /* a coming before a vanish */
    }

    /* gid 0 is no unit */
    view_floors_clear(&view);
    CHECK(view_floors_unit(&view, 0) == NULL);
    stamp = stamp_of(0, 5, VIEW_COMES, 1, 1, 1);
    CHECK(view_floors_stamp(&view, &stamp) == NULL && view.count == 0);
}

/* A coming: what is older than its revisions is the past, what is equal or newer is not. A going:
   only what is newer. Floors never go back, and revisions wrap. */
static void test_floors(void)
{
    static struct ViewFloors view;
    struct ViewStamp stamp;
    const struct ViewUnit *unit;

    view_floors_clear(&view);
    CHECK(!view_floors_in_view(&view, 9));
    stamp = stamp_of(9, 1000, VIEW_COMES, 5, 0, 0xFFFF);
    unit = view_floors_stamp(&view, &stamp);
    CHECK(unit != NULL && unit->in_view && view_floors_in_view(&view, 9) && unit->came == 1000);
    CHECK(unit->floors.has_move && unit->floors.move == 4);       /* 5 is taken, 4 is not */
    CHECK(unit->floors.has_act && unit->floors.act == 0xFFFF);    /* nothing made yet: 1 is newer */
    CHECK(record_rev_newer(1, unit->floors.act));
    CHECK(unit->floors.vitals == 0xFFFE && record_rev_newer(0xFFFF, unit->floors.vitals));
    stamp = stamp_of(9, 1500, VIEW_GOES, 8, 2, 3);
    unit = view_floors_stamp(&view, &stamp);
    CHECK(unit && !unit->in_view && !view_floors_in_view(&view, 9) && unit->went == 1500);
    CHECK(unit->floors.move == 8 && unit->floors.act == 2 && unit->floors.vitals == 3);
    /* notices: before the coming, or at the going and before it while out of view */
    CHECK(view_floors_past_notice(&view, 9, 999));
    CHECK(view_floors_past_notice(&view, 9, 1500) && view_floors_past_notice(&view, 9, 1200));
    CHECK(!view_floors_past_notice(&view, 9, 1501));
    CHECK(!view_floors_past_notice(&view, 77, 1));
    /* a coming whose revisions are behind the floors does not lower them */
    stamp = stamp_of(9, 1600, VIEW_COMES, 3, 3, 3);
    unit = view_floors_stamp(&view, &stamp);
    CHECK(unit->floors.move == 8 && unit->floors.act == 2 && unit->floors.vitals == 3 && unit->in_view);
    CHECK(view_floors_past_notice(&view, 9, 1599) && !view_floors_past_notice(&view, 9, 1600));
    CHECK(!view_floors_past_notice(&view, 9, 1650)); /* in view again: only the coming counts */

    /* the own character: in view without a coming, and its stamps say nothing */
    view_floors_pin(&view, 3);
    CHECK(view_floors_in_view(&view, 3));
    stamp = stamp_of(3, 10, VIEW_GOES, 9, 9, 9);
    CHECK(view_floors_stamp(&view, &stamp) == NULL && view_floors_in_view(&view, 3));
    CHECK(view_floors_find(&view, 3)->floors.has_move == 0);
    view_floors_clear(&view);
    CHECK(view.count == 0 && !view_floors_in_view(&view, 3) && !view_floors_in_view(&view, 9));
}

/* Bounded: when every place is taken, the unit out of view that was asked for longest ago gives
   its place, and one in view never does. */
static void test_bounded(void)
{
    static struct ViewFloors view;
    struct ViewStamp stamp;
    uint32_t gid;

    view_floors_clear(&view);
    for (gid = 1; gid <= VIEW_FLOORS_UNITS; gid++) {
        stamp = stamp_of(gid, gid, VIEW_COMES, 1, 1, 1);
        CHECK(view_floors_stamp(&view, &stamp) != NULL);
    }
    CHECK(view.count == VIEW_FLOORS_UNITS);
    /* all in view: a new one has no place, and nothing was lost */
    CHECK(view_floors_unit(&view, 5000) == NULL && view_floors_in_view(&view, 1));
    stamp = stamp_of(5000, 1, VIEW_COMES, 1, 1, 1);
    CHECK(view_floors_stamp(&view, &stamp) == NULL);
    /* three go; the one asked for since stays longest */
    stamp = stamp_of(10, 20, VIEW_GOES, 1, 1, 1);
    view_floors_stamp(&view, &stamp);
    stamp = stamp_of(11, 21, VIEW_GOES, 1, 1, 1);
    view_floors_stamp(&view, &stamp);
    stamp = stamp_of(12, 22, VIEW_GOES, 1, 1, 1);
    view_floors_stamp(&view, &stamp);
    CHECK(view_floors_unit(&view, 10) != NULL); /* asked for again */
    CHECK(view_floors_unit(&view, 6000) != NULL && view_floors_find(&view, 11) == NULL);
    CHECK(view_floors_unit(&view, 6001) != NULL && view_floors_find(&view, 12) == NULL);
    CHECK(view_floors_unit(&view, 6002) != NULL && view_floors_find(&view, 10) == NULL);
    CHECK(view_floors_unit(&view, 6003) != NULL && view_floors_find(&view, 6000) == NULL);
    for (gid = 13; gid <= VIEW_FLOORS_UNITS; gid++)
        CHECK(view_floors_in_view(&view, gid));
}

/* --- check 1: any order, across stays ---------------------------------------------------- */

enum { UNITS = 2, EVENTS = 48 };
enum { EV_STAMP, EV_MOVE, EV_TIMED };

struct Event {
    int kind;
    uint32_t gid;
    struct ViewStamp stamp;
    struct MoveRecord move;
    struct TimedItem timed;
};

/* The world as the test has it: one move track a unit, and whether its actor is there. */
struct World {
    struct ViewFloors view;
    struct TimedQueue queue;
    struct RecordWait wait;
    struct MoveTrack track[UNITS];
    int lingers; /* an actor stays after its unit went, as a dying one is shown */
    int had[UNITS];
};

static struct MoveTrack *track_of(void *context, uint32_t gid)
{
    struct World *world = (struct World *)context;
    int at = (int)gid - 1;

    if (at < 0 || at >= UNITS)
        return NULL;
    if (!view_floors_in_view(&world->view, gid) && !(world->lingers && world->had[at]))
        return NULL;
    return &world->track[at];
}

static struct Presence presence_of(struct World *world)
{
    struct Presence presence;

    presence.view = &world->view;
    presence.queue = &world->queue;
    presence.wait = &world->wait;
    presence.own = 99;
    presence.track_of = track_of;
    presence.context = world;
    return presence;
}

static void deliver(struct World *world, const struct Event *event)
{
    struct Presence presence = presence_of(world);
    struct TimedItem early;
    int filed;

    switch (event->kind) {
    case EV_STAMP:
        presence_stamp(&presence, 0, &event->stamp);
        if (event->stamp.kind == VIEW_COMES)
            world->had[event->gid - 1] = 1;
        break;
    case EV_MOVE:
        presence_file_move(&presence, 0, event->gid, &event->move);
        break;
    default:
        CHECK(presence_file_timed(&presence, 0, &event->timed, &early) != TIMED_FILED_EARLY);
    }
    /* what waited and has its unit now is filed, as the game does each frame */
    while (presence_file_waiting(&presence, 0, &early, &filed))
        CHECK(filed != TIMED_FILED_EARLY);
}

/* A unit's stays: a coming, records of its three tracks and hits, a going; the last stay may
   not end. Ticks rise with what is made, the revisions count on across stays by unit. */
static int make_events(struct Event *events)
{
    uint16_t rev[UNITS][3];
    uint32_t tick = 0xFFFFF000u; /* the clock wraps on the way */
    int count = 0, stays[UNITS], in_view[UNITS], moves[UNITS], u, k;

    memset(rev, 0, sizeof rev);
    for (u = 0; u < UNITS; u++) {
        stays[u] = 0;
        moves[u] = 0;
        in_view[u] = 0;
        rev[u][0] = (uint16_t)(0xFFF0u + rnd() % 32u);
        rev[u][1] = (uint16_t)rnd();
        rev[u][2] = (uint16_t)rnd();
    }
    while (count < EVENTS) {
        struct Event *e = &events[count];

        /* every unit's stays are over */
        for (u = 0; u < UNITS && !in_view[u] && stays[u] == 3; u++) {
        }
        if (u == UNITS)
            break;

        memset(e, 0, sizeof *e);
        u = (int)(rnd() % UNITS);
        e->gid = (uint32_t)u + 1;
        tick += 1 + rnd() % 40u;
        k = (int)(rnd() % 8u);
        if (!in_view[u] || (k == 0 && count < EVENTS - 2)) {
            /* the first stay begins, or one ends, or another begins */
            if (in_view[u] || stays[u] < 3) {
                e->kind = EV_STAMP;
                e->stamp = stamp_of(e->gid, tick, in_view[u] ? VIEW_GOES : VIEW_COMES, rev[u][0], rev[u][1], rev[u][2]);
                if (!in_view[u])
                    stays[u]++;
                in_view[u] = !in_view[u];
                count++;
            }
            continue;
        }
        if (k <= 3 && moves[u] < MOVE_TRACK_HELD) {
            /* no more than a track holds: nothing is played here, and a full track lets its
               earliest go, which depends on what came first */
            moves[u]++;
            e->kind = EV_MOVE;
            e->move.at_tick = tick;
            e->move.x = (int)(rnd() % 50u);
            e->move.rev = ++rev[u][0];
        } else if (k > 3) {
            e->kind = EV_TIMED;
            e->timed.tick = tick;
            e->timed.actor = e->gid;
            if (k == 4) {
                e->timed.kind = TIMED_HIT;
                e->timed.u.hit.source = e->gid;
                e->timed.u.hit.target = 99;
                e->timed.u.hit.at_tick = tick;
                e->timed.u.hit.damage = (int32_t)(rnd() % 100u);
            } else if (k <= 6) {
                e->timed.kind = TIMED_ACT;
                e->timed.u.act.gid = e->gid;
                e->timed.u.act.at_tick = tick;
                e->timed.u.act.kind = (unsigned char)(rnd() % 2u);
                e->timed.u.act.target = 7;
                e->timed.u.act.rev = ++rev[u][1];
            } else {
                e->timed.kind = TIMED_VITALS;
                e->timed.u.vitals.gid = e->gid;
                e->timed.u.vitals.at_tick = tick;
                e->timed.u.vitals.hp = (int32_t)(rnd() % 1000u);
                e->timed.u.vitals.rev = ++rev[u][2];
            }
        } else {
            continue;
        }
        count++;
    }
    return count;
}

static int same_world(const struct World *a, const struct World *b)
{
    int u, i;

    if (a->queue.count != b->queue.count || a->wait.count != b->wait.count)
        return 0;
    for (i = 0; i < a->queue.count; i++)
        if (memcmp(&a->queue.items[i], &b->queue.items[i], sizeof a->queue.items[i]) != 0)
            return 0;
    for (u = 0; u < UNITS; u++) {
        const struct ViewUnit *x = view_floors_find(&a->view, (uint32_t)u + 1);
        const struct ViewUnit *y = view_floors_find(&b->view, (uint32_t)u + 1);

        if (a->track[u].count != b->track[u].count)
            return 0;
        for (i = 0; i < a->track[u].count; i++)
            if (a->track[u].held[i].rev != b->track[u].held[i].rev || a->track[u].held[i].at_tick != b->track[u].held[i].at_tick)
                return 0;
        if ((x == NULL) != (y == NULL))
            return 0;
        if (x && (memcmp(&x->floors, &y->floors, sizeof x->floors) != 0 || x->in_view != y->in_view))
            return 0;
    }
    return 1;
}

/* Filed in any order a seed gives, the stamps among themselves in the order they were sent (the
   world channel keeps it) and the records and hits anywhere, a third of the records twice (a hit
   has no revision, and the delivery piece hands each message on once): the tracks, the queue and
   the floors are those of filing everything in the order it was made. */
static void test_any_order(void)
{
    static struct World in_order, shuffled;
    static struct Event events[EVENTS], order[EVENTS * 2];
    int round, count, n, i, j, lingers, compared = 0, differ = 0;

    for (round = 0; round < 3000; round++) {
        g_seed = 0x9E3779B9u ^ (uint32_t)round * 2654435761u;
        if (!g_seed)
            g_seed = 1;
        lingers = round % 2;
        count = make_events(events);
        memset(&in_order, 0, sizeof in_order);
        in_order.lingers = lingers;
        for (i = 0; i < count; i++)
            deliver(&in_order, &events[i]);

        /* the records shuffled, a third of them twice; the stamps keep their order */
        n = 0;
        for (i = 0; i < count; i++) {
            order[n++] = events[i];
            if (events[i].kind != EV_STAMP && !(events[i].kind == EV_TIMED && events[i].timed.kind == TIMED_HIT)
                && rnd() % 3u == 0)
                order[n++] = events[i];
        }
        for (i = n - 1; i > 0; i--) {
            struct Event swap;

            j = (int)(rnd() % (uint32_t)(i + 1));
            if (order[i].kind == EV_STAMP && order[j].kind == EV_STAMP)
                continue;
            swap = order[i];
            order[i] = order[j];
            order[j] = swap;
        }
        /* put the stamps back in the order they were sent, into the places stamps took */
        {
            int s = 0;

            for (i = 0; i < n; i++) {
                if (order[i].kind != EV_STAMP)
                    continue;
                while (events[s].kind != EV_STAMP)
                    s++;
                order[i] = events[s++];
            }
        }
        memset(&shuffled, 0, sizeof shuffled);
        shuffled.lingers = lingers;
        for (i = 0; i < n; i++)
            deliver(&shuffled, &order[i]);
        compared++;
        if (!same_world(&in_order, &shuffled)) {
            if (!differ++)
                fprintf(stderr, "round %d: the order of arrival changed the world\n", round);
        }
        CHECK(in_order.wait.count == 0);
    }
    CHECK(compared == 3000 && differ == 0);
}

/* A record that overtakes its unit's going is of the later stay: it is not lost with the stay
   that ends, and one made before the going that arrives after the coming is not played. */
static void test_crossing(void)
{
    static struct World world;
    struct Presence presence;
    struct MoveRecord late, early_move;
    struct TimedItem item, early;
    struct ViewStamp stamp;
    int filed;

    memset(&world, 0, sizeof world);
    presence = presence_of(&world);
    stamp = stamp_of(1, 100, VIEW_COMES, 10, 20, 30);
    presence_stamp(&presence, 0, &stamp);
    world.had[0] = 1;
    memset(&late, 0, sizeof late);
    late.at_tick = 400;
    late.rev = 12; /* made after the unit came back, at 300 */
    CHECK(presence_file_move(&presence, 0, 1, &late) == 1 && world.track[0].count == 1);
    stamp = stamp_of(1, 200, VIEW_GOES, 11, 20, 30);
    presence_stamp(&presence, 0, &stamp);
    CHECK(world.track[0].count == 0 && world.wait.count == 1);
    memset(&early_move, 0, sizeof early_move);
    early_move.at_tick = 150;
    early_move.rev = 11; /* made before the going, arrives after the coming */
    stamp = stamp_of(1, 300, VIEW_COMES, 11, 20, 30);
    presence_stamp(&presence, 0, &stamp);
    while (presence_file_waiting(&presence, 0, &early, &filed)) {
    }
    CHECK(world.track[0].count == 1 && world.track[0].held[0].rev == 12);
    CHECK(presence_file_move(&presence, 0, 1, &early_move) == 0 && world.track[0].count == 1);
    /* a hit of the stay that ended is not shown; one of this stay is */
    memset(&item, 0, sizeof item);
    item.kind = TIMED_HIT;
    item.actor = 1;
    item.tick = 199;
    CHECK(presence_file_timed(&presence, 0, &item, &early) == TIMED_DROPPED && world.queue.count == 0);
    item.tick = 300;
    CHECK(presence_file_timed(&presence, 0, &item, &early) == TIMED_FILED && world.queue.count == 1);
    /* gone, with its actor still shown (dying): a record of a later stay waits and is not filed
       on that actor, and one of the stay that ended does not even wait */
    world.lingers = 1;
    stamp = stamp_of(1, 600, VIEW_GOES, 12, 20, 30);
    presence_stamp(&presence, 0, &stamp);
    CHECK(world.track[0].count == 0 && world.wait.count == 0);
    late.rev = 13;
    late.at_tick = 700;
    CHECK(presence_file_move(&presence, 0, 1, &late) == 0 && world.track[0].count == 0 && world.wait.count == 1);
    late.rev = 12;
    CHECK(presence_file_move(&presence, 0, 1, &late) == 0 && world.wait.count == 1);
    /* the own character's stamps change nothing */
    stamp = stamp_of(99, 500, VIEW_GOES, 1, 1, 1);
    CHECK(presence_stamp(&presence, 0, &stamp) == 0);
}

/* The rules the move track and the timed queue each wrote out before they asked record_against,
   as they were: whether the arriving record is dropped, and whether a held one stays. */
static int move_track_refused(uint32_t held_tick, uint16_t held_rev, uint32_t tick, uint16_t rev)
{
    return held_rev == rev || (record_rev_newer(held_rev, rev) && (int32_t)(held_tick - tick) <= 0);
}

static int move_track_keeps(uint32_t held_tick, uint16_t held_rev, uint32_t tick, uint16_t rev)
{
    return record_rev_newer(held_rev, rev) || (int32_t)(held_tick - tick) < 0;
}

static int timed_queue_replaces(uint32_t held_tick, uint16_t held_rev, uint32_t tick, uint16_t rev)
{
    return record_rev_newer(rev, held_rev) && (int32_t)(held_tick - tick) >= 0;
}

/* One rule for every track: refused, replacing, or beside, the revisions wrapping; and the same
   answers as the rules it took over, but for revisions half the counter apart, where neither is
   newer and the move track dropped a held record whose tick was not earlier. */
static void test_record_against(void)
{
    static const uint32_t ticks[] = { 0, 1, 999, 1000, 1001, 0x7FFFFFFFu, 0x80000000u, 0xFFFFFFFFu };
    static const uint16_t revs[] = { 0, 1, 2, 0x7FFF, 0x8000, 0x8001, 0xFFFE, 0xFFFF };
    unsigned i, j, k, l;

    CHECK(record_against(1000, 5, 1000, 5) == RECORD_REFUSED);  /* it came twice */
    CHECK(record_against(900, 5, 1000, 5) == RECORD_REFUSED);
    CHECK(record_against(1000, 6, 1000, 5) == RECORD_REFUSED);  /* made knowing it */
    CHECK(record_against(900, 6, 1000, 5) == RECORD_REFUSED);
    CHECK(record_against(1100, 6, 1000, 5) == RECORD_BESIDE);   /* older and earlier */
    CHECK(record_against(1000, 5, 1000, 6) == RECORD_REPLACES); /* a cancelled swing */
    CHECK(record_against(1100, 5, 1000, 6) == RECORD_REPLACES);
    CHECK(record_against(900, 5, 1000, 6) == RECORD_BESIDE);    /* newer and later */
    CHECK(record_against(1000, 0xFFFF, 1000, 1) == RECORD_REPLACES); /* across the wrap */
    CHECK(record_against(1000, 1, 1000, 0xFFFF) == RECORD_REFUSED);
    CHECK(record_against(0xFFFFFFF0u, 5, 0x10, 6) == RECORD_BESIDE); /* the tick wraps too */
    for (i = 0; i < sizeof ticks / sizeof ticks[0]; i++)
        for (j = 0; j < sizeof revs / sizeof revs[0]; j++)
            for (k = 0; k < sizeof ticks / sizeof ticks[0]; k++)
                for (l = 0; l < sizeof revs / sizeof revs[0]; l++) {
                    uint32_t ht = ticks[i], t = ticks[k];
                    uint16_t hr = revs[j], r = revs[l];
                    int got = record_against(ht, hr, t, r), half = (uint16_t)(hr - r) == 0x8000u;

                    CHECK((got == RECORD_REFUSED) == move_track_refused(ht, hr, t, r));
                    CHECK((got == RECORD_REPLACES) == timed_queue_replaces(ht, hr, t, r));
                    if (got != RECORD_REFUSED && !half)
                        CHECK((got != RECORD_REPLACES) == move_track_keeps(ht, hr, t, r));
                    if (got != RECORD_REFUSED && half)
                        CHECK(got == RECORD_BESIDE && move_track_keeps(ht, hr, t, r) == ((int32_t)(ht - t) < 0));
                }
}

int main(void)
{
    test_record_against();
    test_read();
    test_floors();
    test_bounded();
    test_crossing();
    test_any_order();
    if (failures) {
        fprintf(stderr, "test_view_floors: %d failed\n", failures);
        return 1;
    }
    printf("test_view_floors: ok\n");
    return 0;
}
