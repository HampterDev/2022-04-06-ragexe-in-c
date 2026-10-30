#include "ui/cooldown.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned frame;
static void compare(FILE *file, const void *value, size_t size)
{
    unsigned char expected[sizeof(((struct CooldownTimer *)0)->verts)];
    assert(size <= sizeof expected && fread(expected, size, 1, file) == 1);
    if (memcmp(expected, value, size)) {
        fprintf(stderr, "Cooldown geometry differs at frame %u (%u bytes)\n", frame, (unsigned)size);
        assert(0);
    }
}

static void check(FILE *file, const struct CooldownTimer *t, unsigned queued)
{
    compare(file, &t->segment, sizeof t->segment);
    compare(file, &t->vertex_count, sizeof t->vertex_count);
    compare(file, &t->angle, sizeof t->angle);
    compare(file, &t->total_ms, sizeof t->total_ms);
    compare(file, &t->start, sizeof t->start);
    compare(file, &t->elapsed, sizeof t->elapsed);
    compare(file, &t->last_minute, sizeof t->last_minute);
    compare(file, &t->color, sizeof t->color);
    compare(file, &queued, sizeof queued);
    compare(file, t->verts, sizeof t->verts);
    ++frame;
}

int main(int argc, char **argv)
{
    unsigned totals[] = {0, 8000, 120000, 0x80000100};
    unsigned progress[] = {0, 1, 999, 1000, 1001, 1999, 2000, 2999, 3999, 4000,
        4999, 5999, 6000, 6999, 7000, 7999, 8000, 8001, 59999, 60000, 60001, 65000, 119999, 120000, 120001};
    unsigned a, b, c, i;
    int mode, playing;
    FILE *file;
    assert(argc == 2);
    file = fopen(argv[1], "rb"); assert(file);
    for (mode = 0; mode < 2; ++mode)
    for (playing = 0; playing < 2; ++playing)
    for (a = 0; a < 4; ++a)
    for (b = 0; b < 2; ++b)
    for (c = 0; c < 2; ++c) {
        struct CooldownTimer timer;
        unsigned start = b ? 0xfffffff0 : 1000;
        cooldown_timer_init(&timer, 7, 24, 4, totals[a], start, c ? 1500 : 0, mode);
        for (i = 0; i < sizeof progress / sizeof progress[0]; ++i) {
            unsigned queued = cooldown_timer_layout(&timer, start + progress[i], 2, 3,
                i % 2 ? 16 : 12, i % 2 ? 11 : 15, 200, 10, 20);
            check(file, &timer, queued);
        }
    }
    assert(frame == 1600 && fgetc(file) == EOF);
    fclose(file);
    {
        struct CooldownTimer first, copy;
        cooldown_timer_init(&first, 2, 10, 4, 8000, 20, 0, 0);
        assert(cooldown_timer_layout(&first, 21, 0, 0, 12, 12, 200, 0, 0));
        copy = first;
        assert(copy.edge_mask == 255);
        cooldown_timer_layout(&first, 2000, 0, 0, 16, 16, 200, 0, 0);
        assert(copy.edges[0].reach == 12 && copy.angle != first.angle);
        first.x = 0;
        assert(!cooldown_timer_layout(&first, 2001, 0, 0, 12, 12, 200, 0, 0));
    }
    puts("Cooldown geometry: 1600 captured frames match bit for bit, including eight segments, replay/clock wrap, first dimensions, expiry and last-minute transition; value copies are independent");
    return 0;
}
