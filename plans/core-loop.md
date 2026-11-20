# Core loop

A proposal for how walking, attacks, skills and monsters travel between this client (`custom`)
and the ring-buffer server: requests go up as intents, and everything a player sees comes down
as timed state. Status, 2026-11-10: design decided (see "Decided"); the fixes of "Seen in play"
and build steps 1 and 2 (the move track for other actors; the act track for basic attacks, the
hit notice and monsters' HP), the own-intent record of step 3 and the cast record and pose hold
of step 4 are built on both sides and pass their test suites. None of it has run client against
server. See "What is built" at the end. Step 5 was redesigned as a simulation core of its own:
`plans/body-core.md`.

## Where it comes from

- **`a_new_beginning/game/online/protocol.odin`.** Inputs up, state down, and state that heals
  itself because it is sent whole. That engine sends the whole world every tick over an
  unreliable transport. RO has TCP and hundreds of actors on screen, so this proposal keeps the
  rule (absolute state, newest wins) and drops the mechanism (everything, every tick).
- **`ring-buffer/docs/golden-backend-transition.md`, section 7.** "Everything a client sees is
  a render of state": the simulation changes state, and packets are made from state at the end
  of a frame. This proposal is the wire format that renderer would want.
- **`plans/netcode.md`, `plans/intents.md`, `plans/action-timing.md`.** The clocks, the stamps,
  the intents and the windup. They stay. Five of their packets are folded into the ones below.

## What is wrong today

The classic protocol tells the client what happened and lets it work out what is true.

- **The client finds every path itself.** A walk packet names two cells, and the client runs
  its own search between them (`actor_sendmsg.c`, message 0). When that search finds no way,
  the actor is left standing while the server walks it. Any cell the two sides see differently
  can do this, for example an ice wall the client has not been told about.
- **A hit pauses a walk on the server only.** Combat sends `DELAY_WALK`, and spatial pushes the
  walker's next step back (`movement-delay.js`) without publishing anything. The client keeps
  walking the actor on the old timing, so after a hit the two disagree by the length of the
  pause.
- **A stop has no time.** `ZC_STOPMOVE` carries a cell and no tick. A cast (`ZC_USESKILL_ACK`)
  carries a length and no start. The client dates both by when they arrive, so every timing
  leans by the latency and its jitter.
- **The client's animation decides when a swing or a skill pose ends.** The server does not
  know it, which is the open skill-rate question of `plans/action-timing.md`.
- **Each plan so far added its own answer packet** (`ZC_ACTION_BEGIN`, `ZC_ACTION_CANCEL`,
  `ZC_MOVE_RESULT`, `ZC_INTENT_ENDED`). Each is one more event the client has to pair up with
  something it remembers.

## Words

- **Intent:** what a player wants. One value per player, newest wins (`plans/intents.md`).
- **Track:** one aspect of an actor that lasts: where it is, what it is doing, its HP.
- **Record:** the value of a track from a given server tick on. A record is complete: it can be
  applied without knowing the one before it, and without anything the client worked out.
- **Notice:** something that is shown once and changes nothing the client keeps: a damage
  number, an effect, a refusal message.

## Rules

1. Whatever lasts travels as a record. A notice never changes a track.
2. Every record and notice carries the server tick it belongs to (`at_tick`). The server sends
   it as soon as it knows it, which may be before that tick. Ticks are compared by their signed
   difference, so the 32-bit wrap does not matter.
3. Of two records of a track, the one with the higher revision is the newer. A record that
   arrives removes every older record of its track with the same or a later `at_tick`, and
   holds from its own `at_tick` on; one that arrives when a newer one with the same or an
   earlier `at_tick` is held, or that is not newer than the record last played, is dropped. So
   records may arrive in any order and more than once (`plans/delivery.md`, part 1). There are
   no cancel packets: a cancelled swing is a newer record on the same track.
4. A record with an end falls back to idle at its end without another packet.
5. The client applies a record when the clock it draws that actor on reaches `at_tick`. A
   record that is already due applies at once.
6. A drawn position is steered, never set. When a record puts an actor somewhere else than it
   is drawn, the actor closes the gap at up to 30% over its walk speed, the rate the client
   already uses for a walk that arrives late (`actor_path.c`). Only a record marked "placed",
   or a gap of more than four cells, is drawn at once.
7. A record goes to everyone who sees the actor when it is made. An actor that comes into view
   arrives with the current record of every track and any that are still waiting for their
   tick. The server keeps no memory of what each viewer has.
8. The client decides nothing about another actor. For its own character it may run ahead of
   the server on the own clock, but it never shows a result (damage, a kill, a drop) before the
   server sent it.
9. The client's map cells are a hint for predicting the own walk and for the cursor. Nothing
   that is drawn from a record depends on them.

## Tracks

| Track | A record says | Sent to | Takes over from |
|---|---|---|---|
| Move | The cell at `at_tick`, how long it stands there, then the steps it walks and at what speed. Also how it got there: walked, pushed, placed. | Viewers | `ZC_NOTIFY_MOVE`, `ZC_NOTIFY_PLAYERMOVE`, `ZC_STOPMOVE`, the slide and knockback packets |
| Act | What the body does from `at_tick` to `end_tick`: idle, swing, cast, skill pose, flinch, sit, dead. With target or cell, skill, and the impact tick of a swing. | Viewers | `ZC_ACTION_BEGIN`, `ZC_ACTION_CANCEL`, `ZC_USESKILL_ACK`, the cast-cancel packet, sit, stand and death in `ZC_NOTIFY_ACT2`, the damage motion inside hit packets |
| Vitals | HP and max HP from `at_tick`. | Viewers allowed to see it | `ZC_HP_INFO`, the HP part of `ZC_PAR_CHANGE` |
| Own intent | What the server holds for this player: the request's number, and whether it is held, approaching, acting or ended, with reason and tick. | Owner | `ZC_MOVE_RESULT`, `ZC_INTENT_ENDED` |
| Own clocks | The ticks from which the character may swing, cast and move. | Owner | The client's own guards (1200 ms attack resend, animation gates) |

Status effects and looks are already absolute packets and stay as they are. So do ground skill
units, items, chat and NPCs.

Two notices in this round: **hit** (source, target, impact tick, damage, count, kind, skill) and
the existing skill-failure messages. A hit and the vitals record it caused carry the same tick,
so the number and the HP bar move in the same frame.

## The loop

Server, each 20 ms frame:

1. Take requests in press-tick order (the hold-back of `plans/netcode.md`) and set intents.
2. Step every living intent against its body (`plans/intents.md`): walk into range, start the
   act, wait, or end.
3. Advance bodies: cell boundaries, impacts, cast ends, flinch ends.
4. Render: send every record made in this frame to the actor's viewers, and the frame's
   notices. A track changed twice in a frame sends its last record.

Client, each drawn frame:

1. File incoming records under their actor, track and tick.
2. Draw every other actor at the render clock: cell and offset from its move record, sprite
   action and frame from its act record. Both are pure functions of record and clock. What it
   aims at the own character (a swing or a cast at it, a hit on it, and the end of such a swing
   or cast) is played on the own clock instead (`view/act_track.h`): the server decides a
   swing's hit at the tick it lands at, and a request sent now is applied at the own clock's
   tick, so the swing lands on the screen at the last moment a request is still applied before
   it.
3. Draw the own character at the own clock, from predicted records that the server's replace.
4. Turn input into one intent, stamp it, send it.

The render clock is the true clock minus a delay, taken from the clock sync and no longer from
the classic tick copy. When the delay is longer than the download time plus jitter, records
arrive before they are due, and other actors move with the server's own spacing at any ping.

## Walking

- **The record carries the path.** The server searched it, so it sends the steps it found: at
  most 32 directions of three bits, 12 bytes. The client walks those steps and searches
  nothing for a record. The step time is the shared rule (`grid_path_step_ms`).
- **A turn** is the next record, dated at the cell boundary where the server turns, so it
  arrives early and joins without a correction. A speed change is the same, with the steps
  that are left.
- **A pause** is a record too. When a hit delays a walk, the record says: on this cell from
  now, the next step lands this many milliseconds on, then these steps. A second hit during
  the pause sends the next one. The flinch on the act track covers the same time.
- **A stop, a knockback and a slide** are records without steps, marked walked, pushed or
  placed.
- **The own walk** is predicted as built in `plans/netcode.md` step 2. The answer is the
  own-intent record, and the server's move record replaces the predicted one. Where the two
  paths differ, rule 6 steers the character onto the server's.
- **A walk the server starts for the own character** (into range for an intent) arrives one
  round trip after its start on the own clock. Rule 6 lets it start where the character
  stands and catch up, where today it begins a round trip into its path. Build step 5
  predicts it and removes the catch-up.

## Seen in play: the own character darts to a cell when walking in circles

Patrick, 2026-11-10, on the client of the first live run (predicted walk on, 60 ms hold-back).
Read from the server's WAL of two sessions (connections 26 and 30 of the test server), with
the walk paths recomputed by the server's own search.

- **The client and the server often turn at different cells.** A turn is predicted at "the
  first cell of the present walk after the request applies". In 31 of 65 and 9 of 19 turns the
  request applied within 25 ms of a cell boundary. The client asks again every 350 ms and the
  hold-back adds 60, and two diagonal steps take 420 ms, so holding the mouse lands on the
  boundary again and again.
- **The walk start's tick is the frame's, not the step's.** `eos-world` stamps the packet with
  `serverTick()` when it renders; spatial started the walk at the step's own tick, which
  `WALK_STARTED` does not carry. 16 of the 36 turns that were off the previous walk's schedule
  were off by exactly 10 ms, the half frame a 210 ms diagonal step leaves. The client's
  boundaries are shifted by that much, which is enough to pick the other cell.
- **A request the client held for its spacing is predicted too late.** The client assumes it
  applies 60 ms after it is sent. The server applies it at the press tick plus 60, or on
  arrival when that has passed. With the client's rule the turn cell was wrong in 6 of 65 and
  1 of 19 turns, all of them held requests; with the server's rule in 2 and 0.
- **A wrong turn cell is drawn as a dash.** The walking state moves the character from where
  it stands to the next cell of its path so that it arrives at that cell's time
  (`actor_state.c`, state 1). When the server's walk replaces the predicted one, the next cell
  can be a cell or two away with 100 ms left.

Not done: the dash itself was not measured on screen, only the disagreement that leads to it.

Changed since (2026-11-10, not seen on screen yet): `WALK_STARTED` carries the walk's own tick
and the packet uses it; a stamped move turns at the first step whose tick is not before the
move was applied, and no longer at a step whose sweep had not run yet; the client predicts a
held request with the server's rule and takes a cell due exactly then; and the own character
closes a gap at 13/10 of its walk speed (rule 6). What follows for this design: `at_tick` is
always the tick the simulation scheduled, never the frame that rendered it, and walking as an
intent removes the client's 350 ms spacing.

## Cells that change during play

Two things change whether a cell can be walked on the server: an ice wall, and another unit
standing on it.

- **Drawing does not depend on them.** Every record carries its steps (rule 9), so an actor
  the server walks is drawn walking, whatever the client believes about the cells.
- **The server re-paths on its own.** A walk that meets a new wall gets a new path, which is a
  new record.
- **Prediction uses the cells as a hint.** The client learns an ice wall from
  `ZC_UPDATE_MAPINFO` and applies it on arrival, which is the earliest it can know. A click
  made before that is predicted through the wall for at most one round trip, then the server's
  record steers the character back. The same correction covers every other cell the client
  does not know about, such as one another unit stands on.
- **The server must send a wall's cells to a player who comes near later.** Today it sends
  them to the area when the wall is placed and to the whole map when it goes. Hercules also
  sends them with the wall's unit on view entry, as far as I remember its `clif.c`; a search
  of `eos-combat` and `eos-world` showed no such send. To check, and to add if it is missing:
  without it the hint stays wrong for as long as the wall stands.

## Attacks and skills

- **Swing:** an act record with the impact tick. The hit notice and the vitals record follow at
  impact. A move during windup produces an idle record and a move record, and the swing is
  gone.
- **Cast:** an act record from start to end with skill and target. An interrupt is a flinch or
  idle record before the end.
- **Skill pose:** the act record after a skill ran, with its length. This puts the pose length
  on the server, which `plans/action-timing.md` stage 2 needs anyway. It takes a table of pose
  kind and length per skill, taken from this client's data.
- **Flinch:** an act record for the damage motion, sent with the move record of the pause.
- **Own clocks:** the server says from which tick the character may swing, cast and move. The
  client draws cooldowns from them and may send early; the own-intent record then says "held
  until tick".
- **The own swing** starts at the click and is played to land on the server's impact: its
  windup is stretched by the round trip, so the hit frame and the damage number meet. At 60 ms
  that is a fifth longer on a 300 ms windup, and the act record corrects the estimate when it
  arrives. Built for the first swing of an order to attack (`world/own_swing.c`,
  `docs/custom-review.md`); the record confirms the swing and does not time it again. It extends the animation fit of `responsive-combat.md`, which scales the swing so
  its hit frame lands at the windup and which the action-timing stage left out.
- **The own intent**: what an order to attack makes of the player's own character is planned
  when the order leaves, by the rules the server runs (`world/own_attack.c`, the actor core):
  the walk into reach, the step under way that lands before a swing, and the swing that ends
  the walk. The plan's records of the walk go into the character's own track and its swing into
  the queue, at the ticks the server will give them, and play on the own clock; the server's
  records come all the same and take their place. While the order lasts the plan is made
  again when there is news: its target walks otherwise than the plan had it, or the server's
  record of the character's walk came. News that comes a moment after the plan was made is of
  that same moment (the server's word of it was on its way when the player clicked), so the
  plan is made again from the same base and takes back what the first one made of the
  character; later news is of now, and the plan goes on from where the character is. The
  order's refusal, its end and any other order of the player's take back what the plan still
  held. Built for an order to attack (`game_mode_own_order` in `game/packets_act.c`); skills
  and pickups are walked into reach by the server's records as before.

## Monsters

A monster is a body like a player's. Its AI is a source of intents: it sets "attack this
target" or "walk to this cell", and the same step walks it into range and acts. On the wire a
monster has the same tracks and nothing of its own.

- A chase is a chain of move records, one per new path, dated at cell boundaries like a
  player's turns.
- A hit on a walking monster is a flinch record and a pause record.
- Its swing is an act record. Whether its damage is decided at the start, as now, or at impact
  is a game rule; the wire carries either.

## Requests

Every request is an intent and gets exactly one visible state at a time: held, approaching,
acting or ended. Walking becomes intent kind 0, so one counter numbers all requests and the
stamp's `seq` is that counter. Nothing is dropped or delayed without the owner seeing it.

Order, which `plans/netcode.md` left open:

- A connection's requests apply in the order it sent them. While one is held back, the
  later moves, intents, sit and stand, and pickups of that connection wait behind it. Other
  packets (chat, windows) pass.
- Two connections' requests with the same press tick apply in arrival order, which is the
  journal's order and so the same in a replay.

## Packets (first cut)

Project id range, little-endian, packed. The server sends these in place of the classic
packets they take over from, to every client: there is no setting for it and nothing is
announced (`plans/predicted-world.md`, step 3a).

`ZC_TRACK_MOVE` 0x0F20, 23 to 35 bytes (built)

| off | size | field | note |
|---|---|---|---|
| 0 | 2 | id | |
| 2 | 2 | length | of the packet |
| 4 | 4 | gid | |
| 8 | 4 | at_tick | |
| 12 | 2 | x | the cell at `at_tick` |
| 14 | 2 | y | |
| 16 | 2 | land_ms | the first step lands this long after `at_tick`; 0 without steps |
| 18 | 2 | speed_ms | per straight cell |
| 20 | 1 | dir | facing while it stands |
| 21 | 1 | cause | low 7 bits: 0 walked or stopped, 1 pushed (drawn as a slide), 2 placed (drawn at once); top bit: made before it was sent, for a viewer who came later |
| 22 | 1 | steps | 0 to 32 |
| 23 | n | path | three bits a step, lowest bits first, the server's direction codes; (3 * steps + 7) / 8 bytes |

`land_ms` took the place of a wait: longer than the first step's own time, the actor stands
first; shorter, the step was under way. One field says both.

`ZC_TRACK_ACT` 0x0F21, 35 bytes (built for idle, swing and cast)

| off | size | field | note |
|---|---|---|---|
| 0 | 2 | id | |
| 2 | 4 | gid | |
| 6 | 4 | at_tick | |
| 10 | 1 | kind | 0 idle, 1 swing, 2 cast; not built: 3 skill pose, 4 flinch, 5 sit, 6 dead |
| 11 | 4 | end_tick | 0: until replaced |
| 15 | 4 | mark_tick | a swing's impact; 0 otherwise |
| 19 | 4 | target | GID, or 0 |
| 23 | 2 | x | ground skills |
| 25 | 2 | y | |
| 27 | 2 | skill_id | 0 for a basic attack |
| 29 | 2 | skill_lv | |
| 31 | 2 | motion_ms | length of the animation, which sets its speed |
| 33 | 1 | element | a cast's, signed |
| 34 | 1 | disposable | a cast's flag |

`ZC_TRACK_VITALS` 0x0F22, 18 bytes (built, for monsters): id, gid (4), at_tick (4), hp (4),
max_hp (4).

`ZC_NOTICE_HIT` 0x0F23, 32 bytes (built, for basic attacks): id, source (4), target (4),
at_tick (4), damage (4), left_damage (4), count (2), kind (1, as `action` in 0x08C8), sp_damage
(1), flinch_ms (2), skill_id (2), skill_lv (2). The target's damage motion is in the notice and
not a flinch record of its own: the client plays it with the number, and where the target
stands meanwhile is the move track's pause.

`ZC_OWN_INTENT` 0x0F24, 14 bytes (built): id, seq (2), state (1: 3, ended), reason (1, the
reasons of `plans/intents.md`), tick (4: legal again from), x (2), y (2): the player's cell on the
server. It is sent when an intent ends; what the intent makes of the player before that, the
player's body says (`ZC_OWN_BODY`). The client reads its seq and state.

`ZC_OWN_BODY` 0x0F25, 36 bytes and the body (built, `plans/predicted-world.md`), every field
on its own alignment:

| off | size | field | note |
|---|---|---|---|
| 0 | 2 | id | 0x0F25 |
| 2 | 2 | length | of the packet |
| 4 | 4 | version | the actor core's rules version |
| 8 | 8 | tick | the server's clock the body is of, whole |
| 16 | 2 | request | the number of the last request the body includes |
| 18 | 2 | request_steps | the first setting of the core's world |
| 20 | 4 | penalty_ms | the second |
| 24 | 4 | placement | the player's placement on a map the body is of (`plans/presence.md`, part 5) |
| 28 | 4 | arrival | the low 32 bits of the server's clock when the request it names arrived, when it names it anew; else 0 (`plans/netcode.md`) |
| 32 | 4 | outside | how many inputs the server's game put to the body that the client's core does not make itself, from the placement on (`plans/predicted-world.md`, "Told again") |
| 36 | n | state | the body as the core's bytes |

It travels through the delivery piece, the newest replacing every earlier one
(`plans/delivery.md`). It took the id planned for `ZC_OWN_CLOCKS`: the ticks from which the character may
swing and move are in the body.

Size per viewer: a swing is 67 bytes (act and hit), 85 when the viewer also gets the HP. The
action-timing stage as first built sent 55 (0x0F10 and 0x08C8). A walk of ten steps is 27 bytes.

## Server

- Spatial and combat already publish facts on channels, and `eos-world` folds them and renders
  packets. Tracks are rendered in `eos-world` from the same folded state. What the classic
  renderer sent for the same facts is not sent any more; it still renders what has no record
  yet (a monster's own walk, a sit, a stand, a pickup, the older skill handlers' casts).
- Needs from the producers: the steps and the tick with each walk start; an event for a
  delayed walk, which has none today; swing, cast and pose as state with start and end; the
  pose table.
- Monster AI as an intent source is a change inside the server and can come after the wire.

## Client

- A small store per actor: for each track the current record and a few waiting ones.
- `pose(records, tick)`: cell, offset, direction, sprite action and frame. Freestanding C with
  its own tests, next to `grid_path.c`.
- The message-driven state machine (`actor_state.c`, `actor_msg.c`) is what a played record
  drives, and what the classic packets the server still sends drive directly.

## Build order

0. Run what is built (`plans/netcode.md`, `plans/intents.md`, `plans/action-timing.md`) client
   against server. Everything here rests on its clocks. Login, map entry and walking ran on
   2026-11-10 and two clock faults were fixed (`plans/netcode.md`, "The first live run"). Still
   to run: attacks with the windup, intents, skills, pickups, a refused move, a second player,
   and all of it at a ping (`RAGEXE_LAG_MS`).
1. Move track for other actors: steps in the record, pauses, steering. This removes the
   position errors and proves the playout on the render clock.
2. Act, hit and vitals for basic attacks, players and monsters. `ZC_ACTION_BEGIN` and
   `ZC_ACTION_CANCEL` go (gone on both sides).
3. Own intent and own clocks, walking as an intent, the order rules. `ZC_MOVE_RESULT` and
   `ZC_INTENT_ENDED` go (gone on both sides).
4. Casts and skill poses, with the pose table. This settles the skill rate on the server.
5. The actor core (`plans/body-core.md`): the rules for where an actor is and what it does
   next as one freestanding C library, run by the server as the authority and by the client to
   predict its own character. It replaces the plan of sharing only the intent step, and takes
   over what steps 1 to 4 fitted into the code that was there.

## What is built

Client (`custom`) and server (ring-buffer worktree `local\worktrees\action-timing`), neither
committed. The server needs no setting for it (the server's `docs/tracks.md`).

| Step | Built | Not built or not covered |
|---|---|---|
| Fixes of "Seen in play" | The walk's own tick in its start packet; the exact turn for stamped moves; the client's apply rule; steering of the own walk | Not seen on screen |
| 1 Move track | `ZC_TRACK_MOVE` for walks, delayed walks and stops of other actors (and of the own character, see step 5), with the server's steps; the record of an actor seen mid-walk; chase walks on their steps' own ticks; the client's reader, queue, playing at the render clock, steering; the measured render delay | Knockbacks, slides and warps are the old packets. A stop is walked off by the client's own search. The own character gets no record. Rule 7's "no memory of what each viewer has" holds, rule 3's queue holds four records. Not run client against server |
| 2 Act, hit, vitals | `ZC_TRACK_ACT` for a swing and its cancel, `ZC_NOTICE_HIT` dated when the hit lands, `ZC_TRACK_VITALS` for monsters to every viewer; the client's queue, played on the render clock or the own clock; `ZC_ACTION_BEGIN`, `ZC_ACTION_CANCEL` and `ZC_NOTIFY_ACT2` for hits are not sent. Monster hits at impact (server's `docs/action-timing.md`) | Skill hits, casts, sit, stand and death are the old packets. Players' HP is the old packets. A swing seen from its middle has no record. Nothing cancels a monster's swing but its impact check. Not run client against server |
| 3 Own intent, own clocks | `ZC_OWN_INTENT` in place of `ZC_MOVE_RESULT` and `ZC_INTENT_ENDED`: a move's answer, and an intent's held, under way, acting and ended, each said once; one counter for moves and intents on the client | `ZC_OWN_CLOCKS` is not built: nothing would read it before step 4. The client acts on a walk's answer and on an end; held and acting are not shown. Walking is still `CZ_REQUEST_MOVE2`, which already ends an intent. The order rules were already the server's (press tick, then arrival). Not run client against server |
| 4 Casts and poses | A cast and its cancel as `ZC_TRACK_ACT` in place of `ZC_USESKILL_ACK` and `ZC_DISPEL`, played with the time the cast still has. The skill rate on the server: a player's act clock is held for one attack delay (`skill_pose_rate`) from when a skill with a pose is carried out, the cast's end for one with a cast time; the list of skills without one is the client's, held to its code by a test | The pose is not a record; the client plays it from the skill's packets. Skill hits are the old packets. The pose's length is a setting and not measured from the animations. A skill with a begin effect in the client's Lua data has a pose in the client whatever the list says. Not run client against server |
| 5 Actor core | The design, `plans/body-core.md`, and on the server the core's frame (5a) and plain walking (5b), which spatial runs; the client has the core's source and asks it where its own predicted walk starts; the owner gets and follows move records of its own character, and walks to within a range and after a unit are the core's; the basic attack's loop is in the core, and the server's combat runs its players' basic attacks through it; the client shows its own swing when the request for it leaves, if the core says it begins then, stretched to meet the damage number and taken back when the server does not confirm it | All of it off by default on the server and not played; the own swing not seen on screen, and only the swing a click begins at once. With `actor_attack` the skill code's casts and act clock and the players' intents are the core's too. Of 5d walking as an intent and the client's predicted intent are not built; 5e is not built |

Found on the way: a chase or range walk (a monster's, an intent's way into range) started
each step at the sweep that ran the one before, so a 150 ms step took 160 ms on the server
while the client drew 150. The fork counts from the step's own tick, and so does the server
now.

## Decided

Taken on 2026-11-10, with the aim Patrick gave: modern, robust, good to play and quick to
answer.

1. **Monster hits are decided at impact.** One rule for every body: a swing has a windup, and
   whoever is out of reach at impact is not hit. A monster's windup is its attack motion, so
   the damage shows when it does today. What changes is that walking out of reach in time
   avoids the hit. This changes how hard monsters are. Built: reach at impact is the
   monster's attack range and one cell.
2. **Monster HP goes to every viewer.** A party sees what it is fighting, and the server keeps
   no list of who hit what.
3. **The render delay is measured.** It follows the download jitter between a floor of 40 ms
   and a cap of 200 ms, slewed like the clock and never jumped. It starts at 100 ms until
   enough records have been timed. The latency rig sets the first numbers.
4. **The own swing starts at the click** and is stretched to land on the server's impact
   ("Attacks and skills").
5. **The four answer packets are run as built first** (build step 0) and folded into tracks
   in steps 2 and 3.

## What this does not do

- No whole-world snapshots. Records travel through a piece that sends each again until it has
  arrived or a newer one of its track has replaced it (`plans/delivery.md`), not in one stream in
  order.
- No rewind and no re-simulation on the server. The verdict of 2026-10-31 stands: press order
  and hold-back are the only things that make play fair. The actor core runs the player's own
  body forward again on the client from the server's state, which Patrick confirmed on
  2026-11-10 is outside that verdict (`plans/body-core.md`, decision 2).
