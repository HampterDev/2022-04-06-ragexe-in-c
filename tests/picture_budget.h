#ifndef TESTS_PICTURE_BUDGET_H
#define TESTS_PICTURE_BUDGET_H

#include <stdint.h>

/* The budget a picture the GPU draws is held to against the one it is compared with
   (plans/gpu-renderer.md, Decisions): the pixels that differ by more than one colour step in some
   channel at most 0.05 % of the picture, and each group of such pixels that touch (side or corner)
   at most 16 pixels. Pixels are 32-bit words whose low three bytes are the colour; the top byte is
   not compared. */
struct PictureDifference {
    long differ;   /* pixels that differ at all */
    long over;     /* pixels that differ by more than a step */
    long largest;  /* the largest group of those that touch */
    int most;      /* the largest difference of a channel */
};

void picture_difference(const uint32_t *a, const uint32_t *b, int width, int height, struct PictureDifference *out);
int picture_within_budget(const struct PictureDifference *d, int width, int height);

#endif
