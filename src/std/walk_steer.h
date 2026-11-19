#ifndef STD_WALK_STEER_H
#define STD_WALK_STEER_H

/* How a drawn character is steered to the place its walk gives it (plans/netcode.md),
   freestanding so it is tested alone. */

/* One frame's step of a steered character from (from_x, from_z) toward (want_x, want_z), the
   place its walk gives it now. A step of at most `reach` is taken whole. So is one longer than
   `jump`, which is too far to walk off. Any other is cut to `reach` along the same line, so a
   walk that puts the character elsewhere is joined over a few cells and not in one frame.
   Returns 1 when the step was cut, 0 when the character is where its walk puts it. */
int walk_steer(float from_x, float from_z, float want_x, float want_z, float reach, float jump,
               float *x, float *z);

/* The ms a frame's steer may cover: what the clock the walk is played on advanced since the last
   frame, but no more than the time that passed. A clock can step forward at once (the own clock
   when its lead grows); the character then runs behind its walk and catches up as any steered
   walk does, rather than being carried the whole step in one frame. Never negative. */
int walk_steer_ms(int clock_ms, int real_ms);

#endif
