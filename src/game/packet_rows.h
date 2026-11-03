#ifndef GAME_PACKET_ROWS_H
#define GAME_PACKET_ROWS_H

#include <stddef.h>

struct GameMode;

/* Game packet tables: the rows of game_mode_dispatch_packet (game/packet_dispatch.c) and of
   game_mode_dispatch_packet2 (game/packet_dispatch2.c).

   A dispatcher's packets are one row list, a macro GAME_PACKET_ROWS_n(PACKET, CONSUMED) in
   its file, with the rows
     PACKET(id, call)  `call` handles the packet. In it, `self` is the game mode, `pkt` the
                       packet as received (it starts with its two-byte ID) and `packet_size`
                       its length; a handler that needs more than one statement is a small
                       static function in the dispatcher's file that `call` calls;
     CONSUMED(id)      the ID is known: the packet is read and dropped without effect.
   The list is expanded three times. GAME_PACKET_ADAPTER makes one function per PACKET row, all
   with the one handler signature GamePacketHandler. GAME_PACKET_ENTRY fills the table, an
   array indexed by packet ID, with those functions; CONSUMED rows get game_packet_consumed.
   GAME_PACKET_CASE turns every ID into a case label of a switch that is never called, so an
   ID listed twice is a compile error, as it was in the dispatch switch.

   The order of the rows means nothing; each ID has at most one row in each table, and no ID
   is in both. An ID without a row is unknown to that table, and the dispatcher does what its
   switch's default did: the first table passes the packet to game_mode_dispatch_packet2, the
   second resets the connection's queues. */

typedef void GamePacketHandler(struct GameMode *self, unsigned char *pkt, size_t packet_size);

#define GAME_PACKET_ADAPTER(id, call) \
    static void game_packet_##id(struct GameMode *self, unsigned char *pkt, size_t packet_size) \
    { \
        (void)self; (void)pkt; (void)packet_size; \
        call; \
    }
#define GAME_PACKET_NO_ADAPTER(id)
#define GAME_PACKET_ENTRY(id, call) [id] = game_packet_##id,
#define GAME_PACKET_CONSUMED_ENTRY(id) [id] = game_packet_consumed,
#define GAME_PACKET_CASE(id, call) case id:
#define GAME_PACKET_CONSUMED_CASE(id) case id:

/* The handler of every CONSUMED row (game/packet_dispatch.c): does nothing. */
void game_packet_consumed(struct GameMode *self, unsigned char *pkt, size_t packet_size);

/* The handler of `id` in `table`, or NULL when the table has no row for it. `id` may be any
   int; game_mode_poll_network_msg passes IDs from 0x8000 on as negative numbers. */
#define GAME_PACKET_HANDLER(table, id) \
    ((id) >= 0 && (size_t)(id) < sizeof(table) / sizeof((table)[0]) ? (table)[id] : NULL)

#endif
