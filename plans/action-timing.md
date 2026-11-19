# Action timing

A redesign of when an action commits and what can cancel it, across this client (`custom`) and
the ring-buffer server. The model is League of Legends': any command replaces the current
action, a swing has a real windup that a move cancels, and the server decides every timing.

Status, 2026-11-12: stage 1 is how the server and the client work, without a setting on either
side (`plans/predicted-world.md`, step 3a). A swing's begin and its cancel reach the client
as records (`ZC_TRACK_ACT`, `plans/core-loop.md`) and its hit as a notice (`ZC_NOTICE_HIT`);
the two packets this stage first had for them, 0x0F10 and 0x0F11, are not used.

## Goals

- The game never drops or silently delays an input.
- Standing still and attacking gives full damage. Moving between swings is optional gain.
- Cancelling a swing too early loses that swing. That risk is the skill.
- One rule, decided on the server. The client sends commands at once and draws what it is told.

## Words

- **Swing**: one basic attack of a player.
- **Windup**: from the swing's start to its impact. The hit is not decided yet.
- **Impact**: the moment the server rolls and applies the hit.
- **Recovery**: from impact to the end of the animation. Nothing is locked.
- **Rate clock**: the earliest time of the next swing (`attackableTime`, start + attack delay).
  Cancelling never moves it earlier than a fresh swing would allow.

## How it works today

- Server: a swing's damage is calculated at its start and sent at once in `ZC_NOTIFY_ACT2`
  (0x08C8). HP changes one attack motion later. A move request is never refused, and a swing in
  flight still lands, so there is no windup to lose.
- Client: a move click during the swing animation is parked (`owed_walk`) until the animation
  ends. Attack and skill intents wait for the animation too. A walk request within 350 ms of the
  last one is dropped. An attack request within 1200 ms of the last one is not sent.
- The lock a player feels is the client's animation, which the server does not know about.

## Stage 1: swings and movement

Stage 1 covers players' basic attacks and movement. Monsters, companions and skills keep the
classic behaviour.

### Rules

1. A swing starts when the rate clock allows. The server announces the start without damage.
2. Impact is `windup_ms` after the start. The server then checks the target (alive, same map,
   in reach), calculates the hit, applies HP and sends the damage.
3. During windup, any of these cancels the swing with no damage: a move request, an attack
   request for another target, a skill, a warp, death, logout. An attack request for the same
   target leaves the swing alone; only once or repeating follows it. A stop
   (`CZ_CANCEL_LOCKON`) ends the repeat and lets the swing land: the client sends it whenever
   the mouse button of a click comes up, so a stop that cancelled would make every quick click
   miss.
4. A cancelled swing does not use up the rate clock: the next swing may start at once, plus
   `action_cancel_penalty_ms`.
5. A swing whose target is gone at impact is cancelled the same way (reason 1).
6. After impact everything is free. A move starts at once. The next swing waits for the rate
   clock, and an attack request sent early is held by the server until then.

### Packets

As first built, and no longer sent: the swing record of `ZC_TRACK_ACT` says what
`ZC_ACTION_BEGIN` said, and its idle record what `ZC_ACTION_CANCEL` said. Both were server to
client, in the project's own id range, little-endian, packed.

`ZC_ACTION_BEGIN` 0x0F10, 21 bytes

| off | size | field | note |
|---|---|---|---|
| 0 | 2 | id | 0x0F10 |
| 2 | 4 | source | GID of the attacker |
| 6 | 4 | target | GID of the target |
| 10 | 4 | server_tick | start of the swing |
| 14 | 2 | attack_motion_ms | sets animation speed, as `attackMT` in 0x08C8 |
| 16 | 2 | windup_ms | impact is this long after `server_tick` |
| 18 | 1 | kind | 0 = basic attack; other values reserved |
| 19 | 2 | skill_id | 0 for a basic attack |

`ZC_ACTION_CANCEL` 0x0F11, 7 bytes

| off | size | field | note |
|---|---|---|---|
| 0 | 2 | id | 0x0F11 |
| 2 | 4 | source | GID of the attacker |
| 6 | 1 | reason | 0 = the unit's own command, 1 = target lost |

`ZC_NOTIFY_ACT2` 0x08C8 keeps its layout. A hit, whether it comes as that packet or as
`ZC_NOTICE_HIT`, for an attacker with an open swing on the same target is that swing's
impact: the damage shows now and the animation is not restarted. Without an open swing it
means what it always did.

No client to server packet changes. The client sends the existing move, act and stop packets.

### Server (ring-buffer)

- Swing start: publish `ACTION_BEGIN` on the combat channel, remember the open swing on the
  attacker's unit data, schedule the impact as a sim timer `windup_ms` later. The rate clock is
  set as today.
- Impact: re-check the target, run the hit calculation, publish `ACTION`, apply HP at once.
- Cancel: drop the impact timer, set the rate clock to now + penalty, publish `ACTION_CANCEL`.
- `eos-world` renders the two events to everyone who sees the attacker, as records.
- Settings, numbers only: `action_windup_rate` (percent of attack motion, default 100),
  `action_cancel_penalty_ms` (default 0).

Built in the worktree `ring-buffer\local\worktrees\action-timing` (branch `action-timing`, not
committed). The numbers are read from the server folder's `config/server.conf`. The server's
own description is `docs/action-timing.md` in that worktree.

What the server does beyond the rules above:

- Triple Attack and Sacrifice replace a swing's impact with their skill damage packet, so that
  swing gets neither a hit nor a cancel. The client forgets it when the attacker's next swing
  begins.
- A swing does not start without ammunition, and one whose ammunition is gone at impact is
  cancelled with reason 1.
- A request held back by a skill's after-cast delay is still refused, as before. Only the rate
  clock holds an attack request.

### Client (custom)

- A swing's begin and its cancel as played records (`game/packets_act.c`, `swing_begin` and
  `swing_cancel`); the impact branch in the hit's path.
- For the own player:
  - a move click during a swing is sent at once instead of parked;
  - an attack click during a swing is sent at once, and the 1200 ms resend guard is gone;
  - a walk request leaves with the frame of its click, the newest click of the frame: no
    spacing is kept between requests (`plans/predicted-world.md`, step 4).

## Stage 2: skills (not built)

- A skill or attack request that arrives while the unit cannot act is held (one slot, newest
  wins) and run on the first legal tick, if that tick is within a buffer window.
- A move request during a cast is held until the cast ends.
- A skill during a swing's windup cancels the swing.
- The skill rate needs a decision first. Today only the client's animation keeps Bash at one
  per swing; the server alone would allow two. Once the client stops gating, the server must
  hold that clock, which needs each skill's pose kind and length on the server.

## Open measurements

- Where the animation's hit frame falls against `windup_ms` for each job and weapon.
