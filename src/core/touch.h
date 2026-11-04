#ifndef CORE_TOUCH_H
#define CORE_TOUCH_H

#include <stdint.h>

/* A touch screen as the client's mouse (core/user_input.h):
   - one finger is the left button: a tap clicks where it lands, a finger held or dragged presses
     there and moves with the finger;
   - two fingers dragged together turn the camera, as the right button dragged does; spread apart
     or pinched together they zoom it, as the wheel does; tapped together they right-click.
   A first finger waits a moment before it presses, so that a second one landing just after it
   makes a two-finger gesture and no left click. The client reads its input once a frame and finds
   what is under the pointer then, so touch_frame hands the input over at most one button change and
   one wheel notch a frame, a button never in the frame the pointer moved before it; and a tap close
   to the last one lands exactly on it, so that two taps make a double click.
   Positions are scene pixels and times milliseconds; the SDL layer (core/app_sdl.c) feeds the
   fingers of direct touch screens and calls touch_frame once a frame. */

void touch_down(uint64_t finger, int x, int y, uint32_t ms);
void touch_move(uint64_t finger, int x, int y, uint32_t ms);
void touch_up(uint64_t finger, int x, int y, uint32_t ms);
/* The system took the touches back: forget the fingers and let go of any button held. */
void touch_cancel(void);
void touch_frame(uint32_t ms);
/* Whether a finger pressed the left button this frame, and where. */
int touch_pressed(int *x, int *y);

#endif
