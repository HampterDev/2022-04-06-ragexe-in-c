#ifndef RENDER_SCREEN_WOBBLE_H
#define RENDER_SCREEN_WOBBLE_H

/* custom: the screen wobble (the player's status 0x108, actor/actor_efst.c) as pure logic. For each
   frame it runs, the original locked the finished frame and went over its rows from the bottom up:
   a row whose number is a multiple of the period (an anchor) stays as it is and is kept, and every
   other row is overwritten with the last row kept, moved right in mode 1 by a random number of
   pixels and in place in mode 0. Since it only ever copies rows it has not changed, each row of the
   result is one row of the frame as it was, moved: struct WobbleRow says which and how far, and the
   device draws the result from that (render/gfx.h, gfx_wobble). Uses nothing else of the client. */

/* Row y of the result is row `source` of the frame, moved right by `shift` pixels; the `shift`
   pixels at its left keep row y's own. */
struct WobbleRow {
    int source;
    int shift;
};

/* What the wobble keeps from one frame to the next: the client's g_wobbleFramesLeft and
   g_wobbleAmplitude (render/cframe.c). */
struct Wobble {
    int framesLeft;
    short amplitude;
};

/* One frame of the wobble over a frame `height` rows tall in `mode` (0 or 1; any other mode moves
   no row): fills rows[0] to rows[height - 1] and advances *wobble as the original did. `next` gives
   the random numbers, the C runtime's rand() in the client (0 to RAND_MAX): it is called once for
   each row that is not an anchor, from the bottom row up, as the original called rand() whatever
   the mode, so that every random number the game draws after the wobble stays the same. A step of
   the random shift was 16 bytes of the frame's 32-bit pixels, 4 pixels. */
void wobble_frame(struct Wobble *wobble, int height, int mode, int (*next)(void *context), void *context,
                  struct WobbleRow *rows);

#endif
