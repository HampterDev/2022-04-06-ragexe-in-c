# Delivery

How records, notices and requests travel between this client (`custom`) and the ring-buffer
server: each one is sent again until it has arrived or until a newer one has made it pointless,
and none waits behind another. What a record says does not change, and no classic packet
changes. Status, 2026-11-12: steps 1 to 4 are built on both sides and pass their tests, and the
client's sockets were run against the gateway program on one machine. The player's requests,
the units' records and the hit notices travel this way; the own body does not yet. Step 6 is
built: the lossy link, the measurement over it, and the playground's line through the piece both
ways. Of step 5 what is left is the own body and a record that crosses its unit's going. Nothing
of it was played. See "What is built".

`plans/core-loop.md` says what a record and a notice are, `plans/transport.md` what carries
packets, `plans/predicted-world.md` what the owner's own body is.

## What one stream in order costs

A record is complete in itself and the newest of its track is the one that counts
(`plans/core-loop.md`, rule 3). The transport does not know that. Records travel on ENet's world
channel, which is one stream in order, together with everything else.

- **A lost datagram holds back everything behind it.** ENet sends a reliable packet again after
  the round trip plus four times its variance (`vendor/enet-1.3.18/protocol.c`), and until it
  has arrived nothing later on the channel is handed on: no walk, no hit, no own body.
- **What is sent again may be stale.** The swing that was lost is sent again although its
  cancel is already queued behind it.
- **A slow link gets everything late.** What a client has not acknowledged waits in a queue of
  up to a megabyte (`ENET_CLIENT_QUEUE_LIMIT` in the gateway's `src/enet_listener.c`), oldest
  first, records that were replaced long ago included.
- **A lost request waited as long.** The server holds a request back for 60 to 100 ms and takes
  press ticks up to 250 ms old (`plans/netcode.md`). One resend at 100 ms of round trip is
  already outside the hold-back, so the request is applied late and out of its press order.
  Step 3 took the requests off the stream.
- **The record's rule needed the stream.** `move_track_file` removed every held record with the
  same or a later tick "as the server made this one knowing them". That is only true when
  records arrive in the order they were made; part 1 below is the rule that holds in any order.

## What kind of problem this is

Reliable delivery of small messages over datagrams, where most messages are state that a newer
message replaces. The fields that have worked on it, and what is taken from each:

- **QUIC's loss recovery (RFC 9002).** Every datagram has a number of its own and is never sent
  again. The other end acknowledges datagram numbers. What a lost datagram carried and is still
  needed goes into a new datagram. Taken: all of it, with its two tests for a loss (three later
  datagrams acknowledged, or nine eighths of the round trip) and its timeout when nothing comes
  back.
- **The Tribes networking model and Halo: Reach.** Data is sent by what it needs: the newest
  state must arrive and older state need not; an event must arrive once and in no order. The
  sender notes what each datagram carried, so a loss names what to send again. Taken: the
  kinds below.
- **Unreal's actor channels.** Order holds within an actor and never across actors. Taken: no
  message waits for a message of another key.
- **Quake 3 and Overwatch.** The commands the server has not acknowledged are repeated to it
  every few milliseconds, so one loss costs a frame and not a resend timer. Taken: for
  requests.
- **Replicated data types (state-based CRDTs).** A merge that gives the same result in any
  order and with any repeats lets the transport reorder, repeat and drop. Taken: the revision,
  which gives a track's records such a merge.

## The design

Three parts.

### 1. A track's records merge in any order

Every track of a unit has a **revision**: a 16-bit number the server's renderer counts up each
time it makes a record of that track. The record carries it. A viewer who comes later gets the
record with the revision it was made with. Revisions are compared by their signed difference,
like ticks. The count belongs to the unit's id and outlives the unit: one that vanishes and comes
back under the same id (a monster's respawn, a player's next login) counts on, so that a record
made before it went never passes for a newer one. A gone unit's count is kept for a minute,
longer than a link may be silent before ENet closes it, and it is part of the server's snapshot
and goes with a unit to another map (the server's `docs/tracks.md`, "Revisions").

Rule 3 of `plans/core-loop.md` becomes:

- Of two records of a track, the one with the higher revision is the newer.
- A record that arrives removes every older record of its track with the same or a later
  `at_tick`.
- A record that arrives when a newer one with the same or an earlier `at_tick` is held is
  dropped. So is one whose revision is already held.

Filed in any order and any number of times, the same records leave the same track. In the order
they were made it is the rule as it is today.

The owner's own body (`ZC_OWN_BODY`) needs no revision: it carries its tick whole, and a state
with an older tick than the one the client holds is refused (`world/predicted.h`).

A notice has no merge. It is shown once, so it must be handed over once (part 2).

A record may now arrive before the packet that makes its actor known, because that packet stays
on the world channel. A record that finds no actor waits by GID (`world/record_wait.c`), for two
seconds at most, and is filed or played when the actor appears (`game_mode_play_tracks`).
Before, a move record without its actor was dropped, and an act or HP record was played to
nobody.

Not solved: **a record that crosses its unit's going.** A unit's going and coming are classic
packets on the world channel, its records go through the piece, and on a link that loses
packets the two can pass each other. A record made after a unit came back under the same GID
can reach the client before the packet that says the unit it had went, be filed on that actor
and go with it; one made before the unit went can reach the client after the unit came back
and show where the unit was. Either holds until the unit's next record of that track. The
revisions keep such a record from passing for a newer one, not from crossing the going. A
cure puts the going and the coming in the order the revisions give, for example as records of
the move track, and is to be designed with step 5.

### 2. One piece that delivers messages

`delivery`, freestanding C11 with its own tests, the same source in the client
(`src/delivery`) and in the gateway (the server's `net/delivery`). It knows nothing of ENet,
of sockets or of packets' contents. State and input in, state and output out: a block of memory
the caller hands in, the time as an argument, no allocation, no clock, no library.

```c
delivery_send(d, kind, key, from, priority, bytes, size);  /* a message to deliver; 0 when refused */
delivery_make(d, now_ms, out, room);       /* the next datagram to put on the wire, or 0 */
delivery_arrived(d, now_ms, bytes, size);  /* a datagram came off the wire */
delivery_take(d, &size);                   /* the next message that arrived, or NULL */
```

`delivery.h` has the datagram's layout. A message is whole packets, as an ENet packet is. It
has one of three kinds:

| Kind | Arrives | Order | Sent again | For |
|---|---|---|---|---|
| Replaceable | unless replaced, perhaps more than once | none | until acknowledged or replaced | records, the own body |
| Single | once | none | until acknowledged | notices |
| Ordered | once | as sent, among the ordered ones | every 15 ms until acknowledged | requests |

- **Replaced.** A replaceable message has a key (the unit and the track) and a position `from`
  (the record's `at_tick`). Sending one ends the sending of every earlier message of its key
  with the same or a later `from`. The own body always says 0, so the newest replaces all. What
  waits for a connection is therefore never more than the records that still count.
- **Acknowledged.** Each datagram starts with its number, the highest number received and a
  bit for each of the 32 before that. A message is done when a datagram that carried it is
  acknowledged.
- **Lost.** A datagram is lost when three later ones are acknowledged, or when a later one is
  and nine eighths of the round trip have passed since it left. Its messages that are neither
  done nor replaced go into the next datagram. When nothing is acknowledged for the round trip
  plus four times its variance and the 25 ms an answer may be kept, the datagram counts as
  lost too, and that wait doubles, three times at most, until an answer comes. The round trip
  is measured from the acknowledgements.
- **Once.** Single and ordered messages are numbered, and the receiver hands each number over
  once. A replaceable message that arrives twice is merged twice, which changes nothing
  (part 1).
- **Requests.** The ordered messages not yet acknowledged leave together, the oldest first: at
  once when one is new, and again every 15 ms. The receiver hands them over in order. A lost
  datagram costs a request those 15 ms and no timeout. A client that looks at its socket 60
  times a second sends them again each time it looks, so there a loss costs one frame.
- **Silence.** An end remembers its last 128 datagrams with messages. While the one whose
  place a further datagram needs is still on its way, that further one goes only every 15 ms,
  and the old one counts as lost. So an end that hears nothing goes on asking, a datagram at a
  time, and is back as soon as one datagram is answered: everything older is then lost by the
  first rule and sent again at once.
- **How much.** A datagram is at most 1,200 bytes and a message at most 1,024. A connection
  can have a budget of bytes a second. Within it, ordered messages go first, then by priority
  (the own body, notices, records), then the longest waiting. A message the piece refuses, for
  its size or because the piece is full, travels on the world channel.
- **Outside data.** A datagram is data from outside: one that is not shaped right changes
  nothing and the piece carries on.

### 3. ENet carries it

ENet stays for what it does well: the connect, the timeout, the two channels in order, the
newest channel. The piece's datagrams travel on a fourth channel as unsequenced packets, which
ENet hands over in any order and never sends again. Every ENet link of the two programs has
the channel and a piece at each end; there is no setting.

- ENet drops packets that are not reliable at the sender when its throttle is below full
  (`protocol.c`, `packetThrottleCounter`). The link is configured so the throttle never falls
  (`enet_peer_throttle_configure` with a deceleration of 0).
- An unsequenced packet above ENet's fragment size would be sent as reliable fragments, so the
  1,200 bytes stay below it.
- **What the client sends is one stream.** The requests go through the piece and everything
  else on the world channel, and the server's parser reads both into one stream in the order
  they arrive. So that neither overtakes what was sent before it on the other, a send goes where
  the other has nothing the server has not acknowledged (`platform/net_enet.c`): a request goes
  on the world channel while world packets sent before it are on their way, and a world packet
  through the piece while requests sent before it are in it. At any moment at most one of the
  two carries something, and each is in order in itself. A world send larger than a message
  waits while the piece carries something. Since the client sends few world packets, requests
  almost always take the piece.
- **A closing socket waits for the piece** first, then for ENet's acknowledgement of the close,
  100 ms together at most.
- A link with fewer channels, to a program built before the piece, has none, and what would go
  through it goes on the world channel. A TCP client of the server (the parity gate's) has no
  channels at all and is served as before.

### What travels how

A packet may leave the world channel for the delivery piece when nothing on the world channel
has to be read before or after it, which is the rule of `plans/transport.md`, and when its
merge does not depend on order.

| What | Kind | Key | |
|---|---|---|---|
| Client: a move, an act (attack, sit, stand), a stop of the attack, a skill, an intent, each with its stamp as one message, and a world packet sent while requests are in the piece | ordered | | built |
| `ZC_TRACK_MOVE`, `ZC_TRACK_ACT`, `ZC_TRACK_VITALS` | replaceable | unit and track | built |
| `ZC_OWN_BODY` | replaceable | one for the connection | not in the table yet: in order with the world |
| `ZC_NOTICE_HIT` | single | | built |
| `ZC_OWN_INTENT`, everything classic, the lists, the timed exchange | as before | | |

`ZC_OWN_INTENT` says each change of a request once and has no merge, so it stays in order
until the own body has taken its job (`plans/predicted-world.md`). The requests are the group
the server already orders among themselves and lets other packets pass
(`plans/core-loop.md`, "Requests").

Whoever sends picks the kind from one table both ends read, `delivery/delivery_route.c` (the
server's `net/delivery`, copied to this client like the piece): by the packet's id, its kind,
its priority, and where a record's unit and tick are, from which its key and `from` are made. A
packet of the server's goes through the piece only when it is exactly one packet as long as its
layout says, so a record in the layout before step 4 goes on the world channel. Whoever
receives takes any packet from anywhere.

In the gateway nothing above the transport changes. A request reaches the parser once and in
order, whatever was repeated on the wire, so the journal has it once. A consumer's send
command is written to the journal once and then handed on; what the piece sends again will not
be written again. The server's game code keeps no memory of what a viewer has, as rule 7 says:
the gateway keeps what a connection has not acknowledged, as every transport that resends
does.

## Constraints

- **The server decides, and a client sees part of the world.** So state comes down and requests
  go up; this plan changes only how they travel.
- **The parity gate and the original client speak TCP,** one stream. Delivery may take order
  away and never adds meaning.
- **The journal replays bit-equal.** What goes into the simulation is journaled as before.
  Acknowledgements and resends are the transport's and are in no journal.
- **The original's packet layouts stay.** The revision goes into the project's own records.
- **Windows x86 and x64, Linux and macOS** (`docs/portability.md`). The piece is freestanding
  and ENet is the carrier on all of them.

## What can be shown to hold

Each a test in C of the piece alone, or of the track's merge alone:

1. **Any order.** A track's records, filed in every order and with repeats, leave the same
   track as filed once in the order they were made.
2. **It arrives.** Two ends of the piece over a wire that drops, repeats, reorders and delays
   datagrams by a seed: once the wire is quiet, every single and ordered message was handed
   over exactly once, the ordered ones in order, and of every key the records that count.
3. **Nothing stale.** After a replaceable message is sent, no datagram made later carries a
   message it replaced.
4. **Bounded.** What waits for a connection is never more than the records that count, the
   notices and requests not yet acknowledged, and the budget is never passed.
5. **A loss costs one sending.** A request whose datagram is dropped is handed over from the
   first datagram that arrives after it, and those leave at most 15 ms apart.
6. **Any bytes.** Random bytes as a datagram never leave the piece invalid.
7. **Deliberate faults.** Each rule above broken on purpose is caught by a test.
8. **On screen.** The playground with loss and reordering shows no finding it does not show
   without them at the same ping.

## Steps

Each step ends with its check passing and leaves the game playable.

| Step | What | Check |
|---|---|---|
| 1 The piece (built) | `delivery` with its tests, native and built for the gateway. The same bytes in both repositories, held by a check as the body core's copy is | 2 to 7 |
| 2 The carrier (built) | The fourth channel on both ends, the throttle, the piece under `platform/net.h` and in `src/enet_listener.c` | The transport tests of both sides; a client and a gateway of which one lacks the channel still talk |
| 3 Requests (built) | The client sends its requests as ordered messages. The gateway hands them on once and in order | 5; the gateway's record of a run has the requests once and in order |
| 4 Order-free records (built) | The revision in the three records and in the renderer; the merge of part 1 in `world/move_track.c` and the act and HP queue; a record without its actor waits by GID | 1; the server's session tests read the revision back; the playground shows nothing new |
| 5 Records on the piece (records and hits built) | The route table both ends read sends records, the own body and hit notices through the piece. The gateway's end has no budget: what a link carries is not known, and what can wait is bounded by the units in view. The playground sends the server's packets the table names through the piece and its wire too. Left: the own body, which the predicted world needs in order after its actor's going, and a record that crosses its unit's going (below) | 3, 4, 8 |
| 6 Loss by seed (built for requests) | A lossy link to play and test over (`tools/lossy_link`): a relay on this machine between the client and the server that loses, repeats and delays every datagram by a seed, ENet's own included. `tests/measure_requests.c` sends the same requests through the piece and on the world channel over it. The playground's client half gets both ends of the piece and the same wire between what the client sends and what the runner hears | 8, and the numbers this plan claims: how long a record and a request are late per loss, with the piece and without |
| 7 The docs (built) | The line "no unreliable transport" of `plans/core-loop.md` and its rule 3 are brought up to date; the server's `docs/tracks.md` gets the revision | Every claim read against the code |

Step 6 can be built before step 5, and should be: it is what shows whether step 5 gained
anything.

## Beside the predicted world

`plans/predicted-world.md` is being built in the same checkouts, and nothing of either plan is
committed. The two meet in few places:

| Files | Predicted world | Delivery |
|---|---|---|
| `src/body`, `src/world/predicted.*`, the own-walk, own-attack and own-swing files, `src/actor`, the packet handlers under `src/game`; the server's `sim/body`, `consumers/` | all steps | never in steps 1 to 3 |
| `platform/net*`, `contents/rag_connection.c`, `src/delivery`; the gateway's `src/enet_listener.*`, `src/packet_channels.*`, `net/delivery` | never | steps 1 to 3, 5 |
| `world/move_track.*`, `world/act_track.*`, `game/packets_act.c`, `game/packets_actor.c`, `core/packet_table.c`; the server's `eos-world/aoi-render.js`, `shared/src/eos-protocol.js`, `scripts/project-packets.js`, `docs/tracks.md` | its steps that change records or their handlers | step 4 |
| `tests/playground`, the server's `consumers/host/playground` | step 0 and every check | step 6 |
| The lists every new piece is entered in: `tests/build.bat`, `tools/libraries.txt`, `tools/check_libraries.bat`, `CMakeLists.txt`, `README.md`, `docs/custom-review.md`, the server's `CMakeLists.txt` | each new piece | each new piece |

So:

- **A step that changes the records' layout or how they are filed waits for a pause between two
  of the other plan's steps.** Whoever builds it says so before the first edit and when the
  checks pass again. Step 4 was built that way.
- **An entry in a shared list is one line, added when its piece builds and passes,** to the
  file as it is at that moment and never by writing the file anew.
- **A check that fails in a file of the other plan is reported to whoever builds that plan,**
  not repaired.
- **`ZC_OWN_BODY` stays on the world channel until both plans agree on its rule.** Its tick
  orders it among its own kind, but not against the world channel: the predicted world lets a
  body that went leave at the world's clock and takes every word that arrives after the going
  as a world without that body (`plans/predicted-world.md`), which needs the own body to arrive
  after the packet that says the body went. Whoever puts it on the piece tells the other plan
  first.
- **The whole suites run once both are at rest,** before a commit of either.

## What is built

Client (`custom`) and server (ring-buffer worktree `local\worktrees\action-timing`), neither
committed.

| Step | Built | Not built or not covered |
|---|---|---|
| 1 The piece | `delivery.c` and `delivery.h`: the three kinds, replacement, acknowledgements, the three ways a datagram counts as lost, the budget, the datagram every 15 ms of an end that hears nothing, and `delivery_check` for tests. Home is the server's `net/delivery`; `src/delivery` and `tests/test_delivery.c` here are the same bytes, held by `tools/check_delivery_copy.py` in `build.bat check`. The tests: 207,282 checks, the same count with 32-bit MSVC at warning level 4, with clang in the server's build and under clang's address and undefined-behaviour sanitizers. Among them six worlds of two ends over a seeded wire (up to 80 of 100 datagrams lost, repeats, delays to 300 ms) that hold check 3 on every datagram made and check 2 at the end, every number and the clock wrapping, the loss rules by their exact millisecond, ten seconds of silence after which 400 messages are there within 60 ms of the first datagram that arrives, and 60,000 datagrams of noise and of changed bits. 56 deliberate faults were each caught | The deliberate faults are a script that was run and is in neither repository. Check 4's bound is shown for a state of which only the newest counts, with as many places as keys, not for records with ticks. A forged acknowledgement ends the sending of a message that never arrived: the piece trusts its wire, as ENet does. The test alone was not built for x64 |
| 2 The carrier | The channel in `platform/net.h` (`PLATFORM_CHANNEL_DELIVERY`) and the gateway's `src/packet_channels.h`; an end of the piece per socket in `platform/net_enet.c` and per client in `src/enet_listener.c`, made when the link has the channel; ENet's throttle turned off on such a link; the piece's datagrams made when the socket is serviced and when the gateway flushes. `tests/test_platform_net.c`: two requests once and in order, three datagrams lost in a row and the request there after the fourth and within 200 ms, nothing arriving twice, a message from the server's end read off the channel, a send too large for the piece and a server without the channel both on the world channel. The gateway's `enet_listener_delivers_requests`: 120 requests reach `onData` once, in order and one message at a time while the test's client drops, repeats and turns around its datagrams and drops answers; a client with one channel is served next to it | The gateway's ends have no budget. A socket that closes waits for the piece and then for ENet, 100 ms together at most: a request still unacknowledged after that is lost with the link. The CMake build of `docs/portability.md` was built for x64 with Visual Studio for the piece's, the route table's, the socket's and the connection's tests only, which pass; Linux and macOS were not built |
| 3 Requests | `rag_connection.c` puts the input stamp and the seven requests it goes before (`CZ_REQUEST_MOVE2`, `CZ_REQUEST_ACT2`, `CZ_CANCEL_LOCKON`, `CZ_USE_SKILL2`, the two ground skills, `CZ_INTENT`) on the delivery channel; a stamp and its request leave in one write and so as one message. `tests/test_packets.c` holds the table over all 65,536 ids. The socket keeps requests and world packets as one stream (`tests/test_platform_net.c`: a request right behind an unacknowledged world packet goes behind it on the world channel; a world packet behind a request the wire lost twice goes through the piece behind it; a world send too large for the piece waits; a request lost twice just before a close is there before the close); six rules of it broken on purpose were each caught. Run as two programs on one machine: the client's socket files against the gateway's `server.exe`, forty stamped moves and ten attacks, and the gateway's record had the 90 packets as sent and in order | No journal of a session with loss was replayed; the gateway's test stands in, since what reaches `onData` is what is journaled. Not played, and not run with the game's own client. Requests held back while a map loads leave with the other held packets on the world channel, in order and without the piece's quick resend |
| 4 Order-free records | The server: the revision in the three records (`ZC_TRACK_MOVE` 25 bytes and the steps, `ZC_TRACK_ACT` 37, `ZC_TRACK_VITALS` 20; `eos-protocol.js`, `scripts/project-packets.js`) and its count in `aoi-render.js`, by unit id, kept a minute after the unit goes, in the snapshot and in a map change. Its tests: the protocol's, the renderer's (counting on after a return, a snapshot, a map change, dropped after a minute; five of those rules broken on purpose were each caught), the tracks session, and the eleven recorded sessions written anew, of which a script checked that each of the 123 records is the one before with its revision added, that a unit's revisions start at 1 and never go back, and that nothing else changed. The client: `world/move_track.c` reads the revision and merges in any order, with what was played as a floor; `world/act_track.c` the same for the act and HP queue, with the floors on the actor (`RecordFloors`); `world/record_wait.c` keeps a record whose actor is not there, 64 at most and 2 s each, and files it when the actor appears; `game/packets_actor.c` and `game/packets_act.c` file through them; the own character's move record gets a revision of its own (`game/own_body.c`). `tests/test_move_track.c`, `test_act_track.c` (any order leaves the same track or queue; a record twice or overtaken is dropped; what was played is a floor; revisions wrap), `test_record_wait.c`, `test_attack_records.c`; `build.bat check` | A record that crosses its unit's going (part 1) |
| 5 Records on the piece, in part | The route table, `delivery_route.c` and its tests (`delivery_route_tests.c`, `tests/test_delivery_route.c` here, held equal by `tools/check_delivery_copy.py`): the client reads it for its requests in place of a table of its own, the gateway's `enet_listener_send` for what it sends, and sends what it names through a client's piece. The gateway's end of the piece has room for 2,048 messages and 128 KB; it has no budget. The gateway's `enet_listener_delivers_records`: of two records of one unit and track sent in one tick only the newer travels; a record of another unit or track and a hit travel too; a record in the old layout goes on the world channel; the first datagram is lost and what it carried comes again. With step 4 the records the server makes match the table, so the units' records and the hits travel through the piece. The playground's driver sends what the table names through its own two ends of the piece and the wire, as the gateway does | The own body is not in the table (see "Beside the predicted world"). Not played, and the gateway was not run with the game's own client |
| 6 Loss by seed | `tools/lossy_link`: `wire.c`, the wire by its seed, freestanding; `relay.c`, two of ENet's UDP sockets around it; `main.c`, the program (`tools\lossy_link\build.bat` makes `build\lossy_link.exe`; `lossy_link <listen port> <server port> loss=N repeat=N delay=MS jitter=MS seed=N`, and the client is pointed at the listen port). `tests/test_lossy_link.c` in the suite: the wire's order, limits, delay, jitter and shares, the same wire from the same seed, and the relay between two sockets. `tests/measure_requests.c` with its `.bat`, outside the suite because it takes real time. The playground: in `tests/playground/play_driver.c` what the client sends on the delivery channel (`rag_connection_packet_channel` says which) goes through both ends of the piece and a wire by the run's seed before the runner is told it; the runner's `--loss` and `--repeat` set the wire | A relay serves one client at a time. Nobody has played over it. The playground's wire holds nothing back, since how long the line is stays the runner's to say, and it carries only what the piece carries: what the server sends is not lost there. The numbers of both are below |

### What the lossy link measured

`tests\measure_requests.bat seed=N requests=300`, on 2026-11-12: two of the client's sockets
send the same 300 requests at the same moments, one every 48 ms, one socket through the piece
and one on the world channel, each over a lossy link of its own that loses 10 of 100 datagrams
each way and holds each 30 ms. The client looks at its sockets every 16 ms. A request is late
when it takes more than 38 ms, and late by what it took beyond the link's 30 ms.

| Seed | Piece: late, by on average, at most | World channel: late, by on average, at most |
|---|---|---|
| 1 | 35 requests, 20 ms, 50 ms | 83 requests, 106 ms, 386 ms |
| 2 | 36 requests, 20 ms, 51 ms | 100 requests, 137 ms, 722 ms |
| 5 | 46 requests, 18 ms, 36 ms | 85 requests, 139 ms, 739 ms |
| 3 | 46 requests, 20 ms, 81 ms | all 300, 8,168 ms, 15,345 ms |

- **Through the piece a loss costs a frame.** The late requests are the ones whose first
  datagram was lost, and they are late by the 16 ms until the client looks at its socket
  again, or by two or three of them when the next was lost too.
- **On the world channel a loss costs ENet's resend timer, and what follows waits behind it.**
  More requests are late than were lost.
- **Seed 3 is a connect whose handshake lost datagrams.** The client holds its link for up
  while the server's end is not, and ENet drops what arrives for such a link. On the world
  channel the first requests then go out again on timers that double from half a second, the
  round trip ENet reckons with grows with them, and the stream stayed 8 to 15 s behind for
  the whole run. The piece on a link with the same seed was not caught by it in this run; in
  runs where it was, its requests waited the 1.1 s the handshake took and no longer.
- **What the measurement found in the piece.** As first built an end stopped sending when 128
  of its datagrams were unanswered and waited for a timeout that had doubled five times, so
  after such a handshake its requests arrived up to 4.6 s late. That is the "Silence" rule of
  part 2 now, with a test of its own, and the resend interval went from 20 ms to 15 ms: at
  16 ms a frame, 20 ms was every second frame.
- The numbers are of real time on a machine that ran other builds. The two ways run side by
  side in one loop, so that held both up alike; a single run is still not exact.

### What the playground showed

Four seeds of 40 s with six monsters, the runs of a row made one after the other on one tree
(2026-11-12):

| Line | Wire | Findings on screen | Of the own character | Own body differs: as drawn, as predicted |
|---|---|---|---|---|
| no lag | loses nothing | 3 | 0 | 1, 10 |
| no lag | 10 of 100 lost | the same 3 | 0 | 1, 10 |
| no lag | 30 lost, 10 repeated | the same 3 | 0 | 1, 11 |
| ping 80, jitter 20 | loses nothing | 4 | 0 | 3, 245 |
| ping 80, jitter 20 | 10 of 100 lost | the same 4 | 0 | 3, 247 |
| ping 80, jitter 20 | 30 lost, 10 repeated | 3 of the 4 | 0 | 2, 232 |

- **Check 8 holds for requests.** Every finding is a monster's, and loss adds none: a request
  whose datagram was lost reaches the server a frame later, which the client's prediction
  takes as it takes jitter.
- **The line through the piece changes nothing when the wire loses nothing.** The old driver
  and the new one, linked from the same objects, gave the same runs packet for packet at no
  lag, at ping 80 with jitter 20 and at ping 150 with jitter 40.

Not covered of step 6 by these runs: no record's lateness was measured, since no record
travelled through the piece then. The lossy link was not played over.

### With records through the piece

Since steps 4 and 5 the units' records and the hits travel through the piece in the playground
too, and its wire loses and repeats what the server sends as it does what the client sends.
Eight seeds of 40 s, the runs made one after the other on one tree (2026-11-12). Six Porings
die before they swing, so those rows carry walks, the player's own hits and the monsters' HP;
six Zombies chase and hit the player, so those carry swings at the player and their hits as
well:

| Monsters, what the player does | Line | Wire | Findings on screen | Own body differs: as drawn, as predicted |
|---|---|---|---|---|
| Porings, fights | no lag | loses nothing | 0 | 2, 60 |
| Porings, fights | ping 80, jitter 20 | loses nothing | 0 | 8, 69 |
| Porings, fights | ping 150, jitter 40 | loses nothing | 0 | 11, 68 |
| Porings, every mood | ping 80, jitter 20 | loses nothing | 0 | 3, 28 |
| Porings, walks | no lag | loses nothing | 0 | 0, 0 |
| Zombies, every mood | no lag | loses nothing | 1 | 27, 113 |
| Zombies, every mood | no lag | 10 of 100 lost | the same 1 | 29, 113 |
| Zombies, every mood | ping 80, jitter 20 | loses nothing | 1 | 33, 118 |
| Zombies, every mood | ping 80, jitter 20 | 10 of 100 lost | the same 1 | 33, 122 |
| Zombies, every mood | ping 80, jitter 20 | 30 lost, 10 repeated | 1, another | 43, 198 |
| Zombies, every mood | ping 150, jitter 40 | loses nothing | 1 | 47, 126 |

- **Check 8 holds for records as far as these runs go.** Every finding is one actor a third
  of a cell or more off its cell while it stands; loss adds none.
- **A lost record shows later, not wrong.** The attack of a swing at the player showed 39 ms
  after its record was sent on a wire that loses nothing, 55 ms with 10 of 100 lost, and at
  ping 80 with 30 lost and 10 repeated 99 ms, at most 624 (`aimed.js` of the runner).
- **The runner draws one delay for a stamp and the request after it**, which leave the client
  in one write and are one message of the piece. The rows above are from before that; with it
  the three Zombie rows that lose nothing had 1, 1 and no finding.

Not covered: the same sets on the tree before step 4, sixteen seeds, and the lossy link, which
was not played over.

## What this does not do

- It does not encrypt, and does not prove a client's address beyond ENet's connect. A public
  server still needs both (`plans/transport.md`).
- It does not adapt to congestion beyond the budget. ENet's channels in order keep their own
  window.
- It does not send by distance. The gateway does not know where anyone stands; the priority is
  by kind of packet.
- It does not make presence, statuses, the inventory or skill hits records. Until they are,
  they stay in order on the world channel.
- It does not change a classic packet, the lists channel or the timed exchange.
