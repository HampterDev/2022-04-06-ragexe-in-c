#ifndef WORLD_OWN_SWING_H
#define WORLD_OWN_SWING_H

#include <stdint.h>
#include "view/act_track.h"

/* The player's own swing, shown when the predicted world's body begins it and not a round trip
 * later when the server's record of it arrives (plans/predicted-world.md). Whether the body
 * swings, when, at whom and with which attack motion is the world's to say (prediction/predicted.h,
 * predicted_swing). This is the book kept of the swing that is shown, so that it is shown
 * once: not again every frame the world has it, not again when the server's record of it
 * comes, and taken back when the world has it no more before it landed (a walk ended it, or
 * the server's word was another). Freestanding: no allocation, no globals. Ticks are the
 * client's, 32 bits that may wrap.
 *
 * Whoever plays the swing shown paces its animation (game/packets_act.c): the frame its hit is
 * on shows when the damage number is here, which comes with the server's notice. */

struct OwnSwing {
    uint32_t target;     /* of the swing shown last */
    uint32_t at_tick;    /* when it began */
    uint32_t lands;      /* and when it lands */
    unsigned char shown; /* OWN_SWING_AHEAD, OWN_SWING_TOLD, or 0: none, or it was taken back */
};

/* Whose word the swing shown is: the world's, shown ahead of the server's record, or the
 * server's record, which the world had not predicted. Only the first is the world's to take
 * back; the server ends its own with an idle record. */
enum { OWN_SWING_AHEAD = 1, OWN_SWING_TOLD = 2 };

/* How close two starts have to be to be the same swing: the world's after a word of the
 * server's and the one shown, or the server's record and the one shown. */
#define OWN_SWING_MATCH_MS 250

void own_swing_reset(struct OwnSwing *self);

enum {
    OWN_SWING_KEEP = 0,      /* nothing: what is shown is what the world has, or was */
    OWN_SWING_SHOW = 1,      /* the world's swing is to be shown */
    OWN_SWING_TAKE_BACK = 2, /* the swing shown is not to land: it is taken back */
    OWN_SWING_REPLACE = 3    /* both: the one shown is taken back and the world's is shown */
};

/* What the world says of the body's swing at `own_now`, once a frame: with `has` one in its
 * windup at `target` that began at at_tick and lands at `lands`, or none. Returns what that
 * asks of what is shown, and keeps the book: after SHOW and REPLACE the world's swing is the
 * one shown, after TAKE_BACK none is. A swing that has landed is not shown after the fact,
 * and one that landed is not taken back, nor one the server told. While a swing the server
 * told has not landed, the world's swing at the same target is that swing. */
int own_swing_follow(struct OwnSwing *self, int has, uint32_t target, uint32_t at_tick, uint32_t lands,
                     uint32_t own_now);

/* Whether a swing at `target` that began at at_tick is the one shown. */
int own_swing_is_shown(const struct OwnSwing *self, uint32_t target, uint32_t at_tick);

/* The record of the swing shown, to play: a swing of `gid` whose attack motion is `motion_ms`. */
void own_swing_shown(const struct OwnSwing *self, uint32_t gid, unsigned motion_ms, struct ActRecord *record);

/* The server's record of a swing of the player's own arrived. Returns 1 when it is the swing
 * shown, which then is not to be played again; 0 when it is another, which is played when
 * its tick comes and is the one shown from now on. */
int own_swing_record(struct OwnSwing *self, const struct ActRecord *record);

#endif
