# The drawn place

How the place an actor is drawn at follows from its move records and its clock alone, so that
nothing can leave it where the records do not put it. Steps 1 to 5 are built; see "What is
built".

`plans/core-loop.md` has the records and rule 6 (a late walk is steered, never set),
`plans/netcode.md` the clocks, `plans/predicted-world.md` the own character's records.

## The problem

The server's body core gives a body's cell as one function of its state and the tick, and the
client runs the same core for the player's own body. What is drawn of an actor has to be such a
function too, or it drifts from what the server has.

- **A place kept by the animation states has a memory of its own.** If the walking state moves
  an actor a frame at a time and a stand, a swing, a flinch or a cast do not move it, a walk
  that a swing or a flinch ends between two cells leaves the actor there, and a stop that names
  the cell it is drawn on changes nothing. The actor then stands, swings and flinches off the
  middle of its cell until its next walk.
- **Every such case needs a rule of its own.** A late walk is steered in the walking state, an
  actor at rest by a rule beside the states, and a state the rules did not think of leaves the
  actor where it happened to be.

## What kind of problem this is

Drawing a thing whose true motion arrives as timed records that can be replaced. The fields
that have worked on it, and what is taken from each:

- **Snapshot interpolation (Quake III, Source).** What is drawn is a function of a buffer of
  timed states and the time it is drawn for. Nothing is added up from frame to frame. Taken:
  the place is a function of the record and the tick.
- **Smoothing a correction (Unreal's character movement, Rocket League, Overwatch).** The
  simulated state is set to the truth at once; what is drawn is the truth and an error that
  shrinks to nothing. Taken: the one thing the drawing remembers is how far it still is from
  the place.
- **Dead reckoning with convergence (distributed simulation).** After news, the drawn thing
  joins the new track over a short time at a bounded speed. Taken: the bound, which is rule 6's.
- **Animation over a capsule (every character controller).** The body's place is the game's;
  the animation is attached to it and never moves it. Taken: a state chooses the animation
  only.

## The design

### 1. The place a record gives at a tick

`world/move_place.c`, freestanding, integer math, with its own tests: from a record's path with
the time of each of its steps, and a tick, the place in 256ths of a cell.

- A record without steps: the middle of its cell.
- A record with steps: before its first step's time the middle of the cell it starts on;
  during a step on the line between the middles of the step's two cells, by the share of the
  step's time that has passed; after its last step the middle of its last cell.

The same record and tick give the same place whatever was asked before. The record is the one
the actor's track has played last on the actor's clock, as today.

### 2. What is drawn follows the place

Once a frame, for every actor, in whatever state it is: the drawn point takes one step toward
the place of this frame, of at most the real time since the last frame at 13/10 of the actor's
walk speed; a gap of more than four cells is taken at once. The bound is by real time and not
by how far the actor's clock moved: a clock moves further in one frame when its lead grows,
and the place may then be a cell on, which is walked off like any other gap. That is
`move_follow` in `world/move_place.c`, beside `move_place`, and `game_actor_play_place`
(`actor/actor_state.c`) calls it once a frame for every actor.

So what is drawn is the place and an offset that only shrinks. While the offset is nothing the
actor is drawn exactly where its record has it. A record that puts the actor elsewhere makes
an offset, which is walked off: this is rule 6 of `plans/core-loop.md`, now the only way a
drawn point moves.

### 3. A state chooses the animation, never the place

- A standing actor whose drawn point moves, walks; a walking actor whose place rests and whose
  drawn point has reached it, stands.
- A swing, a flinch, a cast, sitting and death are not ended by a move record and hold nothing:
  the drawn point follows the place under them.

### 4. Every change of place is a record, and its cause says how it is joined

Nothing writes an actor's place but its record. A slide and a knockback are changes of place
the server makes, so the server sends a record for each: without steps, on the cell the actor
was moved to, dated at the tick the server moved it, with a cause that says it was pushed.
Without that record the latest record would still name the cell from before the push, and the
drawn point would walk back to it.

The cause of the record the place comes from says how the drawn point joins it:

- **walked off**, the bounded step of part 2: a walk, a stop, a correction;
- **slid**, over the slide's own short time: a push;
- **set at once**: an entry, a warp, a placing, and any gap of more than four cells.

The original's packets that place an actor without a record (a move, a stop, a jump, a hit
onto a cell) are turned into records where they are read, so that a stream of them, a replay
among them, is drawn by the same code. Such a stream is in order, so a record made from it is
dated at the actor's clock when it is read. This server sends a record in their place.

### 5. The player's own character

The same two parts. Its records are the predicted world's and its clock is the own clock
(`game/own_body.c`). A correction of the predicted world is a new record like any other: an
offset that is walked off.

### 6. Nothing else writes the place

No animation state steps an actor, there is no rule for an actor at rest, no mark says a walk is
steered or waits for its first step, the client searches no path of its own for another
actor's move packet, and no packet's reader writes an actor's place. `game_actor_set_place`
(`actor/actor_state.c`) is the one way a record becomes the place.

## Constraints

- The server sends a move record for a slide and a knockback, and no longer the original's
  packets that placed the actor for it. No packet's layout changes.
- Which animation plays for which state stays the original's.
- The piece is freestanding C that a 32-bit MSVC build without `/std:c11` compiles.
- One path: the old way of moving an actor goes in the same change.

## What can be shown to hold

1. **The place is a function.** `tests/test_move_place.c`: the same record and tick give the
   same place in any order of asking; a record without steps gives the middle; at each step's
   time a walk gives that cell's middle; between two ticks the place moves no further than
   the walk's speed allows; and for the same walk the body core enters each cell at the time
   the place is on its middle.
2. **The drawn point arrives.** From any drawn point and any sequence of records, filed in any
   order and more than once, the drawn point is on the place once the gap divided by 13/10 of
   the walk speed has passed without news, and no frame's step is longer than the bound unless
   the gap is over four cells.
3. **On screen.** The playground judges every actor by this function: the drawn point less the
   place of the server's record at the tick the actor is drawn on is the offset. It is nothing
   for an actor that has rested for the catch-up time, monsters and the own character, in
   every set, with loss too; and no finding shows that did not show before.
4. **A push is a record.** A session test of the server: a slide and a knockback each send a
   move record without steps on the cell the unit was moved to, dated at the tick it was
   moved, and the original's placing packets are not sent for it. On screen an actor that was
   pushed rests on the cell it was pushed to and does not walk back.
5. **Deliberate faults.** Each rule above broken on purpose is caught by a test.

## Steps

| Step | What | Check |
|---|---|---|
| 1 The place | `world/move_place.c` and its test | 1 |
| 2 Following | The one step a frame in every state; a record only becomes the track's record; what goes (part 6) goes | 2 |
| 3 The states | The animation is chosen from the place and the drawn point; swing, flinch, cast, sitting and death hold nothing | 3 |
| 4 Every place a record | The server's record for a slide and a knockback; the cause chooses how the drawn point joins; the original's placing packets become records where they are read | 3, 4 |
| 5 The measure | The playground judges by the place | 3, 5 |

## What is built

Client branch `claude/netcode-fight-clock-315b2d` and the server's branch `fight-clock`
(ring-buffer), neither merged.

| Step | Built | Not built or not covered |
|---|---|---|
| 1 The place | `world/move_place.c`: `move_place`, the place a record gives at a tick in 256ths of a cell, and `move_follow`, the drawn point's step. `tests/test_move_place.c`, built without `/std:c11` (`tests/build_move_place.bat`): the place in any order of asking, the middles, the speed between two ticks, the body core entering each cell when the place is on its middle, and check 2 over 3,000 random runs of walks and stops coming at random frames. Nine of their rules broken on purpose were each caught | |
| 2 Following | `game_actor_play_place` (`actor/actor_state.c`) plays the track and follows the place once a frame in every state; `game_actor_set_place` is the one way a record becomes the place. Part 6 holds | |
| 3 The states | The animation is chosen from the place and the drawn point; a swing, a flinch, a cast, sitting and death hold nothing. Setting a state takes no snap | |
| 4 Every place a record | The server: a slide, a knockback and a body relocation send a move record without steps, cause pushed, dated at the tick spatial moved the unit, and none of the original's placing packets (`eos-world/aoi-render.js`); the player's own body is placed as pushed and she is told it at once. The tracks session test: a knockback sends that record, and the own client has her body on the new cell, pushed, within 60 ms. The client: the cause chooses how the drawn point joins, and the original's placing packets become records where they are read | A body relocation is held by the slide's path, not by a session of its own. The own character pushed by a monster's skill was not seen on screen |
| 5 The measure | The playground's `places.js` mirrors `move_place`, and `run.mjs` judges every actor by the place of the server's records at the tick it is drawn on. A slide is allowed for its 200 ms and a frame and a half. The driver can use a skill on a body; the mood `magnum` knocks monsters back | Not played |

Check 3, seven sets of four 40 s runs with Zombies at hold-back 0 (no lag, ping 80 and 150, ping 80
with 30 of 100 datagrams lost, and the three with the mood `strike`): no finding on screen, and
no actor off its place at rest but once, in the set with loss. There the own character was 0.53
cells off for 96 ms. It is drawn on the predicted body's place, which the server's body left: a
Zombie's swing landed 14 ms before the tick of a walk the client had predicted, the server rolled
a hit, and the hit held the body's walk for 150 ms. A hit's outcome is the server's roll, so the
predicted body has it only with the server's next word. In the set `magnum`, with 18 pushed
records in its first run, no finding and nothing off at rest.

## What this does not do

- It does not draw anyone ahead of what is known. An actor on the render clock is still drawn
  where it was a moment ago.
- It does not remove corrections. When the server's record differs from what was drawn, the
  drawn point is still off for the time it takes to walk the gap off; it can no longer stay
  off.
