#include "ui/wndmgr/wndmgr.h"
#include "world_state_memory.h"
#include <stdint.h>
#include <stdio.h>

struct UIWindowMgr g_windowMgr;
struct Session g_session1;
HWND g_hMainWnd = (HWND)1;
HMODULE g_hInstance = (HMODULE)2;
static void *owned[16];
static int heap_live, fail_alloc, fail_create, constructions, destructions, messages;
static unsigned next_window = 100;

static void *browser_alloc(size_t size)
{
    unsigned i;
    unsigned char *memory;
    assert(size == sizeof(struct WebBrowser));
    if (fail_alloc) { fail_alloc = 0; return NULL; }
    memory = malloc(size + 16); assert(memory);
    memset(memory, 0xa5, size + 16);
    for (i = 0; i < 16 && owned[i]; ++i) {}
    assert(i < 16); owned[i] = memory; ++heap_live;
    return memory;
}
static void browser_free(void *memory)
{
    unsigned i, j;
    if (!memory) return;
    for (i = 0; i < 16 && owned[i] != memory; ++i) {}
    assert(i < 16);
    for (j = 0; j < 16; ++j) assert(((unsigned char *)memory)[sizeof(struct WebBrowser) + j] == 0xa5);
    owned[i] = NULL; --heap_live; free(memory);
}
#define malloc browser_alloc
#define free browser_free
#include "../src/ui/wndmgr/wndmgr_web_browser.c"
#undef malloc
#undef free

struct WebBrowser *web_browser_construct(struct WebBrowser *self)
{ memset(self, 0, sizeof *self); ++constructions; return self; }
int web_browser_create(struct WebBrowser *self, HWND parent, HINSTANCE instance, int w, int h)
{
    assert(parent == g_hMainWnd && instance == g_hInstance && w == 640 && h == 480);
    self->owner = parent; self->hwnd = (HWND)(uintptr_t)next_window++;
    return !fail_create;
}
void web_browser_destruct(struct WebBrowser *self)
{
    assert(self && g_windowMgr.browser != self && g_windowMgr.active_web_browser != self);
    ++destructions;
}
char *msg_str(int id) { assert(id == 0x62a); return "browser limit"; }
int ui_window_mgr_send_msg_(struct UIWindowMgr *mgr, int msg, int a, int b, int c, int d)
{ (void)msg; (void)a; (void)b; (void)c; (void)d; assert(mgr == &g_windowMgr); ++messages; return 0; }
void ui_window_mgr_destroy_window(struct UIWindowMgr *mgr, struct UIWindow *wnd)
{ (void)mgr; (void)wnd; abort(); }

int main(void)
{
    struct WebBrowser *login, *other;
    HWND hwnd;
    int destroyed, i;
    tree_init(&g_windowMgr.browsers);
    tree_init(&g_windowMgr.windows_by_id);
    tree_init(&g_windowMgr.snap_rects);
    list_init(&g_windowMgr.windows); list_init(&g_windowMgr.quit_list);
    list_init(&g_windowMgr.plates); list_init(&g_windowMgr.hidden_wnds);

    fail_alloc = 1;
    assert(!ui_window_mgr_new_web_browser(&g_windowMgr, 640, 480) && !heap_live && !constructions);
    fail_create = 1;
    assert(!ui_window_mgr_new_web_browser(&g_windowMgr, 640, 480) && !heap_live);
    assert(!ui_window_mgr_get_login_browser(&g_windowMgr, 640, 480) && !g_windowMgr.browser && !heap_live);
    fail_create = 0;
    login = ui_window_mgr_get_login_browser(&g_windowMgr, 640, 480);
    assert(login && ui_window_mgr_get_login_browser(&g_windowMgr, 640, 480) == login && heap_live == 1);
    assert(ui_window_mgr_find_web_browser_by_hwnd(&g_windowMgr, login->hwnd) == login);
    assert(ui_window_mgr_find_web_browser(&g_windowMgr, g_hMainWnd) == login);
    assert(!ui_window_mgr_add_web_browser(&g_windowMgr, login->hwnd, login));
    other = ui_window_mgr_new_web_browser(&g_windowMgr, 640, 480); assert(other && heap_live == 2);
    g_windowMgr.active_web_browser = login;
    destroyed = destructions;
    ui_window_mgr_close_all_windows(&g_windowMgr);
    assert(destructions == destroyed + 2 && !heap_live && !g_windowMgr.browser && !g_windowMgr.active_web_browser);
    assert(g_windowMgr.browsers.size == 0);
    ui_window_mgr_close_all_windows(&g_windowMgr);
    assert(destructions == destroyed + 2 && !heap_live);
    ui_window_mgr_delete_web_browser(&g_windowMgr, (HWND)9999);
    login = ui_window_mgr_get_login_browser(&g_windowMgr, 640, 480); assert(login);
    hwnd = login->hwnd; g_windowMgr.active_web_browser = login;
    ui_window_mgr_delete_web_browser(&g_windowMgr, hwnd);
    assert(!heap_live && !g_windowMgr.browser && !g_windowMgr.active_web_browser && !g_windowMgr.browsers.size);
    ui_window_mgr_delete_web_browser(&g_windowMgr, hwnd);
    other = alloc_browser(); assert(other);
    other->hwnd = (HWND)999;
    add_browser(&g_windowMgr, other->hwnd, NULL);
    assert(ui_window_mgr_add_web_browser(&g_windowMgr, other->hwnd, other));
    assert(ui_window_mgr_find_web_browser_by_hwnd(&g_windowMgr, other->hwnd) == other);
    ui_window_mgr_delete_web_browser(&g_windowMgr, other->hwnd);
    assert(!heap_live && !g_windowMgr.browsers.size);
    for (i = 0; i < 5; ++i) assert(ui_window_mgr_new_web_browser(&g_windowMgr, 640, 480));
    assert(!ui_window_mgr_new_web_browser(&g_windowMgr, 640, 480) && heap_live == 5 && messages == 2);
    ui_window_mgr_close_all_windows(&g_windowMgr);
    assert(!heap_live && constructions == destructions);
    tree_free(&g_windowMgr.browsers, NULL);
    tree_free(&g_windowMgr.windows_by_id, NULL);
    tree_free(&g_windowMgr.snap_rects, NULL);
    list_free(&g_windowMgr.windows, NULL);
    list_free(&g_windowMgr.quit_list, NULL);
    list_free(&g_windowMgr.plates, NULL);
    list_free(&g_windowMgr.hidden_wnds, NULL);
    assert(!live_blocks);
    puts("Browser owners: real maps and teardown, allocation/create failures, retry, typed handles, five-window limit, shared aliases, single destruction and repeated close passed");
    return 0;
}
