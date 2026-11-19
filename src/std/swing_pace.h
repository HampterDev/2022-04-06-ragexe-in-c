#ifndef STD_SWING_PACE_H
#define STD_SWING_PACE_H

/* How fast an attack's animation is played for its hit to be shown at a given moment.
 *
 * An actor's animation shows frame n of an action once n times the action's frame delay have
 * passed, in units of 24 ms (actor/motion_update.c), and an attack shows its hit on the frame
 * its sprite marks for it (attack_motion_count of actor/types.h). So the hit shows hit_frame *
 * frame_delay * 24 ms after the attack began, whatever the server's time for the swing is. The
 * server says when a swing lands (plans/core-loop.md); the frame delay that shows the hit frame
 * then is that time divided by the frame's number.
 *
 * Freestanding: no globals and no library. */

#define SWING_PACE_UNIT_MS 24
/* An attack is played at most this many times as slow, and as fast, as it is on its own: a
 * record that has a swing land in a minute, or long ago, neither holds the animation on a frame
 * nor skips through it. */
#define SWING_PACE_SLOWEST 8
#define SWING_PACE_FASTEST 2

/* The frame delay that shows frame `hit_frame` of an attack `lands_ms` after the attack begins,
 * for an attack whose frame delay on its own is `frame_delay`: no more than SWING_PACE_SLOWEST
 * times that and no less than a SWING_PACE_FASTEST part of it, which is also the answer for a
 * swing that has landed (`lands_ms` not above 0). An attack without a frame to go by
 * (`hit_frame` not above 0) or without a delay of its own keeps the delay it has. */
float swing_pace_frame_delay(float frame_delay, float hit_frame, int lands_ms);

/* When the hit frame shows with a frame delay: the milliseconds after the attack begins. */
float swing_pace_hit_ms(float frame_delay, float hit_frame);

#endif
