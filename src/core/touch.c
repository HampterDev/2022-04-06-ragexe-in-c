#include "core/touch.h"
#include "core/user_input.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum {
    HOLD_MS = 120,      /* a still first finger presses after this long */
    TAP_MS = 250,       /* two fingers up within this long of landing are a tap */
    DOUBLE_MS = 500,    /* the client's double-click time */
    SLOP = 10,          /* scene pixels a finger may wander and still be a tap */
    DOUBLE_SLOP = 24,   /* how near a tap must land to the last one to make a double click */
    PINCH_STEP = 40,    /* scene pixels of spread per wheel notch */
    WHEEL_NOTCH = 120,
    QUEUE_SIZE = 32,
};

enum gesture {
    IDLE,    /* no finger */
    PENDING, /* one finger, not pressed yet */
    LEFT,    /* one finger, left button held */
    TWO,     /* two fingers, neither turning nor zooming yet */
    TURN,    /* two fingers, right button held, the pointer between them */
    ZOOM,    /* two fingers, the spread turned into wheel notches */
    DONE,    /* the gesture ended; waiting for the fingers to lift */
};

enum action_kind { MOVE, PRESS, RELEASE, WHEEL };

struct action {
    enum action_kind kind;
    int a, b; /* MOVE x, y; PRESS and RELEASE the button; WHEEL the delta */
};

struct finger {
    int used;
    uint64_t id;
    int x, y;
};

struct tap {
    uint32_t ms;
    int x, y, valid;
};

static struct {
    struct finger fingers[2];
    int count;
    enum gesture gesture;
    uint32_t since;
    int start_x, start_y, start_spread, zoom_spread;
    int moved; /* a finger of a two-finger gesture moved since the last frame */
    struct tap last_tap[2]; /* left, right */
    /* What the client is to get, in order, and the pointer and buttons as they will be after it. */
    struct action queue[QUEUE_SIZE];
    int head, length;
    int out_x, out_y, out_known, held[2];
    /* The pointer as the client has it, and the left press handed over this frame. */
    int at_x, at_y;
    int pressed;
} T;

static void push(enum action_kind kind, int a, int b)
{
    struct action *last = T.length ? &T.queue[(T.head + T.length - 1) % QUEUE_SIZE] : NULL;
    if (kind == MOVE && last && last->kind == MOVE) {
        last->a = a;
        last->b = b;
        return;
    }
    if (T.length == QUEUE_SIZE)
        return;
    T.queue[(T.head + T.length) % QUEUE_SIZE] = (struct action){kind, a, b};
    T.length++;
}

static void emit_move(int x, int y)
{
    if (T.out_known && x == T.out_x && y == T.out_y)
        return;
    T.out_x = x;
    T.out_y = y;
    T.out_known = 1;
    push(MOVE, x, y);
}

static void emit_button(int button, int down)
{
    if (T.held[button] == down)
        return;
    T.held[button] = down;
    push(down ? PRESS : RELEASE, button, 0);
}

static struct finger *find(uint64_t id)
{
    int i;
    for (i = 0; i < 2; i++)
        if (T.fingers[i].used && T.fingers[i].id == id)
            return &T.fingers[i];
    return NULL;
}

static int distance(int ax, int ay, int bx, int by)
{
    double dx = ax - bx, dy = ay - by;
    return (int)sqrt(dx * dx + dy * dy);
}

static void midpoint(int *x, int *y)
{
    *x = (T.fingers[0].x + T.fingers[1].x) / 2;
    *y = (T.fingers[0].y + T.fingers[1].y) / 2;
}

static int spread(void)
{
    return distance(T.fingers[0].x, T.fingers[0].y, T.fingers[1].x, T.fingers[1].y);
}

/* Where a press lands: on the last tap of the same button when this one is near it in place and
   time, so that the pointer does not move between them and the client sees a double click. */
static void press_at(int button, int x, int y, uint32_t ms)
{
    struct tap *last = &T.last_tap[button];
    if (last->valid && ms - last->ms <= DOUBLE_MS && distance(x, y, last->x, last->y) <= DOUBLE_SLOP) {
        x = last->x;
        y = last->y;
    }
    emit_move(x, y);
    emit_button(button, 1);
    *last = (struct tap){ms, x, y, 1};
}

static void start_left(uint32_t ms)
{
    press_at(INPUT_BUTTON_LEFT, T.start_x, T.start_y, ms);
    T.gesture = LEFT;
}

static void start_two(uint32_t ms)
{
    T.gesture = TWO;
    T.since = ms;
    midpoint(&T.start_x, &T.start_y);
    T.start_spread = T.zoom_spread = spread();
}

static void end_gesture(void)
{
    T.gesture = T.count ? DONE : IDLE;
}

static void zoom_steps(void)
{
    int now = spread();
    for (; now - T.zoom_spread >= PINCH_STEP; T.zoom_spread += PINCH_STEP)
        push(WHEEL, -WHEEL_NOTCH, 0);
    for (; T.zoom_spread - now >= PINCH_STEP; T.zoom_spread -= PINCH_STEP)
        push(WHEEL, WHEEL_NOTCH, 0);
}

/* Two fingers are judged on where both are at the end of a frame, as each finger's move arrives
   on its own: the gesture turns when they went the same way, and zooms when they went apart or
   together, whichever shows first. */
static void two_finger_update(void)
{
    int mx, my, turned, spreaded;
    if (!T.moved)
        return;
    T.moved = 0;
    midpoint(&mx, &my);
    switch (T.gesture) {
    case TWO:
        turned = distance(mx, my, T.start_x, T.start_y);
        spreaded = abs(spread() - T.start_spread);
        if (turned <= SLOP && spreaded <= SLOP)
            break;
        if (turned >= spreaded) {
            emit_move(T.start_x, T.start_y);
            emit_button(INPUT_BUTTON_RIGHT, 1);
            emit_move(mx, my);
            T.gesture = TURN;
        } else {
            emit_move(mx, my);
            T.gesture = ZOOM;
            zoom_steps();
        }
        break;
    case TURN:
        emit_move(mx, my);
        break;
    case ZOOM:
        zoom_steps();
        break;
    default:
        break;
    }
}

void touch_down(uint64_t id, int x, int y, uint32_t ms)
{
    struct finger *slot = NULL;
    int i;
    if (find(id))
        return;
    for (i = 0; i < 2 && !slot; i++)
        if (!T.fingers[i].used)
            slot = &T.fingers[i];
    if (!slot)
        return;
    *slot = (struct finger){1, id, x, y};
    T.count++;
    switch (T.gesture) {
    case IDLE:
        /* Something else may have moved the pointer since (a mouse), unless this is the second tap
           of a double click. */
        if (!(T.last_tap[0].valid && ms - T.last_tap[0].ms <= DOUBLE_MS)
            && !(T.last_tap[1].valid && ms - T.last_tap[1].ms <= DOUBLE_MS))
            T.out_known = 0;
        T.gesture = PENDING;
        T.since = ms;
        T.start_x = x;
        T.start_y = y;
        break;
    case PENDING:
        start_two(ms);
        break;
    case LEFT:
        emit_button(INPUT_BUTTON_LEFT, 0);
        start_two(ms);
        break;
    default:
        break;
    }
}

void touch_move(uint64_t id, int x, int y, uint32_t ms)
{
    struct finger *finger = find(id);
    (void)ms;
    if (!finger)
        return;
    finger->x = x;
    finger->y = y;
    switch (T.gesture) {
    case PENDING:
        if (distance(x, y, T.start_x, T.start_y) > SLOP) {
            emit_move(T.start_x, T.start_y);
            emit_button(INPUT_BUTTON_LEFT, 1);
            T.gesture = LEFT;
            emit_move(x, y);
        }
        break;
    case LEFT:
        emit_move(x, y);
        break;
    case TWO:
    case TURN:
    case ZOOM:
        T.moved = 1;
        break;
    default:
        break;
    }
}

void touch_up(uint64_t id, int x, int y, uint32_t ms)
{
    struct finger *finger = find(id);
    int mx = 0, my = 0;
    if (!finger)
        return;
    if (finger->x != x || finger->y != y)
        T.moved = 1;
    finger->x = x;
    finger->y = y;
    if (T.count == 2) {
        two_finger_update();
        midpoint(&mx, &my);
    }
    finger->used = 0;
    T.count--;
    switch (T.gesture) {
    case PENDING:
        press_at(INPUT_BUTTON_LEFT, T.start_x, T.start_y, ms);
        emit_button(INPUT_BUTTON_LEFT, 0);
        end_gesture();
        break;
    case LEFT:
        emit_move(x, y);
        emit_button(INPUT_BUTTON_LEFT, 0);
        end_gesture();
        break;
    case TWO:
        if (ms - T.since <= TAP_MS) {
            press_at(INPUT_BUTTON_RIGHT, mx, my, ms);
            emit_button(INPUT_BUTTON_RIGHT, 0);
        }
        end_gesture();
        break;
    case TURN:
        emit_button(INPUT_BUTTON_RIGHT, 0);
        end_gesture();
        break;
    default:
        end_gesture();
        break;
    }
}

void touch_cancel(void)
{
    memset(T.fingers, 0, sizeof T.fingers);
    T.count = 0;
    T.moved = 0;
    T.gesture = IDLE;
    emit_button(INPUT_BUTTON_LEFT, 0);
    emit_button(INPUT_BUTTON_RIGHT, 0);
}

void touch_frame(uint32_t ms)
{
    int moved = 0, button = 0, wheeled = 0;
    if (T.gesture == PENDING && ms - T.since >= HOLD_MS)
        start_left(ms);
    two_finger_update();
    T.pressed = 0;
    while (T.length) {
        struct action *a = &T.queue[T.head];
        if (a->kind == MOVE) {
            if (button)
                break;
            T.at_x = a->a;
            T.at_y = a->b;
            input_pointer(T.at_x, T.at_y);
            moved = 1;
        } else if (a->kind == WHEEL) {
            /* The client takes one wheel notch a frame. */
            if (wheeled)
                break;
            input_wheel(a->a);
            wheeled = 1;
        } else {
            if (moved || button)
                break;
            input_button(a->a, a->kind == PRESS, T.at_x, T.at_y);
            T.pressed = a->kind == PRESS && a->a == INPUT_BUTTON_LEFT;
            button = 1;
        }
        T.head = (T.head + 1) % QUEUE_SIZE;
        T.length--;
    }
}

int touch_pressed(int *x, int *y)
{
    if (T.pressed) {
        *x = T.at_x;
        *y = T.at_y;
    }
    return T.pressed;
}
