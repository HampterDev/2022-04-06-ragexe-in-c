#include "std/walk_steer.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void a_step_within_reach_is_taken_whole(void)
{
    float x = -1.0f, z = -1.0f;

    assert(walk_steer(10.0f, 20.0f, 10.5f, 20.0f, 0.65f, 20.0f, &x, &z) == 0);
    assert(x == 10.5f && z == 20.0f);
    assert(walk_steer(10.0f, 20.0f, 10.0f, 20.0f, 0.0f, 20.0f, &x, &z) == 0);
    assert(x == 10.0f && z == 20.0f);
}

static void a_longer_step_is_cut_along_its_line(void)
{
    float x = 0.0f, z = 0.0f;

    /* the walk puts the character 5 away (3-4-5); it may move 1 */
    assert(walk_steer(10.0f, 20.0f, 13.0f, 24.0f, 1.0f, 20.0f, &x, &z) == 1);
    assert(fabsf(x - 10.6f) < 1e-4f && fabsf(z - 20.8f) < 1e-4f);
}

static void a_gap_too_far_to_walk_off_is_taken_at_once(void)
{
    float x = 0.0f, z = 0.0f;

    assert(walk_steer(10.0f, 20.0f, 40.0f, 20.0f, 1.0f, 20.0f, &x, &z) == 0);
    assert(x == 40.0f && z == 20.0f);
    /* exactly the limit is still walked */
    assert(walk_steer(10.0f, 20.0f, 30.0f, 20.0f, 1.0f, 20.0f, &x, &z) == 1);
    assert(fabsf(x - 11.0f) < 1e-4f && z == 20.0f);
}

static void a_cut_walk_catches_up(void)
{
    /* A character a cell (5 units) behind a walk of 150 ms a cell, drawn every 16 ms at 1.3
       times the speed: the walk moves 5 * 16 / 150 a frame and the character 1.3 times that, so
       it is level after about 3.3 cells of walking and every frame until then is cut. */
    float walk = 5.0f, at = 0.0f, reach = 5.0f * 16.0f * 13.0f / (150.0f * 10.0f), z = 0.0f;
    int frames = 0;

    for (;;) {
        walk += 5.0f * 16.0f / 150.0f;
        if (!walk_steer(at, 0.0f, walk, 0.0f, reach, 20.0f, &at, &z))
            break;
        frames++;
    }
    assert(at == walk && frames >= 30 && frames <= 33);
}

int main(void)
{
    a_step_within_reach_is_taken_whole();
    a_longer_step_is_cut_along_its_line();
    a_gap_too_far_to_walk_off_is_taken_at_once();
    a_cut_walk_catches_up();
    puts("walk_steer: ok");
    return 0;
}
