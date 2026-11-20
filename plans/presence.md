# Presence

How an actor's coming into view and its going take part in the order of its records, so that
records, hits and the player's own body can travel in any order and the screen still shows what
the server had. Status, 2026-11-19: all five steps are built on both sides; see "What is built".

`plans/delivery.md` has the records' merge and the delivery piece, `plans/core-loop.md` what a
record is, `plans/predicted-world.md` the own body and the predicted world.

## What is wrong today

A unit's records travel through the delivery piece, in any order. Its coming and its going are
the classic entry and vanish packets, in order on the world channel. Nothing orders the two
paths against each other (`plans/delivery.md`, "Not solved"), and the client keeps nothing that
could.

- **The floors die with the actor.** What a client has played of a unit, as revisions, is a
  member of the actor (`record_floors`, `actors/types.h`) and is zeroed for a new one
  (`actors/game_object.c`). A record made before a unit went, arriving after it came back, is
  newer than nothing and is played: the unit is shown where it was.
- **A record that overtakes the going is lost.** A record made after a unit came back that
  arrives before the vanish of its earlier stay is filed on the actor that is about to go, and
  goes with it.
- **The act and HP queue never loses a gone actor's items.** It is one queue of the game mode,
  by unit id (`game/gamemode.h`, `tracks`), and a vanish removes nothing from it, so an item of
  a unit that went is played on the unit that comes back under the id.
- **An entry brings only a walk.** A unit seen from the middle of a swing or a cast has no act
  record, though rule 7 of `plans/core-loop.md` says it arrives with the current record of
  every track (the server's `eos-world/aoi-render.js`, `ENTER_VIEW`).
- **The predicted world dates a going by the client.** Another body leaves the predicted world
  at the client's own clock when the vanish is read, and whether a word of the server is of a
  world with that body is told by which arrived first (`world/predicted.h`, `predicted_gone`).
  So the own body has to arrive in order with the world channel, where one lost datagram holds
  it back with everything else, and a body that died is kept in the predicted world a round
  trip longer than the server had it: the own character may begin another swing at it.

## What kind of problem this is

State that is replicated over paths without a common order, where a thing can be deleted and
made again under the same name. The fields that have worked on it, and what is taken from each:

- **Replicated data types with tombstones.** A delete is a write with a version of its own, and
  it is kept after the thing is gone, so an older write that arrives late cannot bring the
  thing back. Taken: a going sets floors that outlive the actor.
- **Entity handles with a serial number (Source, and most entity systems).** A reused slot is
  told from its earlier owner by a number that counts up. Taken: the revisions at a coming and
  at a going say which stay a record belongs to. No new counter is needed, the tracks have one.
- **Unreal's actor channels.** A channel's opening and closing are in order with that actor's
  updates and with nothing else. Taken: order within one unit only, never across units.
- **Databases with versions of a row.** A row is seen by the snapshots between the tick it was
  made and the tick it was deleted. Taken: a body is in the predicted worlds of the ticks
  between its coming and its going, whatever arrived first.

## The design

### 1. A stamp on every coming and going

`ZC_VIEW_STAMP` 0x0F26, 17 bytes, the project's own, sent on the world channel directly before
the entry or vanish packet it stamps, as `CZ_INPUT_STAMP` goes before a request:

| off | size | field | note |
|---|---|---|---|
| 0 | 2 | id | 0x0F26 |
| 2 | 4 | gid | the unit |
| 6 | 4 | tick | the server tick the unit is in this viewer's world from (comes), or no longer in it from (goes) |
| 10 | 1 | kind | 0 comes, 1 goes |
| 11 | 2 | rev_move | the unit's revision of each track at that moment |
| 13 | 2 | rev_act | |
| 15 | 2 | rev_vitals | |

The server's renderer has all of it: the revisions are its count by unit id, which outlives the
unit (`plans/delivery.md`, part 1), and the tick is its own. The stamp is the same bytes for
every viewer the packet goes to. The server still keeps no memory of what a viewer has. An
entry that is held back until its player has loaded the map is stamped when it is sent, with
the revisions of that moment. The original's entry and vanish layouts are untouched.

A going's tick is the tick the server's core lost the body, taken from the core and not from
the renderer's clock, so that the two are one number by construction. A unit that only leaves
the viewer's sight, or a GM who hides, keeps its body in the core and goes at the tick the
viewer stopped being told of it. A vanish that leaves the unit in the viewer's world has no
stamp: a player who dies stays where she fell, in the server's core and on the screen, so her
death is no going. An entry or a vanish the server sends for a script (an NPC shown or hidden)
is rendered by the renderer like any other and is stamped.

### 2. Floors belong to the unit id

`world/view_floors.c`, freestanding, with its own tests: for a unit id, the floor of each track
and the ticks of its last coming and going. It takes the place of the floors on the actor, so
there is one home for them. It is bounded; the entries of units that are in view are never the
ones to leave, and a map load empties it.

- **Comes.** The floors become the stamp's revisions: a record older than them is dropped, one
  equal or newer is taken. A notice (a hit) dated before the stamp's tick is dropped.
- **Goes.** With the stamp's revisions r: whatever the actor holds with a revision up to r goes
  with the actor, and so do the unit's items in the act and HP queue. What it holds that is
  newer than r belongs to a later stay and waits by unit id (`world/record_wait.c`, as a
  record without an actor waits today). The floors become "newer than r".
- **A record is filed** against the floors of its unit id, wherever it comes from: off the
  wire, or out of the waiting list when its actor appears. Playing a record raises the floor,
  as now.

In any order of the stamps and the records of a unit, with repeats, the unit's tracks end as
they do in the order the server made them.

### 3. A body is in the predicted worlds between its ticks

`predicted_gone` takes the stamp's tick. A world made from a word of the server holds another
body when the word's tick is before that body's going, and the rule by arrival goes. A going
dated behind the world's clock is a change of the past, as a record dated behind it is: the
world is made again from the server's last word, with the body until its tick and without it
after. What is due at a tick runs before a going of that tick, as the core runs what is due
before an input, so a swing that lands at the tick its target dies lands.

What that buys beside the order: the predicted world loses a dead target at the tick the
server lost it. A swing the client began at it after that tick is taken back when the vanish
is read, without waiting for the server's next word.

### 4. An entry brings what is under way

The renderer sends, with an entry, the unit's act record when it is not idle (an open swing, a
cast), as it sends the walk's record today. The record is marked as made before it was sent
by the top bit of its kind, as the top bit of a move record's cause marks that one: such a
record is played at once and is no measure of the render delay. Rule 7 of `plans/core-loop.md`
then holds for the act track. HP is in the entry packet.

### 5. The own body through the delivery piece

With part 3 the own body is ordered by its tick against everything the predicted world reads:
other words, the requests' ticks, the others' records, the goings. `ZC_OWN_BODY` enters the
route table as replaceable, one key for the connection, so the newest replaces all, a lost one
costs one sending and none waits behind another packet.

One guard is needed that the world channel gave for free: a body of the map the player left
must not pass for one of the map it is on. The body carries the number of the player's
placement, counted by the server each time the player is put on a map (once when she comes
into the game, once at each warp), and `ZC_HOLD_BACK`, which is told once a map and in order,
names the number of this map. A body that comes before the number is kept aside, the newest of
them, and taken once the number is known and is its own; a body with another number is refused.
The ticks of two maps' units cannot stand in for the number: each map is a unit with a clock of
its own, which moves only in the unit's frames and goes on from its last commit after a
restart, and two hosts' clocks are not related.

## Constraints

- The original's packet layouts stay. The stamp is a packet of the project's own beside them.
- The server keeps no memory of what a viewer has (rule 7 of `plans/core-loop.md`).
- The journal replays bit-equal: the stamp is rendered from state, as records are.
- The delivery piece is not changed and still knows nothing of what it carries.

## What can be shown to hold

1. **Any order, across stays.** `tests/test_view_floors.c`: the stamps and records of a unit
   over several stays, filed in every order of a seed and with repeats, leave the tracks and
   the queue as filed in the order they were made.
2. **The predicted world in any order.** `tests/test_predicted.c`: the convergence test with
   the server's words, the others' records and the goings handed over in any order and more
   than once gives the world it gives in order, at every tick compared. Among its cases: the
   own swing kills its target, another's hit kills it a tick before the own swing lands, a
   target goes and comes back within one round trip.
3. **The server says it.** A session test: every entry, and every vanish that takes the unit
   out of the viewer's world, has its stamp directly before it; a player's death has none; the
   stamp's revisions are the renderer's counts; and for every body the core lost, a going of
   that unit carries the tick it was lost at.
4. **On screen.** The playground with Zombies, 30 of 100 datagrams lost and 10 repeated, shows
   no finding it does not show on a wire that loses nothing, and has a check of its own for an
   actor that one side has and the other does not for longer than the line's delay.
5. **The body is not late for a loss.** The measurement of `tests/measure_requests.c`, made
   for the own body: how late a word is per loss, through the piece and on the world channel.
6. **Deliberate faults.** Each rule above broken on purpose is caught by a test.

## Steps

Each step ends with its check passing and leaves the game playable.

| Step | What | Check |
|---|---|---|
| 1 The stamp | The server's renderer sends `ZC_VIEW_STAMP`; the client's packet table knows it and reads it bounded | 3 |
| 2 Floors by unit id | `world/view_floors.c`; coming and going as part 2 says; the floors leave the actor; the queue loses a gone unit's items | 1, 4 |
| 3 Dated goings | `predicted_gone` with the tick; the rule by arrival goes | 2 |
| 4 Entries bring acts | The act record of an open swing or cast with an entry | 3, 4 |
| 5 The own body on the piece | The placement number, the route table's row, `plans/delivery.md` step 5 closed | 4, 5 |

Steps 1 and 4 are the server's and can be built together. Step 2 needs 1, step 3 needs 1,
step 5 needs 3.

## What this does not do

- It does not make the entry packet a record. An actor's look still arrives in order on the
  world channel, and a lost datagram there still holds back what follows it.
- It does not tell a viewer what a unit did while it was out of view.

## What is built

All five steps, on both sides: the client's `custom` and the server's ring-buffer. Steps 1 to 4
are as the design says; step 5:

| Step | Built | Not built or not covered |
|---|---|---|
| 5 The own body on the piece | The route table's row for `ZC_OWN_BODY` (replaceable, the connection's one key, first in priority), the same bytes in the server's `net/delivery` and here. The placement: counted by the server's `eos-world` (`aoi-render.js`), carried with the player to the next map's unit and through a snapshot; `ZC_OWN_BODY` and `ZC_HOLD_BACK` carry it (their layouts in `plans/core-loop.md` and `plans/netcode.md`); the predicted world keeps a body that comes before the number aside and refuses one of another placement (`predicted_placed`). The playground's check 4 is a finding of its own kind, `presence` (the server's `consumers/host/playground/presence.js`, the client's view of a unit in `tests/playground/play_driver.c`). Checks: `tests/test_predicted.c`, `tests/test_delivery_route.c`, the gateway's records test (of two bodies sent in one tick only the newer travels), the server's session and render tests; 15 deliberate faults each caught. Check 5 through the piece, the playground with Zombies, four runs of 40 s: with 10 of 100 datagrams lost a word was 12.6 ms late on average from the gateway's end being handed it to the client's end handing it on (623 words), with 30 of 100 37.1 ms (636 words); 1,904 ms at worst in both; the one such case looked at was the first word of its run, sent before the piece had measured a round trip. No presence finding in any of the playground's sets | Check 5 on the world channel: the playground's wire loses nothing there. A warp between two maps of one host was played in a session test only, not in the playground, whose player stays on one map |
