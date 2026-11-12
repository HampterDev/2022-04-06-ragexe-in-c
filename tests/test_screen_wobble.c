/* render/screen_wobble.c against the loop it replaced: the original's row loop (render/cframe.c
   before the wobble was drawn by the device), run on a frame of 32-bit pixels in memory, and the
   row table applied to the same frame give the same pixels, draw the same random numbers from the
   C runtime in the same order, and leave the same amplitude and frames, for every period, both
   modes, frames of many heights and a pitch wider than the row; and the amplitudes the client
   never reaches neither crash nor draw. */
#include "render/screen_wobble.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char BYTE;

/* The original's loop over a locked frame, as render/cframe.c had it, with the target's lock
   (bits, pitch) and the globals passed in. */
static void original(BYTE *bits, long pitch, int height, BYTE mode, short *amplitude, int *framesLeft)
{
    BYTE *buf = (BYTE *)malloc((size_t)pitch);
    short amp = *amplitude;
    int y;
    assert(buf);
    if (amp < 2)
        *amplitude = amp = 2;
    for (y = height - 1; y >= 0; y--) {
        BYTE *row = bits + y * pitch;
        int period = amp <= 10 ? amp : 20 - amp;
        if (y == height - 1)
            memcpy(buf, row, (size_t)pitch);
        if (y % (BYTE)period == 0) {
            memcpy(buf, row, (size_t)pitch);
        } else {
            int shift = (rand() % 8) << 4;
            if (mode == 0)
                memcpy(row, buf, (size_t)pitch);
            else if (mode == 1)
                memcpy(row + shift, buf, (size_t)(pitch - shift));
        }
        amp = *amplitude;
    }
    amp = (short)(amp + 2);
    *amplitude = amp;
    if (amp < 2) {
        *amplitude = 2;
    } else if (amp > 16) {
        *framesLeft = *framesLeft - 1 < 0 ? 0 : *framesLeft - 1;
        *amplitude = 2;
    }
    free(buf);
}

/* What the device draws from the table: row y from row rows[y].source moved right by rows[y].shift
   pixels, the pixels left of that row y's own. */
static void apply(const BYTE *frame, BYTE *out, long pitch, int width, int height, const struct WobbleRow *rows)
{
    int x, y;
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++) {
            const BYTE *from = x >= rows[y].shift ? frame + rows[y].source * pitch + (x - rows[y].shift) * 4
                                                  : frame + y * pitch + x * 4;
            memcpy(out + y * pitch + x * 4, from, 4);
        }
}

static int c_rand(void *context)
{
    int *calls = context;
    ++*calls;
    return rand();
}

static unsigned pixel_state = 1;

static void fill(BYTE *frame, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++) {
        pixel_state = pixel_state * 1103515245u + 12345u;
        frame[i] = (BYTE)(pixel_state >> 16);
    }
}

static void compare(int width, int padding, int height, int mode, short amplitude, int framesLeft, unsigned seed)
{
    long pitch = (long)width * 4 + padding;
    size_t size = (size_t)pitch * (size_t)height;
    BYTE *frame = malloc(size), *expected = malloc(size), *got = malloc(size);
    struct WobbleRow *rows = malloc((size_t)height * sizeof *rows);
    struct Wobble wobble;
    short originalAmplitude = amplitude;
    int originalFrames = framesLeft, calls = 0, y, after, check;
    assert(frame && expected && got && rows);
    fill(frame, size);
    memcpy(expected, frame, size);
    memcpy(got, frame, size);

    srand(seed);
    original(expected, pitch, height, (BYTE)mode, &originalAmplitude, &originalFrames);
    after = rand();

    srand(seed);
    wobble.framesLeft = framesLeft;
    wobble.amplitude = amplitude;
    wobble_frame(&wobble, height, mode, c_rand, &calls, rows);
    check = rand();
    apply(frame, got, pitch, width, height, rows);

    assert(after == check);
    assert(wobble.amplitude == originalAmplitude && wobble.framesLeft == originalFrames);
    for (y = 0; y < height; y++) {
        assert(!memcmp(expected + y * pitch, got + y * pitch, (size_t)width * 4));
        assert(rows[y].source >= y && rows[y].source < height);
        assert(rows[y].shift >= 0 && rows[y].shift <= 28 && rows[y].shift % 4 == 0);
        assert(mode == 1 || rows[y].shift == 0);
    }
    free(frame);
    free(expected);
    free(got);
    free(rows);
}

/* Every amplitude the client's wobble passes through (2 to 16, the periods 2 to 10), those below 2
   that it clamps and those above that it never stores, in both modes and one that moves nothing,
   over the heights of the game's windows and odd small ones, with pitches wider than the row as
   Direct3D 9's could be. */
static void against_original(void)
{
    static const int heights[] = { 1, 2, 3, 5, 9, 17, 64, 100, 664, 768, 900, 1080, 1441 };
    static const short amplitudes[] = { -32768, -3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
                                        18, 19, 21, 22, 30, 100, 275, 277, 32767 };
    static const int modes[] = { 0, 1, 3 };
    size_t h, a, m;
    unsigned seed = 7;
    for (h = 0; h < sizeof heights / sizeof *heights; h++)
        for (a = 0; a < sizeof amplitudes / sizeof *amplitudes; a++)
            for (m = 0; m < sizeof modes / sizeof *modes; m++) {
                int width = heights[h] > 300 ? 40 : 32 + (int)(h % 3);
                compare(width, (int)(a % 3) * 4, heights[h], modes[m], amplitudes[a], (int)(a % 4), seed++);
            }
}

/* A wobble as the client runs it: from amplitude 2 to 16 in steps of 2, eight frames a run, each
   frame left counting one run down. */
static void runs(void)
{
    struct Wobble wobble;
    struct WobbleRow rows[50];
    int calls = 0, frames = 0;
    wobble.framesLeft = 2;
    wobble.amplitude = 0;
    while (wobble.framesLeft > 0) {
        short before = wobble.amplitude < 2 ? 2 : wobble.amplitude;
        wobble_frame(&wobble, 50, 1, c_rand, &calls, rows);
        assert(wobble.amplitude == (before + 2 > 16 ? 2 : before + 2));
        frames++;
    }
    assert(frames == 16 && wobble.amplitude == 2);
}

/* Amplitudes whose period is 0 as a byte (20, 276, 532), which made the original divide by zero,
   move no row and draw no number; a frame without rows draws none either. */
static void degenerate(void)
{
    static const short zero[] = { 20, 276, 532 };
    struct WobbleRow rows[40];
    struct Wobble wobble;
    size_t i;
    int calls = 0, y;
    for (i = 0; i < sizeof zero / sizeof *zero; i++) {
        wobble.framesLeft = 3;
        wobble.amplitude = zero[i];
        wobble_frame(&wobble, 40, 1, c_rand, &calls, rows);
        assert(calls == 0);
        for (y = 0; y < 40; y++)
            assert(rows[y].source == y && rows[y].shift == 0);
        assert(wobble.amplitude == 2 && wobble.framesLeft == 2);
    }
    wobble.framesLeft = 1;
    wobble.amplitude = 2;
    wobble_frame(&wobble, 0, 1, c_rand, &calls, rows);
    assert(calls == 0 && wobble.amplitude == 4 && wobble.framesLeft == 1);
}

/* Random frames: any height up to 2,000 rows, any amplitude, any mode, against the original. */
static void fuzz(void)
{
    unsigned state = 12345;
    int i;
    for (i = 0; i < 400; i++) {
        int height, mode, framesLeft;
        short amplitude;
        state = state * 1664525u + 1013904223u;
        height = 1 + (int)(state >> 8) % 2000;
        state = state * 1664525u + 1013904223u;
        amplitude = (short)(state >> 16);
        if ((unsigned char)(amplitude <= 10 ? amplitude : 20 - amplitude) == 0 && amplitude >= 2)
            amplitude = 2;
        mode = (int)(state >> 4) % 3;
        framesLeft = (int)(state >> 20) % 5;
        compare(32, (int)(state >> 12) % 2 * 8, height, mode, amplitude, framesLeft, state);
    }
}

int main(void)
{
    against_original();
    runs();
    degenerate();
    fuzz();
    puts("Screen wobble: the row table against the original's row loop (pixels, random numbers, amplitude and "
         "frames) for every period, both modes and pitches wider than the row, a run, the periods of 0 and "
         "random frames passed.");
    return 0;
}
