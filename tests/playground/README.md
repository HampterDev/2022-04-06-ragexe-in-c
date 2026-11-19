# Playground

`ragexe_playground` is the scene harness (`tests/scenes`) with its driver swapped. It runs no
script and plays no map server: it is the client's half of a run in which the real client and
the real server play against each other on one virtual clock, with nothing shown and nothing
waiting for real time. The other half, which hosts the server, makes up what the player does
and checks what would be on screen, is the runner in the server's repository
(`consumers/host/playground` of the ring-buffer worktree; its `run.mjs` says how to run one).

Windows only so far: `build.bat`, then `tests\playground\build.bat`, which links
`build\ragexe_playground.exe` from the same objects.

## What is swapped

Everything the scene harness swaps (the clock, the sockets, the threads, the settings store,
the cursor and key state, Granny's clock, the crash dialog), and in the place of
`scene_driver.c`:

| file            | what it does                                                                  |
|-----------------|-------------------------------------------------------------------------------|
| `play_driver.c` | each tick: reports the frame to the runner, waits for its answer, does it      |
| `play_link.c`   | the line to the runner: one blocking TCP connection on this machine            |

The client is silent in a run whatever its saved options say, and its window is hidden.

## A tick

The driver sends a frame and waits for a step. Because it waits, the server never runs ahead
of the client or behind it, and a run with the same seed is the same every time.

Frame, client to runner, little-endian:

```
u32 size of what follows
u8  1; u8 mode (0 login screen, 1 in a map, 255 between); u8 the player is there; u8 0
u32 tick                      the client's clock, 16 ms a frame
f32 x, y; u8 state; 3 unused  the player
u32 actors, each: u32 gid; f32 x, y; u8 state; u8 is a player; u16 job;
                  u8 the frame its action is on; u8 the frame its attack shows its hit on
                  (255: none); u8 1 when the client has its unit in view
                  (`world/view_floors.h`; an actor shown dying after its going has not);
                  1 unused
u32 packets, each: u16 size; the packet as the client sent it on its map connection
u8  the own clock is known; u8 a body is predicted; 2 unused
u32 own clock                 the server's tick the own character is drawn on
i32 x, y                      the cell the walk it is drawn on has it on at that tick
u32 lands                     the tick the step it has under way lands at; 0: it stands
u32 predicted tick            the server's tick the predicted body is of
224 bytes                     the predicted body (`game/own_body.h`); zero without one
```

Places are in cells, where the actor is drawn: a cell's centre is a whole number. An object of
the world that is no unit has the gid 0xFFFFFFFF.

The last block is the own body, for the runner to hold against the server's body by tick: what
the client draws its character from now (the cell of its walk at the own clock), and what the
predicted world has (`plans/predicted-world.md`), whose bytes are the body core's and have to
be the server's at the same tick.

Step, runner to client:

```
u32 size of what follows
u8  2; 3 unused
u32 bytes; the server's packets for the client, one after another
u32 commands, each: u8 kind; u8 0; u16 size; payload
  1 enter   char map[24]; i32 x, y, dir, job, level, job level, hp, sp, sex; char name[24]
  2 walk    i32 x, y            what a click on a cell does
  3 attack  u32 gid; i32 mode   what a click on an actor does (0 once, 1 held, 7 repeated)
  4 cancel
  5 quit    i32 exit code
```

`enter` takes the client from the login screen straight into a map, as `scene.enter_map` does:
no login and no character server. The client then asks the map server to let it in
(`CZ_ENTER2`), which the runner answers. `walk`, `attack` and `cancel` are the messages the
player's own clicks send its character, so the request, its stamp and the predicted walk are
the client's own.

## Environment

`RAGEXE_PLAY=<port>`: the runner's port on this machine. Without a runner there the client
ends with exit code 2.

`RAGEXE_PLAY_LOSS=<n>`, `RAGEXE_PLAY_REPEAT=<n>`, `RAGEXE_PLAY_SEED=<n>`: the wire of the
delivery piece's line, below. The runner sets them from its `--loss` and `--repeat` and the run's
seed.

## The delivery piece's line

What travels through the delivery piece in the real thing (`plans/delivery.md`) does here too,
inside `play_driver.c`, with a wire between the piece's two ends:

- what the client sends on the delivery channel (a move, an act, a skill, an intent, each with
  its stamp) goes through the client's end, the wire and the end the gateway holds, and the
  runner is told a request when that second end hands it on;
- what the server sends that the route table names (`delivery/delivery_route.h`: the units'
  records, the own body and hits) goes through the gateway's end, the wire and the client's end, and is given
  to the client when its end hands it on. The rest of what the server sends is given to the
  client as it comes, a packet at a time and in the order sent.

The wire (`tools/lossy_link/wire.c`) loses `RAGEXE_PLAY_LOSS` of a hundred datagrams and repeats
`RAGEXE_PLAY_REPEAT` of a hundred, each way, by the seed. It holds nothing back: how long the line
is remains the runner's `--ping` and `--jitter`, which delay a stamp and the request after it
once, as the client sends the two in one write.

On a wire that loses nothing everything arrives in the frame it was sent and in the order it
was sent, and a run is what it was without the piece. What a lost datagram carried is sent again
by the piece when the driver next ticks, 16 ms on, unless something newer replaced it.

When it ends, the client writes on its standard error how late the own body was through the
piece, which the runner reads: of each `ZC_OWN_BODY` the client's end handed on, the time from
the gateway's end being handed it to then, in all and at worst; and how many words of the own
body differed from what was predicted with nothing to explain them (`game_mode_own_body_unexplained`).
