#ifndef RENDER_DISPLAY_OPTIONS_H
#define RENDER_DISPLAY_OPTIONS_H

#include <limits.h>

/* custom: how the game's window is shown. Windowed, the window has a frame, the player can resize
   and maximise it, and its client area, in pixels, is the size the game draws at; fullscreen, the
   window covers the display it is on at that display's own size, with no change of display mode,
   and the game draws at that size. Sizes here are in pixels. */

/* The smallest windowed client area: the options keep no smaller size, the picker offers none, and
   the window cannot be made smaller. 664 rows are the fewest the original's resolution list had. */
#define DISPLAY_MIN_WIDTH 1024
#define DISPLAY_MIN_HEIGHT 664

/* No window position is known. */
#define DISPLAY_NO_POSITION INT_MIN

/* The display options as OptionInfo.lua keeps them: ISFULLSCREENMODE, ISMAXIMIZED (the window is
   maximised when windowed), WIDTH and HEIGHT (the windowed size, the one a maximised window is
   restored to, also kept while fullscreen), Window_XPos and Window_YPos (the corner of the window's
   frame, in window coordinates, where the window last was neither maximised nor fullscreen), and
   OLD_WIDTH and OLD_HEIGHT (the windowed size that position belongs to). */
struct DisplayOptions {
    int fullscreen;
    int maximized;
    int width, height;
    int x, y;
    int placedWidth, placedHeight;
};

typedef int (*DisplayOptionRead)(void *context, const char *name, int fallback);
typedef void (*DisplayOptionWrite)(void *context, const char *name, int value);

/* Reads the options through read, whose fallback stands for a missing option. A fullscreen that
   older clients saved as exclusive or as borderless (ISBORDERLESS) is the one fullscreen; a file
   without ISMAXIMIZED is not maximised; a windowed size outside 1,024 to 8,192 by 664 to 4,096 is
   1,024 by 768; without both of Window_XPos and Window_YPos no position is known. ISBORDERLESS,
   BITPERPIXEL, DEVICECNT and MODECNT are not read. The position is written only when known. */
void display_options_read(struct DisplayOptions *out, DisplayOptionRead read, void *context);
void display_options_write(const struct DisplayOptions *options, DisplayOptionWrite write, void *context);

/* The options follow the window as it is: whether it is fullscreen, so that a window the system
   takes out of fullscreen stays out of it; when it is not, whether it is maximised; and when it is
   neither, its frame's corner as the position and its client size as the windowed size, if the
   options keep that size. A minimised window changes nothing. */
void display_options_track(struct DisplayOptions *options, int fullscreen, int maximized, int minimized,
                           int clientWidth, int clientHeight, int frameX, int frameY);

/* A windowed size kept within the room the window's display has for a client area, but no smaller
   than the smallest windowed size: the size a window opens at and is restored to. */
void display_fit(int *width, int *height, int roomWidth, int roomHeight);

/* Window coordinates for a length in pixels at a pixel density (pixels a window coordinate; 1 on
   Windows and Android, 2 on a Retina display, 1.25 on Wayland at 125%): to the nearest, or rounded
   up so that the window coordinates hold at least that many whole pixels. */
int display_units(int pixels, float density, int roundUp);

#define DISPLAY_SIZES_MAX 32

/* The graphics settings' resolution picker. In fullscreen it is disabled and shows the display's
   own size. Windowed, it shows the given size (the window's, maximised or dragged, or the one
   picked) and offers, smallest first, those of a fixed list of common sizes and the given size
   itself that fit the room the window's display has for a client area (its usable area less the
   window's frame), and that room itself, the largest window that fits; only sizes the options
   keep. With nothing to offer it is disabled. */
struct DisplayPicker {
    int enabled;
    int width, height;
    int count;
    int sizes[DISPLAY_SIZES_MAX][2];
    /* the index of width by height in sizes, or -1 */
    int selected;
};

void display_picker_build(struct DisplayPicker *picker, int fullscreen, int nativeWidth, int nativeHeight,
                          int roomWidth, int roomHeight, int width, int height);

#endif
