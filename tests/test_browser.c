#include "browser/browser.h"
#include "browser/browser_address.h"
#include "contents/mouse.h"
#include "ui/window_access.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

struct Session g_session1;
struct UIWindowMgr g_windowMgr;
struct Mouse g_mouse;
HWND g_hMainWnd = (HWND)1;
const char *g_windowClassName = "browser test";
static int heap_live, wide_live, fail_malloc, fail_bstr, fail_conversion;
static int navigations, queued, authorised, messages, stopped, quit, released, destroyed;
static HRESULT navigate_status, location_status;
static int location_on_failure;
static const WCHAR *location_text;
static WCHAR last_address[32768];
static char last_text[32768];
static long measured_width = 512, measured_height = 384;
static struct Renderer renderer;
struct Renderer *g_renderer = &renderer;
static struct WebBrowser *popup_browser;
static int popup_requests;

struct Allocation { size_t size; unsigned magic; };
static void *owned_alloc(size_t size)
{
    struct Allocation *block;
    if (fail_malloc) { fail_malloc = 0; return NULL; }
    block = malloc(sizeof *block + size + 8);
    assert(block);
    block->size = size; block->magic = 0xb0a05e12;
    memset((char *)(block + 1) + size, 0x5a, 8);
    ++heap_live;
    return block + 1;
}
static void *owned_calloc(size_t count, size_t size)
{
    void *memory = owned_alloc(count * size);
    if (memory) memset(memory, 0, count * size);
    return memory;
}
static void owned_free(void *memory)
{
    struct Allocation *block;
    size_t i;
    if (!memory) return;
    block = (struct Allocation *)memory - 1;
    assert(block->magic == 0xb0a05e12 && heap_live > 0);
    for (i = 0; i < 8; ++i) assert(((unsigned char *)memory)[block->size + i] == 0x5a);
    block->magic = 0; --heap_live; free(block);
}
static BSTR owned_bstr(const WCHAR *text, UINT count)
{
    BSTR value;
    if (fail_bstr) { fail_bstr = 0; return NULL; }
    value = SysAllocStringLen(text, count);
    if (value) ++wide_live;
    return value;
}
static void owned_bstr_free(BSTR value)
{
    if (value) { assert(wide_live > 0); --wide_live; SysFreeString(value); }
}
static int to_wide(UINT cp, DWORD flags, const char *text, int size, WCHAR *out, int count)
{
    if (out && fail_conversion == 1) { fail_conversion = 0; return 0; }
    return MultiByteToWideChar(cp, flags, text, size, out, count);
}
static int to_text(UINT cp, DWORD flags, const WCHAR *text, int size, char *out, int count,
                   const char *fallback, BOOL *used)
{
    if (out && fail_conversion == 2) { fail_conversion = 0; return 0; }
    return WideCharToMultiByte(cp, flags, text, size, out, count, fallback, used);
}
static BOOL show_window(HWND hwnd, int show) { (void)hwnd; (void)show; return TRUE; }
static BOOL visible(HWND hwnd) { (void)hwnd; return TRUE; }
static BOOL set_text(HWND hwnd, const char *text)
{
    (void)hwnd; assert(strlen(text) < sizeof last_text); strcpy(last_text, text); return TRUE;
}
static BOOL cursor(POINT *point) { point->x = point->y = 0; return TRUE; }
static HWND window_at(POINT point) { (void)point; return g_hMainWnd; }
static LONG window_long(HWND hwnd, int index, LONG value)
{ (void)hwnd; (void)index; (void)value; return 0; }
static HWND set_parent(HWND hwnd, HWND parent) { (void)hwnd; (void)parent; return NULL; }
static BOOL destroy_window(HWND hwnd) { assert(hwnd); ++destroyed; return TRUE; }

#define malloc owned_alloc
#define calloc owned_calloc
#define free owned_free
#define SysAllocStringLen owned_bstr
#define SysFreeString owned_bstr_free
#define MultiByteToWideChar to_wide
#define WideCharToMultiByte to_text
#define ShowWindow show_window
#define IsWindowVisible visible
#define SetWindowTextA set_text
#define GetCursorPos cursor
#define WindowFromPoint window_at
#define SetWindowLongA window_long
#define SetParent set_parent
#define DestroyWindow destroy_window
#include "../src/browser/browser_address.c"
#include "../src/browser/web_browser.c"
#include "../src/browser/web_sites.c"
#undef malloc
#undef calloc
#undef free
#undef SysAllocStringLen
#undef SysFreeString
#undef MultiByteToWideChar
#undef WideCharToMultiByte

void mouse_set_x(struct Mouse *self, int x) { self->x = x; }
void mouse_set_y(struct Mouse *self, int y) { self->y = y; }
void mouse_reset_buttons(struct Mouse *self) { (void)self; }
void mouse_center(struct Mouse *self) { (void)self; }
void app_activate(int active) { (void)active; }
struct WebBrowser *ui_window_mgr_new_web_browser(struct UIWindowMgr *mgr, int width, int height)
{
    assert(mgr == &g_windowMgr && width == 700 && height == 560);
    ++popup_requests;
    return popup_browser;
}
void *mem_alloc(size_t bytes) { (void)bytes; abort(); }
void mem_free(void *memory)
{ (void)memory;  abort(); }
struct WebBrowser *ui_window_mgr_find_web_browser_by_hwnd(struct UIWindowMgr *mgr, HWND hwnd)
{ (void)mgr; (void)hwnd; abort(); }
void ui_window_mgr_delete_web_browser(struct UIWindowMgr *mgr, HWND hwnd)
{ (void)mgr; (void)hwnd; abort(); }
struct UIWindow *ui_window_find(int id) { assert(id == 0x11a); return (struct UIWindow *)1; }
void ui_connect_twitter_wnd_set_authorised(struct UIConnectTwitterWnd *wnd, BYTE ok)
{ assert(wnd == (struct UIConnectTwitterWnd *)1); authorised = ok; }
void ui_window_mgr_queue_delete_web_browser(struct UIWindowMgr *mgr, HWND hwnd)
{ assert(mgr == &g_windowMgr && hwnd == (HWND)2); ++queued; }
void ui_window_mgr_modeless_message_box(struct UIWindowMgr *mgr, const char *text, int a, int b, int d, const char *title, int w, int h, const char *e)
{
    (void)a; (void)b; (void)d; (void)title; (void)w; (void)h; (void)e;
    assert(mgr == &g_windowMgr && !strcmp(text, "sns auth failed")); ++messages;
}
int ui_window_mgr_send_msg_(struct UIWindowMgr *mgr, int msg, int a, int b, int c, int d)
{ (void)msg; (void)a; (void)b; (void)d; assert(mgr == &g_windowMgr); ++messages; return 0; }
char *msg_str(int id) { assert(id == 0xca1); return "unexpected page"; }

static HRESULT STDMETHODCALLTYPE navigate(IWebBrowser2 *self, BSTR url, VARIANT *a,
                                          VARIANT *b, VARIANT *c, VARIANT *d)
{
    (void)self;
    assert(url && a->vt == VT_EMPTY && b->vt == VT_EMPTY && c->vt == VT_EMPTY && d->vt == VT_EMPTY);
    assert(wcslen(url) < sizeof last_address / sizeof *last_address);
    wcscpy(last_address, url); ++navigations; return navigate_status;
}
static HRESULT STDMETHODCALLTYPE get_width(IWebBrowser2 *self, long *out)
{ (void)self; *out = measured_width; return S_OK; }
static HRESULT STDMETHODCALLTYPE get_height(IWebBrowser2 *self, long *out)
{ (void)self; *out = measured_height; return S_OK; }
static HRESULT STDMETHODCALLTYPE put_size(IWebBrowser2 *self, long value)
{ (void)self; assert(value > 0); return S_OK; }
static HRESULT STDMETHODCALLTYPE put_visible(IWebBrowser2 *self, VARIANT_BOOL value)
{ (void)self; assert(value == VARIANT_TRUE); return S_OK; }
static HRESULT STDMETHODCALLTYPE get_location(IWebBrowser2 *self, BSTR *out)
{
    (void)self;
    assert(*out == NULL);
    if (location_text && (SUCCEEDED(location_status) || location_on_failure))
        *out = owned_bstr(location_text, (UINT)wcslen(location_text));
    return location_status;
}
static HRESULT STDMETHODCALLTYPE stop(IWebBrowser2 *self) { (void)self; ++stopped; return S_OK; }
static HRESULT STDMETHODCALLTYPE quit_browser(IWebBrowser2 *self) { (void)self; ++quit; return S_OK; }
static ULONG STDMETHODCALLTYPE release(IWebBrowser2 *self) { (void)self; ++released; return 0; }
static IWebBrowser2Vtbl methods = {
    .Release = release, .Navigate = navigate, .Stop = stop, .Quit = quit_browser,
    .get_Width = get_width, .get_Height = get_height, .put_Width = put_size,
    .put_Height = put_size, .put_Visible = put_visible, .get_LocationURL = get_location
};
static IWebBrowser2 browser = {&methods};

static void addresses(void)
{
    char *text, *prefix = (char *)1;
    BSTR wide;
    WCHAR expected[64];
    assert(!browser_address_to_wide(NULL));
    assert(!browser_address_to_text(NULL, CP_ACP));
    text = browser_address_to_text(L"https://한글/주소", 949);
    assert(text && !strcmp(text, "https://한글/주소")); owned_free(text);
    wide = browser_address_to_wide("https://한글/주소");
    assert(MultiByteToWideChar(CP_ACP, 0, "https://한글/주소", -1, expected, 64));
    assert(wide && !wcscmp(wide, expected)); owned_bstr_free(wide);
    wide = browser_address_to_wide(""); assert(wide && !*wide); owned_bstr_free(wide);
    fail_conversion = 1; assert(!browser_address_to_wide("url") && !wide_live);
    fail_conversion = 2; assert(!browser_address_to_text(L"url", CP_ACP) && !heap_live);
    fail_bstr = 1; assert(!browser_address_to_wide("url"));
    fail_malloc = 1; assert(!browser_address_to_text(L"url", CP_ACP));
    fail_malloc = 1; assert(!browser_address_prefix("server:3000", &prefix) && prefix == (char *)1);
    assert(browser_address_prefix("server:3000:3000", &prefix) && !strcmp(prefix, "server")); owned_free(prefix);
    assert(browser_address_prefix(":3000", &prefix) && !prefix);
    assert(browser_address_prefix("server", &prefix) && !prefix);
    assert(browser_address_auth_status("server-x server:3000/success", "server") == BROWSER_AUTH_SUCCESS);
    assert(browser_address_auth_status("server:3000/fail", "server") == BROWSER_AUTH_FAILED);
    assert(browser_address_auth_status("api.twitter.com/oauth/authenticate?a=1", "server") == BROWSER_AUTH_PENDING);
    assert(browser_address_auth_status("elsewhere", "server") == BROWSER_AUTH_UNEXPECTED);
    assert(browser_address_auth_status("", "server") == BROWSER_AUTH_PENDING);
}

static void popups(void)
{
    struct WebBrowser owner;
    struct BrowserSite site = {0};
    VARIANT args[5] = {{0}};
    DISPPARAMS params = {args, NULL, 5, 0};
    VARIANT_BOOL cancel = VARIANT_FALSE;
    char expected[64];
    int before;
    renderer.width = 1000; renderer.height = 800;
    assert(web_dispatch_invoke(&site.dispatch, 0, NULL, 0, 0, NULL, NULL, NULL, NULL) == S_OK);
    assert(web_dispatch_invoke(&site.dispatch, DISPID_NEWWINDOW3, NULL, 0, 0, NULL, NULL, NULL, NULL) == DISP_E_BADPARAMCOUNT);
    params.cArgs = 4;
    assert(web_dispatch_invoke(&site.dispatch, DISPID_NEWWINDOW3, NULL, 0, 0, &params, NULL, NULL, NULL) == DISP_E_BADPARAMCOUNT);
    params.cArgs = 5;
    assert(web_dispatch_invoke(&site.dispatch, DISPID_NEWWINDOW3, NULL, 0, 0, &params, NULL, NULL, NULL) == DISP_E_TYPEMISMATCH);
    args[0].vt = VT_BSTR; args[0].bstrVal = owned_bstr(L"https://한글/주소", 13);
    args[3].vt = VT_BYREF | VT_BOOL;
    assert(web_dispatch_invoke(&site.dispatch, DISPID_NEWWINDOW3, NULL, 0, 0, &params, NULL, NULL, NULL) == DISP_E_TYPEMISMATCH);
    args[3].pboolVal = &cancel;
    fail_malloc = 1;
    assert(web_dispatch_invoke(&site.dispatch, DISPID_NEWWINDOW3, NULL, 0, 0, &params, NULL, NULL, NULL) == S_OK);
    assert(cancel == VARIANT_TRUE && !popup_requests && !heap_live);
    fail_conversion = 2;
    assert(web_dispatch_invoke(&site.dispatch, DISPID_NEWWINDOW3, NULL, 0, 0, &params, NULL, NULL, NULL) == S_OK);
    assert(!popup_requests && !heap_live);
    assert(web_dispatch_invoke(&site.dispatch, DISPID_NEWWINDOW3, NULL, 0, 0, &params, NULL, NULL, NULL) == S_OK);
    assert(popup_requests == 1 && !heap_live);
    memset(&owner, 0, sizeof owner); owner.browser = &browser; owner.address_combo = (HWND)3;
    popup_browser = &owner; before = navigations;
    assert(web_dispatch_invoke(&site.dispatch, DISPID_NEWWINDOW3, NULL, 0, 0, &params, NULL, NULL, NULL) == S_OK);
    assert(popup_requests == 2 && navigations == before + 1 && !heap_live);
    assert(WideCharToMultiByte(CP_ACP, 0, args[0].bstrVal, -1, expected, sizeof expected, NULL, NULL));
    assert(!strcmp(last_text, expected));
    popup_browser = NULL;
    owned_bstr_free(args[0].bstrVal);
    assert(!wide_live);
}

int main(void)
{
    struct WebBrowser owner;
    char url[8192];
    int i, calls;
    addresses();
    popups();
    assert(web_browser_construct(&owner) == &owner && !owner.owner && !owner.auth_url);
    owner.browser = &browser; owner.hwnd = (HWND)2; owner.address_combo = (HWND)3;
    owner.width = measured_width; owner.height = measured_height;
    strcpy(url, "https://server:3000/"); memset(url + strlen(url), 'x', sizeof url - strlen(url) - 1); url[sizeof url - 1] = 0;
    for (i = 0; i < 100; ++i) {
        assert(web_browser_navigate(&owner, url));
        assert(!strcmp(owner.auth_url, "https://server") && !strcmp(last_text, url));
        assert(wcslen(last_address) == strlen(url) && heap_live == 1 && !wide_live);
    }
    calls = navigations;
    fail_malloc = 1; assert(!web_browser_navigate(&owner, "new:3000") && navigations == calls);
    fail_bstr = 1; assert(!web_browser_navigate(&owner, "new:3000") && navigations == calls);
    assert(!strcmp(owner.auth_url, "https://server") && heap_live == 1 && !wide_live);
    assert(!web_browser_navigate(NULL, url) && !web_browser_navigate(&owner, NULL));
    navigate_status = E_FAIL; assert(!web_browser_navigate(&owner, "https://server:3000")); navigate_status = S_OK;
    location_text = L"https://server:3000/success";
    for (i = 0; i < 1000; ++i) web_browser_update(&owner);
    assert(queued == 1000 && authorised == 1 && g_session1.snsAuthorised == 1 && !wide_live && heap_live == 1);
    location_status = E_FAIL; measured_width = 0x123456;
    web_browser_update(&owner); assert(queued == 1000 && !wide_live);
    location_on_failure = 1; web_browser_update(&owner); assert(queued == 1000 && !wide_live);
    location_status = S_OK; location_on_failure = 0; measured_width = owner.width;
    fail_malloc = 1; web_browser_update(&owner); assert(queued == 1000 && !wide_live && heap_live == 1);
    location_text = L"https://server:3000/fail"; web_browser_update(&owner);
    assert(queued == 1001 && !authorised && !g_session1.snsAuthorised && messages == 1);
    location_text = L"https://api.twitter.com/oauth/authenticate"; web_browser_update(&owner); assert(queued == 1001);
    location_text = L"https://elsewhere"; web_browser_update(&owner); assert(queued == 1002 && messages == 2);
    web_browser_destruct(&owner); web_browser_destruct(&owner); web_browser_destruct(NULL);
    assert(!heap_live && !wide_live && stopped == 1 && quit == 1 && released == 1 && destroyed == 1);
    puts("Browser popups: argument validation, canonical cancellation, Korean conversion, allocation failures and borrowed navigation passed");
    puts("Browser: borrowed URLs, Korean conversion, guarded C storage, BSTR cleanup, failed preparation, failed polling, auth results and repeat destruction passed");
    return 0;
}
