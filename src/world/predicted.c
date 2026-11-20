#include "world/predicted.h"

/* The own body's slot is the last: what is due at one tick runs in slot order, so another body
 * has come onto its cell before the own body looks at it, as on the server, which puts the
 * others where they are before it runs its world. */
enum { OWN = PREDICTED_OTHERS, BODIES = PREDICTED_OTHERS + 1, EVENTS = 16 };
/* The calls into the core one run takes at the most. A world that is further behind than that
 * goes on from where it is at the next run. */
#define ROUNDS 256

static void copy(void *to, const void *from, size_t size)
{
    unsigned char *out = to;
    const unsigned char *in = from;
    size_t i;

    for (i = 0; i < size; i++)
        out[i] = in[i];
}

static void zero(void *memory, size_t size)
{
    unsigned char *bytes = memory;
    size_t i;

    for (i = 0; i < size; i++)
        bytes[i] = 0;
}

static int same(const void *a, const void *b, size_t size)
{
    const unsigned char *x = a, *y = b;
    size_t i;

    for (i = 0; i < size; i++)
        if (x[i] != y[i])
            return 0;
    return 1;
}

static uint32_t get16(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8;
}

static uint32_t get32(const unsigned char *p)
{
    return get16(p) | get16(p + 2) << 16;
}

int predicted_read(const unsigned char *packet, size_t size, struct PredictedBody *body)
{
    uint32_t version;

    if (size < PREDICTED_PACKET_HEAD || get16(packet + 2) != size)
        return 0;
    version = get32(packet + 4);
    if (version == BODY_RULES_VERSION && size != PREDICTED_PACKET_HEAD + BODY_STATE_SIZE)
        return 0;
    zero(body, sizeof *body);
    body->version = version;
    body->tick = (uint64_t)get32(packet + 8) | (uint64_t)get32(packet + 12) << 32;
    body->request = (uint16_t)get16(packet + 16);
    body->request_steps = (uint16_t)get16(packet + 18);
    body->penalty_ms = get32(packet + 20);
    body->placement = get32(packet + 24);
    body->arrival = get32(packet + 28);
    body->outside = get32(packet + 32);
    if (version == BODY_RULES_VERSION)
        copy(body->state, packet + PREDICTED_PACKET_HEAD, BODY_STATE_SIZE);
    return 1;
}

static struct BodyWorld *world_of(struct Predicted *predicted)
{
    return (struct BodyWorld *)predicted->world;
}

static const struct BodyWorld *world_at(const struct Predicted *predicted)
{
    return (const struct BodyWorld *)predicted->world;
}

void predicted_init(struct Predicted *predicted, int32_t width, int32_t height)
{
    zero(predicted, sizeof *predicted);
    predicted->width = width;
    predicted->height = height;
}

/* `other` becomes a stay of the body `id` that holds nothing yet, in the core's slot of the
   body's other stays, or in one no other body has. */
static void open_stay(struct Predicted *predicted, struct PredictedOther *other, uint32_t id, uint64_t from)
{
    uint32_t i, slot, taken;

    zero(other, sizeof *other);
    for (i = 0; i < PREDICTED_OTHERS; i++)
        if (predicted->other[i].used && predicted->other[i].id == id) {
            other->slot = predicted->other[i].slot;
            break;
        }
    if (i == PREDICTED_OTHERS)
        for (slot = 0; slot < PREDICTED_OTHERS; slot++) {
            for (i = 0, taken = 0; i < PREDICTED_OTHERS; i++)
                taken |= predicted->other[i].used && predicted->other[i].slot == slot;
            if (!taken) {
                other->slot = (uint8_t)slot;
                break;
            }
        }
    other->used = 1;
    other->id = id;
    other->from = from;
}

/* Whether the stay `other` has the body in the world at `tick`: from its coming, before its going. */
static int holds(const struct PredictedOther *other, uint64_t tick)
{
    return other->from <= tick && (!other->gone || tick < other->gone);
}

/* The record of another body that holds at `tick`: the latest whose tick has come, or the
 * earliest it has when none has. */
static const struct BodyMove *record_at(const struct PredictedOther *other, uint64_t tick)
{
    int i, found = 0;

    for (i = 0; i < other->count; i++)
        if (other->record[i].at_tick <= tick)
            found = i;
    return &other->record[found];
}

/* The other body in `slot` is what `record` says at the world's clock: there from now on, on
 * the walk the record has it on. A body the world does not have yet is added, which only a
 * world that has nothing due takes. What the core refuses of it leaves the body out, or as it
 * was. */
static void put_other(struct BodyWorld *world, const struct BodyGround *ground, uint32_t slot, uint32_t id,
                      const struct BodyMove *record)
{
    uint64_t now = body_now(world), lands;
    struct BodyMove at, was;
    struct BodyInput in;
    struct BodyEvent event;

    body_move_at(record, now, &at);
    if (at.at_tick > now) {
        /* a record that is still to come: the body is on its cell until then */
        lands = at.at_tick + at.land_ms - now;
        at.land_ms = (uint16_t)(at.steps == 0 ? 0 : lands > 0xFFFFu ? 0xFFFFu : lands);
        at.at_tick = now;
    }
    if (body_move(world, slot, &was) != BODY_OK) {
        zero(&in, sizeof in);
        in.tick = now;
        in.kind = BODY_INPUT_ADD;
        in.slot = (uint16_t)slot;
        in.id = id;
        in.x = at.x;
        in.y = at.y;
        in.value = at.speed_ms;
        in.dir = at.dir;
        if (body_input(world, ground, &in) != BODY_OK)
            return;
    }
    body_put_move(world, slot, id, &at);
    while (body_take_event(world, &event)) {
    }
}

/* The other body in `slot` leaves the world at its clock. */
static void take_out(struct BodyWorld *world, const struct BodyGround *ground, uint32_t slot, uint32_t id)
{
    struct BodyMove was;
    struct BodyInput in;

    if (body_move(world, slot, &was) != BODY_OK)
        return;
    zero(&in, sizeof in);
    in.tick = body_now(world);
    in.kind = BODY_INPUT_REMOVE;
    in.slot = (uint16_t)slot;
    in.id = id;
    body_input(world, ground, &in);
}

/* A world in `arena` of what the server said: at its tick, with the others where their records
 * have them then (a body that went at that tick or before is none) and the own body as the
 * server's bytes say. Returns NULL, with the arena of no use, when the bytes are no body the
 * core takes there; otherwise writes the body's id. */
static struct BodyWorld *make(const struct Predicted *predicted, void *arena, const struct BodyGround *ground,
                              const struct PredictedBody *body, uint32_t *id)
{
    struct BodyWorld *world;
    struct BodyEvent event;
    uint32_t i;

    if (body_world_size(BODIES, EVENTS) > PREDICTED_ARENA)
        return NULL;
    world = body_world_init(arena, PREDICTED_ARENA, BODIES, EVENTS, predicted->width, predicted->height, body->tick);
    if (world == NULL)
        return NULL;
    body_set_cancel_penalty(world, body->penalty_ms);
    body_set_request_steps(world, body->request_steps);
    for (i = 0; i < PREDICTED_OTHERS; i++)
        if (predicted->other[i].used && holds(&predicted->other[i], body->tick))
            put_other(world, ground, predicted->other[i].slot, predicted->other[i].id, record_at(&predicted->other[i], body->tick));
    if (body_put_state(world, OWN, body->state) != BODY_OK || !body_take_event(world, &event))
        return NULL;
    *id = event.id;
    return world;
}

static struct PredictedPast *past_entry(struct Predicted *predicted, uint32_t index)
{
    return &predicted->past[(predicted->past_first + index) % PREDICTED_PAST];
}

/* The own body's state at the world's clock joins what is kept of its past, when it is another
 * than the last kept. One state a tick: the last the tick left. */
static void note(struct Predicted *predicted)
{
    const struct BodyWorld *world = world_at(predicted);
    unsigned char state[BODY_STATE_SIZE];
    struct PredictedPast *last = predicted->past_count ? past_entry(predicted, predicted->past_count - 1) : NULL;

    if (body_state(world, OWN, state) != BODY_OK)
        return;
    if (last != NULL && same(last->state, state, sizeof state))
        return;
    if (last == NULL || last->tick != body_now(world)) {
        if (predicted->past_count == PREDICTED_PAST) {
            predicted->past_first = (predicted->past_first + 1) % PREDICTED_PAST;
            predicted->past_count--;
        }
        last = past_entry(predicted, predicted->past_count++);
        last->tick = body_now(world);
    }
    copy(last->state, state, sizeof state);
}

/* What was predicted for `tick`: the state that held then, or NULL when the world has not come
 * to that tick or nothing is kept from so long ago. */
static const struct PredictedPast *past_at(struct Predicted *predicted, uint64_t tick)
{
    const struct PredictedPast *found = NULL;
    uint32_t i;

    if (tick > body_now(world_at(predicted)))
        return NULL;
    for (i = 0; i < predicted->past_count; i++)
        if (past_entry(predicted, i)->tick <= tick)
            found = past_entry(predicted, i);
    return found;
}

/* Whether the own body names the body `id`: it attacks it, swings at it, or its intent is about it. */
static int names_in(const struct BodyWorld *world, uint32_t id)
{
    struct BodyAttack attack;
    struct BodyIntentState intent;

    if (body_attack(world, OWN, &attack) == BODY_OK
        && ((attack.has_target && attack.target_id == id) || (attack.swing_open && attack.swing_target_id == id)))
        return 1;
    return body_intent(world, OWN, &intent) == BODY_OK && intent.has
           && (intent.kind == BODY_WANTS_ATTACK || intent.kind == BODY_WANTS_SKILL) && intent.target_id == id;
}

static int names(const struct Predicted *predicted, uint32_t id)
{
    return names_in(world_at(predicted), id);
}

/* The bodies the own body names in a world: its attack's, its swing's and its intent's targets. */
static void named_in(const struct BodyWorld *world, uint32_t ids[3])
{
    struct BodyAttack attack;
    struct BodyIntentState intent;

    ids[0] = ids[1] = ids[2] = 0;
    if (body_attack(world, OWN, &attack) == BODY_OK) {
        ids[0] = attack.has_target ? attack.target_id : 0;
        ids[1] = attack.swing_open ? attack.swing_target_id : 0;
    }
    if (body_intent(world, OWN, &intent) == BODY_OK && intent.has
        && (intent.kind == BODY_WANTS_ATTACK || intent.kind == BODY_WANTS_SKILL))
        ids[2] = intent.target_id;
}

int predicted_confirm(struct Predicted *predicted, const struct BodyGround *ground, const struct PredictedBody *body)
{
    /* the arena of a trial world. A 32-bit build's stack is aligned to 4 only, so the world's 8
       are found inside the block */
    unsigned char block[PREDICTED_ARENA + 8u];
    void *arena = block + (8u - ((uintptr_t)block & 7u)) % 8u;
    const struct PredictedPast *past;
    uint32_t id, kept = 0, i, word_names[3];
    const struct BodyWorld *made;
    int was, unexplained;

    /* a word of a placement not known yet waits, the newest of them; one of another is
       refused */
    if (!predicted->placed || body->placement != predicted->placement) {
        if (!predicted->placed
            && (!predicted->has_waiting || (int32_t)(body->placement - predicted->waiting.placement) > 0
                || (body->placement == predicted->waiting.placement && body->tick >= predicted->waiting.tick))) {
            predicted->waiting = *body;
            predicted->has_waiting = 1;
        }
        return PREDICTED_REFUSED;
    }
    if (body->version != BODY_RULES_VERSION) {
        predicted->live = 0;
        return PREDICTED_VERSION;
    }
    /* a word older than the last one taken says nothing new */
    if (predicted->live && (body->tick < predicted->confirmed.tick
                            || (body->tick == predicted->confirmed.tick
                                && (int16_t)(body->request - predicted->confirmed.request) < 0)))
        return PREDICTED_REFUSED;
    made = make(predicted, arena, ground, body, &id);
    if (made == NULL)
        return PREDICTED_REFUSED;
    named_in(made, word_names);
    past = predicted->live && !predicted->stale ? past_at(predicted, body->tick) : NULL;
    was = past != NULL && same(past->state, body->state, BODY_STATE_SIZE);
    /* the last word that differed with nothing to explain it, and no record of its tick or
       before came after it either, is unexplained */
    if (predicted->suspect) {
        predicted->unexplained++;
        predicted->suspect = 0;
    }
    /* a word that is not what was predicted for its tick is suspect when it answers no
       request, no request that waits was due by its tick, the server's game put nothing to the
       body since the last word (its `outside` is the last word's), and no record of a body the
       own body names came since the last word: one of its tick or before that comes before the
       next word still explains it */
    unexplained = past != NULL && !was && !predicted->heard && body->request == predicted->confirmed.request
                  && body->outside == predicted->confirmed.outside;
    for (i = 0; unexplained && i < predicted->pending_count; i++)
        if (predicted->pending[i].apply_tick <= body->tick)
            unexplained = 0;
    if (unexplained) {
        predicted->suspect = 1;
        predicted->suspect_tick = body->tick;
    }
    predicted->heard = 0;
    predicted->confirmed = *body;
    for (i = 0; i < 3; i++)
        predicted->word_names[i] = word_names[i];
    predicted->own_id = id;
    /* the requests the word includes have been answered: they wait no longer */
    for (i = 0; i < predicted->pending_count; i++)
        if ((int16_t)(predicted->pending[i].number - body->request) > 0) {
            predicted->pending[kept++] = predicted->pending[i];
        } else if (predicted->pending[i].number == body->request && body->arrival) {
            predicted->way_left = predicted->pending[i].left;
            predicted->way_arrival = body->arrival;
            predicted->way_new = 1;
        }
    i = predicted->pending_count - kept;
    predicted->pending_count = kept;
    predicted->applied = predicted->applied > i ? predicted->applied - i : 0;
    if (was) {
        predicted->same++;
    } else {
        if (predicted->live)
            predicted->different++;
        predicted->stale = 1;
    }
    predicted->live = 1;
    /* a body that went at the word's tick or before is in no world made from now on, which are
       made from this word or a later one: once the world has let it go too, it is forgotten */
    for (i = 0; i < PREDICTED_OTHERS; i++)
        if (predicted->other[i].gone && predicted->other[i].gone <= body->tick
            && (predicted->other[i].out || predicted->stale))
            zero(&predicted->other[i], sizeof predicted->other[i]);
    return was ? PREDICTED_SAME : PREDICTED_DIFFERENT;
}

void predicted_heard(struct Predicted *predicted, uint32_t id, uint64_t tick)
{
    if (!id || !(id == predicted->word_names[0] || id == predicted->word_names[1] || id == predicted->word_names[2]
                 || (predicted->live && names(predicted, id))))
        return;
    predicted->heard = 1;
    if (predicted->suspect && tick <= predicted->suspect_tick)
        predicted->suspect = 0;
}

int predicted_placed(struct Predicted *predicted, const struct BodyGround *ground, uint32_t placement)
{
    if (predicted->placed && predicted->placement == placement)
        return PREDICTED_REFUSED;
    predicted->placement = placement;
    predicted->placed = 1;
    if (!predicted->has_waiting)
        return PREDICTED_REFUSED;
    predicted->has_waiting = 0;
    return predicted_confirm(predicted, ground, &predicted->waiting);
}

int predicted_request(struct Predicted *predicted, const struct PredictedRequest *request)
{
    uint32_t i;

    if (request->kind < BODY_REQUEST_WALK || request->kind > BODY_REQUEST_PICKUP)
        return 0;
    if (predicted->pending_count == PREDICTED_PENDING) {
        for (i = 1; i < PREDICTED_PENDING; i++)
            predicted->pending[i - 1] = predicted->pending[i];
        predicted->pending_count--;
        if (predicted->applied > 0)
            predicted->applied--;
    }
    predicted->pending[predicted->pending_count++] = *request;
    return 1;
}

/* Whether two records have a body on the same cell at `tick` and on the same steps at the
 * same ticks from then on. */
static int same_from(const struct BodyMove *a, const struct BodyMove *b, uint64_t tick)
{
    struct BodyMove one, two;

    body_move_at(a, tick, &one);
    body_move_at(b, tick, &two);
    if (one.x != two.x || one.y != two.y || one.steps != two.steps)
        return 0;
    return one.steps == 0 || (one.at_tick + one.land_ms == two.at_tick + two.land_ms && one.speed_ms == two.speed_ms
                              && same(one.path, two.path, one.steps));
}

int predicted_other(struct Predicted *predicted, uint32_t id, const struct BodyMove *record)
{
    struct PredictedOther *other = NULL;
    struct BodyMove said = *record;
    uint64_t from, came = 0;
    int i, k, fresh = 0;

    /* a record from outside: only what a record can say */
    if (said.steps > BODY_PATH_STEPS)
        said.steps = BODY_PATH_STEPS;
    for (k = 0; k < BODY_PATH_STEPS; k++)
        said.path[k] = (uint8_t)(k < said.steps ? said.path[k] & 7u : 0);
    said.dir &= 7u;
    said.cause = BODY_CAUSE_WALKED;
    zero(said.reserved, sizeof said.reserved);
    if (said.speed_ms > BODY_MAX_SPEED_MS)
        said.speed_ms = BODY_MAX_SPEED_MS;
    if (said.steps == 0)
        said.land_ms = 0;
    /* the stay of the body the record is of: the one between whose coming and going its tick
       is. A record after the last going is of the stay that came after it, which is in the world
       from its earliest record on; one after every going and before every record of a stay is a
       body the world did not have */
    for (i = 0; i < PREDICTED_OTHERS; i++)
        if (predicted->other[i].used && predicted->other[i].id == id && predicted->other[i].gone
            && predicted->other[i].gone <= said.at_tick && predicted->other[i].gone > came)
            came = predicted->other[i].gone;
    for (i = 0; i < PREDICTED_OTHERS && other == NULL; i++) {
        struct PredictedOther *stay = &predicted->other[i];

        if (!stay->used || stay->id != id)
            continue;
        if (holds(stay, said.at_tick)) {
            other = stay;
        } else if (!stay->gone && came && came <= said.at_tick && said.at_tick < stay->from) {
            stay->from = said.at_tick;
            other = stay;
        }
    }
    /* what its last record has it do already is nothing new */
    if (other != NULL && other->count > 0 && other->record[other->count - 1].at_tick <= said.at_tick) {
        const struct BodyMove *last = &other->record[other->count - 1];

        from = said.at_tick;
        if (predicted->live && !predicted->stale && body_now(world_at(predicted)) > from)
            from = body_now(world_at(predicted));
        if (same_from(last, &said, from))
            return 1;
    }
    for (i = 0; i < PREDICTED_OTHERS && other == NULL; i++)
        if (!predicted->other[i].used)
            other = &predicted->other[i];
    /* no room: one that went goes first, then one the own body does not name */
    for (i = 0; i < PREDICTED_OTHERS && other == NULL; i++)
        if (predicted->other[i].gone)
            other = &predicted->other[i];
    for (i = 0; i < PREDICTED_OTHERS && other == NULL; i++)
        if (!names(predicted, predicted->other[i].id))
            other = &predicted->other[i];
    if (other == NULL)
        return 0;
    if (!other->used || other->id != id || !holds(other, said.at_tick)) {
        other->used = 0;
        open_stay(predicted, other, id, came ? said.at_tick : 0);
        fresh = 1;
    }
    /* the record takes the place of every one with the same or a later tick; with no room
       left the earliest goes, as a later record is complete without it */
    for (k = 0; k < other->count && other->record[k].at_tick < record->at_tick; k++) {
    }
    other->count = (uint8_t)k;
    if (other->count == PREDICTED_RECORDS) {
        for (k = 1; k < PREDICTED_RECORDS; k++)
            other->record[k - 1] = other->record[k];
        other->count--;
    }
    other->record[other->count++] = said;
    /* a body the world did not have, or a record of a tick the world is past: the past changed */
    if (predicted->live && (fresh || record->at_tick <= body_now(world_at(predicted))))
        predicted->stale = 1;
    return 1;
}

void predicted_gone(struct Predicted *predicted, uint32_t id, uint64_t tick)
{
    struct PredictedOther *other, *later = NULL;
    int i, k, kept;

    for (i = 0; i < PREDICTED_OTHERS; i++) {
        other = &predicted->other[i];
        if (!other->used || other->id != id || !holds(other, tick > other->from ? tick - 1 : tick))
            continue;
        if (other->gone == tick)
            return;
        /* its records of the going's tick or later are of a later stay */
        for (k = 0, kept = 0; k < other->count; k++)
            if (other->record[k].at_tick < tick)
                other->record[kept++] = other->record[k];
        if (kept < other->count) {
            for (k = 0; k < PREDICTED_OTHERS && later == NULL; k++)
                if (!predicted->other[k].used)
                    later = &predicted->other[k];
            if (later != NULL) {
                open_stay(predicted, later, id, other->record[kept].at_tick);
                for (k = kept; k < other->count; k++)
                    later->record[later->count++] = other->record[k];
            }
            other->count = (uint8_t)kept;
        }
        other->gone = tick;
        other->out = 0;
        /* a going the world is past is a change of the past, as a record dated behind it is */
        if (predicted->live && !predicted->stale && tick <= body_now(world_at(predicted)))
            predicted->stale = 1;
        return;
    }
}

static int give(struct Predicted *predicted, const struct BodyGround *ground, struct BodyInput *in)
{
    in->tick = body_now(world_at(predicted));
    in->slot = OWN;
    in->id = predicted->own_id;
    return body_input(world_of(predicted), ground, in);
}

/* Takes what the core said and answers the own body's questions as a client does that knows no
 * better: with yes. Returns 0 when questions stay open. */
static int settle(struct Predicted *predicted, const struct BodyGround *ground)
{
    struct BodyWorld *world = world_of(predicted);
    struct BodyEvent events[EVENTS];
    struct BodyInput in;
    int count, i, rounds;

    for (rounds = 0; rounds < 32; rounds++) {
        count = 0;
        while (count < EVENTS && body_take_event(world, &events[count]))
            count++;
        if (count == 0)
            return body_asked(world) == 0;
        for (i = 0; i < count; i++) {
            if (events[i].slot != OWN)
                continue;
            zero(&in, sizeof in);
            switch (events[i].kind) {
            case BODY_EVENT_ASK_SWING:
                in.kind = BODY_INPUT_ANSWER;
                in.value = BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_LINE | BODY_ANSWER_READY;
                break;
            case BODY_EVENT_ASK_HIT:
                in.kind = BODY_INPUT_ANSWER;
                in.value = BODY_ANSWER_HIT;
                break;
            case BODY_EVENT_ASK_CAST:
                in.kind = BODY_INPUT_ANSWER;
                in.value = BODY_ANSWER_ACTED;
                break;
            case BODY_EVENT_ASK_BEGIN: /* a monster's swing: none a player's body asks, but bytes from outside can say otherwise */
                in.kind = BODY_INPUT_ANSWER;
                in.value = BODY_ANSWER_BEGIN;
                break;
            case BODY_EVENT_ASK_INTENT:
                in.kind = BODY_INPUT_INTENT;
                in.dir = BODY_INTENT_ANSWER;
                in.target = events[i].other;
                in.value = BODY_SEES_TARGET | BODY_SEES_LINE;
                break;
            case BODY_EVENT_ASK_STAND:
            case BODY_EVENT_ASK_ACT:
            case BODY_EVENT_ASK_WALK:
                in.kind = BODY_INPUT_INTENT;
                in.dir = BODY_INTENT_ANSWER;
                in.target = events[i].other;
                in.value = BODY_INTENT_OK;
                break;
            default:
                continue;
            }
            /* a question that went with its intent has no answer to take: the core refuses it */
            give(predicted, ground, &in);
        }
    }
    return 0;
}

/* A request goes into the world, at its clock. An attack or a skill on a body the world does not
 * have is a cancel by its number, as the server takes one on a unit it has no body for. One the
 * core refuses is what the server makes of it: a walk (to a cell off the map) a request that did
 * nothing, whose number the body keeps all the same; an attack, a skill or a pickup (without a
 * mode, on a cell off the map) a cancel by its number. */
static void apply(struct Predicted *predicted, const struct BodyGround *ground, const struct PredictedRequest *request)
{
    struct BodyInput in;
    uint32_t i;

    zero(&in, sizeof in);
    in.kind = BODY_INPUT_REQUEST;
    in.dir = request->kind;
    in.value = request->number;
    in.x = request->x;
    in.y = request->y;
    in.cause = request->kind == BODY_REQUEST_ATTACK ? request->mode : request->range;
    if (request->kind == BODY_REQUEST_ATTACK || request->kind == BODY_REQUEST_SKILL) {
        /* whom: another body the world has, or for a skill the own one */
        in.dir = BODY_REQUEST_CANCEL;
        in.target_id = request->target_id;
        if (request->kind == BODY_REQUEST_SKILL && request->target_id == predicted->own_id) {
            in.dir = BODY_REQUEST_SKILL;
            in.target = OWN;
        }
        for (i = 0; i < PREDICTED_OTHERS; i++) {
            if (predicted->other[i].used && predicted->other[i].id == request->target_id) {
                in.dir = request->kind;
                in.target = (uint16_t)i;
            }
        }
    }
    if (give(predicted, ground, &in) != BODY_OK) {
        zero(&in, sizeof in);
        in.kind = BODY_INPUT_REQUEST;
        in.dir = request->kind == BODY_REQUEST_ATTACK || request->kind == BODY_REQUEST_SKILL
                         || request->kind == BODY_REQUEST_SKILL_CELL || request->kind == BODY_REQUEST_PICKUP
                     ? BODY_REQUEST_CANCEL
                     : BODY_REQUEST_NONE;
        in.value = request->number;
        give(predicted, ground, &in);
    }
}

/* Makes the world again from the server's last word. */
static int rebuild(struct Predicted *predicted, const struct BodyGround *ground)
{
    uint32_t i;

    if (make(predicted, predicted->world, ground, &predicted->confirmed, &predicted->own_id) == NULL)
        return 0;
    for (i = 0; i < PREDICTED_OTHERS; i++)
        predicted->other[i].out = (uint8_t)(predicted->other[i].gone && predicted->other[i].gone <= predicted->confirmed.tick);
    predicted->applied = 0;
    predicted->records_tick = predicted->confirmed.tick;
    predicted->past_first = 0;
    predicted->past_count = 0;
    predicted->stale = 0;
    note(predicted);
    return 1;
}

/* Runs the world to `tick`: to each tick something is due, a record of another body begins, a
 * body goes or a request is applied at, in turn. At one tick what is due runs first, then the
 * others' records and goings, then the requests, each of which may make something due at
 * once. */
static int advance(struct Predicted *predicted, const struct BodyGround *ground, uint64_t tick)
{
    struct BodyWorld *world = world_of(predicted);
    struct PredictedOther *other;
    uint64_t now, next, due, at;
    int rounds, result, acted;
    uint32_t i, k;

    for (rounds = 0; rounds < ROUNDS; rounds++) {
        now = body_now(world);
        next = tick > now ? tick : now;
        if (body_next_due(world, &due) && due < next)
            next = due;
        for (i = 0; i < PREDICTED_OTHERS; i++) {
            other = &predicted->other[i];
            for (k = 0; other->used && k < other->count; k++) {
                at = other->record[k].at_tick;
                if (at > predicted->records_tick && at < next && !(other->gone && at >= other->gone))
                    next = at > now ? at : now;
            }
            if (other->used && other->gone && !other->out && other->gone < next)
                next = other->gone > now ? other->gone : now;
        }
        if (predicted->applied < predicted->pending_count) {
            at = predicted->pending[predicted->applied].apply_tick;
            if (at < next)
                next = at > now ? at : now;
        }
        result = body_run(world, ground, next);
        if (result != BODY_OK && result != BODY_ANSWER_FIRST && result != BODY_EVENTS_FULL)
            return 0;
        if (!settle(predicted, ground))
            return 0;
        acted = 0;
        if (!(body_next_due(world, &due) && due <= next)) {
            /* the goings first: a body that comes back at the tick it went is another stay */
            for (i = 0; i < PREDICTED_OTHERS; i++) {
                other = &predicted->other[i];
                if (other->used && other->gone && !other->out && other->gone <= next) {
                    take_out(world, ground, other->slot, other->id);
                    other->out = 1;
                }
            }
            for (i = 0; i < PREDICTED_OTHERS; i++) {
                other = &predicted->other[i];
                for (k = 0; other->used && k < other->count; k++) {
                    at = other->record[k].at_tick;
                    if (at > predicted->records_tick && at <= next && !(other->gone && at >= other->gone))
                        put_other(world, ground, other->slot, other->id, &other->record[k]);
                }
            }
            predicted->records_tick = next;
            if (predicted->applied < predicted->pending_count
                && predicted->pending[predicted->applied].apply_tick <= next) {
                apply(predicted, ground, &predicted->pending[predicted->applied++]);
                if (!settle(predicted, ground))
                    return 0;
                acted = 1;
            }
        }
        note(predicted);
        if (!acted && next >= tick && !(body_next_due(world, &due) && due <= next))
            return 1;
    }
    return 1;
}

int predicted_run(struct Predicted *predicted, const struct BodyGround *ground, uint64_t tick)
{
    if (!predicted->live)
        return 0;
    if (predicted->stale && !rebuild(predicted, ground)) {
        predicted->live = 0;
        return 0;
    }
    if (!advance(predicted, ground, tick)) {
        predicted->live = 0;
        return 0;
    }
    return 1;
}

uint64_t predicted_now(const struct Predicted *predicted)
{
    return predicted->live && !predicted->stale ? body_now(world_at(predicted)) : predicted->live ? predicted->confirmed.tick : 0;
}

uint64_t predicted_wide(const struct Predicted *predicted, uint32_t tick)
{
    uint64_t base = predicted_now(predicted);

    return base + (uint64_t)(int64_t)(int32_t)(tick - (uint32_t)base);
}

int predicted_move(const struct Predicted *predicted, struct BodyMove *move)
{
    return predicted->live && body_move(world_at(predicted), OWN, move) == BODY_OK;
}

int predicted_attack(const struct Predicted *predicted, struct BodyAttack *attack)
{
    return predicted->live && body_attack(world_at(predicted), OWN, attack) == BODY_OK;
}

int predicted_swing(const struct Predicted *predicted, struct PredictedSwing *swing)
{
    struct BodyAttack attack;
    uint64_t now;

    if (!predicted_attack(predicted, &attack) || !attack.swing_open)
        return 0;
    now = body_now(world_at(predicted));
    zero(swing, sizeof *swing);
    swing->lands = now + attack.swing_ms;
    swing->tick = swing->lands >= attack.windup ? swing->lands - attack.windup : 0;
    swing->target_id = attack.swing_target_id;
    swing->motion_ms = attack.amotion;
    return 1;
}

int predicted_intent(const struct Predicted *predicted, struct BodyIntentState *intent)
{
    return predicted->live && body_intent(world_at(predicted), OWN, intent) == BODY_OK;
}

int predicted_state(const struct Predicted *predicted, void *state)
{
    return predicted->live && body_state(world_at(predicted), OWN, state) == BODY_OK;
}

int predicted_walk_news(const struct BodyMove *was, const struct BodyMove *now)
{
    struct BodyMove on;
    const unsigned char *a = (const unsigned char *)&on, *b = (const unsigned char *)now;
    size_t i;

    body_move_at(was, now->at_tick, &on);
    for (i = 0; i < sizeof on; i++) {
        if (a[i] != b[i])
            return now->at_tick < was->at_tick ? PREDICTED_WALK_BACK : PREDICTED_WALK_NEW;
    }
    return PREDICTED_WALK_ON;
}
