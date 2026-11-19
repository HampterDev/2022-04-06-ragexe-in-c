#ifndef STD_CLOCK_SYNC_H
#define STD_CLOCK_SYNC_H

#include <stdint.h>

/* An estimate of a server's clock from timed exchanges, the way NTP does it (plans/netcode.md).
   One exchange gives four millisecond stamps: the client's send, the server's receive, the
   server's send, the client's receive. The exchange with the shortest round trip among the
   last few spent the least time in queues, so its offset is the one trusted. The clock in use
   moves toward that offset a little at a time and never runs backward; it jumps only forward,
   and only when it is more than a second behind or the first samples are still coming in.
   Every time is a 32-bit tick that may wrap. */

#define CLOCK_SYNC_SAMPLES 8

struct ClockSync {
    struct ClockSyncSample {
        uint32_t round_trip;
        uint32_t offset;
    } samples[CLOCK_SYNC_SAMPLES];
    int count;           /* samples held, at most CLOCK_SYNC_SAMPLES */
    int next;            /* where the next sample goes */
    uint32_t round_trip; /* of the trusted sample */
    uint32_t target;     /* server minus client, of the trusted sample */
    uint32_t offset;     /* server minus client, in use */
    uint32_t slewed_at;  /* client time up to which `offset` has followed `target` */
};

void clock_sync_reset(struct ClockSync *sync);
/* Takes one exchange. Returns 0 for an exchange that cannot be right (the server took longer
   than the round trip) and changes nothing. */
int clock_sync_sample(struct ClockSync *sync, uint32_t client_send, uint32_t server_receive,
                      uint32_t server_send, uint32_t client_receive);
/* The server's clock at the client time `now`. Before the first sample it is `now`. */
uint32_t clock_sync_now(struct ClockSync *sync, uint32_t now);

#endif
