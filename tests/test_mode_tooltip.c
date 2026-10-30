#include "world/tooltip.h"
#include "world/mode_types.h"
#include "ui/chat/balloon.h"
#include "ui/window.h"
#include "ui/window_access.h"
#include "ui/window_manager_state.h"
#include "render/renderer.h"
#include <mmsystem.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
struct UIWindowMgr g_windowMgr;
static struct Mode mode;
static DWORD now;
static char events[64];
static unsigned event_count, allocation_count, queued_count;
static struct UITransBalloonText *blocks[8];
static struct UIWindow *queued[8], *replace_during_text;
static const char *source_text;
static BYTE expected_flag;
static int fitted_width, fitted_height;

static void event(char code)
{
    assert(event_count + 1 < sizeof events);
    events[event_count++] = code;
    events[event_count] = 0;
}

static void expect_events(const char *expected)
{
    assert(strcmp(events, expected) == 0);
    events[event_count = 0] = 0;
}

static DWORD test_time(void)
{
    event('T');
    return now;
}

#define timeGetTime test_time
#include "../src/world/tooltip.c"
#undef timeGetTime

void *mem_alloc(size_t bytes)
{
    struct UITransBalloonText *p;
    assert(bytes == sizeof *p && allocation_count < 8);
    p = malloc(bytes + 16);
    assert(p);
    memset(p, 0xa5, bytes + 16);
    blocks[allocation_count++] = p;
    event('B');
    return p;
}

char *session_remove_underbar(const char *source, char *destination)
{
    assert(strcmp(source, source_text) == 0);
    strcpy(destination, "normalized tooltip");
    event('N');
    return destination;
}

static void move(struct UIWindow *window, int x, int y)
{
    assert(window == mode.help_balloon);
    window->x = x;
    window->y = y;
    event('M');
}

static struct UIWindowOps methods = {.move = move};

struct UIWindow *ui_trans_balloon_text_construct(struct UIWindow *window)
{
    assert(window);
    memset(window, 0, sizeof(struct UITransBalloonText));
    window->ops = &methods;
    event('C');
    return window;
}

void ui_window_create(struct UIWindow *window, int width, int height)
{
    assert(window == mode.help_balloon && width == 8 && height == 8);
    window->width = width;
    window->height = height;
    event('I');
}

void ui_window_mgr_add_window(struct UIWindowMgr *manager, struct UIWindow *window)
{
    assert(manager == &g_windowMgr && window == mode.help_balloon);
    event('A');
}

void ui_window_mgr_raise_window(struct UIWindowMgr *manager, struct UIWindow *window)
{
    assert(manager == &g_windowMgr && window == mode.help_balloon);
    event('R');
}

void ui_window_mgr_post_quit2(struct UIWindowMgr *manager, struct UIWindow *window)
{
    assert(manager == &g_windowMgr && window && window == mode.help_balloon);
    assert(queued_count < 8);
    queued[queued_count++] = window;
    event('Q');
}

void ui_balloon_text_set_text(struct UIBalloonText *balloon, const char *text, int width)
{
    assert(&balloon->base == mode.help_balloon && width == 0x23);
    assert(balloon->colour_codes == expected_flag);
    assert(strcmp(text, "normalized tooltip") == 0);
    balloon->base.width = fitted_width;
    balloon->base.height = fitted_height;
    event('S');
    if (replace_during_text) {
        mode.help_balloon = replace_during_text;
        replace_during_text = NULL;
    }
}

static void show(const char *text, int x, int y, DWORD colour, BYTE flag)
{
    source_text = text;
    expected_flag = flag;
    mode_show_tooltip(&mode, text, x, y, colour, flag);
}

static void release_blocks(void)
{
    unsigned i, j;
    for (i = 0; i < allocation_count; ++i) {
        const unsigned char *guard = (const unsigned char *)(blocks[i] + 1);
        for (j = 0; j < 16; ++j) assert(guard[j] == 0xa5);
        free(blocks[i]);
    }
    allocation_count = 0;
}

int main(void)
{
    struct UITransBalloonText *first, *replacement;
    unsigned allocated;
    renderer.width = 640;
    renderer.height = 480;
    fitted_width = 120;
    fitted_height = 24;
    now = 100;
    mode.help_balloon_tick = 17;

    show("", 4, 5, 0, 0);
    mode_process_help_balloon(&mode);
    mode_hide_help_balloon(&mode);
    expect_events("");
    assert(!mode.help_balloon && !allocation_count && mode.help_balloon_tick == 17);

    show("한국_tooltip", -100, -200, 0xaa112233, 0x81);
    expect_events("NTBCIASM");
    assert(allocation_count == 1 && mode.help_balloon_tick == now);
    first = (struct UITransBalloonText *)mode.help_balloon;
    assert(first->balloon.base.x == -3 && first->balloon.base.y == -3);
    assert(first->balloon.colour == 0x00332211 && first->balloon.outline == 0);
    assert(first->backdrop.argb == 0x88000000);

    now = 200;
    show("reuse", 1000, 1000, 0xff010203, 0);
    expect_events("NTRSM");
    assert(allocation_count == 1 && mode.help_balloon == &first->balloon.base);
    assert(first->balloon.base.x == 523 && first->balloon.base.y == 459);
    assert(first->balloon.colour == 0x00030201 && mode.help_balloon_tick == 200);
    show("", 0, 0, 0, 1);
    expect_events("");
    assert(mode.help_balloon_tick == 200 && first->balloon.colour == 0x00030201);

    now = 299;
    mode_process_help_balloon(&mode);
    expect_events("T");
    assert(mode.help_balloon && !queued_count);
    now = 300;
    mode_process_help_balloon(&mode);
    expect_events("TQ");
    assert(!mode.help_balloon && queued_count == 1 && queued[0] == &first->balloon.base);
    assert(mode.help_balloon_tick == 200);
    mode_process_help_balloon(&mode);
    expect_events("");

    now = 400;
    show("hide", 12, 34, 0xffffff, 1);
    expect_events("NTBCIASM");
    assert(mode.help_balloon->x == 12 && mode.help_balloon->y == 34);
    first = (struct UITransBalloonText *)mode.help_balloon;
    now = 450;
    mode_hide_help_balloon(&mode);
    expect_events("QT");
    assert(!mode.help_balloon && mode.help_balloon_tick == 450);
    assert(queued_count == 2 && queued[1] == &first->balloon.base);
    mode_hide_help_balloon(&mode);
    expect_events("");

    now = 500;
    show("callback", 0, 0, 0, 0);
    expect_events("NTBCIASM");
    first = (struct UITransBalloonText *)mode.help_balloon;
    replacement = mem_alloc(sizeof *replacement);
    ui_trans_balloon_text_construct(&replacement->balloon.base);
    expect_events("BC");
    replacement->balloon.base.width = 660;
    replacement->balloon.base.height = 500;
    first->balloon.colour = 0x010101;
    first->balloon.outline = 0x020202;
    first->backdrop.argb = 0x03030303;
    replace_during_text = &replacement->balloon.base;
    allocated = allocation_count;
    show("replacement", 1, 2, 0x123456, 1);
    expect_events("NTRSM");
    assert(allocation_count == allocated && mode.help_balloon == &replacement->balloon.base);
    assert(replacement->balloon.base.x == -17 && replacement->balloon.base.y == -17);
    assert(replacement->balloon.colour == 0x563412 && replacement->balloon.outline == 0);
    assert(replacement->backdrop.argb == 0x88000000);
    assert(first->balloon.colour == 0x010101 && first->balloon.outline == 0x020202);
    assert(first->backdrop.argb == 0x03030303);
    mode_hide_help_balloon(&mode);
    expect_events("QT");
    assert(queued[queued_count - 1] == &replacement->balloon.base);
    release_blocks();
    puts("Mode tooltips: creation/reuse, text flags, colour/outline, bounds, deferred close, exact expiry, callback replacement and guarded storage passed");
    return 0;
}
