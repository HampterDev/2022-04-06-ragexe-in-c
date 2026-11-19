/* The predicted world (world/predicted.h, plans/predicted-world.md): the packet it reads, what
   it does with the server's word, the player's requests and the other bodies' records, and
   that it converges: a server world and a client world of the same core, with random requests,
   a line with lag and jitter and things on the server the client cannot know, show the same
   own body at every tick once the server's last word is in. */
#include "world/predicted.h"
#include "body/grid_path.h"
#include <stdio.h>
#include <string.h>

enum { WIDTH = 40, HEIGHT = 40, EAST = 6 };

static unsigned char cells[WIDTH * HEIGHT];
static uint32_t scratch[BODY_SCRATCH_SIZE / 4];
static const struct BodyGround ground = {cells, scratch};
static int failures;

#define CHECK(condition)                                                    \
    do {                                                                    \
        if (!(condition)) {                                                 \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static void put16(unsigned char *p, unsigned v)
{
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
}

static void put32(unsigned char *p, uint32_t v)
{
    put16(p, v & 0xffffu);
    put16(p + 2, v >> 16);
}

/* ZC_OWN_BODY as the server builds it. Returns its length. */
static size_t packet_of(unsigned char *out, const struct PredictedBody *body, size_t state_size)
{
    put16(out, 0x0f25);
    put16(out + 2, (unsigned)(PREDICTED_PACKET_HEAD + state_size));
    put32(out + 4, body->version);
    put32(out + 8, (uint32_t)body->tick);
    put32(out + 12, (uint32_t)(body->tick >> 32));
    put16(out + 16, body->request);
    put16(out + 18, body->request_steps);
    put32(out + 20, body->penalty_ms);
    put32(out + 24, body->placement);
    put32(out + 28, body->arrival);
    memcpy(out + PREDICTED_PACKET_HEAD, body->state, state_size < BODY_STATE_SIZE ? state_size : BODY_STATE_SIZE);
    return PREDICTED_PACKET_HEAD + state_size;
}

/* A world of the core for the tests to take bodies from: body 7 in slot 2 on (x, y), which
   fights with a reach of one cell, a swing every 600 ms that lands after 150, and walks at
   150 ms a cell; body 9 in slot 5 on (mob_x, mob_y). */
enum { OWN_SLOT = 2, MOB_SLOT = 5, OWN_ID = 7, MOB_ID = 9, SERVER_BODIES = 8, SERVER_EVENTS = 32 };
/* Another player's id as her client and the server's core know her: her account's. */
#define PLAYER_ACCOUNT 2000042u
#define SERVER_ARENA (56 + SERVER_BODIES * BODY_STATE_SIZE + SERVER_EVENTS * 32)

static struct BodyInput input(uint64_t tick, int kind, int slot, uint32_t id)
{
    struct BodyInput in;

    memset(&in, 0, sizeof in);
    in.tick = tick;
    in.kind = (uint16_t)kind;
    in.slot = (uint16_t)slot;
    in.id = id;
    return in;
}

/* A server's world of the own body and another, `other_id`, which stands. */
static struct BodyWorld *server_world(uint64_t *arena, uint64_t now, int x, int y, uint32_t other_id, int mob_x, int mob_y)
{
    struct BodyWorld *world = body_world_init(arena, SERVER_ARENA, SERVER_BODIES, SERVER_EVENTS, WIDTH, HEIGHT, now);
    struct BodyEvent event;
    struct BodyInput in;

    in = input(now, BODY_INPUT_ADD, OWN_SLOT, OWN_ID);
    in.x = (int16_t)x;
    in.y = (int16_t)y;
    in.value = 150;
    CHECK(world != NULL && body_input(world, &ground, &in) == BODY_OK);
    in = input(now, BODY_INPUT_ADD, MOB_SLOT, other_id);
    in.x = (int16_t)mob_x;
    in.y = (int16_t)mob_y;
    in.value = 200;
    CHECK(body_input(world, &ground, &in) == BODY_OK);
    in = input(now, BODY_INPUT_FIGHTER, OWN_SLOT, OWN_ID);
    in.value = 600;
    in.x = 300;
    in.y = 1;
    in.target = 150;
    in.dir = BODY_FIGHTER_PLAYER;
    in.cause = 1; /* a player's swing holds her walk */
    CHECK(body_input(world, &ground, &in) == BODY_OK);
    while (body_take_event(world, &event)) {
    }
    return world;
}

/* The server's word of body 7 as `world` has it now. */
static struct PredictedBody word_of(const struct BodyWorld *world, uint16_t request_steps, uint32_t penalty_ms)
{
    struct PredictedBody body;
    struct BodyIntentState intent;

    memset(&intent, 0, sizeof intent);
    memset(&body, 0, sizeof body);
    body.version = BODY_RULES_VERSION;
    body.tick = body_now(world);
    body.request_steps = request_steps;
    body.penalty_ms = penalty_ms;
    CHECK(body_state(world, OWN_SLOT, body.state) == BODY_OK && body_intent(world, OWN_SLOT, &intent) == BODY_OK);
    body.request = intent.request;
    return body;
}

static void test_read(void)
{
    unsigned char packet[PREDICTED_PACKET_HEAD + BODY_STATE_SIZE + 16];
    struct PredictedBody body, read;
    size_t size;
    int i;

    memset(&read, 0, sizeof read);
    memset(&body, 0, sizeof body);
    body.version = BODY_RULES_VERSION;
    body.tick = 0x12a05f2a0ull; /* above 2^32, as the server's clock is */
    body.request = 0xbeef;
    body.request_steps = 17;
    body.penalty_ms = 150;
    body.placement = 0x89abcdefu;
    body.arrival = 0x01234567u;
    for (i = 0; i < (int)BODY_STATE_SIZE; i++)
        body.state[i] = (unsigned char)(i * 7 + 1);
    size = packet_of(packet, &body, BODY_STATE_SIZE);
    CHECK(size == 256 && predicted_read(packet, size, &read) == 1);
    CHECK(read.version == BODY_RULES_VERSION && read.tick == 0x12a05f2a0ull && read.request == 0xbeef);
    CHECK(read.placement == 0x89abcdefu && read.arrival == 0x01234567u);
    CHECK(read.request_steps == 17 && read.penalty_ms == 150 && memcmp(read.state, body.state, BODY_STATE_SIZE) == 0);
    /* shorter than it says, longer than it says, shorter than its head */
    memset(&read, 0xEE, sizeof read);
    CHECK(predicted_read(packet, size - 1, &read) == 0 && predicted_read(packet, size + 1, &read) == 0);
    CHECK(predicted_read(packet, PREDICTED_PACKET_HEAD - 1, &read) == 0 && predicted_read(packet, 0, &read) == 0);
    CHECK(read.version == 0xEEEEEEEEu); /* nothing was written */
    /* this version with a state of another length is none: shorter, longer, none at all */
    size = packet_of(packet, &body, BODY_STATE_SIZE - 8);
    CHECK(predicted_read(packet, size, &read) == 0);
    size = packet_of(packet, &body, BODY_STATE_SIZE + 8);
    CHECK(predicted_read(packet, size, &read) == 0);
    size = packet_of(packet, &body, 0);
    CHECK(predicted_read(packet, size, &read) == 0);
    /* another version is read, whatever the length of its state, and without it */
    body.version = BODY_RULES_VERSION + 1;
    size = packet_of(packet, &body, BODY_STATE_SIZE + 16);
    CHECK(predicted_read(packet, size, &read) == 1 && read.version == BODY_RULES_VERSION + 1 && read.tick == 0x12a05f2a0ull);
    for (i = 0; i < (int)BODY_STATE_SIZE; i++)
        CHECK(read.state[i] == 0);
    size = packet_of(packet, &body, 0);
    CHECK(predicted_read(packet, size, &read) == 1 && read.request == 0xbeef);
    /* but not when it says another length than it has */
    size = packet_of(packet, &body, 16);
    CHECK(predicted_read(packet, size - 1, &read) == 0 && predicted_read(packet, size + 1, &read) == 0);
}

static struct BodyMove standing(uint64_t tick, int x, int y, int speed)
{
    struct BodyMove move;

    memset(&move, 0, sizeof move);
    move.at_tick = tick;
    move.x = (int16_t)x;
    move.y = (int16_t)y;
    move.speed_ms = (uint16_t)speed;
    return move;
}

static struct PredictedRequest walk_to(int number, uint64_t apply, int x, int y)
{
    struct PredictedRequest request;

    memset(&request, 0, sizeof request);
    request.kind = BODY_REQUEST_WALK;
    request.number = (uint16_t)number;
    request.apply_tick = apply;
    request.x = (int16_t)x;
    request.y = (int16_t)y;
    return request;
}

static struct PredictedRequest attack_on(int number, uint64_t apply, uint32_t target, int mode)
{
    struct PredictedRequest request;

    memset(&request, 0, sizeof request);
    request.kind = BODY_REQUEST_ATTACK;
    request.number = (uint16_t)number;
    request.apply_tick = apply;
    request.target_id = target;
    request.mode = (uint8_t)mode;
    return request;
}

/* The pieces by themselves: nothing is predicted before the server's word; a request is shown
   from the tick it is applied at; the word that confirms it changes nothing; one that does not
   is a correction. */
static void test_world(void)
{
    static struct Predicted predicted;
    static uint64_t arena[(SERVER_ARENA + 7) / 8]; /* static: a 32-bit build aligns its stack to 4 */
    struct BodyWorld *server;
    struct PredictedBody word;
    struct PredictedRequest bad, request;
    struct BodyMove move, mob;
    struct BodyAttack attack;
    struct BodyIntentState intent;
    struct BodyInput in;
    struct BodyEvent event;
    unsigned char state[BODY_STATE_SIZE], theirs[BODY_STATE_SIZE];
    const uint64_t T = 0x12a05f200ull;

    memset(cells, 1, sizeof cells);
    predicted_init(&predicted, WIDTH, HEIGHT);
    CHECK(predicted_placed(&predicted, &ground, 0) == PREDICTED_REFUSED);
    CHECK(predicted_run(&predicted, &ground, T) == 0 && predicted_now(&predicted) == 0);
    CHECK(!predicted_move(&predicted, &move) && !predicted_state(&predicted, state) && !predicted_attack(&predicted, &attack));
    CHECK(predicted_wide(&predicted, 5) == 5);

    /* the server's first word: its body is the client's */
    server = server_world(arena, T, 10, 10, MOB_ID, 14, 10);
    word = word_of(server, 17, 150);
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_DIFFERENT && predicted.different == 0);
    CHECK(predicted_now(&predicted) == T && predicted_run(&predicted, &ground, T + 16) == 1 && predicted_now(&predicted) == T + 16);
    CHECK(predicted_state(&predicted, state) && memcmp(state, word.state, sizeof state) == 0);
    CHECK(predicted_move(&predicted, &move) && move.x == 10 && move.y == 10 && move.steps == 0);
    /* the wire's ticks are whole again around its clock, also behind it and across 2^32 */
    CHECK(predicted_wide(&predicted, (uint32_t)(T + 40)) == T + 40 && predicted_wide(&predicted, (uint32_t)(T - 100)) == T - 100);
    CHECK(predicted_wide(&predicted, 0xFFFFFFF0u) == 0xFFFFFFF0ull && predicted_wide(&predicted, 0x10u) == 0x100000010ull && (T >> 32) == 1);

    /* a request: nothing before the tick it is applied at, the walk from that tick on */
    bad = walk_to(1, T + 60, 13, 10);
    bad.kind = BODY_REQUEST_NONE;
    CHECK(predicted_request(&predicted, &bad) == 0);
    bad.kind = BODY_REQUEST_PICKUP + 1;
    CHECK(predicted_request(&predicted, &bad) == 0 && predicted.pending_count == 0);
    request = walk_to(1, T + 60, 13, 10);
    CHECK(predicted_request(&predicted, &request) == 1);
    CHECK(predicted_run(&predicted, &ground, T + 59) && predicted_move(&predicted, &move) && move.steps == 0);
    CHECK(predicted_run(&predicted, &ground, T + 60) && predicted_move(&predicted, &move));
    CHECK(move.steps == 3 && move.at_tick == T + 60 && move.land_ms == 150 && move.path[0] == EAST);
    CHECK(predicted_intent(&predicted, &intent) && intent.request == 1 && predicted.pending_count == 1 && predicted.applied == 1);
    CHECK(predicted_run(&predicted, &ground, T + 250) && predicted_move(&predicted, &move) && move.x == 11 && move.steps == 2);
    /* a run to a tick behind the clock changes nothing */
    CHECK(predicted_run(&predicted, &ground, T + 100) && predicted_now(&predicted) == T + 250);

    /* the server applied it at the same tick, and says so: what was predicted. Nothing changes,
       and the request waits no longer */
    in = input(T + 60, BODY_INPUT_REQUEST, OWN_SLOT, OWN_ID);
    in.dir = BODY_REQUEST_WALK;
    in.value = 1;
    in.x = 13;
    in.y = 10;
    body_set_request_steps(server, 17);
    CHECK(body_run(server, &ground, T + 60) == BODY_OK && body_input(server, &ground, &in) == BODY_OK);
    word = word_of(server, 17, 150);
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_SAME && predicted.same == 1 && predicted.different == 0);
    CHECK(predicted.pending_count == 0 && predicted.applied == 0 && predicted_now(&predicted) == T + 250);
    CHECK(predicted_run(&predicted, &ground, T + 260) && predicted_move(&predicted, &move) && move.x == 11 && move.steps == 2);
    /* the same word again says nothing new; an older one is refused, also one of the same tick
       with an older request */
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_SAME);
    word.tick--;
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_REFUSED);
    word.tick++;
    word.request = 0;
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_REFUSED && predicted.confirmed.request == 1);
    word.request = 1;

    /* a hold the client could not know: the server's word is another body than was predicted.
       The world is made again from it, and is the server's from then on */
    while (body_take_event(server, &event)) {
    }
    CHECK(body_run(server, &ground, T + 210) == BODY_OK && body_take_event(server, &event));
    in = input(T + 300, BODY_INPUT_DELAY, OWN_SLOT, OWN_ID);
    in.value = 400;
    CHECK(body_run(server, &ground, T + 300) == BODY_OK && body_input(server, &ground, &in) == BODY_OK);
    CHECK(predicted_run(&predicted, &ground, T + 380) && predicted_move(&predicted, &move) && move.x == 12);
    word = word_of(server, 17, 150);
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_DIFFERENT && predicted.different == 1);
    CHECK(predicted_run(&predicted, &ground, T + 380) && predicted_move(&predicted, &move) && move.x == 11 && move.steps == 2);
    CHECK(move.at_tick + move.land_ms == T + 700);
    while (body_take_event(server, &event)) {
    }
    CHECK(body_run(server, &ground, T + 380) == BODY_OK);
    CHECK(predicted_state(&predicted, state) && body_state(server, OWN_SLOT, theirs) == BODY_OK && memcmp(state, theirs, sizeof state) == 0);

    /* bytes that are no body, and a word of another version: refused; after the other version
       nothing is predicted until a word of this one */
    word.state[4] = 0; /* not in use */
    word.tick += 10;
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_REFUSED && predicted.live);
    word = word_of(server, 17, 150);
    word.version = BODY_RULES_VERSION + 1;
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_VERSION && !predicted.live);
    CHECK(predicted_run(&predicted, &ground, T + 400) == 0 && !predicted_move(&predicted, &move));
    word.version = BODY_RULES_VERSION;
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_DIFFERENT && predicted_run(&predicted, &ground, T + 400));

    /* an order to attack a body the client has a record of: the walk into reach and the swing
       are the core's, with every question answered yes. One on a body it has no record of is a
       cancel, as the server takes it: the number is kept */
    request = attack_on(2, T + 460, MOB_ID, BODY_ATTACK_ONCE);
    CHECK(predicted_request(&predicted, &request) == 1);
    CHECK(predicted_run(&predicted, &ground, T + 460) && predicted_intent(&predicted, &intent) && !intent.has && intent.request == 2);
    mob = standing(T, 14, 10, 200);
    CHECK(predicted_other(&predicted, MOB_ID, &mob) == 1 && predicted.stale);
    CHECK(predicted_run(&predicted, &ground, T + 460) && predicted_intent(&predicted, &intent));
    CHECK(intent.has && intent.kind == BODY_WANTS_ATTACK && intent.seq == 2 && intent.target_id == MOB_ID && intent.walked);
    /* she lands at 700 by the hold, turns after the monster there, is beside it a step later
       and swings on arrival: the attack is for one swing, so the intent is done with it */
    CHECK(predicted_run(&predicted, &ground, T + 849) && predicted_attack(&predicted, &attack) && !attack.swing_open);
    CHECK(predicted_run(&predicted, &ground, T + 850) && predicted_move(&predicted, &move) && move.x == 13 && move.steps == 0);
    CHECK(predicted_attack(&predicted, &attack) && attack.swing_open && attack.swing_target_id == MOB_ID && attack.has_target);
    CHECK(predicted_intent(&predicted, &intent) && !intent.has);
    CHECK(predicted_run(&predicted, &ground, T + 1000) && predicted_attack(&predicted, &attack) && !attack.swing_open);
    /* a record of the monster for a tick the world is past changes the past: made again, the
       world has her walk after it to where it went */
    request = attack_on(3, T + 1500, MOB_ID, BODY_ATTACK_REPEAT);
    CHECK(predicted_request(&predicted, &request) == 1);
    CHECK(predicted_run(&predicted, &ground, T + 1600) && predicted_move(&predicted, &move) && move.x == 13 && move.steps == 0);
    mob = standing(T + 1400, 20, 10, 200);
    CHECK(predicted_other(&predicted, MOB_ID, &mob) == 1 && predicted.stale);
    CHECK(predicted_run(&predicted, &ground, T + 1600) && predicted_move(&predicted, &move) && move.steps > 0 && predicted.pending_count == 2);
    /* one that is still to come does not: the world takes it when its clock is there */
    mob = standing(T + 2000, 13, 12, 200);
    CHECK(predicted_other(&predicted, MOB_ID, &mob) == 1 && !predicted.stale);
    /* gone at a tick still to come: nothing of the past changes, it leaves when the world's
       clock is there, and told again changes nothing; then the attack finds nobody, and its
       intent ends */
    predicted_gone(&predicted, MOB_ID, T + 2100);
    CHECK(!predicted.stale && predicted.other[0].gone == T + 2100);
    predicted_gone(&predicted, MOB_ID, T + 2100);
    CHECK(!predicted.stale && predicted.other[0].gone == T + 2100 && !predicted.other[1].used);
    CHECK(predicted_run(&predicted, &ground, T + 2200) && predicted_intent(&predicted, &intent) && !intent.has);

    /* the others the world holds: one that went gives its place first, and a fourth then takes
       the place of one the own body does not name */
    CHECK(predicted_other(&predicted, 21, &mob) && predicted_other(&predicted, 22, &mob) && predicted_other(&predicted, 23, &mob));
    CHECK(predicted.other[0].id == 23 && predicted.other[1].id == 21 && predicted.other[2].id == 22);
    CHECK(predicted_other(&predicted, 24, &mob) == 1 && predicted.other[0].id == 24 && predicted.other[1].id == 21);
    /* more requests than the list holds: the oldest goes */
    {
        int i;

        for (i = 0; i < PREDICTED_PENDING + 3; i++) {
            request = walk_to(10 + i, T + 5000, 12, 12);
            CHECK(predicted_request(&predicted, &request) == 1);
        }
        CHECK(predicted.pending_count == PREDICTED_PENDING && predicted.pending[PREDICTED_PENDING - 1].number == 10 + PREDICTED_PENDING + 2);
        CHECK(predicted.pending[0].number == 13);
    }
}

/* A prediction and a server world that begin as one: body 7 on (x, y), body 9 on (mob_x, mob_y),
   of which the client has a record, and the server's first word taken. */
static struct BodyWorld *begin(struct Predicted *predicted, uint64_t *arena, uint64_t tick, int x, int y, int mob_x, int mob_y)
{
    struct BodyWorld *server = server_world(arena, tick, x, y, MOB_ID, mob_x, mob_y);
    struct BodyMove mob = standing(tick, mob_x, mob_y, 200);
    struct PredictedBody word;

    memset(cells, 1, sizeof cells);
    body_set_request_steps(server, 17);
    body_set_cancel_penalty(server, 150);
    predicted_init(predicted, WIDTH, HEIGHT);
    predicted_placed(predicted, &ground, 0);
    word = word_of(server, 17, 150);
    CHECK(predicted_other(predicted, MOB_ID, &mob) == 1);
    CHECK(predicted_confirm(predicted, &ground, &word) == PREDICTED_DIFFERENT && predicted_run(predicted, &ground, tick) == 1);
    return server;
}

/* The server's world takes a request of body 7 at `tick`. */
static void server_takes(struct BodyWorld *server, uint64_t tick, const struct PredictedRequest *request)
{
    struct BodyInput in = input(tick, BODY_INPUT_REQUEST, OWN_SLOT, OWN_ID);
    struct BodyEvent event;
    uint64_t due;

    while (body_next_due(server, &due) && due <= tick) {
        CHECK(body_run(server, &ground, due) == BODY_OK);
        while (body_take_event(server, &event)) {
        }
    }
    in.dir = request->kind;
    in.value = request->number;
    in.x = request->x;
    in.y = request->y;
    CHECK(body_run(server, &ground, tick) == BODY_OK && body_input(server, &ground, &in) == BODY_OK);
    while (body_take_event(server, &event)) {
    }
}

/* What is predicted is neither lost nor doubled, and happens at its tick. */
static void test_requests(void)
{
    static struct Predicted predicted;
    static uint64_t arena[(SERVER_ARENA + 7) / 8];
    const uint64_t T = 0x12a05f200ull;
    struct BodyWorld *server = begin(&predicted, arena, T, 10, 10, 30, 30);
    struct PredictedRequest first = walk_to(0xFFFF, T + 60, 13, 10), second = walk_to(1, T + 200, 10, 13);
    struct PredictedRequest third = walk_to(2, T + 500, 12, 12), fourth = walk_to(3, T + 500, 14, 14);
    struct PredictedBody word;
    struct BodyIntentState intent;
    struct BodyMove move;
    struct BodyEvent event;
    unsigned char state[BODY_STATE_SIZE], theirs[BODY_STATE_SIZE];
    uint64_t due;

    /* a word for a tick the world has not come to is not what was predicted for it, even of a
       body that is as it was: the world is made again at that tick */
    CHECK(predicted_run(&predicted, &ground, T + 16) && body_run(server, &ground, T + 40) == BODY_OK);
    word = word_of(server, 17, 150);
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_DIFFERENT && predicted.stale);
    CHECK(predicted_run(&predicted, &ground, T + 16) && predicted_now(&predicted) == T + 40);

    /* two requests wait, their numbers on either side of the wrap. The server applied the first
       40 ms later than the client reckoned: its word includes that one and is another body
       than was predicted. The second waits on, and is in the world made again, at its own
       tick, in the middle of the run that makes it */
    CHECK(predicted_request(&predicted, &first) == 1 && predicted_request(&predicted, &second) == 1);
    CHECK(predicted_run(&predicted, &ground, T + 300) && predicted_intent(&predicted, &intent) && intent.request == 1);
    server_takes(server, T + 100, &first);
    word = word_of(server, 17, 150);
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_DIFFERENT);
    CHECK(predicted.pending_count == 1 && predicted.pending[0].number == 1 && predicted.different == 2);
    CHECK(predicted_run(&predicted, &ground, T + 300) && predicted_intent(&predicted, &intent) && intent.request == 1);
    server_takes(server, T + 200, &second);
    word = word_of(server, 17, 150);
    while (body_next_due(server, &due) && due <= T + 300) {
        CHECK(body_run(server, &ground, due) == BODY_OK);
        while (body_take_event(server, &event)) {
        }
    }
    CHECK(body_run(server, &ground, T + 300) == BODY_OK);
    CHECK(predicted_state(&predicted, state) && body_state(server, OWN_SLOT, theirs) == BODY_OK && memcmp(state, theirs, sizeof state) == 0);
    CHECK(predicted_move(&predicted, &move) && move.x == 11 && move.y == 10 && move.steps > 0 && move.at_tick == T + 250);
    /* and the word of the second is what was predicted */
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_SAME && predicted.pending_count == 0 && predicted.different == 2);

    /* two requests applied at one tick: the state kept for that tick is the one both left */
    CHECK(predicted_request(&predicted, &third) == 1 && predicted_request(&predicted, &fourth) == 1);
    CHECK(predicted_run(&predicted, &ground, T + 600) && predicted.applied == 2);
    server_takes(server, T + 500, &third);
    server_takes(server, T + 500, &fourth);
    word = word_of(server, 17, 150);
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_SAME && predicted.pending_count == 0 && predicted.applied == 0);
}

/* A server world that answers every question of body 7 with yes, run to `tick`. */
static void server_yes_to(struct BodyWorld *server, uint64_t tick)
{
    struct BodyEvent event;
    struct BodyInput in;
    uint64_t due;
    int rounds = 0, result;

    for (;;) {
        while (body_take_event(server, &event)) {
            if (event.slot != OWN_SLOT)
                continue;
            in = input(event.tick, BODY_INPUT_ANSWER, OWN_SLOT, OWN_ID);
            if (event.kind == BODY_EVENT_ASK_SWING) {
                in.value = BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_LINE | BODY_ANSWER_READY;
            } else if (event.kind == BODY_EVENT_ASK_HIT) {
                in.value = BODY_ANSWER_HIT;
            } else if (event.kind >= BODY_EVENT_ASK_INTENT && event.kind <= BODY_EVENT_ASK_WALK) {
                in.kind = BODY_INPUT_INTENT;
                in.dir = BODY_INTENT_ANSWER;
                in.target = event.other;
                in.value = event.kind == BODY_EVENT_ASK_INTENT ? BODY_SEES_TARGET | BODY_SEES_LINE : BODY_INTENT_OK;
            } else {
                continue;
            }
            CHECK(body_input(server, &ground, &in) == BODY_OK);
        }
        if (!body_next_due(server, &due) || due > tick || ++rounds > 4000)
            break;
        result = body_run(server, &ground, due);
        CHECK(result == BODY_OK || result == BODY_ANSWER_FIRST);
    }
    CHECK(body_asked(server) == 0 && body_run(server, &ground, tick) == BODY_OK);
}

/* A skill on a monster out of its range, a ground skill out of its range, a pickup two cells off
   and one four cells off: each predicted is the server's body at every tick, the walk into
   reach and the act included, and the server's word of it is what was predicted. */
static void test_skills_and_pickups(void)
{
    static struct Predicted predicted;
    static uint64_t arena[(SERVER_ARENA + 7) / 8];
    const uint64_t T = 0x12a05f200ull;
    static const struct {
        uint8_t kind, range;
        int16_t x, y;
        int walks;
    } cases[] = {
        {BODY_REQUEST_SKILL, 3, 0, 0, 1},       /* on the monster eight cells east */
        {BODY_REQUEST_SKILL_CELL, 4, 10, 18, 1}, /* eight cells north */
        {BODY_REQUEST_PICKUP, 2, 12, 11, 0},     /* two cells off: picked up from where she stands */
        {BODY_REQUEST_PICKUP, 2, 14, 10, 1},     /* four: she walks until it is two cells off */
    };
    struct BodyWorld *server;
    struct PredictedRequest request;
    struct PredictedBody word;
    struct BodyIntentState intent;
    struct BodyMove move;
    struct BodyInput in;
    struct BodyEvent event;
    unsigned char state[BODY_STATE_SIZE], theirs[BODY_STATE_SIZE];
    uint64_t tick;
    size_t k;
    int walked, differ;

    for (k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        server = begin(&predicted, arena, T, 10, 10, 18, 10);
        memset(&request, 0, sizeof request);
        request.kind = cases[k].kind;
        request.number = 1;
        request.apply_tick = T + 100;
        request.range = cases[k].range;
        request.x = cases[k].x;
        request.y = cases[k].y;
        if (request.kind == BODY_REQUEST_SKILL)
            request.target_id = MOB_ID;
        CHECK(predicted_request(&predicted, &request) == 1);
        server_yes_to(server, T + 100);
        in = input(T + 100, BODY_INPUT_REQUEST, OWN_SLOT, OWN_ID);
        in.dir = request.kind;
        in.value = request.number;
        in.x = request.x;
        in.y = request.y;
        in.cause = request.range;
        in.target = request.kind == BODY_REQUEST_SKILL ? MOB_SLOT : 0;
        in.target_id = request.target_id;
        CHECK(body_input(server, &ground, &in) == BODY_OK);
        walked = 0;
        differ = 0;
        memset(&move, 0, sizeof move);
        for (tick = T + 100; tick <= T + 2000; tick += 10) {
            server_yes_to(server, tick);
            CHECK(predicted_run(&predicted, &ground, tick));
            CHECK(predicted_state(&predicted, state) && body_state(server, OWN_SLOT, theirs) == BODY_OK);
            if (memcmp(state, theirs, sizeof state) != 0 && !differ++)
                fprintf(stderr, "case %u: the predicted body is not the server's at %llu\n", (unsigned)k, (unsigned long long)(tick - T));
            CHECK(predicted_move(&predicted, &move));
            walked |= move.steps > 0;
        }
        CHECK(differ == 0 && walked == cases[k].walks);
        /* the act was asked for and carried out: the intent is over, standing in reach */
        CHECK(predicted_intent(&predicted, &intent) && !intent.has && intent.request == 1 && move.steps == 0);
        word = word_of(server, 17, 150);
        CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_SAME && predicted.pending_count == 0);
        while (body_take_event(server, &event)) {
        }
    }
    /* a skill on a body the world has no record of is a cancel, as the server takes one on a unit
       it has no body for: the intent the body had goes with it */
    (void)begin(&predicted, arena, T, 10, 10, 18, 10);
    memset(&request, 0, sizeof request);
    request.kind = BODY_REQUEST_SKILL_CELL;
    request.number = 1;
    request.apply_tick = T + 100;
    request.range = 1;
    request.x = 30;
    request.y = 10;
    CHECK(predicted_request(&predicted, &request) == 1);
    request.kind = BODY_REQUEST_SKILL;
    request.number = 2;
    request.apply_tick = T + 200;
    request.target_id = 4242;
    CHECK(predicted_request(&predicted, &request) == 1);
    CHECK(predicted_run(&predicted, &ground, T + 150) && predicted_intent(&predicted, &intent) && intent.has && intent.walked);
    CHECK(predicted_run(&predicted, &ground, T + 250) && predicted_intent(&predicted, &intent) && !intent.has && intent.request == 2);
}

/* The other bodies' records: which holds when, and whom the own body can lose. */
static void test_records(void)
{
    static struct Predicted predicted;
    static uint64_t arena[(SERVER_ARENA + 7) / 8];
    const uint64_t T = 0x12a05f200ull;
    struct PredictedRequest order = attack_on(1, T + 1000, MOB_ID, BODY_ATTACK_REPEAT), walk = walk_to(2, T + 3050, 10, 13);
    struct BodyMove mob, move;
    struct BodyAttack attack;
    struct BodyIntentState intent;
    struct PredictedSwing swing;

    begin(&predicted, arena, T, 10, 10, 11, 10);
    /* a record that has the body where its last one has it says nothing new: nothing is filed
       and the past stays; one for an earlier tick is filed, the same cell or not */
    mob = standing(T + 400, 11, 10, 200);
    CHECK(predicted_other(&predicted, MOB_ID, &mob) == 1 && predicted.other[0].count == 1 && !predicted.stale);
    mob.steps = 2;
    mob.land_ms = 200;
    mob.path[0] = mob.path[1] = 6;
    CHECK(predicted_other(&predicted, MOB_ID, &mob) == 1 && predicted.other[0].count == 2);
    /* the same walk said from its second cell on, as someone has it who looks later */
    mob = standing(T + 600, 12, 10, 200);
    mob.steps = 1;
    mob.land_ms = 200;
    mob.path[0] = 6;
    CHECK(predicted_other(&predicted, MOB_ID, &mob) == 1 && predicted.other[0].count == 2);
    /* and its end, standing there */
    mob = standing(T + 900, 13, 10, 200);
    CHECK(predicted_other(&predicted, MOB_ID, &mob) == 1 && predicted.other[0].count == 2);
    /* standing a cell on is news */
    mob = standing(T + 900, 14, 10, 200);
    CHECK(predicted_other(&predicted, MOB_ID, &mob) == 1 && predicted.other[0].count == 3);
    CHECK(!predicted_swing(&predicted, &swing));

    begin(&predicted, arena, T, 10, 10, 11, 10);
    /* a record takes the place of every one of its body with the same or a later tick */
    mob = standing(T + 1600, 30, 30, 200);
    CHECK(predicted_other(&predicted, MOB_ID, &mob) == 1);
    mob = standing(T + 2000, 31, 31, 200);
    CHECK(predicted_other(&predicted, MOB_ID, &mob) == 1 && predicted.other[0].count == 3);
    mob = standing(T + 1550, 20, 10, 200);
    CHECK(predicted_other(&predicted, MOB_ID, &mob) == 1 && predicted.other[0].count == 2);
    CHECK(predicted.other[0].record[0].at_tick == T && predicted.other[0].record[1].at_tick == T + 1550 && !predicted.stale);
    /* a record to come holds from its tick on and not before: the monster is beside her when
       the order is applied, and she swings */
    CHECK(predicted_request(&predicted, &order) == 1 && predicted_run(&predicted, &ground, T + 1100));
    CHECK(predicted_attack(&predicted, &attack) && attack.swing_open && predicted_move(&predicted, &move) && move.steps == 0);
    /* the swing in its windup, for who shows it: when it began, when it lands, at whom, and
       the body's attack motion */
    CHECK(predicted_swing(&predicted, &swing) && swing.tick == T + 1000 && swing.lands == T + 1150);
    CHECK(swing.target_id == MOB_ID && swing.motion_ms == 300);
    CHECK(predicted_run(&predicted, &ground, T + 1540) && predicted_move(&predicted, &move) && move.steps == 0);
    /* landed, it is none any more */
    CHECK(!predicted_swing(&predicted, &swing));
    /* and the world stops at that tick to take it: the look of her attack that is due 50 ms
       later finds the monster gone off, and she walks after it */
    CHECK(predicted_run(&predicted, &ground, T + 1650) && predicted_attack(&predicted, &attack) && !attack.swing_open);
    CHECK(predicted_move(&predicted, &move) && move.steps > 0 && move.at_tick == T + 1600);
    CHECK(predicted_intent(&predicted, &intent) && intent.has && intent.walked);

    /* a body the world did not have changes the past, whenever its record begins */
    mob = standing(T + 9000, 5, 5, 200);
    CHECK(predicted_other(&predicted, 21, &mob) == 1 && predicted.stale && predicted_run(&predicted, &ground, T + 1650));
    /* made again, the world has the monster on the record that held then, and her as before */
    CHECK(predicted_move(&predicted, &move) && move.steps > 0 && move.at_tick == T + 1600 && move.x == 10);
    /* with all three places taken, a fourth takes the place of one she does not name: the one
       she is after stays */
    CHECK(predicted_other(&predicted, 22, &mob) == 1 && predicted_other(&predicted, 23, &mob) == 1);
    CHECK(predicted.other[0].id == MOB_ID && predicted.other[1].id == 23 && predicted.other[2].id == 22);
    CHECK(predicted_run(&predicted, &ground, T + 1700) && predicted_intent(&predicted, &intent) && intent.has && intent.target_id == MOB_ID);

    /* the world's settings are the server's: a swing that a walk ends in its windup lets the
       next begin no sooner than the penalty the server said */
    begin(&predicted, arena, T, 10, 10, 11, 10);
    order.apply_tick = T + 3000;
    CHECK(predicted_request(&predicted, &order) == 1 && predicted_run(&predicted, &ground, T + 3040));
    CHECK(predicted_attack(&predicted, &attack) && attack.swing_open);
    CHECK(predicted_request(&predicted, &walk) == 1 && predicted_run(&predicted, &ground, T + 3060));
    CHECK(predicted_attack(&predicted, &attack) && !attack.swing_open && attack.attackable == T + 3050 + 150);
    /* and the swing the walk ended is none to show */
    CHECK(!predicted_swing(&predicted, &swing));
}

/* Runs a prediction and a server world that answers yes from `from` to `to`, every 10 ms, and
   counts the ticks at which the own body differs. */
static int differences(struct Predicted *predicted, struct BodyWorld *server, uint64_t from, uint64_t to)
{
    unsigned char state[BODY_STATE_SIZE], theirs[BODY_STATE_SIZE];
    uint64_t tick;
    int differ = 0;

    for (tick = from; tick <= to; tick += 10) {
        server_yes_to(server, tick);
        CHECK(predicted_run(predicted, &ground, tick));
        CHECK(predicted_state(predicted, state) && body_state(server, OWN_SLOT, theirs) == BODY_OK);
        differ += memcmp(state, theirs, sizeof state) != 0;
    }
    return differ;
}

/* The server's world takes an order of body 7 to attack the monster at `tick`. */
static int server_attack(struct BodyWorld *server, uint64_t tick, int number, int mode)
{
    struct BodyInput in = input(tick, BODY_INPUT_REQUEST, OWN_SLOT, OWN_ID);

    server_yes_to(server, tick);
    in.dir = BODY_REQUEST_ATTACK;
    in.value = (uint16_t)number;
    in.cause = (uint8_t)mode;
    in.target = MOB_SLOT;
    in.target_id = MOB_ID;
    return body_input(server, &ground, &in);
}

/* A body is in the worlds of the ticks between its coming and its going, whatever arrived first
   (plans/presence.md, part 3). Her one swing at the monster lands and kills it on the server,
   and its going, dated at that tick, is read here a while later: the world is made again, the
   swing lands in it as on the server, and her attack delay runs from the swing's start as
   there. Killed by another a tick before her swing lands, the swing lands on nobody. A word of a
   tick before the going that arrives after it has the body. */
static void test_gone(void)
{
    static struct Predicted predicted;
    static uint64_t arena[(SERVER_ARENA + 7) / 8];
    const uint64_t T = 0x12a05f200ull;
    struct PredictedRequest order = attack_on(1, T + 100, MOB_ID, BODY_ATTACK_ONCE);
    struct BodyWorld *server = begin(&predicted, arena, T, 10, 10, 11, 10);
    struct BodyMove somebody = standing(T, 30, 30, 200), back = standing(T + 900, 14, 10, 200);
    struct PredictedBody word, late, mid;
    struct BodyAttack attack;
    struct BodyInput out;
    struct BodyEvent event;
    int i, stays;

    CHECK(predicted_request(&predicted, &order) == 1 && server_attack(server, T + 100, 1, BODY_ATTACK_ONCE) == BODY_OK);
    CHECK(differences(&predicted, server, T + 100, T + 120) == 0);
    word = word_of(server, 17, 150);
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_SAME);
    CHECK(predicted_attack(&predicted, &attack) && attack.swing_open && attack.attackable == T + 700);
    CHECK(differences(&predicted, server, T + 130, T + 240) == 0);
    /* the swing lands at 250 and the monster dies of it there, on the server */
    server_yes_to(server, T + 250);
    while (body_take_event(server, &event)) {
    }
    out = input(T + 250, BODY_INPUT_REMOVE, MOB_SLOT, MOB_ID);
    CHECK(body_input(server, &ground, &out) == BODY_OK);
    /* the client has it a while longer, until its going is read */
    CHECK(predicted_run(&predicted, &ground, T + 300));
    CHECK(predicted_other(&predicted, 21, &somebody) == 1 && predicted.stale);
    predicted_gone(&predicted, MOB_ID, T + 250);
    CHECK(predicted.other[0].id == MOB_ID && predicted.other[0].gone == T + 250);
    CHECK(differences(&predicted, server, T + 310, T + 600) == 0);
    mid = word_of(server, 17, 150);
    CHECK(differences(&predicted, server, T + 601, T + 900) == 0);
    CHECK(predicted_attack(&predicted, &attack) && !attack.swing_open && attack.attackable == T + 700);
    /* told again, nothing changes */
    predicted_gone(&predicted, MOB_ID, T + 250);
    CHECK(!predicted.stale && predicted.other[0].gone == T + 250);
    /* one that comes back is another stay, in the world from its first record on, in the core's
       slot of the one that went; and the past changed */
    CHECK(predicted_other(&predicted, MOB_ID, &back) == 1 && predicted.stale);
    for (i = 0, stays = 0; i < PREDICTED_OTHERS; i++)
        if (predicted.other[i].used && predicted.other[i].id == MOB_ID)
            stays += predicted.other[i].gone == T + 250 ? 1 : predicted.other[i].gone == 0 && predicted.other[i].from == T + 900 ? 2 : 100;
    CHECK(stays == 3);
    /* made again, the world has no monster between its going and its first record back */
    {
        struct BodyMove mob;

        CHECK(predicted_confirm(&predicted, &ground, &mid) != PREDICTED_REFUSED);
        CHECK(predicted_run(&predicted, &ground, T + 850) && predicted_now(&predicted) == T + 850);
        CHECK(body_move((const struct BodyWorld *)predicted.world, predicted.other[0].slot, &mob) != BODY_OK);
        CHECK(predicted_run(&predicted, &ground, T + 900));
        CHECK(body_move((const struct BodyWorld *)predicted.world, predicted.other[0].slot, &mob) == BODY_OK && mob.x == 14);
    }
    for (i = 1; i < PREDICTED_OTHERS; i++)
        if (predicted.other[i].used && predicted.other[i].id == MOB_ID)
            CHECK(predicted.other[i].slot == predicted.other[0].slot);

    /* another's hit kills it a tick before her swing lands: the swing lands on nobody */
    server = begin(&predicted, arena, T, 10, 10, 11, 10);
    CHECK(predicted_request(&predicted, &order) == 1 && server_attack(server, T + 100, 1, BODY_ATTACK_ONCE) == BODY_OK);
    CHECK(differences(&predicted, server, T + 100, T + 240) == 0);
    server_yes_to(server, T + 249);
    while (body_take_event(server, &event)) {
    }
    out = input(T + 249, BODY_INPUT_REMOVE, MOB_SLOT, MOB_ID);
    CHECK(body_input(server, &ground, &out) == BODY_OK);
    CHECK(predicted_run(&predicted, &ground, T + 320));
    predicted_gone(&predicted, MOB_ID, T + 249);
    CHECK(predicted.stale);
    CHECK(differences(&predicted, server, T + 330, T + 900) == 0);

    /* a word of a tick before the going that arrives after it has the body, and the first word
       after it lets the body go for good */
    server = begin(&predicted, arena, T, 10, 10, 11, 10);
    CHECK(predicted_request(&predicted, &order) == 1 && server_attack(server, T + 100, 1, BODY_ATTACK_ONCE) == BODY_OK);
    CHECK(differences(&predicted, server, T + 100, T + 260) == 0);
    late = word_of(server, 17, 150);
    CHECK(differences(&predicted, server, T + 261, T + 300) == 0);
    out = input(T + 300, BODY_INPUT_REMOVE, MOB_SLOT, MOB_ID);
    CHECK(body_input(server, &ground, &out) == BODY_OK);
    predicted_gone(&predicted, MOB_ID, T + 300);
    CHECK(differences(&predicted, server, T + 301, T + 600) == 0 && predicted.other[0].out);
    CHECK(predicted_confirm(&predicted, &ground, &late) == PREDICTED_SAME && predicted.other[0].used);
    CHECK(differences(&predicted, server, T + 601, T + 700) == 0);
    word = word_of(server, 17, 150);
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_SAME && !predicted.other[0].used);

    /* a going dated behind the word the client holds already, as the server's core lost the body
       before it could run to the present: the world is made again from that word without it */
    server = begin(&predicted, arena, T, 10, 10, 11, 10);
    CHECK(predicted_request(&predicted, &order) == 1 && server_attack(server, T + 100, 1, BODY_ATTACK_ONCE) == BODY_OK);
    CHECK(differences(&predicted, server, T + 100, T + 280) == 0);
    out = input(T + 280, BODY_INPUT_REMOVE, MOB_SLOT, MOB_ID);
    CHECK(body_input(server, &ground, &out) == BODY_OK);
    CHECK(predicted_run(&predicted, &ground, T + 300));
    server_yes_to(server, T + 300);
    while (body_take_event(server, &event)) {
    }
    word = word_of(server, 17, 150);
    CHECK(predicted_confirm(&predicted, &ground, &word) != PREDICTED_REFUSED && word.tick == T + 300);
    predicted_gone(&predicted, MOB_ID, T + 280);
    CHECK(predicted.stale);
    CHECK(differences(&predicted, server, T + 301, T + 900) == 0);
}

/* A request the core refuses is what the server makes of it: a walk one that did nothing, whose
   number the body keeps; an attack, a skill or a pickup a cancel by its number, which ends the
   attack under way. */
static void test_refused(void)
{
    static struct Predicted predicted;
    static uint64_t arena[(SERVER_ARENA + 7) / 8];
    const uint64_t T = 0x12a05f200ull;
    struct PredictedRequest order = attack_on(1, T + 100, MOB_ID, BODY_ATTACK_REPEAT), no_mode = attack_on(2, T + 400, MOB_ID, 7);
    struct PredictedRequest off_map = walk_to(3, T + 900, WIDTH + 5, 10);
    struct BodyWorld *server = begin(&predicted, arena, T, 10, 10, 11, 10);
    struct BodyAttack attack;
    struct BodyIntentState intent;
    struct BodyInput in;

    CHECK(predicted_request(&predicted, &order) == 1 && predicted_request(&predicted, &no_mode) == 1);
    CHECK(predicted_request(&predicted, &off_map) == 1);
    CHECK(server_attack(server, T + 100, 1, BODY_ATTACK_REPEAT) == BODY_OK);
    CHECK(differences(&predicted, server, T + 100, T + 390) == 0);
    /* the server's game: the core refuses the order without a mode, and it is a cancel */
    CHECK(server_attack(server, T + 400, 2, 7) == BODY_BAD_VALUE);
    in = input(T + 400, BODY_INPUT_REQUEST, OWN_SLOT, OWN_ID);
    in.dir = BODY_REQUEST_CANCEL;
    in.value = 2;
    CHECK(body_input(server, &ground, &in) == BODY_OK);
    CHECK(differences(&predicted, server, T + 400, T + 890) == 0);
    CHECK(predicted_attack(&predicted, &attack) && !attack.has_target && predicted_intent(&predicted, &intent) && intent.request == 2);
    /* and the walk off the map one that did nothing */
    server_yes_to(server, T + 900);
    in = input(T + 900, BODY_INPUT_REQUEST, OWN_SLOT, OWN_ID);
    in.dir = BODY_REQUEST_WALK;
    in.value = 3;
    in.x = WIDTH + 5;
    in.y = 10;
    CHECK(body_input(server, &ground, &in) == BODY_BAD_CELL);
    in.dir = BODY_REQUEST_NONE;
    CHECK(body_input(server, &ground, &in) == BODY_OK);
    CHECK(differences(&predicted, server, T + 900, T + 1200) == 0);
    CHECK(predicted_intent(&predicted, &intent) && intent.request == 3);
}

/* --- convergence ------------------------------------------------------------------------- */

static uint32_t rng_state;

static uint32_t rnd(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state >> 8;
}

static uint32_t between(uint32_t low, uint32_t high)
{
    return low + rnd() % (high - low + 1u);
}

static uint64_t digest(const unsigned char *bytes, size_t size)
{
    uint64_t h = 0xcbf29ce484222325ull;
    size_t i;

    for (i = 0; i < size; i++)
        h = (h ^ bytes[i]) * 0x100000001b3ull;
    return h | 1u; /* never 0, which says that nothing was seen */
}

/* What travels down the line: the server's word of the own body, a record of the monster, or
   that the monster is gone, with the tick it went at (ZC_VIEW_STAMP). A record carries its
   revision and a going the revision of the move track then, as the server counts them by unit. */
enum { DOWN_BODY = 1, DOWN_RECORD = 2, DOWN_GONE = 3 };

struct Down {
    uint64_t arrives;
    uint64_t tick;
    int kind;
    uint32_t id;
    uint16_t rev;
    struct PredictedBody body;
    struct BodyMove record;
};

/* A request on its way up, with the tick the server applies it at. */
struct Up {
    uint64_t applies;
    struct PredictedRequest request;
};

enum { LINE = 256, RUN_MS = 14000, QUIET_MS = 4000, SETTLE_MS = 1000, HOLD_MS = 60, FRAME_MS = 16 };

struct Line {
    struct Down down[LINE]; /* in the order sent; each leaves when it arrives */
    struct Up up[LINE];
    int down_count, up_first, up_count;
    uint64_t last_down, last_up; /* in order, neither way does one packet overtake another */
    /* down the line, what the server says overtakes what it said before and comes twice, as
       the words, the others' records and the goings may when each travels on its own */
    int disorder;
};

struct Server {
    uint64_t arena[(SERVER_ARENA + 7) / 8];
    struct BodyWorld *world;
    struct BodyMove mob_walk;   /* the monster's own walk, which is not the core's: the server puts it where that has it */
    struct BodyMove mob_sent;   /* what the client was last told of it */
    unsigned char told[BODY_STATE_SIZE];
    uint32_t mob_id;
    uint16_t mob_rev;    /* the revision of the monster's move track, counted on across its stays */
    uint64_t gone_tick;  /* when it went last */
    uint64_t back_at;    /* it comes back under the same id then; 0 for not */
    unsigned lag;
    int kill;            /* the own swing that landed now kills it */
    int kills_own, kills_other, returns;
    int has_told, changed, chaos, mob_there;
    uint16_t request_steps;
    uint32_t penalty_ms;
    int refused, not_yes, surprises;
};

struct Outcome {
    int requests, refused, not_yes, surprises, same, different, compared, late;
    int kills_own, kills_other, returns, overtaken;
};

static void put_down(struct Line *line, const struct Down *item)
{
    if (line->down_count == LINE) {
        CHECK(!"the line holds no more");
        return;
    }
    line->down[line->down_count++] = *item;
}

static void send_down(struct Line *line, struct Down *item, uint64_t now, unsigned lag, unsigned jitter)
{
    item->arrives = now + lag + (jitter ? rnd() % (jitter + 1u) : 0u);
    if (!line->disorder) {
        if (item->arrives < line->last_down)
            item->arrives = line->last_down;
        line->last_down = item->arrives;
        put_down(line, item);
        return;
    }
    /* now and then one is held back, as a lost datagram's is until it is sent again, and now
       and then one comes twice */
    if (rnd() % 4u == 0)
        item->arrives += 10u + rnd() % (2u * lag + jitter + 40u);
    put_down(line, item);
    if (rnd() % 5u == 0) {
        item->arrives += 1u + rnd() % (lag + jitter + 30u);
        put_down(line, item);
    }
}

/* Takes what has arrived by `now` and came first, of what was sent first. Returns 0 when nothing
   has. */
static int take_down(struct Line *line, uint64_t now, struct Down *item, int *overtaken)
{
    int i, at = -1;

    for (i = 0; i < line->down_count; i++)
        if (line->down[i].arrives <= now && (at < 0 || line->down[i].arrives < line->down[at].arrives))
            at = i;
    if (at < 0)
        return 0;
    if (at > 0)
        (*overtaken)++;
    *item = line->down[at];
    for (i = at + 1; i < line->down_count; i++)
        line->down[i - 1] = line->down[i];
    line->down_count--;
    return 1;
}

/* An input of the server's game. One that changed the own body is news for its player. */
static int server_put(struct Server *server, struct BodyInput in)
{
    unsigned char before[BODY_STATE_SIZE], after[BODY_STATE_SIZE];
    int result;

    in.tick = body_now(server->world);
    body_state(server->world, OWN_SLOT, before);
    result = body_input(server->world, &ground, &in);
    body_state(server->world, OWN_SLOT, after);
    if (result == BODY_OK && memcmp(before, after, sizeof after) != 0)
        server->changed = 1;
    return result;
}

static int mob_walks(const struct Server *server)
{
    struct BodyMove at;

    body_move_at(&server->mob_walk, body_now(server->world), &at);
    return server->mob_there && at.steps > 0;
}

/* The server's game answers the core: with yes, and while there is chaos now and then not. */
static void server_settle(struct Server *server)
{
    struct BodyEvent events[SERVER_EVENTS];
    struct BodyIntentState intent;
    struct BodyInput in;
    int count, i, rounds, yes;

    for (rounds = 0; rounds < 32; rounds++) {
        count = 0;
        while (count < SERVER_EVENTS && body_take_event(server->world, &events[count]))
            count++;
        if (count == 0)
            break;
        for (i = 0; i < count; i++) {
            if (events[i].slot != OWN_SLOT)
                continue;
            yes = !server->chaos || rnd() % 6u != 0;
            in = input(0, BODY_INPUT_ANSWER, OWN_SLOT, OWN_ID);
            switch (events[i].kind) {
            case BODY_EVENT_ASK_SWING:
                in.value = BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_LINE | BODY_ANSWER_READY;
                if (!yes) {
                    in.value = (uint16_t)(rnd() & (BODY_ANSWER_CAN_FIGHT | BODY_ANSWER_LINE | BODY_ANSWER_READY | BODY_ANSWER_CASTING));
                    in.target_id = rnd() % 3u == 0 ? between(1, 400) : 0;
                }
                if (mob_walks(server))
                    in.value |= BODY_ANSWER_WALKS;
                break;
            case BODY_EVENT_ASK_HIT:
                in.value = yes ? BODY_ANSWER_HIT : 0;
                /* now and then the own swing kills the monster it lands on */
                if (yes && server->chaos && server->mob_there && rnd() % 2u == 0)
                    server->kill = 1;
                break;
            case BODY_EVENT_ASK_CAST:
                in.value = BODY_ANSWER_ACTED;
                break;
            case BODY_EVENT_ASK_INTENT:
                in.kind = BODY_INPUT_INTENT;
                in.dir = BODY_INTENT_ANSWER;
                in.target = events[i].other;
                in.value = BODY_SEES_TARGET | BODY_SEES_LINE;
                if (!yes) {
                    in.value = (uint16_t)(rnd() & (BODY_SEES_TARGET | BODY_SEES_LINE | BODY_SEES_CASTING | BODY_SEES_SITTING));
                    in.target_id = rnd() % 3u == 0 ? between(1, 450) : 0;
                }
                /* the monster is not there for the game when it is gone: an intent about it sees no target */
                if (body_intent(server->world, OWN_SLOT, &intent) == BODY_OK && intent.target_id != 0 && !server->mob_there)
                    in.value = (uint16_t)(in.value & ~BODY_SEES_TARGET);
                if (mob_walks(server))
                    in.value |= BODY_SEES_TARGET_WALKS;
                break;
            case BODY_EVENT_ASK_STAND:
            case BODY_EVENT_ASK_ACT:
            case BODY_EVENT_ASK_WALK:
                in.kind = BODY_INPUT_INTENT;
                in.dir = BODY_INTENT_ANSWER;
                in.target = events[i].other;
                in.value = (uint16_t)(yes ? BODY_INTENT_OK : rnd() % 2u ? BODY_INTENT_STUCK : 0);
                break;
            default:
                continue;
            }
            if (!yes)
                server->not_yes++;
            server_put(server, in);
        }
    }
    CHECK(body_asked(server->world) == 0);
}

/* The monster is put where its own walk has it, at the world's clock, before the world is run
   on: as the server does with a unit whose walk is not the core's. */
static void server_place_mob(struct Server *server, uint64_t tick)
{
    struct BodyMove at, has;
    struct BodyInput in;

    if (!server->mob_there || body_move(server->world, MOB_SLOT, &has) != BODY_OK)
        return;
    body_move_at(&server->mob_walk, tick, &at);
    if (at.x == has.x && at.y == has.y)
        return;
    in = input(0, BODY_INPUT_PLACE, MOB_SLOT, server->mob_id);
    in.x = at.x;
    in.y = at.y;
    in.dir = at.dir;
    in.cause = BODY_CAUSE_PLACED;
    server_put(server, in);
    server_settle(server);
}

static void server_run_to(struct Server *server, uint64_t tick)
{
    uint64_t due;
    int result;

    for (;;) {
        if (!body_next_due(server->world, &due) || due > tick)
            due = tick;
        server_place_mob(server, due);
        result = body_run(server->world, &ground, due);
        CHECK(result == BODY_OK || result == BODY_ANSWER_FIRST || result == BODY_EVENTS_FULL);
        server_settle(server);
        if (due == tick && !(body_next_due(server->world, &due) && due <= tick))
            break;
    }
}

/* The monster sets out on a walk of its own, which the client is told as a record. */
static void server_mob_walk(struct Server *server, int x, int y)
{
    struct GridPathMap map = {WIDTH, HEIGHT, cells};
    struct GridPathStep steps[BODY_PATH_STEPS];
    struct BodyMove at;
    uint64_t now = body_now(server->world);
    int count, i;

    body_move_at(&server->mob_walk, now, &at);
    /* a step under way lands first, as the core has it too */
    if (at.steps > 0)
        body_move_at(&server->mob_walk, at.at_tick + at.land_ms, &at);
    count = grid_path_search(&map, at.x, at.y, x, y, steps, 12, scratch, sizeof scratch);
    if (count <= 0)
        return;
    at.steps = (uint8_t)count;
    for (i = 0; i < BODY_PATH_STEPS; i++)
        at.path[i] = (uint8_t)(i < count ? steps[i].dir : 0);
    at.land_ms = (uint16_t)grid_path_step_ms(at.speed_ms, at.path[0] & 1);
    if (at.at_tick < now) {
        at.at_tick = now;
    } else {
        /* from the cell the step under way leads to, when it lands there */
        at.land_ms = (uint16_t)(at.land_ms + (at.at_tick - now));
        body_move_at(&server->mob_walk, now, &server->mob_walk);
        at.x = server->mob_walk.x;
        at.y = server->mob_walk.y;
        at.dir = server->mob_walk.dir;
        at.steps = (uint8_t)(count < BODY_PATH_STEPS ? count + 1 : BODY_PATH_STEPS);
        for (i = BODY_PATH_STEPS - 1; i > 0; i--)
            at.path[i] = at.path[i - 1];
        at.path[0] = server->mob_walk.path[0];
        at.land_ms = server->mob_walk.land_ms;
        at.at_tick = now;
        for (i = at.steps; i < BODY_PATH_STEPS; i++)
            at.path[i] = 0;
    }
    server->mob_walk = at;
}

/* The monster dies at the server's clock, and now and then comes back under its id within a
   round trip, elsewhere. */
static void server_mob_dies(struct Server *server)
{
    struct BodyEvent event;

    server_put(server, input(0, BODY_INPUT_REMOVE, MOB_SLOT, server->mob_id));
    server->mob_there = 0;
    server->gone_tick = body_now(server->world);
    server->back_at = rnd() % 2u ? server->gone_tick + between(5, 2u * server->lag + 5u) : 0;
    while (body_take_event(server->world, &event)) {
    }
}

/* The monster appears on a cell that can be stood on: under a new id, or under its own when it
   comes back. Returns 0 when the cell drawn cannot be. */
static int server_mob_appears(struct Server *server, int same_id)
{
    struct BodyInput in = input(0, BODY_INPUT_ADD, MOB_SLOT, same_id ? server->mob_id : server->mob_id + 1);
    struct BodyEvent event;

    in.x = (int16_t)between(6, WIDTH - 7);
    in.y = (int16_t)between(6, HEIGHT - 7);
    if (!cells[in.y * WIDTH + in.x])
        return 0;
    in.value = 200;
    if (server_put(server, in) != BODY_OK)
        return 0;
    server->mob_id = in.id;
    server->mob_there = 1;
    server->back_at = 0;
    server->mob_walk = standing(body_now(server->world), in.x, in.y, 200);
    memset(&server->mob_sent, 0xFF, sizeof server->mob_sent);
    while (body_take_event(server->world, &event)) {
    }
    return 1;
}

/* What happens to the monster at the server's tick, after what was due: the own swing that
   landed now killed it; another's hit kills it a tick before the own swing lands; it comes back. */
static void server_mob_fate(struct Server *server)
{
    struct BodyAttack attack;

    if (server->kill && server->mob_there) {
        server_mob_dies(server);
        server->kills_own++;
    }
    server->kill = 0;
    if (server->chaos && server->mob_there && body_attack(server->world, OWN_SLOT, &attack) == BODY_OK
        && attack.swing_open && attack.swing_target_id == server->mob_id && attack.swing_ms == 1 && rnd() % 2u == 0) {
        server_mob_dies(server);
        server->kills_other++;
    }
    if (!server->mob_there && server->back_at && body_now(server->world) >= server->back_at && server_mob_appears(server, 1))
        server->returns++;
}

/* Something happens to the own body that its player's client cannot know before it is told. */
static void server_surprise(struct Server *server)
{
    struct BodyInput in = input(0, BODY_INPUT_DELAY, OWN_SLOT, OWN_ID);
    struct BodyMove own;
    int x, y;

    body_move(server->world, OWN_SLOT, &own);
    server->surprises++;
    switch (rnd() % 9u) {
    case 0:
    case 1: /* a hit holds her */
        in.value = (uint16_t)between(80, 500);
        break;
    case 2: /* she is pushed to a cell nearby, if that one can be stood on */
        x = own.x + (int)(rnd() % 5u) - 2;
        y = own.y + (int)(rnd() % 5u) - 2;
        if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT || !cells[y * WIDTH + x])
            return;
        in.kind = BODY_INPUT_PLACE;
        in.x = (int16_t)x;
        in.y = (int16_t)y;
        in.dir = own.dir;
        in.cause = BODY_CAUSE_PUSHED;
        break;
    case 3: /* the game stops her walk */
        in.kind = BODY_INPUT_STOP;
        break;
    case 4: /* she fights otherwise: another reach, another speed of attack */
        in.kind = BODY_INPUT_FIGHTER;
        in.value = (uint16_t)between(300, 900);
        in.x = (int16_t)between(150, 400);
        in.y = (int16_t)between(1, 4);
        in.target = (uint16_t)(rnd() % 3u ? between(60, 250) : 0);
        in.dir = BODY_FIGHTER_PLAYER;
        in.cause = 1;
        break;
    case 5: /* a skill's delay */
        in.kind = BODY_INPUT_ACT_CLOCK;
        in.target_id = between(100, 700);
        break;
    case 6: /* she walks at another speed */
        in.kind = BODY_INPUT_SPEED;
        in.value = (uint16_t)between(100, 220);
        break;
    case 7: /* the monster walks off */
        if (server->mob_there)
            server_mob_walk(server, (int)between(4, WIDTH - 5), (int)between(4, HEIGHT - 5));
        return;
    default: /* the monster dies, and another appears elsewhere */
        if (server->mob_there)
            server_mob_dies(server);
        else
            server_mob_appears(server, 0);
        return;
    }
    server_put(server, in);
    server_settle(server);
}

/* The server's frame is over: its player is told the body when what was put in changed it and
   it is not what she was told last; and the monster's record when it has a new one. */
static void server_tell(struct Server *server, struct Line *line, unsigned lag, unsigned jitter)
{
    struct Down item;
    struct BodyMove at;
    uint64_t now = body_now(server->world);

    memset(&item, 0, sizeof item);
    if (server->changed) {
        server->changed = 0;
        item.kind = DOWN_BODY;
        item.body = word_of(server->world, server->request_steps, server->penalty_ms);
        if (!server->has_told || memcmp(server->told, item.body.state, BODY_STATE_SIZE) != 0) {
            memcpy(server->told, item.body.state, BODY_STATE_SIZE);
            server->has_told = 1;
            send_down(line, &item, now, lag, jitter);
        }
    }
    if (!server->mob_there) {
        if (server->mob_sent.speed_ms != 0) {
            item.kind = DOWN_GONE;
            item.id = server->mob_id;
            item.tick = server->gone_tick;
            item.rev = server->mob_rev;
            memset(&server->mob_sent, 0, sizeof server->mob_sent);
            send_down(line, &item, now, lag, jitter);
        }
        return;
    }
    body_move_at(&server->mob_sent, now, &at);
    if (memcmp(&at, &server->mob_walk, sizeof at) != 0) {
        body_move_at(&server->mob_walk, now, &at);
        if (memcmp(&at, &server->mob_walk, sizeof at) == 0 || server->mob_walk.at_tick == now) {
            /* a new record, as the walk is now */
            item.kind = DOWN_RECORD;
            item.id = server->mob_id;
            item.rev = ++server->mob_rev;
            item.record = server->mob_walk;
            server->mob_sent = server->mob_walk;
            send_down(line, &item, now, lag, jitter);
        }
    }
}

static uint64_t server_at[RUN_MS + 2000], client_at[RUN_MS + 2000];

/* One run: `chaos_ms` of a player who asks for things and a server on which things happen,
   then quiet. The other body, the one she attacks, is named `other_id` on both sides. Returns
   what happened, and checks that the two bodies are one from when the quiet has settled. */
static struct Outcome converge(uint32_t seed, unsigned lag, unsigned jitter, int clock_error, int chaos, int first_number,
                               uint32_t other_id, int disorder)
{
    static struct Predicted predicted;
    static struct Server server;
    static struct Line line;
    const uint64_t start = 0xFFFFF000ull + (uint64_t)seed * 977u; /* the clock crosses 2^32 in the run */
    const uint64_t chaos_end = start + RUN_MS - QUIET_MS, end = start + RUN_MS;
    struct Outcome outcome;
    struct PredictedRequest request;
    struct Down arrived, *down = &arrived;
    struct Up *up;
    /* the move floor of each id the monster had, as the client keeps it by unit id
       (world/view_floors.h): a record not newer than it is the past */
    struct {
        uint32_t id;
        uint16_t rev;
    } floors[64];
    int floor_count = 0, f;
    struct BodyInput in;
    struct BodyIntentState intent;
    unsigned char state[BODY_STATE_SIZE];
    uint64_t t, own_clock, next_request, next_surprise, last_arrives = 0, settled = 0;
    unsigned lead = (lag > HOLD_MS ? lag : HOLD_MS);
    uint32_t different_then = 0;
    uint16_t number = (uint16_t)first_number;
    int result, mismatch = 0, refusal;
    size_t i;

    memset(&outcome, 0, sizeof outcome);
    memset(&line, 0, sizeof line);
    line.disorder = disorder;
    memset(&server, 0, sizeof server);
    memset(server_at, 0, sizeof server_at);
    memset(client_at, 0, sizeof client_at);
    rng_state = seed;
    for (i = 0; i < sizeof cells; i++)
        cells[i] = (unsigned char)(rnd() % 9u != 0);
    cells[20 * WIDTH + 20] = cells[20 * WIDTH + 24] = 1;
    predicted_init(&predicted, WIDTH, HEIGHT);
    predicted_placed(&predicted, &ground, 0);
    server.request_steps = 17;
    server.penalty_ms = chaos ? 120 : 0;
    server.world = server_world(server.arena, start, 20, 20, other_id, 24, 20);
    body_set_request_steps(server.world, server.request_steps);
    body_set_cancel_penalty(server.world, server.penalty_ms);
    server.mob_id = other_id;
    server.lag = lag;
    server.mob_there = 1;
    server.mob_walk = standing(start, 24, 20, 200);
    memset(&server.mob_sent, 0xFF, sizeof server.mob_sent);
    server.changed = 1;
    server.chaos = chaos;
    next_request = start + 300;
    next_surprise = start + between(400, 1500);

    for (t = start; t < end; t++) {
        /* --- the server: what is due, the requests that are applied now, what else happens --- */
        server_run_to(&server, t);
        server_mob_fate(&server);
        while (line.up_count > 0 && line.up[line.up_first].applies <= t) {
            up = &line.up[line.up_first];
            line.up_first = (line.up_first + 1) % LINE;
            line.up_count--;
            in = input(0, BODY_INPUT_REQUEST, OWN_SLOT, OWN_ID);
            in.dir = up->request.kind;
            in.value = up->request.number;
            in.x = up->request.x;
            in.y = up->request.y;
            in.cause = up->request.kind == BODY_REQUEST_ATTACK ? up->request.mode : up->request.range;
            if (up->request.kind == BODY_REQUEST_ATTACK || up->request.kind == BODY_REQUEST_SKILL) {
                in.target = MOB_SLOT;
                in.target_id = up->request.target_id;
            }
            /* now and then the game refuses a request before the core sees it, and the core
               refuses some (an attack on a monster that is gone, one without a mode). As the
               server's game: an attack, a skill or a pickup it refuses is a cancel by its
               number, anything else a request that did nothing, which keeps its number */
            refusal = in.dir == BODY_REQUEST_ATTACK || in.dir == BODY_REQUEST_SKILL || in.dir == BODY_REQUEST_SKILL_CELL
                              || in.dir == BODY_REQUEST_PICKUP
                          ? BODY_REQUEST_CANCEL
                          : BODY_REQUEST_NONE;
            if (server.chaos && rnd() % 8u == 0) {
                in.dir = (uint8_t)refusal;
                server.refused++;
            }
            result = server_put(&server, in);
            if (result != BODY_OK) {
                in.dir = (uint8_t)refusal;
                CHECK(server_put(&server, in) == BODY_OK);
                server.refused++;
            }
            server_settle(&server);
        }
        if (server.chaos && t >= next_surprise && t < chaos_end) {
            server_surprise(&server);
            next_surprise = t + between(150, 1200);
        }
        if (t == chaos_end)
            server.chaos = 0;
        server_tell(&server, &line, lag, jitter);
        CHECK(body_state(server.world, OWN_SLOT, state) == BODY_OK);
        server_at[t - start] = digest(state, sizeof state);

        /* --- the client, a frame every 16 ms: what arrived, what the player asks, the world --- */
        if ((t - start) % FRAME_MS != 0)
            continue;
        while (take_down(&line, t, &arrived, &outcome.overtaken)) {
            for (f = 0; f < floor_count && floors[f].id != down->id; f++) {
            }
            if (down->kind != DOWN_BODY && f == floor_count) {
                CHECK(floor_count < 64);
                floors[floor_count].id = down->id;
                floors[floor_count].rev = (uint16_t)(down->rev - 1u);
                floor_count++;
            }
            if (down->kind == DOWN_BODY) {
                /* a word older than the last taken, or the same again, is refused */
                result = predicted_confirm(&predicted, &ground, &down->body);
                CHECK(result == PREDICTED_SAME || result == PREDICTED_DIFFERENT || (disorder && result == PREDICTED_REFUSED));
            } else if (down->kind == DOWN_RECORD) {
                if ((int16_t)(uint16_t)(down->rev - floors[f].rev) > 0) {
                    floors[f].rev = down->rev;
                    CHECK(predicted_other(&predicted, down->id, &down->record) == 1);
                }
            } else {
                if ((int16_t)(uint16_t)(down->rev - floors[f].rev) > 0)
                    floors[f].rev = down->rev;
                predicted_gone(&predicted, down->id, down->tick);
            }
        }
        /* the own clock: the server's when a request sent now is applied there, as the client
           reckons it, which may be a little off */
        own_clock = t + lead + (uint64_t)(int64_t)clock_error;
        if (t >= next_request && t < chaos_end && predicted.live && predicted.past_count > 0) {
            memset(&request, 0, sizeof request);
            if (++number == 0)
                number = 1;
            request.number = number;
            request.apply_tick = own_clock;
            switch (rnd() % 11u) {
            case 0:
            case 1:
            case 2:
                request.kind = BODY_REQUEST_ATTACK;
                request.target_id = server.mob_id; /* the one she sees, which may be gone by then */
                request.mode = (uint8_t)(rnd() % 4u);
                if (request.mode == 3)
                    request.mode = 7; /* no mode: both sides refuse it alike */
                break;
            case 3:
                request.kind = BODY_REQUEST_RELEASE;
                break;
            case 4:
                request.kind = BODY_REQUEST_CANCEL;
                break;
            case 5:
                request.kind = BODY_REQUEST_SKILL;
                request.target_id = server.mob_id;
                request.range = (uint8_t)between(0, 9);
                break;
            case 6:
            case 7:
                /* a ground skill, or a pickup, on a cell near her */
                request.kind = (uint8_t)(rnd() % 2u ? BODY_REQUEST_SKILL_CELL : BODY_REQUEST_PICKUP);
                {
                    struct BodyMove own;

                    predicted_move(&predicted, &own);
                    request.x = (int16_t)(own.x + (int)(rnd() % 21u) - 10);
                    request.y = (int16_t)(own.y + (int)(rnd() % 21u) - 10);
                }
                request.range = (uint8_t)(request.kind == BODY_REQUEST_PICKUP ? 2 : between(1, 9));
                break;
            default:
                request.kind = BODY_REQUEST_WALK;
                CHECK(predicted_move(&predicted, &(struct BodyMove){0}));
                {
                    struct BodyMove own;

                    predicted_move(&predicted, &own);
                    request.x = (int16_t)(own.x + (int)(rnd() % 17u) - 8);
                    request.y = (int16_t)(own.y + (int)(rnd() % 17u) - 8);
                    if (rnd() % 16u == 0)
                        request.x = (int16_t)(WIDTH + 3); /* off the map */
                }
                break;
            }
            /* the last thing she asks for is one that goes on: an attack, or a long walk */
            if (t + 900 >= chaos_end) {
                request.kind = (uint8_t)(seed % 2u ? BODY_REQUEST_ATTACK : BODY_REQUEST_WALK);
                request.target_id = server.mob_id;
                request.mode = BODY_ATTACK_REPEAT;
                request.x = (int16_t)between(5, WIDTH - 6);
                request.y = (int16_t)between(5, HEIGHT - 6);
                next_request = end;
            } else {
                next_request = t + (rnd() % 4u == 0 ? between(20, 120) : between(150, 900));
            }
            CHECK(predicted_request(&predicted, &request) == 1);
            /* up the line: it arrives after the way up, never before one sent earlier, and the
               server applies it at the tick its stamp named, the own clock, when it arrives
               before that, and on arrival when it came later than the client allowed for */
            if (line.up_count < LINE) {
                up = &line.up[(line.up_first + line.up_count++) % LINE];
                up->request = request;
                up->applies = t + lag + (jitter ? rnd() % (jitter + 1u) : 0u);
                if (up->applies < last_arrives)
                    up->applies = last_arrives;
                if (up->applies <= t)
                    up->applies = t + 1;
                last_arrives = up->applies;
                if (up->applies <= request.apply_tick)
                    up->applies = request.apply_tick;
                else
                    outcome.late++;
            }
            outcome.requests++;
        }
        if (!predicted_run(&predicted, &ground, own_clock)) {
            CHECK(!predicted.live && predicted.confirmed.tick == 0); /* before the server's first word arrived */
            continue;
        }
        if (predicted_state(&predicted, state) && predicted_now(&predicted) == own_clock && own_clock < end)
            client_at[own_clock - start] = digest(state, sizeof state);
        /* once the last word of the chaos is in, nothing the server says is news */
        if (t >= chaos_end + SETTLE_MS && !settled) {
            settled = t;
            different_then = predicted.different;
        }
    }
    /* from then on the client's body at a tick is the server's at that tick */
    for (t = settled + lead + 64; settled && t < end; t++) {
        if (client_at[t - start] == 0)
            continue;
        outcome.compared++;
        if (client_at[t - start] != server_at[t - start] && !mismatch++)
            fprintf(stderr, "seed %u lag %u jitter %u disorder %d: the bodies differ at tick +%u\n", (unsigned)seed, lag, jitter, disorder,
                    (unsigned)(t - start));
    }
    CHECK(mismatch == 0);
    CHECK(settled != 0 && outcome.compared > 100);
    CHECK(predicted.different == different_then); /* no correction after the chaos */
    /* (words of the last moments may still be on their way down) */
    CHECK(predicted.live && predicted.pending_count == 0 && line.up_count == 0);
    /* every request was taken once: the server's body has the number of the last */
    CHECK(body_intent(server.world, OWN_SLOT, &intent) == BODY_OK && intent.request == number);
    CHECK(predicted_intent(&predicted, &intent) && intent.request == number);
    outcome.kills_own = server.kills_own;
    outcome.kills_other = server.kills_other;
    outcome.returns = server.returns;
    outcome.refused = server.refused;
    outcome.not_yes = server.not_yes;
    outcome.surprises = server.surprises;
    outcome.same = (int)predicted.same;
    outcome.different = (int)predicted.different;
    return outcome;
}

/* What a move of the own body says to one who drew an earlier one: nothing while the body only
   takes the steps of that walk, something when the walk is another, and that the past changed
   when the other one was made before it. */
static void test_walk_news(void)
{
    struct BodyMove walk, on, other, before;
    uint64_t tick;

    memset(&walk, 0, sizeof walk);
    walk.at_tick = 1000;
    walk.x = 5;
    walk.y = 5;
    walk.land_ms = 150;
    walk.speed_ms = 150;
    walk.steps = 3;
    walk.path[0] = walk.path[1] = walk.path[2] = 6; /* east */
    CHECK(predicted_walk_news(&walk, &walk) == PREDICTED_WALK_ON);
    /* every tick of the walk, and long after it: the same walk further along, then its end */
    for (tick = 1000; tick < 2000; tick += 7) {
        body_move_at(&walk, tick, &on);
        CHECK(predicted_walk_news(&walk, &on) == PREDICTED_WALK_ON);
    }
    body_move_at(&walk, 1450, &on);
    CHECK(on.steps == 0 && on.x == 8 && on.y == 5 && on.at_tick == 1450);
    /* one who drew the walk at its second step is told nothing by its third */
    body_move_at(&walk, 1150, &before);
    body_move_at(&walk, 1300, &on);
    CHECK(before.steps == 2 && predicted_walk_news(&before, &on) == PREDICTED_WALK_ON);
    /* a turn at the second cell: another walk */
    body_move_at(&walk, 1150, &other);
    other.path[0] = 0; /* north */
    CHECK(predicted_walk_news(&walk, &other) == PREDICTED_WALK_NEW);
    /* a stop on the cell it is on, made now */
    body_move_at(&walk, 1150, &other);
    other.steps = 0;
    other.land_ms = 0;
    memset(other.path, 0, sizeof other.path);
    CHECK(predicted_walk_news(&walk, &other) == PREDICTED_WALK_NEW);
    /* a step that lands later than the walk said (a hold): another walk */
    body_move_at(&walk, 1150, &other);
    other.land_ms = 400;
    CHECK(predicted_walk_news(&walk, &other) == PREDICTED_WALK_NEW);
    /* the stand the walk began from, made before it: the walk was not so */
    memset(&before, 0, sizeof before);
    before.at_tick = 400;
    before.x = 5;
    before.y = 5;
    before.speed_ms = 150;
    CHECK(predicted_walk_news(&walk, &before) == PREDICTED_WALK_BACK);
    CHECK(predicted_walk_news(&before, &walk) == PREDICTED_WALK_NEW);
    /* a stand is a stand at any later tick */
    CHECK(predicted_walk_news(&before, &before) == PREDICTED_WALK_ON);
}

/* A word that answers a waiting request and carries that request's arrival hands back when the
   request left and when it arrived, once: a sample of the way up. A word without an arrival, or
   one that answers it again, hands back nothing. */
static void test_way_up_samples(void)
{
    static struct Predicted predicted;
    static uint64_t arena[(SERVER_ARENA + 7) / 8];
    const uint64_t T = 0x12a05f200ull;
    struct BodyWorld *server = begin(&predicted, arena, T, 10, 10, 30, 30);
    struct PredictedRequest first = walk_to(5, T + 60, 13, 10), second = walk_to(6, T + 200, 10, 13);
    struct PredictedBody word;

    first.left = 0xfffffff0u;
    second.left = 77;
    CHECK(predicted_request(&predicted, &first) == 1 && predicted_request(&predicted, &second) == 1);
    server_takes(server, T + 60, &first);
    word = word_of(server, 17, 150);
    word.arrival = 0;
    CHECK(predicted_confirm(&predicted, &ground, &word) != PREDICTED_REFUSED && !predicted.way_new);
    server_takes(server, T + 200, &second);
    word = word_of(server, 17, 150);
    word.arrival = 0x12345;
    CHECK(predicted_confirm(&predicted, &ground, &word) != PREDICTED_REFUSED && predicted.way_new);
    CHECK(predicted.way_left == 77 && predicted.way_arrival == 0x12345);
    predicted.way_new = 0;
    CHECK(predicted_confirm(&predicted, &ground, &word) != PREDICTED_REFUSED && !predicted.way_new);
}

/* The server tells a player's body again when two seconds passed without a word of it. A body
   that the server changed behind the client's back, with no word, request or record, is so
   put right within the two seconds: the word told again differs, and the world runs on as the
   server's. Nothing explained it, which is counted when the next word comes. A record that came
   before the word explains it, and so does one of its tick or before that comes after it (the
   word travels first); one of a later tick does not. */
static void test_told_again(void)
{
    static struct Predicted predicted;
    static uint64_t arena[(SERVER_ARENA + 7) / 8];
    const uint64_t T = 0x12a05f200ull;
    struct BodyWorld *server;
    struct BodyInput in;
    struct BodyEvent event;
    struct PredictedBody word;
    unsigned char state[BODY_STATE_SIZE], theirs[BODY_STATE_SIZE];
    int heard;

    /* 0: no record; 1: one before the word; 2: one of its tick after it; 3: one of a later tick after it */
    for (heard = 0; heard <= 3; heard++) {
        server = begin(&predicted, arena, T, 10, 10, 30, 30);
        CHECK(predicted_run(&predicted, &ground, T + 400));
        /* behind the client's back: the server's body is put on another cell */
        in = input(T + 500, BODY_INPUT_PLACE, OWN_SLOT, OWN_ID);
        in.x = 20;
        in.y = 20;
        CHECK(body_run(server, &ground, T + 500) == BODY_OK && body_input(server, &ground, &in) == BODY_OK);
        while (body_take_event(server, &event)) {
        }
        /* the client's world is ahead of the server's word, as the own clock is */
        CHECK(predicted_run(&predicted, &ground, T + 2100));
        CHECK(body_run(server, &ground, T + 2000) == BODY_OK);
        CHECK(predicted_state(&predicted, state) && body_state(server, OWN_SLOT, theirs) == BODY_OK && memcmp(state, theirs, sizeof state) != 0);
        /* told again two seconds after the last word */
        word = word_of(server, 17, 150);
        if (heard == 1)
            predicted_heard(&predicted, T + 1990);
        CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_DIFFERENT);
        CHECK(predicted.unexplained == 0 && predicted.suspect == (heard != 1));
        if (heard >= 2)
            predicted_heard(&predicted, heard == 2 ? T + 2000 : T + 2001);
        CHECK(predicted_run(&predicted, &ground, T + 2200) && body_run(server, &ground, T + 2200) == BODY_OK);
        CHECK(predicted_state(&predicted, state) && body_state(server, OWN_SLOT, theirs) == BODY_OK && memcmp(state, theirs, sizeof state) == 0);
        /* the next word told again is what was predicted */
        CHECK(body_run(server, &ground, T + 4000) == BODY_OK && predicted_run(&predicted, &ground, T + 4100));
        word = word_of(server, 17, 150);
        CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_SAME);
        CHECK(predicted.unexplained == (uint32_t)(heard == 0 || heard == 3) && !predicted.suspect);
    }
}

/* A word is taken only when it is of the placement the player is in: one that comes before
   the number is known waits, the newest of them, and one of another placement is refused,
   in whichever order the words of two maps and the number arrive. */
static void test_placement(void)
{
    static struct Predicted predicted;
    static uint64_t arena[(SERVER_ARENA + 7) / 8];
    struct BodyWorld *server;
    struct PredictedBody old, word, later;
    unsigned char state[BODY_STATE_SIZE];
    const uint64_t T = 0x12a05f200ull;

    memset(cells, 1, sizeof cells);
    server = server_world(arena, T, 10, 10, MOB_ID, 14, 10);
    old = word_of(server, 17, 150);
    old.placement = 4;
    old.tick = T + 900; /* a map's clock may be ahead of the next one's */
    word = word_of(server, 17, 150);
    word.placement = 5;
    later = word;
    later.tick = T + 50;

    /* the number comes last: the newest word of the newest placement waits for it */
    predicted_init(&predicted, WIDTH, HEIGHT);
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_REFUSED && !predicted.live);
    CHECK(predicted_confirm(&predicted, &ground, &old) == PREDICTED_REFUSED && !predicted.live);
    CHECK(predicted_confirm(&predicted, &ground, &later) == PREDICTED_REFUSED && !predicted.live);
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_REFUSED && predicted.waiting.tick == T + 50);
    CHECK(predicted_placed(&predicted, &ground, 5) == PREDICTED_DIFFERENT && predicted.live && predicted.confirmed.tick == T + 50);
    CHECK(predicted_placed(&predicted, &ground, 5) == PREDICTED_REFUSED && !predicted.has_waiting);
    /* a word of the map that was left, late, is no word of this one */
    CHECK(predicted_confirm(&predicted, &ground, &old) == PREDICTED_REFUSED && predicted.confirmed.tick == T + 50);
    CHECK(predicted_confirm(&predicted, &ground, &later) != PREDICTED_REFUSED);

    /* only a word of the map that was left came before the number: nothing is taken */
    predicted_init(&predicted, WIDTH, HEIGHT);
    CHECK(predicted_confirm(&predicted, &ground, &old) == PREDICTED_REFUSED);
    CHECK(predicted_placed(&predicted, &ground, 5) == PREDICTED_REFUSED && !predicted.live && !predicted.has_waiting);
    CHECK(!predicted_state(&predicted, state));
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_DIFFERENT && predicted.live);

    /* the number first: a word of the old placement after it is refused, one of this taken */
    predicted_init(&predicted, WIDTH, HEIGHT);
    CHECK(predicted_placed(&predicted, &ground, 5) == PREDICTED_REFUSED);
    CHECK(predicted_confirm(&predicted, &ground, &old) == PREDICTED_REFUSED && !predicted.live && !predicted.has_waiting);
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_DIFFERENT && predicted.live);

    /* numbers that wrap: the one after 0xFFFFFFFF is newer */
    predicted_init(&predicted, WIDTH, HEIGHT);
    old.placement = 0xFFFFFFFFu;
    word.placement = 0;
    CHECK(predicted_confirm(&predicted, &ground, &word) == PREDICTED_REFUSED);
    CHECK(predicted_confirm(&predicted, &ground, &old) == PREDICTED_REFUSED && predicted.waiting.placement == 0);
    CHECK(predicted_placed(&predicted, &ground, 0) == PREDICTED_DIFFERENT && predicted.confirmed.tick == word.tick);
}

static void test_convergence(void)
{
    static const struct {
        unsigned lag, jitter;
        int clock_error;
    } lines[] = {{0, 0, 0}, {20, 10, 0}, {40, 30, 3}, {90, 60, -4}, {150, 120, 7}};
    struct Outcome total, one;
    uint32_t seed;
    size_t k;
    int without;

    /* without lag, chaos or a clock that is off: the server applies every request at the tick
       the client reckoned, answers yes to everything, and nothing happens that the client
       cannot know. Not one of its words is a correction */
    for (seed = 1; seed <= 6; seed++) {
        one = converge(seed, 0, 0, 0, 0, 100, MOB_ID, 0);
        CHECK(one.requests > 10 && one.different == 0 && one.same > 10 && one.late == 0);
    }
    /* with all of it: in order, and with what the server says overtaking what it said before and
       coming twice, which converges on the same world */
    memset(&total, 0, sizeof total);
    without = failures;
    for (k = 0; k < 2 * (sizeof lines / sizeof lines[0]); k++) {
        size_t line = k % (sizeof lines / sizeof lines[0]);
        int disorder = k >= sizeof lines / sizeof lines[0];

        for (seed = 1; seed <= 24; seed++) {
            /* some runs start with numbers that wrap in the run, and in some she attacks another
               player, whom the server's core names by her account as her client knows her */
            one = converge(seed * 31u + (uint32_t)line, lines[line].lag, lines[line].jitter, lines[line].clock_error, 1,
                           seed % 4u == 0 ? 0xFFF0 : 1, seed % 3u == 0 ? PLAYER_ACCOUNT : MOB_ID, disorder);
            total.requests += one.requests;
            total.refused += one.refused;
            total.not_yes += one.not_yes;
            total.surprises += one.surprises;
            total.same += one.same;
            total.different += one.different;
            total.compared += one.compared;
            total.late += one.late;
            total.kills_own += one.kills_own;
            total.kills_other += one.kills_other;
            total.returns += one.returns;
            total.overtaken += one.overtaken;
        }
    }
    /* the runs are worth it only if all of that happened in them */
    CHECK(total.requests > 2000 && total.refused > 100 && total.not_yes > 300 && total.surprises > 500);
    CHECK(total.same > 1000 && total.different > 500 && total.late > 100 && total.compared > 20000);
    CHECK(total.kills_own > 15 && total.kills_other > 15 && total.returns > 100 && total.overtaken > 400);
    if (failures != without) {
        fprintf(stderr, "convergence: %d requests, %d refused, %d answers not yes, %d surprises, %d words as predicted, %d corrections, %d applied late, %d ticks compared\n",
                total.requests, total.refused, total.not_yes, total.surprises, total.same, total.different, total.late, total.compared);
        fprintf(stderr, "convergence: %d own swings killed, %d killed a tick before the own swing landed, %d came back, %d overtaken\n",
                total.kills_own, total.kills_other, total.returns, total.overtaken);
    }
}

int main(void)
{
    test_read();
    test_world();
    test_requests();
    test_records();
    test_skills_and_pickups();
    test_gone();
    test_refused();
    test_walk_news();
    test_placement();
    test_way_up_samples();
    test_told_again();
    test_convergence();
    if (failures) {
        fprintf(stderr, "predicted: %d failed\n", failures);
        return 1;
    }
    puts("predicted: ok");
    return 0;
}
