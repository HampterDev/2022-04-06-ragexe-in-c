#ifndef WORLD_PREDICTED_H
#define WORLD_PREDICTED_H

#include "body/body.h"

/* The predicted world (plans/predicted-world.md): the player's own character shown ahead of
 * the server, by the server's own rules. It is a world of the body core (body/body.h), the
 * code the server runs, with the own body in it and the bodies that one depends on (the one it
 * attacks). Freestanding: no allocation, no globals, no clock; the caller hands in the memory
 * and the ticks. Ticks are the server's, whole (64 bits), as the core's are.
 *
 * It holds three things:
 *   - what the server last said the own body is (ZC_OWN_BODY: the core's state of it, the
 *     tick it is of, and the number of the last request of the player's that it includes);
 *   - the requests the player made that the server has not confirmed, in order, each with its
 *     number and the tick the server applies it at;
 *   - what the client knows of the other bodies: their move records, and when one went.
 * From these the world is made: the confirmed body at its tick, the others where their records
 * have them, the requests put in at their ticks, every question of the core answered with yes,
 * and all of it run to the own clock. The same is done again whenever the past changed: the
 * server's word was not what had been predicted for its tick, or a record of another body
 * came for a tick the world is already past. Otherwise the world only runs on.
 *
 * A word of the server that is what was predicted for its tick changes nothing that is seen:
 * the requests it confirms leave the list. Everything else is one correction: a refusal, a hit
 * that holds the walk, a target that died, a request the server applied at another tick.
 *
 * What the client cannot know makes the prediction wrong until the server's next word: an
 * answer of the game's that was not yes, another body that is not where its record has it, a
 * skill's range the server has otherwise than the client's skill list.
 *
 * A word is of one placement of the player on a map: the server counts them, the word carries
 * the number, and the map's ZC_HOLD_BACK names the number of the map the player is on. A word
 * that comes before that is kept aside, the newest of them, and is taken once the number is
 * known and is its own; a word of another placement is refused. So a word of a map the player
 * left does not pass for one of this map, in whatever order the two arrive.
 *
 * The bytes of a ZC_OWN_BODY and the records are data from outside: what does not fit is
 * refused and changes nothing. */

/* The other bodies the world holds beside the own one. */
#define PREDICTED_OTHERS 3
/* The requests that can wait for the server's word. */
#define PREDICTED_PENDING 16
/* The records of another body that can wait for their tick. */
#define PREDICTED_RECORDS 4
/* The own body's states by tick that are kept, to compare the server's word with. */
#define PREDICTED_PAST 48
/* The bytes of the world's arena: the core's header, the bodies and a ring of sixteen events. */
#define PREDICTED_ARENA 1464u

/* ZC_OWN_BODY, 0x0F25, little-endian and packed, 32 bytes and the state:
 *
 *   off size field
 *     0    2 id            0x0F25
 *     2    2 length        of the packet
 *     4    4 version       the rules version of the server's core (BODY_RULES_VERSION)
 *     8    8 tick          the server's clock the state is of, whole
 *    16    2 request       the number of the last request of the player's the state includes
 *    18    2 request_steps the longest way a request's walk may take (body_set_request_steps)
 *    20    4 penalty_ms    how long after a cancelled swing the next may begin
 *    24    4 placement     the number of the player's placement on a map the body is of
 *    28    4 arrival       the low 32 bits of the server's clock when the request the body names
 *                          arrived; 0 when the body names none anew
 *    32    n state         the body (body_state): BODY_STATE_SIZE bytes of this version
 */
#define PREDICTED_PACKET_HEAD 32u

struct PredictedBody {
    uint64_t tick;
    uint32_t version;
    uint32_t penalty_ms;
    uint32_t placement;
    uint32_t arrival;
    uint16_t request;
    uint16_t request_steps;
    unsigned char state[BODY_STATE_SIZE]; /* zero when the version is not this core's */
};

/* Reads a ZC_OWN_BODY of `size` bytes. Returns 0 and writes nothing when the packet is shorter
 * than its head, says another length than it has, or is of this core's version without being
 * as long as that version's state makes it. A packet of another version is read, without its
 * state: its version says that nothing can be predicted. */
int predicted_read(const unsigned char *packet, size_t size, struct PredictedBody *body);

/* A request of the player's. */
struct PredictedRequest {
    uint64_t apply_tick; /* the server's tick it is applied at: the one its stamp named */
    uint32_t target_id;  /* an attack, a skill on a body: whom (the own body's id for a skill on itself) */
    uint32_t left;       /* the caller's clock when it left for the server, handed back when a word answers it */
    uint16_t number;     /* its number: the stamp's */
    uint8_t kind;        /* BODY_REQUEST_WALK to _PICKUP, but _NONE */
    uint8_t mode;        /* an attack: BODY_ATTACK_* */
    int16_t x, y;        /* a walk: where to; a skill on a cell, a pickup: the cell */
    uint8_t range;       /* a skill's range, a pickup's distance, in cells */
    uint8_t reserved[3];
};

struct PredictedOther {
    uint32_t id;
    uint8_t used;
    uint8_t count;                             /* records held */
    uint8_t out;                               /* gone, and out of the world as it runs */
    uint8_t slot;                              /* its slot in the core: every stay of an id has the same */
    uint64_t from;                             /* the tick it came into the world at; 0 for a body known without a coming */
    uint64_t gone;                             /* the tick it left the world at; 0 while it is in it */
    struct BodyMove record[PREDICTED_RECORDS]; /* by at_tick, earliest first */
};

struct PredictedPast {
    uint64_t tick; /* the state holds from this tick on */
    unsigned char state[BODY_STATE_SIZE];
};

/* The whole of a prediction. Its memory is the caller's, aligned to 8 as the core's arena has
 * to be: static, or allocated, not a local of a 32-bit build. */
struct Predicted {
    uint64_t world[(PREDICTED_ARENA + 7u) / 8u]; /* the core's arena, run to the own clock */
    struct PredictedBody confirmed;              /* the server's last word */
    struct PredictedBody waiting;                /* the newest word that came before its placement was known */
    struct PredictedRequest pending[PREDICTED_PENDING];
    struct PredictedOther other[PREDICTED_OTHERS];
    struct PredictedPast past[PREDICTED_PAST];   /* a ring: the own body's states, oldest first */
    uint64_t records_tick; /* the others' records up to this tick are in the world */
    uint32_t past_first, past_count;
    uint32_t pending_count;
    uint32_t applied;     /* how many of the pending requests the world has taken */
    uint32_t same;        /* the server's words that were what had been predicted */
    uint32_t different;   /* and those that were not: corrections */
    uint32_t unexplained; /* of those, the ones without a request or a record to explain them */
    uint64_t unexplained_tick; /* the tick of the last of them */
    uint64_t suspect_tick;     /* while `suspect`: the tick of a word that may be one of them */
    uint32_t own_id;      /* the own body's id, as the server's word has it */
    uint32_t placement;   /* the number of the placement the player is in (while `placed`) */
    uint32_t way_left;    /* while `way_new`: the clock the request a word answered left at, */
    uint32_t way_arrival; /* and the server's clock when it arrived there */
    int32_t width, height;
    uint8_t live;         /* the server said what the body is, in this core's version */
    uint8_t stale;        /* the past changed: the world is made again before it runs on */
    uint8_t placed;       /* the number of this placement is known */
    uint8_t has_waiting;  /* `waiting` holds a word */
    uint8_t way_new;      /* a word answered a request with its arrival: how long the way up took */
    uint8_t heard;        /* a record came since the last word */
    uint8_t suspect;      /* the last word differed, and nothing explained it yet */
    uint8_t reserved[1];
};

enum {
    PREDICTED_SAME = 0,      /* the word is what was predicted for its tick */
    PREDICTED_DIFFERENT = 1, /* it is not, or nothing was predicted for that tick: the world is made again */
    PREDICTED_REFUSED = 2,   /* bytes that are no body, a word older than the last or of another placement: nothing changed */
    PREDICTED_VERSION = 3    /* another rules version: nothing is predicted until a word of this one */
};

/* An empty prediction for a map of width by height cells. */
void predicted_init(struct Predicted *predicted, int32_t width, int32_t height);

/* The server's word of the own body. `ground` is the map as the core reads it. When it names a
 * request that waits and the arrival of that request, the request's `left` and the arrival are
 * kept in `way_left` and `way_arrival` and `way_new` is set, for the caller to take: a sample of
 * the way up (std/way_up.h). Before the
 * number of the placement is known the word is kept aside, if it is the newest so far, and
 * PREDICTED_REFUSED is returned. */
int predicted_confirm(struct Predicted *predicted, const struct BodyGround *ground, const struct PredictedBody *body);

/* A record of the world came, of the server's tick `tick`: a unit's move, act or vitals, a hit,
 * a coming or a going. A word that differs from what was predicted for its tick counts as
 * `unexplained` when it answers no request, no waiting request was due by its tick, no record
 * came since the last word, and none of its tick or before comes until the next word (which
 * travels first and may overtake it): such a word is a fault in a rule or in the prediction.
 * It is counted when the next word comes, its tick in `unexplained_tick`. */
void predicted_heard(struct Predicted *predicted, uint64_t tick);

/* The number of the player's placement on this map (ZC_HOLD_BACK). A word kept aside of that
 * placement is taken now, and what predicted_confirm says of it is returned; otherwise
 * PREDICTED_REFUSED. The number said again changes nothing. */
int predicted_placed(struct Predicted *predicted, const struct BodyGround *ground, uint32_t placement);

/* A request of the player's that leaves for the server. Returns 0 when it is none the core
 * knows. When more wait than the list holds, the oldest goes. */
int predicted_request(struct Predicted *predicted, const struct PredictedRequest *request);

/* The move record of another body, complete from its tick on as the server's records are: it
 * takes the place of every record of that body with the same or a later tick. One that has
 * the body where its last record has it, on the same steps at the same ticks, says nothing
 * new and changes nothing, so a caller may say what it sees of a body every frame. Returns 0
 * when the world holds PREDICTED_OTHERS others already and none of them can go: the own body
 * names each. A record is of the stay of the body its tick is in; one dated at or after the
 * body's last going is of the body that came back, which is in the world from its earliest
 * record on. */
int predicted_other(struct Predicted *predicted, uint32_t id, const struct BodyMove *record);

/* Another body is gone from the tick `tick` of the server on (ZC_VIEW_STAMP, plans/presence.md):
 * it is in the worlds made of words before that tick and in none of later ones, whatever arrived
 * first. What is due at that tick runs before it goes, so a swing that lands then lands. A going
 * the world is already past is a change of the past: the world is made again. Its records of
 * that tick or later are of a later stay, which comes into the world at the earliest of them.
 * The same going told again changes nothing. */
void predicted_gone(struct Predicted *predicted, uint32_t id, uint64_t tick);

/* Runs the world to `tick`, the own clock, making it again first when the past changed. A tick
 * behind the world's clock changes nothing. Returns 1 while the own body is predicted. */
int predicted_run(struct Predicted *predicted, const struct BodyGround *ground, uint64_t tick);

/* The world's clock; 0 while nothing is predicted. */
uint64_t predicted_now(const struct Predicted *predicted);

/* A tick of the wire (32 bits, which wrap) as the whole tick nearest the world's clock. */
uint64_t predicted_wide(const struct Predicted *predicted, uint32_t tick);

/* The own body as the world has it now: its move record, its attack, its intent, and its bytes
 * (BODY_STATE_SIZE). Each returns 0 while nothing is predicted. */
int predicted_move(const struct Predicted *predicted, struct BodyMove *move);
int predicted_attack(const struct Predicted *predicted, struct BodyAttack *attack);
int predicted_intent(const struct Predicted *predicted, struct BodyIntentState *intent);
int predicted_state(const struct Predicted *predicted, void *state);

/* The swing the own body has in its windup, for someone who shows it: when it began and when
 * it lands, at whom it is, and the body's attack motion. */
struct PredictedSwing {
    uint64_t tick, lands;
    uint32_t target_id;
    uint16_t motion_ms;
    uint8_t reserved[2];
};

/* Returns 0 while nothing is predicted or the body has no swing in its windup. */
int predicted_swing(const struct Predicted *predicted, struct PredictedSwing *swing);

/* What a move of the own body says to someone who acted on an earlier one, `was`:
 *   PREDICTED_WALK_ON    nothing: `now` is `was` played on to its tick, the body took its steps
 *   PREDICTED_WALK_NEW   another walk, or a stand elsewhere, made at `was`'s tick or after it
 *   PREDICTED_WALK_BACK  another one made before `was`: the past changed, and what `was` said
 *                        was not so (the server's word took a predicted walk back) */
enum { PREDICTED_WALK_ON = 0, PREDICTED_WALK_NEW = 1, PREDICTED_WALK_BACK = 2 };
int predicted_walk_news(const struct BodyMove *was, const struct BodyMove *now);

#endif
