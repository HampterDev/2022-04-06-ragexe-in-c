#ifndef GAME_NET_CLOCK_H
#define GAME_NET_CLOCK_H

#include "contents/connection_api.h"
#include "game/gamemode.h"
#include "session/state.h"
#include "session/session_state.h"
#include "platform/clock.h"
#include "std/way_up.h"
#include "game/own_actor.h"

/* The clocks of plans/netcode.md. The render clock, which everyone else is drawn on, stays
   session_get_server_time. */

/* The server's clock is known: a timed exchange has been answered on this map. From then on
   the player's own walk starts at the click and is drawn on the own clock, and what the server
   sends as timed records (plans/core-loop.md) has a clock to be played on. */
static __inline int game_mode_knows_clock(const struct GameMode *self)
{
    return self->clock_sync.count != 0;
}

/* The server's clock now, from the timed exchanges; the render clock until the first one. */
static __inline DWORD game_mode_true_time(struct GameMode *self)
{
    if (!self->clock_sync.count)
        return session_get_server_time(&g_session1);
    return clock_sync_now(&self->clock_sync, platform_ticks_ms());
}

/* The clock the player's own character is drawn on, and the tick a request sent now names for
   the server to apply it at (CZ_INPUT_STAMP): when a request sent now arrives there, by the
   exchanges' way up and its margin (std/way_up.h), but no sooner than the hold-back after the
   server's clock now. A request that takes longer than that is applied on arrival, and the
   player's body tells it. The clock never runs backward. */
static __inline DWORD game_mode_own_time(struct GameMode *self)
{
    DWORD held = game_mode_true_time(self) + self->hold_back_ms;
    DWORD arrives;

    if (!self->way_up.count)
        return held;
    arrives = way_up_arrival(&self->way_up, platform_ticks_ms());
    return (int)(arrives - held) > 0 ? arrives : held;
}

/* The render clock is the server's clock less the render delay, so that a record is here
   before its tick. Once a frame. */
static __inline void game_mode_pin_render_clock(struct GameMode *self)
{
    DWORD now = platform_ticks_ms();

    if (game_mode_knows_clock(self))
        session_pin_server_time(&g_session1, clock_sync_now(&self->clock_sync, now)
                                             - render_delay_now(&self->render_delay, now));
}

#define CZ_INPUT_STAMP 0xf01

#pragma pack(push, 1)
struct PacketInputStamp {
    unsigned short id;
    unsigned short seq;
    DWORD tick;      /* the server's tick to apply the request at: the own clock now */
    DWORD seen_tick; /* the render clock now: what is on screen */
};
#pragma pack(pop)
SIZE_ASSERT(PacketInputStamp, 12);

/* Stamps the request sent next with the tick it is to be applied at, the own clock now, and with
   what is on screen. The stamp is only queued: it leaves in one write with its request. Returns
   the stamp's number, which is never 0; its tick is kept as stamp_tick. */
static __inline unsigned short game_mode_send_input_stamp(struct GameMode *self)
{
    struct PacketInputStamp packet;

    if (++self->input_seq == 0)
        self->input_seq = 1;
    packet.id = CZ_INPUT_STAMP;
    packet.seq = self->input_seq;
    packet.tick = game_mode_own_time(self);
    self->stamp_tick = packet.tick;
    packet.seen_tick = session_get_server_time(&g_session1);
    rag_connection_queue_packet(rag_connection_instance(), sizeof packet, &packet);
    return packet.seq;
}

/* What the player wants, which the server walks into range for and carries out
   (plans/intents.md). */
#define CZ_INTENT 0xf02

enum { INTENT_ATTACK = 1, INTENT_SKILL_ACTOR = 2, INTENT_SKILL_GROUND = 3, INTENT_PICKUP = 4,
       INTENT_RELEASE = 5 };

#pragma pack(push, 1)
struct PacketIntent {
    unsigned short id;
    unsigned short seq;
    unsigned char kind;
    unsigned char mode;
    unsigned target;
    unsigned short x;
    unsigned short y;
    unsigned short skill_id;
    unsigned short skill_lv;
};
#pragma pack(pop)
SIZE_ASSERT(PacketIntent, 18);

/* The own body's swing as the predicted world has it: what is shown of it follows
   (packets_act.c, which asks the own body for it once a frame). */
void game_mode_own_swing(struct GameMode *self, int has, unsigned target, DWORD at_tick, DWORD lands,
                         unsigned motion_ms);

static __inline void game_mode_send_intent(struct GameMode *self, int kind, int mode, unsigned target, int x,
                                           int y, int skill_id, int skill_lv)
{
    struct PacketIntent packet;
    unsigned short stamp = game_mode_send_input_stamp(self);

    /* One counter numbers every request (plans/core-loop.md): an intent has its stamp's
       number, as a move has, so the server's answers to both are told apart by it. A release
       belongs to the intent it releases, whose end the server answers by that intent's
       number. */
    if (kind != INTENT_RELEASE)
        self->intent_seq = stamp;
    packet.id = CZ_INTENT;
    packet.seq = self->intent_seq;
    packet.kind = (unsigned char)kind;
    packet.mode = (unsigned char)mode;
    packet.target = target;
    packet.x = (unsigned short)x;
    packet.y = (unsigned short)y;
    packet.skill_id = (unsigned short)skill_id;
    packet.skill_lv = (unsigned short)skill_lv;
    rag_connection_send_packet(rag_connection_instance(), sizeof packet, &packet);
    game_mode_own_actor_intent(self, stamp, kind, mode, target, x, y, skill_id, skill_lv);
}

#endif
