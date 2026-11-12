/* render/cframe.c's frame extras around the screen wobble: none without effects, without frames
   left, in mode 2 or on a renderer without rows; otherwise the device is asked to draw one over the
   renderer's height, and the rows it gets, the random numbers drawn and the amplitude and frames
   left are wobble_frame's (render/screen_wobble.h) from the same seed. A device that cannot draw
   the wobble makes the frame draw no random number and advance nothing, as the original's frame
   that could not lock the screen. */
#include "../src/render/cframe.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct Session g_session1;
static int asked, can_draw, asked_count;
static struct WobbleRow drawn[64];

struct ResMgr *g_resMgr(void) { abort(); }
void *res_mgr_get(struct ResMgr *self, const char *name) { (void)self; (void)name; abort(); }
void screen_sprite_frame(struct Renderer *renderer, int x, int y, struct ActRes *act, struct SprRes *spr, int action,
                         int motion, float scale, float angle, DWORD colour, int flags)
{
    (void)renderer; (void)x; (void)y; (void)act; (void)spr; (void)action; (void)motion; (void)scale; (void)angle;
    (void)colour; (void)flags;
    abort();
}
int session_get_battlefield_state(struct Session *self) { assert(self == &g_session1); return 0; }
void renderer_draw_battle_field_icon(struct Renderer *self) { (void)self; abort(); }

int gfx_wobble(const struct WobbleRow *rows, int count, void (*fill)(void *context), void *context)
{
    ++asked;
    asked_count = count;
    assert(rows && count > 0 && count <= 64 && fill);
    if (!can_draw)
        return 0;
    fill(context);
    memcpy(drawn, rows, (size_t)count * sizeof *rows);
    return 1;
}

/* the C runtime's random numbers, counted */
static int c_rand(void *context)
{
    ++*(int *)context;
    return rand();
}

/* The frame's extras over a renderer `height` rows tall from seed: whether the device was asked, and
   that the C runtime's next random number is the one after `draws` of them. */
static int run(int height, unsigned seed, int draws)
{
    struct Renderer renderer;
    int next, i;
    srand(seed);
    for (i = 0; i < draws; i++)
        rand();
    next = rand();
    memset(&renderer, 0, sizeof renderer);
    renderer.height = height;
    asked = 0;
    srand(seed);
    renderer_draw_frame_extras(&renderer);
    assert(rand() == next);
    return asked;
}

static void set_wobble(int effect, int frames, int mode, short amplitude)
{
    g_session1.effectOn = effect;
    g_wobbleFramesLeft = frames;
    g_wobbleMode = (BYTE)mode;
    g_wobbleAmplitude = amplitude;
}

static void check_none(void)
{
    can_draw = 1;
    set_wobble(0, 3, 1, 5);
    assert(!run(40, 1, 0));
    set_wobble(1, 0, 1, 5);
    assert(!run(40, 2, 0));
    set_wobble(1, 3, 2, 5);
    assert(!run(40, 3, 0));
    set_wobble(1, 3, 1, 5);
    assert(!run(0, 4, 0) && g_wobbleFramesLeft == 3 && g_wobbleAmplitude == 5);
    can_draw = 0;
    assert(run(40, 5, 0) && asked_count == 40 && g_wobbleFramesLeft == 3 && g_wobbleAmplitude == 5);
}

static void check_drawn(int mode, int height, int frames, short amplitude, unsigned seed)
{
    struct WobbleRow want[64];
    struct Wobble wobble;
    int y, draws = 0;
    srand(seed);
    wobble.framesLeft = frames;
    wobble.amplitude = amplitude;
    wobble_frame(&wobble, height, mode, c_rand, &draws, want);
    assert(draws > 0);
    can_draw = 1;
    set_wobble(1, frames, mode, amplitude);
    assert(run(height, seed, draws) && asked_count == height);
    for (y = 0; y < height; y++)
        assert(drawn[y].source == want[y].source && drawn[y].shift == want[y].shift);
    assert(g_wobbleFramesLeft == wobble.framesLeft && g_wobbleAmplitude == wobble.amplitude);
}

int main(void)
{
    check_none();
    check_drawn(1, 40, 3, 5, 11);
    check_drawn(0, 40, 3, 5, 12);
    check_drawn(1, 64, 1, 16, 13);
    check_drawn(1, 7, 2, 1, 14);
    puts("Frame extras: no wobble without effects, frames, a mode that moves rows or rows to move; the rows, "
         "random numbers, amplitude and frames of wobble_frame; nothing drawn or advanced where the device "
         "cannot draw the wobble passed");
    return 0;
}
