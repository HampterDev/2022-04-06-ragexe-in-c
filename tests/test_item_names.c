#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "iteminfo/iteminfo.h"
#include "ui/window_mgr.h"
#include "owned_storage_test_allocator.h"

static unsigned legacy_allocations;
void *mem_alloc(size_t size) { void *p = malloc(size); assert(p); ++legacy_allocations; return p; }
void mem_free(void *p)
{  if (p) { assert(legacy_allocations); --legacy_allocations; free(p); } }
__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }

#include "../src/iteminfo/item_name.c"
#include "../src/ui/items/item_name_runs.c"

unsigned g_codePage = 949;
unsigned char g_useCharsetTag;
struct UIWindowMgr g_windowMgr;
static struct ItemTableEntry table;
static struct CardAffix cards[5];
static const char *maker = "장인", *pet_prefix = "사랑받는 ";
static unsigned requested_id, waits;
const struct ItemTableEntry *item_table_get(int id) { (void)id; return &table; }
const struct CardAffix *card_affix_get(int id) { return &cards[(unsigned)id < 5 ? id : 0]; }
unsigned char card_affix_has(int id) { return id > 0 && id < 5; }
struct TextView player_name_lookup(unsigned id) { requested_id = id; return text_view_cstr(maker); }
char *msg_str(int id)
{
    switch (id) {
    case 0x245: return "이름 대기";
    case 0x2f4: return (char *)pet_prefix;
    case 0x1be: return "Double";
    case 0x1bf: return "Triple";
    case 0x1c0: return "Quadruple";
    case 0xba0: return "Special ";
    case 0x1c2: return "Plain ";
    case 0x1c4: return "Water ";
    case 0x1c6: return "Earth ";
    case 0x1c3: return "Fire ";
    case 0x1c5: return "Wind ";
    case 0x1cb: return "Star ";
    case 0x1cc: return "DoubleStar ";
    case 0x1cd: return "TripleStar ";
    default: assert(0); return "";
    }
}
unsigned char item_info_is_equipment_type(const struct ItemInfo *item)
{
    switch (item->type) {
    case 4: case 5: case 8: case 9: case 11: case 12: case 13: case 14: case 15: case 21: return 1;
    default: return 0;
    }
}
unsigned char item_info_has_maker_slot(const struct ItemInfo *item)
{ return item->slot[0] == ITEM_SLOT_NAMED || item->slot[0] == ITEM_SLOT_FORGED; }
unsigned char item_info_is_pet_range_id(const struct ItemInfo *item)
{ unsigned id = (unsigned)item_id(item); return id - 9001 <= 0x1f2 || id - 10000 <= 999; }
void ui_window_mgr_add_to_name_waiting_list(struct UIWindowMgr *manager, struct UIWindow *window)
{ assert(manager == &g_windowMgr && window); ++waits; }

struct DrawCall { int x, y, length; unsigned long colour, shadow; unsigned hash; };
static struct DrawCall drawn[4096];
static size_t draw_count;
int ui_window_text_width(struct UIWindow *window, const char *text, int length,
                       int face, int height, int arg3, int arg4)
{
    (void)window; (void)face; (void)height; (void)arg3; (void)arg4;
    return 5 * (length ? length : (int)strlen(text));
}
void ui_window_draw_shadow_text(struct UIWindow *window, int x, int y, const char *text,
                             int length, COLORREF colour, COLORREF shadow, int face,
                             int height, int font_arg)
{
    struct DrawCall *call;
    unsigned hash = 2166136261u;
    int i;
    (void)window; (void)face; (void)height; (void)font_arg;
    assert(length >= 0 && draw_count < sizeof drawn / sizeof drawn[0]);
    for (i = 0; i < length; ++i) hash = (hash ^ (unsigned char)text[i]) * 16777619u;
    call = &drawn[draw_count++];
    memset(call, 0, sizeof *call);
    call->x = x; call->y = y; call->length = length;
    call->colour = colour; call->shadow = shadow; call->hash = hash;
}

#ifdef ITEM_NAMES_REFERENCE
/* The archived references still read the service type; the client is fixed at langtype 20. */
int g_serviceType = 20;
void string_vector_reserve(struct Vec *v, size_t count) { (void)v; (void)count; abort(); }
int crt_sprintf(char *out, const char *format, ...)
{
    int size;
    va_list args;
    va_start(args, format); size = vsprintf(out, format, args); va_end(args);
    return size;
}
#include "item_names_reference.c"
#include "item_runs_reference.c"
#endif

static void init_item(struct ItemInfo *item, int id)
{
    char number[32];
    memset(item, 0, sizeof *item);
    string_init(&item->name);
    string_init(&item->id_text);
    snprintf(number, sizeof number, "%d", id);
    string_assign_cstr(&item->id_text, number);
    item->identified = 1;
    table.name = "검"; table.unidName = "미확인"; table.slotCount = 3;
    cards[1].prefix = "칼날"; cards[1].postfix = "힘";
    cards[2].prefix = ""; cards[2].postfix = "물";
    cards[3].prefix = ""; cards[3].postfix = "";
    cards[4].prefix = ""; cards[4].postfix = "";
    maker = "장인"; pet_prefix = "사랑받는 ";
    g_useCharsetTag = 0;
}
static void equals(const struct String *text, const char *expected)
{ assert(text->size == strlen(expected) && !strcmp(string_cstr(text), expected)); }

static void native_cases(void)
{
    struct ItemInfo item;
    struct String text = {0}, other = {0};
    struct ItemNameRuns runs = {0};
    struct UIWindow window = {0};
    char *long_name = malloc(24001), *old;
    size_t end, marked;
    unsigned i;

    init_item(&item, 501);
    assert(item_info_get_name(&item) == table.name);
    item_info_get_display_name(&item, &text); equals(&text, "검");
    item.identified = 0; item_info_get_display_name(&item, &text); equals(&text, "미확인");
    table.unidName = NULL; item_info_get_display_name(&item, &text); equals(&text, " ");
    item.identified = 1; item.type = 4; item.refine = 7;
    item.slot[0] = item.slot[1] = 1; item.slot[2] = 2; item.slot[3] = 3;
    item_info_get_display_name(&item, &text); equals(&text, "+7 Double 칼날 검 힘 Double 물 [3]");
    string_assign(&other, string_cstr(&text), text.size);
    string_format(&text, "%s/%s/%d", string_cstr(&text), string_cstr(&text) + 3, 12);
    assert(strstr(string_cstr(&text), "/Double") && strstr(string_cstr(&text), "/12"));
    equals(&other, "+7 Double 칼날 검 힘 Double 물 [3]");
    string_assign_cstr(&text, "%s:%d");
    string_format(&text, string_cstr(&text), "한국", 42); equals(&text, "한국:42");
    string_append_format(&text, "/%s", string_cstr(&text)); equals(&text, "한국:42/한국:42");
    string_format(&text, ""); equals(&text, "");

    for (i = 0; i < 12000; ++i) memcpy(long_name + i * 2, "한", 2);
    long_name[24000] = 0; table.name = long_name; cards[1].prefix = long_name;
    item_info_get_display_name(&item, &text); assert(text.size > 48000);
    item.slot[0] = ITEM_SLOT_FORGED; item.slot[1] = 5 << 8 | 3;
    item.slot[2] = 0x1234; item.slot[3] = 0x5678;
    item_info_name_runs(&window, &item, &runs, 1, 0);
    assert(requested_id == 0x56781234 && runs.count == 6);
    assert(!memcmp(runs.text.data + runs.marked, maker, strlen(maker)));
    end = runs.ends[runs.count - 1]; marked = runs.marked; old = runs.text.data;
    string_append_format(&runs.text, "%s", long_name);
    string_append_format(&runs.text, "%s", long_name);
    item_name_runs_end(&runs);
    assert(runs.text.data != old && runs.marked == marked && runs.ends[runs.count - 2] == end);
    assert(runs.ends[runs.count - 1] == runs.text.size);
    for (i = 0; i < 200; ++i) {
        item_info_name_runs(&window, &item, &runs, 1, 0);
        assert(runs.count == 6 && runs.ends[5] == runs.text.size);
    }
    table.name = "검"; item.type = 0; item.slot[0] = 0; item.refine = 0;
    item_info_name_runs(&window, &item, &runs, 0, 0);
    assert(runs.count == 1 && runs.marked == (size_t)-1);
    pet_prefix = "%s%%끝";
    string_clear(&text); item_name_append_pet_prefix(&item, &text); equals(&text, "%s%끝");
    table.name = "|"; g_useCharsetTag = 1;
    item_info_name_runs(&window, &item, &runs, 0, 0);
    draw_count = 0; ui_window_draw_runs_shortened(&window, 3, 4, 1, &runs, 0, 12); assert(draw_count);
    table.name = "\x81";
    item_info_name_runs(&window, &item, &runs, 0, 0);
    draw_count = 0; ui_window_draw_runs_shortened(&window, 3, 4, 1, &runs, 0, 12); assert(draw_count);
    string_free(&runs.text); string_free(&text); string_free(&other);
    item_info_destruct(&item); free(long_name); assert(!outstanding);
}

#ifdef ITEM_NAMES_REFERENCE
static void archived_cases(void)
{
    static const int ids[] = {501, 9001, 9499, 10000, 10999, 11000, 2634};
    struct UIWindow window = {0};
    unsigned i;
    for (i = 0; i < 6000; ++i) {
        struct ItemInfo item;
        struct String text = {0};
        struct String reference;
        struct ItemNameRuns runs = {0};
        const char *end_storage[8];
        struct Vec ends = {end_storage, end_storage, end_storage + 8};
        char old_text[4096] = "", *cursor = old_text, *mark = NULL;
        COLORREF colour = 0;
        size_t n, count;
        struct DrawCall expected[4096];
        unsigned wanted, old_waits, new_waits;
        int width = (i % 4) == 0 ? 2000 : (i % 4) == 1 ? 30 : (i % 4) == 2 ? 90 : 160;
        init_item(&item, ids[i % 7]);
        item.type = i % 23;
        item.identified = i % 11 == 0 ? 0 : i % 13 == 0 ? 2 : 1;
        item.refine = i % 3 == 0 ? -1 : i % 3 == 1 ? 0 : 7;
        item.slot[0] = i % 5 == 0 ? ITEM_SLOT_NAMED : i % 5 == 1 ? ITEM_SLOT_FORGED : (int)(i % 5);
        item.slot[1] = i % 5 == 0 ? 0 : i % 5 == 1 ? (int)(((i % 4) * 5) << 8 | (i % 6)) : (int)(i % 4);
        item.slot[2] = i % 4; item.slot[3] = i % 3;
        if (i % 17 == 0) table.name = "";
        if (i % 19 == 0) table.unidName = NULL;
        if (i % 23 == 0) maker = "";
        if (i % 29 == 0) pet_prefix = "%%pet ";
        table.slotCount = i % 5;
        reference_ItemInfo_GetDisplayName(&item, &reference);
        item_info_get_display_name(&item, &text);
        assert(text.size == reference.size && !memcmp(string_cstr(&text), string_cstr(&reference), text.size));
        string_free(&reference);
        waits = 0;
        reference_ItemInfo_NameRuns(&window, &item, &colour, &ends, &cursor, &mark, i % 2, (i / 2) % 2);
        old_waits = waits; wanted = requested_id; waits = 0;
        item_info_name_runs(&window, &item, &runs, i % 2, (i / 2) % 2);
        new_waits = waits;
        assert(!strcmp(old_text, string_cstr(&runs.text)) && old_waits == new_waits && wanted == requested_id);
        count = (const char **)ends.last - (const char **)ends.first;
        assert(count == runs.count && colour == runs.mark_colour);
        assert(runs.marked == (mark ? (size_t)(mark - old_text) : (size_t)-1));
        for (n = 0; n < count; ++n) assert(runs.ends[n] == (size_t)(((const char **)ends.first)[n] - old_text));
        draw_count = 0;
        reference_UIWindow_DrawRunsShortened(&window, 7, 13, width, &ends, old_text, mark, colour, 0x123456, 0, 12);
        count = draw_count; memcpy(expected, drawn, count * sizeof *drawn);
        draw_count = 0;
        ui_window_draw_runs_shortened(&window, 7, 13, width, &runs, 0x123456, 12);
        assert(count == draw_count && !memcmp(expected, drawn, count * sizeof *drawn));
        string_free(&runs.text); string_free(&text);
        item_info_destruct(&item); assert(!outstanding && !legacy_allocations);
    }
    puts("Item-name reference: 6000 display names, colored runs and renderer call sequences match archived behavior.");
}
#endif

int main(void)
{
    native_cases();
#ifdef ITEM_NAMES_REFERENCE
    archived_cases();
#endif
    assert(!outstanding && !legacy_allocations);
    puts("Item names: borrowed queries, Korean text, long names/affixes, format aliasing, stable run offsets, suffix growth, malformed bytes and repeated cleanup passed");
    return 0;
}
