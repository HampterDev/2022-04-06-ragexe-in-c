#include "picture_budget.h"
#include <stdio.h>
#include <stdlib.h>

void picture_difference(const uint32_t *a, const uint32_t *b, int width, int height, struct PictureDifference *out)
{
    long n = (long)width * height, i;
    unsigned char *mark = malloc((size_t)n);
    long *stack = malloc((size_t)n * sizeof *stack);
    out->differ = out->over = out->largest = 0;
    out->most = 0;
    if (!mark || !stack) {
        fprintf(stderr, "picture_difference: no memory for %d by %d\n", width, height);
        exit(1);
    }
    for (i = 0; i < n; i++) {
        int c, d = 0;
        for (c = 0; c < 24; c += 8) {
            int e = abs((int)(a[i] >> c & 255) - (int)(b[i] >> c & 255));
            if (e > d)
                d = e;
        }
        out->differ += d > 0;
        if (d > out->most)
            out->most = d;
        mark[i] = d > 1;
        out->over += mark[i];
    }
    for (i = 0; i < n; i++) {
        long group = 0, top = 0;
        if (mark[i] != 1)
            continue;
        stack[top++] = i;
        mark[i] = 2;
        while (top) {
            long at = stack[--top];
            int x = (int)(at % width), y = (int)(at / width), dx, dy;
            group++;
            for (dy = -1; dy <= 1; dy++)
                for (dx = -1; dx <= 1; dx++) {
                    int xx = x + dx, yy = y + dy;
                    if (xx >= 0 && xx < width && yy >= 0 && yy < height && mark[(long)yy * width + xx] == 1) {
                        mark[(long)yy * width + xx] = 2;
                        stack[top++] = (long)yy * width + xx;
                    }
                }
        }
        if (group > out->largest)
            out->largest = group;
    }
    free(mark);
    free(stack);
}

int picture_within_budget(const struct PictureDifference *d, int width, int height)
{
    return d->over * 2000 <= (long)width * height && d->largest <= 16;
}
