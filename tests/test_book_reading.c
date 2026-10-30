#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <mmsystem.h>
#include "ui/items/book_wnd.h"
#include "ui/window_mgr.h"
#include "ui/controls/popups.h"

static DWORD now;
static DWORD test_time(void) { return now; }
#define timeGetTime test_time
#include "../src/ui/items/book_reading.c"
#undef timeGetTime

struct UIWindowMgr g_windowMgr;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
unsigned g_codePage = 949;
unsigned char g_useCharsetTag = 1;
static int fail_text, fail_balloon, live_text, live_balloon;
static int chat_calls, balloon_calls, added, closed, finished, moved;
static char chat_copy[256], balloon_copy[256];
static void *text_allocation;

static void *test_malloc(size_t n)
{
    void *p;
    if (fail_text) return NULL;
    p = malloc(n);
    assert(p && !text_allocation);
    text_allocation = p;
    ++live_text;
    return p;
}
static void test_free(void *p)
{
    if (!p) return;
    assert(p == text_allocation && live_text == 1);
    memset(p, 0xdd, strlen((char *)p));
    free(p);
    text_allocation = NULL;
    --live_text;
}
#define malloc test_malloc
#define free test_free
#include "../src/core/owned_text.c"
#include "../src/core/text_format.c"
#undef malloc
#undef free

void *mem_alloc(size_t n)
{
    void *p;
    assert(n == 0xcc);
    if (fail_balloon) return NULL;
    p = calloc(1, n);
    assert(p);
    ++live_balloon;
    return p;
}
static void balloon_move(struct UIWindow *wnd, int x, int y)
{
    assert(wnd && x == 590 && y == 0x51);
    ++moved;
}
struct UIWindow *ui_trans_balloon_text_construct(struct UIWindow *wnd)
{
    static struct UIWindowOps table;
    table.move = balloon_move;
    wnd->ops = &table;
    wnd->width = 100;
    return wnd;
}
void ui_window_create(struct UIWindow *wnd, int width, int height)
{ assert(wnd && width == 8 && height == 8); }
void ui_window_mgr_add_to_window_list(struct UIWindowMgr *mgr, struct UIWindow *wnd)
{ assert(mgr == &g_windowMgr && wnd); ++added; }
int ui_window_mgr_send_msg_(struct UIWindowMgr *mgr, int message, int a, int b, int c, int d)
{
    assert(mgr == &g_windowMgr && !c && !d);
    if (message == UIWM_MSG_CHAT_LINE) {
        assert(b == BOOK_SPEAK_COLOUR && live_text == 1);
        strcpy(chat_copy, (const char *)a);
        ++chat_calls;
    } else {
        assert(message == 3 && !a && !b);
        ++finished;
    }
    return 0;
}
void ui_balloon_text_set_text(struct UIBalloonText *wnd, const char *text, int width)
{
    assert(wnd && text == text_allocation && live_text == 1 && width == 100);
    strcpy(balloon_copy, text);
    ++balloon_calls;
}
void ui_window_mgr_post_quit2(struct UIWindowMgr *mgr, struct UIWindow *wnd)
{
    assert(mgr == &g_windowMgr && wnd && live_balloon == 1);
    free(wnd);
    --live_balloon;
}
BYTE ui_window_mgr_post_quit1(struct UIWindowMgr *mgr, int id)
{ assert(mgr == &g_windowMgr && id == BOOK_WND_ID); ++closed; return 1; }

int main(void)
{
    struct UIBookWnd book = {0};
    struct OwnedText lines[] = {
        {(char *)" ", 1}, {(char *)"|c0앞^123456뒤", sizeof "|c0앞^123456뒤" - 1}, {(char *)"last", 4}
    };
    renderer.width = 1280;
    book.lines.items = lines;
    book.lines.size = book.lines.capacity = 3;
    assert(!strcmp(book_line(&book, -1), "") && !strcmp(book_line(&book, 3), ""));
    book.line_count = 3;
    book.speak_time = 100;
    now = 5000;
    ui_book_wnd_on_process((struct UIWindow *)(&book));
    assert(!chat_calls);
    book.speaking = 1;
    now = 4100;
    ui_book_wnd_on_process((struct UIWindow *)(&book));
    assert(!chat_calls);
    now = 5000;
    fail_text = 1;
    ui_book_wnd_on_process((struct UIWindow *)(&book));
    assert(book.speak_line == 1 && book.speak_time == 100 && !chat_calls && !live_text);
    fail_text = 0;
    fail_balloon = 1;
    ui_book_wnd_on_process((struct UIWindow *)(&book));
    assert(chat_calls == 1 && !balloon_calls && !book.balloon && !live_text && !live_balloon);
    fail_balloon = 0;
    ui_book_wnd_on_process((struct UIWindow *)(&book));
    assert(chat_calls == 2 && balloon_calls == 1 && added == 1 && moved == 1);
    assert(book.speak_line == 2 && book.speak_time == now && live_text == 0 && live_balloon == 1);
    assert(strcmp(chat_copy, "|c0|c0앞|c0뒤") == 0 && strcmp(chat_copy, balloon_copy) == 0);
    now += 4001;
    ui_book_wnd_on_process((struct UIWindow *)(&book));
    assert(book.speak_line == 3 && chat_calls == 3 && balloon_calls == 2 && added == 1);
    assert(strcmp(chat_copy, "last") == 0 && strcmp(balloon_copy, "last") == 0 && !live_text);
    now += 4001;
    ui_book_wnd_on_process((struct UIWindow *)(&book));
    assert(!book.balloon && !book.speak_time && closed == 1 && !live_balloon && finished == 3);
    ui_book_wnd_on_process((struct UIWindow *)(&book));
    assert(closed == 1 && !live_text);
    puts("Book reading: actual processing, text ownership, allocation failures, synchronous consumers, timer/blank-line behavior and cleanup passed.");
    return 0;
}
