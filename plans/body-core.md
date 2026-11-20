# Actor core

Build step 5 of `plans/core-loop.md`, redesigned on 2026-11-10: the rules for where an actor is
and what it does next become one freestanding C library, the actor core. The server runs it as
the authority and this client runs the same source to predict its own character. Status,
2026-11-10: the frame (5a) and walking (5b) are built on the server, and spatial runs its
walks through the core; it has not been played. The client has the core's source and asks
it where and when its own walk starts. The owner gets move records of its own character
and the client follows them; not played. The basic attack's loop is in the core too: the
server's combat runs its players' basic attacks through it, and the skill code's casts and
its act clock with them (5c), and keeps their intents there (5d); not played. None of this
is a setting of the server: it is how the server works. The client shows its own swing at
the click when the core says one begins; not seen on screen.
Since 2026-11-12 (rules version 10) a player's walk and order to attack go into the core as
one request each, the core sees for an order's intent itself, and the owner is told its
body (`ZC_OWN_BODY`), from which the client keeps a
predicted world without drawing from it yet: `plans/predicted-world.md` has that plan and
what is built of it.
See "What is built".

## Why

Patrick, 2026-11-10: steps 1 to 4 were fitted into code that was there. He asked whether the
system should be written from the ground up as a clean island, a pure simulation with tests,
that the rest calls into.

What the retrofit left:

- **The rules have several homes.** On the server, "when may this actor act again" is three
  clocks set in a dozen places: `ud.canactTick` (11 assignments in `skill-use.js`,
  `companion-skill.js` and `skill-wiring.js`), `state.skillDelayUntilMs` (the older skill
  handlers in `eos-combat/module.js`) and `ud.attackableTime` (`unit-attack.js`). The pose hold
  of step 4 first counted from the wrong moment for skills with a cast time, because the rule
  had to be added at several of them.
- **Walking and acting were in different modules.** Spatial owned the walk (the fork's walk
  in JS, and chase walks of its own), combat owned the act, and the intent step, in JS in
  combat, asked both through a world of callbacks.
- **The wire is a translation.** `eos-world/aoi-render.js` makes records out of the classic
  events (walk started, cast started, swing begins). Each record kind needed its own mapping
  and its own gaps: a swing seen from its middle has no record, the own character gets none.
- **The client's copy of a rule is checked, not shared.** `src/world/grid_path.c` is the
  server's path search again in C, held to the server's JS by vectors. The list of skills
  without a pose is copied by hand into the server. Predicting the own acts would add a third
  copy, of the intent step.

Where it is going anyway: the server's `docs/golden-backend-transition.md`, section 8, wants
the whole simulation as a freestanding pure function over one arena, in C, integer math, with
the journal as input. This is the first unit of that, chosen because it is the part both
sides need identical.

## What a body is

A body is an actor as far as time and place go: a player, a monster, a homunculus, an NPC that
walks. The core owns three things about it and nothing else.

| Part | State | Today |
|---|---|---|
| Move | Cell, facing, the steps of its walk and which one is under way, the tick that step lands, walk speed, the tick it may next move | Spatial's `walkers`, `movement-delay.js`, `ud.canmoveTick` |
| Act | What it does (idle, swing, cast, pose), from when to when, the impact tick and reach of an open swing, the three clocks as one rule | `ud` in `unit-attack.js` and `skill-use.js`, `state.skillDelayUntilMs`, the pose hold |
| Intent | The one thing its owner wants (walk, attack, skill on an actor or a cell, pickup), its number, its phase, its walk tries | `state.intent` in `intents.js`; a walk is not an intent yet |

**The state of the first two parts is the record.** A move record is the body's move state
written out, and an act record its act state. The server's renderer copies them out when they
change, and a client puts them into a body it shows. There is no mapping between a rule's state
and its packet, so a body seen from the middle of anything has a record.

Not in the core, and asked of the host when a rule needs it:

- HP, SP, statuses, damage, what a skill does and costs, who may hit whom.
- A skill's range, cast time and delay. They come from stats and statuses.
- Monster AI. It is a source of requests, like a player's client.
- Who sees whom, and packets.

## Shape

- **Freestanding C11.** `<stdint.h>` and `<stddef.h>` only. No allocation, no globals, no
  clock, no library call, integer math only. `grid_path.c` is written this way already and
  moves in.
- **One arena.** The caller hands in a block of memory. It holds the world's header, a fixed
  array of body slots and a fixed ring of events. Nothing in it is a pointer, only slot
  numbers, and every field has a fixed width, so the same bytes mean the same on every build. A
  snapshot is a copy of the block.
- **Cells are read, not owned.** The walkable grid is the caller's, as `struct GridPathMap` is
  today. When cells change, the caller says which.
- **Ticks** are 64-bit milliseconds inside the core and never wrap. The wire's 32-bit ticks
  (rule 2 of the core loop) are made from them where a record is rendered, and widened again
  where one is read. Decided while building 5a: with 32 bits, a body that stands for 25 days
  has a tick the clock can no longer be compared with.
- **Outside data.** Every input is checked against the arena and refused with a code when it
  does not fit: a slot out of range, a cell off the map, a tick in the past. The core never
  asserts on an input.

### Inputs

Each input is a fixed-size record with the tick it applies at. The server's journal and the
hold-back decide that tick before the core sees the request; the core does not know about
latency.

| Input | From |
|---|---|
| Add a body at a cell, remove it | Spawn, map change, logout |
| Stats: walk speed, attack delay and motion, attack range | Status calculation |
| Condition: dead, sitting, cannot move, cannot act | Statuses, death, sit and stand |
| Request: walk to a cell, attack, skill on an actor or a cell, pickup, release, cancel; with its number | A player's packet, a monster's AI |
| Answer to a question (below) | Combat |
| Place: this cell now, as walked, pushed or placed | Knockback, slide, warp |
| Delay: may not move, or may not act, before this tick | A hit's flinch, a skill's delay |
| Cells changed | Ice wall, another body's cell where that blocks |

### Time

The core has no timers. Every body carries the ticks its next things are due: the step under
way lands, a swing lands, a cast ends, a clock runs out, the intent looks again. The caller
drives it:

```c
while (actor_next_due(world, &tick) && tick_not_after(tick, frame_time)) {
    actor_run(world, cells, tick);          /* everything due at exactly this tick */
    while (actor_take_event(world, &event))
        react(&event);                     /* may put inputs in at this tick */
}
```

- Everything due at a tick runs before an input of that tick, as the server's timers do today
  (`shared/src/sim/timers.js`).
- Things due at the same tick run in slot order, and within a body in a fixed order: move,
  act, intent. This is the core's own order. Today's is the order the timers were made in.
- A step lands on its own tick, whatever the frame. The sweep that made a chase walk's 150 ms
  step take 160 goes whether tracks are on or not.

### Events and questions

| Event | The host does |
|---|---|
| Move changed, act changed | Renders the body's record to its viewers |
| Intent changed: held, under way, acting, ended with a reason | Renders it to the owner |
| Clocks changed | Renders them to the owner |
| Entered a cell | Area of interest, NPC touch, traps: what `onStep` does today |
| Question: may this act begin? | Answers yes with what the act takes (cast time, delay, walk delay, whether it has a pose), or no with a reason. Not built |
| Question: may this swing begin? | Answers whether the two may fight, whether the body casts, whether the line of fire is free, whether it has its ammunition. Built |
| Question: the hit is made now | Calculates the hit, applies it, puts in what follows (a death, a flinch), and answers whether a hit was made. Built; the core has checked the reach before it asks |
| A cast ended | Carries the skill out |

A question must be answered before the core runs a later tick; `actor_run` refuses to pass an
open one. The questions are exactly the places where content decides, so they are also exactly
the places where the client's prediction can be wrong.

### Rules that move in

They are the server's present rules, with the fork's leftovers dropped where the project
already chose otherwise.

- **Path:** `grid_path.c` as it is.
- **Walk:** the fork's walk as `spatial` had it in JS, with the exact turn always (a request
  turns the walk at the first step whose tick is not before the request applied), walks to
  within a range and after a unit, the walk delay.
- **Swing:** the fork's attack as `eos-combat/unit-attack.js` has it, with the project's
  windup: attack delay, windup, reach at impact, cancel and its penalty, the repeat. One rule
  for players and monsters.
- **Act clock:** one rule in place of the three clocks. Cast time, the skill's delay and the
  pose hold are terms of it. The pose table (which skills have one) is a constant in the core,
  and this client's `tests/test_skill_poses.c` holds it to the client's code directly. The hand
  copy in the server goes. As built: the cast and the clock are the core's; what a skill puts
  under the clock (its delay or its pose, whichever is longer) is the game's answer at the
  cast's end, so the pose table stayed in the server; and the older skill handlers' delay is
  still a clock of the game's own ("What is built").
- **Intent:** the rule the server's JS keeper of intents had: its step, the 300 ms wait limit,
  the 300 ms between walks, the eight tries. Walking becomes intent kind 0, which step 3 left open.

## Being hit while walking

Patrick asked on 2026-11-10 whether the core covers the client's hit lock and Endure.

What happens today:

- **Server.** A hit with flinch time sends `DELAY_WALK` from eighteen places in
  `eos-combat/module.js`. Spatial pushes a walker's next step back by that time and the walk
  goes on afterwards (`movement-delay.js`). A unit that stands is not held at all: the delay
  is dropped, so it can walk off in the middle of its flinch.
- **Endure.** Outside guild wars and battlegrounds the status calculation gives an Endure
  target a flinch time of 0 (`status-calc.js`, `calcDmotion`), so no delay is sent and the hit
  goes out as the endure kind.
- **Client.** A hit that lands puts its target into the flinch state (`actor_sendmsg.c`, state
  4), which ends a walk on the client. It does not for the endure kinds (types 4 and 9), for a
  dead actor, or for a player in the middle of a swing. With tracks another actor's flinch
  comes from the hit notice and its position from the pause record. The own character gets no
  record yet: a hit takes it out of its walk on the client while the server walks it on after
  the pause. What the client does with the own character when the flinch ends is not traced.

In the core (built, part 5b):

- **A hold is an input:** this body takes no step before this tick. A step under way lands
  when the hold ends, which is a new record, so every viewer shows the pause.
- **A body that stands is held too.** Asked to walk during the hold, it stands until the hold
  ends and then takes its first step. This is the hit lock as the game had it, and new on
  this server.
- **Endure stays combat's decision.** No flinch time, no hold, and the body walks through the
  hit. The core knows nothing of statuses.

Built since: the owner gets the pause record too, and the client
follows it, so the own character walks on after its flinch. Still to do: combat's eighteen
places become the one input (5c).

Open, and Patrick's: a hit pauses a walk and the walk goes on. In the classic game, as far as
I remember Hercules' `unit_set_walkdelay`, a hit ends a player's walk and the player clicks
again. The core can do either; it does what this server does now.

## On the server

- **Where it runs.** The game logic is modules inside the Node host's units, one unit per map,
  each with its own clock, timers and snapshot (`consumers/host/unit.js`). A consumer in another
  language is possible at the gateway (`consumers/c/`), but a separate process cannot take part
  in a unit's frame. So the core runs inside the unit as wasm: one instance per unit, its
  memory the arena, the unit's snapshot a copy of it, a body moved between units by an export
  and an import as a module's entity state is today. Clang with the wasm32 target is already
  what `tools/build_grid_path_wasm.bat` uses. When the host itself becomes C (section 8), the
  same source links in natively.
- **A `body` module** owns the instance. Spatial's walkers and combat's timing state go.
  Spatial keeps the cells, the area of interest and the NPC touch areas; combat keeps
  everything about content and answers the questions. As built it is not a module of its
  own: `spatial/core-walks.js` holds the worlds inside spatial, and combat, which needs the
  same bodies since 5c, calls them directly ("Decided here", 8).
- **The renderer** sends a record when the core says a track changed.
- **Settings**: `action_windup_rate`, `action_cancel_penalty_ms` and `skill_pose_rate` become
  the core's settings, set when the world is made.

## On the client

- The freestanding pieces of steps 1 to 4 (`move_track`, `act_track`, `drawn`,
  `grid_path`) become users of the core's records or move into it. Reading a record off the
  wire stays the client's, bounded as now.
- **The own character is a body in a client-side world**, with the bodies its intent needs (the
  target) put in from their records. A request goes into that world at the tick the server will
  apply it, which its stamp names, and what the core makes of it is filed as predicted records in
  the same queues the server's records use. Drawing does not change.
- **The client answers every question with yes.** For a swing that is the whole prediction:
  the swing starts at the click, and the core gives the impact tick the drawing stretches it
  to (decision 4 of the core loop). For a skill the client does not know the cast time, so
  the predicted cast has no end and shows no bar; the bar comes with the server's record.
  As built for an order to attack (`world/own_attack.c`): the world is made for the one
  question, of the character and its target on the walks they are on, and thrown away; the
  client keeps when the next swing may begin, how the character fought last, and the base and
  the result of the order's last plan.
- **Correction.** The owner gets its own records, and with them the number of the last request
  they include and the clocks (`ZC_OWN_BODY`, in the place planned for `ZC_OWN_CLOCKS`). The
  client puts that state into the own body, applies again the requests with a later number,
  and runs to the own clock. With one intent per player that is at most a request or two. The
  drawn position is steered to the result (rule 6), never set.
- **A different core means no prediction.** The core has a rules version. The server names its
  own at login, and a client whose version differs shows the own character from the server's
  records alone: correct, and one round trip later.

## What can be shown to hold

Not a proof. A pure function over a block of bytes can be checked far harder than code that
calls into a game, and these are the checks, each a test program in C:

1. **Same bytes.** The same arena and inputs give the same arena and events: twice in a row,
   through a snapshot taken at any point, and native against wasm.
2. **Any input.** Random bytes as inputs never leave the arena invalid. `actor_world_check`
   says what valid means: slots in range, a walk's position within its steps, cells on the
   map, due ticks not behind the clock.
3. **Time.** The clock never goes back, events come out in tick order, and a tick's work ends:
   everything that ran is due later or gone.
4. **One act.** A body's acts never overlap. Two swings of a body are at least its attack
   delay apart unless the first was cancelled. No swing starts during a cast.
5. **Walk.** Every step goes to a neighbour that is walkable when the step starts, a diagonal
   one only past two walkable sides, and lands exactly its step time after the one before
   plus any delay. A body does not move before its move clock.
6. **The record is the truth.** Run a world on random inputs, collect the records it gives
   out, and play them by the core loop's rules 3 to 5. At every tick the played cell and act
   are the body's. This is the property the whole design is for.
7. **Prediction.** A client world given the same requests at the same ticks, with every
   answer yes and nothing from outside, gives the server's records exactly. With outside
   inputs and any latency, the client's body equals the server's once the last correction is
   in, and the steered position never jumps unless the gap is over four cells.
8. **Intent.** An intent is in one phase at a time, ends once or is replaced, and cannot live
   on without walking, acting or waiting for a named tick.
9. **Small worlds, all of them.** On a map of a few cells with two bodies, every sequence of
   requests up to a fixed length, checked against 2 to 8. The turn rule is checked this way
   over every phase of a step.
10. **The wrap.** The core has none. The vectors run across 2^32 and 2^63 to show it, and
    the two places that turn a tick into 32 bits and back are tested where they are made.

While a rule moved, its JS was the oracle: for generated sessions the core and the JS walk,
attack and keeper of intents gave the same events. The JS of a player's walk, attack and
intent is gone; its answers stay as digests in the server's `consumers/body/*-recorded.test.mjs`
and as recorded lines of whole sessions (`consumers/host/recorded`), as
the piece `rules`' `tests/data/grid_path_vectors.txt` does for the path search.

## Build order

Each part ends with both suites passing and the JS it replaces deleted.

| Part | Server | Client |
|---|---|---|
| 5a Frame | The arena, the clock, inputs for adding, placing and removing, the checker, the native and wasm builds, checks 1 to 3 and 10 | Nothing: a copy without a user would be dead code |
| 5b Walk | In the core: walks, turns, holds, stops, speed (built), then chase and range walks. In the server: the `body` module, spatial's walkers go. Checks 5, 6 for moves, 9 | Takes the copy and its check; `move_track` reads the core's record; the own walk is predicted by the core; the owner gets its own move records and `ZC_OWN_BODY` |
| 5c Act | Swings and the act clock in the core; combat answers questions; the three clocks and the pose copy go. Check 4, 6 for acts | The own swing starts at the click with the core's impact tick |
| 5d Intent | The intent step in the core, walking as an intent. Check 8 | Held, under way and acting are shown from the predicted intent; check 7 |
| 5e Monsters | Monster AI puts in requests and no longer walks and swings through its own code | Nothing |

What steps 1 to 4 left open closes on the way: knockbacks, slides and warps become move
records in 5b (the place input), a swing seen from its middle and the pose as a record in 5c.
Skill hits as notices are content and stay a step of their own.

## What is built

On the server (ring-buffer worktree `local\worktrees\action-timing`), not committed. Its
`docs/body-core.md` has the layout and how to build.

| Part | Built | Not built or not covered |
|---|---|---|
| Requests, `ZC_OWN_BODY`, the predicted world | Rules version 10, the server's `ZC_OWN_BODY` and the client's `prediction/predicted.c`: see `plans/predicted-world.md`, "What is built" | Nothing is drawn from the predicted world; the rows below on the client's present predictors still say what is drawn |
| 5a Frame | `rules`' `actor/actor.c`: the world in an arena of fixed-width fields, a 64-bit clock, add, place and remove, the move-changed event, `actor_world_check`. Native tests in the server's `make check`; the same core as wasm gives the same digests | Nothing calls the core. No random input in the scenarios happens to be applied with a far-off tick |
| 5b Walk, in the client | The client builds the core the server builds; the path search moved into it. The core's own tests run in the client's suite and give the server's digests. `world/own_walk.c` puts the player's character into a world of one body and asks the core where and when a requested walk starts; `player_predict_walk` uses that and has no rule of its own left. `actor_put_move` was added to the core for it | No `ZC_OWN_BODY` and no rules version sent. The client's `move_track` still has its own record type. `player_predict_walk` has no test of its own. Not played |
| 5b Own records | The owner gets `ZC_TRACK_MOVE` for its character's walks, in the place of `ZC_NOTIFY_PLAYERMOVE`, and for a delayed walk. The client follows such a record as it follows the classic walk start, plays it on the own clock like any actor's record, and turns it for a request that still waits, asked of the core on the walk as the server has it. A held first step counts as a step under way | Not played. A stop is still the classic position fix. Holds are not predicted: a held start is learned from the record. The client's handling (`player_own_move_record`, playing a record on the player) has no test |
| 5d Intent | In the core (rules version 7): the intent's step and keeper: set, clear, release, run, swung, walk failed; the step every 100 ms, the 300 ms wait limit, the 300 ms between walks, the eight tries; phases told on change; four questions the game answers (what it sees, stand up, act, walk). Held to digests of what the server's JS keeper of intents said: six runs of 4,000 inputs, the same asked and told in the same order, the same next step. In the server `eos-combat/core-intents.js` keeps a player's intent there with the game's world; the sessions with attack and skill intents are held against recorded lines, and a restore with an intent under way replays | Walking is not an intent (kind 0): a walk request is the old packet and ends an intent. What an intent sees is all the game's answer, also what the world could see itself (a walk under way, a swing open). The client shows nothing of a predicted intent (held, under way, acting): check 7 is not built. Monsters have none |
| 5c Casts and the act clock | In the core (rules version 6): a cast with its end, at which the core asks and the game answers whether the skill was carried out and what goes under the act clock; its cancel; the clock put there or only later; a clock of the game's own told with the answer to a look. In the server `skill-use.js` keeps a player's `ud->canact_tick` and cast timer in the core and carries the skill out when the core asks. Sessions of an acolyte who casts, attacks around casts, is interrupted by a hit and warps mid-cast are held against recorded lines; a restore in the middle of a cast replays. Twenty deliberate faults in the core's rules were each caught. Found and fixed on the way: a cast of the skill system survived a warp and was carried out where the player arrived | What a skill takes (cast time, delay, pose, cooldown, whether a hit interrupts it) stays the game's answer; the pose table is not in the core. The older skill handlers' delay is still a clock of combat's own, told at each look. The eighteen `DELAY_WALK` places are where they were. Companions' and monsters' casts are not the core's. Whether a cast forbids a swing is the game's answer, since Free Cast is content; the server's attack has no Free Cast at all, as before. The client predicts no cast |
| 5c Own swing, in the client | `world/own_swing.c`: the book of the swing shown ahead of the server's record. When an order's plan says a swing begins, it is filed as a record at that tick and plays on the own clock, stretched by the time the hit notice takes to come; the server's record of it is not played again; without one it is taken back; one that has not begun when the player gives another order is not to come. Tests of the module and of the packet code | Only the first swing of an order: one the attack delay holds back and an attack's later swings come with the server's records. The act clock and the cancel penalty are not known to the client. No correction of the animation's timing when the record arrives |
| 5d Own attack order, in the client | `world/own_attack.c`: when an order to attack leaves, the core is asked what it makes of the character, in a world of the character, its target and a third body for the question of reach. The two are put in on the walks they are on (`world/own_walk.c`), the server's intent is played by its rules (a look when the order is applied, on every cell the character enters and on the intent's beat of 100 ms; an attack in reach; a walk after the target out of reach, again no sooner than 300 ms and a cell closer each time), and the world is run to the first swing. The plan is the records of the character's walk and that swing, shown through the character's own move track and the act queue. While the order lasts the plan is made again on news of the target or of the character's own walk (`game_mode_own_order`, `order_follow` in `game/packets_act.c`). A target whose walk came as the fork's packet is planned on the server's way between the walk's ends, not the way the client drew. Tests of the module (`tests/test_own_attack.c`) and of the packet code; the headless playground shows no jump and no wrong resting cell of the own character from an order to attack without lag | The target's walk is the one it is on now: what it does later is news. A walk of the target that starts after the plan's base counts as under way from the base. Walls in the line of fire, the act clock and the game's own clocks are not known. Skills and pickups are not planned. No world kept between plans, no `ZC_OWN_BODY`. With lag of 80 ms the playground still shows the own character set onto a cell after hit and run |
| 5c Act, in the server | A player's basic attack is the core's. The attack runs in the worlds spatial keeps; `eos-combat/core-attack.js` puts the requests in, answers the core's questions, and makes the calls the fork's attack makes on the game, in its order. Combat calls the worlds directly (`consumers/body/unit-bodies.js`), not over the unit's bus. The worlds run at their due ticks by a timer of their own. Sessions of a player and a monster in a map host are held against recorded lines: a fight standing, out of a walk, with intents, a dying and a killed target; more tests hold the cancel penalty, a restore from a snapshot in the middle of a fight, and what the core does otherwise than the fork on purpose. The core took what combat still needed (rules version 5): the attack's clock set by the host, once, the forgetting stop, the walking target as part of the answer, an event for a swing without ammunition, a reader of the attack | The act's three clocks stay combat's: the core is told the one that counts at each look, with the stats. No casts, skills or pose hold in the core, and the eighteen `DELAY_WALK` places are where they were. A homunculus's, a mercenary's and a monster's attack are not the core's. `unit-attack.js` stays for them, without the windup. An attack and its attack delay do not cross a map change. No session fights across a wall. The client predicts no swing |
| 5c Act, in the core | The basic attack as inputs and events: fighter stats, attack, attack stop, swing cancel, act clock; the attack delay, the windup and its impact, the penalty after a cancel, far and no line of fire (which starts the core's chase), the turn. What only the game knows the core asks, and waits: may it swing, and was the hit made. Held to digests of what the fork's attack in JS with the project's windup said (`consumers/body/attack-recorded.test.mjs`): six runs of 3,000 inputs, the same doings in the same order and the same tick due next; the comparison found one fault in the core. Fourteen deliberate faults were each caught | Nothing calls it: `eos-combat` still fights with `unit-attack.js`, its three clocks and the pose copy are where they were. No casts, no skills, no pose hold in the core. The client predicts no swing. The comparison leaves out units that walk |
| 5b Chase walks | In the core: a walk to within a range of a cell and a walk after another body, which looks where the other is on every cell it comes onto, stops within the range and tells when the other is gone. In the server spatial's walks to within a range and after a unit go through it; whatever a unit walks after gets a body, kept on its unit's cell | Not played. It is the core's own rule: the walk's way of at most 32 steps, other units not in the way, the unit on its cell when the step lands. Monsters' own walks are still driven by the mob consumer. The scenarios' own checks do not know the range rule; the examples hold it |
| 5b Walk, in the server | `spatial/core-walks.js` runs spatial's walks in the core, one world a map; holds reach it through `DELAY_WALK`, also for a player that stands; cell flags update the world's cells; the worlds' bytes are in the unit's snapshot and state hash. A session with a turn, holds, an ice wall, a speed change, a stop, a warp and a despawn is held against recorded lines, and its journal replays. A unit restored in the middle of a walk goes on the same | Not played. Monsters' own walks are still the mob consumer's. Spatial still owns where a unit is and whether it may move; the core is told when a walk starts. Combat still sends holds from eighteen places. The server needs clang the first time it starts |
| 5b Walk, in the core | Walk, stop, hold and speed as inputs; `actor_next_due` and `actor_run`; the cell and no-path events; `actor_move_at`, which plays a record; `grid_path.c` copied in; the wasm a host loads and its JS loader (`consumers/body/core.js`). Check 5 and check 6 for moves hold after every call of four scenarios of 50,000 inputs with cells opening and closing. Held to digests of what the fork's walk in JS said (`consumers/body/walk-recorded.test.mjs`): about 14,800 steps of 7,650 walks, the same cell, steps left and next landing after every one. Thirteen deliberate faults were each caught | Whether a body may move at all (sitting, dead, a status). Check 9 (every sequence on a small map) is not written. `grid_path.c` is a second copy that nothing compares with the client's. Two rules differ from the JS on purpose and are kept out of the comparison: a hold on a standing body, and a request at the tick a step landed |

## Decided here

Taken on 2026-11-10 under the aim Patrick gave for the core loop. He answered the first two
the same day.

1. **The source has one home**, the piece `rules` of the set `game` (`actor/actor.c` and
   `map/grid_path.c`), which the server and this client both build from the submodule `game`;
   neither keeps a copy, so the two cannot differ for the same pin of `game`.
2. **The client replays its own requests onto the server's state.** The core loop says "no
   rewind and no re-simulation". Patrick: that meant the server. It does not re-simulate or
   rewind, and the only things that make play fair are the press order and the hold-back. So
   this is allowed: one body, the player's own, run forward from a newer start over a request
   or two. Without it a correction would throw away whatever the player asked for in the last
   round trip.
3. **Ties go by slot.** Two things due at the same tick run in slot order. It is the same in
   every replay, and it favours the body that has been on the map longer. If that shows in
   play, the start slot can turn with the tick.
4. **No second implementation.** The server does not keep a JS version of a rule once the
   core has it. The old answers are kept as digests of what the JS said (the server's
   `consumers/body/walk-recorded.test.mjs`, `attack-recorded.test.mjs`,
   `intent-recorded.test.mjs`) and as recorded lines of whole sessions
   (`consumers/host/recorded/`).
5. **A hold counts for a body that stands** ("Being hit while walking"). It will change play
   when the core goes live: a unit hit while standing cannot walk off during its flinch.
6. **A request at the very tick a step landed starts from that cell at once.** The JS walks
   one more cell first, because its sweep ran before the request.
7. **An input waits for what is due.** The core refuses an input while a step at or before
   its tick is due, so the order of a landing and a request at one tick is the same on the
   server and in a client that predicts.
8. **Combat calls the worlds directly** (5c, in the server). "On the server" below says a
   `body` module would own the worlds and the others would talk to it. As built the worlds
   stayed in spatial and combat reaches them through `consumers/body/unit-bodies.js`, by
   calls and not by the unit's bus. The bus hands a message on only after the handler that
   sent it has returned, so an attack's effects would have come in another order than they
   do now, and two sessions could not have been compared byte for byte. It is the one place
   where two modules share state other than by messages, and it is the shape a host in C
   has anyway: one program, the core linked in, the content answering by calls.
9. **With the attack in the core the worlds run at their due ticks**, by a timer of their
   own, and not at the walk sweep. A look of an attack is due at a tick of its own and the
   core runs nothing past a step that is due, so a step lands on its own tick in the sim's
   time too, up to 19 ms sooner than at the sweep. This is what "A step lands on its own
   tick, whatever the frame" above asked for; it comes with the attack in the core.
10. **What the game knows only at the look goes in with the answer**: how the unit fights
   and its act clock. The fork reads both at that moment, so combat puts them in right
   before it answers, and needs no place that tells the core of every change of a status.
   Since the casts moved in, the act clock is the core's own, and the answer carries only a
   clock the game still keeps by itself.
11. **Whether a cast forbids a swing is the game's answer.** "One act" (check 4) said no
   swing starts during a cast. The fork lets a player with Free Cast swing while casting,
   and that is content. So the core holds a swing back by the act clock, which a cast puts
   at its end, and ends an attack for a cast only when the game says the cast forbids it.
12. **A cast's end is a question.** The skill is carried out by the game, and what it puts
   under the clock is not known before. The core asks, and holds back what the body has due
   at that tick, so an attack's look that falls on the cast's end sees the skill's delay.
13. **An intent is the game's to drive and to see for.** The core keeps the intent, runs its
   step by itself every 100 ms and decides by the table; when else the step runs (a cell
   entered, an attack out of reach, a swing begun) the host says, where intents.js was
   called, and everything the step looks at is the game's answer. So the core's intent and
   intents.js can be compared word for word, and combat's own view, with its own lag behind
   spatial, decides as it did. A core that looks at its own bodies for range and walking is
   for when monsters and the client use it.
14. **An intent's questions are its own.** The act an intent asks for is carried out by an
   attack, which asks a question of its own while the intent's is open. So a body can have
   one question of each open, and each has its own answer.

## What this does not do

- Damage, statuses and skill effects stay where they are. Porting them is the rest of
  section 8.
- It does not make other actors predicted. They are drawn from records on the render clock,
  and what they aim at the own character on the own clock (`plans/netcode.md`).
- It does not change a packet that is built, apart from adding `ZC_OWN_BODY`.
