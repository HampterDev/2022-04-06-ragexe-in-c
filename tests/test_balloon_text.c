#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/chat/balloon.h"
#include "ui/draw.h"
#include "iteminfo/item_api.h"

static void *allocations[1024];
static unsigned live;
static void *tracked_realloc(void *old, size_t size)
{
    unsigned i;
    void *p;
    for (i = 0; i < 1024 && allocations[i] != old; ++i) {}
    assert(i < 1024);
    p = realloc(old, size); assert(p);
    allocations[i] = p;
    if (!old) ++live;
    return p;
}
static void *tracked_malloc(size_t size) { return tracked_realloc(NULL, size); }
static void tracked_free(void *p)
{
    unsigned i;
    if (!p) return;
    for (i = 0; i < 1024 && allocations[i] != p; ++i) {}
    assert(i < 1024 && live); allocations[i] = NULL; --live; free(p);
}
#define malloc tracked_malloc
#define realloc tracked_realloc
#define free tracked_free
#include "../src/core/owned_text.c"
#include "../src/core/text_array.c"
#include "../src/ui/chat/balloon.c"
#undef malloc
#undef realloc
#undef free
#include "balloon_text_unreached.h"

unsigned char g_useCharsetTag, g_showNameA, g_showNameB;
int g_nameFontHeight = 12;
static unsigned resized, invalidated, destroyed, drawn;
static char drawn_text[128][128];
static COLORREF drawn_colour[128];
const char *language_char_next(const char *text) { return CharNextExA(949, text, 0); }
struct ItemInfo *item_info_construct(struct ItemInfo *item) { memset(item, 0, sizeof *item); return item; }
void item_info_set_item_id(struct ItemInfo *item, int id) { (void)item; (void)id; abort(); }
const char *item_info_get_name(const struct ItemInfo *item) { (void)item; abort(); }
void *mem_alloc(size_t size) { return tracked_malloc(size); }
void mem_free(void *memory) { tracked_free(memory); }
__declspec(noreturn) void fatal_error(const char *message) { fputs(message, stderr); abort(); }

static int resources_available, gif_available, resource_token;
static struct BitmapRes monster_mark;
static unsigned filtered, resource_checks;
struct ResMgr *g_resMgr(void) { return (struct ResMgr *)&resource_token; }
void *res_mgr_get(struct ResMgr *mgr, const char *path)
{
    assert(mgr == g_resMgr());
    assert(strstr(path, "montype_boss.bmp") || strstr(path, "montype_mvp.bmp"));
    return resources_available ? &monster_mark : NULL;
}
BYTE res_mgr_exists(struct ResMgr *mgr, struct String path)
{
    BYTE found;
    assert(mgr == g_resMgr());
    ++resource_checks;
    found = resources_available && (!strstr(string_cstr(&path), ".gif") || gif_available);
    string_free(&path);
    return found;
}
int word_filter_contains(const struct WordFilter *filter, const char *text)
{ assert(filter == &g_nameFilter); ++filtered; return strstr(text, "bad") != NULL; }
void word_filter_censor(const struct WordFilter *filter, char *text)
{
    char *word;
    assert(filter == &g_nameFilter);
    while ((word = strstr(text, "bad")) != NULL) memcpy(word, "***", 3);
}
struct UIWindow *ui_window_construct(struct UIWindow *window, int composites)
{ assert(!composites); memset(window, 0, sizeof *window); return window; }
void ui_window_destruct(struct UIWindow *window) { assert(window); ++destroyed; }
void ui_window_resize_base(struct UIWindow *window, int width, int height)
{ window->width = width; window->height = height; ++resized; }
void ui_window_invalidate_base(struct UIWindow *window) { assert(window); ++invalidated; }
int ui_window_text_width(struct UIWindow *window, const char *text, int len, int face, int height, int a, int b)
{ (void)face; (void)a; (void)b; assert(window && height == 12); return len ? len : (int)strlen(text); }
int ui_window_text_height(struct UIWindow *window, const char *text, int len, int face, int height, int a, int b)
{ (void)text; (void)len; (void)face; (void)a; (void)b; assert(window && height == 12); return 12; }
int ui_window_vertical_text_length(struct UIWindow *window, const char *text, int len, int face, int height, int a)
{ (void)face; (void)a; assert(window && height == 12); return len ? len : (int)strlen(text); }
void ui_window_clear_canvas(struct UIWindow *window, DWORD colour) { assert(window); (void)colour; drawn = 0; }
void ui_window_draw_round_frame(struct UIWindow *window, int x, int y, int width, int height, COLORREF colour)
{ assert(window && !x && !y && width == window->width && height == window->height); (void)colour; }
static void draw(const char *text, COLORREF colour)
{ assert(drawn < 128); strcpy(drawn_text[drawn], text); drawn_colour[drawn++] = colour; }
void ui_window_text_out(struct UIWindow *window, int x, int y, const char *text, int len,
                      int face, int height, COLORREF colour, int a, int b)
{ (void)len; (void)face; (void)a; (void)b; assert(window && x == 6 && y >= 4 && height == 12); draw(text, colour); }
void ui_window_draw_shadow_text(struct UIWindow *window, int x, int y, const char *text, int len,
                             COLORREF colour, COLORREF outline, int face, int height, int a)
{ (void)len; (void)outline; (void)face; (void)a; assert(window && x >= 0 && y >= 0 && height == 12); draw(text, colour); }
void ui_window_text_out_runs(struct UIWindow *window, int x, int y, const char *text, int len,
                          COLORREF *colour, int face, int height, BYTE shadow)
{ (void)len; (void)face; (void)shadow; assert(window && x == 6 && y >= 4 && height == 12); draw(text, *colour); }

static void assert_line(const struct UINameBalloonText *name, size_t index, const char *expected)
{ assert(index < name->balloon.lines.size); assert(!strcmp(text_array_at(&name->balloon.lines, index).data, expected)); }

static void name_card_cases(void)
{
    struct UINameBalloonText name;
    struct ActorNameCard card = {0}, before;
    unsigned i, old_resized;
    const char *const labels[] = {"한국#hidden", "bad party with long name", "bad guild", "bad rank", "Champion", "NPC title"};
    struct String *fields[] = {&card.name, &card.party, &card.guild, &card.position,
                                 &card.achievement_title, &card.npc_title};
    for (i = 0; i < 6; ++i) {
        string_init(fields[i]);
        string_assign_cstr(fields[i], labels[i]);
    }
    before = card;
    ui_name_balloon_text_construct(&name);
    g_serviceType = 1;
    ui_name_balloon_text_set_name_text(&name, &card, 1, 0);
    assert(name.balloon.lines.size == 3 && name.balloon.base.height == 48);
    assert_line(&name, 0, "NPC title");
    assert_line(&name, 1, "[Champion] 한국 (*** party with long name)");
    assert_line(&name, 2, "*** guild [*** rank]");
    assert(name.balloon.base.width == (int)strlen("[Champion] 한국 (*** party with long name)") + 8);
    assert(filtered == 3 && !memcmp(&card, &before, sizeof card));
    for (i = 0; i < 6; ++i) assert(!strcmp(string_cstr(fields[i]), labels[i]));
    g_serviceType = 0;
    ui_name_balloon_text_set_name_text(&name, &card, 0, 0);
    assert_line(&name, 1, "[Champion] 한국#hidden (*** party with long name) ");
    assert_line(&name, 2, "*** guild (*** rank) ");
    g_showNameA = 1;
    ui_name_balloon_text_set_name_text(&name, &card, 1, 0);
    assert_line(&name, 0, "NPC title");
    assert_line(&name, 1, "*** rank");
    assert_line(&name, 2, "한국");
    for (i = 1; i < 6; ++i) string_clear(fields[i]);
    ui_name_balloon_text_set_name_text(&name, &card, 1, 0);
    assert(name.balloon.lines.size == 1 && name.balloon.base.width == 32);
    assert_line(&name, 0, "한국");
    g_showNameA = 0;
    resources_available = 1;
    monster_mark.width = 27; monster_mark.height = 43;
    ui_name_balloon_text_set_name_text(&name, &card, 1, 1);
    assert(name.mark == 1 && name.balloon.base.width == 59 && name.balloon.base.height == 43);
    ui_name_balloon_text_set_name_text(&name, &card, 1, 2);
    assert(name.mark == 2 && name.balloon.base.width == 59);
    card.group_id = 8;
    ui_name_balloon_text_set_name_text(&name, &card, 1, 2);
    assert(name.balloon.base.width == 32 && name.balloon.base.height == 20);
    card.group_id = 0; resources_available = 0; old_resized = resized;
    ui_name_balloon_text_set_name_text(&name, &card, 1, 1);
    assert(resized == old_resized);
    ui_name_balloon_text_destruct(&name);
    for (i = 0; i < 6; ++i) string_free(fields[i]);
    assert(!live);
}

static void name_orientation_cases(void)
{
    unsigned orientation;
    for (orientation = 0; orientation < 2; ++orientation) {
        struct UINameBalloonText *name;
        const struct UINameBalloonTextOps *ops;
        unsigned old_resized;
        if (orientation) {
            struct UIVerticalNameBalloonText *vertical = mem_alloc(sizeof *vertical);
            name = &vertical_name_balloon_construct(vertical)->name;
        } else {
            name = ui_name_balloon_text_construct(mem_alloc(sizeof *name));
        }
        ops = (const struct UINameBalloonTextOps *)name->balloon.base.ops;
        ops->set_text(name, "a long ground item name with 한국");
        assert(name->balloon.base.width == (orientation ? 20 : (int)strlen("a long ground item name with 한국") + 8));
        assert(name->balloon.base.height == (orientation ? (int)strlen("a long ground item name with 한국") + 8 : 20));
        ops->set_text(name, text_array_at(&name->balloon.lines, 0).data + 2);
        assert_line(name, 0, "long ground item name with 한국");
        old_resized = resized;
        ops->set_text(name, "long ground item name with 한국");
        assert(resized == old_resized);
        resources_available = 1; gif_available = 1;
        ui_name_balloon_text_set_emblem(name, "a_long_emblem_name.bmp", 0);
        assert(!strcmp(string_cstr(&name->emblem), "a_long_emblem_name.gif"));
        assert(name->balloon.base.width == name->text_length + 25);
        gif_available = 0;
        ui_name_balloon_text_set_emblem(name, "emblem.bmp", 1);
        assert(!strcmp(string_cstr(&name->emblem), "emblem.bmp"));
        assert(name->emblem_frame == 1 && name->balloon.base.width == name->text_length + 29);
        resources_available = 0; old_resized = resized;
        ui_name_balloon_text_set_emblem(name, "missing.bmp", 0);
        assert(!name->emblem.size && resized == old_resized);
        ui_name_balloon_text_set_emblem(name, "", 0);
        assert(!name->emblem.size && resized == old_resized);
        ui_name_balloon_text_set_emblem(name, "missing.bmp", 1);
        assert(!name->emblem.size && resized == old_resized + 1);
        ui_window_destroy(UI_WINDOW(&name->balloon.base));
        assert(!live);
    }
    assert(resource_checks == 16);
}
int main(void)
{
    struct UITransBalloonText balloon = {0};
    struct UINameBalloonText name = {0};
    unsigned i, before;
    assert(ui_trans_balloon_text_construct(&balloon.balloon.base) == &balloon.balloon.base);
    assert(!ui_trans_balloon_text_get_line(&balloon, 0).size);
    {
        struct UIBackdropBox expected = {-7, 13, 55, 27, 123, 0x68123456, 456}, actual;
        struct UITransBalloonText before;
        balloon.backdrop = expected;
        before = balloon;
        memset(&actual, 0, sizeof actual);
        assert(ui_window_get_backdrop_box(UI_WINDOW(&balloon.balloon.base), &actual) == 1);
        assert(!memcmp(&actual, &expected, sizeof actual));
        assert(!memcmp(&balloon, &before, sizeof balloon));
    }
    for (i = 0; i < 100; ++i) {
        ui_balloon_text_set_text(&balloon.balloon, "한국abcdef", 4);
        assert(balloon.balloon.lines.size == 3);
        assert(!strcmp(ui_trans_balloon_text_get_line(&balloon, 0).data, "한국"));
        assert(!ui_trans_balloon_text_get_line(&balloon, 3).size);
        assert(!ui_trans_balloon_text_get_line(&balloon, (size_t)-1).size);
        ui_balloon_text_on_draw((struct UIWindow *)(&balloon.balloon));
        assert(drawn == 3 && !strcmp(drawn_text[0], "한국") && !strcmp(drawn_text[2], "ef"));
        ui_trans_balloon_text_set_first_line(&balloon, ui_trans_balloon_text_get_line(&balloon, 1).data + 1, 0x123456);
        assert(balloon.balloon.lines.size == 1 && !strcmp(ui_trans_balloon_text_get_line(&balloon, 0).data, "bcd"));
        ui_trans_balloon_text_add_colored_line(&balloon, "second", 0);
        ui_trans_balloon_text_on_draw((struct UIWindow *)(&balloon));
        assert(drawn == 2 && drawn_colour[0] == 0x123456 && drawn_colour[1] == balloon.balloon.colour);
        ui_trans_balloon_text_set_line(&balloon, "last", -1);
        ui_trans_balloon_text_set_line(&balloon, "bad", 2);
        ui_trans_balloon_text_set_line(&balloon, "bad", -2);
        assert(!strcmp(ui_trans_balloon_text_get_line(&balloon, 1).data, "last"));
        ui_trans_balloon_text_set_line(&balloon, ui_trans_balloon_text_get_line(&balloon, 1).data + 1, 1);
        assert(!strcmp(ui_trans_balloon_text_get_line(&balloon, 1).data, "ast"));
        before = resized;
        ui_balloon_text_set_lines(&balloon.balloon, ui_trans_balloon_text_get_line(&balloon, 0).data, 1);
        assert(resized == before && balloon.balloon.lines.size == 3);
        ui_balloon_text_set_text(&balloon.balloon, "", 10);
        assert(resized == before && balloon.balloon.lines.size == 3);
    }
    ui_trans_balloon_text_set_first_line(&balloon, "row", 1);
    for (i = 0; i < 63; ++i) ui_trans_balloon_text_add_colored_line(&balloon, "row", i + 2);
    assert(balloon.line_colours.size == 64 && balloon.line_colours.items[63] == 64);
    ui_trans_balloon_text_on_draw((struct UIWindow *)(&balloon)); assert(drawn == 64 && drawn_colour[63] == 64);
    ui_trans_balloon_text_destruct(&balloon);
    assert(!live && destroyed == 1);
    ui_name_balloon_text_construct(&name);
    ui_name_balloon_text_set_text(&name, "abcdef");
    ui_name_balloon_text_set_text(&name, text_array_at(&name.balloon.lines, 0).data + 1);
    assert(!strcmp(text_array_at(&name.balloon.lines, 0).data, "bcdef"));
    before = resized; ui_name_balloon_text_set_text(&name, "bcdef"); assert(resized == before);
    ui_name_balloon_text_destruct(&name);
    assert(!live && destroyed == 2 && invalidated);
    name_card_cases();
    name_orientation_cases();
    puts("Balloon text: constructors, drawing, aliasing, bounds, name-card formatting, both orientations, emblems and cleanup passed.");
    return 0;
}
