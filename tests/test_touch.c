#include "core/user_input.h"
#include "core/touch.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* core/user_input.h as a log of what core/touch.c hands over, one "|"-ended item a frame. */
static char frame_log[256], log_text[4096];
static uint32_t now = 1000;

static void add(char *text, size_t size, const char *item)
{
    size_t length = strlen(text);
    snprintf(text + length, size - length, "%s", item);
}

void input_pointer(int x, int y)
{
    char item[32];
    snprintf(item, sizeof item, "move %d,%d ", x, y);
    add(frame_log, sizeof frame_log, item);
}

void input_button(int button, int down, int x, int y)
{
    char item[48];
    snprintf(item, sizeof item, "%s %s %d,%d ", down ? "press" : "release", button ? "right" : "left", x, y);
    add(frame_log, sizeof frame_log, item);
}

void input_wheel(int delta)
{
    char item[32];
    snprintf(item, sizeof item, "wheel %d ", delta);
    add(frame_log, sizeof frame_log, item);
}

static void frames(int count)
{
    while (count-- > 0) {
        now += 16;
        frame_log[0] = 0;
        touch_frame(now);
        add(log_text, sizeof log_text, frame_log[0] ? frame_log : "- ");
        add(log_text, sizeof log_text, "| ");
    }
}

static void expect(const char *want)
{
    if (strcmp(log_text, want)) {
        fprintf(stderr, "touch: got  %s\n       want %s\n", log_text, want);
        assert(0);
    }
    log_text[0] = 0;
}

int main(void)
{
    int x, y;

    /* A tap: the pointer moves, then the press and the release come a frame each. */
    touch_down(1, 100, 100, now);
    now += 40;
    touch_up(1, 102, 101, now);
    frames(1);
    assert(!touch_pressed(&x, &y));
    frames(1);
    assert(touch_pressed(&x, &y) && x == 100 && y == 100);
    frames(2);
    expect("move 100,100 | press left 100,100 | release left 100,100 | - | ");

    /* A second tap near the first one soon after lands on it: no move, so a double click. */
    now += 200;
    touch_down(2, 108, 95, now);
    now += 30;
    touch_up(2, 108, 95, now);
    frames(3);
    expect("press left 100,100 | release left 100,100 | - | ");

    /* Later and elsewhere: an ordinary tap again. */
    now += 900;
    touch_down(3, 300, 200, now);
    now += 30;
    touch_up(3, 300, 200, now);
    frames(3);
    expect("move 300,200 | press left 300,200 | release left 300,200 | ");

    /* A finger held still presses after a moment, then drags. */
    now += 900;
    touch_down(4, 400, 300, now);
    frames(9);
    expect("- | - | - | - | - | - | - | move 400,300 | press left 400,300 | ");
    touch_move(4, 405, 300, now);
    touch_move(4, 420, 310, now);
    frames(1);
    touch_up(4, 430, 310, now);
    frames(3);
    expect("move 420,310 | move 430,310 | release left 430,310 | - | ");

    /* A finger dragged at once presses where it landed. */
    now += 900;
    touch_down(5, 50, 50, now);
    now += 10;
    touch_move(5, 80, 50, now);
    frames(4);
    expect("move 50,50 | press left 50,50 | move 80,50 | - | ");
    touch_up(5, 80, 50, now);
    frames(2);
    expect("release left 80,50 | - | ");

    /* Two fingers dragged the same way turn the camera: the right button, no left click. */
    now += 900;
    touch_down(6, 100, 100, now);
    now += 30;
    touch_down(7, 200, 100, now);
    now += 20;
    touch_move(7, 230, 100, now);
    touch_move(6, 130, 100, now);
    frames(4);
    expect("move 150,100 | press right 150,100 | move 180,100 | - | ");
    touch_up(6, 130, 100, now);
    touch_move(7, 260, 100, now);
    touch_up(7, 260, 100, now);
    frames(2);
    expect("release right 180,100 | - | ");

    /* Spread apart they zoom in, a notch per step and one a frame; pinched together, out. A finger
       moving on its own between the two moves of a frame does not count. */
    now += 900;
    touch_down(8, 200, 200, now);
    touch_down(9, 300, 200, now);
    now += 100;
    touch_move(9, 340, 200, now);
    touch_move(8, 160, 200, now);
    frames(3);
    expect("move 250,200 wheel -120 | wheel -120 | - | ");
    touch_move(9, 260, 200, now);
    touch_move(8, 240, 200, now);
    frames(5);
    expect("wheel 120 | wheel 120 | wheel 120 | wheel 120 | - | ");
    touch_up(8, 240, 200, now);
    touch_up(9, 260, 200, now);
    frames(1);
    expect("- | ");

    /* Tapped together: a right click between them; twice, a right double click. */
    now += 900;
    touch_down(10, 500, 400, now);
    now += 20;
    touch_down(11, 540, 400, now);
    now += 100;
    touch_up(10, 500, 400, now);
    touch_up(11, 540, 400, now);
    frames(4);
    expect("move 520,400 | press right 520,400 | release right 520,400 | - | ");
    now += 100;
    touch_down(12, 505, 402, now);
    touch_down(13, 545, 402, now);
    now += 80;
    touch_up(12, 505, 402, now);
    touch_up(13, 545, 402, now);
    frames(3);
    expect("press right 520,400 | release right 520,400 | - | ");

    /* A second finger after the first one pressed lets go of the left button first. */
    now += 900;
    touch_down(14, 600, 300, now);
    frames(10);
    expect("- | - | - | - | - | - | - | move 600,300 | press left 600,300 | - | ");
    touch_down(15, 700, 300, now);
    touch_move(15, 740, 300, now);
    touch_move(14, 640, 300, now);
    frames(4);
    expect("release left 600,300 | move 650,300 | press right 650,300 | move 690,300 | ");

    /* The system takes the touches back: the button held goes up, the fingers are forgotten. */
    touch_cancel();
    touch_move(15, 760, 300, now);
    touch_up(14, 600, 300, now);
    frames(2);
    expect("release right 690,300 | - | ");

    /* A third finger is not followed, and two fingers held without moving do nothing. */
    now += 900;
    touch_down(20, 100, 500, now);
    touch_down(21, 200, 500, now);
    touch_down(22, 300, 500, now);
    touch_move(22, 900, 500, now);
    touch_up(22, 900, 500, now);
    now += 400;
    touch_up(20, 100, 500, now);
    touch_up(21, 200, 500, now);
    frames(2);
    expect("- | - | ");

    puts("touch: taps, double taps, hold, drag, two-finger turn, zoom and taps, cancel passed");
    return 0;
}
