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

#endif
