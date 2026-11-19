#ifndef STD_WAY_UP_H
#define STD_WAY_UP_H

#include <stdint.h>

/* When a packet the client sends now arrives at the server, by the server's clock, from the
   timed exchanges (plans/netcode.md). The client names in each request the server's tick to apply
   it at, and the server waits for that tick only when the request arrives before it: the tick
   has to lie past the arrival, or the server applies the request on arrival and the client's
   own world was wrong about it.

   Each exchange says how long its request took: the server's clock when the request arrived
   less the client's clock when it left, the clocks' offset and the way up in one number. The
   way a request sent now takes is reckoned as the second longest of the last WAY_UP_SAMPLES
   exchanges (the longest while fewer than three are held), so that one request held up on the
   way does not set it, and half as much again as that lies above the shortest, a margin from
   their spread. That is the lead: a request sent at client time `now` arrives by now + lead.
   The lead grows at once when the exchanges say so, and shrinks one millisecond in WAY_UP_SLEW,
   so `now + lead` never runs backward. Every time is a 32-bit tick that may wrap. */

#define WAY_UP_SAMPLES 16
#define WAY_UP_SLEW 10

struct WayUp {
    uint32_t took[WAY_UP_SAMPLES]; /* the server's clock at a request's arrival less the client's at its sending */
    int count;                     /* samples held, at most WAY_UP_SAMPLES */
    int next;                      /* where the next sample goes */
    uint32_t lead;                 /* in use */
    uint32_t slewed_at;            /* client time up to which `lead` has followed its target */
};

void way_up_reset(struct WayUp *way);
/* Takes one exchange: the client's clock when its request left and the server's when it arrived. */
void way_up_sample(struct WayUp *way, uint32_t client_send, uint32_t server_receive);
/* The server's tick at which a packet sent at the client time `now` arrives. Before the first
   sample (`count` 0) it is `now`, which is no tick of the server's. */
uint32_t way_up_arrival(struct WayUp *way, uint32_t now);

#endif
