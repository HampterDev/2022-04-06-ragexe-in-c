# Netcode

How this client (`custom`) and the ring-buffer server keep time, order inputs and predict, built
on decisions already taken. Status, 2026-11-10: built on both sides, passing their test suites,
and run client against server for login, map entry and walking. See "What is built" and "The
first live run" at the end. Since 2026-11-12 this is the only way the two sides work (`plans/predicted-world.md`, step 3a):
neither has a switch for it, the server announces no features, and the client plays only on a
server that speaks it.

## Where the decisions come from

- **Netcode fairness decisions, 2026-10-31.** The session in `a_new_beginning` that ended with
  the written verdict. It was drafted for that repo's engine; the rules carry over.
- **`herc-client-integration/plans/proceed-rework.md` and `responsive-combat.md`, 2026-10-30.**
  Server-owned intents and hit points, written against HampterHercules.
- **`ring-buffer/docs/golden-backend-transition.md`, section 8.** The simulation as freestanding
  code that also runs in the client for prediction.

## The decisions, restated for this stack

1. **The server never rewinds or re-simulates.** It resolves a little behind real time
   (hold-back) and sorts what arrived by when it was pressed.
2. **An input carries two stamps:** the press tick (when the player pressed, on the server's
   clock) and the seen tick (the world state on screen at that moment). The server records
   arrival and only accepts a press tick inside `[arrival - limit, arrival]`.
3. **Contested actions resolve in press-tick order.** Uncontested ones apply on arrival. Combat
   hold-back is at most 100 ms.
4. **The client's clock is synchronized continuously, NTP style:** four timestamps per exchange,
   the lowest round trips trusted, the clock slewed. It never runs backward, and it jumps
   forward only when it is more than a second behind or its first eight samples are still
   coming in.
5. **Prediction is for the player's own character only.** Others are drawn from what the server
   said. Wrong results are never shown and then taken back.
6. **Game rules that both sides run are one piece of freestanding code,** tested alone.

## What exists today

| | Client | Server |
|---|---|---|
| Clock | Server tick copied on receipt, 72 ms behind, pushed forward by move packets (`session_sync_server_time`). No latency in the estimate. | Logical millisecond clock, 20 ms frames, inputs stamped with their arrival inside the frame. |
| Time packets | `CZ_REQUEST_TIME2` (0x0360) every 12 s with the client's tick. | Answers `ZC_NOTIFY_TIME` with its tick. The client's tick is not echoed. |
| Input order | Sent when the client's own gates allow. | Applied in arrival order. |
| Own walk | Starts when the server's answer arrives. A half-ping local start exists (`player.c`, message 9) but nothing arms it. | Applies a new target at the next cell boundary; no answer when it refuses. |
| Paths | `world/pathfinder.c`, the original's. | `spatial/hercules-path.js`, the fork's. Not checked against each other. |

## Build order

Each step is usable alone and is the base of the next.

### 0. Latency rig

A setting in the client that holds every packet for a fixed time each way, so 100 ms and 300 ms
round trips can be played on one machine. Nothing below can be felt or checked without it, and
the action-timing stage needs it too.

### 1. Clock sync

- New server packet `ZC_TIME_SYNC` (0x0F12, 14 bytes): the client's tick echoed from
  `CZ_REQUEST_TIME2`, the server's receive tick, the server's send tick. No request packet
  changes, so the gateway is untouched.
- Client: a freestanding estimator (offset and round trip from the four stamps, the best of the
  last eight samples, slewed). The request goes out every 250 ms until eight samples are in,
  then every 2 s.
- Two clocks from one estimate: the **true clock** for stamps and the own character, and the
  **render clock** for everyone else, a fixed delay behind it. Today's 72 ms plays that role
  without saying so.
- Limit: the server stamps receive and send with its logical clock, so its own output delay
  (up to a frame) leans the estimate by half of that. The gateway could stamp real times later.

### 2. Predicted own walking

- The own character starts walking at the click, on the true clock plus the upload time, so it
  runs where the server will have it.
- The server's answer replaces the predicted start. A refusal needs a new answer packet, so a
  predicted walk the server never started is undone at once and not by timeout.
- Walk requests get a sequence number (new request packet, so the gateway table is regenerated
  and the gateway rebuilt).
- Needs step 4's path agreement, or the correction at the answer is visible.

### 3. Stamped inputs and hold-back

- Move, attack and skill requests carry press tick and seen tick.
- The server's sequencer holds contested inputs up to the hold-back and applies them in
  press-tick order. The journal records the applied order, so replay is unchanged.
- This is the step that touches the server's frame loop and its parity gate.

### 4. One implementation of shared rules

- First candidates: the path search, the walk timing, the intent step of `proceed-rework.md`.
- Freestanding C, compiled into the client and to wasm for the server, with tests that the two
  builds agree. Until a rule is ported, a shared set of test vectors proves the C and the
  JavaScript give the same answers.

## Protocol

All new packets are in the project's id range, little-endian and packed. Nothing is announced
or switched: the server always answers the timed exchange, reads stamps and intents and holds
requests back, and the client always uses them.

`ZC_HOLD_BACK` 0x0F14, 8 bytes, in answer to each `CZ_NOTIFY_ACTORINIT` (0x007D) of a player:
the one number of the server's the client reckons with, and the number of the player's
placement on the map it is on, which every body of that map carries (`plans/presence.md`,
part 5). The client forgets both each time it loads a map and sends that packet when it has.

| off | size | field | note |
|---|---|---|---|
| 0 | 2 | id | 0x0F14 |
| 2 | 2 | hold_back_ms | the least time a request waits after its press on this map: the own clock leads the server's by at least this; 0 on an ordinary map unless the server sets it, 60 on one where players fight each other |
| 4 | 4 | placement | the player's placements on a map, counted by the server: one at login, one at each warp |

`ZC_TIME_SYNC` 0x0F12, 14 bytes, after each `ZC_NOTIFY_TIME`

| off | size | field | note |
|---|---|---|---|
| 0 | 2 | id | 0x0F12 |
| 2 | 4 | client_tick | echoed from `CZ_REQUEST_TIME2` |
| 6 | 4 | server_receive | server tick when the request was applied |
| 10 | 4 | server_send | server tick when the answer was made |

`CZ_INPUT_STAMP` 0x0F01, 12 bytes, sent directly before the request it stamps

| off | size | field | note |
|---|---|---|---|
| 0 | 2 | id | 0x0F01 |
| 2 | 2 | seq | counts up per stamp, wraps |
| 4 | 4 | tick | the server's tick to apply the request at: the own clock at the press |
| 8 | 4 | seen_tick | the render clock at the press |

It stamps the next packet of the connection if that is a move (0x035F), an attack or sit/stand
(0x0437), a stop of the attack (0x0118), a skill (0x0438, 0x0B10, 0x0366, 0x0367), a pickup
(0x0362) or an intent (0x0F02). Before anything else it is dropped.

The server applies a stamped request at the tick its stamp names when it arrives before that
tick and the tick lies no more than `hold_limit_ms` (250 unless set) after its arrival, and on
arrival otherwise. Requests of one tick apply in the order they arrived, the held ones before
one that arrives at that very tick. Unstamped requests apply on arrival. The answer to a
stamped request is the player's body (`ZC_OWN_BODY`, `plans/predicted-world.md`) with the
stamp's number.

The client's clocks: the **true clock** is the local clock plus the synced offset. The **render
clock** is today's `session_get_server_time`. The **own clock** is when a request sent now
arrives at the server, but no sooner than the true clock plus the hold-back. The way a request
takes is measured by the timed exchanges: each says the server's clock at the request's arrival
less the local clock at its sending, which holds the offset and the way up in one number. Of the
last sixteen the second longest is taken, so that one held up on the way does not set it, and
half its height above the shortest is added as a margin from their spread (`std/way_up.c`).
The lead that gives grows at once and shrinks a millisecond in ten, so the own clock never runs
backward. The own character is drawn on the own clock, and a request names it as the tick to
apply it at, so the client puts the request into its world at the tick the server applies it.
Only a request that took longer than the client allowed for is applied elsewhere, on arrival,
and the player's body says so.

What another actor aims at the own character is played on the own clock too
(`view/act_track.h`): a swing or a cast at it, a hit on it, and the idle record that ends such
a swing or cast. The server decides a swing's hit at the tick the swing lands at, and applies a
request at the tick it names, so on the own clock the swing lands on the screen at the last
moment a request sent now is applied before it. On the render clock it landed there later by
the own clock's lead and the render delay, a tenth of a second and more in which the screen
showed a hit still to come that nothing sent could avoid. The record of such a swing is here
only after its tick on the own clock, by the way down and the lead, so the swing begins at once.
Its damage number comes with the server's notice, a way down and the lead after the landing. Where another actor stands, and everything
it does that is not aimed at the own character, stays on the render clock. So the screen can
still have an actor on the last of a walk the server has ended when its swing at the own
character is due: the swing, or the cast, waits until the actor is drawn where it stands, no
longer than until the render clock reaches the swing's tick, and is fit into the time it has
left then. An actor that walks up is therefore not attacking sooner than it is drawn arriving;
one that stands attacks the render delay sooner.

The attack of every swing the server announces is paced to its landing (`std/swing_pace.c`): the
frame its hit is on shows when the clock the swing is played on reaches the tick the swing lands
at, whatever speed the attack has on its own, at most twice as fast and eight times as slow as
that. So the attack at the own character connects on the screen at the last moment a request
sent now is applied before it, and a slow monster's swing takes on the screen the time the
server has it take. The player's own swing is paced to when its damage number is here, a way
down and the lead after it lands.

## Every confirmed request measures the way up

The own clock leads the server's by the way a request takes there (`std/way_up.c`). Its only
samples are the timed exchanges: one every 2 s, sixteen kept, so the estimate looks back half
a minute and sees a change of the line two seconds late at the earliest. What is left of
misprediction under lag is requests that arrive after the tick they name, most of them in the
first seconds on a map, while few exchanges have been timed (`plans/predicted-world.md`,
step 7).

Each request the server confirms is a sample of the same kind, of the path that matters:

- **Server.** A stamped request's arrival is known where its unit reads it (`arrivalMs` in
  `consumers/host/unit.js`), and only the stamp's number goes on from there. `spatial` keeps
  the arrival beside the number it hands the core, outside the core, which knows nothing of
  latency: `core-walks.js` keeps it on the unit right before the request's number goes to the
  core (a walk in `spatial`, an intent in `eos-combat`). `ZC_OWN_BODY` gains four bytes: the
  low 32 bits of the server's clock at the arrival of the request the body names, or 0 when the
  body names none anew.
- **Client.** Each request that waits in the predicted world keeps the local clock at which it
  left (`PredictedRequest.left`). A word of the server that confirms it with an arrival hands
  the pair back once (`way_new`), and `game/own_actor.c` gives it to `way_up_sample`, as an
  exchange's is. The estimator does not change: the second longest of the last sixteen and half its
  height above the shortest, growing at once and shrinking a millisecond in ten.

While someone plays, the samples then come at the pace of the clicks. A request that lost a
datagram on its way is a sample of that loss, which the second longest keeps from setting the
lead by itself.

Check: `tests/test_way_up.c` with samples from both sources; the playground at a ping of 150 ms
with 40 ms of jitter, where the ticks the predicted body differs for (`plans/predicted-world.md`,
step 7: walking 0.55%, fights 4.29%) must fall, and the requests that arrive after their tick
in the first ten seconds of a map must be fewer than before.

Built on both sides. `tests/test_way_up.c` has samples of both sources: a slower line is followed
two requests after it, where the exchanges alone have not followed it yet, and a request that lost
a datagram does not set the lead. `rules`' `tests/predicted_tests.c`: a word hands back its sample once, only
for the request it names and only with an arrival. The playground at a ping of 150 ms with 40 ms
of jitter, four runs of 40 s with six Porings: requests that arrived after their tick went from
1 (none in the first ten seconds on the map) to 0; with 10 of 100 datagrams lost too, from 3 (2
in the first ten seconds) to 0. The own body differed as predicted 13 and 15 times there before,
9 and 9 after. Not measured: the share of ticks of step 7's sets of `plans/predicted-world.md`.

Not checked: whether the estimate can be kept across a map change. The connection is the same,
but each map's unit has a clock of its own, and a sample holds the clocks' offset.

Open, and the owner's: the stamp's `seen_tick` is sent with every request and only logged. It
is either given a user or taken out of the stamp.

## The hold-back is not enforced (as built)

The server applies a request at the tick its stamp names, or on arrival when that tick is past
(`applyAtMs`, the server's `consumers/shared/src/netcode.js`). The hold-back is a number the
client is told and adds itself. A client that names an earlier tick, or sends no stamp, is
applied on arrival: never before it arrives, so no worse than a server without a hold-back,
but the equal footing holds only between clients that keep to it. It cannot be enforced, since
a client can also pretend a slow line. It can be watched: a connection whose requests keep
arriving at or after the tick they name while its round trip, which the delivery piece
measures, is well under the hold-back is not keeping to it. Not built.

The hold-back is the map's: only where players fight each other does the order of two players'
presses matter, so an ordinary map has none unless the server sets one, and a request there
applies when it arrives. Not built: sizing a contest map's number from the pings of the players
on it, which needs the server to know each connection's way up.

## Where the action-timing stage differs from the October plans

- **Windup length.** `responsive-combat.md` decided 30% of the attack delay (60% of the attack
  motion). The stage as built defaults to 100% of the attack motion. Setting
  `action_windup_rate = 60` matches the decision.
- **Animation fit.** The plan scales the swing so its hit frame lands at the windup. The server
  leaves that to the client, which paces the attack to the tick the swing lands at
  (`std/swing_pace.c`).
- **Skill rate.** The plans let skills run on the server's clocks alone. The pose explainer of
  2026-11-08 shows the client's animation is what keeps Bash at one per swing. That conflict
  is open.
- **Intents.** `proceed-rework.md` moves "walk into range, then act" to the server with
  `CZ_INTENT` and `ZC_INTENT_ENDED`. The stage kept the client's proceed logic and only removed
  its gates during a swing. The plan's design is built since (`plans/intents.md`), and the
  client's proceed logic is gone for everything an intent covers.

## What is built

Client (`custom`, not committed) and server (ring-buffer worktree
`local\worktrees\action-timing`, not committed). The server needs no setting for it; it is
started from that worktree, so that the gateway that knows 0x0F01 is the one running. Two
numbers can be set in the server folder's `config/server.conf`: `hold_back_ms`, the hold-back of
an ordinary map (0 unless set), `hold_back_contest_ms`, that of a map where players fight each
other (60 unless set; a map with the fork's mapflag `pvp`, `gvg`, `gvg_castle`, `gvg_te`,
`gvg_te_castle` or `battleground`), and `hold_limit_ms` (250 unless set). Each map's unit tells
its own hold-back, and the client keeps the number of the map it is on. To play at a ping, set `RAGEXE_LAG_MS` to
half the round trip before starting the client.

| Step | Built | Not built or not covered |
|---|---|---|
| 0 Latency rig | `RAGEXE_LAG_MS`, both directions | No test of its own |
| 1 Clock sync | Estimator, `ZC_TIME_SYNC`, the three clocks | The server stamps with its logical clock, so its output delay is not separated |
| 2 Predicted walk | Start at the click, once the server's clock is known; a turn mid-walk once the server accepted it; refusal; the server's path for walks the player asked for | Server-started walks of the own character begin a round trip into their path and are drawn on the client's own search |
| 3 Stamps and hold-back | `CZ_INPUT_STAMP` before move, attack, skill, pickup, naming the tick to apply it at; applied there, in the order of those ticks, ties in arrival order | Seen tick is only logged; other packets of a connection can overtake its held requests |
| 4 Shared rules | The walk search and step time as one C file, proven equal to the server's code natively and as wasm | The server still runs its JavaScript; the intent step is not written |
| Way up from requests | The arrival of the request a body names anew in `ZC_OWN_BODY`; each waiting request's leaving time in the predicted world; the pair a sample of `way_up_sample` ("Every confirmed request measures the way up") | Not measured: the share of ticks the predicted body differs for in the sets of `plans/predicted-world.md`, step 7 |

Found on the way: this client sends ground skills as 0x0AF4, and no module of the server
handles that id, so ground skills from this client reach nothing today.

## The first live run (2026-11-10)

As it was on that day, when the server still announced its features with `ZC_NET_FEATURES`,
answered a stamped move with `ZC_MOVE_RESULT`, and had a setting for each: those are
`ZC_HOLD_BACK` and `ZC_OWN_INTENT` now, and the settings are gone.

The client against a server started from the worktree, on one machine, with the three settings
above. The server ran in a folder of its own (`local\servers\transport` in the worktree, port
6910) and not in the dev folder, because the worktree's journal is not the main checkout's. The
client ran from a copy of the game folder whose `clientinfo.xml` names that port. What the
server's WAL showed:

- **The features never reached a client that could keep them.** The server sent
  `ZC_NET_FEATURES` 0.07 s after the connection; the client loaded its map until 2 s and again
  until 4 s, and forgets the features at each load. No stamp went out and the keep-alive stayed
  at 12 s. The server now answers each `CZ_NOTIFY_ACTORINIT` with the features.
- **The clock started up to a second behind after a map load.** The answer to the time request
  of the first load waited out the second load and became the first sample, with a 2 s round
  trip. The first stamp was 813 ms behind the server's tick at its arrival and the error left
  at a tenth per second. The client now drops an answer to a request from before the load, and
  a clock that is behind is set forward at once while its first eight samples come in. After
  that the first stamp, 1.4 s after the load, was 8 ms behind.
- **What worked as built:** eight time requests 250 ms apart and then one every 2 s; a stamp
  before every move, a few microseconds ahead of its request in the server's record; every
  move answered with `ZC_MOVE_RESULT` (53 of 53 accepted in one session); a walk from standing
  starting 60 ms after its press tick with the 60 ms hold-back.
- **Seen and left:** a walk request the client holds for its spacing window is stamped with its
  click, and such stamps arrived up to 225 ms after their press tick, close to the 250 ms the
  server accepts. Not run: attacks, skills, pickups, refusals, a second player, any latency
  (`RAGEXE_LAG_MS`), and how the predicted walk looks.
