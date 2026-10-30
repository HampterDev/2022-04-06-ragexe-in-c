#include "ui/window_access.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/items/book_wnd.h"
#include "ui/controls/controls.h"
#include "ui/window_mgr.h"
#include "ui/game.h"
#include "platform/language.h"

static void *allocations[128];
static size_t lengths[128];
static unsigned live, fail_allocation;
static void *tracked_malloc(size_t size)
{
    unsigned i;
    void *p;
    if (fail_allocation) return NULL;
    p = malloc(size); assert(p);
    for (i = 0; i < 128 && allocations[i]; ++i) {}
    assert(i < 128); allocations[i] = p; lengths[i] = size; ++live;
    return p;
}
static void tracked_free(void *p)
{
    unsigned i;
    if (!p) return;
    for (i = 0; i < 128 && allocations[i] != p; ++i) {}
    assert(i < 128 && live);
    memset(p, 0xdd, lengths[i]); free(p); allocations[i] = NULL; --live;
}
#define malloc tracked_malloc
#define free tracked_free
#include "../src/core/owned_text.c"
#include "../src/ui/items/book_wnd.c"
#undef malloc
#undef free
#include "book_text_unreached.h"

struct UIWindowMgr g_windowMgr;
struct Session g_session1;
struct SkinMgr g_skinMgr;
struct Language *g_language;
const char *g_registryKeyPath = "unused test registry path";
int g_serviceType;
unsigned char g_useCharsetTag;
static const char *file_text;
static int fail_read, closed, destroyed, clear_count, shown, hidden, quits, raised, add_count;
static char added[20][128];

const char *session_character_name(const struct Session *session) { (void)session; return "test"; }
const struct ItemTableEntry *item_table_get(int id)
{ static const struct ItemTableEntry item = {.name = "Test book"}; assert(id == 1); return &item; }
const char *language_language_file(struct Language *language, const char *path)
{ (void)language; return path; }
const char *language_charset_tag(struct Language *language, unsigned char folder)
{ (void)language; assert(folder == 1); return "|c0"; }
const char *language_char_next(const char *text) { return CharNextExA(949, text, 0); }
int ui_window_text_width(struct UIWindow *window, const char *text, int length,
                        int face, int height, int a, int b)
{ assert(window && !face && height == 12 && !a && !b); return length ? length : (int)strlen(text); }
int crt_sprintf(char *out, const char *format, ...)
{ int n; va_list args; va_start(args, format); n = vsprintf(out, format, args); va_end(args); return n; }
struct File *file_construct(struct File *file) { memset(file, 0, sizeof *file); return file; }
unsigned char file_open(struct File *file, const char *path, int mode,
                          unsigned char resolve, unsigned char disk_only)
{
    assert(!strcmp(path, "book\\1.txt") && !mode && resolve && !disk_only);
    file->size = (int)strlen(file_text); return 1;
}
unsigned char file_read(struct File *file, void *out, int size)
{ assert(size == file->size); if (fail_read) return 0; memcpy(out, file_text, size); return 1; }
void file_close(struct File *file) { assert(file); ++closed; }
void file_destruct(struct File *file) { assert(file); ++destroyed; }
void ui_skin_color_chip(struct SkinMgr *skin, int x, int y, int *r, int *g, int *b)
{ assert(skin == &g_skinMgr && x == 2 && y == 2); *r = 1; *g = 2; *b = 3; }
void ui_window_mgr_post_quit2(struct UIWindowMgr *mgr, struct UIWindow *window)
{ (void)mgr; (void)window; abort(); }
void ui_window_mgr_raise_window(struct UIWindowMgr *mgr, struct UIWindow *window)
{ assert(mgr == &g_windowMgr && window); ++raised; }
void ui_window_set_show_base(struct UIWindow *window, int visible) { assert(window); if (visible) ++shown; else ++hidden; }
void ui_window_invalidate_base(struct UIWindow *window) { assert(window); }
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *mgr, int id) { assert(mgr == &g_windowMgr && id); ++quits; return 1; }
static void clear(struct UIWindow *window) { assert(window); ++clear_count; add_count = 0; }
static int add(struct UIWindow *window, struct UIWindow *sender, int msg, int a, int b, int c, int d)
{
    assert(window && !sender && msg == UI_MSG_ADD_TEXT && b == 100 && !c && !d && add_count < 20);
    strcpy(added[add_count++], (const char *)a); return 0;
}
int main(void)
{
    struct UIBookWnd book = {0};
    struct UIWindow viewer = {0};
    struct {
        struct UIWindowOps base;
        void (*create)(void);
        void (*add_item)(void);
        void (*clear)(struct UIWindow *);
    } viewer_table = {{.send_msg = add}, NULL, NULL, clear};
    unsigned i;
    viewer.ops = &viewer_table.base;
    viewer.width = 200; book.text = &viewer;
    file_text = "%123456\r\n한국\nlast";
    for (i = 0; i < 50; ++i) {
        book_open(&book, "1", 0, 1);
        assert(book.lines.size == 2 && book.line_count == 2 && book.page_count == 1 && live == 2);
        assert(book.red == 0x12 && book.green == 0x34 && book.blue == 0x56);
        assert(!strcmp(book_line(&book, 0), "한국") && !strcmp(book_line(&book, 1), "last"));
        g_useCharsetTag = 1; book_goto(&book, 1);
        assert(add_count == 2 && !strcmp(added[0], "|c0한국") && !strcmp(added[1], "|c0last") && live == 2);
        g_useCharsetTag = 0; book_goto(&book, 1);
        assert(add_count == 2 && !strcmp(added[0], "한국") && !strcmp(added[1], "last") && live == 2);
    }
    assert(closed == 50 && destroyed == 50 && hidden == 50 && !quits && clear_count == 100 && shown == 100 && raised == 100);
    fail_allocation = 1;
    book_open(&book, "1", 0, 1); assert(live == 2 && hidden == 50 && closed == 51 && destroyed == 51);
    g_useCharsetTag = 1; book_goto(&book, 1); assert(!add_count && live == 2);
    fail_allocation = 0; fail_read = 1;
    book_open(&book, "1", 0, 1); assert(live == 2 && hidden == 50 && closed == 52 && destroyed == 52);
    fail_read = 0; file_text = "%";
    book_open(&book, "1", 0, 1);
    assert(live == 1 && book.red == 1 && book.green == 2 && book.blue == 3 && !strcmp(book_line(&book, 0), "%"));
    text_array_clear(&book.lines); assert(!live);
    puts("Book text: repeated file loading, native lines, header removal, page rendering, temporary cleanup and allocation/read failures passed.");
    return 0;
}
