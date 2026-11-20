# The drawn place

How the place an actor is drawn at follows from its move records, its clock and the real time
alone, so that nothing can leave it where the records do not put it. It is built; see "What is
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
- **A step a frame depends on the frames.** A drawn point that takes one bounded step each frame
  moves by what each frame's division leaves: at very short frames it stalls, and the same real
  time gives another distance cut into other frames.

## What kind of problem this is

Drawing a thing whose true motion arrives as timed records that can be replaced. The fields
that have worked on it, and what is taken from each:

- **Snapshot interpolation (Quake III, Source).** What is drawn is a function of a buffer of
  timed states and the time it is drawn for. Nothing is added up from frame to frame. Taken:
  the place is a function of the record and the tick.
- **Smoothing a correction (Unreal's character movement, Rocket League, Overwatch).** The
  simulated state is set to the truth at once; what is drawn is the truth and an error that
  shrinks to nothing over a set time. Taken: a push is the place and an offset that shrinks over
  the push's time.
- **A fixed time step with an accumulator (game loops, physics engines).** Real time is added
  to an accumulator and spent in whole units, and what is left waits for the next frame. Taken:
  the walked point's reach is real time spent in whole 256ths, and the rest is carried.
- **Dead reckoning with convergence (distributed simulation).** After news, the drawn thing
  joins the new track over a short time at a bounded speed. Taken: the bound, which is rule 6's.
- **Animation over a capsule (every character controller).** The body's place is the game's;
  the animation is attached to it and never moves it. Taken: a state chooses the animation
  only.

## The design

One piece, `world/drawn.c` (`world/drawn.h`): freestanding integer C like the body core, with no
clock, no float, no library, no allocation and no 64-bit product, which a 32-bit MSVC build
without `/std:c11` compiles. Its state is one block per actor (`struct Drawn`): the record the
place comes from, the drawn point with the share of a 256th beyond it, the reach not yet taken,
and a slide's offset and time. The actor feeds it and reads it, and does nothing else with where
it is.

### 1. The place a record gives at a tick

`drawn_place`: from a record's path with the time of each of its steps, and a tick, the place in
256ths of a cell.

- A record without steps: the middle of its cell.
- A record with steps: before its first step's time the middle of the cell it starts on;
  during a step on the line between the middles of the step's two cells, by the share of the
  step's time that has passed; after its last step the middle of its last cell.

The same record and tick give the same place whatever was asked before. The record is the one
the actor's track has played last on the actor's clock.

### 2. The drawn point joins the place

`drawn_take`: a record becomes the actor's place at the actor's tick, with its walk speed. The
drawn point stays where it is, and the record's cause decides how it joins the new place:

- **walked** (a walk, a stop, a correction): each frame the point steps toward the place of the
  frame, at most 13/10 of the walk speed for the frame's real time, so it never moves faster than
  that, whatever the place does; a gap of more than four cells is taken at once. A record with
  steps is joined at its own speed;
- **slid** (a push): the point is the place and the offset it had at the push, which shrinks to
  nothing over the push's time, whatever its length up to 32 cells, the most whose products stay
  in 32 bits; a longer one is taken at once;
- **at once**: a placing, and an actor's first record.

`drawn_frame`, once a frame with the actor's tick and the real time: a walked point's reach is
the frame's real time counted in parts of a 256th; it is spent in whole 256ths, and the part not
yet a whole 256th, and the share of a 256th a step moves the point by, are carried to the next
frame. So the same real time reaches as far however it is cut into frames. A point on its place
keeps up to two 256ths of reach waiting, so that it takes the place's next 256th with it and a
point on a walking place stays on it. A clock that moves further than the frame's real time (the
own clock's lead grows, the render clock is corrected) moves the place, and the point walks after
it like after any other gap, never jumping with the clock. The frame gives the place, the drawn
point, how far the point moved (its heading), whether it rests, walks or slides, and whether it
is on its place.

### 3. A state chooses the animation, never the place

- A standing actor whose point walks, walks; a walking actor whose point no longer walks, stands.
- A slide is drawn under whatever the actor does: a knocked back actor flinches while it slides.
- A swing, a flinch, a cast, sitting and death are not ended by a move record and hold nothing:
  the drawn point follows the place under them.

### 4. Every change of place is a record, and its cause says how it is joined

Nothing writes an actor's place but its record. A slide, a knockback and a move a skill makes
(Asura Strike, Body Relocation, Charge Attack) are changes of place the server makes, so the
server sends a record for each: without steps, on the cell the actor was moved to, dated at the
tick the server moved it, with a cause that says it was pushed. Without that record the latest
record would still name the cell from before the push, and the drawn point would walk back to
it. A unit other than a player that the server sets on another cell of its map gets such a record
with a cause that says it was placed; a player who is set elsewhere loads the map anew.

A push slides over the original's knockback time, 200 ms. After Asura Strike, whose motion the
original follows by a dash at 55 ms a straight cell, a push slides at that speed along its
length, and the strike's end motion plays when the slide is over.

### 5. The player's own character

The same piece. Its records are the predicted world's and its clock is the own clock
(`game/own_body.c`). A correction of the predicted world is a new record like any other: an
offset that is walked off.

### 6. Nothing else writes the place

No animation state steps an actor and there is no rule for an actor at rest. The actor's float
position is written from the piece's drawn point each frame and never read back into it; whoever
asks where an actor is (`game_actor_on_place`, the swing's wait for a walk, the predicted world's
other bodies, the playground's driver) asks the piece. `game_actor_set_place` (`actor/actor_state.c`)
is the one way a record becomes the place.

The original's packets that place an actor without a record and that this server does not send
are consumed without effect: `ZC_NOTIFY_MOVE`, `ZC_NOTIFY_PLAYERMOVE`, `ZC_STOPMOVE`,
`ZC_STOPMOVE_FORCE`, `ZC_HIGHJUMP`, `ZC_FASTMOVE`, and the two hits onto a cell,
`ZC_NOTIFY_ACT_POSITION` and `ZC_NOTIFY_SKILL_POSITION`. The entries it does send are turned
into records where they are read: an entry is a placing on its cell, a walking entry on the cell
its walk starts on, and the server sends the walk's own record after it. The client searches no
path of its own for any actor.

## Constraints

- The server sends a move record for a slide, a knockback and a body relocation, and no longer
  the original's packets that placed the actor for them. No packet's layout changes.
- Which animation plays for which state stays the original's.
- The piece is freestanding C that a 32-bit MSVC build without `/std:c11` compiles, and that
  clang builds to wasm for the playground's runner.
- One path: the old way of moving an actor goes in the same change.

## What can be shown to hold

1. **The place is a function.** `tests/test_drawn.c`: the same record and tick give the same
   place in any order of asking; a record without steps gives the middle; at each step's time a
   walk gives that cell's middle; between two ticks the place moves no further than the walk's
   speed allows; and for the same walk the body core enters each cell at the time the place is
   on its middle.
2. **The drawn point arrives, whatever the frames.** For walk speeds of 100 to 2000 ms a cell and
   frame times of 1 to 50 ms, and mixed: a point on a walking place stays on it at every frame;
   a gap is walked off in the same real time, within one frame, and no frame's step is longer
   than 13/10 of the speed allows, a place that walks on included; a walked gap of more than four cells is at once; a push of 6
   and of 18 cells is joined over its time and not in one frame, one of more than 32 at once; a
   jump of the clock is walked off and not drawn; records filed through a move track in any
   order and more than once end with the point on the newest's place; and a fuzz run of
   records, ticks and frame times gives the point of a slow reference written in doubles.
3. **On screen.** The playground's runner judges every actor by the place the client's own piece
   gives, built to wasm beside the playground's client: the drawn point less the place of the
   server's record at the tick the actor is drawn on is the offset. It is nothing for an actor
   that has rested for the catch-up time, monsters and the own character; and no finding shows
   that did not show before.
4. **A push is a record.** Session tests of the server: a knockback, by a skill's blow and by a
   count of cells, sends a move record without steps on the cell the unit was moved to, dated at
   the tick it was moved, pushed, and the original's placing packets are not sent for it; a
   monster set on another cell of its map sends one placed. On screen an actor that was pushed
   rests on the cell it was pushed to and does not walk back.
5. **Deliberate faults.** Each rule above broken on purpose is caught by a test.

## What is built

Client branch `claude/netcode-fight-clock-315b2d` and the server's branch `fight-clock`
(ring-buffer), neither merged.

| Part | Built | Not built or not covered |
|---|---|---|
| The piece | `world/drawn.c` and `world/drawn.h`: `drawn_place`, `drawn_take`, `drawn_frame`, `drawn_record`, `drawn_on_place`. `tests/test_drawn.c`, built without `/std:c11` (`tests/build_drawn.bat`), holds checks 1 and 2. Nineteen of the piece's rules broken on purpose were each caught | |
| The actor | `struct Drawn place` in the actor; `game_actor_set_place` feeds it, `game_actor_play_place` reads it once a frame and writes the actor's position from it, and the animation follows its motion. The actor's flag for Asura Strike's dash names what sets it | |
| The original's placing packets | The eight that this server does not send are consumed without effect (part 6); their readers, the actor messages only they sent and the client's path search for an actor are gone | |
| The server | A slide, a knockback by a blow or by a count of cells, and a move a skill makes are pushed records, and a monster set on another cell of its map a placed one (`spatial/module.js`, `eos-world/aoi-render.js`); the player's own body is placed as pushed and she is told it at once. The tracks session tests hold each, and that the own client has her body on the new cell, pushed, within 60 ms | A body relocation slides over the knockback's time, not at the original's dash of 15 ms a cell: the record does not say which push it is |
| The measure | `tools/build_drawn_wasm.bat` builds the piece's `drawn_place` to `drawn.wasm`, and the playground's build puts it beside its client. The runner loads it, holds it to the core's step times before each run, and judges every actor by it (`places.js`). The driver reports each actor's drawn point in 256ths from the piece | Not played |

Check 3, eight sets of four 40 s runs with Zombies at hold-back 0 (no lag, ping 80 and 150, ping 80
with 30 of 100 datagrams lost, the three with the mood `strike`, and `magnum`, which knocks
monsters back): no finding on screen, and no actor off its place at rest but once, in the set with
loss. There the own character was 0.53 cells off for 96 ms: it is drawn on the predicted body's
place, which the server's body left when a Zombie's swing landed 14 ms before the tick of a walk
the client had predicted, the server rolled a hit, and the hit held the body's walk for 150 ms.
The predicted body has a hit only with the server's next word.

## What this does not do

- It does not draw anyone ahead of what is known. An actor on the render clock is still drawn
  where it was a moment ago.
- It does not remove corrections. When the server's record differs from what was drawn, the
  drawn point is still off for the time it takes to walk the gap off; it can no longer stay
  off.
- It does not foresee what the server decides. The own character is drawn on the predicted
  body, and an outcome the server rolls, such as a hit that holds a walk, reaches the predicted
  body only with the server's next word.
