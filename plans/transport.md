# Transport

How packets travel between this client (`custom`) and the ring-buffer server. The packets
themselves are not changed by anything here. Status, 2026-11-12: the client speaks ENet only,
on three channels in order and a fourth for the delivery piece, and logs in over one
connection; the gateway speaks ENet and TCP, and the two were run against each other. What is
left of stages 3 to 5 is listed in them.

## What TCP costs this game, and what it does not

Ragnarok's protocol is a stream of events: "this actor walks from A to B from tick T", "this
hit lands at tick T for N". An event cannot be dropped, and most events depend on earlier ones
(an actor exists before it is hit). So the protocol needs what TCP gives, delivery in order, and
the headline features of UDP game transports, dropping what is stale and unordered delivery,
have little to work on.

What TCP does cost:

- **Small sends waiting for acknowledgements** (Nagle's algorithm with delayed acknowledgement).
  Fixed in stage 1 with one socket option.
- **A lost packet stalls everything behind it** until TCP resends it, and for a thin stream like
  a game's that is its timer, 200 to 300 ms and doubling. The netcode's hold-back is 60 to
  100 ms and the server accepts press ticks 250 ms old, so one loss is felt. Stage 2 resends
  after about a round trip. Stage 3 stops a loss in one kind of traffic from stalling another.

Not a cost at this game's rates: congestion control, bandwidth, the size of headers.

## Stage 1: TCP as it should be (built, on the server)

- `TCP_NODELAY` on every socket the gateway accepts. The client had it too while it spoke TCP.
- An input stamp leaves in one write with the request it stamps (`rag_connection_queue_packet`),
  which on ENet is one packet.

## Stage 2: ENet (built)

ENet 1.3.18 (`vendor/enet-1.3.18`, as published) on both sides, one reliable channel carrying
the byte stream TCP carries. Above the socket nothing changes: the client's packet reader, the
gateway's parser, the records, the consumers.

- **Client: ENet only.** The socket interface (`platform/net.h`) is implemented once, by
  `platform/net_enet.c`, for every platform. The TCP sockets are gone, and with them the
  one-second fallback this stage first had and its `RAGEXE_TRANSPORT` setting. A socket is not
  writable until the server has answered the connect; one whose server has not answered after
  five seconds, or went silent later, reads as closed. The client therefore connects only to a
  server that speaks ENet, and no longer to a plain Ragnarok server.
- **Server: ENet and TCP.** The gateway takes ENet clients on its port number's UDP port
  (`src/enet_listener.c`) unless the folder's `config/server.conf` says `enet = off`, next to
  its TCP listener. TCP stays because the parity gate drives the Hercules fork and ring-buffer
  with one scripted TCP client, and the original client speaks nothing else. An ENet
  client takes its connection id from the counter the TCP clients use, and its bytes reach the
  same parser. A consumer's disconnect lets the last packet out first, as on TCP. The gateway
  still sleeps when nothing happens: a wake thread watches the UDP socket, and keeps a 10 ms
  timer only while an ENet client is connected.
- **Closing.** A client that closes waits up to 100 ms for the server to acknowledge what was
  sent last, then closes anyway.
- **Not there:** encryption, and proof of a client's address beyond ENet's connect handshake.
  The TCP path has neither. A public server needs both before UDP is opened to the world.
- **What was given up:** a player whose network blocks UDP cannot connect.

What it gains is the resend after about a round trip. On one machine nothing is lost, so this
cannot be felt or measured without a tool that drops packets (clumsy, for one).

## Stage 3: three channels (built)

ENet numbers its reliable packets per channel. A packet lost on one channel holds back only that
channel. With everything on one channel, a lost fragment of a mail list held back the walk and
the hit behind it for a round trip, as on TCP.

### The rule

A packet may leave the world channel when nothing on the world channel has to be read before or
after it. Packets that have to stay in order among themselves move together, to the same
channel. The server's TCP clients (the parity gate's) get everything as one stream, so the
server may not come to rely on a channel being independent: channels only take ordering away,
they never add meaning.

Whoever sends a packet picks its channel, from a table of the ids it sends
(`rag_connection.c` in the client, `src/packet_channels.c` in the gateway). Whoever receives
takes any packet on any channel, so the two tables need not agree. Each ENet packet holds whole
packets; the gateway closes a client whose ENet packet ends inside one, since the bytes of two
channels would otherwise mix. The client reads a whole packet from whichever channel has one,
the world's first.

### Channel 0: the world (in order, resent)

Everything not named below: spawns and vanishes, walks, attacks, skills, damage, status
changes, the own character's parameters, the inventory, equipment, NPC dialogues and shops,
the map change, the login, the project's own packets except the time answer. An input stamp and
the request it stamps are always here, next to each other.

### Channel 1: text and lists (in order, resent)

| Group | Server to client | Client to server |
|---|---|---|
| Whisper, party, guild and server-wide text | `ZC_WHISPER` 0x0097 and 0x09de, their acks, the whisper settings and list, `ZC_NOTIFY_CHAT_PARTY` 0x0109, `ZC_GUILD_CHAT` 0x017f, `ZC_BROADCAST` 0x009a and 0x01c3 | `CZ_WHISPER` 0x0096, the whisper settings and list request, `CZ_REQUEST_CHAT_PARTY` 0x0108, `CZ_GUILD_CHAT` 0x017e |
| Guild emblem images | `ZC_GUILD_EMBLEM_IMG` 0x0152 | the request 0x0151 and the upload 0x0153 |
| Friends | the list, state, add and delete (0x0201, 0x0206, 0x0207, 0x0209, 0x020a) | add, delete and the answer to a request |
| Mail | every `ZC_*RODEX*`, `ZC_MAIL_*` and `ZC_IRMAIL_*` (30 ids) | every `CZ_*RODEX*`, `CZ_*MAIL_*` and `CZ_IRMAIL_*` (28 ids) |
| Quest log | the quest lists, added and deleted quests, hunt counts (14 ids) | `CZ_ACTIVE_QUEST` 0x02b6 |
| Rankings | the ranking answers (7 ids) | the ranking requests (6 ids) |

That is 69 ids from the server and 46 from the client. The full lists, with names, are in the
two tables.

### Channel 2: the newest (never resent, dropped when a newer one arrived first)

Only the timed exchange: the client's `CZ_REQUEST_TIME2` 0x0360 and the server's `ZC_TIME_SYNC`
0x0f12. A resent sample would time its resend. The client does nothing when an answer is
missing (`server_time_answered` is set and never read) and asks again. The original's answer
`ZC_NOTIFY_TIME` 0x007f stays on the world channel.

### Left on the world channel, and why

- **Storage and cart contents.** This client's item lists are the split lists 0x0b08 to 0x0b0b,
  which the inventory, the cart and the storage share and tell apart by a field. An id cannot
  move the storage without the inventory, and the inventory names items by index in packets
  that mix with the world's. Moving them needs the channel chosen by that field.
- **Guild and party windows.** The lists, the member updates and what they change on actors
  (`ZC_UPDATE_GDID`, `ZC_CHANGE_GUILD`, the party's HP bars over heads) are many packets that
  have to stay in order among themselves. They can move as one group once someone has listed
  the group completely; a packet left behind would show a stale member.
- **Party and guild members' HP and map dots** as newest-only. They are sent when a value
  changes, so a lost last change would stay wrong until the next one. That needs the server to
  repeat them on a timer first.
- **Chat said aloud** (0x008d, 0x008e): it names the actor whose head the bubble goes over, and
  on a Hercules server the same request carries commands.
- **The skill list and the warp list**: skill use on the world channel reads them.

### What is not known

- How often a loss falls into a list during play that matters. Without a number from a link
  that loses packets, the gain is an argument and not a measurement.
- Whether any consumer relies on the order of two packets the tables put on different
  channels. The groups were chosen to make that unlikely; nobody read the consumers for it.

## Stage 4: one connection for a login (built for the way in)

The original logs in over four connections, each closed before the next: the account server
(`CT_AUTH` 0x0acf), the login server (0x0064), the character server (`CH_ENTER` 0x0065) and the
zone server (`CZ_ENTER` 0x0436). On each hop the client says again who it is, with the account
id and the auth code the login gave it. Here all four go to the same gateway port, so the hops
exist only because the original had separate servers.

### What is built

- **The client keeps the link.** At the three places a stage ends and the next server is
  connected (`login/auth.c`, `login/account.c`, `login/zone.c`) the client calls
  `connection_hop` where it called `connection_disconnect`. The socket is kept and serviced,
  what still arrives on it is thrown away, and the next `connection_connect` takes it over when
  it goes to the same address and port. A connect to another address closes the kept link and
  opens a new one, as before. The packets of the login are unchanged.
- **The server needed no change for the way in.** It keys the login's stages by connection id
  and never required a new one: the same connection may send 0x0064, then 0x0065 and 0x0066,
  then 0x0436. The parity gate's client still uses separate connections, which works as before.
- **The player's entity id is still the connection id,** now the one the login was made on.

### What is not built

- **Going back without closing.** Returning to character selection, a logout and a change of
  zone server still close the connection, and the next stage opens a new one. The server cleans
  a player up when its connection closes and at no other time (the despawn, the presence going
  offline, combat's timers and statuses, the luggage going home), and it ignores a `CH_ENTER`
  on a connection that is on a map.
- **Reconnecting.** A token from the login that lets a new connection take over a session
  after a drop. The server keeps a player's unit for 30 s after its transport closes, which is
  the time a reconnect would have. It needs the entity to outlive a connection, so the entity
  id can no longer be the connection id.
- **Dropping the identity from the later stages.** The client still sends the account id and
  auth code in `CH_ENTER` and `CZ_ENTER`, as the packets' layouts have them. The server checks
  the character id and the auth code at `CZ_ENTER`; it could check them against what the same
  connection logged in as.

### Known gap

A late packet of an ended stage is thrown away only while the link is kept. One that arrives
after the next stage has taken the link over is read by that stage. The server answers requests
and sends nothing on its own during the login, so none is expected.

## Stage 5: the delivery piece (built for requests)

A fourth channel carries datagrams that ENet neither resends nor orders. An end of the delivery
piece on each side of the link sends what they carried again until it is acknowledged, a message
at a time and none waiting behind another. The player's requests go through it, so a lost
datagram costs a request 15 ms and not ENet's resend timer. Records, the own body and hit
notices are to follow. `plans/delivery.md` has the design, the steps and what is built.
