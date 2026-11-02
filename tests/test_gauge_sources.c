#include "ui/character/gauge.h"
#include "ui/character/gauge_sources.h"
#include "ui/window.h"
#include "ui/draw.h"
#include "ui/skin.h"
#include "session/state.h"
#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Session g_session1;
static void *allocation;
static size_t allocation_size;
static int allocations, destructions;
static int fills, clears, bitmaps, numbers;
static int fill_width[16];
static COLORREF fill_colour[16];
static long long shown_current, shown_maximum;
static int bitmap_colour;

void *mem_alloc(size_t size)
{
    assert(!allocation);
    allocation = malloc(size + 16);
    assert(allocation);
    allocation_size = size;
    memset(allocation, 0xa5, size + 16);
    allocations++;
    return allocation;
}

void mem_free(void *p)
{
    size_t size = allocation_size;
    size_t i;
    assert(p == allocation && size == allocation_size);
    for (i = 0; i < 16; ++i)
        assert(((unsigned char *)p)[size + i] == 0xa5);
    free(p);
    allocation = NULL;
    destructions++;
}

struct UIWindow *ui_window_construct(struct UIWindow *window, int composites)
{
    assert(!composites);
    memset(window, 0, sizeof(*window));
    window->width = 60;
    window->height = 13;
    return window;
}

void ui_window_destruct(struct UIWindow *window) { assert(window == allocation); }
void ui_window_clear_canvas(struct UIWindow *window, DWORD colour)
{
    (void)colour;
    assert(window == allocation);
    clears++;
}
void ui_window_fill_rect(struct UIWindow *window, int x, int y, int width, int height, COLORREF colour)
{
    (void)x; (void)y;
    assert(window == allocation && height == 3 && fills < 16);
    fill_width[fills] = width;
    fill_colour[fills++] = colour;
}

struct ResMgr *g_resMgr(void) { return (struct ResMgr *)&g_session1; }
const char *ui_skin_path(const char *path) { return path; }
void *res_mgr_get(struct ResMgr *manager, const char *name)
{
    static struct BitmapRes bitmap;
    assert(manager == g_resMgr());
    bitmap.width = 4;
    bitmap.height = 1;
    if (strstr(name, "gzered")) bitmap_colour = 1;
    if (strstr(name, "gzeblue")) bitmap_colour = 2;
    return &bitmap;
}
void ui_window_draw_bitmap(struct UIWindow *window, int x, int y, struct BitmapRes *bitmap, int keep_alpha)
{
    (void)x; (void)y; (void)keep_alpha;
    assert(window == allocation && bitmap);
    bitmaps++;
}
int crt_sprintf(char *out, const char *format, ...)
{
    int result;
    va_list args;
    va_start(args, format);
    result = vsprintf(out, format, args);
    va_end(args);
    return result;
}
void string_format(struct String *out, const char *format, ...)
{
    va_list args;
    assert(strcmp(format, "%3" PRId64 "  /  %3" PRId64) == 0 && !out->data);
    va_start(args, format);
    shown_current = va_arg(args, long long);
    shown_maximum = va_arg(args, long long);
    va_end(args);
}
void string_free(struct String *s) { assert(!s->data && !s->capacity); }
void ui_window_text_out_centred(struct UIWindow *window, int x, int y, const char *text,
    int width, int face, int height, COLORREF colour, int extra)
{
    (void)x; (void)y; (void)text; (void)width; (void)face; (void)height; (void)colour; (void)extra;
    assert(window == allocation);
    numbers++;
}
int session_get_param(struct Session *session, int type)
{
    assert(session == &g_session1 && (type == 0xe8 || type == 0xe9));
    return type == 0xe8 ? -27 : 68;
}

static long long read_wide(const void *context) { return *(const long long *)context; }
static void draw(struct UIWindow *window)
{
    fills = clears = bitmaps = numbers = bitmap_colour = 0;
    window->ops->on_draw(window);
}

static void test_values(void)
{
    int value = INT_MIN;
    long long wide = LLONG_MIN;
    struct GaugeValue source = gauge_value_from_int(&value), copy = source;
    struct GaugeValue custom = {read_wide, &wide};
    assert(gauge_value_read(&copy) == INT_MIN);
    value = INT_MAX;
    assert(gauge_value_read(&copy) == INT_MAX);
    assert(gauge_value_read(&custom) == LLONG_MIN);
    wide = LLONG_MAX;
    assert(gauge_value_read(&custom) == LLONG_MAX);
    source = gauge_ap_source(); copy = gauge_max_ap_source();
    assert(gauge_value_read(&source) == -27 && gauge_value_read(&copy) == 68);
    assert(allocations == 0);
}

static void test_player_gauge(void)
{
    int hp = 25, max_hp = 100, sp = 50, max_sp = 100, food = 20;
    long long wide = 1LL << 40, maximum = 1LL << 41;
    struct GaugeValue large = {read_wide, &wide}, large_max = {read_wide, &maximum};
    struct UIWindow *window = ui_player_gauge_create();
    int count = allocations, i;
    draw(window);
    assert(clears == 1 && fills == 0);
    ui_player_gauge_set(window, PLAYER_GAUGE_HP, gauge_value_from_int(&hp), gauge_value_from_int(&max_hp));
    ui_player_gauge_set(window, PLAYER_GAUGE_SP, gauge_value_from_int(&sp), gauge_value_from_int(&max_sp));
    draw(window);
    assert(fills == 4 && fill_width[1] == 14 && fill_colour[1] == 0x25ea16);
    hp = 24;
    draw(window);
    assert(fill_width[1] == 13 && fill_colour[1] == 0x1616ea);
    ui_player_gauge_set(window, PLAYER_GAUGE_FOOD, gauge_value_from_int(&food), gauge_value_from_int(&max_hp));
    ui_player_gauge_set_layout(window, 7);
    draw(window);
    assert(fills == 2 && fill_width[1] == 11);
    ui_player_gauge_set_layout(window, 8);
    draw(window);
    assert(fills == 6);
    ui_player_gauge_set_layout(window, 13);
    ui_player_gauge_set(window, PLAYER_GAUGE_HP, large, large_max);
    draw(window);
    assert(fills == 2 && fill_width[1] == 29);
    wide *= 2;
    draw(window);
    assert(fill_width[1] == 58);
    for (i = 0; i < 100; ++i)
        ui_player_gauge_set(window, PLAYER_GAUGE_HP, large, large_max);
    assert(allocations == count);
    ui_player_gauge_set(window, PLAYER_GAUGE_HP, (struct GaugeValue){0}, (struct GaugeValue){0});
    draw(window);
    assert(fills == 0);
    window->ops->destroy(window);
    assert(!allocation);
}

static void test_value_gauge(enum ValueGaugeStyle style)
{
    long long current = 1LL << 40, maximum = 1LL << 42;
    struct GaugeValue value = {read_wide, &current}, max = {read_wide, &maximum};
    struct UIWindow *window = ui_value_gauge_create(style);
    int count = allocations;
    draw(window);
    assert(bitmaps == 0 && numbers == 0);
    ui_value_gauge_set(window, value, max);
    draw(window);
    assert(bitmaps > 0 && bitmap_colour == 2 && numbers == 1);
    assert(shown_current == current && shown_maximum == maximum);
    current = maximum;
    draw(window);
    assert(numbers == 1 && bitmap_colour == (style == VALUE_GAUGE_AP ? 1 : 2));
    current = 1; maximum = 10;
    draw(window);
    assert(numbers == 1 && bitmap_colour == (style == VALUE_GAUGE_AP ? 2 : 1));
    current = 11;
    draw(window);
    assert(numbers == 0 && bitmaps == 0);
    current = maximum = 0;
    draw(window);
    assert(numbers == (style == VALUE_GAUGE_AP));
    assert(allocations == count);
    window->ops->destroy(window);
    assert(!allocation);
}

int main(void)
{
    test_values();
    test_player_gauge();
    test_value_gauge(VALUE_GAUGE_PLAYER);
    test_value_gauge(VALUE_GAUGE_AP);
    test_value_gauge(VALUE_GAUGE_HOMUN);
    assert(allocations == 4 && destructions == 4);
    puts("gauges: live borrowed values, signed/64-bit reads, drawing decisions and owned window cleanup passed");
    return 0;
}
