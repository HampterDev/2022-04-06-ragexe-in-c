#include "render/screen_wobble.h"

void wobble_frame(struct Wobble *wobble, int height, int mode, int (*next)(void *context), void *context,
                  struct WobbleRow *rows)
{
    short amplitude = wobble->amplitude;
    unsigned char period;
    int source = height - 1, y;

    if (amplitude < 2)
        wobble->amplitude = amplitude = 2;
    /* the original's period as a byte; 0, which the client's amplitudes (2 to 16) never give, made
       it divide by zero: here it moves no row and draws no number */
    period = (unsigned char)(amplitude <= 10 ? amplitude : 20 - amplitude);
    for (y = height - 1; y >= 0; y--) {
        rows[y].source = y;
        rows[y].shift = 0;
        if (!period)
            continue;
        if (y % period == 0) {
            source = y;
        } else {
            /* (rand() % 8) << 4 bytes of a row of 32-bit pixels */
            int shift = (int)((unsigned)next(context) % 8) * 4;
            if (mode == 0 || mode == 1)
                rows[y].source = source;
            if (mode == 1)
                rows[y].shift = shift;
        }
    }
    amplitude = (short)(amplitude + 2);
    wobble->amplitude = amplitude;
    if (amplitude < 2) {
        wobble->amplitude = 2;
    } else if (amplitude > 16) {
        wobble->framesLeft = wobble->framesLeft - 1 < 0 ? 0 : wobble->framesLeft - 1;
        wobble->amplitude = 2;
    }
}
