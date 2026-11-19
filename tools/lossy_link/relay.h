#ifndef LOSSY_LINK_RELAY_H
#define LOSSY_LINK_RELAY_H

#include "lossy_link/wire.h"

/* A lossy link on this machine (plans/delivery.md, step 6): a UDP port that passes every
 * datagram on to a server's port and the server's answers back, through a wire that loses,
 * repeats and delays them by a seed (lossy_link/wire.h). The client connects to the relay's
 * port and both ends run as they are, ENet's own resends included, so what a loss costs is
 * what it costs on a real link. One client at a time: the one that sent last.
 *
 * The sockets are ENet's (vendor/enet-1.3.18), as on every platform; enet_initialize comes
 * before the first call here. Nothing in here waits: lossy_relay_pump takes what has arrived
 * and sends what is due, and is called every millisecond or so. */

struct LossyRelay;

/* Listens on 127.0.0.1:`listen_port`, any free port for 0, and passes on to
 * 127.0.0.1:`server_port`. Returns NULL when a socket cannot be had. */
struct LossyRelay *lossy_relay_open(uint16_t listen_port, uint16_t server_port, uint32_t seed,
                                    const struct LossyWireConfig *config);
uint16_t lossy_relay_port(const struct LossyRelay *relay);
void lossy_relay_pump(struct LossyRelay *relay, uint32_t now_ms);
/* Waits up to `milliseconds` for a datagram from the client. */
void lossy_relay_wait(struct LossyRelay *relay, uint32_t milliseconds);
/* The wire, for its counts: direction 0 is from the client to the server. */
const struct LossyWire *lossy_relay_wire(const struct LossyRelay *relay);
void lossy_relay_close(struct LossyRelay *relay);

#endif
