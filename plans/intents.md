# Intents

"Walk into range, then act" moves from the client to the server. This is
`herc-client-integration/plans/proceed-rework.md` (2026-10-30), restated for the ring-buffer
server and the packets this client already has. Status, 2026-11-09: built on both sides and
passing their test suites, never run client against server. See "What is built" at the end.
Since 2026-11-12 intents are the only way (`plans/predicted-world.md`, step 3a): the server has
no setting for them and announces nothing, and the client's own chase is gone for what an
intent covers.

## The idea

- **Intent:** what the player wants. One value per player, set only by input; the newest wins.
- **Body:** what the character is doing, read from the server's own timers and state.
- **Step:** one function without state that compares the two and says what to do next.

The client sends the intent once and draws what happens. It no longer chases targets, checks
ranges or reads its animation to decide when to send.

## What this round covers

Attack, skill on an actor, skill on the ground, pickup; a skill unit is a target as an actor
is. Talking to an NPC and orders to pets, homunculi and mercenaries stay on their present
paths.

## Packets

Little-endian, packed, in the project's id range. The server always reads `CZ_INTENT`.

`CZ_INTENT` 0x0F02, 18 bytes

| off | size | field | note |
|---|---|---|---|
| 0 | 2 | id | 0x0F02 |
| 2 | 2 | seq | counts up per intent, wraps, never 0 |
| 4 | 1 | kind | 1 attack, 2 skill on an actor, 3 skill on the ground, 4 pickup, 5 release, 6 cancel |
| 5 | 1 | mode | attack only: 0 repeat, 1 hold, 2 once |
| 6 | 4 | target | GID of the actor, or the ground item's id for a pickup |
| 10 | 2 | x | ground skill only |
| 12 | 2 | y | |
| 14 | 2 | skill_id | skills only |
| 16 | 2 | skill_lv | |

A `CZ_INPUT_STAMP` comes directly before it, as before any request, and the intent has the
stamp's number.

The end of an intent is `ZC_OWN_INTENT` 0x0F24 (`plans/core-loop.md`) with the intent's number,
the state ended, a reason, and the server tick from which the same intent would be legal (0
when that is not the point). The id 0x0F15, which carried the end before that record did, is
not used.

Reasons: 0 done (the act happened and the intent does not repeat), 1 the target is gone or
cannot be acted on, 2 the character cannot act (dead, a hard status), 3 the act's clock is too
far ahead (`next_tick` says when), 4 no path into range, 5 the act itself was refused (the
existing failure packet, if there is one, is still sent).

It is sent whenever the server ends an intent on its own, and when a release ends a Hold (reason
0), since the client cannot know whether the attack had swung. A release carries the number of
the intent it releases. It is not sent when the player replaces the intent, cancels it, sends a
walk, or sends a plain attack, skill, pickup or stop request, each of which ends the intent
too.

## Rules

Attack modes, as the mouse works today:

- **Repeat** (Ctrl-click, `/noctrl`): attack until something ends it.
- **Hold:** repeat while the button is down. At the release it ends if a swing has started
  since the intent was set, and becomes Once otherwise.
- **Once:** one swing, then done.

Body, checked in this order, from state the server already has:

| Body | Walk toward the target | Act |
|---|---|---|
| Dead, disabled (stun, freeze, stone, sleep and the like) | no, the intent ends (reason 2) | no |
| Sitting | stand first | stand first |
| Casting | no, wait | no, wait |
| Swing in windup | the intent's own walk waits for the impact | a new intent on another target or a skill cancels the swing |
| Walking, idle | yes | yes |

Step, each time it runs:

1. Target gone or not actable: end (reason 1).
2. Body dead or disabled: end (reason 2). Sitting: stand, and run again.
3. In range, by the same test the act itself uses: if the act's clock is ready, act. If the
   clock is ready within 300 ms, wait. If further away, end (reason 3 with the tick).
4. Out of range: if the body may walk, walk to the first cell on the way to the target from
   which the act's own range test passes, line of fire included. Never a cell further from the
   target than the character is. No such cell or no path: end (reason 4).

When Step runs: when the intent is set, on each cell the character enters, when a repeating
attack finds its target out of range, and every 100 ms while an intent is alive. A new walk
target is chosen at most every 300 ms. Being in range acts at once.

What ends an intent without a word to the client: a newer intent, a cancel, a walk request, a
plain attack, skill, pickup or stop request, a map change, logout.

What the act is, per kind: the server's present handling of the matching request (attack or
continuous attack, skill on a target, skill on a cell, item pickup), entered at the point after
range chasing, so every other check and every failure packet stays as it is.

An attack intent stays alive while it repeats; the server's own attack loop does the swings,
and Step only brings the character back into range. A skill or pickup intent ends when the act
was started or refused.

## Client

- A click on a monster, a skill on an actor or the ground and a click on an item send one
  `CZ_INTENT`, and so do a skill aimed at a skill unit and an attack on one. Releasing the
  mouse button sends the release. The client's own walk into reach (the proceed logic of
  `actors/player.c`) is removed.
- The end of the newest intent (`ZC_OWN_INTENT`, ended) clears the attack cursor state.
- Walking is unchanged: a walk request, which the server takes as the end of the intent.

## What is built

Server: ring-buffer worktree `local\worktrees\action-timing`, described in that worktree's
`docs/intents.md`. It needs no setting; run the gateway built in that worktree, which frames
`CZ_INTENT`.

Client: `CZ_INTENT` from the attack, skill and pickup messages of the player, the release from
the mouse button, the end of the newest intent clearing the attack cursor. Each goes into the
predicted world as a request of the actor core (`plans/predicted-world.md`).

Where the server differs from the rules above:

- The step, reach and the walk into range are the actor core's: the walk goes with the act's
  range, one cell shorter for each walk that arrived out of range (a wall in the line of fire).
  The step does not pick the first cell of a path from which the act's own test passes.
- "Disabled" is the stun and Blade Stop tests the action handlers use. Other statuses that stop
  acting reach the act, whose own checks refuse it (reason 5).
- A walk that fails while the intent walks ends it with reason 2 when the world is frozen and
  reason 4 otherwise, a movement lock included.
- After eight walks that did not reach range the intent ends (reason 4).
- A skill counts as carried out when it set the act clock or started a cast.
- Not covered: talking to NPCs, orders to companions. An attack on a skill unit ends at once:
  the server attacks no skill unit.

Not checked: any of it against the real client; the parity gate.
