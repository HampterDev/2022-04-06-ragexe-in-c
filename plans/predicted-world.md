# The predicted world

How the client shows the player's own character ahead of the server: one persistent simulation
on the client, the server's own code, corrected by one path. It replaces the separate
predictors the client has today (the own walk, the turn, the attack order, the swing, the
holds), each of which carries its own rules and its own repair.

It builds what `plans/body-core.md` says under "On the client" and does not change that
design. `plans/core-loop.md` has the packets and the clocks, `plans/netcode.md` the stamps.

Status, 2026-11-12: all eight steps (0 to 7, with 3a) are built on both sides and pass their
checks; nothing is committed yet. See "What is built".

## What kind of problem this is

Client-side prediction over a deterministic simulation with an authoritative server. The
fields that have worked on it longest, and what is taken from each:

- **QuakeWorld and Source: prediction and reconciliation.** The client keeps the last state the
  server confirmed and the commands it sent since. Every frame it applies those commands again
  on top of the confirmed state, with the server's movement code. A server state names the last
  command it includes; older commands are dropped. Taken: all of it. It is the whole shape.
- **Rollback fighting games (GGPO).** The state is a block of bytes that can be saved and put
  back, so going back to a tick and running forward again is cheap and exact. Taken: the
  world is the body core's arena, which is such a block already.
- **Lockstep strategy games.** Both sides run the same code and compare a hash of the state to
  find the first tick they differ at. Taken: the server and the client compare the own body's
  bytes by tick, in tests and in the playground, so a difference is found where it starts and
  not where it shows.
- **Databases with a write-ahead log.** Confirmed state plus a log of what is not confirmed
  yet; recovery is replay. Taken: the pending requests are that log, in order, numbered.
- **Overwatch.** One simulation for server and client, inputs with numbers, the client ahead
  of the server by the way there, mispredictions corrected by replay and smoothed in drawing
  only. Taken: smoothing is presentation and never state.

## The design

Three parts, and nothing else predicts.

### 1. The rules live in the core, all of them

The body core (`src/body`, the server's `sim/body`) already decides walking, attacking, the
intent's step and the clocks. What the server's game code still decides by itself, and the
client therefore has to copy by hand, moves in:

- **What a request does.** Today the server's combat code turns a client request into several
  inputs: a move ends the intent and the swing and then walks; an attack order ends another
  target's attack and sets an intent. The core gets one input for a request, with its kind and
  number, and does those things itself.
- **Reach.** The game answers the intent's question "is the target in reach" from its own copy
  of the distance rules. The core has both bodies and decides it.
- **What the game alone knows stays a question:** whether the two may fight, a wall in the line
  of fire, ammunition, whether a hit was made, what a skill costs in time. These are facts of
  content, asked as events and answered as inputs, as now.

After this the server's game code supplies facts and the core decides everything that moves a
body. A rule is written once.

### 2. The client keeps one world

`world/predicted.c`, freestanding, with its own tests. It holds:

- **The confirmed world:** the core's arena as of a server tick, with the own body as the
  server last said it was, and the bodies it depends on (its target) as their records say.
- **The pending requests:** every request the player sent that the server has not confirmed,
  in order, each with its number and the tick the server applies it at, which its stamp names
  (`game/net_clock.h`, `std/way_up.c`).
- **A short history** of the own body's bytes by tick, for the comparison below.

Every frame it copies the confirmed world, puts in the pending requests and the other bodies'
records at their ticks, answers the core's questions (yes, where the client does not know
better), and runs to the own clock. What is drawn is the own body of that world: its move
record goes to the character's move track when it changed, its act state to the act queue.
Drawing, steering and the animation fit stay as they are and read nothing else.

### 3. One correction

The server sends the owner its own body: the core's state of it, the tick, and the number of
the last request that state includes (`ZC_OWN_BODY`). It is sent whenever something that was
put into the server's core changed that body: an input of the game's, an answer to one of the
core's questions, and each request, whose number the body keeps, so that this is the
request's confirmation. What the core does with the body by itself is not sent.

The client compares it with what it had predicted for that tick. Equal: the requests up to
that number leave the pending list and nothing is seen. Different: the confirmed world is set
to the server's state, the requests with a later number are applied again, and the world runs
to the own clock. The character is steered to the result, never set. A refusal, a hit that
holds the walk, a target that died, a knockback and a wrong guess are all this one case.

A client whose core has another rules version than the server's does not predict.

**Told again when nothing told it (not built).** A body is told when something from outside
changed it, so a client that is wrong without such a cause stays wrong until the next one. The
server tells a player's body again when two seconds have passed without a word of it, on a
timer of the unit, which the journal replays. The client treats it as any word: equal, and
nothing is seen; different, and it is the one correction. It counts the words that differed
without a request or a record to explain them, since each is a fault in a rule or in the
prediction and worth a line in the log. Check: the convergence test of `tests/test_predicted.c`
with a body changed behind the client's back converges within the two seconds; the playground's
count of such words is 0.

## What goes away

Each of these was a special case of "apply the pending requests to the confirmed state" and
went in the step that took its job over (steps 4 and 5; the last row goes with step 7):

| What it was | Its job |
|---|---|
| `player_predict_walk`, `redirect`, `predicted*`, `walk_plain` in `actor/player.c` | The own walk and its turn |
| `actor/walk_answer.c`: the turn that waits, the move's answer, the kept walk, the hold, the order's book | Timing, refusal, restoring |
| `world/own_walk.c` | Where a walk starts |
| `world/own_attack.c`, `order_plan`, `order_follow`, `order_stand` in `game/packets_act.c` | The attack order and keeping it right |
| `world/own_swing.c`: withdraw, overdue | Taking a wrong swing back |
| The own-track clearing in `game_mode_zc_stopmove`, the refusal path of `ZC_OWN_INTENT` | Correction |
| The client's copy of the intent's rules | Rules the core has |

`std/walk_steer.c` keeps how a drawn character is steered to its walk. `world/own_swing.c` keeps the
book of the swing that is shown, and how a swing shown ahead is stretched to meet its damage
number.

## What can be shown to hold

- **Same inputs, same bytes.** The core's scenario digests, as now.
- **Convergence.** In a test with a server world and a client world of the same core, random
  requests, lag, jitter and server-side surprises (holds, refusals, a target that moves): after
  the server's state for request *n* arrives and nothing new happens, the client's own body
  equals the server's, byte for byte, at every tick from then on.
- **Nothing predicted is lost or doubled.** Each request is applied once in the confirmed
  world, whatever order confirmations arrive in.
- **On screen.** The playground compares the own body's bytes on both sides by tick and
  reports the first tick they differ at, with the request that preceded it. Its jump, far and
  rest checks stay.
- **One path.** No file but `world/predicted.c` puts a request into a world; the library
  table and a check in `build.bat check` hold that.

## Constraints

- The original's packet layouts stay. `ZC_OWN_BODY` is a packet of the project's own.
- The server's journal replays bit-equal: what goes into the core is journaled as now.
- No switches. What this plan builds is how the server and the client work, not an option
  beside the old way: a step adds no on/off setting, and where it takes a job over, the old
  code for that job goes with it. Only a number someone tunes is a setting (`hold_back_ms`,
  `hold_limit_ms`, `action_windup_rate`, `skill_pose_rate`, `action_cancel_penalty_ms`). A
  developer's aid that draws or delays something and decides nothing in the game
  (`RAGEXE_NET_OVERVIEW`, `RAGEXE_LAG_MS`) is not a switch in this sense. The cost is named
  once: a client that does not speak the project's netcode can no longer play on this server.
- Where the server keeps the fork's behaviour, the core keeps it; departures are listed in
  the server's approved deltas.
- Other actors are not predicted: they are drawn from their records a render delay behind.

## Steps

Each step ends with the listed check passing and leaves the game playable.

| Step | What | Check |
|---|---|---|
| 0 The measure | The playground compares the own body on both sides by tick and names the first difference. The present findings are the baseline | It reports today's known differences |
| 1 Requests in the core | The core takes a request as one input and does what the server's combat code does around it today; it decides reach. Rules version up; the server's game code shrinks to facts | Core tests and scenario digests; the server's session tests send the same bytes |
| 2 `ZC_OWN_BODY` | The server sends the owner its body with tick and last request number | A session test reads it back equal to the core's state |
| 3 The predicted world | `world/predicted.c`: confirmed world, pending requests, replay, comparison | The convergence test above, with deliberate faults each caught |
| 3a One way | The on/off settings this line of work grew go, and on is the only way: on the server `time_sync`, `input_stamps`, `intents`, `tracks`, `own_records`, `action_windup`, `mob_windup`, `skill_pose`, `body_core`, `body_chase`, `body_attack` and `own_body`, with the code each kept for off; `ZC_NET_FEATURES` keeps only the numbers it carries; in the client the branches on those feature flags and the classic handling behind them. Old code that still has a user stays until the step that takes that user over (a monster's classic walk and attack: step 6; the game's skill and pickup intents: step 7) and goes there. A test that compared on with off keeps what it expected as recorded bytes | The whole suites on both sides; the playground's findings are no more than before; no setting of the list is read anywhere |
| 4 Walking through it | The own walk is drawn from the predicted world; the walk predictors go | Playground, walking moods, with and without lag: no finding of the own character, quick clicks included |
| 5 Attack orders through it | The order, its walk and its first swing come from the predicted world; the attack predictors and their repairs go | Playground, fights and hit and run, with and without lag: no finding of the own character |
| 6 Monsters as bodies | Monsters walk and attack through the core and are sent as records, so a target in the client's world is exact | The monster findings of the playground are gone |
| 7 The rest | Skills and pickups as requests in the same way; the leftovers named above are deleted; the docs say what is | `build.bat check`, the whole suites on both sides, the playground |

Steps 1 and 2 are the server's and can be built together. Step 3 needs neither and can be
built beside them. Step 3a comes before 4, so that 4 to 7 are each built and checked on one
path only. Step 6 does not depend on 4 and 5.

## What this does not do

- It does not predict what other players or monsters do.
- It does not change how a hit, a cast bar or a damage number is drawn.
- It does not hide lag: what the server decides against the player's request is seen one way
  down later, as a correction steered over a few frames.

## What is built

All eight steps, on both sides (the client's `custom`, the server's worktree
`local\worktrees\action-timing`), not committed. The own character's walk, its orders, its
skills and pickups and the first swing of each are drawn from the predicted world, the
predictors that drew them before are gone, and every unit, monsters and companions too, walks
and attacks through the body core. A row's last column names what its step left; where a
later step settled it, that row says so.

| Step | Built | Not built or not covered |
|---|---|---|
| 0 The measure | The playground compares the own body by tick (`consumers/host/playground/body-checks.js` on the server's side, `tests/playground/play_driver.c` here). The client reports, each frame and for the tick of its own clock, the cell the walk its character is drawn on has it on, and the body of its predicted world as the core's bytes. The runner has the server's body with the tick of every change (`acts.watch` in `spatial/core-walks.js`) and judges a sample once the server's clock is there. A difference that lasts is one finding: the tick it begins at, the last request before it, which part of the body differs, how long it lasted. The baseline, eight runs of 40 s with six monsters and every mood: without lag, 15 findings on screen (12 of monsters; of the own character two drawn far from the server in quick walk clicks and one at rest on the wrong part of its cell after a fight), the drawn cell differs from the server's 13 times (0.8% of 19,833 ticks; ten after a walk request, three after an order to attack), and the predicted body 183 times (20.6%), each after an order to attack; with a ping of 80 ms and 20 ms of jitter, 21 on screen (14 of monsters; six times the own character far, one jump), the drawn cell 31 times (1.7%), the predicted body 382 times (26.9%) | The cell is compared with a slack of 12 ms around the tick, as the client's clock is its reckoning of the server's. The predicted body's attack orders are not measured fairly yet: the world has no other bodies (step 5), so every order to attack is a difference until the server's next word |
| 1 Requests in the core | Rules version 10. `BODY_INPUT_REQUEST` with the request's number, which the body keeps: a walk (the intent goes, the attack and its swing end, then the walk, over the steps the server lets a request take), an order to attack (an attack on another body ends; an intent the core sees for itself), a release, a cancel, and a request that did nothing. For such an intent the core reads reach, the running attack, the swing and the walk from its own bodies, looks from every cell and when the attack is out of reach, begins the attack and the walk after its target itself, counts the swings, and ends the attack with the intent. The game answers with facts: the target is there, the body can act, the line of fire, the target walks, leave to attack, leave to walk. `body_state` and `body_put_state`: a body as 224 bytes another world can take. In the server, `spatial` puts a player's walk in as a request and `eos-combat` an order, its release and its cancel; combat's `intentView` and `unit-attack.js`'s `reach` are not asked for these. Checks: the core's tests with 46 deliberate faults each noticed by an example, the scenario digests natively, as wasm and with 32-bit MSVC; the server's fight sessions send the lines recorded of them (3a) | Skills and pickups are no requests the core knows; their intents are the game's to see for. The classic attack request (`CZ_REQUEST_ACT2`) is not one either. A monster is no walking body of the core: it stands where its unit is |
| 2 `ZC_OWN_BODY` | 0x0F25, sent to every player, with no setting: the core's rules version, the tick as the core has it (64 bits), the number of the last request the body includes, the world's two settings, and the body's bytes. Told when the player is put on a map and whenever an input or an answer changed the body, so each request is confirmed by the body that includes it; not for what the core does by itself. A session test reads each told body back into a fresh world of the core, and compares the last with the server's own. Here: the packet table and dispatch row, `predicted_read` (bounded), `game/own_body.c` | Nothing tells a body without a cause: a client that went wrong without an outside input stays wrong until the next one. About 240 bytes at every swing's start. A body that was never told how it fights is told so only with the first order |
| 3 The predicted world | `world/predicted.c`, freestanding: the server's last word, the pending requests with their numbers and ticks, the other bodies' records, the world made from these and run to the own clock with every question answered yes, a past of the own body's states by tick, and the comparison of the server's word with what was predicted for its tick. `game/own_body.c` keeps one from `ZC_OWN_BODY`, puts the player's walks and orders to attack in as they leave and runs it once a frame, without drawing from it. `tests/test_predicted.c`: the pieces by example, and the convergence test: 126 runs of a server world and a client world with random requests, lag up to 150 ms with jitter up to 120, a clock that is off, refusals, answers that are not yes, holds, pushes, stops, other stats and speed, a monster that walks, dies and comes back. From a second after the last of it the client's body is the server's at every tick it is compared at (over 20,000), no word of the server is a correction any more, and no request is left waiting. 44 deliberate faults in `predicted.c` were each noticed | The world is given no other bodies by the game yet, so an order to attack is predicted as a request that did nothing. An intent the game sees for (a skill, a pickup) is held as it is. A record of another body that begins at the very tick something of the own body is due is taken after that. The check that no file but `predicted.c` puts a request into a world waits for the predictors to go (steps 4 and 5) |
| 3a One way | On the server none of the twelve settings is read. Time sync, input stamps, intents, records (of a player's own walk too), the windup of a player's and of a monster's swing, the skill pose, the core for every walk `spatial` runs and for a player's attack and intent, and `ZC_OWN_BODY` are how it works. Gone with them: `spatial`'s walk in JS and its own search to within a range and after a unit (`plain-walk.js`, `pathfinder.js`, `walk-retarget.js`, `movement-delay.js`), the windup in `unit-attack.js`, the JS keeper of intents, the fork's swing through the core, and the four packets nothing sends any more (0x0F10, 0x0F11, 0x0F13, 0x0F15). A player attacks and has an intent through the core or not at all. The numbers stay settings, with the values the game is played with when the config has no line: `hold_back_ms` 60, `press_limit_ms` 250, `action_windup_rate` 100, `skill_pose_rate` 100, `action_cancel_penalty_ms` 0. 0x0F14 is `ZC_HOLD_BACK`, four bytes with the hold-back alone. It stays a packet of its own, told once a map: the body is told at every change of it, and the present predictors need the number without the predicted world. In the client the feature flags are gone, and with them the walk into reach the original made itself for an attack, a pickup and a skill, the handlers of the four packets, and what only those used; the one check left is whether the server's clock is known. Checks: the server's session tests hold lines recorded from the code with every setting on, before the old paths went, and the server sends them byte for byte (`consumers/host/recorded`, about 250 KB, the bodies told among them); the three comparisons of the core with the JS (walk, attack with windup, intent) hold digests taken from the JS's side of them; the whole suites on both sides, the server's `make check` and `build.bat check` here. The playground against the baseline of step 0, own character on screen and ticks the drawn cell differs: walking 2 and 4 before, 2 and 4 after; all moods 3 and 13, then 0 and 4; walking with a ping of 80 ms and 20 ms of jitter 0 and 8, then 0 and 8; all moods with that lag 7 and 31, then 1 and 5. All findings on screen, monsters among them: 2, 15, 0 and 21 before, 2, 14, 0 and 17 after | Still there, each with its user: a monster's own walk, driven step by step by the `mob` module and sent as the fork's packet, and its attack (step 6); the game's seeing for skill and pickup intents in `eos-combat` (step 7); `unit-attack.js` for a homunculus and a mercenary, with unit.c's player branches that no player reaches; in the client the walk into reach for a skill or an attack on a skill unit, which no intent covers. The flag that is gone also chose how far apart walk requests were sent; step 4 settled that anew. A snapshot of a server from before is taken with its JS walks and attacks ended; not tried on a real server folder. The scene captures and the CMake builds were not run, and nothing was looked at on screen. A client without the project's netcode cannot play on this server, and this client plays only on such a server |
| 4 Walking through it | The own character's walk is the predicted world's. Each frame, and at once after a request or a word of the server's, `game/own_body.c` takes the body's move and files it in the character's move track when it says something new (`predicted_walk_news` in `world/predicted.c`); the actor code plays it on the own clock as it plays any record, steered. A word of the server's that differs is the one correction: the world is made again and the character is steered to what it then says. Gone: the walk predictors (`player_predict_walk`, the turn that waited for its cell, the walk answer's repairs, the kept record of the own walk, the walk hold), the start of `world/own_walk.c`, their fields and tests. The own character's `ZC_STOPMOVE`, walk-ok and `ZC_TRACK_MOVE` are not read while a body is predicted; the server no longer sends the last two, nor the answer to a move (`SPATIAL_EVT.MOVE_RESULT`): the body with the request's number says it. A walk request leaves with the frame of its click, the newest click of the frame, and no spacing is kept. Measured with 350 ms, 150 ms and none between requests: no finding of the own character with any of them, and a click reaches the server after 92 ms, 40 ms and one frame on average; a held button repeats every 600 ms, so the core needs no bound. Check: the playground's walking moods, 16 runs of 40 s a link: findings of the own character 0 / 0 / 0 without lag, with a ping of 80 ms and 20 ms of jitter, and with 150 ms and 40 ms (before: 2 / 0 / 32) | With lag the predicted body is not the server's for about 48 ms after a request (4.5% of the ticks at ping 80, 15.2% at ping 150): the server applies a request on arrival when its press tick and the hold-back are past, which is later than the client reckoned. Each is one steered correction, and none showed as a finding. `ZC_STOPMOVE` still goes to the own client and is not read |
| 5 Attack orders through it | The world is told of the bodies the own one names and of the newest order's target (`game/own_body.c`): a walk that came as a record as it is, a monster's classic walk as the server's way between its two ends at the walk's ticks, otherwise the cell the actor is drawn on. The order's walk into reach is the body's move, as in step 4. Its swing is the world's (`predicted_swing`): shown when the world opens it, taken back when the world takes it away, and the server's record of a swing that was shown is not played again (`world/own_swing.c`, the book of the swing shown); the hit's notice brings the damage number as before. The server tells a player's body how she fights when she is put on a map, so the first swing is predicted too. Gone: `world/own_attack.c`, `world/own_walk.c`, `actor/walk_answer.c`, the order code and its fields in `game/packets_act.c` and `game/gamemode.h`, `MODE_MSG_OWN_ORDER`, `path_finder_walk_at`, `path_finder_own_attack_plan`, their tests. `tools/check_one_world.py`, run by `build.bat check`, holds that no file but `world/predicted.c` puts a request into a world. Check: the playground, 16 runs of 40 s a set: findings of the own character 0 on all three links in fights and hit and run (before: 12 / 13 / 75) and in all moods (before: 3 / 7 / 48); in fights without lag the predicted body differs for 1.7% of the ticks (before: 31.1%). The whole suites on both sides, the server's `make check` and `build.bat check` here | An order to attack another player is not shown ahead: the server's body names its target by the unit's id, which is not the id a client knows a player by, so such an order is drawn from the server's words. A monster is in the world where its classic walk has it; a walk a hit held or cut short is not known until the next packet (step 6). The monsters' findings in the fight sets are 23 / 27 / 31 where they were 18 / 24 / 20: the character fights sooner and more than before, and a trial that left the drawing of other actors as it was gave the same counts. A change of the character's stats between orders reaches the body with the next order or answer. Of `ZC_OWN_INTENT` only the end is read, and the server still sends the states before it. Not run: the scene captures, the CMake builds; nothing was looked at on screen or played by a person |
| 6 Monsters as bodies | Rules version 11. A monster's walk is the core's: the `mob` module finds its way by the fork's rules for a monster (the easy walk, the trim to the chase range) and hands the steps to `spatial` (`WALK_TO` with its steps); the core takes each at its tick, and the module hears every cell (`MOVED`) and does there what the fork's walk timer did (the walk count and its skills, `min_chase`, a target that moved, the chase's end). A walk delay is said when it is set, so a held or cut walk goes out as a record (`WALK_DELAYED`), as every walk start does (`ZC_TRACK_MOVE`). A monster's attack, a homunculus's and a mercenary's are the core's: a fighter has a kind (other, player, monster) and a flag for a swing that holds its walk; a monster out of reach is told its chase (`BODY_EVENT_CHASE`), which the `mob` module walks; before its swing the core asks (`ASK_BEGIN`), so the AI's skill and link hooks run first; its impact reaches a cell further than its range; a look that forgets its target is told (`UNATTACKABLE`). Gone: the `mob` module's walk timer and stepping, `UNIT_WALK`, `applyUnitWalk`, the fork's walk packet and its resends for a monster, `mob-attack.js`'s own swing, `MOB_CMD.ATTACK`, combat's `handleMobSwing`, `unit-attack.js` with `attackUds`, companion-combat's chases; here the rebuilding of a monster's classic walk and `path_finder_find_server_path`. Found on the way: the monsters' AI read a player's target from the classic attack packets, so an attack by intent was never seen; it reads the core's now. A snapshot from before: a world of rules version 10 is refused with a plain message, the JS walks and attacks it holds are ended. Check: the playground, 16 runs of 40 s a set, nine sets: no finding on screen in any set, of the own character or of a monster (monsters before: fights 23 / 27 / 31, all moods 25 / 33 / 32); in fights without lag the predicted body differs for 1.6% of the ticks (before: 1.7%), in other fights, as monsters now see attacks by intent. The server's touched suites, the host sessions with a monster's swing (`host/mob-attack-session.test.mjs`), the core's tests with 15 deliberate faults and the server's with 15 more, each noticed | The departures from the fork that the server's `docs/body-core.md` names (a step under way lands before a monster's swing, a monster's swing has a windup, a look comes before the AI's think of its tick) are recorded in the server's `parity/approved-deltas.json`, with those of the earlier steps. With lag the predicted body still differed for 10% and 32% of the fight ticks, after requests the server applied later than the client reckoned (settled in step 7). A snapshot from before is tried in tests, not on a real server folder |
| 7 The rest | Rules version 12. Every intent is the core's own: a skill on a body (the own one too), a skill on a cell and a pickup are requests (`SKILL`, `SKILL_CELL`, `PICKUP`, each with its range), predicted in the same world; a skill on a skill unit is a skill on its cell. An attack on an Ice Wall or a laid trap is an attack on a body the server stands for that unit, which the predicted world has no body for, so it is not predicted; an attack on any other skill unit the server ends at once. Combat's own seeing for intents and the core's intent phase event go. The tick a request is applied at is agreed in the packet: the stamp (`CZ_INPUT_STAMP`, 12 bytes) names it, the client's clock at the press, which is the later of the server's clock and the hold-back and of the client's own clock and the way up (`std/way_up.c`: of the last 16 exchanges the second longest and half its height above the shortest; it grows at once and shrinks a millisecond in ten). The server applies a request at that tick when it arrives before it and the tick is at most `hold_limit_ms` (250) after its arrival, otherwise on arrival. One id per unit: the core names a player's body by the id her clients know her by, so an order on another player is predicted. A player gets no `ZC_STOPMOVE` about herself and only the end of `ZC_OWN_INTENT`; a change of her stats reaches her body at once. The server reads 0x0AF4 (the companions' ground skills). Gone: `press_limit_ms`, `walk_apply_tick`, `game_mode_apply_time` (`std/walk_predict.*` is `std/walk_steer.*`, the steer being what was left), here the walk into reach for a skill or an attack on a skill unit and what only it used (the pending value 6, `attack_sent`, `attacks`, `attack_kind`, four messages, `game_mode_cell_along_line`), the own branch of `ZC_NOTIFY_PLAYERMOVE`, and on both sides what only a gone path used. Talkie Box and Graffiti open their text window again. Classic skill requests that stay, each with its sender: 0x0438 (a skill on oneself, every homunculus and mercenary skill), 0x0AF4 (the companion AI's ground skills), 0x0367 (Talkie Box, Graffiti). In the predicted world a body that goes leaves it at the world's clock and keeps its past: a world made again from the server's last word has it until then, and a word that comes after the going is of a world without it (before, a monster that the own swing killed vanished from the replayed past, the swing counted as lost, and the next swing was predicted up to an attack delay too soon, for seconds). An attack, a skill or a pickup the core refuses is a cancel by its number, as on the server. Check: the playground, 16 runs of 40 s a set, nine sets, against the state after step 3a: no finding on screen in any set (before, own character and monsters together: walking 2 / 0 / 32, fights 30 / 37 / 95, all moods 29 / 37 / 81, without lag, with a ping of 80 ms and 20 ms of jitter, and with 150 ms and 40 ms); the predicted body differs for walking 0.00 / 0.13 / 0.55% of the ticks (before 0.00 / 2.87 / 12.26), in fights 1.36 / 2.83 / 4.29% (31.12 / 40.97 / 50.32), in all moods 0.67 / 1.19 / 2.49% (18.90 / 23.89 / 32.73). The whole suites on both sides, the client's with the original's oracle, the server's `make check`, `build.bat check`, the CMake builds for x64 and Win32 with their tests, the x64 client, and the scene captures against a client of before step 6 | The playground's player walks and attacks; skills and pickups are held by the host sessions and `tests/test_predicted.c`, not played. What still differs is corrected by the server's next word, within 128 ms without lag: an order on a monster that died or went on the server, which the client learns with that word, and a monster whose new walk the client learns a way later than the server took it, so that for a moment one side swings where the other still walks into reach. With lag, beside those, requests that arrive after the tick they name (in the walking sets all within the first ten seconds after the map was loaded, while few exchanges have measured the way up). The scene captures on this machine differ from one run to the next of the same client, in the 3D ground; the final client's differ from those of the client of before step 6 no more than that, and on no actor or window |

