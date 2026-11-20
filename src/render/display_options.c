#include "render/display_options.h"
#include <string.h>

/* The windowed sizes the options take: the original's resolution list kept modes up to 8,192 by
   4,096, and a saved height below 664 gave 1,024 by 768. */
#define MAX_WIDTH 0x2000
#define MAX_HEIGHT 0x1000
#define DEFAULT_WIDTH 0x400
#define DEFAULT_HEIGHT 0x300

static int takes(int w, int h)
{
    return w >= DISPLAY_MIN_WIDTH && w <= MAX_WIDTH && h >= DISPLAY_MIN_HEIGHT && h <= MAX_HEIGHT;
}

void display_options_read(struct DisplayOptions *out, DisplayOptionRead read, void *context)
{
    out->fullscreen = read(context, "ISFULLSCREENMODE", 0) != 0;
    out->maximized = read(context, "ISMAXIMIZED", 0) != 0;
    out->width = read(context, "WIDTH", DEFAULT_WIDTH);
    out->height = read(context, "HEIGHT", DEFAULT_HEIGHT);
    if (!takes(out->width, out->height)) {
        out->width = DEFAULT_WIDTH;
        out->height = DEFAULT_HEIGHT;
    }
    out->x = read(context, "Window_XPos", DISPLAY_NO_POSITION);
    out->y = read(context, "Window_YPos", DISPLAY_NO_POSITION);
    if (out->x == DISPLAY_NO_POSITION || out->y == DISPLAY_NO_POSITION)
        out->x = out->y = DISPLAY_NO_POSITION;
    out->placedWidth = read(context, "OLD_WIDTH", 0);
    out->placedHeight = read(context, "OLD_HEIGHT", 0);
}

void display_options_write(const struct DisplayOptions *options, DisplayOptionWrite write, void *context)
{
    write(context, "ISFULLSCREENMODE", options->fullscreen != 0);
    write(context, "ISMAXIMIZED", options->maximized != 0);
    write(context, "WIDTH", options->width);
    write(context, "HEIGHT", options->height);
    if (options->x != DISPLAY_NO_POSITION && options->y != DISPLAY_NO_POSITION) {
        write(context, "Window_XPos", options->x);
        write(context, "Window_YPos", options->y);
    }
    write(context, "OLD_WIDTH", options->placedWidth);
    write(context, "OLD_HEIGHT", options->placedHeight);
}

void display_options_track(struct DisplayOptions *options, int fullscreen, int maximized, int minimized,
                           int clientWidth, int clientHeight, int frameX, int frameY)
{
    if (minimized)
        return;
    options->fullscreen = fullscreen != 0;
    if (fullscreen)
        return;
    options->maximized = maximized != 0;
    if (maximized)
        return;
    options->x = frameX;
    options->y = frameY;
    if (takes(clientWidth, clientHeight)) {
        options->width = clientWidth;
        options->height = clientHeight;
    }
}

void display_fit(int *width, int *height, int roomWidth, int roomHeight)
{
    if (*width > roomWidth)
        *width = roomWidth;
    if (*height > roomHeight)
        *height = roomHeight;
    if (*width < DISPLAY_MIN_WIDTH)
        *width = DISPLAY_MIN_WIDTH;
    if (*height < DISPLAY_MIN_HEIGHT)
        *height = DISPLAY_MIN_HEIGHT;
}

int display_units(int pixels, float density, int roundUp)
{
    int units;
    if (!(density > 0.0f))
        return pixels;
    if (!roundUp)
        return (int)(pixels / density + 0.5f);
    units = (int)(pixels / density);
    while ((float)units * density < (float)pixels)
        units++;
    return units;
}

/* 4:3, 5:4, 16:9, 16:10 and 21:9 sizes from 1,024 by 768 up, by width and then height. */
static const int common_sizes[][2] = {
    {1024, 768}, {1152, 864}, {1280, 720}, {1280, 768}, {1280, 800}, {1280, 960}, {1280, 1024},
    {1360, 768}, {1366, 768}, {1400, 1050}, {1440, 900}, {1600, 900}, {1600, 1200}, {1680, 1050},
    {1920, 1080}, {1920, 1200}, {2048, 1152}, {2560, 1080}, {2560, 1440}, {2560, 1600}, {3440, 1440},
    {3840, 1600}, {3840, 2160}, {5120, 1440}, {5120, 2160}, {5120, 2880},
};

static int before(const int *a, int w, int h)
{
    return a[0] < w || (a[0] == w && a[1] < h);
}

/* Puts w by h into the picker's sizes in their order, once. */
static void offer(struct DisplayPicker *picker, int w, int h)
{
    int i = 0, k;

    while (i < picker->count && before(picker->sizes[i], w, h))
        i++;
    if ((i < picker->count && picker->sizes[i][0] == w && picker->sizes[i][1] == h)
        || picker->count == DISPLAY_SIZES_MAX)
        return;
    for (k = picker->count; k > i; k--) {
        picker->sizes[k][0] = picker->sizes[k - 1][0];
        picker->sizes[k][1] = picker->sizes[k - 1][1];
    }
    picker->sizes[i][0] = w;
    picker->sizes[i][1] = h;
    picker->count++;
}

void display_picker_build(struct DisplayPicker *picker, int fullscreen, int nativeWidth, int nativeHeight,
                          int roomWidth, int roomHeight, int width, int height)
{
    size_t i;

    memset(picker, 0, sizeof *picker);
    picker->selected = -1;
    if (fullscreen) {
        picker->width = nativeWidth;
        picker->height = nativeHeight;
        return;
    }
    picker->width = width;
    picker->height = height;
    for (i = 0; i < sizeof common_sizes / sizeof common_sizes[0]; i++)
        if (common_sizes[i][0] <= roomWidth && common_sizes[i][1] <= roomHeight)
            offer(picker, common_sizes[i][0], common_sizes[i][1]);
    if (takes(width, height) && width <= roomWidth && height <= roomHeight)
        offer(picker, width, height);
    /* the largest window that fits, where the options can keep its size */
    if (takes(roomWidth, roomHeight))
        offer(picker, roomWidth, roomHeight);
    for (i = 0; i < (size_t)picker->count; i++)
        if (picker->sizes[i][0] == width && picker->sizes[i][1] == height)
            picker->selected = (int)i;
    picker->enabled = picker->count > 0;
}
