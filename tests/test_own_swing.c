/* The player's own swing shown ahead of the server's record (world/own_swing.h): what the
   predicted world's word of the swing asks of what is shown, and what the server's record of a
   swing makes of it. */
#include "world/own_swing.h"
#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            failures++;                                                     \
        }                                                                   \
    } while (0)

enum { OWN = 7, MONSTER = 100, OTHER = 101 };

/* The server's record of a swing of the player's own: an attack motion of `motion`, landing
   `impact` after its start. */
static struct ActRecord swing(uint32_t tick, uint32_t target, unsigned motion, unsigned impact)
{
    struct ActRecord record;

    memset(&record, 0, sizeof record);
    record.gid = OWN;
    record.at_tick = tick;
    record.kind = ACT_SWING;
    record.end_tick = tick + motion;
    record.mark_tick = tick + impact;
    record.target = target;
    record.motion_ms = motion;
    return record;
}

/* The world's swing is shown once, kept while the world has it, and left alone once landed. */
static void test_shown_once(void)
{
    struct OwnSwing own;
    struct ActRecord record;

    own_swing_reset(&own);
    CHECK(own_swing_follow(&own, 0, 0, 0, 0, 5000) == OWN_SWING_KEEP && !own.shown);
    /* the body begins a swing at 5000 that lands at 5300 */
    CHECK(own_swing_follow(&own, 1, MONSTER, 5000, 5300, 5000) == OWN_SWING_SHOW);
    CHECK(own.shown == OWN_SWING_AHEAD && own.target == MONSTER && own.at_tick == 5000 && own.lands == 5300);
    CHECK(own_swing_is_shown(&own, MONSTER, 5000) && own_swing_is_shown(&own, MONSTER, 5250));
    CHECK(!own_swing_is_shown(&own, MONSTER, 5251) && !own_swing_is_shown(&own, OTHER, 5000));
    /* its record: the swing as the world has it */
    own_swing_shown(&own, OWN, 300, &record);
    CHECK(record.gid == OWN && record.kind == ACT_SWING && record.target == MONSTER && record.at_tick == 5000);
    CHECK(record.mark_tick == 5300 && record.motion_ms == 300 && record.end_tick == 5300);
    /* every frame the world has it: nothing */
    CHECK(own_swing_follow(&own, 1, MONSTER, 5000, 5300, 5016) == OWN_SWING_KEEP);
    /* a word of the server's that has it begin a little earlier or later: the same swing */
    CHECK(own_swing_follow(&own, 1, MONSTER, 4960, 5260, 5100) == OWN_SWING_KEEP && own.at_tick == 5000);
    CHECK(own_swing_follow(&own, 1, MONSTER, 5250, 5550, 5260) == OWN_SWING_KEEP);
    /* the server's record of it is not played again */
    record = swing(5020, MONSTER, 300, 300);
    CHECK(own_swing_record(&own, &record) == 1 && own.shown == OWN_SWING_AHEAD && own.at_tick == 5000);
    /* landed, the world has it no more: nothing is taken back */
    CHECK(own_swing_follow(&own, 0, 0, 0, 0, 5300) == OWN_SWING_KEEP && own.shown);
    /* the next swing of the attack is another one */
    CHECK(own_swing_follow(&own, 1, MONSTER, 5600, 5900, 5600) == OWN_SWING_SHOW && own.at_tick == 5600);
}

/* A swing the world has no more before it landed is taken back; another one takes its place. */
static void test_taken_back(void)
{
    struct OwnSwing own;

    own_swing_reset(&own);
    CHECK(own_swing_follow(&own, 1, MONSTER, 5000, 5300, 5000) == OWN_SWING_SHOW);
    /* a walk ended it in its windup */
    CHECK(own_swing_follow(&own, 0, 0, 0, 0, 5100) == OWN_SWING_TAKE_BACK && !own.shown);
    CHECK(own_swing_follow(&own, 0, 0, 0, 0, 5116) == OWN_SWING_KEEP);
    /* the same swing predicted again is shown again */
    CHECK(own_swing_follow(&own, 1, MONSTER, 5000, 5300, 5132) == OWN_SWING_SHOW);
    /* the world has another in its place: at someone else, or begun much later */
    CHECK(own_swing_follow(&own, 1, OTHER, 5100, 5400, 5148) == OWN_SWING_REPLACE);
    CHECK(own.target == OTHER && own.at_tick == 5100 && own.lands == 5400);
    CHECK(own_swing_follow(&own, 1, OTHER, 5500, 5800, 5200) == OWN_SWING_REPLACE);
    /* a swing of the world's that has landed already is not shown after the fact, and what is
       shown and has not landed is then not the world's */
    CHECK(own_swing_follow(&own, 1, OTHER, 4000, 4300, 5300) == OWN_SWING_TAKE_BACK && !own.shown);
    CHECK(own_swing_follow(&own, 1, OTHER, 4000, 4300, 5316) == OWN_SWING_KEEP && !own.shown);
}

/* A swing the server told and the world had not predicted is the server's: played from its
   record, not shown a second time, and not the world's to take back. */
static void test_told(void)
{
    struct OwnSwing own;
    struct ActRecord record = swing(5000, MONSTER, 300, 300);

    own_swing_reset(&own);
    CHECK(own_swing_record(&own, &record) == 0 && own.shown == OWN_SWING_TOLD && own.at_tick == 5000 && own.lands == 5300);
    /* the world does not have it: it stays */
    CHECK(own_swing_follow(&own, 0, 0, 0, 0, 5050) == OWN_SWING_KEEP && own.shown == OWN_SWING_TOLD);
    /* the world has it, begun whenever: it is that swing */
    CHECK(own_swing_follow(&own, 1, MONSTER, 5000, 5300, 5060) == OWN_SWING_KEEP);
    CHECK(own_swing_follow(&own, 1, MONSTER, 4600, 5290, 5070) == OWN_SWING_KEEP && own.shown == OWN_SWING_TOLD);
    /* the same record again (sent twice) is the one shown */
    CHECK(own_swing_record(&own, &record) == 1);
    /* at someone else the world's is another swing, shown beside the server's */
    CHECK(own_swing_follow(&own, 1, OTHER, 5100, 5400, 5100) == OWN_SWING_SHOW && own.shown == OWN_SWING_AHEAD);
    /* a record of another swing while one is shown ahead takes its place in the book */
    record = swing(6000, MONSTER, 300, 300);
    CHECK(own_swing_record(&own, &record) == 0 && own.shown == OWN_SWING_TOLD && own.target == MONSTER);
    /* ticks wrap */
    own_swing_reset(&own);
    CHECK(own_swing_follow(&own, 1, MONSTER, 0xFFFFFF00u, 0x00000040u, 0xFFFFFF00u) == OWN_SWING_SHOW);
    CHECK(own_swing_follow(&own, 1, MONSTER, 0xFFFFFF00u, 0x00000040u, 0x00000010u) == OWN_SWING_KEEP);
    CHECK(own_swing_follow(&own, 0, 0, 0, 0, 0x00000020u) == OWN_SWING_TAKE_BACK);
}

int main(void)
{
    test_shown_once();
    test_taken_back();
    test_told();
    if (failures) {
        fprintf(stderr, "own_swing: %d failures\n", failures);
        return 1;
    }
    puts("own_swing: ok");
    return 0;
}
