#ifndef CORE_USER_INPUT_H
#define CORE_USER_INPUT_H

/* The client's keyboard and pointer input, whoever delivers it: the SDL application layer
   (core/app_sdl.c, touch screens through core/touch.c) and the scene scripts
   (tests/scenes/scene_driver.c). Keys are Windows virtual-key codes, which the client names its keys
   by; positions are scene pixels. */

/* One key transition. */
struct InputKey {
    int vk;
    int scancode; /* PC/AT set 1 make code; 0x100 marks the E0-prefixed (extended) keys */
    int down;
    int repeat;   /* down while already down: auto-repeat */
    int alt;      /* an Alt key is held */
    int count;    /* auto-repeats folded into this one transition, as a Windows key message counts
                     them (a key the IME took, core/app_sdl.c); 0 counts as 1 */
};

enum { INPUT_BUTTON_LEFT, INPUT_BUTTON_RIGHT, INPUT_BUTTON_MIDDLE };

/* The Windows key message parameter for a transition (repeat count, scan code, extended, context,
   previous state, transition), as the client's window manager and edit boxes read it. */
int input_key_lparam(const struct InputKey *key);

/* Routes a key: a press with Alt held, or F10, goes to the window manager's system-key handling (the
   original's WM_SYSKEYDOWN), any other press and every release to the client's key handling. */
void input_key(const struct InputKey *key);

/* One character typed on its own: Backspace or printable ASCII (platform/language.h,
   language_on_char); other text goes to language_commit. */
void input_char(int ch);

void input_pointer(int x, int y);
void input_button(int button, int down, int x, int y);
/* 120 per notch away from the user, as WM_MOUSEWHEEL counts. */
void input_wheel(int delta);

#endif
