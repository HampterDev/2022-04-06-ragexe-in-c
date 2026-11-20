#ifndef BODY_BODY_H
#define BODY_BODY_H

/* The body core (docs/body-core.md): where an actor is and what it does next, as a pure
 * function over one block of memory. Freestanding C11: these two headers only, no allocation,
 * no globals, no clock, no library call, integer math only.
 *
 * A body is an actor as far as time and place go. So far the core makes bodies walk: a walk to
 * a cell along the server's path (grid_path.h), to within a range of a cell, or after another
 * body until it is within a range of it; a turn onto a new target at the next cell, a pause
 * when something holds the body, a stop, a change of speed. And it makes them attack: the
 * swing after swing of a basic attack, each no sooner than the attack delay allows and not in
 * a cast, with a windup before it lands, walking after a target that is out of reach. Players,
 * monsters and other units (a homunculus, a mercenary) attack by the same rule, with what the
 * fork does differently for each kind (BODY_FIGHTER_*): a monster's way after its target is the
 * game's to find, so the core says where it is to go (BODY_EVENT_CHASE) and does not walk it.
 * A body that is to swing while it walks is not stopped on the cell it is leaving, as the fork stops
 * it: the step it has under way lands first, the walk ends there, and the swing is looked at
 * again on that cell. So what is drawn of a body in the middle of a step never has to go back. A
 * swing that a command ends in its windup holds the body from walking no longer: a player who
 * walks away from a swing walks at once. What only
 * the game's content knows, it asks (below). Casts and what a body's owner wants (intents) come
 * on top.
 *
 * What a body's owner asks for is a request (BODY_INPUT_REQUEST): a walk, an attack, a skill, a
 * pickup, the end of one. The core does all that follows from it: a walk ends what the body was
 * after and the swing it had begun; an attack, a skill or a pickup walks the body into reach and
 * has the act carried out, and an attack swings and goes on. The same requests at the same ticks
 * in a copy of the world give the same body, which is how a client shows its own character ahead
 * of the server (body_state, body_put_state).
 *
 * The arena. The caller hands in the memory, body_world_size() bytes aligned to 8. It holds
 * the world's header, a fixed array of body slots and a fixed ring of events. Nothing in it is
 * a pointer and every field has a fixed width, so a snapshot is a copy of the bytes, and the
 * same inputs give the same bytes on every little-endian build. A slot that is free and an
 * event that was taken are zero.
 *
 * The ground is the caller's: which cells can be walked, and working memory for a path search
 * (struct BodyGround). It is read when a walk is asked for and when a step lands, so a cell
 * that changes needs no input of its own.
 *
 * Time. Ticks are milliseconds in 64 bits and never wrap. The 32-bit ticks of the wire are
 * the caller's to make from them. The core has no timers: a walking body's next step is due
 * at a tick, and the caller drives the world there:
 *
 *     while (body_next_due(world, &due) && due <= until) {
 *         body_run(world, ground, due);
 *         while (body_take_event(world, &event))
 *             ...
 *     }
 *
 * What is due at a tick runs before an input of that tick: body_input refuses an input while
 * something at or before its tick is due.
 *
 * Questions. Whether a body may hit another, whether it is casting, whether a wall is in the
 * line of fire, whether a hit was made: the core does not know, so it asks with an event
 * (BODY_EVENT_ASK_*) and the body waits for the answer (BODY_INPUT_ANSWER). Nothing at a later
 * tick runs while a question is open: body_run and body_input say BODY_ANSWER_FIRST. Inputs at
 * the tick of the question are taken, so that what the caller does about a hit (a body that
 * dies and is removed, an attack that is stopped) comes before its answer.
 *
 * Inputs are data from outside. Every field is checked, and an input that does not fit is
 * refused with a code and changes nothing, the clock included. */

#include <stddef.h>
#include <stdint.h>

/* Goes up with every change to what the core does with an input. A server and a client with
 * different versions do not predict each other. */
#define BODY_RULES_VERSION 14u

/* The most slots a world has; a slot number fits 16 bits. */
#define BODY_MAX_BODIES 0xFFFFu
/* The least and the most events a world's ring holds. One step landing makes up to four, one
 * look of an attack up to six. */
#define BODY_MIN_EVENTS 8u
#define BODY_MAX_EVENTS 0x100000u
/* The widest and tallest map; a cell fits a signed 16-bit number. */
#define BODY_MAX_SIDE 0x8000
/* The steps of one walk. */
#define BODY_PATH_STEPS 32
/* The slowest walk, in milliseconds a straight cell, and the longest hold: together a step's
 * landing stays within the 16 bits a move record has for it. */
#define BODY_MAX_SPEED_MS 10000u
#define BODY_MAX_DELAY_MS 50000u
/* The widest range a walk can be asked to come within, and the longest attack range, in cells. */
#define BODY_MAX_RANGE 64u
/* As far as a body sees: beyond it there is no line of fire (AREA_SIZE). */
#define BODY_SIGHT 22
/* The working memory of a path search, in bytes (GRID_PATH_SCRATCH_SIZE). */
#define BODY_SCRATCH_SIZE 18456u

enum {
    BODY_OK = 0,
    BODY_BAD_WORLD = 1,   /* body_world_check: the bytes are not a world */
    BODY_BAD_KIND = 2,    /* an input kind the core does not know */
    BODY_BAD_SLOT = 3,    /* no such slot; a slot in use for an add; a free slot, or another id's, otherwise;
                             a chase after no body, or after itself */
    BODY_BAD_TICK = 4,    /* before the world's clock */
    BODY_BAD_CELL = 5,    /* off the map */
    BODY_BAD_VALUE = 6,   /* a direction, cause, speed, delay or range that does not exist */
    BODY_EVENTS_FULL = 7, /* the ring has no room for the events this makes; take events and call again */
    BODY_NO_PATH = 8,     /* a walk: no way there from the body's cell, or none short enough */
    BODY_DUE_FIRST = 9,   /* something is due at or before this tick (before it, for body_run): run it first */
    BODY_ANSWER_FIRST = 10 /* a question is open: answer it. From body_run also: what could run has run,
                              answer and call again with the same tick */
};

/* How a body came to its cell. */
enum { BODY_CAUSE_WALKED = 0, BODY_CAUSE_PUSHED = 1, BODY_CAUSE_PLACED = 2 };

/* A body's move state, which is also its move record: complete from at_tick on, without what
 * was true before. The body is on (x, y) at at_tick. With steps, the first lands land_ms after
 * at_tick, whatever its own time is: longer, the body stands first; shorter, the step was
 * under way. Each later step takes its own time at speed_ms a straight cell
 * (grid_path_step_ms). Without steps, land_ms is how long a push (cause pushed) takes to show,
 * which no rule reads; it is 0 for every other move without steps. Directions are the server's
 * codes, 0 north (y + 1) and on counter-clockwise to 7 north-east. */
struct BodyMove {
    uint64_t at_tick;
    int16_t x, y;
    uint16_t land_ms, speed_ms;
    uint8_t dir, cause, steps;
    uint8_t reserved[5];            /* zero */
    uint8_t path[BODY_PATH_STEPS]; /* one direction a step; zero past `steps` */
};

enum {
    BODY_INPUT_ADD = 1,    /* a body with `id` appears in the free slot, on (x, y), facing dir, at speed `value` */
    BODY_INPUT_REMOVE = 2, /* the body leaves; its slot is free again */
    BODY_INPUT_PLACE = 3,  /* the body is on (x, y) from `tick` on, facing dir, by `cause`; a walk ends.
                              Pushed, `value` is how long the push takes to show (the move's
                              land_ms); otherwise it is 0 */
    BODY_INPUT_WALK = 4,   /* the body walks to (x, y), over at most `value` steps (0: BODY_PATH_STEPS) */
    BODY_INPUT_STOP = 5,   /* a walk ends on the cell the body is on; the step under way is not taken */
    BODY_INPUT_DELAY = 6,  /* the body takes no step before `tick` + `value` ms */
    BODY_INPUT_SPEED = 7,  /* the body's speed is `value` from its next step on */
    BODY_INPUT_NEAR = 8,   /* the body walks until it is within `value` cells of (x, y) */
    BODY_INPUT_CHASE = 9,  /* the body walks after the body `target_id` in slot `target` until it is
                              within `value` cells of it, 1 at the least; it looks where that
                              body is each time it comes onto a cell */
    BODY_INPUT_FIGHTER = 10, /* how the body fights: attack delay `value`, 1 ms at the least, and attack
                                motion `x`, ms; attack range `y` cells; `target` ms from a
                                swing's start to its impact (0: it lands as it starts); `dir`
                                its kind (BODY_FIGHTER_*); `cause` its BODY_FIGHTS_* flags. A
                                body that was not told does not attack */
    BODY_INPUT_ATTACK = 11,  /* the body attacks the body `target_id` in slot `target`, once, or with
                                value 1 again and again (unit_attack, after the caller found the
                                target one that may be attacked) */
    BODY_INPUT_ATTACK_STOP = 12, /* the attack ends (unit_stop_attack); with value 1 a swing in its
                                    windup still lands. Value 2: the target may not be attacked
                                    (unit_unattackable): it is forgotten and the attack goes on no
                                    more, but a look that waits still comes and a swing still lands */
    BODY_INPUT_SWING_CANCEL = 13, /* a swing in its windup ends without a hit, and holds the body from
                                     walking no longer */
    BODY_INPUT_ACT_CLOCK = 14, /* the act clock (ud->canact_tick): no skill is used before `tick` +
                                  `target_id` ms (a skill's delay). It holds no swing (the fork's
                                  skill_delay_attack_enable). With value 1 the clock is only ever
                                  put later by this */
    BODY_INPUT_ANSWER = 15,  /* the answer to the body's open question: `value` is BODY_ANSWER_* bits */
    BODY_INPUT_ATTACK_CLOCK = 16, /* the next swing may begin at `tick` + `target_id` ms, sooner or later
                                     than the attack delay said (a combo's window). With value 1 an
                                     attack that waits looks then (unit_cancel_combo) */
    BODY_INPUT_ATTACK_ONCE = 17,  /* the attack goes on no more after its next swing; its target stays */
    BODY_INPUT_CAST = 18,         /* the body casts for `target_id` ms, and the act clock is at the cast's
                                     end or `value` ms from now, whichever is later (unit_skilluse_id2:
                                     the cast time, or the attack motion and the least delay between
                                     skills). When the cast is over the core asks (BODY_EVENT_ASK_CAST).
                                     With no cast time nothing is cast and only the clock is set.
                                     Refused while a cast is under way */
    BODY_INPUT_CAST_CANCEL = 19,  /* the cast under way ends without its skill, and the act clock is now
                                     (unit_skillcastcancel). Refused when none is under way */
    BODY_INPUT_INTENT = 20,       /* about the body's intent, which a request gave it: `dir` is
                                     BODY_INTENT_*, which says what the other fields are */
    BODY_INPUT_REQUEST = 21       /* what the body's owner asks for: `dir` is BODY_REQUEST_*, which says
                                     what the other fields are, and `value` the request's number, which
                                     the body keeps as the last it took (0: the request has none) */
};

/* How a fighter fights (BODY_INPUT_FIGHTER's `cause`), as bits. */
#define BODY_FIGHTS_HOLDS 1u     /* a swing holds its walk for the attack motion (attack_walk_delay) */
#define BODY_FIGHTS_FREE_CAST 2u /* it may swing while it casts (Free Cast) */

/* What kind of unit a fighter is (BODY_INPUT_FIGHTER's `dir`), as far as the fork's attack treats
 * them apart. A player is told when its target is beyond its reach (BODY_EVENT_FAR), when its
 * requested swing fixes its place and when it came too early. A monster whose target is beyond
 * its range goes after it by a way the game finds (BODY_EVENT_CHASE), and is asked before each
 * swing (BODY_EVENT_ASK_BEGIN); its swing lands on a target within its range and a cell more.
 * Another unit (a homunculus, a mercenary) is neither. */
enum { BODY_FIGHTER_OTHER = 0, BODY_FIGHTER_PLAYER = 1, BODY_FIGHTER_MONSTER = 2 };

/* What a BODY_INPUT_REQUEST asks for (its `dir`). An attack, a skill and a pickup each give the
 * body an intent with the request's number in the place of the one it had: what its owner wants,
 * which the core walks the body into reach for and has carried out (below). Each ends the attack
 * the body has, and a swing in its windup with it, but an attack on the body attacked already. */
enum {
    BODY_REQUEST_NONE = 0,    /* nothing but its number: a request the host refused, or one of a kind
                                 the core does not know */
    BODY_REQUEST_WALK = 1,    /* walk to (x, y): the intent goes without a word, the attack ends and
                                 a swing in its windup with it, and the body walks there as by
                                 BODY_INPUT_WALK, over at most as many steps as the world lets a
                                 request take (body_set_request_steps). Without a way the rest
                                 stays done, and BODY_EVENT_REFUSED says that no walk was taken */
    BODY_REQUEST_ATTACK = 2,  /* attack the body `target_id` in slot `target`, in mode `cause`
                                 (BODY_ATTACK_*) */
    BODY_REQUEST_RELEASE = 3, /* the button of a Hold attack came up: done if it swung, Once otherwise,
                                 and an attack that is Once now goes on no more after its next swing */
    BODY_REQUEST_CANCEL = 4,  /* the intent goes without a word and the attack ends, a swing in its
                                 windup with it */
    BODY_REQUEST_SKILL = 5,   /* a skill on the body `target_id` in slot `target`, which may be the body
                                 itself, within `cause` cells (skill_get_range2) */
    BODY_REQUEST_SKILL_CELL = 6, /* a skill on (x, y), within `cause` cells: a ground skill, or one on
                                    what is on that cell and is no body (a skill unit) */
    BODY_REQUEST_PICKUP = 7   /* the item on (x, y) is picked up, from within `cause` cells of it */
};

/* What a BODY_INPUT_INTENT does (its `dir`). */
enum {
    BODY_INTENT_CLEAR = 0, /* the intent goes without a word: a warp, a request the game takes itself */
    BODY_INTENT_ANSWER = 1 /* the answer to the intent's open question: `value` bits, `target_id` ms,
                              and `target` the intent's number, as the question gave it */
};

/* What an intent is for: an attack on a body, a skill on a body, a skill on a cell, a pickup.
 * Only an attack goes on after it was carried out. */
enum { BODY_WANTS_ATTACK = 1, BODY_WANTS_SKILL = 2, BODY_WANTS_SKILL_CELL = 3, BODY_WANTS_PICKUP = 4 };
/* Attack modes, as the mouse works: repeat, hold (while the button is down), once. */
enum { BODY_ATTACK_REPEAT = 0, BODY_ATTACK_HOLD = 1, BODY_ATTACK_ONCE = 2 };
/* Why an intent ended. */
enum {
    BODY_INTENT_DONE = 0, BODY_INTENT_TARGET_GONE = 1, BODY_INTENT_CANNOT_ACT = 2, BODY_INTENT_TOO_EARLY = 3,
    BODY_INTENT_NO_PATH = 4, BODY_INTENT_REFUSED = 5
};

#define BODY_INTENT_STEP_MS 100u /* how often a live intent's step runs by itself */
#define BODY_INTENT_WAIT_MS 300u /* an act the clock lets run within this is waited for; a later one ends the intent */
#define BODY_INTENT_REWALK_MS 300u /* the least time between two walks an intent asks for */
#define BODY_INTENT_WALKS 8u     /* walks asked for without getting into reach before it gives up */

/* An intent's step runs when the request is taken, on every cell the body comes onto, when its
 * attack finds the target out of reach, and every BODY_INTENT_STEP_MS while it lives. It asks what
 * the game sees (BODY_EVENT_ASK_INTENT), and then: a target that is gone, or a body that cannot
 * act, ends the intent; a body that sits is to stand up (BODY_EVENT_ASK_STAND); one that casts
 * waits. In reach: an attack that runs on the target waits; an act the clock lets run now is
 * carried out (BODY_EVENT_ASK_ACT); one it lets run within BODY_INTENT_WAIT_MS waits; a later one
 * ends the intent. A skill or a pickup is carried out from a cell the body stands on: a walk ends
 * there, and with a step under way the body comes onto that step's cell first, and looks again
 * from it. Out of reach: a walk the intent asked for that is under way, or a swing in its windup,
 * waits; otherwise the body walks into reach (BODY_EVENT_ASK_WALK), at most once in
 * BODY_INTENT_REWALK_MS and a cell closer each time, and after BODY_INTENT_WALKS walks that did not
 * get it there the intent ends.
 *
 * Reach is the core's: an attack's is the attack's (a player's as the client measures it, one more
 * against a walking target, the line of fire); a skill's is its range (check_distance) and,
 * beyond two cells, a line of fire no longer than the view (battle_check_range); a pickup's is its
 * distance (check_distance). The walk into reach goes after the target, or to the cell, to within
 * the attack's range or the skill's less a cell for every walk asked for before, one at the least;
 * a pickup's to within a cell of the item, and then onto it.
 *
 * The bits of an answer to BODY_EVENT_ASK_INTENT, what only the game knows; with them `target_id`
 * is for how many ms more the act's clock (and a skill's own cooldown) lets it not run: */
#define BODY_SEES_TARGET 1u       /* the target is there to be acted on */
#define BODY_SEES_CANNOT_ACT 2u   /* the body is dead, or kept from acting */
#define BODY_SEES_SITTING 4u
#define BODY_SEES_CASTING 8u
#define BODY_SEES_LINE 16u        /* no wall is in the line of fire to the target */
#define BODY_SEES_TARGET_WALKS 32u /* an attack's target walks, by a walk that is not this world's */
/* To BODY_EVENT_ASK_STAND, BODY_EVENT_ASK_ACT and BODY_EVENT_ASK_WALK: it stood up; the act was
 * carried out, or for an attack the game's leave to attack, with which the core begins the attack
 * as by BODY_INPUT_ATTACK (the intent is refused if that attack lost its target by the time its
 * first look is answered); the body may walk, and the core walks it into reach. */
#define BODY_INTENT_OK 1u
#define BODY_INTENT_STUCK 2u /* to BODY_EVENT_ASK_WALK, without OK: the body may not walk at all */

/* The bits of an answer. To BODY_EVENT_ASK_SWING: */
#define BODY_ANSWER_CAN_FIGHT 1u /* both live and the one may hit the other (battle_check_target & co) */
#define BODY_ANSWER_CASTING 2u   /* the body casts a cast the world does not keep, and may not swing
                                    meanwhile. The world's own cast holds a swing by itself, unless the
                                    body may swing while it casts (BODY_FIGHTS_FREE_CAST) */
#define BODY_ANSWER_LINE 4u      /* no wall is in the line of fire between them (path_search_long) */
#define BODY_ANSWER_READY 8u     /* a swing with a windup can start (it has its ammunition) */
#define BODY_ANSWER_WALKS 32u    /* the target walks, by a walk that is not this world's: it is reached a
                                    cell further, as one whose walk is */
/* To BODY_EVENT_ASK_HIT: */
#define BODY_ANSWER_HIT 16u      /* a hit was made (the target lived and the attack was not ATK_NONE) */
/* To BODY_EVENT_ASK_BEGIN: */
#define BODY_ANSWER_BEGIN 128u   /* the swing begins. Without it the game did something in its place (a
                                    monster's skill): no swing begins and the attack looks no more */
/* To BODY_EVENT_ASK_CAST, with which `target_id` is the ms the act clock is put at from now: */
#define BODY_ANSWER_ACTED 64u    /* the skill was carried out: the clock is only put later by it (its delay).
                                    Without: it was not, and the clock is put there, sooner or later */

/* Why a swing ended in its windup. */
enum { BODY_CANCEL_COMMAND = 0, BODY_CANCEL_LOST = 1 };

/* One input. All but an add name the body by its slot and its id, so a slot the caller
 * remembers wrongly is refused and not another body moved; a chase names its target the same
 * way. Fields a kind does not use are not read. */
struct BodyInput {
    uint64_t tick;
    uint32_t id;
    uint16_t kind, slot;
    int16_t x, y;
    uint16_t value;
    uint8_t dir, cause;
    uint32_t target_id;
    uint16_t target;
    uint8_t reserved[2];
};

enum {
    BODY_EVENT_MOVE = 1,   /* the body's move state changed otherwise than its record said: send the record again */
    BODY_EVENT_CELL = 2,   /* the body entered (x, y) by a step in direction dir */
    BODY_EVENT_NO_PATH = 3, /* what the body was to walk to cannot be reached from (x, y), or the body
                               it was after is gone; it stands */
    BODY_EVENT_ASK_SWING = 4, /* a question: may the body swing at `other` now? Answer with CAN_FIGHT,
                                 CASTING, LINE and READY */
    BODY_EVENT_ASK_HIT = 5,   /* a question: the body's swing lands on `other` now. Make the hit, and
                                 answer HIT when one was made. `value` 1: it is the impact of a
                                 swing that began earlier (BODY_EVENT_SWING) */
    BODY_EVENT_SWING = 6,     /* the body began a swing at `other`, which lands `value` ms on */
    BODY_EVENT_SWING_CANCEL = 7, /* its swing ended in its windup; `value` is BODY_CANCEL_* */
    BODY_EVENT_FAR = 8,       /* a player's attack is out of its reach of `other` (clif_movetoattack) */
    BODY_EVENT_FIX = 9,       /* its place is to be told again (clif_fixpos): a player's requested swing,
                                 not in the middle of a step, whose place is its record's; and a walk
                                 after the target that the attack began ends within its range */
    BODY_EVENT_FACE = 10,     /* the body turned to `dir`, towards the one it attacks */
    BODY_EVENT_ATTACKING = 11, /* the body's attack looked at its target `other` (none when it is gone) and
                                  swung or had nothing to wait for; `value` 1: and goes on, and the
                                  game turns its unit to the target as it sees the two */
    BODY_EVENT_UNREADY = 12,  /* a swing at `other` with a windup did not begin: the answer was not READY */
    BODY_EVENT_ASK_CAST = 13, /* a question: the body's cast is over now. Carry its skill out, and answer
                                 ACTED when it was, with what it puts under the act clock */
    /* About the body's intent; `other` is the intent's number, and no body. The four questions
       are answered with BODY_INTENT_ANSWER. */
    BODY_EVENT_ASK_INTENT = 14, /* a question: what does the game see of the intent now (BODY_SEES_*)? */
    BODY_EVENT_ASK_STAND = 15,  /* a question: the body sits. Stand it up, and answer OK when it stands */
    BODY_EVENT_ASK_ACT = 16,    /* a question: carry the intent out now (an attack: may the body attack
                                   its target now?), and answer OK when it was not refused */
    BODY_EVENT_ASK_WALK = 17,   /* a question: may the body walk into reach now, closer by `value` (the
                                   walks asked for so far)? Answer OK when it may */
    BODY_EVENT_INTENT_END = 18, /* the intent ended: `value` is the reason; `other_id` the ms until its act
                                   may run, when it came too early */
    BODY_EVENT_REFUSED = 19,    /* the walk of the request `value` was not taken: there is no way to its
                                   cell from (x, y) */
    BODY_EVENT_ASK_BEGIN = 20,  /* a question: a monster's swing at `other` is to begin now. Do what the game
                                   does then (a skill in its place, the link with others), and
                                   answer BEGIN when the swing begins */
    BODY_EVENT_CHASE = 21,      /* a monster's attack is to go after `other` until it is within `value`
                                   cells of it (unit_walktobl with the attack going on); the game
                                   finds the way and walks it. The attack looks no more */
    BODY_EVENT_UNATTACKABLE = 22 /* a look of the attack's timer found no target it may fight and forgot it
                                   (unit_unattackable) */
};

/* (x, y) and dir are the body's when the event was made. `other` and `other_id` name a second
 * body where the kind says so, an intent's number and a time where it says that, and are zero
 * otherwise, as `value` is. */
struct BodyEvent {
    uint64_t tick;
    uint32_t id;
    uint16_t kind, slot;
    int16_t x, y;
    uint8_t dir;
    uint8_t reserved; /* zero */
    uint16_t value;
    uint32_t other_id;
    uint16_t other;
    uint8_t reserved2[2]; /* zero */
};

/* The caller's ground for one call: a byte per cell of the world's map, row y at
 * walkable + y * width, 0 for a cell that cannot be walked; and BODY_SCRATCH_SIZE bytes of
 * working memory aligned to 4, which hold nothing between calls. */
struct BodyGround {
    const uint8_t *walkable;
    void *scratch;
};

struct BodyWorld;

uint32_t body_rules_version(void);

/* The bytes a world of `bodies` slots and a ring of `events` events needs, or 0 when a count
 * is out of its range. */
size_t body_world_size(uint32_t bodies, uint32_t events);

/* Makes an empty world in `arena` for a map of width by height cells, its clock at `now`.
 * Returns NULL when the arena is missing, not aligned to 8 or smaller than body_world_size(),
 * or when a count or a side is out of range. */
struct BodyWorld *body_world_init(void *arena, size_t size, uint32_t bodies, uint32_t events,
                                  int32_t width, int32_t height, uint64_t now);

/* Whether `size` bytes at `arena` are a world every other function may be given: BODY_OK or
 * BODY_BAD_WORLD. For bytes from outside, a snapshot read back from a file above all. It reads
 * nothing beyond `size`. */
int body_world_check(const void *arena, size_t size);

uint64_t body_now(const struct BodyWorld *world);

/* How long after a swing that was cancelled in its windup the next may begin, in ms; 0 in a
 * new world. */
void body_set_cancel_penalty(struct BodyWorld *world, uint32_t ms);

/* The longest way the walk of a request may take, in steps (the server's max_walk_path): 1 to
 * BODY_PATH_STEPS, which it is in a new world. A walk the game asks for by BODY_INPUT_WALK says
 * its own. */
void body_set_request_steps(struct BodyWorld *world, uint32_t steps);

/* The number of questions that wait for their answer. */
uint32_t body_asked(const struct BodyWorld *world);

/* Applies one input and moves the clock to its tick. Returns BODY_OK, or the reason the input
 * was refused; a refused input changes nothing. */
int body_input(struct BodyWorld *world, const struct BodyGround *ground, const struct BodyInput *input);

/* The tick the next thing is due at. Returns 0 when nothing is. */
int body_next_due(const struct BodyWorld *world, uint64_t *tick);

/* Moves the clock to `tick` and runs what is due then, bodies in slot order and for each its
 * step, then its cast's end, its swing's impact, its attack and its intent's step. BODY_BAD_TICK when the tick is before the
 * clock, BODY_DUE_FIRST when something is due before it. On BODY_EVENTS_FULL the clock is at
 * `tick` and part of the work is done: take events and call again with the same tick. On
 * BODY_ANSWER_FIRST a question is open: with a later tick nothing was done; with the clock's
 * own tick what could run has run. Take the events, answer, and call again. */
int body_run(struct BodyWorld *world, const struct BodyGround *ground, uint64_t tick);

/* Takes the oldest waiting event. Returns 0 when none waits. Events come out in the order
 * they were made, which is the order of their ticks. */
int body_take_event(struct BodyWorld *world, struct BodyEvent *event);

/* The move state of the body in `slot`: BODY_OK, or BODY_BAD_SLOT for a slot that does not
 * exist or is free. */
int body_move(const struct BodyWorld *world, uint32_t slot, struct BodyMove *move);

/* A body's attack and its act clock, for a host to read. */
struct BodyAttack {
    uint64_t attackable;      /* no swing begins before this tick */
    uint64_t can_act;         /* the act clock: no skill is used before this tick */
    uint32_t target_id;       /* whom it attacks, with the slot `target`, while `has_target` */
    uint32_t swing_target_id; /* whom its swing in its windup is at, with `swing_target`, while `swing_open` */
    uint16_t target, swing_target;
    uint8_t has_target, attack_continue;
    uint8_t armed;      /* a look at the target waits for its tick */
    uint8_t swing_open; /* a swing is in its windup */
    uint8_t casting;    /* a cast is under way */
    uint8_t looking;    /* a look at the target waits for the game's answer */
    uint16_t amotion;   /* its attack motion, ms, as it was last told how it fights */
    uint16_t windup;    /* ms from a swing's start to its impact */
    uint16_t swing_ms;  /* ms from the world's clock until the swing in its windup lands, while `swing_open` */
};

/* The attack of the body in `slot`: BODY_OK, or BODY_BAD_SLOT for a slot that does not exist or
 * is free. */
int body_attack(const struct BodyWorld *world, uint32_t slot, struct BodyAttack *attack);

/* A body's intent, for a host to read. */
struct BodyIntentState {
    uint16_t seq;       /* the request's number */
    uint8_t has;        /* it has one; the rest is zero when not */
    uint8_t kind, mode; /* the mode as it is now: a Hold released before it swung is Once */
    uint8_t walked;     /* a walk was asked for */
    uint8_t reserved[2]; /* zero */
    uint32_t target_id; /* whom it is about, with the slot `target`, for an attack or a skill on a body */
    uint16_t target;
    uint16_t request;   /* the number of the last request the body took, with an intent or without */
};

/* The intent of the body in `slot`: BODY_OK, or BODY_BAD_SLOT for a slot that does not exist or
 * is free. */
int body_intent(const struct BodyWorld *world, uint32_t slot, struct BodyIntentState *intent);

/* A body as bytes another world can take: all the core keeps of it, with the bodies it names
 * (the one it walks after, attacks or swings at, the one its intent is about) by their ids
 * alone, as a slot is one world's business, and without a question it waits on, whose event
 * went to its own world's host. Two bodies that give the same bytes are the same body, but
 * for such a question: a host that tells a body on takes its bytes when none is open. */
#define BODY_STATE_SIZE 224u

/* Writes the BODY_STATE_SIZE bytes of the body in `slot` to `state`, which need not be aligned:
 * BODY_OK, or BODY_BAD_SLOT for a slot that does not exist or is free. */
int body_state(const struct BodyWorld *world, uint32_t slot, void *state);

/* Makes the body in `slot`, free or not, what `state` says, a state at the world's clock: for a
 * host that is told a body and does not make it, a client with its own character as the server
 * has it. The bodies it names are found in this world by their ids; one that is not here is
 * gone for it, as one that left. The bytes are data from outside: BODY_BAD_SLOT for a slot that
 * does not exist, BODY_BAD_VALUE for bytes that are no body this world's rules could have left
 * at its clock (a step or a look that is overdue, a record from after it, a cell off the map,
 * an unknown direction), that name the body itself, or that have a question open, whose event
 * went to another host; BODY_EVENTS_FULL. Then nothing is changed. The clock stays. */
int body_put_state(struct BodyWorld *world, uint32_t slot, const void *state);

/* Makes the body in `slot` what a record says: for a host that is told records and does not
 * make them, a client with a body it shows. The record has to be a state at the world's clock:
 * not later than it, on the map with its walk's end, its first step not yet due before the
 * clock, and well-formed (steps, directions, cause, speed, zero where nothing is). Returns
 * BODY_OK, or BODY_BAD_SLOT, BODY_BAD_TICK, BODY_BAD_CELL, BODY_BAD_VALUE or BODY_EVENTS_FULL,
 * and then changes nothing. The clock stays. A record played to the clock (body_move_at) is
 * such a state. */
int body_put_move(struct BodyWorld *world, uint32_t slot, uint32_t id, const struct BodyMove *record);

/* What a record says at `tick`: the record itself before its first step lands, otherwise the
 * record the body would give then if nothing else happened to it. This is how a record is
 * played, and a world's body is always what its last record says. A record from outside is
 * safe to give: more steps than a path has count as BODY_PATH_STEPS, and a direction is read
 * by its low three bits. */
void body_move_at(const struct BodyMove *record, uint64_t tick, struct BodyMove *at);

#endif
