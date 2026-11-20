#ifndef BODY_SCENARIO_H
#define BODY_SCENARIO_H

/* Generated runs of the body core for its tests, freestanding like the core, so the native
 * test and the wasm build run the very same code. A scenario plays the host of a small world
 * on a map with walls: it drives the world to a tick, gives it an input made from a seed (a
 * likely one, a wrong one, or plain random bytes), takes the events and now and then opens or
 * closes a cell. It checks what must always hold (below) and folds every result, every event
 * and, every 256 inputs and at the end, the world's bytes into a digest. The digests of sim/body/body_vectors.h are
 * what both builds must give.
 *
 * Checked after every call into the core:
 *   - the world passes body_world_check;
 *   - a refused input left every byte as it was; an applied one set the clock to its tick;
 *   - the clock did not go back, and events come out in the order of their ticks;
 *   - a step goes to a neighbouring cell that can be walked, in the direction it says, and
 *     does not land while a hold on its body lasts;
 *   - once the events are taken, every body is exactly what its last record says at that tick
 *     (body_move_at): what a viewer plays from the records is where the bodies are;
 *   - no answer of the game takes a body back that is in the middle of a step: the step
 *     still lands, when it was going to (a swing waits for it);
 *   - a body's swings do not overlap, and none begins before its attack delay since the one
 *     before, before the penalty after a cancelled one, or before its act clock; a swing's
 *     impact is asked for only of a swing that is open, and a swing is lost only once;
 *   - a walk is said to be refused only of the request the body took last, which asked for one;
 *   - a body's bytes (body_state), put into a copy of the world, are taken and give the same
 *     bytes again.
 *
 * The scenario answers the core's questions as a game would, mostly with yes. */

#include "body/body.h"

#define BODY_SCENARIO_BODIES 8
#define BODY_SCENARIO_EVENTS 12
#define BODY_SCENARIO_WIDTH 24
#define BODY_SCENARIO_HEIGHT 20

struct BodyScenario {
    uint64_t digest;
    uint64_t last_event_tick;
    uint32_t rng;
    uint32_t failed; /* the number of the input a check failed at, from 1; 0 when none did */
    uint32_t inputs;
    uint32_t results[12];  /* how often body_input gave each BODY_* code */
    uint32_t events[32];   /* how many events of each kind were taken */
    uint32_t intents, intents_ended; /* intents a request gave, and intents ended with a word */
    uint32_t wanted[5];              /* the requests taken that gave an intent, by its kind (BODY_WANTS_*) */
    uint32_t acts[5];                /* the acts asked for, by the intent's kind */
    uint32_t states;                 /* bodies whose bytes were put back */
    uint32_t swings;       /* swings that began or were made */
    uint32_t refused_runs; /* body_run calls that were refused as they should be */
    /* what the scenario knows of each slot from the events alone */
    struct BodyMove record[BODY_SCENARIO_BODIES];
    uint64_t hold[BODY_SCENARIO_BODIES]; /* the end of the longest skill's hold put on the slot's body */
    int16_t x[BODY_SCENARIO_BODIES], y[BODY_SCENARIO_BODIES];
    uint8_t known[BODY_SCENARIO_BODIES];   /* `record` is the record of the body in the slot */
    uint8_t located[BODY_SCENARIO_BODIES]; /* (x, y) is a cell an event gave for the slot */
    /* each slot's attack, as the events and the inputs say it */
    uint64_t next_swing[BODY_SCENARIO_BODIES]; /* no swing begins before this */
    uint16_t adelay[BODY_SCENARIO_BODIES];
    uint8_t swing_open[BODY_SCENARIO_BODIES];
    uint8_t swing_lost[BODY_SCENARIO_BODIES]; /* an impact was answered with no hit: the swing is told lost */
    uint8_t casting[BODY_SCENARIO_BODIES];   /* a cast is under way */
    uint8_t free_cast[BODY_SCENARIO_BODIES]; /* it may swing while it casts (BODY_FIGHTS_FREE_CAST) */
    /* the intent a body has, as the inputs and the events said it */
    uint8_t intent[BODY_SCENARIO_BODIES], intent_kind[BODY_SCENARIO_BODIES], intent_mode[BODY_SCENARIO_BODIES];
    uint16_t intent_seq[BODY_SCENARIO_BODIES];
    uint16_t request[BODY_SCENARIO_BODIES];      /* the number the body's last request came with */
    uint8_t walk_asked[BODY_SCENARIO_BODIES];    /* that request asked for a walk */
    /* the questions that wait for the scenario's answer */
    uint32_t ask_id[BODY_SCENARIO_BODIES * 2];
    uint16_t ask_slot[BODY_SCENARIO_BODIES * 2];
    uint16_t ask_seq[BODY_SCENARIO_BODIES * 2]; /* of an intent's question: the intent's number */
    /* 1: may it swing, 2: a hit now, 3: a swing's impact, 4: a cast's end; of an intent 5: what is
       seen, 6: stand up, 7: act, 8: walk */
    uint8_t ask_kind[BODY_SCENARIO_BODIES * 2];
    uint32_t ask_count;
    /* the ground: the host's, not the world's */
    uint8_t cells[BODY_SCENARIO_WIDTH * BODY_SCENARIO_HEIGHT];
};

/* The bytes of a scenario's world; `arena` below is this large, aligned to 8. */
size_t body_scenario_arena_size(void);

/* The bytes of working memory a scenario needs; `work` below is this large, aligned to 8. It
 * carries nothing from call to call. */
size_t body_scenario_work_size(void);

/* Starts a scenario: an empty world in `arena` with its clock at start_tick, and a map. Returns
 * 0 when the world cannot be made. */
int body_scenario_begin(struct BodyScenario *scenario, void *arena, uint32_t seed, uint64_t start_tick);

/* Gives the world `inputs` more inputs. `arena` may be a copy of the bytes the last call left. */
void body_scenario_run(struct BodyScenario *scenario, void *arena, void *work, uint32_t inputs);

/* The digest of the run, the ground and the world as it is now; 0 when a check failed. */
uint64_t body_scenario_end(const struct BodyScenario *scenario, const void *arena);

/* All three in one call, for a host that cannot hold the struct (the wasm test). */
uint64_t body_scenario(uint32_t seed, uint64_t start_tick, uint32_t inputs, void *arena, void *work);

#endif
