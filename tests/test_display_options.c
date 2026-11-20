/* The display options (render/display_options.c): what OptionInfo.lua files of older clients load
   as, what the client writes back, how the options follow the window, how a windowed size fits a
   display, window coordinates for pixels, and the graphics settings' resolution picker in either
   mode. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "render/display_options.h"

struct Option {
    const char *name;
    int value;
};

/* An OptionInfo.lua as the client reads it (session_get_option_int): the options it has, the
   fallback for one it has not or that is -1, and the names asked for. */
struct OptionFile {
    const struct Option *options;
    int count;
    const char *asked[16];
    int askedCount;
};

static int read_option(void *context, const char *name, int fallback)
{
    struct OptionFile *file = context;
    int i;
    assert(file->askedCount < 16);
    file->asked[file->askedCount++] = name;
    for (i = 0; i < file->count; i++)
        if (!strcmp(file->options[i].name, name))
            return file->options[i].value == -1 ? fallback : file->options[i].value;
    return fallback;
}

static void load(const struct Option *options, int count, struct DisplayOptions *out)
{
    static const char *const gone[] = { "ISBORDERLESS", "BITPERPIXEL", "DEVICECNT", "MODECNT" };
    struct OptionFile file = { options, count, { 0 }, 0 };
    int i;
    size_t k;
    memset(out, 0x55, sizeof *out);
    display_options_read(out, read_option, &file);
    assert(file.askedCount == 8);
    for (i = 0; i < file.askedCount; i++)
        for (k = 0; k < sizeof gone / sizeof gone[0]; k++)
            assert(strcmp(file.asked[i], gone[k]));
}

static void expect(const struct DisplayOptions *got, int fullscreen, int maximized, int width, int height,
                   int placedWidth, int placedHeight)
{
    assert(got->fullscreen == fullscreen && got->maximized == maximized);
    assert(got->width == width && got->height == height);
    assert(got->placedWidth == placedWidth && got->placedHeight == placedHeight);
}

static void expect_at(const struct DisplayOptions *got, int x, int y)
{
    assert(got->x == x && got->y == y);
}

#define LOAD(options, out) load(options, (int)(sizeof options / sizeof options[0]), out)

static void old_files(void)
{
    /* written by the client with exclusive fullscreen at 1360 by 768 */
    static const struct Option exclusive[] = {
        { "ISFULLSCREENMODE", 1 }, { "ISBORDERLESS", 0 }, { "WIDTH", 1360 }, { "HEIGHT", 768 },
        { "Window_XPos", 0 }, { "Window_YPos", 0 }, { "OLD_WIDTH", 1360 }, { "OLD_HEIGHT", 768 },
        { "BITPERPIXEL", 32 }, { "DEVICECNT", 1 }, { "MODECNT", 4 },
    };
    /* borderless fullscreen, with 1600 by 900 picked as the resolution */
    static const struct Option borderless[] = {
        { "ISFULLSCREENMODE", 1 }, { "ISBORDERLESS", 1 }, { "WIDTH", 1600 }, { "HEIGHT", 900 },
        { "OLD_WIDTH", 1600 }, { "OLD_HEIGHT", 900 }, { "BITPERPIXEL", 32 }, { "DEVICECNT", 0 },
        { "MODECNT", 11 },
    };
    /* the original client's file has no ISBORDERLESS, which was 1 when missing */
    static const struct Option original[] = {
        { "ISFULLSCREENMODE", 1 }, { "WIDTH", 1920 }, { "HEIGHT", 1080 }, { "BITPERPIXEL", 16 },
    };
    static const struct Option windowed[] = {
        { "ISFULLSCREENMODE", 0 }, { "ISBORDERLESS", 1 }, { "WIDTH", 1600 }, { "HEIGHT", 900 },
        { "Window_XPos", 1231 }, { "Window_YPos", 202 }, { "OLD_WIDTH", 1600 }, { "OLD_HEIGHT", 900 },
        { "BITPERPIXEL", 32 }, { "MODECNT", 11 },
    };
    static const struct Option nonzero[] = { { "ISFULLSCREENMODE", 2 } };
    static const struct Option none[] = { { "Trilinear", 1 } };
    static const struct Option small[] = { { "WIDTH", 800 }, { "HEIGHT", 600 }, { "OLD_WIDTH", 800 },
                                           { "OLD_HEIGHT", 600 } };
    static const struct Option largest[] = { { "WIDTH", 0x2000 }, { "HEIGHT", 0x1000 } };
    static const struct Option lowest[] = { { "WIDTH", 1024 }, { "HEIGHT", 0x298 } };
    /* this client's own: maximised, restored to 1440 by 900 at a place on a display left of the
       primary one; any nonzero value counts */
    static const struct Option maximized[] = {
        { "ISFULLSCREENMODE", 0 }, { "ISMAXIMIZED", 1 }, { "WIDTH", 1440 }, { "HEIGHT", 900 },
        { "Window_XPos", -1700 }, { "Window_YPos", 80 }, { "OLD_WIDTH", 1440 }, { "OLD_HEIGHT", 900 },
    };
    static const struct Option odd[] = { { "ISMAXIMIZED", -7 }, { "WIDTH", 1500 }, { "HEIGHT", 850 } };
    /* half a position, or -1, which the client's reader gives as missing: no position */
    static const struct Option half[] = { { "Window_XPos", 300 } };
    static const struct Option minus[] = { { "Window_XPos", -1 }, { "Window_YPos", 40 } };
    struct DisplayOptions got;

    LOAD(exclusive, &got);
    expect(&got, 1, 0, 1360, 768, 1360, 768);
    expect_at(&got, 0, 0);
    LOAD(borderless, &got);
    expect(&got, 1, 0, 1600, 900, 1600, 900);
    expect_at(&got, DISPLAY_NO_POSITION, DISPLAY_NO_POSITION);
    LOAD(original, &got);
    expect(&got, 1, 0, 1920, 1080, 0, 0);
    LOAD(windowed, &got);
    expect(&got, 0, 0, 1600, 900, 1600, 900);
    expect_at(&got, 1231, 202);
    LOAD(nonzero, &got);
    expect(&got, 1, 0, 1024, 768, 0, 0);
    LOAD(none, &got);
    expect(&got, 0, 0, 1024, 768, 0, 0);
    expect_at(&got, DISPLAY_NO_POSITION, DISPLAY_NO_POSITION);
    /* a height below 664 gave 1,024 by 768 before too; the saved position no longer matches */
    LOAD(small, &got);
    expect(&got, 0, 0, 1024, 768, 800, 600);
    LOAD(largest, &got);
    expect(&got, 0, 0, 0x2000, 0x1000, 0, 0);
    LOAD(lowest, &got);
    expect(&got, 0, 0, 1024, 0x298, 0, 0);
    LOAD(maximized, &got);
    expect(&got, 0, 1, 1440, 900, 1440, 900);
    expect_at(&got, -1700, 80);
    LOAD(odd, &got);
    expect(&got, 0, 1, 1500, 850, 0, 0);
    LOAD(half, &got);
    expect_at(&got, DISPLAY_NO_POSITION, DISPLAY_NO_POSITION);
    LOAD(minus, &got);
    expect_at(&got, DISPLAY_NO_POSITION, DISPLAY_NO_POSITION);
}

static void bad_sizes(void)
{
    static const int sizes[][2] = {
        { 0, 768 }, { -1024, 768 }, { 1, 768 }, { 1023, 768 }, { 0x2001, 768 }, { 1024, 0x297 },
        { 1024, 0x1001 }, { 1024, -1 }, { -1, -1 }, { 0x7fffffff, 0x7fffffff }, { (int)0x80000000, 768 },
    };
    size_t i;
    for (i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
        struct Option options[] = { { "ISFULLSCREENMODE", 0 }, { "WIDTH", sizes[i][0] }, { "HEIGHT", sizes[i][1] } };
        struct DisplayOptions got;
        LOAD(options, &got);
        expect(&got, 0, 0, 1024, 768, 0, 0);
    }
}

struct Written {
    char names[10][24];
    int values[10];
    int count;
};

static void write_option(void *context, const char *name, int value)
{
    struct Written *out = context;
    assert(out->count < 10 && strlen(name) < sizeof out->names[0]);
    strcpy(out->names[out->count], name);
    out->values[out->count++] = value;
}

static void written(void)
{
    static const char *const names[] = { "ISFULLSCREENMODE", "ISMAXIMIZED", "WIDTH", "HEIGHT", "Window_XPos",
                                         "Window_YPos", "OLD_WIDTH", "OLD_HEIGHT" };
    static const struct DisplayOptions saved[] = {
        { 1, 0, 1600, 900, 0, 0, 1600, 900 }, { 0, 0, 1360, 768, 1231, 202, 1360, 768 },
        { 0, 1, 1024, 768, -1700, 80, 1024, 768 }, { 1, 1, 1920, 1200, 5120, 0, 1920, 1200 },
    };
    struct DisplayOptions flags = { 5, -2, 1280, 720, 10, 10, 1280, 720 };
    struct DisplayOptions nowhere = { 0, 0, 1280, 720, DISPLAY_NO_POSITION, DISPLAY_NO_POSITION, 1280, 720 };
    struct Written out;
    struct Option options[8];
    struct DisplayOptions back;
    size_t i;
    int k;

    for (i = 0; i < sizeof saved / sizeof saved[0]; i++) {
        memset(&out, 0, sizeof out);
        display_options_write(&saved[i], write_option, &out);
        /* nothing but these eight: ISBORDERLESS, BITPERPIXEL, DEVICECNT and MODECNT are gone */
        assert(out.count == 8);
        for (k = 0; k < 8; k++) {
            assert(!strcmp(out.names[k], names[k]));
            options[k].name = out.names[k];
            options[k].value = out.values[k];
        }
        LOAD(options, &back);
        assert(!memcmp(&back, &saved[i], sizeof back));
    }
    memset(&out, 0, sizeof out);
    display_options_write(&flags, write_option, &out);
    assert(out.count == 8 && out.values[0] == 1 && out.values[1] == 1);
    /* no position known: none written, so none is read back */
    memset(&out, 0, sizeof out);
    display_options_write(&nowhere, write_option, &out);
    assert(out.count == 6);
    for (k = 0; k < 6; k++)
        assert(strcmp(out.names[k], "Window_XPos") && strcmp(out.names[k], "Window_YPos"));
}

static void tracked(void)
{
    struct DisplayOptions o = { 0, 0, 1600, 900, 100, 50, 1600, 900 };

    /* windowed, dragged and moved: the client size is the windowed size, the frame's corner the
       position */
    display_options_track(&o, 0, 0, 0, 1700, 950, 120, 60);
    expect(&o, 0, 0, 1700, 950, 1600, 900);
    expect_at(&o, 120, 60);
    /* maximised: the windowed size and the position stay the ones it is restored to */
    display_options_track(&o, 0, 1, 0, 1920, 1009, -8, -8);
    expect(&o, 0, 1, 1700, 950, 1600, 900);
    expect_at(&o, 120, 60);
    /* minimised from maximised: nothing changes, so it comes back maximised */
    display_options_track(&o, 0, 0, 1, 0, 0, -32000, -32000);
    expect(&o, 0, 1, 1700, 950, 1600, 900);
    expect_at(&o, 120, 60);
    /* fullscreen: the maximised state, the windowed size and the position are kept for coming out of
       it */
    display_options_track(&o, 1, 0, 0, 2560, 1440, 1920, 0);
    expect(&o, 1, 1, 1700, 950, 1600, 900);
    expect_at(&o, 120, 60);
    /* the system takes the window out of fullscreen (macOS, a window manager): it stays out */
    display_options_track(&o, 0, 1, 0, 1920, 1009, -8, -8);
    expect(&o, 0, 1, 1700, 950, 1600, 900);
    /* and back into it, which the options follow too */
    display_options_track(&o, 1, 0, 0, 2560, 1440, 1920, 0);
    expect(&o, 1, 1, 1700, 950, 1600, 900);
    /* restored: back to the windowed size and its place */
    display_options_track(&o, 0, 0, 0, 1700, 950, 120, 60);
    expect(&o, 0, 0, 1700, 950, 1600, 900);
    expect_at(&o, 120, 60);
    /* a size the options do not keep is not kept; the place is */
    display_options_track(&o, 0, 0, 0, 1000, 600, 300, 200);
    expect(&o, 0, 0, 1700, 950, 1600, 900);
    expect_at(&o, 300, 200);
    display_options_track(&o, 0, 0, 0, 9000, 900, 300, 200);
    expect(&o, 0, 0, 1700, 950, 1600, 900);
    display_options_track(&o, 0, 0, 0, DISPLAY_MIN_WIDTH, DISPLAY_MIN_HEIGHT, 300, 200);
    expect(&o, 0, 0, DISPLAY_MIN_WIDTH, DISPLAY_MIN_HEIGHT, 1600, 900);
}

static void fitted(void)
{
    int w, h;

    /* within the room: as it is */
    w = 1600, h = 900;
    display_fit(&w, &h, 1904, 1001);
    assert(w == 1600 && h == 900);
    /* a former exclusive mode at the display's size opens at the room */
    w = 1920, h = 1080;
    display_fit(&w, &h, 1904, 1001);
    assert(w == 1904 && h == 1001);
    /* only the side that does not fit */
    w = 2560, h = 900;
    display_fit(&w, &h, 1904, 1001);
    assert(w == 1904 && h == 900);
    /* a room smaller than the smallest windowed size gives that size */
    w = 1600, h = 900;
    display_fit(&w, &h, 1008, 641);
    assert(w == DISPLAY_MIN_WIDTH && h == DISPLAY_MIN_HEIGHT);
    w = 1366, h = 768;
    display_fit(&w, &h, 1350, 689);
    assert(w == 1350 && h == 689);
}

static void units(void)
{
    static const float densities[] = { 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 2.25f, 3.0f };
    size_t i;

    /* the minimum, rounded up, holds at least the minimum's pixels at every density */
    for (i = 0; i < sizeof densities / sizeof densities[0]; i++) {
        float d = densities[i];
        int w = display_units(DISPLAY_MIN_WIDTH, d, 1), h = display_units(DISPLAY_MIN_HEIGHT, d, 1);
        assert((int)(w * d) >= DISPLAY_MIN_WIDTH && (int)(h * d) >= DISPLAY_MIN_HEIGHT);
        /* and is the fewest window coordinates that do */
        assert((int)((w - 1) * d) < DISPLAY_MIN_WIDTH && (int)((h - 1) * d) < DISPLAY_MIN_HEIGHT);
    }
    /* at 125% the nearest would be 819 by 531, 1023 by 663 pixels, a size the options refuse */
    assert(display_units(1024, 1.25f, 0) == 819 && display_units(664, 1.25f, 0) == 531);
    assert(display_units(1024, 1.25f, 1) == 820 && display_units(664, 1.25f, 1) == 532);
    assert(display_units(1024, 2.0f, 1) == 512 && display_units(664, 2.0f, 1) == 332);
    assert(display_units(1024, 1.5f, 1) == 683 && display_units(664, 1.5f, 1) == 443);
    /* to the nearest */
    assert(display_units(1600, 1.0f, 0) == 1600 && display_units(1600, 2.0f, 0) == 800);
    assert(display_units(1601, 2.0f, 0) == 801 && display_units(1600, 1.5f, 0) == 1067);
    /* no density known: pixels */
    assert(display_units(1600, 0.0f, 0) == 1600 && display_units(1600, -1.0f, 1) == 1600);
}

static int listed(const struct DisplayPicker *picker, int w, int h)
{
    int i;
    for (i = 0; i < picker->count; i++)
        if (picker->sizes[i][0] == w && picker->sizes[i][1] == h)
            return i;
    return -1;
}

static void ordered_and_fitting(const struct DisplayPicker *picker, int roomWidth, int roomHeight)
{
    int i;
    assert(picker->count <= DISPLAY_SIZES_MAX);
    for (i = 0; i < picker->count; i++) {
        assert(picker->sizes[i][0] <= roomWidth && picker->sizes[i][1] <= roomHeight);
        if (i)
            assert(picker->sizes[i - 1][0] < picker->sizes[i][0]
                   || (picker->sizes[i - 1][0] == picker->sizes[i][0] && picker->sizes[i - 1][1] < picker->sizes[i][1]));
    }
}

static void picker(void)
{
    /* 1920 by 1080 with a 40-pixel taskbar, and a frame 16 wide and 39 high: room for 1904 by 1001 */
    static const int fitting[][2] = {
        { 1024, 768 }, { 1152, 864 }, { 1280, 720 }, { 1280, 768 }, { 1280, 800 }, { 1280, 960 },
        { 1360, 768 }, { 1366, 768 }, { 1440, 900 }, { 1600, 900 }, { 1904, 1001 },
    };
    struct DisplayPicker p;
    int i;

    /* fullscreen: disabled, at the display's own size, whatever the windowed size */
    display_picker_build(&p, 1, 5120, 1440, 5104, 1361, 1600, 900);
    assert(!p.enabled && p.width == 5120 && p.height == 1440 && p.count == 0 && p.selected == -1);
    display_picker_build(&p, 1, 1920, 1080, 0, 0, 0, 0);
    assert(!p.enabled && p.width == 1920 && p.height == 1080 && p.count == 0 && p.selected == -1);

    /* windowed: the common sizes that fit and the room itself, the windowed size chosen among them */
    display_picker_build(&p, 0, 1920, 1080, 1904, 1001, 1600, 900);
    assert(p.enabled && p.width == 1600 && p.height == 900);
    assert(p.count == (int)(sizeof fitting / sizeof fitting[0]));
    for (i = 0; i < p.count; i++)
        assert(p.sizes[i][0] == fitting[i][0] && p.sizes[i][1] == fitting[i][1]);
    assert(p.selected == 9);
    assert(listed(&p, 1280, 1024) < 0 && listed(&p, 1680, 1050) < 0 && listed(&p, 1920, 1080) < 0);

    /* a windowed size of its own that fits is offered in its place */
    display_picker_build(&p, 0, 1920, 1080, 1904, 1001, 1500, 850);
    assert(p.enabled && p.count == 12 && p.selected == 9 && listed(&p, 1500, 850) == 9);
    assert(listed(&p, 1440, 900) == 8 && listed(&p, 1600, 900) == 10 && listed(&p, 1904, 1001) == 11);
    ordered_and_fitting(&p, 1904, 1001);
    display_picker_build(&p, 0, 1920, 1080, 1904, 1001, 1904, 1001);
    assert(p.count == 11 && p.selected == 10);
    ordered_and_fitting(&p, 1904, 1001);
    display_picker_build(&p, 0, 1920, 1080, 1904, 1001, 1024, 700);
    assert(p.count == 12 && p.selected == 0);
    ordered_and_fitting(&p, 1904, 1001);

    /* one that does not fit (a former exclusive mode at the display's size) is shown, not offered */
    display_picker_build(&p, 0, 1920, 1080, 1904, 1001, 1920, 1080);
    assert(p.enabled && p.width == 1920 && p.height == 1080 && p.count == 11 && p.selected == -1);
    ordered_and_fitting(&p, 1904, 1001);

    /* a wide display: up to the sizes its height still holds, and the room */
    display_picker_build(&p, 0, 5120, 1440, 5104, 1361, 2560, 1080);
    ordered_and_fitting(&p, 5104, 1361);
    assert(listed(&p, 2048, 1152) >= 0 && listed(&p, 1920, 1200) >= 0 && listed(&p, 2560, 1080) == p.selected);
    assert(listed(&p, 2560, 1440) < 0 && listed(&p, 3440, 1440) < 0 && listed(&p, 5120, 1440) < 0);
    assert(p.count == 19 && listed(&p, 5104, 1361) == 18);

    /* laptops with a 40-pixel taskbar, where none of the common sizes fits: the room alone, 1366 by
       768 leaving 1350 by 689 and 1280 by 800 leaving 1264 by 721 */
    display_picker_build(&p, 0, 1366, 768, 1350, 689, 1024, 768);
    assert(p.enabled && p.count == 1 && listed(&p, 1350, 689) == 0 && p.selected == -1);
    assert(p.width == 1024 && p.height == 768);
    display_picker_build(&p, 0, 1280, 800, 1264, 721, 1264, 721);
    assert(p.enabled && p.count == 1 && p.selected == 0);
    display_picker_build(&p, 0, 1366, 768, 1350, 689, 1100, 680);
    assert(p.enabled && p.count == 2 && p.selected == 0 && listed(&p, 1350, 689) == 1);
    /* a room narrower than the options keep (a 1024 by 768 display) offers nothing either */
    display_picker_build(&p, 0, 1024, 768, 1008, 689, 1024, 768);
    assert(!p.enabled && p.count == 0);

    /* the current value: a maximised window's size, larger than the room a framed window has, is
       shown but not offered; a size dragged to that fits is offered and chosen */
    display_picker_build(&p, 0, 1920, 1080, 1904, 1001, 1920, 1009);
    assert(p.enabled && p.width == 1920 && p.height == 1009 && p.selected == -1 && listed(&p, 1920, 1009) < 0);
    display_picker_build(&p, 0, 1920, 1080, 1904, 1001, 1733, 977);
    assert(p.enabled && p.width == 1733 && p.height == 977 && p.selected == listed(&p, 1733, 977) && p.selected > 0);
    ordered_and_fitting(&p, 1904, 1001);

    /* a room lower than the options keep (1280 by 720 with a taskbar): nothing to offer, disabled
       at the windowed size */
    display_picker_build(&p, 0, 1280, 720, 1264, 641, 1024, 768);
    assert(!p.enabled && p.count == 0 && p.selected == -1 && p.width == 1024 && p.height == 768);
    display_picker_build(&p, 0, 0, 0, 0, 0, 1024, 768);
    assert(!p.enabled && p.count == 0);

    /* every common size, and its own beyond them, which is the room, on a room large enough */
    display_picker_build(&p, 0, 0x2000, 0x1000, 0x2000, 0x1000, 0x2000, 0x1000);
    ordered_and_fitting(&p, 0x2000, 0x1000);
    assert(p.count == 27 && p.selected == 26);
}

int main(void)
{
    old_files();
    bad_sizes();
    written();
    tracked();
    fitted();
    units();
    picker();
    puts("Display options: older exclusive, borderless and windowed files, bad sizes, what is written, and how they follow the window, the fit to a display, window coordinates at several densities, and the picker in fullscreen, windowed and on small displays passed.");
    return 0;
}
