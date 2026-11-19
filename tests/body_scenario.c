#include "body_scenario.h"
#include "body/grid_path.h"

#define CELLS (BODY_SCENARIO_WIDTH * BODY_SCENARIO_HEIGHT)
/* How long after a cancelled swing the next may begin in a scenario's world. */
#define PENALTY 80u

static const int8_t DIR_X[8] = {0, -1, -1, -1, 0, 1, 1, 1};
static const int8_t DIR_Y[8] = {1, 1, 0, -1, -1, -1, 0, 1};

static uint32_t next(struct BodyScenario *scenario)
{
    scenario->rng = scenario->rng * 1664525u + 1013904223u;
    return scenario->rng >> 8; /* the low bits of this generator repeat quickly */
}

static void fold_byte(struct BodyScenario *scenario, unsigned char byte)
{
    scenario->digest = (scenario->digest ^ byte) * 0x100000001b3ull; /* FNV-1a */
}

static void fold(struct BodyScenario *scenario, uint64_t value)
{
    int i;

    for (i = 0; i < 8; i++)
        fold_byte(scenario, (unsigned char)(value >> (8 * i)));
}

static void copy(void *to, const void *from, size_t size)
{
    unsigned char *out = to;
    const unsigned char *in = from;
    size_t i;

    for (i = 0; i < size; i++)
        out[i] = in[i];
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

static size_t aligned(size_t size)
{
    return (size + 7u) & ~(size_t)7u;
}

size_t body_scenario_arena_size(void)
{
    return body_world_size(BODY_SCENARIO_BODIES, BODY_SCENARIO_EVENTS);
}

size_t body_scenario_work_size(void)
{
    return aligned(body_scenario_arena_size()) + BODY_SCRATCH_SIZE;
}

int body_scenario_begin(struct BodyScenario *scenario, void *arena, uint32_t seed, uint64_t start_tick)
{
    unsigned char *bytes = (unsigned char *)scenario;
    size_t i;

    for (i = 0; i < sizeof *scenario; i++)
        bytes[i] = 0;
    scenario->digest = 0xcbf29ce484222325ull;
    scenario->rng = seed;
    /* about one cell in six is a wall */
    for (i = 0; i < CELLS; i++)
        scenario->cells[i] = (uint8_t)(next(scenario) % 6u != 0);
    if (body_world_init(arena, body_scenario_arena_size(), BODY_SCENARIO_BODIES, BODY_SCENARIO_EVENTS,
                        BODY_SCENARIO_WIDTH, BODY_SCENARIO_HEIGHT, start_tick) == NULL)
        return 0;
    body_set_cancel_penalty(arena, PENALTY);
    return 1;
}

/* One input in sixteen is random bytes. The others are near what a host sends: mostly walks,
 * most of them to a cell near where the body was last seen, some to within a range of a cell
 * or after another body, slots and cells a little over their range, an id that is sometimes not
 * the body's, a value that is sometimes too large. */
static void make_input(struct BodyScenario *scenario, uint64_t tick, int keep, struct BodyInput *input)
{
    static const uint16_t kinds[32] = {
        BODY_INPUT_ADD,     BODY_INPUT_ADD,     BODY_INPUT_ADD,         BODY_INPUT_CAST,         BODY_INPUT_REMOVE,
        BODY_INPUT_PLACE,   BODY_INPUT_REQUEST, BODY_INPUT_WALK,        BODY_INPUT_WALK,         BODY_INPUT_REQUEST,
        BODY_INPUT_INTENT,  BODY_INPUT_INTENT,  BODY_INPUT_ATTACK_ONCE, BODY_INPUT_NEAR,         BODY_INPUT_CHASE,
        BODY_INPUT_REQUEST, BODY_INPUT_STOP,    BODY_INPUT_DELAY,       BODY_INPUT_CAST_CANCEL,  BODY_INPUT_SPEED,
        BODY_INPUT_FIGHTER, BODY_INPUT_FIGHTER, BODY_INPUT_FIGHTER,     BODY_INPUT_ATTACK,       BODY_INPUT_ATTACK,
        BODY_INPUT_ATTACK,  BODY_INPUT_ATTACK,  BODY_INPUT_ATTACK_CLOCK, BODY_INPUT_ATTACK_STOP, BODY_INPUT_ATTACK_STOP,
        BODY_INPUT_SWING_CANCEL, BODY_INPUT_ACT_CLOCK,
    };
    unsigned char *bytes = (unsigned char *)input;
    size_t i;

    /* while the events are kept, what the scenario knows of an attack comes from them alone:
       nothing is put in that would change it behind their back */
    if (!keep && (next(scenario) & 15u) == 0) {
        for (i = 0; i < sizeof *input; i++)
            bytes[i] = (unsigned char)next(scenario);
        return;
    }
    input->tick = tick;
    input->kind = kinds[next(scenario) & 31u];
    if (keep && (input->kind == BODY_INPUT_ADD || input->kind == BODY_INPUT_REMOVE || input->kind == BODY_INPUT_FIGHTER
                 || input->kind == BODY_INPUT_ACT_CLOCK || input->kind == BODY_INPUT_ATTACK_CLOCK
                 || input->kind == BODY_INPUT_CAST || input->kind == BODY_INPUT_CAST_CANCEL
                 || input->kind == BODY_INPUT_INTENT || input->kind == BODY_INPUT_REQUEST))
        input->kind = BODY_INPUT_WALK;
    input->slot = (uint16_t)(next(scenario) % (BODY_SCENARIO_BODIES + 1u));
    input->id = 1000u + input->slot + (input->kind != BODY_INPUT_ADD && next(scenario) % 16u == 0);
    input->x = (int16_t)((int)(next(scenario) % (BODY_SCENARIO_WIDTH + 2u)) - 1);
    input->y = (int16_t)((int)(next(scenario) % (BODY_SCENARIO_HEIGHT + 2u)) - 1);
    input->dir = (uint8_t)(next(scenario) % 9u);
    input->cause = (uint8_t)(next(scenario) % 4u);
    /* a chase is after a body in another slot, or now and then after nobody, itself or a wrong id */
    input->target = (uint16_t)(next(scenario) % (BODY_SCENARIO_BODIES + 1u));
    input->target_id = 1000u + input->target + (next(scenario) % 16u == 0);
    input->reserved[0] = input->reserved[1] = 0;
    if ((input->kind == BODY_INPUT_WALK || input->kind == BODY_INPUT_NEAR || input->kind == BODY_INPUT_REQUEST)
        && input->slot < BODY_SCENARIO_BODIES && scenario->located[input->slot]
        && next(scenario) % 4u != 0) {
        input->x = (int16_t)(scenario->x[input->slot] + (int)(next(scenario) % 13u) - 6);
        input->y = (int16_t)(scenario->y[input->slot] + (int)(next(scenario) % 13u) - 6);
    }
    if (input->kind == BODY_INPUT_DELAY)
        input->value = (uint16_t)(next(scenario) % 16u == 0 ? 60000u : next(scenario) % 600u);
    else if (input->kind == BODY_INPUT_WALK)
        input->value = (uint16_t)(next(scenario) % 4u == 0 ? 1u + next(scenario) % 12u : 0u);
    else if (input->kind == BODY_INPUT_NEAR || input->kind == BODY_INPUT_CHASE)
        input->value = (uint16_t)(next(scenario) % 16u == 0 ? 100u : next(scenario) % 5u);
    else if (input->kind == BODY_INPUT_ATTACK)
        input->value = (uint16_t)(next(scenario) % 16u == 0 ? 2u : next(scenario) % 2u);
    else if (input->kind == BODY_INPUT_ATTACK_STOP)
        input->value = (uint16_t)(next(scenario) % 16u == 0 ? 3u : next(scenario) % 3u);
    else if (input->kind == BODY_INPUT_ATTACK_CLOCK) {
        /* the next swing sooner or later than the attack delay said, with the look or without */
        input->value = (uint16_t)(next(scenario) % 16u == 0 ? 2u : next(scenario) % 2u);
        input->target_id = next(scenario) % 900u;
    }
    else if (input->kind == BODY_INPUT_FIGHTER) {
        /* attack delay, attack motion, range, windup (a third of them swing at once), the kind of
           unit, and its BODY_FIGHTS_* flags */
        input->value = (uint16_t)(150u + next(scenario) % 900u);
        input->x = (int16_t)(next(scenario) % 16u == 0 ? -1 : (int)(next(scenario) % 500u));
        input->y = (int16_t)(next(scenario) % 16u == 0 ? 100 : (int)(next(scenario) % 14u));
        input->target = (uint16_t)(next(scenario) % 3u == 0 ? 0u : next(scenario) % 400u);
        input->dir = (uint8_t)(next(scenario) % 16u == 0 ? BODY_FIGHTER_MONSTER + 1u : next(scenario) % 3u);
        input->cause = (uint8_t)(next(scenario) % 16u == 0 ? 4u : next(scenario) % 4u);
    } else if (input->kind == BODY_INPUT_ACT_CLOCK) {
        input->target_id = next(scenario) % 1200u;
        input->value = (uint16_t)(next(scenario) % 16u == 0 ? 2u : next(scenario) % 2u);
    } else if (input->kind == BODY_INPUT_INTENT) {
        /* mostly the intent goes; otherwise what is none. An answer is the scenario's own to
           give, so one made here is to no question or to the wrong intent */
        input->dir = (uint8_t)(next(scenario) % 4u == 0 ? next(scenario) % 3u : BODY_INTENT_CLEAR);
        input->value = (uint16_t)next(scenario);
        input->target = (uint16_t)next(scenario);
    } else if (input->kind == BODY_INPUT_REQUEST) {
        /* a walk a third of the time, otherwise an attack, a skill on a body (now and then on the
           body itself) or on a cell, a pickup, and now and then any of them: a release, a cancel,
           nothing, or what is none; an attack's mode, a skill's range or a pickup's distance that is
           sometimes none; a number, now and then none */
        if (next(scenario) % 3u == 0)
            input->dir = BODY_REQUEST_WALK;
        else if (next(scenario) % 4u == 0)
            input->dir = (uint8_t)(next(scenario) % 9u);
        else
            input->dir = (uint8_t)(next(scenario) % 2u == 0 ? BODY_REQUEST_ATTACK : BODY_REQUEST_SKILL + next(scenario) % 3u);
        if (input->dir == BODY_REQUEST_SKILL && next(scenario) % 8u == 0) {
            input->target = input->slot;
            input->target_id = input->id;
        }
        if (input->dir == BODY_REQUEST_ATTACK)
            input->cause = (uint8_t)(next(scenario) % 16u == 0 ? 3u : next(scenario) % 3u);
        else
            input->cause = (uint8_t)(next(scenario) % 16u == 0 ? BODY_MAX_RANGE + 1u : next(scenario) % 10u);
        input->value = (uint16_t)(next(scenario) % 8u == 0 ? 0u : next(scenario));
    } else if (input->kind == BODY_INPUT_CAST) {
        /* a cast time (a third of the skills have none) and the floor under the act clock */
        input->target_id = next(scenario) % 3u == 0 ? 0u : 50u + next(scenario) % 900u;
        input->value = (uint16_t)(next(scenario) % 600u);
    } else
        input->value = (uint16_t)(next(scenario) % 16u == 0 ? 20000u : 50u + next(scenario) % 400u);
}

/* Takes every waiting event and checks it against what the scenario knew of its body. `fresh`:
 * the events were made by the call just before, so a hold the scenario knows of was in force
 * when a step landed. */
/* `answered`: the events are of answers the scenario just gave, and of nothing else. */
static int take_events(struct BodyScenario *scenario, struct BodyWorld *world, int fresh, int answered)
{
    struct BodyEvent event;
    struct BodyMove was;
    int ok = 1, slot;

    while (ok && body_take_event(world, &event)) {
        slot = event.slot;
        ok = event.tick >= scenario->last_event_tick && slot < BODY_SCENARIO_BODIES;
        scenario->last_event_tick = event.tick;
        scenario->events[event.kind & 31u]++;
        fold(scenario, event.tick);
        fold(scenario, ((uint64_t)event.id << 32) | ((uint64_t)event.kind << 16) | event.slot);
        fold(scenario, ((uint64_t)event.dir << 32) | ((uint64_t)(uint16_t)event.x << 16) | (uint16_t)event.y);
        fold(scenario, ((uint64_t)event.other_id << 32) | ((uint64_t)event.other << 16) | event.value);
        if (!ok)
            break;
        if (event.kind == BODY_EVENT_REFUSED) {
            /* of the request the body took last, which asked for a walk */
            ok = scenario->walk_asked[slot] && event.value == scenario->request[slot] && event.other == 0 && event.other_id == 0;
        } else if (event.kind == BODY_EVENT_CHASE) {
            /* a monster to go after another body, to within a range the core keeps */
            ok = event.other < BODY_SCENARIO_BODIES && event.other != slot && event.value <= BODY_MAX_RANGE;
        } else if (event.kind == BODY_EVENT_UNATTACKABLE) {
            ok = event.other == 0 && event.other_id == 0 && event.value == 0;
        } else if (event.kind == BODY_EVENT_ASK_BEGIN) {
            /* a monster's swing is to begin */
            ok = scenario->ask_count < BODY_SCENARIO_BODIES * 2u;
            if (ok) {
                scenario->ask_slot[scenario->ask_count] = event.slot;
                scenario->ask_id[scenario->ask_count] = event.id;
                scenario->ask_seq[scenario->ask_count] = 0;
                scenario->ask_kind[scenario->ask_count++] = 9;
            }
        } else if (event.kind >= BODY_EVENT_ASK_INTENT && event.kind <= BODY_EVENT_INTENT_END) {
            /* of the intent the body has, by its number */
            ok = scenario->intent[slot] && event.other == scenario->intent_seq[slot];
            if (ok && event.kind <= BODY_EVENT_ASK_WALK) {
                ok = scenario->ask_count < BODY_SCENARIO_BODIES * 2u && (event.kind != BODY_EVENT_ASK_WALK || event.value < BODY_INTENT_WALKS);
                if (ok) {
                    scenario->ask_slot[scenario->ask_count] = event.slot;
                    scenario->ask_id[scenario->ask_count] = event.id;
                    scenario->ask_seq[scenario->ask_count] = event.other;
                    scenario->ask_kind[scenario->ask_count++] = (uint8_t)(5 + event.kind - BODY_EVENT_ASK_INTENT);
                }
                /* a skill or a pickup is carried out from a cell the body stands on */
                if (ok && event.kind == BODY_EVENT_ASK_ACT) {
                    scenario->acts[scenario->intent_kind[slot] % 5u]++;
                    if (scenario->intent_kind[slot] != BODY_WANTS_ATTACK && event.tick == body_now(world))
                        ok = body_move(world, event.slot, &was) == BODY_OK && was.steps == 0;
                }
            } else if (ok) {
                /* ended: once, and too early only by more than an intent waits */
                ok = event.value <= BODY_INTENT_REFUSED
                     && (event.value == BODY_INTENT_TOO_EARLY ? event.other_id > BODY_INTENT_WAIT_MS : event.other_id == 0);
                scenario->intent[slot] = 0;
                scenario->intents_ended++;
            }
        } else if (event.kind == BODY_EVENT_ASK_SWING || event.kind == BODY_EVENT_ASK_HIT || event.kind == BODY_EVENT_ASK_CAST) {
            ok = scenario->ask_count < BODY_SCENARIO_BODIES * 2u;
            if (ok) {
                scenario->ask_slot[scenario->ask_count] = event.slot;
                scenario->ask_id[scenario->ask_count] = event.id;
                scenario->ask_seq[scenario->ask_count] = 0;
                scenario->ask_kind[scenario->ask_count++] =
                    (uint8_t)(event.kind == BODY_EVENT_ASK_CAST ? 4 : event.kind == BODY_EVENT_ASK_SWING ? 1 : event.value ? 3 : 2);
            }
            /* a cast that is over was under way, and is none now */
            if (event.kind == BODY_EVENT_ASK_CAST) {
                ok = ok && scenario->casting[slot];
                scenario->casting[slot] = 0;
            }
            /* no swing is made in a cast, but by a body that may swing while it casts */
            if (event.kind == BODY_EVENT_ASK_HIT && !event.value)
                ok = ok && !(scenario->casting[slot] && !scenario->free_cast[slot]);
            /* the impact of a swing that is open closes it; a hit that is made at once has none */
            if (event.kind == BODY_EVENT_ASK_HIT) {
                ok = ok && scenario->swing_open[slot] == (event.value != 0);
                scenario->swing_open[slot] = 0;
            }
        } else if (event.kind == BODY_EVENT_SWING) {
            ok = !scenario->swing_open[slot] && event.tick >= scenario->next_swing[slot]
                 && !(scenario->casting[slot] && !scenario->free_cast[slot]);
            scenario->swing_open[slot] = 1;
            scenario->swings++;
            if (event.tick + scenario->adelay[slot] > scenario->next_swing[slot])
                scenario->next_swing[slot] = event.tick + scenario->adelay[slot];
        } else if (event.kind == BODY_EVENT_SWING_CANCEL) {
            /* of a swing that is open, or of one whose impact made no hit */
            ok = scenario->swing_open[slot] != scenario->swing_lost[slot];
            scenario->swing_open[slot] = 0;
            scenario->swing_lost[slot] = 0;
            scenario->next_swing[slot] = event.tick + PENALTY;
        } else if (event.kind == BODY_EVENT_FACE) {
            /* a body that turned is another record: its state now, as after a move event */
            scenario->known[slot] = (uint8_t)(body_move(world, event.slot, &scenario->record[slot]) == BODY_OK);
        }
        if (event.kind == BODY_EVENT_CELL) {
            ok = scenario->located[slot] && event.dir < 8
                 && event.x == scenario->x[slot] + DIR_X[event.dir] && event.y == scenario->y[slot] + DIR_Y[event.dir]
                 && scenario->cells[event.y * BODY_SCENARIO_WIDTH + event.x] != 0
                 && (!fresh || event.tick >= scenario->hold[slot]);
        } else if (event.kind == BODY_EVENT_MOVE) {
            was = scenario->record[slot];
            if (answered && scenario->known[slot])
                body_move_at(&scenario->record[slot], event.tick, &was);
            /* the body may be gone, or changed again since: then its state is the later one */
            scenario->known[slot] = (uint8_t)(body_move(world, event.slot, &scenario->record[slot]) == BODY_OK);
            /* an answer never takes a body back that has left its cell: the step it has under
               way still lands, when it was going to */
            if (answered && scenario->known[slot] && was.steps > 0
                && event.tick + grid_path_step_ms(was.speed_ms, was.path[0] & 1) > was.at_tick + was.land_ms) {
                const struct BodyMove *now = &scenario->record[slot];

                ok = now->steps > 0 && now->x == was.x && now->y == was.y && now->path[0] == was.path[0]
                     && now->at_tick + now->land_ms == was.at_tick + was.land_ms;
            }
        }
        scenario->x[slot] = event.x;
        scenario->y[slot] = event.y;
        scenario->located[slot] = 1;
    }
    return ok;
}

/* Takes every waiting event and answers the questions among them as a game would, mostly with
 * yes, until none is open. */
static int settle(struct BodyScenario *scenario, struct BodyWorld *world, const struct BodyGround *ground, int fresh)
{
    struct BodyInput answer = {0};
    uint32_t i, count, bits, slot, ms;
    int rounds = 0;

    if (!take_events(scenario, world, fresh, 0))
        return 0;
    while (scenario->ask_count > 0) {
        if (++rounds > 64)
            return 0;
        count = scenario->ask_count;
        for (i = 0; i < count; i++) {
            slot = scenario->ask_slot[i];
            bits = (next(scenario) % 8u != 0 ? BODY_ANSWER_CAN_FIGHT : 0) | (next(scenario) % 16u == 0 ? BODY_ANSWER_CASTING : 0)
                   | (next(scenario) % 8u != 0 ? BODY_ANSWER_LINE : 0) | (next(scenario) % 8u != 0 ? BODY_ANSWER_READY : 0)
                   | (next(scenario) % 8u != 0 ? BODY_ANSWER_HIT : 0) | (next(scenario) % 4u == 0 ? BODY_ANSWER_WALKS : 0)
                   | (next(scenario) % 4u != 0 ? BODY_ANSWER_ACTED : 0);
            /* with a cast's end what goes under the act clock; with a look, now and then, a clock of
               the game's own, which holds no swing (the fork's skill_delay_attack_enable) */
            ms = scenario->ask_kind[i] == 4 ? next(scenario) % 700u : next(scenario) % 8u == 0 ? 1u + next(scenario) % 500u : 0u;
            answer.tick = body_now(world);
            answer.kind = BODY_INPUT_ANSWER;
            answer.slot = (uint16_t)slot;
            answer.id = scenario->ask_id[i];
            answer.value = (uint16_t)bits;
            answer.target_id = ms;
            answer.dir = 0;
            answer.target = 0;
            /* before a monster's swing: mostly it begins, now and then a skill takes its place */
            if (scenario->ask_kind[i] == 9)
                answer.value = (uint16_t)(bits | (next(scenario) % 4u != 0 ? BODY_ANSWER_BEGIN : 0));
            if (scenario->ask_kind[i] >= 5 && scenario->ask_kind[i] <= 8) {
                /* an intent's question, if the intent is still the one that asked: what the game
                   sees, mostly a target that is there, a body that can act and a line of fire,
                   and the target walking now and then; or that it stood up, acted, could walk */
                if (!scenario->intent[slot] || scenario->intent_seq[slot] != scenario->ask_seq[i])
                    continue;
                bits = scenario->ask_kind[i] == 5
                           ? (next(scenario) % 16u != 0 ? BODY_SEES_TARGET : 0) | (next(scenario) % 32u == 0 ? BODY_SEES_CANNOT_ACT : 0)
                                 | (next(scenario) % 16u == 0 ? BODY_SEES_SITTING : 0) | (next(scenario) % 8u == 0 ? BODY_SEES_CASTING : 0)
                                 | (next(scenario) % 8u != 0 ? BODY_SEES_LINE : 0) | (next(scenario) % 4u == 0 ? BODY_SEES_TARGET_WALKS : 0)
                           : (next(scenario) % 8u != 0 ? BODY_INTENT_OK : 0) | (next(scenario) % 4u == 0 ? BODY_INTENT_STUCK : 0);
                ms = next(scenario) % 3u == 0 ? 0u : next(scenario) % 450u;
                answer.kind = BODY_INPUT_INTENT;
                answer.dir = BODY_INTENT_ANSWER;
                answer.value = (uint16_t)bits;
                answer.target = scenario->ask_seq[i];
                answer.target_id = ms;
                if (body_input(world, ground, &answer) != BODY_OK)
                    return 0;
                fold(scenario, ((uint64_t)ms << 16) | bits);
                continue;
            }
            if (body_input(world, ground, &answer) != BODY_OK)
                return 0;
            fold(scenario, ((uint64_t)ms << 16) | bits);
            if (scenario->ask_kind[i] == 2 && (bits & BODY_ANSWER_HIT)) {
                /* a hit that was made at once is a swing */
                if (answer.tick < scenario->next_swing[slot] || (scenario->casting[slot] && !scenario->free_cast[slot]))
                    return 0;
                scenario->swings++;
                scenario->next_swing[slot] = answer.tick + scenario->adelay[slot];
            } else if (scenario->ask_kind[i] == 3 && !(bits & BODY_ANSWER_HIT)) {
                scenario->swing_lost[slot] = 1;
            }
        }
        /* the answers were to the first `count`; what they asked is behind them */
        for (i = count; i < scenario->ask_count; i++) {
            scenario->ask_slot[i - count] = scenario->ask_slot[i];
            scenario->ask_id[i - count] = scenario->ask_id[i];
            scenario->ask_seq[i - count] = scenario->ask_seq[i];
            scenario->ask_kind[i - count] = scenario->ask_kind[i];
        }
        scenario->ask_count -= count;
        if (!take_events(scenario, world, 0, 1))
            return 0;
    }
    return body_asked(world) == 0;
}

/* What a request that was taken did to what the scenario knows of its body: its number, and the
 * intent it ended, made or released. */
static void request_put(struct BodyScenario *scenario, const struct BodyInput *input)
{
    /* the intent each request gives; 0 for none */
    static const uint8_t WANTS[] = {0, 0, BODY_WANTS_ATTACK, 0, 0, BODY_WANTS_SKILL, BODY_WANTS_SKILL_CELL, BODY_WANTS_PICKUP};
    uint32_t slot = input->slot;
    uint8_t wants = input->dir < sizeof WANTS ? WANTS[input->dir] : 0;

    scenario->request[slot] = input->value;
    scenario->walk_asked[slot] = input->dir == BODY_REQUEST_WALK;
    if (input->dir == BODY_REQUEST_WALK || input->dir == BODY_REQUEST_CANCEL)
        scenario->intent[slot] = 0;
    if (wants != 0) {
        scenario->intent[slot] = 1;
        scenario->intent_seq[slot] = input->value;
        scenario->intent_kind[slot] = wants;
        scenario->intent_mode[slot] = wants == BODY_WANTS_ATTACK ? input->cause : 0;
        scenario->intents++;
        scenario->wanted[wants]++;
    } else if (input->dir == BODY_REQUEST_RELEASE && scenario->intent[slot] && scenario->intent_kind[slot] == BODY_WANTS_ATTACK
               && scenario->intent_mode[slot] == BODY_ATTACK_HOLD) {
        scenario->intent_mode[slot] = BODY_ATTACK_ONCE;
    }
}

/* A body's bytes can be put back: in a copy of the world, which has no event waiting and no
 * question open, the body in `slot` takes its own state, says so, and gives the same bytes
 * again. The world itself is left alone. */
static int state_holds(struct BodyScenario *scenario, const struct BodyWorld *world, void *work, uint32_t slot)
{
    unsigned char state[BODY_STATE_SIZE], again[BODY_STATE_SIZE];
    struct BodyWorld *twin = work;
    struct BodyMove move;
    struct BodyEvent event;

    if (body_state(world, slot, state) != BODY_OK)
        return body_move(world, slot, &move) != BODY_OK;
    scenario->states++;
    copy(twin, world, body_scenario_arena_size());
    return body_put_state(twin, slot, state) == BODY_OK && body_take_event(twin, &event) && event.kind == BODY_EVENT_MOVE
           && event.slot == slot && event.tick == body_now(world) && body_state(twin, slot, again) == BODY_OK
           && same(state, again, sizeof state) && body_world_check(twin, body_scenario_arena_size()) == BODY_OK;
}

/* With no event waiting, every body is what its last record says now. */
static int records_hold(struct BodyScenario *scenario, const struct BodyWorld *world)
{
    struct BodyMove now, said;
    uint32_t slot;

    for (slot = 0; slot < BODY_SCENARIO_BODIES; slot++) {
        if (body_move(world, slot, &now) != BODY_OK)
            continue;
        if (!scenario->known[slot])
            return 0;
        body_move_at(&scenario->record[slot], body_now(world), &said);
        if (!same(&now, &said, sizeof now))
            return 0;
    }
    return 1;
}

/* Drives the world to `tick` as a host does. While `keep` the events are left in the ring
 * until it is full. */
static int run_to(struct BodyScenario *scenario, struct BodyWorld *world, const struct BodyGround *ground,
                  uint64_t tick, int keep)
{
    uint64_t due;
    int result;

    while (body_next_due(world, &due) && due <= tick) {
        result = body_run(world, ground, due);
        fold(scenario, (uint64_t)result);
        if (body_world_check(world, body_scenario_arena_size()) != BODY_OK || body_now(world) != due)
            return 0;
        if (result != BODY_OK && result != BODY_EVENTS_FULL && result != BODY_ANSWER_FIRST)
            return 0;
        /* a full ring and an open question are taken care of at once; otherwise the events
           wait while they are kept */
        if (result != BODY_OK || !keep) {
            if (!settle(scenario, world, ground, !keep && result != BODY_EVENTS_FULL))
                return 0;
            if (!keep && !records_hold(scenario, world))
                return 0;
        }
    }
    /* nothing is due up to `tick`: the clock goes there */
    if (body_now(world) < tick && body_run(world, ground, tick) != BODY_OK)
        return 0;
    return body_now(world) == tick;
}

/* body_run refuses a tick behind the clock, and one that skips what is due, and changes
 * nothing. */
static int refused_runs(struct BodyScenario *scenario, struct BodyWorld *world, const struct BodyGround *ground,
                        void *before)
{
    size_t size = body_scenario_arena_size();
    uint64_t due;

    copy(before, world, size);
    if (body_now(world) > 0) {
        if (body_run(world, ground, body_now(world) - 1) != BODY_BAD_TICK || !same(before, world, size))
            return 0;
        scenario->refused_runs++;
    }
    if (body_next_due(world, &due)) {
        if (body_run(world, ground, due + 1) != BODY_DUE_FIRST || !same(before, world, size))
            return 0;
        scenario->refused_runs++;
    }
    return 1;
}

void body_scenario_run(struct BodyScenario *scenario, void *arena, void *work, uint32_t inputs)
{
    struct BodyWorld *world = arena;
    size_t size = body_scenario_arena_size();
    struct BodyGround ground;
    struct BodyInput input = {0};
    uint64_t before, tick;
    int result = BODY_BAD_KIND, ok, keep;

    ground.walkable = scenario->cells;
    ground.scratch = (unsigned char *)work + aligned(size);
    while (inputs-- > 0 && !scenario->failed) {
        scenario->inputs++;
        /* by turns the events are kept for 128 inputs, so that the ring fills and refuses, and
         * taken after every call */
        keep = (scenario->inputs >> 7) & 1u;
        before = body_now(world);
        tick = before + next(scenario) % 50u;
        /* what a time of keeping left */
        ok = keep || settle(scenario, world, &ground, 0);
        if (ok && next(scenario) % 32u == 0)
            ok = refused_runs(scenario, world, &ground, work);
        /* one time in sixteen the host forgets to run what is due first */
        if (ok && next(scenario) % 16u != 0)
            ok = run_to(scenario, world, &ground, tick, keep);
        if (ok) {
            if (next(scenario) % 16u == 0 && body_now(world) > 0)
                tick = body_now(world) - 1; /* behind the clock */
            make_input(scenario, tick, keep, &input);
            copy(work, arena, size);
            before = body_now(world);
            result = body_input(world, &ground, &input);
            fold(scenario, (uint64_t)result);
            scenario->results[result % 12]++;
            ok = body_world_check(arena, size) == BODY_OK;
            if (result != BODY_OK) {
                ok = ok && same(work, arena, size);
            } else {
                ok = ok && body_now(world) == input.tick && input.tick >= before;
                if (input.kind == BODY_INPUT_ADD || input.kind == BODY_INPUT_REMOVE) {
                    /* a new body: nothing is known of its attack, and nothing holds it */
                    scenario->hold[input.slot] = 0;
                    scenario->next_swing[input.slot] = 0;
                    scenario->adelay[input.slot] = 0;
                    scenario->swing_open[input.slot] = 0;
                    scenario->swing_lost[input.slot] = 0;
                    scenario->casting[input.slot] = 0;
                    scenario->free_cast[input.slot] = 0;
                    scenario->intent[input.slot] = 0;
                    scenario->request[input.slot] = 0;
                    scenario->walk_asked[input.slot] = 0;
                } else if (input.kind == BODY_INPUT_INTENT) {
                    /* taken, it was a clear */
                    scenario->intent[input.slot] = 0;
                } else if (input.kind == BODY_INPUT_REQUEST) {
                    request_put(scenario, &input);
                } else if (input.kind == BODY_INPUT_DELAY && input.tick + input.value > scenario->hold[input.slot]) {
                    scenario->hold[input.slot] = input.tick + input.value;
                } else if (input.kind == BODY_INPUT_FIGHTER) {
                    scenario->adelay[input.slot] = input.value;
                    scenario->free_cast[input.slot] = (input.cause & BODY_FIGHTS_FREE_CAST) != 0;
                } else if (input.kind == BODY_INPUT_CAST) {
                    if (input.target_id > 0)
                        scenario->casting[input.slot] = 1;
                } else if (input.kind == BODY_INPUT_CAST_CANCEL) {
                    scenario->casting[input.slot] = 0;
                } else if (input.kind == BODY_INPUT_ATTACK_CLOCK) {
                    /* the attack delay's end is what the host says, sooner or later */
                    scenario->next_swing[input.slot] = input.tick + input.target_id;
                }
            }
        }
        /* an input that asks is answered at once, also while the events are kept */
        if (ok && (!keep || body_asked(world) > 0))
            ok = settle(scenario, world, &ground, 0) && (keep || records_hold(scenario, world));
        /* and with nothing waiting, a body is its bytes */
        if (ok && !keep)
            ok = state_holds(scenario, world, work, next(scenario) % BODY_SCENARIO_BODIES);
        /* a cell opens or closes, only while no event waits: an event's cell is as it was. One
         * in six ends up closed, as when the map was made. */
        if (ok && !keep && next(scenario) % 3u == 0)
            scenario->cells[next(scenario) % CELLS] = (uint8_t)(next(scenario) % 6u != 0);
        /* and now and then the cell a walk was just sent to closes behind the request */
        if (ok && !keep && result == BODY_OK && input.kind == BODY_INPUT_WALK && next(scenario) % 8u == 0)
            scenario->cells[input.y * BODY_SCENARIO_WIDTH + input.x] = 0;
        if (!ok || body_world_check(arena, size) != BODY_OK)
            scenario->failed = scenario->inputs;
        /* the world's bytes go into the digest on the way too, not only at the end: a state
           that is wrong for a while and then written over shows */
        if ((scenario->inputs & 255u) == 0) {
            const unsigned char *bytes = arena;
            size_t i;

            for (i = 0; i < size; i++)
                fold_byte(scenario, bytes[i]);
        }
    }
}

uint64_t body_scenario_end(const struct BodyScenario *scenario, const void *arena)
{
    struct BodyScenario end = *scenario;
    const unsigned char *bytes = arena;
    size_t size = body_scenario_arena_size(), i;

    if (scenario->failed)
        return 0;
    for (i = 0; i < CELLS; i++)
        fold_byte(&end, scenario->cells[i]);
    for (i = 0; i < size; i++)
        fold_byte(&end, bytes[i]);
    return end.digest != 0 ? end.digest : 1;
}

uint64_t body_scenario(uint32_t seed, uint64_t start_tick, uint32_t inputs, void *arena, void *work)
{
    struct BodyScenario scenario;

    if (!body_scenario_begin(&scenario, arena, seed, start_tick))
        return 0;
    body_scenario_run(&scenario, arena, work, inputs);
    return body_scenario_end(&scenario, arena);
}
